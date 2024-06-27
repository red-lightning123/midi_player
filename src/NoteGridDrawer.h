class NoteGridDrawer
{
private:
	Shader noteOnShader{ {
		{"project/shaders/noteOn/VS.glsl", GL_VERTEX_SHADER},
		{"project/shaders/noteOn/GS.glsl", GL_GEOMETRY_SHADER},
		{"project/shaders/noteOn/FS.glsl", GL_FRAGMENT_SHADER} } };
	Shader noteOffShader{ {
		{"project/shaders/noteOff/VS.glsl", GL_VERTEX_SHADER},
		{"project/shaders/noteOff/GS.glsl", GL_GEOMETRY_SHADER},
		{"project/shaders/noteOff/FS.glsl", GL_FRAGMENT_SHADER} } };
	unsigned int onVao{ 0 };
	unsigned int offVao{ 0 };
	unsigned int noteOffTexture{  };
	unsigned int noteOnTexture{  };
public:
	NoteGridDrawer()
	{
		glGenVertexArrays(1, &(this->onVao));
		glGenVertexArrays(1, &(this->offVao));
		noteOffTexture = loadTexture("off.png", "project/resources/images");
		noteOnTexture = loadTexture("on.png", "project/resources/images");
	}
	~NoteGridDrawer()
	{
		glDeleteTextures(1, &(this->noteOffTexture));
		glDeleteTextures(1, &(this->noteOnTexture));
		glDeleteVertexArrays(1, &(this->onVao));
		glDeleteVertexArrays(1, &(this->offVao));
	}
	void draw(const glm::mat4& projection, Cam& cam, double elapsedTime, std::vector<Synthesizer::ActiveNote>& activeNotes)
	{
		noteOffShader.use();

		noteOffShader.setglmMat4Uniform("VP", projection* cam.getViewMatrix());
		noteOffShader.setglmMat4Uniform("M", glm::mat4(1.0f));

		glBindVertexArray(offVao);
		glBindTexture(GL_TEXTURE_2D, noteOffTexture);
		for (unsigned int channel{ 0 }; channel < 16; ++channel)
		{
			for (int key{ 0 }; key < 128; ++key)
			{
				noteOffShader.setglmVec3Uniform("cpos", glm::vec3(2.0f * (key - 64), 2.0f * channel, 0.0f));
				glDrawArrays(GL_POINTS, 0, 1);
			}
		}

		noteOnShader.use();

		noteOnShader.setglmMat4Uniform("VP", projection * cam.getViewMatrix());
		noteOnShader.setglmMat4Uniform("M", glm::mat4(1.0f));


		glBindVertexArray(onVao);
		glBindTexture(GL_TEXTURE_2D, noteOnTexture);
		
		for (auto& note : activeNotes)
		{
			noteOnShader.setFloatUniform("timer", elapsedTime - note.elapsedTimeAtStart);
			noteOnShader.setFloatUniform("channel", note.channel);
			noteOnShader.setglmVec3Uniform("cpos", glm::vec3(2.0f * (note.key - 64), 2.0f * note.channel, 0.1f));
			glDrawArrays(GL_POINTS, 0, 1);
			noteOnShader.setglmVec3Uniform("cpos", glm::vec3(2.0f * (note.key - 64), 2.0f * note.channel, -0.1f));
			glDrawArrays(GL_POINTS, 0, 1);
		}
		glBindVertexArray(0);
		this->noteOnShader.unuse();
	}
};
