#include <GL/gl3w.h>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

#include "camera.h"
#include "error.h"
#include "shader.h"
#include "shadow.h"
#include "texture.h"
#include "casteljau.h"
#include "objParser.h"
#include <string>
 
#include <vector>
#define _USE_MATH_DEFINES

#include <math.h>

#include <fstream>
#include "tankVerts.cpp"

SCamera camera;

#define NUM_BUFFERS 27
#define NUM_VAOS 27
#define NUMTEXTURES 10

GLuint Buffers[NUM_BUFFERS];
GLuint VAOs[NUM_VAOS];

int WIDTH = 1024;
int HEIGHT = 768;

#define SH_MAP_WIDTH 2048
#define SH_MAP_HEIGHT 2048
//#define SH_MAP_WIDTH 4096
//#define SH_MAP_HEIGHT 4096

#define NUM_LIGHTS 3

glm::vec3 lightDirection[NUM_LIGHTS];
glm::vec3 lightPos[NUM_LIGHTS];

glm::vec3 initSunPos = glm::vec3(0.f, 0.f, 19.f);
glm::vec3 sunRotation = glm::vec3(-2.f, 0.f, 0.f);

int lightType[] = { 1 , 2  , 2 };
float attConst[] = { 1.f, 1.5f, 1.5f };
float attLin[] = { 0.01f, 0.01f, 0.02f, };
float attQuad[] = { 0.001f, 0.001f, 0.002f, };

#define RADIUS 1
#define NUM_SECTORS 30
#define NUM_STACKS 30
#define SPHERE_VERTS (8 * 3 * 2 * NUM_SECTORS * (NUM_STACKS - 1))

int cameraType = 0;

int flashLight = 0;

#define CANNONBALL_SPEED 0.7;

float cannonBallPos = -1;


void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// Window closes on esc
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	// Space will fire/reset a cannonball
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
		if (cannonBallPos == -1) {
			cannonBallPos = 0;
		}
		else if (cannonBallPos == 0) {
			cannonBallPos = 0.1;
		}
		else {
			cannonBallPos = -1;
		}
	}
	// Left alt changes the sun between light types (positional, spotlight, directional)
	if (key == GLFW_KEY_LEFT_ALT && action == GLFW_PRESS) {
		lightType[0] = (lightType[0] + 1) % 3;
	}
	// Tab swaps between camera types
	if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
		cameraType = (cameraType + 1) % 2;
		if (cameraType == 0) {
			//InitCamera(camera);
			cam_dist = glm::distance(glm::vec3(0.f, 0.f, 0.f), camera.Position);
			glm::vec3 direction = glm::normalize(camera.Position);

			camera.Yaw = glm::degrees(std::atan2(direction.z, direction.x));
			camera.Pitch = glm::degrees(std::asin(direction.y));

			MoveAndOrientCamera(camera, glm::vec3(0.f, 0.f, 0.f), cam_dist, 0, 0);
		}
		else {
			glfwSetCursorPos(window, WIDTH / 2, HEIGHT / 2);
		}
	}
	float x_offset = 0.f;
	float y_offset = 0.f;
	bool cam_changed = false;
	switch (cameraType) {
	case 0:// MODEL VIEWER
		if ((key == GLFW_KEY_RIGHT || key == GLFW_KEY_D) && action == GLFW_REPEAT) {
			x_offset = 1.f;
			y_offset = 0.f;
			cam_changed = true;
		}
		if ((key == GLFW_KEY_LEFT || key == GLFW_KEY_A) && action == GLFW_REPEAT) {
			x_offset = -1.f;
			y_offset = 0.f;
			cam_changed = true;
		}if ((key == GLFW_KEY_UP || key == GLFW_KEY_W) && action == GLFW_REPEAT) {
			x_offset = 0.f;
			y_offset = -1.f;
			cam_changed = true;
		}
		if ((key == GLFW_KEY_DOWN || key == GLFW_KEY_S) && action == GLFW_REPEAT) {
			x_offset = 0.f;
			y_offset = 1.f;
			cam_changed = true;
		}
		if (key == GLFW_KEY_R && action == GLFW_REPEAT) {
			cam_dist -= 0.1f;
			cam_changed = true;
		}
		if (key == GLFW_KEY_F && action == GLFW_REPEAT) {
			cam_dist += 0.1f;
			cam_changed = true;
		}

		if (cam_changed)
		{
			MoveAndOrientCamera(camera, glm::vec3(0.f, 0.f, 0.f), cam_dist, x_offset, y_offset);
		}
		break;
	case 1: // Fly Through

		float f_offset = 0.f;
		float r_offset = 0.f;
		if ((key == GLFW_KEY_RIGHT || key == GLFW_KEY_D) && action == GLFW_REPEAT) {
			r_offset = 1.f;
			cam_changed = true;
		}
		if ((key == GLFW_KEY_LEFT || key == GLFW_KEY_A) && action == GLFW_REPEAT) {
			r_offset = -1.f;
			cam_changed = true;
		}if ((key == GLFW_KEY_UP || key == GLFW_KEY_W) && action == GLFW_REPEAT) {
			f_offset = 1.f;
			cam_changed = true;
		}
		if ((key == GLFW_KEY_DOWN || key == GLFW_KEY_S) && action == GLFW_REPEAT) {
			f_offset = -1.f;
			cam_changed = true;
		}

		if (cam_changed)
		{
			MoveFlyThruCamera(camera, f_offset, r_offset);
		}
		break;
	}

}

void MouseMoveCallback(GLFWwindow* window, double xpos, double ypos) {
	// if using the flythrough camera, moving the mouse looks around
	if (cameraType == 1) {
		float x_offset, y_offset;
		glfwSetCursorPos(window, WIDTH / 2, HEIGHT / 2);

		x_offset = (float)WIDTH / 2 - xpos;
		y_offset = (float)HEIGHT / 2 - ypos;

		OrientFlyThruCamera(camera, x_offset, y_offset);
	}
}

void SizeCallback(GLFWwindow* window, int w, int h)
{
	// Resizes the drawn window when resizing the actual window
	WIDTH = w;
	HEIGHT = h;
	glViewport(0, 0, w, h);
}

void addVertex(float* vertexArr, int& addIndex, float* vertices, int index)
{
	vertexArr[addIndex++] = vertices[index++];
	vertexArr[addIndex++] = vertices[index++];
	vertexArr[addIndex++] = vertices[index++];

	vertexArr[addIndex++] = vertices[index++];
	vertexArr[addIndex++] = vertices[index++];
	vertexArr[addIndex++] = vertices[index++];

	vertexArr[addIndex++] = vertices[index++];
	vertexArr[addIndex++] = vertices[index++];
}

