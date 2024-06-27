#include <glad/glad.h>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>

#ifndef SHADER_H
#define SHADER_H

class Shader
{
public:
	struct ShaderInfo
	{
		std::string path;
		GLenum type;
	};
private:
	std::string getShaderTypeName(const GLenum type)
	{
		std::string shaderName{ "unknown" };

		switch (type)
		{
		case GL_VERTEX_SHADER: shaderName = "vertex"; break;
		case GL_TESS_CONTROL_SHADER: shaderName = "tessellation control"; break;
		case GL_TESS_EVALUATION_SHADER: shaderName = "tessellation evaluation"; break;
		case GL_GEOMETRY_SHADER: shaderName = "geometry"; break;
		case GL_FRAGMENT_SHADER: shaderName = "fragment"; break;
		}
		return shaderName;
	}

	std::string readFile(const std::string& path)
	{
		std::ifstream file;
		file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		file.open(path);
		std::stringstream stream;
		stream << file.rdbuf();
		file.close();
		return stream.str();
	}

	unsigned int loadShader(const ShaderInfo& shaderInfo)
	{
		std::string shaderTypeName{ getShaderTypeName(shaderInfo.type) };

		std::ifstream file;
		file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		std::string source;
		try
		{
			source = readFile(shaderInfo.path);
		}
		catch (const std::ifstream::failure& e)
		{
			std::cerr << shaderTypeName << " shader loader error - parsing failure: \n" << e.what() << '\n';
		}
		const char* shaderCode{ source.c_str() };
		int success{ 0 };
		char infoLog[512];

		unsigned int shader{ glCreateShader(shaderInfo.type) };
		glShaderSource(shader, 1, &shaderCode, NULL);
		glCompileShader(shader);
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(shader, 512, NULL, infoLog);
			std::cerr << "shader loader error - Failure in " << shaderTypeName << " shader compilation: \n" << infoLog << '\n';
		}
		glAttachShader(ID, shader);
		return shader;
	}
public:
	unsigned int ID;

	Shader(const std::vector<ShaderInfo>& shaderInfos)
		:ID{ glCreateProgram() }
	{
		std::vector<unsigned int> shaderIDs;
		shaderIDs.reserve(shaderInfos.size());
		for (ShaderInfo shaderInfo : shaderInfos)
			shaderIDs.push_back(this->loadShader(shaderInfo));
		glLinkProgram(ID);
		int linkingSuccess;
		glGetProgramiv(ID, GL_LINK_STATUS, &linkingSuccess);
		if (!linkingSuccess)
		{
			char infoLog[512];
			glGetProgramInfoLog(ID, 512, NULL, infoLog);
			std::cerr << "Shader constructor error - failure in linking shader program: \n" << infoLog << '\n';
		}
		for (const unsigned int shader : shaderIDs)
			glDeleteShader(shader);
	}
	~Shader()
	{
		glDeleteProgram(ID);
	}
	void use()
	{
		glUseProgram(ID);
	}
	void unuse()
	{
		glUseProgram(0);
	}
	void setBoolUniform(const std::string& name, const bool value) const
	{
		glUniform1i(glGetUniformLocation(ID, name.c_str()), int(value));
	}
	void setIntUniform(const std::string& name, const int value) const
	{
		glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
	}
	void setFloatUniform(const std::string& name, const float value) const
	{
		glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
	}
	void setglmMat4Uniform(const std::string& name, const glm::mat4& value)
	{
		glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
	}
	void setglmVec3Uniform(const std::string& name, const glm::vec3& value)
	{
		glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value));
	}
};

#endif
