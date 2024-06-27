#include "Lfo.h"
#include "VolEnv.h"

struct VoiceProperties
{
	unsigned int channel;
	unsigned int key;
	unsigned int velocity;
	unsigned int midiEndTime;
	const SoundfontFile::Bag* presetBag;
	const SoundfontFile::Bag* instrumentBag;
	VoiceProperties(const unsigned int channel, const unsigned int key, const unsigned int velocity, const unsigned int midiEndTime, const SoundfontFile::Bag& presetBag, const SoundfontFile::Bag& instrumentBag)
		:channel(channel), key(key), velocity(velocity), midiEndTime(midiEndTime), presetBag(&presetBag), instrumentBag(&instrumentBag)
	{
	}
};

struct Sample {
	int startAddress;
	int endAddress;
	int startloopAddress;
	int endloopAddress;
	double dataPoint;
	double rootPitch;
	unsigned int rate;
	SoundfontFile::SampleHeader::SampleType type;
};

struct Voice
{
	unsigned int channel;
	unsigned int key;
	unsigned int velocity;
	unsigned int midiEndTime;
	const SoundfontFile::Bag* presetBag;
	const SoundfontFile::Bag* instrumentBag;
	Sample sample;

	Lfo vibLfo;
	Lfo modLfo;
	float vibLfoToPitchCents;
	float modLfoToPitchCents;
	float modLfoToFilterCutoff;
	float modLfoToVolume;

	VolEnv volEnv;
	unsigned int sampleModeFlags;

	float panIntrinsic;
	float panCombined;


	float attenuationFactorIntrinsic;
	float attenuationFactorCombined;

	float intrinsicSDPPA;
	float sampleDataPointsPerAmp;


	Voice(const VoiceProperties& properties, const SoundfontFile::SampleHeader& sampleHeader, const UnitConverter& converter, float samplingInterval)
		:channel(properties.channel), key(properties.key), velocity(properties.velocity), midiEndTime(properties.midiEndTime), presetBag(properties.presetBag), instrumentBag(properties.instrumentBag)
	{
		using GeneratorTypes = SoundfontFile::Bag::GeneratorTypes;
		int startOffset{ this->getInstrumentGeneratorValue<int>(GeneratorTypes::startAddrsOffset) + 32768 * this->getInstrumentGeneratorValue<int>(GeneratorTypes::startAddrsCoarseOffset) };
		int endOffset{ this->getInstrumentGeneratorValue<int>(GeneratorTypes::endAddrsOffset) + 32768 * this->getInstrumentGeneratorValue<int>(GeneratorTypes::endAddrsCoarseOffset) };
		int startloopOffset{ this->getInstrumentGeneratorValue<int>(GeneratorTypes::startloopAddrsOffset) + 32768 * this->getInstrumentGeneratorValue<int>(GeneratorTypes::startloopAddrsCoarseOffset) };
		int endloopOffset{ this->getInstrumentGeneratorValue<int>(GeneratorTypes::endloopAddrsOffset) + 32768 * this->getInstrumentGeneratorValue<int>(GeneratorTypes::endloopAddrsCoarseOffset) };

		makeSample(this->sample, sampleHeader, startOffset, endOffset, startloopOffset, endloopOffset, converter);

		constexpr float emuAttenuationFactor{ 0.4f };
		const int initialAttenuation{ this->getCombinedGeneratorValue<int>(GeneratorTypes::initialAttenuation) };
		const float attenuation{ emuAttenuationFactor * initialAttenuation + converter.velocityToAttenuation(this->velocity) };
		this->attenuationFactorIntrinsic = powf(10.0f, -attenuation / 200.0f);
		this->updateExternalAttenuation(1.0f);
		this->sampleModeFlags = this->getInstrumentGeneratorValue<unsigned int>(GeneratorTypes::sampleModes) % 4;
		this->panIntrinsic = this->getCombinedGeneratorValue<int>(GeneratorTypes::pan) / 1000.0f;
		this->updateExternalPan(0.0f);

		this->intrinsicSDPPA = this->sample.rate * this->sample.rootPitch * samplingInterval;
		this->updateExternalPitchBendFactor(1.0f);

		this->vibLfo.setDelayTime(converter.absTimecentsToSecs(this->getCombinedGeneratorValue<int>(GeneratorTypes::delayVibLfo)));
		this->vibLfo.setFrequency(8.176f * powf(2.0f, this->getCombinedGeneratorValue<int>(GeneratorTypes::freqVibLfo) / 1200.0f));
		this->vibLfoToPitchCents = this->getCombinedGeneratorValue<int>(GeneratorTypes::vibLfoToPitch);

		this->modLfo.setDelayTime(converter.absTimecentsToSecs(this->getCombinedGeneratorValue<int>(GeneratorTypes::delayModLfo)));
		this->modLfo.setFrequency(8.176f * powf(2.0f, this->getCombinedGeneratorValue<int>(GeneratorTypes::freqModLfo) / 1200.0f));
		this->modLfoToPitchCents = this->getCombinedGeneratorValue<int>(GeneratorTypes::modLfoToPitch);
		this->modLfoToFilterCutoff = this->getCombinedGeneratorValue<int>(GeneratorTypes::modLfoToFilterFc);
		this->modLfoToVolume = this->getCombinedGeneratorValue<int>(GeneratorTypes::modLfoToVolume);

		this->volEnv.setDelayTime(converter.absTimecentsToSecs(this->getCombinedGeneratorValue<int>(GeneratorTypes::delayVolEnv)));
		this->volEnv.setAttackTime(converter.absTimecentsToSecs(this->getCombinedGeneratorValue<int>(GeneratorTypes::attackVolEnv)));
		this->volEnv.setHoldTime(converter.absTimecentsToSecs(this->getCombinedGeneratorValue<int>(GeneratorTypes::holdVolEnv) - (this->key - 60) * this->getCombinedGeneratorValue<int>(SoundfontFile::Bag::GeneratorTypes::keynumToVolEnvHold)));
		this->volEnv.setDecayTime(converter.absTimecentsToSecs(this->getCombinedGeneratorValue<int>(GeneratorTypes::decayVolEnv) - (this->key - 60) * this->getCombinedGeneratorValue<int>(SoundfontFile::Bag::GeneratorTypes::keynumToVolEnvDecay)));
		this->volEnv.setSustainValue(this->getCombinedGeneratorValue<int>(GeneratorTypes::sustainVolEnv));
		this->volEnv.setReleaseTime(converter.absTimecentsToSecs(this->getCombinedGeneratorValue<int>(GeneratorTypes::releaseVolEnv)));
	}