void addVertex(std::vector<float> &vertexArr, float* vertices, int index, int lowerOffset, glm::vec3 nor)
{
	vertexArr.push_back(vertices[index++]);
	vertexArr.push_back(vertices[index++] - lowerOffset);
	vertexArr.push_back(vertices[index++]);
	
	vertexArr.push_back(0);
	vertexArr.push_back(0);
	
	vertexArr.push_back(nor.x);
	vertexArr.push_back(nor.y);
	vertexArr.push_back(nor.z);
}

float* createSphere(bool inverseN)
{
	int verts = 2 * 3 * (NUM_SECTORS + 1) * (NUM_STACKS + 1);
	float* vertices = (float*)malloc(8 * (NUM_STACKS + 1) * (NUM_SECTORS + 1) * sizeof(float));
	if (vertices == NULL)
	{
		printf("Vertices MALLOC failed!\n");
	}
	float x, y, z, xz;
	float nx, ny, nz, lengthInv = 1.0f / RADIUS;
	float u, v;

	float sectorStep = 2 * M_PI / NUM_SECTORS;
	float stackStep = M_PI / NUM_STACKS;
	float sectorAngle, stackAngle;

	int index = 0;
	// Calculate all vertices needed
	for (int i = 0; i <= NUM_STACKS; i++)
	{
		stackAngle = (M_PI / 2.f) - i * stackStep;
		xz = (float)RADIUS * cosf(stackAngle);
		y = (float)RADIUS * sinf(stackAngle);

		for (int j = 0; j <= NUM_SECTORS; j++)
		{
			sectorAngle = j * sectorStep;

			// vertex pos
			x = xz * cosf(sectorAngle);
			z = xz * sinf(sectorAngle);
			vertices[index++] = x;
			vertices[index++] = y;
			vertices[index++] = z;

			// UV coordintaes
			float u = (float)j / NUM_STACKS;
			float v = (float)i / NUM_SECTORS;

			vertices[index++] = u;
			vertices[index++] = v;

			// vertex Normals
			nx = x * lengthInv;
			ny = y * lengthInv;
			nz = z * lengthInv;

			if (inverseN) {
				vertices[index++] = -(nx);
				vertices[index++] = -(ny);
				vertices[index++] = -(nz);
			}
			else {
				vertices[index++] = (nx);
				vertices[index++] = (ny);
				vertices[index++] = (nz);
			}
		}
	}

	// Loop the vertices and connect them into triangles and quads
	float* sphere = (float*)malloc(SPHERE_VERTS * sizeof(float));
	if (sphere == NULL)
	{
		printf("Sphere MALLOC failed!\n");
	}
	int sphereIndex = 0;
	for (int i = 0; i < NUM_STACKS; ++i)
	{
		int vi1 = i * (NUM_SECTORS + 1);
		int vi2 = (i + 1) * (NUM_SECTORS + 1);
		for (int j = 0; j < NUM_SECTORS; ++j, ++vi1, ++vi2)
		{
			int v1, v2, v3, v4;

			// Each sector needs 4 vertices (2 triangles)
			v1 = vi1 * 8;
			v2 = vi2 * 8;
			v3 = vi1 * 8 + 8;
			v4 = vi2 * 8 + 8;

			// Top
			if (i == 0)
			{
				addVertex(sphere, sphereIndex, vertices, v1);
				addVertex(sphere, sphereIndex, vertices, v2);
				addVertex(sphere, sphereIndex, vertices, v4);
			}
			// Bottom
			else if (i == (NUM_STACKS - 1))
			{
				addVertex(sphere, sphereIndex, vertices, v1);
				addVertex(sphere, sphereIndex, vertices, v2);
				addVertex(sphere, sphereIndex, vertices, v3);
			}
			// Middle
			else
			{
				addVertex(sphere, sphereIndex, vertices, v1);
				addVertex(sphere, sphereIndex, vertices, v2);
				addVertex(sphere, sphereIndex, vertices, v4);

				addVertex(sphere, sphereIndex, vertices, v1);
				addVertex(sphere, sphereIndex, vertices, v3);
				addVertex(sphere, sphereIndex, vertices, v4);

			}
		}
	}
	free(vertices);
	return sphere;
}

std::vector<float> createCylinder() {
	// Actually makes a very thin cyl
	std::vector<float> vertices;

	float x, y, z, xz;
	float nx, ny, nz, lengthInv = 1.0f / RADIUS;

	float sectorStep = 2 * M_PI / NUM_SECTORS;
	float sectorAngle;

	y = 0.f;
	// Calculate All vertices needed
	for (int j = 0; j <= NUM_SECTORS; j++)
	{
		sectorAngle = j * sectorStep;

		// vertex pos
		x = RADIUS * cosf(sectorAngle);
		z = RADIUS * sinf(sectorAngle);
		vertices.push_back(x);
		vertices.push_back(y);
		vertices.push_back(z);

		float u = (x + RADIUS) / 2 * RADIUS;
		float v = (z + RADIUS) / 2 * RADIUS;

		vertices.push_back(u * 50.f);
		vertices.push_back(v * 50.f);

		// vertex Normals
		nx = 0.f;
		ny = 1.f;
		nz = 0.f;

		vertices.push_back(nx);
		vertices.push_back(ny);
		vertices.push_back(nz);
	}
	
	std::vector<float> circle;
	for (int i = 0; i < 2; i++) {
	// Connect vertices	
		for (int j = 0; j <= NUM_SECTORS; j++)
		{
			int p1 = j * 8;
			int p2;
			if (j == NUM_SECTORS) {
				p2 = 0;
			}
			else {
				p2 = (j + 1) * 8;
			}

			int norOff = i == 0 ? 1 : -1;

			// Each point connects to middle and next point
			circle.push_back(vertices[p1++]);
			circle.push_back(vertices[p1++] - i);
			circle.push_back(vertices[p1++]);

			circle.push_back(vertices[p1++]);
			circle.push_back(vertices[p1++]);

			circle.push_back(vertices[p1++]);
			circle.push_back(vertices[p1++] * norOff);
			circle.push_back(vertices[p1++]);


			circle.push_back(vertices[p2++]);
			circle.push_back(vertices[p2++] - i);
			circle.push_back(vertices[p2++]);

			circle.push_back(vertices[p2++]);
			circle.push_back(vertices[p2++]);

			circle.push_back(vertices[p2++]);
			circle.push_back(vertices[p2++] * norOff);
			circle.push_back(vertices[p2++]);


			circle.push_back(0.f);
			circle.push_back(0.f - i);
			circle.push_back(0.f);

			circle.push_back(25.f);
			circle.push_back(25.f);

			circle.push_back(0.f);
			circle.push_back(1.f * norOff);
			circle.push_back(0.f);
		}
	}

	// Connects the top and bottom of the cyl
	for (int j = 0; j <= NUM_SECTORS; j++) {
		int p1 = j * 8;
		int p2;
		if (j == NUM_SECTORS) {
			p2 = 0;
		}
		else {
			p2 = (j + 1) * 8;
		}
		// Calculate normals for each of the sides
		glm::vec3 tan = (glm::vec3(vertices[p2], vertices[p2 + 1], vertices[p2 + 2]) - glm::vec3(vertices[p1], vertices[p1 + 1], vertices[p1 + 2]));
		glm::vec3 nor = glm::normalize(glm::vec3(-tan.y, tan.x, tan.z));

		addVertex(circle, vertices.data(), p1, 0, nor);
		addVertex(circle, vertices.data(), p2, 0, nor);
		addVertex(circle, vertices.data(), p2, 1, nor);
											
		addVertex(circle, vertices.data(), p1, 0, nor);
		addVertex(circle, vertices.data(), p1, 1, nor);
		addVertex(circle, vertices.data(), p2, 1, nor);

	}

	return circle;
}

