// this is an old version of Synthesizer, before the file was split into a handful smaller classes. It should be safely removable

#include "MidiFile.h"
#include "SftFile.h"

#include <array>
#include <vector>
#include <set>
#include <algorithm>
#include <mutex>
#include <functional>

class Synthesizer
{
private:
	inline static std::array<float, 255> semitoneExponents;
	inline static std::array<float, 128> velToAtt;
	inline static bool initializedLUTs = false;
	std::mutex activeNotesMutex;
public:
	struct ActiveNote
	{
		float elapsedTimeAtStart;
		float midiElapsedTimeAtStart;
		unsigned int endTime;
		unsigned char channel;
		unsigned char key;
		unsigned char velocity;
	};
	std::function<void(const std::string&)> doLyricEvent{ [](const std::string& lyric) {} };
private:

	class WavetableOscillator
	{
	private:
		std::array<unsigned int, 16> channelVolumes;
		std::array<unsigned int, 16> channelExpressions;
		std::array<float, 16> channelAttenuationFactors;
		std::array<unsigned int, 16> channelPitchBends;
		std::array<float, 16> channelPitchBendFactors;
		
		static constexpr unsigned int channelVolumeDefaultValue{ 100 };
		static constexpr unsigned int channelExpressionDefaultValue{ 127 };
		static constexpr unsigned int channelPitchBendDefaultValue{ 0x2000 };
	private:
		void resetChannelVolumeAndExpression(const unsigned int channel)
		{
			this->channelVolumes[channel] = channelVolumeDefaultValue;
			this->channelExpressions[channel] = channelExpressionDefaultValue;
			this->updateChannelAttenuationFactor(channel);
		}
		void resetChannelPitchBend(const unsigned int channel)
		{
			this->setChannelPitchBend(channel, channelPitchBendDefaultValue);
		}
	public:
		void resetAllControllers()
		{
			for (unsigned int channel{ 0 }; channel < 16; ++channel)
			{
				this->resetChannelVolumeAndExpression(channel);
				this->resetChannelPitchBend(channel);
			}
		}
		WavetableOscillator()
		{
			this->resetAllControllers();
		}
		struct Sample
		{
			unsigned int channel;
			unsigned int key;
			unsigned int velocity;
			unsigned int midiEndTime;
			double dataPoint;
			const SoundfontFile::Bag* presetBag;
			const SoundfontFile::Bag* instrumentBag;
			const SoundfontFile::SampleHeader* sample;
			enum class EnvPhase
			{
				None,
				Delay,
				Attack,
				Hold,
				Decay,
				Sustain,
				Release,
				Finished
			};
			EnvPhase volEnvPhase{ EnvPhase::None };
			double volEnvPhaseStartTime{  };
			float initialPhaseVolEnv{  };

			unsigned int startAddress;
			unsigned int endAddress;
			unsigned int startloopAddress;
			unsigned int endloopAddress;

			float attenuationFactor;

			float volEnvDelayTime;
			float volEnvAttackTime;
			float volEnvHoldTime;
			float volEnvDecayTime;
			float volEnvReleaseTime;

			double rootPitch;

