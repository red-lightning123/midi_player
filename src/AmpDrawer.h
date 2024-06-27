#include "Amp.h"

class AmpDrawer
{
private:
	Shader shader{
		{
		{"project/shaders/amp/VS.glsl", GL_VERTEX_SHADER},
		{"project/shaders/amp/FS.glsl", GL_FRAGMENT_SHADER}
		}
	};
	unsigned int vao;
	unsigned int vbo;
	unsigned int currentAmp;
	unsigned int maxAmpCount;
public:
	AmpDrawer()
	{
		this->currentAmp = 0;
		this->maxAmpCount = 2000000;
		glGenVertexArrays(1, &(this->vao));
		glGenBuffers(1, &(this->vbo));

		glBindVertexArray(this->vao);
		glBindBuffer(GL_ARRAY_BUFFER, this->vbo);

		glBufferData(GL_ARRAY_BUFFER, 4 * maxAmpCount * sizeof(float), NULL, GL_DYNAMIC_DRAW);

		glVertexAttribPointer(0, 1, GL_FLOAT, false, 2 * sizeof(float), 0);
		glVertexAttribPointer(1, 1, GL_FLOAT, false, 2 * sizeof(float), (void*)sizeof(float));
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);


		glBindVertexArray(0);
	}
	~AmpDrawer()
	{
		glDeleteVertexArrays(1, &(this->vao));
		glDeleteBuffers(1, &(this->vbo));
	}
	void add(const Amp& amp)
	{
		std::array<float, 4> bakedAmps{  };
		bakedAmps[0] = amp.left;
		bakedAmps[1] = amp.absoluteTime;
		bakedAmps[2] = -amp.right;
		bakedAmps[3] = amp.absoluteTime;
		glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
		glBufferSubData(GL_ARRAY_BUFFER, bakedAmps.size() * currentAmp * sizeof(float), bakedAmps.size() * sizeof(float), bakedAmps.data());
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		++currentAmp;
		if (currentAmp == maxAmpCount) {
			currentAmp = 0;
		}
	}
	void draw(const glm::mat4& projection, Cam& cam, const float timeElapsed)
	{
		this->shader.use();
		this->shader.setglmMat4Uniform("VP", projection * cam.getViewMatrix());
		this->shader.setglmMat4Uniform("M", glm::mat4(1.0f));
		this->shader.setFloatUniform("timeElapsed", timeElapsed);
		glBindVertexArray(this->vao);
		glDrawArrays(GL_LINES, 0, 2 * maxAmpCount);
		glBindVertexArray(0);
		this->shader.unuse();
	}
};
