#include "VisualNote.h"

class VisualNoteDrawer
{
private:
	Shader shader{
		{
		{"project/shaders/visualNote/VS.glsl", GL_VERTEX_SHADER},
		{"project/shaders/visualNote/FS.glsl", GL_FRAGMENT_SHADER}
		}
	};
	unsigned int vao;
	unsigned int vbo;
	unsigned int noteCount;
public:
	VisualNoteDrawer()
	{
		glGenVertexArrays(1, &(this->vao));
		glGenBuffers(1, &(this->vbo));

		glBindVertexArray(this->vao);
		glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
		
		glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, 3 * sizeof(unsigned int), 0);
		glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, 3 * sizeof(unsigned int), (void*)sizeof(unsigned int));
		glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, 3 * sizeof(unsigned int), (void*)(2 * sizeof(unsigned int)));
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		glBindVertexArray(0);
	}
	~VisualNoteDrawer()
	{
		glDeleteVertexArrays(1, &(this->vao));
		glDeleteBuffers(1, &(this->vbo));
	}
	void bake(const std::vector<VisualNote>& notes)
	{
		this->noteCount = notes.size();
		std::vector<unsigned int> bakedNotes;
		bakedNotes.reserve(6 * noteCount);
		for (const VisualNote& note : notes)
		{
			bakedNotes.push_back(note.start);
			bakedNotes.push_back(note.key);
			bakedNotes.push_back(note.channel);
			bakedNotes.push_back(note.end);
			bakedNotes.push_back(note.key);
			bakedNotes.push_back(note.channel);
		}
		glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
		glBufferData(GL_ARRAY_BUFFER, bakedNotes.size() * sizeof(unsigned int), bakedNotes.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	void draw(const glm::mat4& projection, Cam& cam, const float timeElapsed)
	{
		this->shader.use();
		this->shader.setglmMat4Uniform("VP", projection * cam.getViewMatrix());
		this->shader.setglmMat4Uniform("M", glm::mat4(1.0f));
		this->shader.setFloatUniform("timeElapsed", timeElapsed);
		glBindVertexArray(this->vao);
		glDrawArrays(GL_LINES, 0, 2 * noteCount);
		glBindVertexArray(0);
		this->shader.unuse();
	}
};