			Sample(const unsigned int channel, const unsigned int key, const unsigned int velocity, const unsigned int midiEndTime, const SoundfontFile::SampleHeader& sample, const SoundfontFile::Bag& presetBag, const SoundfontFile::Bag& instrumentBag)
				:channel(channel), key(key), velocity(velocity), midiEndTime(midiEndTime), dataPoint(sample.start), sample(&sample), presetBag(&presetBag), instrumentBag(&instrumentBag)
			{
				startAddress = this->sample->start + this->getInstrumentGeneratorValue<int>(SoundfontFile::Bag::GeneratorTypes::startAddrsOffset) + 32768 * this->getInstrumentGeneratorValue<int>(SoundfontFile::Bag::GeneratorTypes::startAddrsCoarseOffset);
				endAddress = this->sample->end + this->getInstrumentGeneratorValue<int>(SoundfontFile::Bag::GeneratorTypes::endAddrsOffset) + 32768 * this->getInstrumentGeneratorValue<int>(SoundfontFile::Bag::GeneratorTypes::endAddrsCoarseOffset);
				startloopAddress = this->sample->loopStart + this->getInstrumentGeneratorValue<int>(SoundfontFile::Bag::GeneratorTypes::startloopAddrsOffset) + 32768 * this->getInstrumentGeneratorValue<int>(SoundfontFile::Bag::GeneratorTypes::startloopAddrsCoarseOffset);
				endloopAddress = this->sample->loopEnd + this->getInstrumentGeneratorValue<int>(SoundfontFile::Bag::GeneratorTypes::endloopAddrsOffset) + 32768 * this->getInstrumentGeneratorValue<int>(SoundfontFile::Bag::GeneratorTypes::endloopAddrsCoarseOffset);

				const int initialAttenuation{ this->getCombinedGeneratorValue<int>(SoundfontFile::Bag::GeneratorTypes::initialAttenuation) };
				const float attenuation{ initialAttenuation + velToAtt[this->velocity] };
				attenuationFactor = powf(10.0f, -attenuation / 200.0f);

				volEnvDelayTime = absTimecentsToSecs(this->getCombinedGeneratorValue<int>(SoundfontFile::Bag::GeneratorTypes::delayVolEnv));
				volEnvAttackTime = absTimecentsToSecs(this->getCombinedGeneratorValue<int>(SoundfontFile::Bag::GeneratorTypes::attackVolEnv));
				volEnvHoldTime = absTimecentsToSecs(this->getCombinedGeneratorValue<int>(SoundfontFile::Bag::GeneratorTypes::holdVolEnv) - (this->key - 60) * this->getCombinedGeneratorValue<int>(SoundfontFile::Bag::GeneratorTypes::keynumToVolEnvHold));
				volEnvDecayTime = absTimecentsToSecs(this->getCombinedGeneratorValue<int>(SoundfontFile::Bag::GeneratorTypes::decayVolEnv) - (this->key - 60) * this->getCombinedGeneratorValue<int>(SoundfontFile::Bag::GeneratorTypes::keynumToVolEnvDecay));
				volEnvReleaseTime = absTimecentsToSecs(this->getCombinedGeneratorValue<int>(SoundfontFile::Bag::GeneratorTypes::releaseVolEnv));
			

				unsigned int pitch{ this->sample->originalPitch };

				const int overridingRootKey{ this->getInstrumentGeneratorValue<int>(SoundfontFile::Bag::GeneratorTypes::overridingRootKey)};

				if (overridingRootKey != -1)
				{
					pitch = overridingRootKey;
				}

				if (pitch == 255) pitch = 60;

				const int scaleTuning{ this->getCombinedGeneratorValue<int>(SoundfontFile::Bag::GeneratorTypes::scaleTuning) };
				const int coarseTuning{ this->getCombinedGeneratorValue<int>(SoundfontFile::Bag::GeneratorTypes::coarseTune) };
				const int fineTuning{ this->getCombinedGeneratorValue<int>(SoundfontFile::Bag::GeneratorTypes::fineTune) };


				rootPitch = pow(semitoneToPitchFactor(this->key - pitch), scaleTuning / 100.0f) * semitoneToPitchFactor(coarseTuning) * pow(2.0f, fineTuning / 1200.0f);
			}
			template<class T>
			T getCombinedGeneratorValue(const SoundfontFile::Bag::GeneratorTypes generator) const;
			template<>
			int getCombinedGeneratorValue(const SoundfontFile::Bag::GeneratorTypes generator) const
			{
				if (presetBag->generators[(unsigned int)(generator)].isSet)
				{
					return int(presetBag->generators[(unsigned int)(generator)].amount.shortAmount) + int(instrumentBag->generators[(unsigned int)(generator)].amount.shortAmount);
				}
				else
				{
					return int(instrumentBag->generators[(unsigned int)(generator)].amount.shortAmount);
				}
			}
			template<>
			unsigned int getCombinedGeneratorValue(const SoundfontFile::Bag::GeneratorTypes generator) const
			{
				if (presetBag->generators[(unsigned int)(generator)].isSet)
				{
					return (unsigned int)(presetBag->generators[(unsigned int)(generator)].amount.wordAmount) + (unsigned int)(instrumentBag->generators[(unsigned int)(generator)].amount.wordAmount);
				}
				else
				{
					return (unsigned int)(instrumentBag->generators[(unsigned int)(generator)].amount.wordAmount);
				}
			}
			template<>
			SoundfontFile::Bag::Generator::GeneratorAmount::Range getCombinedGeneratorValue(const SoundfontFile::Bag::GeneratorTypes generator) const
			{
				if (presetBag->generators[(unsigned int)(generator)].isSet)
				{
					return { std::max(presetBag->generators[(unsigned int)(generator)].amount.range.low, instrumentBag->generators[(unsigned int)(generator)].amount.range.low), std::min(presetBag->generators[(unsigned int)(generator)].amount.range.high, instrumentBag->generators[(unsigned int)(generator)].amount.range.high) };
				}
				else
				{
					return instrumentBag->generators[(unsigned int)(generator)].amount.range;
				}
			}

