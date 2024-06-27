class VolEnv {
private:
	enum class Phase
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
	float currentValue{ 0.0f };
	Phase phase{ Phase::None };
	double phaseStartTime{  };
	class LogNumeric {
	private:
		float valueNormal;
		float valueLog10;
	public:
		void setTo(float valueNormal) {
			this->valueNormal = valueNormal;
			this->valueLog10 = log10f(valueNormal);
		}
		void setToLog10(float valueLog10) {
			this->valueNormal = powf(10.0f, valueLog10);
			this->valueLog10 = valueLog10;
		}

		float get() {
			return this->valueNormal;
		}
		float getLog10() {
			return this->valueLog10;
		}
	};
	LogNumeric phaseStartValue{  };
	float delayTime;
	float attackTime;
	float holdTime;
	float decayTime;
	int sustainValue;
	float releaseTime;
public:
	void setDelayTime(float delayTime) {
		this->delayTime = delayTime;
	}
	void setAttackTime(float attackTime) {
		this->attackTime = attackTime;
	}
	void setHoldTime(float holdTime) {
		this->holdTime = holdTime;
	}
	void setDecayTime(float decayTime) {
		this->decayTime = decayTime;
	}
	void setSustainValue(int sustainValue) {
		this->sustainValue = sustainValue;
	}
	void setReleaseTime(float releaseTime) {
		this->releaseTime = releaseTime;
	}
	float getCurrentValue() const {
		return this->currentValue;
	}
	void release(const double elapsedTime) {
		this->phaseStartTime = elapsedTime;
		this->phase = Phase::Release;
		this->phaseStartValue.setTo(this->currentValue);
	}
	bool isReleasing() const {
		return this->phase == Phase::Release;
	}
	bool isFinished() const {
		return this->phase == Phase::Finished;
	}

	void update(const double elapsedTime) {
		if (this->phase == Phase::None)
		{
			this->phaseStartTime = elapsedTime;
			this->phase = Phase::Delay;
		}
		if (this->phase == Phase::Delay)
		{
			if (elapsedTime - this->phaseStartTime >= this->delayTime)
			{
				this->phaseStartTime += this->delayTime;
				this->phase = Phase::Attack;
			}
			else
			{
				this->currentValue = 0.0f;
			}
		}
		if (this->phase == Phase::Attack)
		{
			if (elapsedTime - this->phaseStartTime >= this->attackTime)
			{
				this->phaseStartTime += this->attackTime;
				this->phase = Phase::Hold;
			}
			else
			{
				this->currentValue = (elapsedTime - this->phaseStartTime) / this->attackTime;
			}
		}
		if (this->phase == Phase::Hold)
		{
			if (elapsedTime - this->phaseStartTime >= this->holdTime)
			{
				this->phaseStartTime += this->holdTime;
				this->phase = Phase::Decay;
				this->phaseStartValue.setToLog10(0.0f);
			}
			else
			{
				this->currentValue = 1.0f;
			}
		}
		if (this->phase == Phase::Decay)
		{
			if (20.0f * this->phaseStartValue.getLog10() - 100.0f * (elapsedTime - this->phaseStartTime) / this->decayTime <= -std::max(this->sustainValue, 0) / 10.0f)
			{
				this->phaseStartTime += this->decayTime;
				this->phase = Phase::Sustain;
				this->phaseStartValue.setToLog10(-std::max(0, this->sustainValue) / 200.0f);
			}
			else
			{
				this->currentValue = pow(10.0f, (20.0f * this->phaseStartValue.getLog10() - 100.0f * (elapsedTime - this->phaseStartTime) / this->decayTime) / 20.0f);
			}
		}

		if (this->phase == Phase::Sustain)
		{
			this->currentValue = this->phaseStartValue.get();
		}

		if (this->phase == Phase::Release)
		{
			if (20.0f * this->phaseStartValue.getLog10() - 100.0f * (elapsedTime - this->phaseStartTime) / this->releaseTime <= -100.0f)
			{
				this->phase = Phase::Finished;
				this->currentValue = 0.0f;
			}
			else
			{
				this->currentValue = pow(10.0f, (20.0f * this->phaseStartValue.getLog10() - 100.0f * (elapsedTime - this->phaseStartTime) / this->releaseTime) / 20.0f);
			}
		}
	}
};