	void updateExternalPitchBendFactor(float pitchBendFactor) {
		this->sampleDataPointsPerAmp = this->intrinsicSDPPA * pitchBendFactor;
	}

	void updateExternalAttenuation(float attExt) {
		this->attenuationFactorCombined = this->attenuationFactorIntrinsic * attExt;
	}

	void updateExternalPan(float panExt) {
		float pan{ this->panIntrinsic + panExt };
		if (pan < -0.5f) {
			pan = -0.5f;
		} else if (pan > 0.5f) {
			pan = 0.5f;
		}
		this->panCombined = pan;
	}

	float getSampleDataPointsPerAmp() {
		return this->sampleDataPointsPerAmp;//voice.sample.rate * voice.sample.rootPitch * this->knobSets[voice.channel].getPitchBendFactor() * this->samplingTimeInterval
	}

	float getCombinedAttenuationFactor() {
		return this->attenuationFactorCombined;
	}

	float getCombinedPan() {
		return this->panCombined;
	}

	void clipDataPoint(double midiElapsedTime) {
		switch (this->sampleModeFlags)
		{
			case 0:
			case 2:
			{
				break;
			}
			case 1:
			{
				if (this->sample.dataPoint > this->sample.endloopAddress)
				{
					this->sample.dataPoint = this->sample.startloopAddress + glm::mod(this->sample.dataPoint - this->sample.startloopAddress, (double)(this->sample.endloopAddress - this->sample.startloopAddress));
				}
				break;
			}
			case 3:
			{
				if (midiElapsedTime < this->midiEndTime) {
					if (this->sample.dataPoint > this->sample.endloopAddress)
					{
						this->sample.dataPoint = this->sample.startloopAddress + glm::mod(this->sample.dataPoint - this->sample.startloopAddress, (double)(this->sample.endloopAddress - this->sample.startloopAddress));
					}
				}
				break;
			}
		}
	}


	void makeSample(Sample& sample, const SoundfontFile::SampleHeader& sampleHeader, const int startOffset, const int endOffset, const int startloopOffset, const int endloopOffset, const UnitConverter& converter) {
		sample.startAddress = sampleHeader.start + startOffset;
		sample.endAddress = sampleHeader.end + endOffset;
		sample.startloopAddress = sampleHeader.loopStart + startloopOffset;
		sample.endloopAddress = sampleHeader.loopEnd + endloopOffset;
		sample.rootPitch = this->calculateRootPitch(sampleHeader.originalPitch, converter);
		sample.dataPoint = sample.startAddress;
		sample.rate = sampleHeader.sampleRate;
		sample.type = sampleHeader.type;
	}

	double calculateRootPitch(const unsigned int originalPitch, const UnitConverter& converter) {
		unsigned int pitch{ originalPitch };
		const int overridingRootKey{ this->getInstrumentGeneratorValue<int>(SoundfontFile::Bag::GeneratorTypes::overridingRootKey)};

		if (overridingRootKey != -1)
		{
			pitch = overridingRootKey;
		}

		if (pitch == 255) pitch = 60;

		const int scaleTuning{ this->getCombinedGeneratorValue<int>(SoundfontFile::Bag::GeneratorTypes::scaleTuning) };
		const int coarseTuning{ this->getCombinedGeneratorValue<int>(SoundfontFile::Bag::GeneratorTypes::coarseTune) };
		const int fineTuning{ this->getCombinedGeneratorValue<int>(SoundfontFile::Bag::GeneratorTypes::fineTune) };
		return pow(converter.semitoneToPitchFactor(this->key - pitch), scaleTuning / 100.0f) * converter.semitoneToPitchFactor(coarseTuning) * pow(2.0f, fineTuning / 1200.0f);
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