			template<class T>
			T getInstrumentGeneratorValue(const SoundfontFile::Bag::GeneratorTypes generator) const;
			template<>
			int getInstrumentGeneratorValue(const SoundfontFile::Bag::GeneratorTypes generator) const
			{
				return (int)(instrumentBag->generators[(unsigned int)(generator)].amount.shortAmount);
			}
			template<>
			unsigned int getInstrumentGeneratorValue(const SoundfontFile::Bag::GeneratorTypes generator) const
			{
				return (unsigned int)(instrumentBag->generators[(unsigned int)(generator)].amount.wordAmount);
			}
			template<>
			SoundfontFile::Bag::Generator::GeneratorAmount::Range getInstrumentGeneratorValue(const SoundfontFile::Bag::GeneratorTypes generator) const
			{
				return instrumentBag->generators[(unsigned int)(generator)].amount.range;
			}
		};
		std::vector<Sample> samples;
		std::vector<int> samplePool;
		void setChannelVolume(const unsigned int channel, const unsigned int volume)
		{
			channelVolumes[channel] = volume;
			this->updateChannelAttenuationFactor(channel);
		}
		void setChannelExpression(const unsigned int channel, const unsigned int expression)
		{
			channelExpressions[channel] = expression;
			this->updateChannelAttenuationFactor(channel);
		}
		void updateChannelAttenuationFactor(const unsigned int channel)
		{
			const float channelAttenuation{ velToAtt[this->channelVolumes[channel]] + velToAtt[this->channelExpressions[channel]] };
			channelAttenuationFactors[channel] = powf(10.0f, -channelAttenuation / 200.0f);
		}
		void setChannelPitchBend(const unsigned int channel, const unsigned int pitchBend)
		{
			channelPitchBends[channel] = pitchBend;
			this->updateChannelPitchBendFactor(channel);
		}
		void updateChannelPitchBendFactor(const unsigned int channel)
		{
			channelPitchBendFactors[channel] = pow(2.0f, (channelPitchBends[channel] / float(0x2000) - 1.0f) / 12.0f);
		}
		void setSamplePool(const std::vector<int>& samplePool)
		{
			this->samplePool = samplePool;
		}
		void playSample(const Sample& sample)
		{
			samples.push_back(sample);
		}
		std::pair<double, double> nextSampleDataPoint(const double elapsedTime, const double midiElapsedTime, const unsigned int samplesPerSecond)
		{
			std::pair<double, double> sampleDataPoint{ 0.0, 0.0 };


			for (Sample& sample : samples)
			{
				const float attenuationFactor{ sample.attenuationFactor * this->channelAttenuationFactors[sample.channel] };

				float volEnv{ 0.0f };
				

				if (sample.volEnvPhase == Sample::EnvPhase::None)
				{
					sample.volEnvPhaseStartTime = elapsedTime;
					sample.volEnvPhase = Sample::EnvPhase::Delay;
					sample.initialPhaseVolEnv = 0.0f;
				}
				if (sample.volEnvPhase == Sample::EnvPhase::Delay)
				{
					if (elapsedTime - sample.volEnvPhaseStartTime >= sample.volEnvDelayTime)
					{
						sample.volEnvPhaseStartTime += sample.volEnvDelayTime;
						sample.volEnvPhase = Sample::EnvPhase::Attack;
						sample.initialPhaseVolEnv = 0.0f;
					}
					else
					{
						volEnv = 0.0f;
					}
				}
				if (sample.volEnvPhase == Sample::EnvPhase::Attack)
				{
					if (elapsedTime - sample.volEnvPhaseStartTime >= sample.volEnvAttackTime)
					{
						sample.volEnvPhaseStartTime += sample.volEnvAttackTime;
						sample.volEnvPhase = Sample::EnvPhase::Hold;
						sample.initialPhaseVolEnv = 1.0f;
					}
					else
					{
						volEnv = (elapsedTime - sample.volEnvPhaseStartTime) / sample.volEnvAttackTime;
					}
				}
				if (sample.volEnvPhase == Sample::EnvPhase::Hold)
				{
					if (elapsedTime - sample.volEnvPhaseStartTime >= sample.volEnvHoldTime)
					{
						sample.volEnvPhaseStartTime += sample.volEnvHoldTime;
						sample.volEnvPhase = Sample::EnvPhase::Decay;
						sample.initialPhaseVolEnv = 1.0f;
					}
					else
					{
						volEnv = 1.0f;
					}
				}
				if (sample.volEnvPhase == Sample::EnvPhase::Decay)
				{
					if (20.0f * log10(sample.initialPhaseVolEnv) - 100.0f * (elapsedTime - sample.volEnvPhaseStartTime) / sample.volEnvDecayTime <= -std::max(sample.getCombinedGeneratorValue<int>(SoundfontFile::Bag::GeneratorTypes::sustainVolEnv), 0) / 10.0f)
					{
						sample.volEnvPhaseStartTime += sample.volEnvDecayTime;
						sample.volEnvPhase = Sample::EnvPhase::Sustain;
						sample.initialPhaseVolEnv = pow(10.0f, -std::max(0, sample.getCombinedGeneratorValue<int>(SoundfontFile::Bag::GeneratorTypes::sustainVolEnv)) / 200.0f);
					}
					else
					{
						volEnv = pow(10.0f, (20.0f * log10(sample.initialPhaseVolEnv) - 100.0f * (elapsedTime - sample.volEnvPhaseStartTime) / sample.volEnvDecayTime) / 20.0f);
					}
				}

				if (sample.volEnvPhase == Sample::EnvPhase::Sustain)
				{
					volEnv = sample.initialPhaseVolEnv;
				}

				if (sample.volEnvPhase != Sample::EnvPhase::Release)
				{
					if (sample.midiEndTime <= midiElapsedTime && sample.volEnvPhase != Sample::EnvPhase::Finished)
					{
						sample.volEnvPhaseStartTime = elapsedTime;
						sample.volEnvPhase = Sample::EnvPhase::Release;
					}
				}

				if (sample.volEnvPhase == Sample::EnvPhase::Release)
				{
					if (20.0f * log10(sample.initialPhaseVolEnv) - 100.0f * (elapsedTime - sample.volEnvPhaseStartTime) / sample.volEnvReleaseTime <= -100.0f)
					{
						sample.volEnvPhase = Sample::EnvPhase::Finished;
						volEnv = 0.0f;
					}
					else
					{
						volEnv = pow(10.0f, (20.0f * log10(sample.initialPhaseVolEnv) - 100.0f * (elapsedTime - sample.volEnvPhaseStartTime) / sample.volEnvReleaseTime) / 20.0f);
					}
				}

				if (sample.dataPoint <= sample.endAddress)
				{
					float sampleValue{ attenuationFactor * samplePool[int(sample.dataPoint)] * volEnv };
					switch (sample.sample->type)
					{
					case SoundfontFile::SampleHeader::SampleType::mono:
						sampleDataPoint.first += sampleValue * 0.5f;
						sampleDataPoint.second += sampleValue * 0.5f;
						break;
					case SoundfontFile::SampleHeader::SampleType::right:
						sampleDataPoint.first += sampleValue;
						break;
					case SoundfontFile::SampleHeader::SampleType::left:
						sampleDataPoint.second += sampleValue;
						break;
					default:
						std::cout << "Unsupported sample type found!\n";
						throw -1;
					}
				}

				sample.dataPoint += sample.sample->sampleRate * sample.rootPitch * channelPitchBendFactors[sample.channel] / float(samplesPerSecond);

				switch (sample.getInstrumentGeneratorValue<unsigned int>(SoundfontFile::Bag::GeneratorTypes::sampleModes) % 4)
				{
				case 0:
				case 2:
				{
					break;
				}
				case 1:
				{
					if (sample.dataPoint > sample.endloopAddress)
					{
						sample.dataPoint = sample.startloopAddress + glm::mod(sample.dataPoint - sample.startloopAddress, (double)(sample.endloopAddress - sample.startloopAddress));
					}
					break;
				}
				case 3:
				{
					if (midiElapsedTime < sample.midiEndTime)
						if (sample.dataPoint > sample.endloopAddress)
						{
							sample.dataPoint = sample.startloopAddress + glm::mod(sample.dataPoint - sample.startloopAddress, (double)(sample.endloopAddress - sample.startloopAddress));
						}
					break;
				}
				}
			}

			samples.erase(std::remove_if(samples.begin(), samples.end(), [&](const auto& sample)
				{
				return sample.dataPoint > sample.endAddress || sample.volEnvPhase == Sample::EnvPhase::Finished;
				}), samples.end());
			return sampleDataPoint;
		}
	};
	WavetableOscillator wavetableOscillator;
	std::vector<ActiveNote> active;
	std::array<const SoundfontFile::Preset*, 16> assignedPresets;

