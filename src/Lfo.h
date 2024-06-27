class Lfo {
private:
	enum class Phase
	{
		None,
		Delay,
		Loop
	};
	float currentValue{ 0.0f };
	Phase phase{ Phase::None };
	double startTime{  };
	float delayTime;
	float frequency;
	bool rising{ true };
public:
	void setDelayTime(float delayTime) {
		this->delayTime = delayTime;
	}
	void setFrequency(float frequency) {
		this->frequency = frequency;
	}
	float getCurrentValue() const {
		return this->currentValue;
	}

	void update(const double elapsedTime) {
		if (this->phase == Phase::None)
		{
			this->startTime = elapsedTime;
			this->phase = Phase::Delay;
		}
		if (this->phase == Phase::Delay)
		{
			if (elapsedTime - this->startTime >= this->delayTime)
			{
				this->startTime += this->delayTime;
				this->phase = Phase::Loop;
			}
			else
			{
				this->currentValue = 0.0f;
			}
		}
		if (this->phase == Phase::Loop)
		{
			if (rising) {
				this->currentValue = (elapsedTime - startTime) * frequency;
				if (this->currentValue >= 1.0f) {
					this->currentValue = 1.0f;
					this->startTime = elapsedTime;
					rising = false;
				}
			} else {
				this->currentValue = 1.0f - (elapsedTime - startTime) * frequency;
				if (this->currentValue <= 0.0f) {
					this->currentValue = 0.0f;
					this->startTime = elapsedTime;
					rising = true;
				}
			}
		}
	}
};