void drawCannonball(unsigned int shaderProgram, GLuint cannonballTex, int index)
{
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[index]);
	glBindVertexArray(VAOs[index]);
	glBindTexture(GL_TEXTURE_2D, cannonballTex);

	glm::mat4 model = glm::mat4(1.f);
	model = glm::translate(model, glm::vec3(4.3f, 2.4f, 0.f));
	model = glm::scale(model, glm::vec3(0.25f, 0.25f, 0.25f));

	// Ball hasn't been fired yet ignore it
	if (cannonBallPos == -1) {

	}
	// if fired then translate to it's current place
	else if (cannonBallPos > 1) {
		model = glm::translate(model, glm::vec3((float)cannonBallPos - 1, (float)(cannonBallPos -1) / 10, 0.f));
		model = glm::rotate(model, (float)cannonBallPos / 5, glm::vec3(1.f, 1.f, 1.f));
	}

	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	int modelType = 0;
	glUniform1i(glGetUniformLocation(shaderProgram, "modelType"), modelType);

	glDrawArrays(GL_TRIANGLES, 0, SPHERE_VERTS);
}

void drawSky(unsigned int shaderProgram, GLuint skyTex, int index)
{
	if (lightType[0] == 0) return;
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[index]);
	glBindVertexArray(VAOs[index]);
	glBindTexture(GL_TEXTURE_2D, skyTex);

	glm::mat4 model = glm::mat4(1.f);
	model = glm::scale(model, glm::vec3(25, 25, 25));
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	int modelType = 1;
	glUniform1i(glGetUniformLocation(shaderProgram, "modelType"), modelType);

	glDrawArrays(GL_TRIANGLES, 0, SPHERE_VERTS);

}

void drawFloor(unsigned int shaderProgram, GLuint floorTex, int index, int verts)
{
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[index]);
	glBindVertexArray(VAOs[index]);
	glBindTexture(GL_TEXTURE_2D, floorTex);

	glm::mat4 model = glm::mat4(1.f);
	model = glm::scale(model, glm::vec3(25.f, 0.1f, 25.f));
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	int modelType = 2;
	glUniform1i(glGetUniformLocation(shaderProgram, "modelType"), modelType);

	glDrawArrays(GL_TRIANGLES, 0, verts);
}

void drawSun(unsigned int shaderProgram, GLuint sunTex, int index)
{
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[index]);
	glBindVertexArray(VAOs[index]);
	glBindTexture(GL_TEXTURE_2D, sunTex);


	glm::mat4 model = glm::mat4(1.f);
	// Rotate sun around the sky
	model = glm::rotate(model, (float)glfwGetTime() / 20, sunRotation);
	model = glm::translate(model, initSunPos);
	model = glm::translate(model, glm::vec3(6.f) * glm::normalize(initSunPos));
	// Reverse the rotation to appear at the same angle
	model = glm::rotate(model, (float)glfwGetTime() / 20, -sunRotation);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	int modelType = 3;
	glUniform1i(glGetUniformLocation(shaderProgram, "modelType"), modelType);

	glDrawArrays(GL_TRIANGLES, 0, SPHERE_VERTS);
}

void drawCannon(unsigned int shaderProgram, GLuint cannonTex, int index, int verts)
{
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[index]);
	glBindVertexArray(VAOs[index]);
	glBindTexture(GL_TEXTURE_2D, cannonTex);

	glm::mat4 model = glm::mat4(1.f);
	model = glm::rotate(model, (float) M_PI/2 , glm::vec3(-1.f, 0.f, 0.f));
	model = glm::scale(model, glm::vec3(.04f, .04f, .04f));

	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	int modelType = 4;
	glUniform1i(glGetUniformLocation(shaderProgram, "modelType"), modelType);

	glDrawArrays(GL_TRIANGLES, 0, verts);
}

void drawTank(unsigned int shaderProgram, int index, int verts)
{
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[index]);
	glBindVertexArray(VAOs[index]);

	glm::mat4 baseModel = glm::mat4(1.f);
	// rotates the tank in a circle around the scene
	baseModel = glm::rotate(baseModel, (float)glfwGetTime(), glm::vec3(.0f, -1.f, .0f));
	baseModel = glm::translate(baseModel, glm::vec3(0.f, 0.f, 10.f));
	// Rotates the top part of the tank
	if (index == 12 || index == 13 || index == 14 || index == 15) baseModel = glm::rotate(baseModel, (float)glfwGetTime() * 1.3f, glm::vec3(0.f, 1.f, 0.f));
	baseModel = glm::translate(baseModel, glm::vec3(1.f, 0.5625f, -5.7f));
	baseModel = glm::scale(baseModel, glm::vec3(5.f));

	// Removes the gap between top objects and bottom objects :|
	if (index == 12 || index == 13 || index == 14 || index == 15) {
		baseModel = glm::translate(baseModel, glm::vec3(0.f, -.005f, .0f));
	}

	glm::mat4 model = baseModel;

	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	// colours different parts differently
	int modelType = (index == 10 || index == 14) ? 6 : 5;

	glUniform1i(glGetUniformLocation(shaderProgram, "modelType"), modelType);

	glDrawArrays(GL_TRIANGLES, 0, verts);
	
	// Draws the other wheel
	if (index == 10) {
		model = glm::translate(model,  glm::vec3(0.f, 0.f, .725f));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

		glDrawArrays(GL_TRIANGLES, 0, verts);
	}
}

