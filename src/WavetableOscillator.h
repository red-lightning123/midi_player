#include "MidiFile.h"
#include "SftFile.h"
#include "UnitConverter.h"
#include "Voice.h"
#include "ChannelKnobSet.h"
#include "StereoPair.h"

#include <array>
#include <vector>
#include <set>
#include <algorithm>
#include <mutex>
#include <functional>

class WavetableOscillator
{
private:
	UnitConverter converter;

	std::array<ChannelKnobSet, 16> knobSets;
	float samplingTimeInterval;


	void resetAllControllers()
	{
		for (unsigned int channel{ 0 }; channel < 16; ++channel)
		{
			this->knobSets[channel].resetAllKnobs();
		}
		this->updateAllVoicePans();
		this->updateAllVoiceAttenuations();
		this->updateAllVoicePitchBends();
	}

	void updateAllVoicePans() {
		for (Voice& voice : this->voices) {
			voice.updateExternalPan(this->knobSets[voice.channel].getPanFactor());
		}
	}

	void updateVoicePansForChannel(unsigned int channel) {
		for (Voice& voice : this->voices) {
			if (voice.channel == channel) {
				voice.updateExternalPan(this->knobSets[voice.channel].getPanFactor());
			}
		}
	}

	void updateAllVoiceAttenuations() {
		for (Voice& voice : this->voices) {
			voice.updateExternalAttenuation(this->knobSets[voice.channel].getAttenuationFactor());
		}
	}

	void updateVoiceAttenuationsForChannel(unsigned int channel) {
		for (Voice& voice : this->voices) {
			if (voice.channel == channel) {
				voice.updateExternalAttenuation(this->knobSets[voice.channel].getAttenuationFactor());
			}
		}
	}

	void updateAllVoicePitchBends() {
		for (Voice& voice : this->voices) {
			voice.updateExternalPitchBendFactor(this->knobSets[voice.channel].getPitchBendFactor());
		}
	}

	void updateVoicePitchBendsForChannel(unsigned int channel) {
		for (Voice& voice : this->voices) {
			if (voice.channel == channel) {
				voice.updateExternalPitchBendFactor(this->knobSets[voice.channel].getPitchBendFactor());
			}
		}
	}
public:
	void setChannelVolume(const unsigned int channel, const unsigned int volume)
	{
		this->knobSets[channel].setVolume(volume);
		this->updateVoiceAttenuationsForChannel(channel);
	}
	void setChannelExpression(const unsigned int channel, const unsigned int expression)
	{
		this->knobSets[channel].setExpression(expression);
		this->updateVoiceAttenuationsForChannel(channel);
	}
	void setChannelPitchBend(const unsigned int channel, const unsigned int pitchBend)
	{
		this->knobSets[channel].setPitchBend(pitchBend);
		this->updateVoicePitchBendsForChannel(channel);
	}
	void setChannelPan(const unsigned int channel, const unsigned int pan)
	{
		this->knobSets[channel].setPan(pan);
		this->updateVoicePansForChannel(channel);
	}
	void setChannelVibrato(const unsigned int channel, const unsigned int vibrato)
	{
		this->knobSets[channel].setVibratoCents(vibrato);
	}
	void setChannelPressure(const unsigned int channel, const unsigned int pressure)
	{
		this->knobSets[channel].setChannelPressureCents(pressure);
	}

	WavetableOscillator()
	{
		for (unsigned int channel{ 0 }; channel < 16; ++channel)
		{
			this->knobSets[channel] = ChannelKnobSet(converter);
		}
	}
	std::vector<Voice> voices;
	std::vector<int> samplePool;
	void setSamplePool(const std::vector<int>& samplePool)
	{
		this->samplePool = samplePool;
	}
	void playVoice(const VoiceProperties& voiceProperties, const SoundfontFile::SampleHeader& sampleHeader)
	{
		this->voices.push_back(Voice{ voiceProperties, sampleHeader, this->converter, this->samplingTimeInterval });
	}
	void eraseInactiveVoices() {
		this->voices.erase(std::remove_if(this->voices.begin(), this->voices.end(), [&](const auto& voice)
			{
				return voice.sample.dataPoint > voice.sample.endAddress || voice.volEnv.isFinished();
			}), this->voices.end());
	}
	void setSamplesPerSecond(const unsigned int samplesPerSecond) {
		// storing the reciprocal is an optimization
		// this (typically constant) value will later be divided by, and as division tends to be slower than multiplication
		// multiplying with the reciprocal is faster than dividing by it
		this->samplingTimeInterval = 1.0f / samplesPerSecond;
	}
	StereoPair nextAmp(const double elapsedTime, const double midiElapsedTime)
	{
		StereoPair amp{ 0.0, 0.0 };


		for (Voice& voice : this->voices)
		{
			const float attenuationFactor{ /*voice.getCombinedAttenuationFactor()*/ voice.attenuationFactorIntrinsic * this->knobSets[voice.channel].getAttenuationFactor() };

			if (voice.midiEndTime <= midiElapsedTime)
			{
				if (!(voice.volEnv.isReleasing() || voice.volEnv.isFinished()))
				{
					voice.volEnv.release(elapsedTime);
				}
			}

			voice.vibLfo.update(elapsedTime);
			voice.modLfo.update(elapsedTime);

			voice.volEnv.update(elapsedTime);

			//float pan{ voice.getCombinedPan() };
			float pan{ voice.panIntrinsic + this->knobSets[voice.channel].getPanFactor() };
			if (pan < -0.5f) {
				pan = -0.5f;
			} else if (pan > 0.5f) {
				pan = 0.5f;
			}

			if (voice.sample.dataPoint <= voice.sample.endAddress) {
				//float f1{ 360.3f };
				//float f2{ 368.4f };
				//double x{ voice.sample.dataPoint - voice.sample.startAddress };
				//float sampleValue{ attenuationFactor * 1.0f * (sinf((f1 + f2) / 2 * x) * sinf((f1 - f2) / 2 * x)) * 256.0f * 256.0f * 256.0f /** voice.volEnv.getCurrentValue()*/ };
				float sampleValue{ attenuationFactor * samplePool[int(voice.sample.dataPoint)] * voice.volEnv.getCurrentValue() * powf(10.0f, voice.modLfo.getCurrentValue() * voice.modLfoToVolume / 200.0f) };
				amp.right += (0.5f + pan) * sampleValue;
				amp.left += (0.5f - pan) * sampleValue;
			}

			float vibLfoCentDeviation{ voice.vibLfo.getCurrentValue() * (voice.vibLfoToPitchCents + 50.0f * this->knobSets[voice.channel].getTotalVibratoCents() / 128.0f) };
			float modLfoCentDeviation{ voice.modLfo.getCurrentValue() * voice.modLfoToPitchCents };

			voice.sample.dataPoint += voice.getSampleDataPointsPerAmp() * powf(2.0f, (vibLfoCentDeviation + modLfoCentDeviation) / 1200.0f);
			//float rootPitch{ converter.semitoneToPitchFactor(voice.key) };
			//float C0{ 8.18 };
			//voice.sample.dataPoint += 6.28f * C0 * rootPitch * this->knobSets[voice.channel].getPitchBendFactor() * this->samplingTimeInterval;

			voice.clipDataPoint(midiElapsedTime);
		}

		this->eraseInactiveVoices();

		return amp;
	}
};