	MidiFile midiFile;
	SoundfontFile soundfontFile;
	
	double midiElapsedTime{ 0.0 };
	double elapsedTime{ 0.0 };

	unsigned int samplesPerSecond;

	static const unsigned int standardWesternTempoInBeatsPerMinute{ 120 };
	static const unsigned int secondsPerMinute{ 60 };
	static const unsigned int microsecondsPerSecond{ 1'000'000 };
	static const unsigned int microsecondsPerMinute{ secondsPerMinute * microsecondsPerSecond };
	unsigned int microsecondsPerQuarterNote{ microsecondsPerMinute / standardWesternTempoInBeatsPerMinute };

	std::vector<unsigned int> eventIndices;
	std::vector<unsigned int> lastEventTimes;

	void setMidiEventBehaviors()
	{
		midiFile.eventBehaviorTable.note.doWhenPlayed([&](const auto note)
			{
				ActiveNote newNote;
				newNote.channel = note->channel;
				newNote.key = note->key;
				newNote.velocity = note->velocity;
				newNote.endTime = note->endTime;
				newNote.midiElapsedTimeAtStart = midiElapsedTime;
				newNote.elapsedTimeAtStart = elapsedTime;
				active.push_back(newNote);


				const SoundfontFile::Preset& preset{ *assignedPresets[newNote.channel] };
				
				for (unsigned int presetBagIndex : preset.opt[newNote.key][newNote.velocity])
				{
					const SoundfontFile::Bag& presetBag{ preset.bags[presetBagIndex] };
					
					const SoundfontFile::Instrument& instrument{ soundfontFile.instruments[presetBag.generators[(unsigned int)(SoundfontFile::Bag::GeneratorTypes::instrument)].amount.wordAmount] };
					
					for (unsigned int instrumentBagIndex : instrument.opt[newNote.key][newNote.velocity])
					{
						const SoundfontFile::Bag& instrumentBag{ instrument.bags[instrumentBagIndex] };
						const SoundfontFile::SampleHeader& sample{ soundfontFile.samples[instrumentBag.generators[(unsigned int)(SoundfontFile::Bag::GeneratorTypes::sampleID)].amount.wordAmount] };
						
						
						wavetableOscillator.playSample(WavetableOscillator::Sample{ newNote.channel, newNote.key, newNote.velocity, newNote.endTime, sample, presetBag, instrumentBag });
					}
				}
			});
		midiFile.eventBehaviorTable.noteOff.doWhenPlayed([&](const auto noteOff)
			{
				//std::cout << "Error: notes were not ordered in memory correctly\n";
			});
		midiFile.eventBehaviorTable.noteOn.doWhenPlayed([&](const auto noteOn)
			{
				std::cout << "Error: notes were not ordered in memory correctly\n";
			});
		midiFile.eventBehaviorTable.programChange.doWhenPlayed([&](const auto programChange)
			{
				if (this->soundfontFile.presets.contains({ programChange->channel == 9 ? 128u : 0u, programChange->channel == 9 ? 0u : programChange->program }))
				{
					const SoundfontFile::Preset& preset{ this->soundfontFile.presets.at({ programChange->channel == 9 ? 128u : 0u, programChange->channel == 9 ? 0u : programChange->program }) };
					this->assignedPresets[programChange->channel] = &preset;
				}
				else if (this->soundfontFile.presets.size() > 0)
				{
					this->assignedPresets[programChange->channel] = &(this->soundfontFile.presets.begin()->second);
				}
			});
		midiFile.eventBehaviorTable.tempoChange.doWhenPlayed([&](const auto tempoChange)
			{
				this->microsecondsPerQuarterNote = tempoChange->microsecondsPerQuarterNote;
			});
		midiFile.eventBehaviorTable.lyric.doWhenPlayed([&](const auto lyric)
			{
				this->doLyricEvent(lyric->lyric);
			});
		midiFile.eventBehaviorTable.controlChange.doWhenPlayed([&](const auto controlChange)
			{
				if (controlChange->controller == 7)
				{
					wavetableOscillator.setChannelVolume(controlChange->channel, controlChange->value);
				}
				else if (controlChange->controller == 11)
				{
					wavetableOscillator.setChannelExpression(controlChange->channel, controlChange->value);
				}
			});
		midiFile.eventBehaviorTable.pitchBend.doWhenPlayed([&](const auto pitchBend)
			{
				wavetableOscillator.setChannelPitchBend(pitchBend->channel, pitchBend->mostSignificant7Bits * (1 << 7) + pitchBend->leastSignificant7Bits);
			});
	}

