#pragma once
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include "file.h"
#include <stdio.h>


GLuint CompileShader(const char* vsFilename, const char* fsFilename)
{
	int success;
	char infolog[512];

	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	char* vertexShaderSource = read_file(vsFilename);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infolog);
		fprintf(stderr, "Vertex Shader Comilaation Fail - %s\n", infolog);
	}

	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	char* fragmentShaderSource = read_file(fsFilename);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infolog);
		fprintf(stderr, "Fragment Shader Comilaation Fail - %s\n", infolog);
	}

	unsigned int program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
	glGetShaderiv(program, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(program, 512, NULL, infolog);
		fprintf(stderr, "Shader Program Link Fail - %s\n", infolog);
	}

	free(vertexShaderSource);
	free(fragmentShaderSource);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return program;
}