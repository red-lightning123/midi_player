#include "MidiFile.h"
#include "SftFile.h"
#include "WavetableOscillator.h"

#include <array>
#include <vector>
#include <set>
#include <algorithm>
#include <mutex>
#include <functional>

class Synthesizer
{
private:
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
						const SoundfontFile::SampleHeader& sampleHeader{ soundfontFile.samples[instrumentBag.generators[(unsigned int)(SoundfontFile::Bag::GeneratorTypes::sampleID)].amount.wordAmount] };
						const VoiceProperties voiceProperties{ newNote.channel, newNote.key, newNote.velocity, newNote.endTime, presetBag, instrumentBag };
						wavetableOscillator.playVoice(voiceProperties, sampleHeader);
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
				if (controlChange->controller == 1)
				{
					wavetableOscillator.setChannelVibrato(controlChange->channel, controlChange->value);
				}
				else if (controlChange->controller == 7)
				{
					wavetableOscillator.setChannelVolume(controlChange->channel, controlChange->value);
				}
				else if (controlChange->controller == 10)
				{
					wavetableOscillator.setChannelPan(controlChange->channel, controlChange->value);
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
public:
	Synthesizer(const MidiFile& midiFile, const SoundfontFile& soundfontFile, const unsigned int samplesPerSecond)
		:
		midiFile(midiFile),
		soundfontFile(soundfontFile),
		samplesPerSecond(samplesPerSecond)
	{
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
	StereoPair nextAmp()
	{
		activeNotesMutex.lock();
		this->nextMidiTimePoint();
		activeNotesMutex.unlock();
		StereoPair amp{ 0.0, 0.0 };
		elapsedTime += 1.0f / samplesPerSecond;


		activeNotesMutex.lock();
		active.erase(std::remove_if(active.begin(), active.end(), [&](auto const& activeNote) { return activeNote.endTime < midiElapsedTime; }), active.end());
		activeNotesMutex.unlock();

		
		amp = wavetableOscillator.nextAmp(elapsedTime, midiElapsedTime);
		return { amp.right / (256.0 * 256.0 * 256.0), amp.left / (256.0 * 256.0 * 256.0) };
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
		for (Voice& voice : this->wavetableOscillator.voices)
			voice.midiEndTime = 0;
		this->microsecondsPerQuarterNote = microsecondsPerMinute / standardWesternTempoInBeatsPerMinute;
		this->midiElapsedTime = 0.0f;
		for (unsigned int trackIndex{ 0 }; trackIndex < midiFile.tracks.size(); ++trackIndex)
		{
			this->eventIndices[trackIndex] = 0;
			this->lastEventTimes[trackIndex] = 0;
		}
	}
	void setSamplesPerSecond(const unsigned int samplesPerSecond) {
		// storing the reciprocal is an optimization
		// this (typically constant) value will later be divided by, and as division tends to be slower than multiplication
		// multiplying with the reciprocal is faster than dividing by it
		this->wavetableOscillator.setSamplesPerSecond(samplesPerSecond);
	}
	float getSecondsSincePlayStart()
	{
		return this->elapsedTime;
	}
	float getMidiTicksSincePlayStart()
	{
		return this->midiElapsedTime;
	}
	MidiFile getLoadedMidi()
	{
		return midiFile;
	}
};
