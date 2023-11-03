#pragma once
//#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

void KeyCallback(GLFWwindow* window, int key, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}
	if (key == GLFW_KEY_LEFT && action == GLFW_REPEAT)
	{
		x_offset -= 0.01f;
	}
}