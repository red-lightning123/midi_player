class FreqDrawer {
private:
	struct Complex {
		float real;
		float imag;
		Complex operator+(Complex& other) {
			return Complex{ this->real + other.real, this->imag + other.imag };
		}
		Complex operator-(Complex& other) {
			return Complex{ this->real - other.real, this->imag - other.imag };
		}
		Complex operator*(Complex& other) {
			return Complex{ this->real * other.real - this->imag * other.imag, this->real * other.imag + this->imag * other.real };
		}
	};

	std::vector<std::vector<Complex>> fftTwiddleLUT;
	/*Complex fftStep(std::vector<float>& signal, unsigned int k, unsigned int size, unsigned int downSampleLevel, unsigned int downSampleFactor, unsigned int shiftFactor) {
		if (size % 2 == 1) {
			if (size == 1) {
				return Complex{ signal[shiftFactor], 0.0f };
			} else {
				std::cout << "can't compute fft of signal with non-power-of-2 length";
				throw;
			}
		} else {
			Complex fft1{ fftStep(signal, k, size / 2, downSampleLevel + 1, downSampleFactor * 2, shiftFactor) };
			Complex fft2{ fftStep(signal, k, size / 2, downSampleLevel + 1, downSampleFactor * 2, shiftFactor + downSampleFactor) };
			Complex twiddle{ fftTwiddleLUT[downSampleLevel][k] };
			fft1.real += twiddle.real * fft2.real;
			fft1.real -= twiddle.imag * fft2.imag;
			fft1.imag += twiddle.real * fft2.imag;
			fft1.imag += twiddle.imag * fft2.real;
			return fft1;
		}
	}

	Complex fft(std::vector<float>& signal, int k) {
		return fftStep(signal, k, signal.size(), 0, 1, 0);
	}*/

	void fftStep(std::vector<Complex>& output, unsigned int outputStart, std::vector<float>& signal, unsigned int size, unsigned int level, unsigned int stride, unsigned int shift) {
		if (size == 1) {
			output[outputStart] = { signal[shift], 0.0f };
		} else {
			fftStep(output, outputStart, signal, size / 2, level + 1, 2 * stride, shift);
			fftStep(output, outputStart + size / 2, signal, size / 2, level + 1, 2 * stride, shift + stride);

			for (int k{ 0 }; k < size / 2; ++k) {
				Complex p{ output[outputStart + k] };
				Complex q{ fftTwiddleLUT[level][k] * output[outputStart + k + size / 2] };
				output[outputStart + k] = p + q;
				output[outputStart + k + size / 2] = p - q;
			}
		}
	}

	std::vector<Complex> fft(std::vector<float>& signal) {
		std::vector<Complex> output(signal.size());
		fftStep(output, 0, signal, signal.size(), 0, 1, 0);
		return output;
	}

	/*std::vector<std::vector<Complex>> dftLUT;
	Complex dft(std::vector<float>& signal, int k) {
		Complex result{ 0.0f, 0.0f };
		for (int i{ 0 }; i < signal.size(); ++i) {
			result += this->dftLUT[k][i] * { signal[i], signal[i] };
		}
		return result;
	}*/

	std::deque<float> amps;
	unsigned int currentFrame;
	unsigned int maxFrameCount;

	Shader shader{
		{
		{"project/shaders/freq/VS.glsl", GL_VERTEX_SHADER},
		{"project/shaders/freq/FS.glsl", GL_FRAGMENT_SHADER}
		}
	};
	unsigned int vao;
	unsigned int vbo;
	unsigned int ampCount;
public:
	void constructFourierTwiddleLUT() {
		unsigned int size{ this->ampCount };
		this->fftTwiddleLUT.resize(std::countr_zero(this->ampCount));
		for (int level{ 0 }; level < this->fftTwiddleLUT.size(); ++level) {
			this->fftTwiddleLUT[level].resize(size / 2);
			for (int k{ 0 }; k < this->fftTwiddleLUT[level].size(); ++k) {
				this->fftTwiddleLUT[level][k] = { cosf(-k * 6.28f / size), sinf(-k * 6.28f / size) };
			}
			size /= 2;
		}
		/*this->dftLUT.resize(this->maxAmpCount);
		for (int k{ 0 }; k < this->maxAmpCount; ++k) {
			this->dftLUT[k].resize(this->maxAmpCount);
			for (int i{ 0 }; i < this->maxAmpCount; ++i) {
				this->dftLUT[k][i].real = cos(-k * (6.28f / this->maxAmpCount) * i);
				this->dftLUT[k][i].imag = sin(-k * (6.28f / this->maxAmpCount) * i);
			}
		}*/
	}

	FreqDrawer()
	{
		this->currentFrame = 0;
		this->maxFrameCount = 2000;
		this->ampCount = 4096;
		this->constructFourierTwiddleLUT();

		while (amps.size() < this->ampCount) {
			this->amps.push_back(0.0f);
		}

		glGenVertexArrays(1, &(this->vao));
		glGenBuffers(1, &(this->vbo));

		glBindVertexArray(this->vao);
		glBindBuffer(GL_ARRAY_BUFFER, this->vbo);

		glBufferData(GL_ARRAY_BUFFER, 12 * amps.size() * maxFrameCount * sizeof(float), NULL, GL_DYNAMIC_DRAW);
		
		glVertexAttribPointer(0, 1, GL_FLOAT, false, 3 * sizeof(float), 0);
		glVertexAttribPointer(1, 1, GL_FLOAT, false, 3 * sizeof(float), (void*)sizeof(float));
		glVertexAttribPointer(2, 1, GL_FLOAT, false, 3 * sizeof(float), (void*)(2 * sizeof(float)));
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		glBindVertexArray(0);
	}
	~FreqDrawer()
	{
		glDeleteVertexArrays(1, &(this->vao));
		glDeleteBuffers(1, &(this->vbo));
	}
	void add(const Amp& amp)
	{
		amps.push_back((amp.left + amp.right) / 2.0f);
		amps.pop_front();
	}
	void nextFrame(const float timeElapsed, const float samplesPerSecond) {
		std::vector<float> signal{ amps.begin(), amps.end() };
		std::vector<Complex> fourier{ fft(signal) };
		std::vector<std::pair<float, float>> freqsIntensities{  };
		freqsIntensities.reserve(signal.size());
		for (unsigned int k{ 0 }; k < signal.size(); ++k) {
			float freq{ (float)(k) / signal.size() * samplesPerSecond };
			float intensity{ sqrtf(fourier[k].real * fourier[k].real + fourier[k].imag * fourier[k].imag) };
			freqsIntensities.push_back({ freq, intensity });
		}

		std::vector<float> bakedFreqs{  };
		bakedFreqs.reserve(12 * amps.size());
		for (unsigned int k{ 0 }; k < signal.size() - 1; ++k) {
			bakedFreqs.push_back(freqsIntensities[k].first);
			bakedFreqs.push_back(freqsIntensities[k].second);
			bakedFreqs.push_back(timeElapsed);
			bakedFreqs.push_back(freqsIntensities[k + 1].first);
			bakedFreqs.push_back(freqsIntensities[k + 1].second);
			bakedFreqs.push_back(timeElapsed);

			bakedFreqs.push_back(freqsIntensities[k].first);
			bakedFreqs.push_back(0.0f);
			bakedFreqs.push_back(timeElapsed);//sqrt(fourier[k].real * fourier[k].real + fourier[k].imag * fourier[k].imag));
			bakedFreqs.push_back(freqsIntensities[k].first);
			bakedFreqs.push_back(0.0f);
			bakedFreqs.push_back(timeElapsed);
		}
		glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
		glBufferSubData(GL_ARRAY_BUFFER, bakedFreqs.size() * currentFrame * sizeof(float), bakedFreqs.size() * sizeof(float), bakedFreqs.data());
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		++currentFrame;
		if (currentFrame == maxFrameCount) {
			currentFrame = 0;
		}
	}
	void draw(const glm::mat4& projection, Cam& cam, const float timeElapsed)
	{
		this->shader.use();
		this->shader.setglmMat4Uniform("VP", projection * cam.getViewMatrix());
		this->shader.setglmMat4Uniform("M", glm::mat4(1.0f));
		this->shader.setFloatUniform("timeElapsed", timeElapsed);
		glBindVertexArray(this->vao);
		glDrawArrays(GL_LINES, 0, 4 * amps.size() * maxFrameCount);
		glBindVertexArray(0);
		this->shader.unuse();
	}
};
