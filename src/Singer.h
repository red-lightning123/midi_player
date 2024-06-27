#include <ft2build.h>
#include FT_FREETYPE_H
#define FT_NO_ERROR 0

class Singer
{
private:
	std::string currentLyric{ "" };
	std::mutex currentLyricMutex{ };
	Shader lyricShader;
	unsigned int vao, vbo;

	struct Glyph
	{
		unsigned int textureID;
		glm::ivec2 size;
		glm::ivec2 bearing;
		unsigned int advance;
	};
	std::unordered_map<unsigned long, Glyph> characterMap;
	FT_Face font;
public:
	Singer(const FT_Library& freetype, const std::string& fontPath)
		:lyricShader
		{
			{
				{ "project/shaders/lyric/VS.glsl", GL_VERTEX_SHADER},
				{ "project/shaders/lyric/FS.glsl", GL_FRAGMENT_SHADER }
			}
		}
	{
		if (FT_New_Face(freetype, fontPath.c_str(), 0, &font) != FT_NO_ERROR)
		{
			std::cout << "Failure loading singer font\n";
			throw 1;
		}

		FT_Set_Pixel_Sizes(font, 0, 96 * 4);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		for (unsigned long character{ 0 }; character < 2560 /* TODO: for low memory usage, we initially only want to load the ascii range i. e. [0, 256) */; ++character)
		{
			if (FT_Load_Char(font, character, FT_LOAD_RENDER) != FT_NO_ERROR)
			{
				std::cout << "Failure loading glyph\n";
				throw 1;
			}
			unsigned int texture;
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, font->glyph->bitmap.width, font->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, font->glyph->bitmap.buffer);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			const Glyph glyph
			{
				texture,
				{ font->glyph->bitmap.width, font->glyph->bitmap.rows },
				{ font->glyph->bitmap_left, font->glyph->bitmap_top },
				static_cast<unsigned int>(font->glyph->advance.x)
			};
			characterMap.insert(std::pair<unsigned long, Glyph>(character, glyph));
		}

		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(0, 2, GL_FLOAT, false, 4 * sizeof(float), 0);
		glVertexAttribPointer(1, 2, GL_FLOAT, false, 4 * sizeof(float), (void*)(2 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		lyricShader.use();
		lyricShader.setglmMat4Uniform("projection", glm::ortho(-960.0f, 960.0f, -960.0f, 960.0f));
		lyricShader.unuse();
	}

	//TODO: add destructor (Remember to delete glyph textures and vaos/vbos, also call FT_Font_Done(this.font))

	void singLyric(const std::string lyric)
	{
		bool lyricContainsPrintableCharacter{ true }; // TODO: this should be false
		for (auto it = utf8::iterator(lyric.begin(), lyric.begin(), lyric.end()); it.base() != lyric.end(); ++it)
		{
			if (*it >= u' ')
			{
				lyricContainsPrintableCharacter = true;
				break;
			}
		}
		if (lyricContainsPrintableCharacter) {
			currentLyricMutex.lock();
			this->currentLyric = lyric;
			currentLyricMutex.unlock();
		}
	}

	void drawLyrics(const glm::vec2& position)
	{
		glBindVertexArray(vao);

		glDisable(GL_DEPTH_TEST);
		
		lyricShader.use();

		currentLyricMutex.lock();

		std::stack<unsigned long> sequence{  };

		unsigned long totalWidth{ 0 };
		int height{ characterMap.at(0x5e2).size.y };

		for (auto it{ utf8::iterator(currentLyric.begin(), currentLyric.begin(), currentLyric.end()) }; it.base() != currentLyric.end(); ++it) {
			sequence.push(*it);
			if (characterMap.count(*it) > 0) {
				const Glyph& glyph{ characterMap.at(*it) };
				totalWidth += glyph.advance / 64.0f;
			}
		}

		glm::vec2 cursor{ position - glm::vec2(totalWidth / 2, height / 2) };

		while (!sequence.empty())
		{
			unsigned long character{ sequence.top() };
			sequence.pop();
			if (character == u'\n')
			{
				cursor.x = position.x;
				cursor.y -= 100.0f;
				continue;
			}

			if (characterMap.count(character) == 0) {
				//FT_Error loadCharResult{ FT_Load_Char(font, character, FT_LOAD_RENDER) };
                        	//if (loadCharResult != FT_NO_ERROR)
                        	//{
                                //	std::cout << "Failure loading glyph: " << std::hex << character << ", error: " << std::dec << loadCharResult << '\n';
                                	//throw 1;
                        	//}
                        	unsigned int texture;
                        	glGenTextures(1, &texture);
                        	glBindTexture(GL_TEXTURE_2D, texture);
                        	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, font->glyph->bitmap.width, font->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, font->glyph->bitmap.buffer);
                        	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                        	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                        	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                        	const Glyph glyph
                        	{
                                	texture,
                                	{ font->glyph->bitmap.width, font->glyph->bitmap.rows },
                                	{ font->glyph->bitmap_left, font->glyph->bitmap_top },
                                	static_cast<unsigned int>(font->glyph->advance.x)
                        	};
                        	characterMap.insert(std::pair<unsigned long, Glyph>(character, glyph));
			}

			if (characterMap.count(character) > 0)
			{
				const Glyph glyph{ characterMap.at(character) };
				
				const glm::vec4 v1{ cursor.x + glyph.bearing.x, cursor.y + glyph.bearing.y - glyph.size.y, 0.0f, 1.0f };
				const glm::vec4 v2{ cursor.x + glyph.bearing.x, cursor.y + glyph.bearing.y, 0.0f, 0.0f };
				const glm::vec4 v3{ cursor.x + glyph.bearing.x + glyph.size.x, cursor.y + glyph.bearing.y - glyph.size.y, 1.0f, 1.0f };
				const glm::vec4 v4{ cursor.x + glyph.bearing.x + glyph.size.x, cursor.y + glyph.bearing.y, 1.0f, 0.0f };

				const float vertices[]
				{
					v1.x, v1.y, v1.z, v1.w,
					v2.x, v2.y, v2.z, v2.w,
					v3.x, v3.y, v3.z, v3.w,
					v4.x, v4.y, v4.z, v4.w,
					v2.x, v2.y, v2.z, v2.w,
					v3.x, v3.y, v3.z, v3.w
				};


				glBindTexture(GL_TEXTURE_2D, glyph.textureID);
				
				glBindBuffer(GL_ARRAY_BUFFER, vbo);
				glBufferData(GL_ARRAY_BUFFER, 4 * 6 * sizeof(float), vertices, GL_STREAM_DRAW);
				glBindBuffer(GL_ARRAY_BUFFER, 0);

				glDrawArrays(GL_TRIANGLES, 0, 6);
				
				glBindTexture(GL_TEXTURE_2D, 0);

				cursor.x += glyph.advance / 64.0f;
			}
		}
		
		currentLyricMutex.unlock();

		lyricShader.unuse();

		glEnable(GL_DEPTH_TEST);

		glBindVertexArray(0);
	}
};
