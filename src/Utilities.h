#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image/stb_image.h>



#include <iostream>

#ifndef REDUNDANT_UTILITIES
#define REDUNDANT_UTILITIES

constexpr float planeSize{ 100.0f };

float planeVertices[] = {
	// positions          // texture Coords
	 0.5f * planeSize, -0.5f,  0.5f * planeSize,  4.0f * planeSize, 0.0f,
	-0.5f * planeSize, -0.5f,  0.5f * planeSize,  0.0f, 0.0f,
	-0.5f * planeSize, -0.5f, -0.5f * planeSize,  0.0f, 4.0f * planeSize ,

	 0.5f * planeSize, -0.5f,  0.5f * planeSize,  4.0f * planeSize, 0.0f,
	-0.5f * planeSize, -0.5f, -0.5f * planeSize,  0.0f, 4.0f * planeSize,
	 0.5f * planeSize, -0.5f, -0.5f * planeSize,  4.0f * planeSize, 4.0f * planeSize
};




void frameBufferSizeCallback(GLFWwindow* const window, int width, int height)
{
	if (width < height)
		glViewport((width - height) / 2, 0, height, height);
	else
		glViewport(0, (height - width) / 2, width, width);
}

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

int initialize(GLFWwindow** window)
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	* window = glfwCreateWindow(1920, 1080, "Application Name", NULL, NULL);
	if (*window == NULL)
	{
		std::cout << "Failure in window creation.\n";
		glfwTerminate();
		return 1;
	}
	glfwMakeContextCurrent(*window);
	glfwSetFramebufferSizeCallback(*window, frameBufferSizeCallback);

	if (!gladLoadGLLoader(GLADloadproc(glfwGetProcAddress)))
	{
		std::cout << "Failure in GLAD initialization.\n";
		return 2;
	}
	
	//wireframe mode
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

	glViewport(0, (1080 - 1920) / 2, 1920, 1920);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	return 0;
}

unsigned int loadTexture(const char* name, const std::string& directory)
{
	stbi_set_flip_vertically_on_load(true);
	unsigned int tex{  };
	glGenTextures(1, &tex);
	int width{  };
	int height{  };
	int nrChannels{  };
	unsigned char* data{ stbi_load((directory + '/' + std::string(name)).c_str(), &width, &height, &nrChannels, 0) };

	if (data)
	{
		GLenum format;
		switch (nrChannels)
		{
		case 1: format = GL_RED; break;
		case 3: format = GL_RGB; break;
		case 4: format = GL_RGBA; break;
		default: std::cout << "Unsupported texture format used in file \"" << directory + '/' + std::string(name) << "\"\n"; return tex;
		}
		glBindTexture(GL_TEXTURE_2D, tex);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
		std::cout << "Failure in loading texture from the file \"" << directory + '/' + std::string(name) << "\"\n";

	stbi_image_free(data);
	return tex;
}

#endif