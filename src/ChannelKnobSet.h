class ChannelKnobSet {
private:
	UnitConverter* converter;
	unsigned int volume;
	unsigned int expression;
	float attenuationFactor;
	unsigned int pitchBend;
	float pitchBendFactor;
	unsigned int pan;
	float panFactor;
	float vibratoCents;
	float channelPressureCents;
	float totalVibratoCents;

	static constexpr unsigned int defaultVolume{ 100 };
	static constexpr unsigned int defaultExpression{ 127 };
	static constexpr unsigned int defaultPitchBend{ 0x2000 };
	static constexpr unsigned int defaultPan{ 64 };
	static constexpr unsigned int defaultVibrato{ 0 };
	static constexpr unsigned int defaultChannelPressure{ 0 };

private:
	void updateAttenuationFactor()
	{
		const float attenuation{ this->converter->velocityToAttenuation(this->volume) + this->converter->velocityToAttenuation(this->expression) };
		this->attenuationFactor = powf(10.0f, -attenuation / 200.0f);
	}
	void updatePitchBendFactor()
	{
		this->pitchBendFactor = powf(2.0f, (this->pitchBend / float(0x1000) - 2.0f) / 12.0f);
	}
	void updatePanFactor()
	{
		this->panFactor = 2.0f * this->pan / 128.0f - 1.0f;
	}
	void updateTotalVibratoCents()
	{
		this->totalVibratoCents = this->vibratoCents + this->channelPressureCents;
	}
	void resetVolumeAndExpression()
	{
		this->volume = defaultVolume;
		this->expression = defaultExpression;
		this->updateAttenuationFactor();
	}
	void resetPitchBend()
	{
		this->setPitchBend(defaultPitchBend);
	}
	void resetPan()
	{
		this->setPan(defaultPan);
	}
	void resetVibratoAndChannelPressure()
	{
		this->vibratoCents = defaultVibrato;
		this->channelPressureCents = defaultChannelPressure;
		this->updateTotalVibratoCents();
	}
public:
	unsigned int getVolume() const {
		return volume;
	}
	unsigned int getExpression() const {
		return expression;
	}
	float getAttenuationFactor() const {
		return attenuationFactor;
	}
	unsigned int getPitchBend() const {
		return pitchBend;
	}
	float getPitchBendFactor() const {
		return pitchBendFactor;
	}
	unsigned int getPan() const {
		return pan;
	}
	float getPanFactor() const {
		return panFactor;
	}
	unsigned int getVibratoCents() const {
		return vibratoCents;
	}
	unsigned int getChannelPressureCents() const {
		return channelPressureCents;
	}
	float getTotalVibratoCents() const {
		return totalVibratoCents;
	}

	void setVolume(const unsigned int volume)
	{
		this->volume = volume;
		this->updateAttenuationFactor();
	}
	void setExpression(const unsigned int expression)
	{
		this->expression = expression;
		this->updateAttenuationFactor();
	}
	void setPitchBend(const unsigned int pitchBend)
	{
		this->pitchBend = pitchBend;
		this->updatePitchBendFactor();
	}
	void setPan(const unsigned int pan)
	{
		this->pan = pan;
		this->updatePanFactor();
	}
	void setVibratoCents(const unsigned int vibrato)
	{
		this->vibratoCents = vibrato;
		this->updateTotalVibratoCents();
	}
	void setChannelPressureCents(const unsigned int channelPressure)
	{
		this->channelPressureCents = channelPressure;
		this->updateTotalVibratoCents();
	}

	void resetAllKnobs() {
		this->resetVolumeAndExpression();
		this->resetPitchBend();
		this->resetPan();
		this->resetVibratoAndChannelPressure();
	}

	ChannelKnobSet()
	{
	}
	ChannelKnobSet(UnitConverter& converter)
		:converter(&converter) {
		this->resetAllKnobs();
	}
};