void drawLamp(unsigned int shaderProgram, int index, int verts)
{
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[index]);
	glBindVertexArray(VAOs[index]);

	// Lamp on the grass
	glm::mat4 model = glm::mat4(1.f);
	model = glm::rotate(model, (float)M_PI / 2, glm::vec3(0.f, 1.f, 0.f));
	model = glm::translate(model, glm::vec3(0.f, 2.65f, -6.f));
	model = glm::rotate(model, (float) M_PI/2, glm::vec3(0.f, 1.f, 0.f));
	model = glm::scale(model, glm::vec3(.006f));

	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	int modelType = 7;
	glUniform1i(glGetUniformLocation(shaderProgram, "modelType"), modelType);

	glDrawArrays(GL_TRIANGLES, 0, verts);
	
	// Lamp on the curb
	model = glm::mat4(1.f);
	model = glm::rotate(model, (float)M_PI / 2, glm::vec3(0.f, 1.f, 0.f));
	model = glm::translate(model, glm::vec3(-10.f, 3.1f, -15.1f));
	model = glm::rotate(model, (float) M_PI /2, glm::vec3(0.f, -1.f, 0.f));
	model = glm::scale(model, glm::vec3(.006f, .006f, .006f));

	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	glDrawArrays(GL_TRIANGLES, 0, verts);
}

void drawStreet(unsigned int shaderProgram, GLuint streetTex, int index, int verts)
{
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[index]);
	glBindVertexArray(VAOs[index]);
	glBindTexture(GL_TEXTURE_2D, streetTex);

	glm::mat4 model = glm::mat4(1.f);
	model = glm::rotate(model, (float)M_PI / 2, glm::vec3(0.f, 1.f, 0.f));
	model = glm::translate(model, glm::vec3(-25.f, 0.05f, -15.5f));

	model = glm::scale(model, glm::vec3(50, 1, 10));
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	int modelType = 9;
	glUniform1i(glGetUniformLocation(shaderProgram, "modelType"), modelType);

	glDrawArrays(GL_TRIANGLES, 0, verts);
}

void drawChest(unsigned int shaderProgram, GLuint chestTex, int index, int verts) {
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[index]);
	glBindVertexArray(VAOs[index]);
	glBindTexture(GL_TEXTURE_2D, chestTex);

	glm::mat4 model = glm::mat4(1.f);
	model = glm::rotate(model, (float) M_PI/2, glm::vec3(0.f, -1.f, 0.f));
	model = glm::translate(model, glm::vec3(3.f, .665f, 0.f));
	// Chest body
	if (index == 24) {
		model = glm::scale(model, glm::vec3(2.f, 1.f, 2.5f));
		model = glm::translate(model, glm::vec3(0.f, 0.f, .33f));
	}
	// Chest Lid
	else if (index == 23) {
		model = glm::translate(model, glm::vec3(0.f, -.1f, .08f));
		model = glm::scale(model, glm::vec3(2.f, 1.f, 1.f));
	}

	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	int modelType = 8;
	glUniform1i(glGetUniformLocation(shaderProgram, "modelType"), modelType);

	glDrawArrays(GL_TRIANGLES, 0, verts);
}

void drawFillChest(unsigned int shaderProgram, GLuint cannonballTex, int index) {
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[index]);
	glBindVertexArray(VAOs[index]);
	glBindTexture(GL_TEXTURE_2D, cannonballTex);
	
	// draws multiple cannonballs with offset from each other
	glm::mat4 model = glm::mat4(1.f);
	model = glm::translate(model, glm::vec3(.67f, .8f, 2.3f));
	model = glm::scale(model, glm::vec3(0.3f));

	// Bottom Layer
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 4; j++) {
			model = glm::translate(model, glm::vec3(-2.f, 0.f, 0.f));
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

			int modelType = 0;
			glUniform1i(glGetUniformLocation(shaderProgram, "modelType"), modelType);

			glDrawArrays(GL_TRIANGLES, 0, SPHERE_VERTS);
		}
		model = glm::translate(model, glm::vec3(8.f, 0.f, 0.f));
		model = glm::translate(model, glm::vec3(0.f, 0.f, 2.2f));
	}

	// Top Layer
	model = glm::mat4(1.f);
	model = glm::translate(model, glm::vec3(0.4f, 1.25f, 2.66f));
	model = glm::scale(model, glm::vec3(0.3f));
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 3; j++) {
			model = glm::translate(model, glm::vec3(-2.15f, 0.f, 0.f));
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

			int modelType = 0;
			glUniform1i(glGetUniformLocation(shaderProgram, "modelType"), modelType);

			glDrawArrays(GL_TRIANGLES, 0, SPHERE_VERTS);
		}
		model = glm::translate(model, glm::vec3(6.45f, 0.f, 0.f));
		model = glm::translate(model, glm::vec3(0.f, 0.f, 2.3f));
	}
}

void drawCurb(unsigned int shaderProgram, int index, int verts) {
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[index]);
	glBindVertexArray(VAOs[index]);

	glm::mat4 model = glm::mat4(1.f);
	model = glm::translate(model, glm::vec3(-20.5f, 0.26f, 0.f));
	model = glm::rotate(model, (float)M_PI, glm::vec3(1.f, 0.f, 0.f));
	model = glm::scale(model, glm::vec3(10.f, 0.4f, 50.f));

	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	int modelType = 7;
	glUniform1i(glGetUniformLocation(shaderProgram, "modelType"), modelType);

	glDrawArrays(GL_TRIANGLES, 0, verts);

}

