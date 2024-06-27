class PianoDrawer
{
private:
	Shader whiteKeysShader{ {
		{"project/shaders/whitePiano/VS.glsl", GL_VERTEX_SHADER},
		{"project/shaders/whitePiano/GS.glsl",GL_GEOMETRY_SHADER},
		{"project/shaders/whitePiano/FS.glsl", GL_FRAGMENT_SHADER} } };
	Shader blackKeysShader{ {
		{"project/shaders/blackPiano/VS.glsl", GL_VERTEX_SHADER},
		{"project/shaders/blackPiano/GS.glsl",GL_GEOMETRY_SHADER},
		{"project/shaders/blackPiano/FS.glsl", GL_FRAGMENT_SHADER} } };
	unsigned int whiteKeysVao{ 0 };
	unsigned int blackKeysVao{ 0 };
	enum KeyColor {
		White,
		Black
	};
	struct KeyPosition {
		unsigned int midi;
		int cmaj;
	};
	struct Key {
		KeyPosition position;
		KeyColor color;
	};
	std::vector<KeyPosition> whiteKeyPositions{  };
	std::vector<KeyPosition> blackKeyPositions{  };
	void assignKeys() {
		// generate the basic octave pattern
		int cmajOffset{ 27 };
		std::array<Key, 12> octavePattern{  };
		for (unsigned int patternIndex{ 0 }; patternIndex < octavePattern.size(); ++patternIndex)
		{
			KeyColor color{ KeyColor::White };
			switch (patternIndex)
			{
			// these cases correspond to black keys on the keyboard
			//   0 1 2 3 4   5 6 7 8 9 1011
			// |   #   #   |   #   #   #   |
			// |   #   #   |   #   #   #   |
			// |   |   |   |   |   |   |   |
			case 1:
			case 3:
			case 6:
			case 8:
			case 10:
				color = KeyColor::Black;
			}
			Key key{ { patternIndex, cmajOffset }, color };
			octavePattern[patternIndex] = key;
			if (key.color == KeyColor::White) {
				cmajOffset += 1;
			}
		}

		// generate each octave from the pattern
		for (unsigned int octave{ 0 }; octavePattern.size() * octave < 128; ++octave) {
			for (Key& key : octavePattern) {
				KeyPosition position{ key.position };
				position.midi += octavePattern.size() * octave;
				if (position.midi >= 128) {
					continue;
				}
				position.cmaj += 7 * octave;
				if (key.color == KeyColor::White) {
					this->whiteKeyPositions.push_back(position);
				} else {
					this->blackKeyPositions.push_back(position);
				}
			}
		}
	}
public:
	PianoDrawer()
	{
		this->assignKeys();
		glGenVertexArrays(1, &(this->whiteKeysVao));
		glGenVertexArrays(1, &(this->blackKeysVao));
	}
	~PianoDrawer()
	{
		glDeleteVertexArrays(1, &(this->whiteKeysVao));
		glDeleteVertexArrays(1, &(this->blackKeysVao));
	}
	void draw(const glm::mat4& projection, Cam& cam, std::vector<Synthesizer::ActiveNote>& activeNotes)
	{
		std::array<char, 128> pressedKeys{  };
		for (auto note : activeNotes)
		{
			pressedKeys[note.key] = note.channel + 1;
		}
		whiteKeysShader.use();

		whiteKeysShader.setglmMat4Uniform("VP", projection * cam.getViewMatrix());

		glBindVertexArray(whiteKeysVao);
		for (KeyPosition& keyPosition : this->whiteKeyPositions) {
			whiteKeysShader.setIntUniform("channel", pressedKeys[keyPosition.midi]);
			whiteKeysShader.setglmMat4Uniform("M", glm::translate(glm::mat4(1.0f), glm::vec3(24.0f / 7.0f * (keyPosition.cmaj - 64), -2.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), (pressedKeys[keyPosition.midi] != 0 ? 32.0f : 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 50.0f / 165.0f * 7.0f)));
			glDrawArrays(GL_POINTS, 0, 1);
		}
		blackKeysShader.use();

		blackKeysShader.setglmMat4Uniform("VP", projection * cam.getViewMatrix());

		glBindVertexArray(blackKeysVao);
		for (KeyPosition& keyPosition : this->blackKeyPositions) {
			blackKeysShader.setIntUniform("channel", pressedKeys[keyPosition.midi]);
			blackKeysShader.setglmMat4Uniform("M", glm::translate(glm::mat4(1.0f), glm::vec3(24.0f / 7.0f * (keyPosition.cmaj - 64.5), -1.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), (pressedKeys[keyPosition.midi] != 0 ? 32.0f : 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 50.0f / 165.0f * 7.0f)));
			glDrawArrays(GL_POINTS, 0, 1);
		}
		glBindVertexArray(0);
		this->blackKeysShader.unuse();
	}
};
