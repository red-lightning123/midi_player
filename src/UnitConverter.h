class UnitConverter {
private:
	std::array<float, 255> semitoneExponents;
	std::array<float, 128> velToAtt;
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
		velToAtt[0] = -400.0f * -1000.0f; // TODO: this is currently a hack
		for (unsigned int vel{ 1 }; vel < 128; ++vel)
		{
			velToAtt[vel] = -400.0f * log10f(vel / 127.0f);
		}
	}
public:
	UnitConverter()
	{
		initializeSemitoneExponentsArray();
		initializeVelToAttArray();
	}
	float semitoneToPitchFactor(int semitone) const
	{
		return semitoneExponents[127u + semitone];
	}

	float absTimecentsToSecs(int timecents) const
	{
		return pow(2.0f, timecents / 1200.0f);
	}

	float velocityToAttenuation(int velocity) const
	{
		return velToAtt[velocity];
	}
};