void drawObjects(unsigned int shaderProgram, GLuint* textures, std::vector<int> verts) {
	drawCannonball(shaderProgram, textures[0], 0); // Draws fired cannonball
	drawSky(shaderProgram, fmod(((float)glfwGetTime() / 20), M_PI * 2) < M_PI ? textures[1] : textures[7], 1); // Draws the sky invSphere
	drawFloor(shaderProgram, textures[2], 2, verts[2]); // Draws the grass floor cyl
	drawSun(shaderProgram, textures[3], 0); // Draws the sun
	drawCannon(shaderProgram, textures[4], 3, verts[3]); // Draws The cannon
	drawCannon(shaderProgram, textures[5], 4, verts[4]); //
	drawCannon(shaderProgram, textures[6], 5, verts[5]); //
	drawCannon(shaderProgram, textures[4], 6, verts[6]); //
	drawCannon(shaderProgram, textures[4], 7, verts[7]); //
	drawCannon(shaderProgram, textures[4], 8, verts[8]); // Would loop if it wasn't for the shared textures
	for (int i = 9; i < 16; i++) {
		drawTank(shaderProgram, i, verts[i]); // Draws all the tank objects
	}
	for (int i = 16; i < 23; i++) {
		drawLamp(shaderProgram, i, verts[i]); // Draws all the lamp objects
	}
	drawChest(shaderProgram, textures[9], 23, verts[23]); // Draws the chest lid
	drawChest(shaderProgram, textures[9],  24, verts[24]); // Draws the chest body
	drawFillChest(shaderProgram, textures[0], 0); // Fills the chest body with cannonballs
	drawStreet(shaderProgram, textures[8], 25, verts[25]); // Draws the road quad
	drawCurb(shaderProgram, 24, verts[24]); // Draws the curb (it's actually an upsidedown chest body coloured differently)
}