	static float semitoneToPitchFactor(int semitone)
	{
		return semitoneExponents[127u + semitone];
	}

	static float absTimecentsToSecs(int timecents)
	{
		return pow(2.0f, timecents / 1200.0f);
	}

	void initializeSemitoneExponentsArray()
	{
		float exponent{ 1.0f };
		for (unsigned int note{ 127 }; note < 255; ++note)
		{
			static const float semitone{ float(pow(2, 1.0f / 12.0f)) };
			semitoneExponents[note] = exponent;
			exponent *= semitone;
		}
		for (unsigned int note{ 0 }; note < 127; ++note)
		{
			semitoneExponents[note] = 1.0f / semitoneExponents[254 - note];
		}
	}
	void initializeVelToAttArray()
	{
		velToAtt[0] = 127.0f / 128.0f; // TODO: this is currently a hack
		for (unsigned int vel{ 1 }; vel < 128; ++vel)
		{
			velToAtt[vel] = -400.0f * log10f(vel / 127.0f);
		}
	}
	void initializeLUTs()
	{
		initializeSemitoneExponentsArray();
		initializeVelToAttArray();
	}
public:
	Synthesizer(const MidiFile& midiFile, const SoundfontFile& soundfontFile, const unsigned int samplesPerSecond)
		:
		midiFile(midiFile),
		soundfontFile(soundfontFile),
		samplesPerSecond(samplesPerSecond)
	{
		if (!initializedLUTs)
		{
			initializeLUTs();
			initializedLUTs = true;
		}
		for (unsigned int channel{ 0 }; channel < 16; ++channel)
		{
			if (this->soundfontFile.presets.contains({ channel == 9 ? 128u : 0u, 0u }))
			{
				const SoundfontFile::Preset& preset{ this->soundfontFile.presets.at({ channel == 9 ? 128u : 0u, 0u }) };
				this->assignedPresets[channel] = &preset;
			}
			else if (this->soundfontFile.presets.size() > 0)
			{
				this->assignedPresets[channel] = &(this->soundfontFile.presets.begin()->second);
			}
		}
		eventIndices.resize(midiFile.tracks.size());
		lastEventTimes.resize(midiFile.tracks.size());
		setMidiEventBehaviors();
		this->wavetableOscillator.setSamplePool(soundfontFile.sampleDataPoints);
	}
	void nextMidiTimePoint()
	{
		using Track = MidiFileCore::Track;
		const double secondsPerQuarterNote{ microsecondsPerQuarterNote / double(microsecondsPerSecond) };
		switch (midiFile.timeDivision->getFormat())
		{
			using Header = MidiFileCore::Header;
		case Header::TimeDivision::Format::metrical:
		{
			const Header::MetricalTimeDivision* metricalTimeDivision{ static_cast<Header::MetricalTimeDivision*>(midiFile.timeDivision.get()) };
			const unsigned int deltaTimeTicksPerQuarterNote{ metricalTimeDivision->ticksPerQuarterNote };
			const double deltaTimeTicksPerSecond{ deltaTimeTicksPerQuarterNote / double(secondsPerQuarterNote) };

			const double deltaTimeTicksPerFrame{ deltaTimeTicksPerSecond / double(samplesPerSecond) };
			midiElapsedTime += deltaTimeTicksPerFrame;
		}
		break;
		case Header::TimeDivision::Format::timecode:
		{
			const Header::TimecodeBasedTimeDivision* timeCodeTimeDivision{ static_cast<Header::TimecodeBasedTimeDivision*>(midiFile.timeDivision.get()) };
			const unsigned int deltaTimeTicksPerFrame{ timeCodeTimeDivision->ticksPerFrame };

			const unsigned int fps{ static_cast<unsigned int>(-timeCodeTimeDivision->negativeSMPTEFormat) };
			midiElapsedTime += deltaTimeTicksPerFrame / double(samplesPerSecond * secondsPerQuarterNote);
		}
		break;
		}

		for (unsigned int trackIndex{ 0 }; trackIndex < midiFile.tracks.size(); ++trackIndex)
		{
			unsigned int& eventIndex{ eventIndices[trackIndex] };
			Track& track{ midiFile.tracks[trackIndex] };
			if (eventIndex < track.eventCount())
			{
				while (midiElapsedTime - lastEventTimes[trackIndex] > track[eventIndex].deltaTime())
				{
					lastEventTimes[trackIndex] += track[eventIndex].deltaTime();
					midiFile.play(track[eventIndex]);



					++eventIndex;
					if (!(eventIndex < track.eventCount()))
					{
						break;
					}
				}
			}
		}
	}
	std::pair<float, float> nextSample()
	{
		activeNotesMutex.lock();
		this->nextMidiTimePoint();
		activeNotesMutex.unlock();
		std::pair<double, double> sampleDataPoint{ 0.0f, 0.0f };
		elapsedTime += 1.0f / samplesPerSecond;


		activeNotesMutex.lock();
		active.erase(std::remove_if(active.begin(), active.end(), [&](auto const& activeNote) { return activeNote.endTime < midiElapsedTime; }), active.end());
		activeNotesMutex.unlock();

		
		sampleDataPoint = wavetableOscillator.nextSampleDataPoint(elapsedTime, midiElapsedTime, samplesPerSecond);
		return { sampleDataPoint.first / (256.0 * 256.0 * 256.0), sampleDataPoint.second / (256.0 * 256.0 * 256.0) };
	}
	std::vector<ActiveNote> getPlayingNoteTree()
	{
		std::vector<ActiveNote> playingNotes;
		activeNotesMutex.lock();
		playingNotes = active;
		activeNotesMutex.unlock();
		return playingNotes;
	}
	bool isPlaying()
	{
		bool someTrackIsPlaying{ false };
		for (unsigned int trackIndex{ 0 }; trackIndex < midiFile.tracks.size(); ++trackIndex)
		{
			if (eventIndices[trackIndex] < midiFile.tracks[trackIndex].eventCount())
			{
				someTrackIsPlaying = true;
				break;
			}
		}
		return someTrackIsPlaying;
	}
	void replay()
	{
		for (WavetableOscillator::Sample& sample : this->wavetableOscillator.samples)
			sample.midiEndTime = 0;
		this->microsecondsPerQuarterNote = microsecondsPerMinute / standardWesternTempoInBeatsPerMinute;
		this->midiElapsedTime = 0.0f;
		for (unsigned int trackIndex{ 0 }; trackIndex < midiFile.tracks.size(); ++trackIndex)
		{
			this->eventIndices[trackIndex] = 0;
			this->lastEventTimes[trackIndex] = 0;
		}
	}
	float getSecondsSincePlayStart()
	{
		return this->elapsedTime;
	}
	float midiTicksSincePlayStart()
	{
		return this->midiElapsedTime;
	}
	MidiFile getLoadedMidi()
	{
		return midiFile;
	}
};
