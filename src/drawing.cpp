#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include <GL/glew.h>

#include "drawing.h"


Shader::Shader() : progInit(false) {
	progId = glCreateProgram();
}

unsigned int Shader::getProgramId() {
	if (!progInit) {
		std::cerr << "Shader program has not been linked yet." << std::endl;
	}

	return progId;
}

void Shader::addShader(std::string path, GLenum type) {
	// if program has been linked, don't do anything
	if (progInit) {
		std::cerr << "Cannot add shader, program has already been linked." << std::endl;
		return;
	}

	// use filestream to access path
	std::ifstream fileStream;
	fileStream.open(path);

	if (!fileStream.is_open()) {
		std::cerr << ("Error adding shader: could not open file at path \"" + path + "\"") << std::endl;
		return;
	}

	// transfer filestream data to c string
	std::stringstream strStream;
	strStream << fileStream.rdbuf();
	std::string sourceCode = strStream.str();
	const char* sourceCodeC = sourceCode.c_str();

	int shaderId = glCreateShader(type);
	glShaderSource(shaderId, 1, &sourceCodeC, nullptr);
	glCompileShader(shaderId);

	// error check compilation
	int successStatus;
	glGetShaderiv(shaderId, GL_COMPILE_STATUS, &successStatus);
	if (!successStatus) {
		char log[512];
		glGetShaderInfoLog(shaderId, 512, nullptr, log);
		std::cerr << "Error compiling shader." << std::endl;
		std::cerr << log << std::endl;
		return;
	}

	glAttachShader(progId, shaderId);
}

void Shader::linkProgram() {
	// create id
	glLinkProgram(progId);

	// error checking
	int successStatus;
	glGetProgramiv(progId, GL_LINK_STATUS, &successStatus);
	if (!successStatus) {
		char log[512];
		glGetProgramInfoLog(progId, 512, nullptr, log);

		std::cerr << "Error linking shader program." << std::endl;
		std::cerr << log << std::endl;
		return;
	}

	progInit = true;
}