void renderWithShadow(unsigned int renderShaderProgram, ShadowStruct shadow[NUM_LIGHTS], glm::mat4 projectedLightSpaceMatrix[], GLuint* textures, std::vector<int> verts)
{
	glViewport(0, 0, WIDTH, HEIGHT);

	static const GLfloat bgd[] = { .8f, .8f, .8f, 1.f };
	glClearBufferfv(GL_COLOR, 0, bgd);
	glClear(GL_DEPTH_BUFFER_BIT);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glUseProgram(renderShaderProgram);

	GLenum textureIDs[] = {
		GL_TEXTURE0,
		GL_TEXTURE1,
		GL_TEXTURE2,
		GL_TEXTURE3,
		GL_TEXTURE4,
		GL_TEXTURE5,
	};
	// Defines the texture channel used by each shadow map
	for (int i = 0; i < NUM_LIGHTS; i++) {
		glActiveTexture(textureIDs[i]);
		glBindTexture(GL_TEXTURE_2D, shadow[i].Texture);
		int maxLength = snprintf(nullptr, 0, "shadowMap[%d]", i);

		char* uniformName = new char[maxLength + 1];

		snprintf(uniformName, maxLength + 1 * sizeof(char), "shadowMap[%d]", i);

		glUniform1i(glGetUniformLocation(renderShaderProgram, uniformName), i);
		delete[] uniformName;
	}
	// Defines the texture channel used by the object texture
	glActiveTexture(textureIDs[NUM_LIGHTS]);
	glUniform1i(glGetUniformLocation(renderShaderProgram, "Texture"), NUM_LIGHTS);

	glUniform1i(glGetUniformLocation(renderShaderProgram, "numLights"), NUM_LIGHTS);

	glUniformMatrix4fv(glGetUniformLocation(renderShaderProgram, "projectedLightSpaceMatrix"), NUM_LIGHTS, GL_FALSE, glm::value_ptr(projectedLightSpaceMatrix[0]));
	glUniform3fv(glGetUniformLocation(renderShaderProgram, "lightDirection"), NUM_LIGHTS, glm::value_ptr(lightDirection[0]));
	glUniform3fv(glGetUniformLocation(renderShaderProgram, "lightPos"), NUM_LIGHTS, glm::value_ptr(lightPos[0]));
	glUniform3fv(glGetUniformLocation(renderShaderProgram, "lightColour"), 1, glm::value_ptr(glm::vec3(1.f, 1.f, 1.f)));
	glUniform3f(glGetUniformLocation(renderShaderProgram, "camPos"), camera.Position.x, camera.Position.y, camera.Position.z);
	glUniform1iv(glGetUniformLocation(renderShaderProgram, "lightType"), NUM_LIGHTS, lightType);

	glUniform1fv(glGetUniformLocation(renderShaderProgram, "attConst"), NUM_LIGHTS, attConst);
	glUniform1fv(glGetUniformLocation(renderShaderProgram, "attLin"), NUM_LIGHTS, attLin);
	glUniform1fv(glGetUniformLocation(renderShaderProgram, "attQuad"), NUM_LIGHTS, attQuad);

	glm::mat4 model = glm::mat4(1.f);
	glUniformMatrix4fv(glGetUniformLocation(renderShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	glm::mat4 view = glm::mat4(1.f);
	view = glm::lookAt(camera.Position, camera.Position + camera.Front, camera.Up);
	glUniformMatrix4fv(glGetUniformLocation(renderShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

	glm::mat4 projection = glm::mat4(1.f);
	projection = glm::perspective(glm::radians(45.f), (float)WIDTH / (float)HEIGHT, .01f, 100.f);
	glUniformMatrix4fv(glGetUniformLocation(renderShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	drawObjects(renderShaderProgram, textures, verts);
}

void generateDepthMap(unsigned int shadowShaderProgram, ShadowStruct shadow[NUM_LIGHTS], glm::mat4 projectedLightSpaceMatrix[], GLuint* textures, std::vector<int> verts)
{
	glViewport(0, 0, SH_MAP_WIDTH, SH_MAP_HEIGHT);

	// Creates a shadow map for each light source
	for (int i = 0; i < NUM_LIGHTS; i++) {
		glBindFramebuffer(GL_FRAMEBUFFER, shadow[i].FBO);

		GLenum fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (fbStatus != GL_FRAMEBUFFER_COMPLETE)
		{
			printf("Framebuffer is incomplete: %x\n", fbStatus);
			return;
		}

		glClear(GL_DEPTH_BUFFER_BIT);

		glUseProgram(shadowShaderProgram);

		glUniformMatrix4fv(glGetUniformLocation(shadowShaderProgram, "projectedLightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(projectedLightSpaceMatrix[i]));

		drawObjects(shadowShaderProgram, textures, verts);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int main(int argc, char** argv)
{
	glfwInit();

	// Enables antiAliasing
	glfwWindowHint(GLFW_SAMPLES, 32);

	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Assignment 3", NULL, NULL);
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetWindowSizeCallback(window, SizeCallback);
	glfwSetCursorPosCallback(window, MouseMoveCallback);

	gl3wInit();

	int max_samples;
	glGetIntegerv(GL_MAX_SAMPLES, &max_samples);
	printf("Max Samples Supported: %d\n", max_samples);

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(DebguMessageCallback, 0);

	GLuint program = CompileShader("phong.vert", "phong.frag");
	GLuint shadowProgram = CompileShader("shadow.vert", "shadow.frag");

	ShadowStruct shadow[NUM_LIGHTS];
	for (int i = 0; i < NUM_LIGHTS; i++) {
		shadow[i] = setup_shadowmap(SH_MAP_WIDTH, SH_MAP_HEIGHT);
	}

	float* sphere = createSphere(false);
	float* invSphere = createSphere(true);

	std::vector<float> floor = createCylinder();

	const char* grassMipmapFiles[6] = {
	"Assets/grass/grass2_1024.bmp",
	"Assets/grass/grass2_512.bmp",
	"Assets/grass/grass2_256.bmp",
	"Assets/grass/grass2_128.bmp",
	"Assets/grass/grass2_64.bmp",
	"Assets/grass/grass2_32.bmp"
	};

	GLuint* textures = (GLuint*)calloc(NUMTEXTURES, sizeof(GLuint));
	textures[0] = setup_texture("Assets/cannonball.bmp", false);
	textures[1] = setup_texture("Assets/sky_texture2.bmp", false);
	textures[2] = setup_mipmaps(grassMipmapFiles, 6, true);
	textures[3] = setup_texture("Assets/sun.bmp", false);
	textures[4] = setup_texture("Assets/Cannon/14054_Pirate_Ship_Cannon_on_Cart_wheel_diff.bmp", false);
	textures[5] = setup_texture("Assets/Cannon/14054_Pirate_Ship_Cannon_on_Cart_cart_diff.bmp", false);
	textures[6] = setup_texture("Assets/Cannon/14054_Pirate_Ship_Cannon_on_Cart_barrel_diff.bmp", false);
	textures[7] = setup_texture("Assets/sky_texture1.bmp", false);
	textures[8] = setup_texture("Assets/road.bmp", true);
	textures[9] = setup_texture("Assets/Chest.bmp", true);

	std::vector<std::vector<float>> cannonObject;
	objParser("Assets/Cannon/pirate_cannon.obj", cannonObject, true);
	
	std::vector<std::vector<float>> lampObject;
	objParser("Assets/Street_Lamp.obj", lampObject, false);

	std::vector<std::vector<float>> tank;
	loadTankVerts(tank);

	std::vector<point> ctrl_points;
	ctrl_points.push_back(point(1.5f, .5f, -0.5f, 0.f, -1.f, 0.f));
	ctrl_points.push_back(point(1.5f, 0.f, -0.5f, 0.f, -1.f, 0.f));
	ctrl_points.push_back(point(.5f, 0.f, -0.5f, 0.f, -1.f, 0.f));
	ctrl_points.push_back(point(.5f, .5f, -0.5f, 0.f, -1.f, 0.f));

	int num_evaluations = 29;
	std::vector<point> curve = EvaluateBezierCurve(ctrl_points, num_evaluations);

	int num_curve_verts = 0;
	int num_curve_floats = 0;
	float* curve_vertices = MakeFloatsFromVector(curve, num_curve_verts, num_curve_floats, 2.5f, 0.1f);

	float chest[] = {
			//back face
			-0.5f, -0.5f, -0.5f,  0.f, 0.f,	   0.f, 0.f, -1.f,
			0.5f, -0.5f, -0.5f,   1.f, 0.f,	   0.f, 0.f, -1.f,
			0.5f,  0.5f, -0.5f,   1.f, 1.f,	   0.f, 0.f, -1.f,
			0.5f,  0.5f, -0.5f,   1.f, 1.f,	   0.f, 0.f, -1.f,
			-0.5f,  0.5f, -0.5f,  0.f, 1.f,    0.f, 0.f, -1.f,
			-0.5f, -0.5f, -0.5f,  0.f, 0.f,    0.f, 0.f, -1.f,

			//front face
			-0.5f, -0.5f,  0.5f,  0.f, 0.f,	    0.f, 0.f, 1.f,
			0.5f, -0.5f,  0.5f,  1.f, 0.f,	    0.f, 0.f, 1.f,
			0.5f,  0.5f,  0.5f,  1.f, 1.f,	    0.f, 0.f, 1.f,
			0.5f,  0.5f,  0.5f,  1.f, 1.f,	    0.f, 0.f, 1.f,
			-0.5f,  0.5f,  0.5f,  0.f, 1.f,	    0.f, 0.f, 1.f,
			-0.5f, -0.5f,  0.5f,  0.f, 0.f,	    0.f, 0.f, 1.f,

			//left face
			-0.5f,  0.5f,  0.5f,  1.f, 1.f,	    -1.f, 0.f, 0.f,
			-0.5f,  0.5f, -0.5f,  1.f, 0.f,	    -1.f, 0.f, 0.f,
			-0.5f, -0.5f, -0.5f,  0.f, 0.f,	    -1.f, 0.f, 0.f,
			-0.5f, -0.5f, -0.5f,  0.f, 0.f,	    -1.f, 0.f, 0.f,
			-0.5f, -0.5f,  0.5f,  0.f, 1.f,	    -1.f, 0.f, 0.f,
			-0.5f,  0.5f,  0.5f,  1.f, 1.f,	    -1.f, 0.f, 0.f,

			//right face
			0.5f,  0.5f,  0.5f,  1.f, 1.f,	   1.f, 0.f, 0.f,
			0.5f,  0.5f, -0.5f,  1.f, 0.f,	   1.f, 0.f, 0.f,
			0.5f, -0.5f, -0.5f,  0.f, 0.f,	   1.f, 0.f, 0.f,
			0.5f, -0.5f, -0.5f,  0.f, 0.f,	   1.f, 0.f, 0.f,
			0.5f, -0.5f,  0.5f,  0.f, 1.f,	   1.f, 0.f, 0.f,
			0.5f,  0.5f,  0.5f,  1.f, 1.f,	   1.f, 0.f, 0.f,

			//bottom face
			-0.5f, -0.5f, -0.5f,  0.f, 0.f,	   0.f, -1.f, 0.f,
			0.5f, -0.5f, -0.5f,   1.f, 0.f,	   0.f, -1.f, 0.f,
			0.5f, -0.5f,  0.5f,   1.f, 1.f,	   0.f, -1.f, 0.f,
			0.5f, -0.5f,  0.5f,   1.f, 1.f,	   0.f, -1.f, 0.f,
			-0.5f, -0.5f,  0.5f,  0.f, 1.f,	   0.f, -1.f, 0.f,
			-0.5f, -0.5f, -0.5f,  0.f, 0.f,	   0.f, -1.f, 0.f,

			// INSIDE FACES
			-.49f, -.49f, -.49f,   0.f, 0.f,   0.f, 0.f, 1.f,
			.49f, -.49f, -.49f,    1.f, 0.f,   0.f, 0.f, 1.f,
			.49f,  .5f, -.49f,     1.f, 1.f,  0.f, 0.f, 1.f,
			.49f,  .5f, -.49f,     1.f, 1.f,  0.f, 0.f, 1.f,
			-.49f,  .5f, -.49f,    0.f, 1.f,   0.f, 0.f, 1.f,
			-.49f, -.49f, -.49f,   0.f, 0.f,   0.f, 0.f, 1.f,

			//front face		  
			-.49f, -.49f,  .49f,   0.f, 0.f,    0.f, 0.f, -1.f,
			.49f, -.49f,  .49f,   1.f, 0.f,	   0.f, 0.f, -1.f,
			.49f,  .5f,  .49f,    1.f, 1.f,	    0.f, 0.f, -1.f,
			.49f,  .5f,  .49f,    1.f, 1.f,	    0.f, 0.f, -1.f,
			-.49f,  .5f,  .49f,    0.f, 1.f,   0.f, 0.f, -1.f,
			-.49f, -.49f,  .49f,   0.f, 0.f,    0.f, 0.f, -1.f,

			//left face			  
			-.49f,  .5f,  .49f,    1.f, 1.f,   1.f, 0.f, 0.f,
			-.49f,  .5f, -.49f,    1.f, 0.f,   1.f, 0.f, 0.f,
			-.49f, -.49f, -.49f,   0.f, 0.f,    1.f, 0.f, 0.f,
			-.49f, -.49f, -.49f,   0.f, 0.f,    1.f, 0.f, 0.f,
			-.49f, -.49f,  .49f,   0.f, 1.f,    1.f, 0.f, 0.f,
			-.49f,  .5f,  .49f,    1.f, 1.f,   1.f, 0.f, 0.f,

			//right face		  
			.49f,  .5f,  .49f,    1.f, 1.f,	  -1.f, 0.f, 0.f,
			.49f,  .5f, -.49f,    1.f, 0.f,	  -1.f, 0.f, 0.f,
			.49f, -.49f, -.49f,   0.f, 0.f,	  -1.f, 0.f, 0.f,
			.49f, -.49f, -.49f,   0.f, 0.f,	  -1.f, 0.f, 0.f,
			.49f, -.49f,  .49f,   0.f, 1.f,	  -1.f, 0.f, 0.f,
			.49f,  .5f,  .49f,    1.f, 1.f,	  -1.f, 0.f, 0.f,

			//bottom face		  
			-.49f, -.49f, -.49f,   0.f, 0.f,   0.f, 1.f, 0.f,
			.49f, -.49f, -.49f,    1.f, 0.f,  0.f, 1.f, 0.f,
			.49f, -.49f,  .49f,    1.f, 1.f,  0.f, 1.f, 0.f,
			.49f, -.49f,  .49f,    1.f, 1.f,  0.f, 1.f, 0.f,
			-.49f, -.49f,  .49f,   0.f, 1.f,   0.f, 1.f, 0.f,
			-.49f, -.49f, -.49f,   0.f, 0.f,   0.f, 1.f, 0.f,


			// Connections
			.49f, .5f , .5f,	0.f, 0.f,   0.f, 1.f, 0.f,
			.5f, .5f, .5f,		0.f, 0.f,   0.f, 1.f, 0.f,
			.5f, .5f, -.5f,		0.f, 0.f,   0.f, 1.f, 0.f,
			.49f, .5f , .5f,	0.f, 0.f,   0.f, 1.f, 0.f,
			.49f, .5f, -.5f,	0.f, 0.f,   0.f, 1.f, 0.f,
			.5f, .5f, -.5f,		0.f, 0.f,   0.f, 1.f, 0.f,

			-.49f, .5f , .5f,	0.f, 0.f,   0.f, 1.f, 0.f,
			-.5f, .5f, .5f,	 	0.f, 0.f,   0.f, 1.f, 0.f,
			-.5f, .5f, -.5f, 	0.f, 0.f,   0.f, 1.f, 0.f,
			-.49f, .5f , .5f,	0.f, 0.f,   0.f, 1.f, 0.f,
			-.49f, .5f, -.5f,	0.f, 0.f,   0.f, 1.f, 0.f,
			-.5f, .5f, -.5f, 	0.f, 0.f,   0.f, 1.f, 0.f,

			.49f, .5f, .49f,	0.f, 0.f,   0.f, 1.f, 0.f,
			.49f, .5f, .5f,		0.f, 0.f,   0.f, 1.f, 0.f,
			-.49f, .5f, .49f,	0.f, 0.f,   0.f, 1.f, 0.f,

			.49f, .5f, .5f,	   0.f, 0.f,    0.f, 1.f, 0.f,
			-.49f, .5f, .5f,	0.f, 0.f,   0.f, 1.f, 0.f,
			-.49f, .5f, .49f,	0.f, 0.f,   0.f, 1.f, 0.f,

			.49f, .5f, -.49f,	0.f, 0.f,   0.f, 1.f, 0.f,
			.49f, .5f, -.5f, 	0.f, 0.f,   0.f, 1.f, 0.f,
			-.49f, .5f, -.49f,	0.f, 0.f,   0.f, 1.f, 0.f,

			.49f, .5f, -.5f,	0.f, 0.f,   0.f, 1.f, 0.f,
			-.49f, .5f, -.5f,	0.f, 0.f,   0.f, 1.f, 0.f,
			-.49f, .5f, -.49f,	0.f, 0.f,   0.f, 1.f, 0.f,
		};

	float road[] = {
		1.f, 0.f, 1.f,	5.f , 1.f,	0.f, 1.f, 0.f,
		1.f, 0.f, 0.f,	5.f, 0.f,	0.f, 1.f, 0.f,
		0.f, 0.f, 0.f,	0.f, 0.f,	0.f, 1.f, 0.f,

		1.f, 0.f, 1.f,	5.f , 1.f,	0.f, 1.f, 0.f,
		0.f, 0.f, 0.f,	0.f, 0.f,	0.f, 1.f, 0.f,
		0.f, 0.f, 1.f,	0.f, 1.f,	0.f, 1.f, 0.f,

	};
	
	InitCamera(camera);
	cam_dist = 20.f;
	MoveAndOrientCamera(camera, glm::vec3(0, 0, 0), cam_dist, 0.f, 0.f);

	// Create Buffers and VAOs
	glCreateBuffers(NUM_BUFFERS, Buffers);
	glGenVertexArrays(NUM_VAOS, VAOs);

	int index = -1;
	std::vector<int> verts;

	// Cannonball = 0
	// Sun = 0
	index++;
	glNamedBufferStorage(Buffers[index], SPHERE_VERTS * sizeof(float), sphere, 0);
	glBindVertexArray(VAOs[index]);
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[index]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(2);
	verts.push_back(SPHERE_VERTS);

	// Sky = 1
	index++;
	glNamedBufferStorage(Buffers[index], SPHERE_VERTS * sizeof(float), invSphere, 0);
	glBindVertexArray(VAOs[index]);
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[index]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(2);
	verts.push_back(SPHERE_VERTS);

	// Floor = 2
	index++;
	glNamedBufferStorage(Buffers[index], floor.size() * sizeof(float), floor.data(), 0);
	glBindVertexArray(VAOs[index]);
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[index]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(2);
	verts.push_back(floor.size());

	for (int i = 0; i < cannonObject.size(); i++) {
		// Cannon = 3 - 8
		index++;
		glNamedBufferStorage(Buffers[index], cannonObject.at(i).size() * sizeof(float), cannonObject[i].data(), 0);
		glBindVertexArray(VAOs[index]);
		glBindBuffer(GL_ARRAY_BUFFER, Buffers[index]);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)(5 * sizeof(float)));
		glEnableVertexAttribArray(2);
		verts.push_back(cannonObject.at(i).size());
	}
	for (int i = 0; i < 7; i++) {
		// Tank = 9 - 15
		index++;
		glNamedBufferStorage(Buffers[index], tank.at(i).size() * sizeof(float), tank.at(i).data(), 0);
		glBindVertexArray(VAOs[index]);
		glBindBuffer(GL_ARRAY_BUFFER, Buffers[index]);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (6 * sizeof(float)), (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, (6 * sizeof(float)), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		verts.push_back(tank.at(i).size());
	}
	for (int i = 0; i < 7; i++) {
		// Lamp = 16 - 22
		index++;
		glNamedBufferStorage(Buffers[index], lampObject.at(i).size() * sizeof(float), lampObject.at(i).data(), 0);
		glBindVertexArray(VAOs[index]);
		glBindBuffer(GL_ARRAY_BUFFER, Buffers[index]);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (6 * sizeof(float)), (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, (6 * sizeof(float)), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		verts.push_back(lampObject.at(i).size());
	}
	// Chest Lid
	index++;
	glNamedBufferStorage(Buffers[index], num_curve_floats * sizeof(float), curve_vertices, 0);
	glBindVertexArray(VAOs[index]);
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[index]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(2);
	verts.push_back(num_curve_floats);
	
	// Chest body
	index++;
	glNamedBufferStorage(Buffers[index], 84 * 8 * sizeof(float), chest, 0);
	glBindVertexArray(VAOs[index]);
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[index]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(2);
	verts.push_back(84 * 8);
	
	// Road
	index++;
	glNamedBufferStorage(Buffers[index], 6 * 8 * sizeof(float), road, 0);
	glBindVertexArray(VAOs[index]);
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[index]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(2);
	verts.push_back(6 * 8);

	glEnable(GL_DEPTH_TEST);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	
	// Defines the light source Pos and Dirctions for both lamp posts
	glm::mat4 lightModel = glm::mat4(1.f);
	lightModel = glm::rotate(lightModel, (float)M_PI / 2, glm::vec3(0.f, 1.f, 0.f));
	lightModel = glm::translate(lightModel, glm::vec3(0.f, 6.5f, -8.6f));
	lightPos[1] = glm::vec3(lightModel * glm::vec4(0.f, 0.f, 0.f, 1.f));
	lightDirection[1] = glm::vec3(-0.05f, -1.f, 0.f);
	
	lightModel = glm::mat4(1.f);
	lightModel = glm::rotate(lightModel, (float)M_PI / 2, glm::vec3(0.f, 1.f, 0.f));
	lightModel = glm::translate(lightModel, glm::vec3(-10.f, 9.75f, -12.5f));
	lightPos[2] = glm::vec3(lightModel * glm::vec4(0.f, 0.f, 0.f, 1.f));
	lightDirection[2] = glm::vec3(0.05f, -1.f, 0.f);

	while (!glfwWindowShouldClose(window))
	{
		// Sets the sun light position and direction
		glm::mat4 model = glm::mat4(1.f);
		float angle = (float)glfwGetTime() / 20;
		model = glm::rotate(model, angle, sunRotation);
		model = glm::translate(model, -initSunPos);

		lightPos[0] = glm::vec3(model * glm::vec4(0.f, 0.f, 0.f, -1.f));
		lightDirection[0] = glm::normalize(glm::vec3(0.f, 1.f, 0.f) - lightPos[0]);

		// If the cannonball is fired increase it's position
		if (cannonBallPos >= 0) {
			cannonBallPos += CANNONBALL_SPEED;
		} if (cannonBallPos >= 100) {
			cannonBallPos = -1;
		}

		// Defines the projected light space matrices for each light source
		glm::mat4 projectedLightSpaceMatrix[NUM_LIGHTS];
		for (int lightIndex = 0; lightIndex < NUM_LIGHTS; lightIndex++)
		{
			glm::mat4 lightProjection;
			glm::mat4 lightView;
			// Spotlight and positional
			if (lightType[lightIndex] == 1 || lightType[lightIndex] == 2) {
				float near_plane = 5.f; float far_plane = 75.f;
				lightProjection = glm::perspective(glm::radians(90.f), (float)WIDTH / (float)HEIGHT, near_plane, far_plane);
				lightView = glm::lookAt(lightPos[lightIndex], lightPos[lightIndex] + lightDirection[lightIndex], glm::vec3(0.0f, 1.0f, 0.0f));
			}
			// Directional
			else if (lightType[lightIndex] == 0) {
				float near_plane = 10.f, far_plane = 75.f;
				lightProjection = glm::ortho(-25.f, 25.f, -25.f, 25.f, near_plane, far_plane);
				lightView = glm::lookAt(lightPos[lightIndex], lightPos[lightIndex] + lightDirection[lightIndex], glm::vec3(0.0f, 1.0f, 0.f));
			}
			projectedLightSpaceMatrix[lightIndex] = lightProjection * lightView;
		}

		generateDepthMap(shadowProgram, shadow, projectedLightSpaceMatrix, textures, verts);

		//saveShadowMapToBitmap(shadow[0].Texture, SH_MAP_WIDTH, SH_MAP_HEIGHT);

		renderWithShadow(program, shadow, projectedLightSpaceMatrix, textures, verts);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	free(textures);
	free(sphere);
	free(invSphere);
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}