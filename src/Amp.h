struct Amp {
	unsigned int absoluteTime;
	float left;
	float right;
	Amp(unsigned int absoluteTime, float left, float right)
		:absoluteTime(absoluteTime), left(left), right(right) {
	}
};
