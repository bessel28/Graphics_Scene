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
#include <string>
 
#include <vector>
#define _USE_MATH_DEFINES

#include <math.h>

#include <fstream>

SCamera camera;

#define NUM_BUFFERS 23
#define NUM_VAOS 23
#define NUMTEXTURES 7

GLuint Buffers[NUM_BUFFERS];
GLuint VAOs[NUM_VAOS];

int WIDTH = 1024;
int HEIGHT = 768;

#define SH_MAP_WIDTH 2048
#define SH_MAP_HEIGHT 2048
#define SH_MAP_WIDTH 4096
#define SH_MAP_HEIGHT 4096

#define NUM_LIGHTS 2

glm::vec3 lightDirection[NUM_LIGHTS];
glm::vec3 lightPos[NUM_LIGHTS];

glm::vec3 initSunPos = glm::vec3(0.f, 0.f, 19.f);
glm::vec3 sunRotation = glm::vec3(-2.f, 0.f, 0.f);

int lightType[] = { 1 , 2 };

#define RADIUS 1
#define NUM_SECTORS 30
#define NUM_STACKS 30
#define SPHERE_VERTS (8 * 3 * 2 * NUM_SECTORS * (NUM_STACKS - 1))

int cameraType = 0;

int flashLight = 0;

#define CANNONBALL_SPEED 0.6;

float cannonBallPos = -1;


void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
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
	if (key == GLFW_KEY_LEFT_ALT && action == GLFW_PRESS) {
		lightType[0] = (lightType[0] + 1) % 3;
		printf("Light Type: %d %d\n", lightType[0], lightType[1]);
	}
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
	WIDTH = w;
	HEIGHT = h;
	glViewport(0, 0, w, h);
}

void objParser(const char* filename, std::vector<std::vector<float>>& objects, bool readTex) {
	char* obj = read_file(filename);
	const char* delim = "\n";
	char* saveptr, * saveptr2;

	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> texCoords;

	std::vector<float > object;

	int objNum = 0;

	char* line = strtok_s(obj, delim, &saveptr);
	while (line != NULL)
	{
		std::string curline(line);
		char* words = strtok_s(line, " ", &saveptr2);
		std::string word(words);
		if (word == "#") {
			words = strtok_s(NULL, " ", &saveptr2);
			std::string word(words);
			if (word == "object") {
				if (objNum != 0) {
					objects.push_back(object);
					object.clear();
				}
				objNum++;
			}
		}
		else if (word == "o") {
			if (objNum != 0) {
				objects.push_back(object);
				object.clear();
			}
			objNum++;
		}
		else if (word == "v") {
			glm::vec3 v;
			words = strtok_s(NULL, " ", &saveptr2);
			v.x = std::strtof(words, nullptr);
			words = strtok_s(NULL, " ", &saveptr2);
			v.y = std::strtof(words, nullptr);
			words = strtok_s(NULL, " ", &saveptr2);
			v.z = std::strtof(words, nullptr);
			vertices.push_back(v);
		}
		else if (word == "vn") {
			glm::vec3 v;
			words = strtok_s(NULL, " ", &saveptr2);
			v.x = std::strtof(words, nullptr);
			words = strtok_s(NULL, " ", &saveptr2);
			v.y = std::strtof(words, nullptr);
			words = strtok_s(NULL, " ", &saveptr2);
			v.z = std::strtof(words, nullptr);
			normals.push_back(v);
		}
		else if (word == "vt") {
			if (readTex) {
				glm::vec2 v;
				words = strtok_s(NULL, " ", &saveptr2);
				v.x = std::strtof(words, nullptr);
				words = strtok_s(NULL, " ", &saveptr2);
				v.y = std::strtof(words, nullptr);
				texCoords.push_back(v);
			}
		}
		else if (word == "f") {
			std::vector<std::vector<float>> face;

			for (int i = 0; i < 4; i++) {
				std::vector<float> vertex;
				char* saveptr3;
				int index;
				words = strtok_s(NULL, " ", &saveptr2);
				if (words != NULL) {
					index = std::atoi(strtok_s(words, "/", &saveptr3));
					glm::vec3 v = vertices.at(index - 1);
					vertex.push_back(v.x);
					vertex.push_back(v.y);
					vertex.push_back(v.z);

					index = std::atoi(strtok_s(NULL, "/", &saveptr3));
					if (readTex) {
						glm::vec2 t = texCoords.at(index - 1);
						vertex.push_back(t.x);
						vertex.push_back(t.y);
					}

					index = std::atoi(strtok_s(NULL, "/", &saveptr3));
					glm::vec3 n = normals.at(index - 1);
					vertex.push_back(n.x);
					vertex.push_back(n.y);
					vertex.push_back(n.z);

					face.push_back(vertex);
				}
			}
			int vertSize = readTex ? 8 : 6;

			object.insert(object.end(), face.at(0).data(), face.at(0).data() + vertSize);
			object.insert(object.end(), face.at(1).data(), face.at(1).data() + vertSize);
			object.insert(object.end(), face.at(2).data(), face.at(2).data() + vertSize);

			if (face.size() == 4) {
				object.insert(object.end(), face.at(0).data(), face.at(0).data() + vertSize);
				object.insert(object.end(), face.at(2).data(), face.at(2).data() + vertSize);
				object.insert(object.end(), face.at(3).data(), face.at(3).data() + vertSize);
			}
		}
		line = strtok_s(NULL, delim, &saveptr);
	}
	objects.push_back(object);

	printf("Loaded OBJ %s\n", filename);
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

float* createCircle(int& num_Verts, bool inverseN)
{
	int verts = 2 * 3 * (NUM_SECTORS + 1) * (NUM_STACKS + 1);
	num_Verts = verts;
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
	/*for (int i = 0; i < index; i += 8)
	{
		printf("[%d] - %f, %f, %f  %f, %f\n", i / 8, vertices[i], vertices[i + 1], vertices[i + 2], vertices[i + 3], vertices[i + 4]);
	}*/


	float* sphere = (float*)malloc(8 * verts * sizeof(float));
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

			if (i == 0)
			{
				addVertex(sphere, sphereIndex, vertices, v1);
				addVertex(sphere, sphereIndex, vertices, v2);
				addVertex(sphere, sphereIndex, vertices, v4);
			}
			else if (i == (NUM_STACKS - 1))
			{
				addVertex(sphere, sphereIndex, vertices, v1);
				addVertex(sphere, sphereIndex, vertices, v2);
				addVertex(sphere, sphereIndex, vertices, v3);
			}
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

void drawCannonball(unsigned int shaderProgram, GLuint cannonballTex, int index)
{
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[index]);
	glBindVertexArray(VAOs[index]);
	glBindTexture(GL_TEXTURE_2D, cannonballTex);

	glm::mat4 model = glm::mat4(1.f);
	model = glm::translate(model, glm::vec3(4.3f, -.5f, 0.f));
	
	model = glm::scale(model, glm::vec3(0.25f, 0.25f, 0.25f));

	if (cannonBallPos == -1) {
		return;
	}
	else if (cannonBallPos > 0) {
		model = glm::translate(model, glm::vec3((float)cannonBallPos, (float)cannonBallPos / 10, 0.f));
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
	//model = glm::rotate(model, (float)glfwGetTime() / 2, glm::vec3(0.f, 1.f, 0.f));
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	int modelType = 1;
	glUniform1i(glGetUniformLocation(shaderProgram, "modelType"), modelType);

	glDrawArrays(GL_TRIANGLES, 0, SPHERE_VERTS);

}

void drawFloor(unsigned int shaderProgram, GLuint floorTex, int index)
{
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[index]);
	glBindVertexArray(VAOs[index]);
	glBindTexture(GL_TEXTURE_2D, floorTex);

	glm::mat4 model = glm::mat4(1.f);
	model = glm::translate(model, glm::vec3(0, -3, 0));

	model = glm::scale(model, glm::vec3(25, 0.1, 25));
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	int modelType = 2;
	glUniform1i(glGetUniformLocation(shaderProgram, "modelType"), modelType);

	glDrawArrays(GL_TRIANGLES, 0, 36);

	modelType = 5;
	glUniform1i(glGetUniformLocation(shaderProgram, "modelType"), modelType);
	model = glm::translate(model, glm::vec3(0, -0.1, 0));
	model = glm::rotate(model, (float)M_PI, glm::vec3(1.0f, .0f, .0f));
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	glDrawArrays(GL_TRIANGLES, 0, 36);
}

void drawSun(unsigned int shaderProgram, GLuint sunTex, int index)
{
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[index]);
	glBindVertexArray(VAOs[index]);
	glBindTexture(GL_TEXTURE_2D, sunTex);

	glm::mat4 model = glm::mat4(1.f);
	model = glm::rotate(model, (float)glfwGetTime() / 20, sunRotation);
	model = glm::translate(model, initSunPos);
	model = glm::translate(model, glm::vec3(6.f) * glm::normalize(initSunPos));
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
	model = glm::translate(model, glm::vec3(0.f, -2.9f, 0.f));
	model = glm::rotate(model, (float) M_PI/2 , glm::vec3(-1.f, 0.f, 0.f));
	model = glm::scale(model, glm::vec3(.04f, .04f, .04f));

	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	int modelType = 4;
	glUniform1i(glGetUniformLocation(shaderProgram, "modelType"), modelType);

	glDrawArrays(GL_TRIANGLES, 0, verts * sizeof(float));
}

void drawTank(unsigned int shaderProgram, int index, int verts)
{
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[index]);
	glBindVertexArray(VAOs[index]);

	glm::mat4 baseModel = glm::mat4(1.f);
	baseModel = glm::rotate(baseModel, (float)glfwGetTime(), glm::vec3(.0f, -1.f, .0f));
	baseModel = glm::translate(baseModel, glm::vec3(0.f, 0.f, 10.f));
	if (index == 12 || index == 13 || index == 14 || index == 15) baseModel = glm::rotate(baseModel, (float)glfwGetTime() * 1.3f, glm::vec3(0.f, 1.f, 0.f));
	baseModel = glm::translate(baseModel, glm::vec3(1.f, -2.4375f, -5.7f));
	baseModel = glm::scale(baseModel, glm::vec3(5.f));

	if (index == 12 || index == 13 || index == 14 || index == 15) {
		baseModel = glm::translate(baseModel, glm::vec3(0.f, -.005f, .0f));
		
	}

	glm::mat4 model = baseModel;

	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	int modelType = 5;
	if (index == 10 || index == 14) modelType = 6;

	glUniform1i(glGetUniformLocation(shaderProgram, "modelType"), modelType);

	glDrawArrays(GL_TRIANGLES, 0, verts * sizeof(float));

	if (index == 10) {
		model = glm::translate(model,  glm::vec3(0.f, 0.f, .725f));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));



		glDrawArrays(GL_TRIANGLES, 0, verts * sizeof(float));
	}
}

void drawLamp(unsigned int shaderProgram, int index, int verts)
{
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[index]);
	glBindVertexArray(VAOs[index]);

	glm::mat4 model = glm::mat4(1.f);
	model = glm::rotate(model, (float)M_PI / 2, glm::vec3(0.f, 1.f, 0.f));
	model = glm::translate(model, glm::vec3(0.f, -.2f, -6.f));
	model = glm::rotate(model, (float) M_PI/2, glm::vec3(0.f, 1.f, -0.f));
	model = glm::scale(model, glm::vec3(.006f));

	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	int modelType = 7;
	glUniform1i(glGetUniformLocation(shaderProgram, "modelType"), modelType);

	glDrawArrays(GL_TRIANGLES, 0, verts * sizeof(float));
}

void drawObjects(unsigned int shaderProgram, GLuint* textures, std::vector<int> verts) {
	drawCannonball(shaderProgram, textures[0], 0);
	drawSky(shaderProgram, textures[1], 1);
	drawFloor(shaderProgram, textures[2], 2);
	drawSun(shaderProgram, textures[3], 0);
	drawCannon(shaderProgram, textures[4], 3, verts[3]);
	drawCannon(shaderProgram, textures[5], 4, verts[4]);
	drawCannon(shaderProgram, textures[6], 5, verts[5]);
	drawCannon(shaderProgram, textures[4], 6, verts[6]);
	drawCannon(shaderProgram, textures[4], 7, verts[7]);
	drawCannon(shaderProgram, textures[4], 8, verts[8]);
	for (int i = 9; i < 16; i++) {
		drawTank(shaderProgram, i, verts[i]);
	}
	for (int i = 16; i < 23; i++) {
		drawLamp(shaderProgram, i, verts[i]);
	}

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

	for (int i = 0; i < NUM_LIGHTS; i++) {
		glActiveTexture(textureIDs[i]);
		glBindTexture(GL_TEXTURE_2D, shadow[i].Texture);
		int maxLength = snprintf(nullptr, 0, "shadowMap[%d]", i);

		char* uniformName = new char[maxLength + 1];

		snprintf(uniformName, maxLength + 1 * sizeof(char), "shadowMap[%d]", i);

		glUniform1i(glGetUniformLocation(renderShaderProgram, uniformName), i);
		delete[] uniformName;
	}

	glActiveTexture(textureIDs[NUM_LIGHTS]);
	glUniform1i(glGetUniformLocation(renderShaderProgram, "Texture"), NUM_LIGHTS);

	glUniform1i(glGetUniformLocation(renderShaderProgram, "numLights"), NUM_LIGHTS);
	glUniform1i(glGetUniformLocation(renderShaderProgram, "numLights2"), NUM_LIGHTS);

	glUniformMatrix4fv(glGetUniformLocation(renderShaderProgram, "projectedLightSpaceMatrix"), NUM_LIGHTS, GL_FALSE, glm::value_ptr(projectedLightSpaceMatrix[0]));
	glUniform3fv(glGetUniformLocation(renderShaderProgram, "lightDirection"), NUM_LIGHTS, glm::value_ptr(lightDirection[0]));
	glUniform3fv(glGetUniformLocation(renderShaderProgram, "lightPos"), NUM_LIGHTS, glm::value_ptr(lightPos[0]));
	glUniform3fv(glGetUniformLocation(renderShaderProgram, "lightColour"), 1, glm::value_ptr(glm::vec3(1.f, 1.f, 1.f)));
	glUniform3f(glGetUniformLocation(renderShaderProgram, "camPos"), camera.Position.x, camera.Position.y, camera.Position.z);
	glUniform1iv(glGetUniformLocation(renderShaderProgram, "lightType"), NUM_LIGHTS, lightType);

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

void loadTankVerts(std::vector<std::vector<float>> &tank) {
	float * tank0 = new float[36*6]
	{
		0.37626, 0.386811, 0.805815, -0, -0, -1,
		-0.728167, 0.058654, 0.805815, -0, -0, -1,
		-0.728167, 0.386811, 0.805815, -0, -0, -1,
		0.37626, 0.386811, 0.805815, -0, -0, -1,
		0.37626, 0.058654, 0.805815, -0, -0, -1,
		-0.728167, 0.058654, 0.805815, -0, -0, -1,
		-0.728167, 0.386811, 1.51267, -0, -0, 1,
		0.37626, 0.058654, 1.51267, -0, -0, 1,
		0.37626, 0.386811, 1.51267, -0, -0, 1,
		-0.728167, 0.386811, 1.51267, -0, -0, 1,
		-0.728167, 0.058654, 1.51267, -0, -0, 1,
		0.37626, 0.058654, 1.51267, -0, -0, 1,
		0.37626, 0.386811, 1.51267, 1, -0, -0,
		0.37626, 0.058654, 0.805815, 1, -0, -0,
		0.37626, 0.386811, 0.805815, 1, -0, -0,
		0.37626, 0.386811, 1.51267, 1, -0, -0,
		0.37626, 0.058654, 1.51267, 1, -0, -0,
		0.37626, 0.058654, 0.805815, 1, -0, -0,
		-0.728167, 0.386811, 0.805815, -1, -0, -0,
		-0.728167, 0.058654, 1.51267, -1, -0, -0,
		-0.728167, 0.386811, 1.51267, -1, -0, -0,
		-0.728167, 0.386811, 0.805815, -1, -0, -0,
		-0.728167, 0.058654, 0.805815, -1, -0, -0,
		-0.728167, 0.058654, 1.51267, -1, -0, -0,
		-0.728167, 0.386811, 0.805815, -0, 1, -0,
		0.37626, 0.386811, 1.51267, -0, 1, -0,
		0.37626, 0.386811, 0.805815, -0, 1, -0,
		-0.728167, 0.386811, 0.805815, -0, 1, -0,
		-0.728167, 0.386811, 1.51267, -0, 1, -0,
		0.37626, 0.386811, 1.51267, -0, 1, -0,
		-0.728167, 0.058654, 1.51267, -0, -1, -0,
		0.37626, 0.058654, 0.805815, -0, -1, -0,
		0.37626, 0.058654, 1.51267, -0, -1, -0,
		-0.728167, 0.058654, 1.51267, -0, -1, -0,
		-0.728167, 0.058654, 0.805815, -0, -1, -0,
		0.37626, 0.058654, 0.805815, -0, -1, -0,
	};
	float * tank1 = new float[960 * 6]
	{
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		0.350997, 0.076314, 0.709347, -0, -0, -1,
		0.349276, 0.063361, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		0.349276, 0.063361, 0.88203, -0, -0, 1,
		0.350997, 0.076314, 0.88203, -0, -0, 1,
		0.350997, 0.076314, 0.709347, 1, -0, -0,
		0.349276, 0.063361, 0.88203, 0.9664, -0.2572, -0,
		0.349276, 0.063361, 0.709347, 0.9664, -0.2572, -0,
		0.349276, 0.063361, 0.88203, 0.9664, -0.2572, -0,
		0.350997, 0.076314, 0.709347, 1, -0, -0,
		0.350997, 0.076314, 0.88203, 1, -0, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		0.349276, 0.063361, 0.709347, -0, -0, -1,
		0.344124, 0.050487, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		0.344124, 0.050487, 0.88203, -0, -0, 1,
		0.349276, 0.063361, 0.88203, -0, -0, 1,
		0.349276, 0.063361, 0.709347, 0.9664, -0.2572, -0,
		0.344124, 0.050487, 0.88203, 0.8815, -0.4721, -0,
		0.344124, 0.050487, 0.709347, 0.8815, -0.4721, -0,
		0.344124, 0.050487, 0.88203, 0.8815, -0.4721, -0,
		0.349276, 0.063361, 0.709347, 0.9664, -0.2572, -0,
		0.349276, 0.063361, 0.88203, 0.9664, -0.2572, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		0.344124, 0.050487, 0.709347, -0, -0, -1,
		0.335572, 0.037773, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		0.335572, 0.037773, 0.88203, -0, -0, 1,
		0.344124, 0.050487, 0.88203, -0, -0, 1,
		0.344124, 0.050487, 0.709347, 0.8815, -0.4721, -0,
		0.335572, 0.037773, 0.88203, 0.7764, -0.6303, -0,
		0.335572, 0.037773, 0.709347, 0.7764, -0.6303, -0,
		0.335572, 0.037773, 0.88203, 0.7764, -0.6303, -0,
		0.344124, 0.050487, 0.709347, 0.8815, -0.4721, -0,
		0.344124, 0.050487, 0.88203, 0.8815, -0.4721, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		0.335572, 0.037773, 0.709347, -0, -0, -1,
		0.323674, 0.025296, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		0.323674, 0.025296, 0.88203, -0, -0, 1,
		0.335572, 0.037773, 0.88203, -0, -0, 1,
		0.335572, 0.037773, 0.709347, 0.7764, -0.6303, -0,
		0.323674, 0.025296, 0.88203, 0.6731, -0.7395, -0,
		0.323674, 0.025296, 0.709347, 0.6731, -0.7395, -0,
		0.323674, 0.025296, 0.88203, 0.6731, -0.7395, -0,
		0.335572, 0.037773, 0.709347, 0.7764, -0.6303, -0,
		0.335572, 0.037773, 0.88203, 0.7764, -0.6303, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		0.323674, 0.025296, 0.709347, -0, -0, -1,
		0.308503, 0.013134, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		0.308503, 0.013134, 0.88203, -0, -0, 1,
		0.323674, 0.025296, 0.88203, -0, -0, 1,
		0.323674, 0.025296, 0.709347, 0.6731, -0.7395, -0,
		0.308503, 0.013134, 0.88203, 0.5811, -0.8138, -0,
		0.308503, 0.013134, 0.709347, 0.5811, -0.8138, -0,
		0.308503, 0.013134, 0.88203, 0.5811, -0.8138, -0,
		0.323674, 0.025296, 0.709347, 0.6731, -0.7395, -0,
		0.323674, 0.025296, 0.88203, 0.6731, -0.7395, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		0.308503, 0.013134, 0.709347, -0, -0, -1,
		0.290152, 0.001361, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		0.290152, 0.001361, 0.88203, -0, -0, 1,
		0.308503, 0.013134, 0.88203, -0, -0, 1,
		0.308503, 0.013134, 0.709347, 0.5811, -0.8138, -0,
		0.290152, 0.001361, 0.88203, 0.502, -0.8649, -0,
		0.290152, 0.001361, 0.709347, 0.502, -0.8649, -0,
		0.290152, 0.001361, 0.88203, 0.502, -0.8649, -0,
		0.308503, 0.013134, 0.709347, 0.5811, -0.8138, -0,
		0.308503, 0.013134, 0.88203, 0.5811, -0.8138, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		0.290152, 0.001361, 0.709347, -0, -0, -1,
		0.268734, -0.00995, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		0.268734, -0.00995, 0.88203, -0, -0, 1,
		0.290152, 0.001361, 0.88203, -0, -0, 1,
		0.290152, 0.001361, 0.709347, 0.502, -0.8649, -0,
		0.268734, -0.00995, 0.88203, 0.4346, -0.9006, -0,
		0.268734, -0.00995, 0.709347, 0.4346, -0.9006, -0,
		0.268734, -0.00995, 0.88203, 0.4346, -0.9006, -0,
		0.290152, 0.001361, 0.709347, 0.502, -0.8649, -0,
		0.290152, 0.001361, 0.88203, 0.502, -0.8649, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		0.268734, -0.00995, 0.709347, -0, -0, -1,
		0.244382, -0.020729, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		0.244382, -0.020729, 0.88203, -0, -0, 1,
		0.268734, -0.00995, 0.88203, -0, -0, 1,
		0.268734, -0.00995, 0.709347, 0.4346, -0.9006, -0,
		0.244382, -0.020729, 0.88203, 0.377, -0.9262, -0,
		0.244382, -0.020729, 0.709347, 0.377, -0.9262, -0,
		0.244382, -0.020729, 0.88203, 0.377, -0.9262, -0,
		0.268734, -0.00995, 0.709347, 0.4346, -0.9006, -0,
		0.268734, -0.00995, 0.88203, 0.4346, -0.9006, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		0.244382, -0.020729, 0.709347, -0, -0, -1,
		0.217245, -0.030909, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		0.217245, -0.030909, 0.88203, -0, -0, 1,
		0.244382, -0.020729, 0.88203, -0, -0, 1,
		0.244382, -0.020729, 0.709347, 0.377, -0.9262, -0,
		0.217245, -0.030909, 0.88203, 0.3272, -0.945, -0,
		0.217245, -0.030909, 0.709347, 0.3272, -0.945, -0,
		0.217245, -0.030909, 0.88203, 0.3272, -0.945, -0,
		0.244382, -0.020729, 0.709347, 0.377, -0.9262, -0,
		0.244382, -0.020729, 0.88203, 0.377, -0.9262, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		0.217245, -0.030909, 0.709347, -0, -0, -1,
		0.187491, -0.040428, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		0.187491, -0.040428, 0.88203, -0, -0, 1,
		0.217245, -0.030909, 0.88203, -0, -0, 1,
		0.217245, -0.030909, 0.709347, 0.3272, -0.945, -0,
		0.187491, -0.040428, 0.88203, 0.2836, -0.9589, -0,
		0.187491, -0.040428, 0.709347, 0.2836, -0.9589, -0,
		0.187491, -0.040428, 0.88203, 0.2836, -0.9589, -0,
		0.217245, -0.030909, 0.709347, 0.3272, -0.945, -0,
		0.217245, -0.030909, 0.88203, 0.3272, -0.945, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		0.187491, -0.040428, 0.709347, -0, -0, -1,
		0.155303, -0.049228, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		0.155303, -0.049228, 0.88203, -0, -0, 1,
		0.187491, -0.040428, 0.88203, -0, -0, 1,
		0.187491, -0.040428, 0.709347, 0.2836, -0.9589, -0,
		0.155303, -0.049228, 0.88203, 0.2449, -0.9695, -0,
		0.155303, -0.049228, 0.709347, 0.2449, -0.9695, -0,
		0.155303, -0.049228, 0.88203, 0.2449, -0.9695, -0,
		0.187491, -0.040428, 0.709347, 0.2836, -0.9589, -0,
		0.187491, -0.040428, 0.88203, 0.2836, -0.9589, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		0.155303, -0.049228, 0.709347, -0, -0, -1,
		0.12088, -0.057254, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		0.12088, -0.057254, 0.88203, -0, -0, 1,
		0.155303, -0.049228, 0.88203, -0, -0, 1,
		0.155303, -0.049228, 0.709347, 0.2449, -0.9695, -0,
		0.12088, -0.057254, 0.88203, 0.2101, -0.9777, -0,
		0.12088, -0.057254, 0.709347, 0.2101, -0.9777, -0,
		0.12088, -0.057254, 0.88203, 0.2101, -0.9777, -0,
		0.155303, -0.049228, 0.709347, 0.2449, -0.9695, -0,
		0.155303, -0.049228, 0.88203, 0.2449, -0.9695, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		0.12088, -0.057254, 0.709347, -0, -0, -1,
		0.084434, -0.064456, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		0.084434, -0.064456, 0.88203, -0, -0, 1,
		0.12088, -0.057254, 0.88203, -0, -0, 1,
		0.12088, -0.057254, 0.709347, 0.2101, -0.9777, -0,
		0.084434, -0.064456, 0.88203, 0.1783, -0.984, -0,
		0.084434, -0.064456, 0.709347, 0.1783, -0.984, -0,
		0.084434, -0.064456, 0.88203, 0.1783, -0.984, -0,
		0.12088, -0.057254, 0.709347, 0.2101, -0.9777, -0,
		0.12088, -0.057254, 0.88203, 0.2101, -0.9777, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		0.084434, -0.064456, 0.709347, -0, -0, -1,
		0.04619, -0.07079, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		0.04619, -0.07079, 0.88203, -0, -0, 1,
		0.084434, -0.064456, 0.88203, -0, -0, 1,
		0.084434, -0.064456, 0.709347, 0.1783, -0.984, -0,
		0.04619, -0.07079, 0.88203, 0.149, -0.9888, -0,
		0.04619, -0.07079, 0.709347, 0.149, -0.9888, -0,
		0.04619, -0.07079, 0.88203, 0.149, -0.9888, -0,
		0.084434, -0.064456, 0.709347, 0.1783, -0.984, -0,
		0.084434, -0.064456, 0.88203, 0.1783, -0.984, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		0.04619, -0.07079, 0.709347, -0, -0, -1,
		0.006383, -0.076218, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		0.006383, -0.076218, 0.88203, -0, -0, 1,
		0.04619, -0.07079, 0.88203, -0, -0, 1,
		0.04619, -0.07079, 0.709347, 0.149, -0.9888, -0,
		0.006383, -0.076218, 0.88203, 0.1216, -0.9926, -0,
		0.006383, -0.076218, 0.709347, 0.1216, -0.9926, -0,
		0.006383, -0.076218, 0.88203, 0.1216, -0.9926, -0,
		0.04619, -0.07079, 0.709347, 0.149, -0.9888, -0,
		0.04619, -0.07079, 0.88203, 0.149, -0.9888, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		0.006383, -0.076218, 0.709347, -0, -0, -1,
		-0.034741, -0.080704, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.034741, -0.080704, 0.88203, -0, -0, 1,
		0.006383, -0.076218, 0.88203, -0, -0, 1,
		0.006383, -0.076218, 0.709347, 0.1216, -0.9926, -0,
		-0.034741, -0.080704, 0.88203, 0.0957, -0.9954, -0,
		-0.034741, -0.080704, 0.709347, 0.0957, -0.9954, -0,
		-0.034741, -0.080704, 0.88203, 0.0957, -0.9954, -0,
		0.006383, -0.076218, 0.709347, 0.1216, -0.9926, -0,
		0.006383, -0.076218, 0.88203, 0.1216, -0.9926, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.034741, -0.080704, 0.709347, -0, -0, -1,
		-0.076928, -0.084223, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.076928, -0.084223, 0.88203, -0, -0, 1,
		-0.034741, -0.080704, 0.88203, -0, -0, 1,
		-0.034741, -0.080704, 0.709347, 0.0957, -0.9954, -0,
		-0.076928, -0.084223, 0.88203, 0.0708, -0.9975, -0,
		-0.076928, -0.084223, 0.709347, 0.0708, -0.9975, -0,
		-0.076928, -0.084223, 0.88203, 0.0708, -0.9975, -0,
		-0.034741, -0.080704, 0.709347, 0.0957, -0.9954, -0,
		-0.034741, -0.080704, 0.88203, 0.0957, -0.9954, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.076928, -0.084223, 0.709347, -0, -0, -1,
		-0.119919, -0.086752, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.119919, -0.086752, 0.88203, -0, -0, 1,
		-0.076928, -0.084223, 0.88203, -0, -0, 1,
		-0.076928, -0.084223, 0.709347, 0.0708, -0.9975, -0,
		-0.119919, -0.086752, 0.88203, 0.0468, -0.9989, -0,
		-0.119919, -0.086752, 0.709347, 0.0468, -0.9989, -0,
		-0.119919, -0.086752, 0.88203, 0.0468, -0.9989, -0,
		-0.076928, -0.084223, 0.709347, 0.0708, -0.9975, -0,
		-0.076928, -0.084223, 0.88203, 0.0708, -0.9975, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.119919, -0.086752, 0.709347, -0, -0, -1,
		-0.163448, -0.088276, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.163448, -0.088276, 0.88203, -0, -0, 1,
		-0.119919, -0.086752, 0.88203, -0, -0, 1,
		-0.119919, -0.086752, 0.709347, 0.0468, -0.9989, -0,
		-0.163448, -0.088276, 0.88203, 0.0233, -0.9997, -0,
		-0.163448, -0.088276, 0.709347, 0.0233, -0.9997, -0,
		-0.163448, -0.088276, 0.88203, 0.0233, -0.9997, -0,
		-0.119919, -0.086752, 0.709347, 0.0468, -0.9989, -0,
		-0.119919, -0.086752, 0.88203, 0.0468, -0.9989, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.163448, -0.088276, 0.709347, -0, -0, -1,
		-0.207248, -0.088785, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.207248, -0.088785, 0.88203, -0, -0, 1,
		-0.163448, -0.088276, 0.88203, -0, -0, 1,
		-0.163448, -0.088276, 0.709347, 0.0233, -0.9997, -0,
		-0.207248, -0.088785, 0.88203, -0, -1, -0,
		-0.207248, -0.088785, 0.709347, -0, -1, -0,
		-0.207248, -0.088785, 0.88203, -0, -1, -0,
		-0.163448, -0.088276, 0.709347, 0.0233, -0.9997, -0,
		-0.163448, -0.088276, 0.88203, 0.0233, -0.9997, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.207248, -0.088785, 0.709347, -0, -0, -1,
		-0.251047, -0.088276, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.251047, -0.088276, 0.88203, -0, -0, 1,
		-0.207248, -0.088785, 0.88203, -0, -0, 1,
		-0.207248, -0.088785, 0.709347, -0, -1, -0,
		-0.251047, -0.088276, 0.88203, -0.0233, -0.9997, -0,
		-0.251047, -0.088276, 0.709347, -0.0233, -0.9997, -0,
		-0.251047, -0.088276, 0.88203, -0.0233, -0.9997, -0,
		-0.207248, -0.088785, 0.709347, -0, -1, -0,
		-0.207248, -0.088785, 0.88203, -0, -1, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.251047, -0.088276, 0.709347, -0, -0, -1,
		-0.294576, -0.086752, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.294576, -0.086752, 0.88203, -0, -0, 1,
		-0.251047, -0.088276, 0.88203, -0, -0, 1,
		-0.251047, -0.088276, 0.709347, -0.0233, -0.9997, -0,
		-0.294576, -0.086752, 0.88203, -0.0468, -0.9989, -0,
		-0.294576, -0.086752, 0.709347, -0.0468, -0.9989, -0,
		-0.294576, -0.086752, 0.88203, -0.0468, -0.9989, -0,
		-0.251047, -0.088276, 0.709347, -0.0233, -0.9997, -0,
		-0.251047, -0.088276, 0.88203, -0.0233, -0.9997, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.294576, -0.086752, 0.709347, -0, -0, -1,
		-0.337567, -0.084223, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.337567, -0.084223, 0.88203, -0, -0, 1,
		-0.294576, -0.086752, 0.88203, -0, -0, 1,
		-0.294576, -0.086752, 0.709347, -0.0468, -0.9989, -0,
		-0.337567, -0.084223, 0.88203, -0.0708, -0.9975, -0,
		-0.337567, -0.084223, 0.709347, -0.0708, -0.9975, -0,
		-0.337567, -0.084223, 0.88203, -0.0708, -0.9975, -0,
		-0.294576, -0.086752, 0.709347, -0.0468, -0.9989, -0,
		-0.294576, -0.086752, 0.88203, -0.0468, -0.9989, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.337567, -0.084223, 0.709347, -0, -0, -1,
		-0.379755, -0.080704, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.379755, -0.080704, 0.88203, -0, -0, 1,
		-0.337567, -0.084223, 0.88203, -0, -0, 1,
		-0.337567, -0.084223, 0.709347, -0.0708, -0.9975, -0,
		-0.379755, -0.080704, 0.88203, -0.0957, -0.9954, -0,
		-0.379755, -0.080704, 0.709347, -0.0957, -0.9954, -0,
		-0.379755, -0.080704, 0.88203, -0.0957, -0.9954, -0,
		-0.337567, -0.084223, 0.709347, -0.0708, -0.9975, -0,
		-0.337567, -0.084223, 0.88203, -0.0708, -0.9975, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.379755, -0.080704, 0.709347, -0, -0, -1,
		-0.420878, -0.076218, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.420878, -0.076218, 0.88203, -0, -0, 1,
		-0.379755, -0.080704, 0.88203, -0, -0, 1,
		-0.379755, -0.080704, 0.709347, -0.0957, -0.9954, -0,
		-0.420878, -0.076218, 0.88203, -0.1216, -0.9926, -0,
		-0.420878, -0.076218, 0.709347, -0.1216, -0.9926, -0,
		-0.420878, -0.076218, 0.88203, -0.1216, -0.9926, -0,
		-0.379755, -0.080704, 0.709347, -0.0957, -0.9954, -0,
		-0.379755, -0.080704, 0.88203, -0.0957, -0.9954, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.420878, -0.076218, 0.709347, -0, -0, -1,
		-0.460685, -0.07079, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.460685, -0.07079, 0.88203, -0, -0, 1,
		-0.420878, -0.076218, 0.88203, -0, -0, 1,
		-0.420878, -0.076218, 0.709347, -0.1216, -0.9926, -0,
		-0.460685, -0.07079, 0.88203, -0.149, -0.9888, -0,
		-0.460685, -0.07079, 0.709347, -0.149, -0.9888, -0,
		-0.460685, -0.07079, 0.88203, -0.149, -0.9888, -0,
		-0.420878, -0.076218, 0.709347, -0.1216, -0.9926, -0,
		-0.420878, -0.076218, 0.88203, -0.1216, -0.9926, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.460685, -0.07079, 0.709347, -0, -0, -1,
		-0.498929, -0.064456, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.498929, -0.064456, 0.88203, -0, -0, 1,
		-0.460685, -0.07079, 0.88203, -0, -0, 1,
		-0.460685, -0.07079, 0.709347, -0.149, -0.9888, -0,
		-0.498929, -0.064456, 0.88203, -0.1783, -0.984, -0,
		-0.498929, -0.064456, 0.709347, -0.1783, -0.984, -0,
		-0.498929, -0.064456, 0.88203, -0.1783, -0.984, -0,
		-0.460685, -0.07079, 0.709347, -0.149, -0.9888, -0,
		-0.460685, -0.07079, 0.88203, -0.149, -0.9888, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.498929, -0.064456, 0.709347, -0, -0, -1,
		-0.535375, -0.057254, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.535375, -0.057254, 0.88203, -0, -0, 1,
		-0.498929, -0.064456, 0.88203, -0, -0, 1,
		-0.498929, -0.064456, 0.709347, -0.1783, -0.984, -0,
		-0.535375, -0.057254, 0.88203, -0.2101, -0.9777, -0,
		-0.535375, -0.057254, 0.709347, -0.2101, -0.9777, -0,
		-0.535375, -0.057254, 0.88203, -0.2101, -0.9777, -0,
		-0.498929, -0.064456, 0.709347, -0.1783, -0.984, -0,
		-0.498929, -0.064456, 0.88203, -0.1783, -0.984, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.535375, -0.057254, 0.709347, -0, -0, -1,
		-0.569798, -0.049228, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.569798, -0.049228, 0.88203, -0, -0, 1,
		-0.535375, -0.057254, 0.88203, -0, -0, 1,
		-0.535375, -0.057254, 0.709347, -0.2101, -0.9777, -0,
		-0.569798, -0.049228, 0.88203, -0.2449, -0.9695, -0,
		-0.569798, -0.049228, 0.709347, -0.2449, -0.9695, -0,
		-0.569798, -0.049228, 0.88203, -0.2449, -0.9695, -0,
		-0.535375, -0.057254, 0.709347, -0.2101, -0.9777, -0,
		-0.535375, -0.057254, 0.88203, -0.2101, -0.9777, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.569798, -0.049228, 0.709347, -0, -0, -1,
		-0.601986, -0.040428, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.601986, -0.040428, 0.88203, -0, -0, 1,
		-0.569798, -0.049228, 0.88203, -0, -0, 1,
		-0.569798, -0.049228, 0.709347, -0.2449, -0.9695, -0,
		-0.601986, -0.040428, 0.88203, -0.2836, -0.9589, -0,
		-0.601986, -0.040428, 0.709347, -0.2836, -0.9589, -0,
		-0.601986, -0.040428, 0.88203, -0.2836, -0.9589, -0,
		-0.569798, -0.049228, 0.709347, -0.2449, -0.9695, -0,
		-0.569798, -0.049228, 0.88203, -0.2449, -0.9695, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.601986, -0.040428, 0.709347, -0, -0, -1,
		-0.63174, -0.030909, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.63174, -0.030909, 0.88203, -0, -0, 1,
		-0.601986, -0.040428, 0.88203, -0, -0, 1,
		-0.601986, -0.040428, 0.709347, -0.2836, -0.9589, -0,
		-0.63174, -0.030909, 0.88203, -0.3272, -0.945, -0,
		-0.63174, -0.030909, 0.709347, -0.3272, -0.945, -0,
		-0.63174, -0.030909, 0.88203, -0.3272, -0.945, -0,
		-0.601986, -0.040428, 0.709347, -0.2836, -0.9589, -0,
		-0.601986, -0.040428, 0.88203, -0.2836, -0.9589, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.63174, -0.030909, 0.709347, -0, -0, -1,
		-0.658877, -0.020729, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.658877, -0.020729, 0.88203, -0, -0, 1,
		-0.63174, -0.030909, 0.88203, -0, -0, 1,
		-0.63174, -0.030909, 0.709347, -0.3272, -0.945, -0,
		-0.658877, -0.020729, 0.88203, -0.377, -0.9262, -0,
		-0.658877, -0.020729, 0.709347, -0.377, -0.9262, -0,
		-0.658877, -0.020729, 0.88203, -0.377, -0.9262, -0,
		-0.63174, -0.030909, 0.709347, -0.3272, -0.945, -0,
		-0.63174, -0.030909, 0.88203, -0.3272, -0.945, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.658877, -0.020729, 0.709347, -0, -0, -1,
		-0.683229, -0.00995, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.683229, -0.00995, 0.88203, -0, -0, 1,
		-0.658877, -0.020729, 0.88203, -0, -0, 1,
		-0.658877, -0.020729, 0.709347, -0.377, -0.9262, -0,
		-0.683229, -0.00995, 0.88203, -0.4346, -0.9006, -0,
		-0.683229, -0.00995, 0.709347, -0.4346, -0.9006, -0,
		-0.683229, -0.00995, 0.88203, -0.4346, -0.9006, -0,
		-0.658877, -0.020729, 0.709347, -0.377, -0.9262, -0,
		-0.658877, -0.020729, 0.88203, -0.377, -0.9262, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.683229, -0.00995, 0.709347, -0, -0, -1,
		-0.704647, 0.001361, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.704647, 0.001361, 0.88203, -0, -0, 1,
		-0.683229, -0.00995, 0.88203, -0, -0, 1,
		-0.683229, -0.00995, 0.709347, -0.4346, -0.9006, -0,
		-0.704647, 0.001361, 0.88203, -0.502, -0.8649, -0,
		-0.704647, 0.001361, 0.709347, -0.502, -0.8649, -0,
		-0.704647, 0.001361, 0.88203, -0.502, -0.8649, -0,
		-0.683229, -0.00995, 0.709347, -0.4346, -0.9006, -0,
		-0.683229, -0.00995, 0.88203, -0.4346, -0.9006, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.704647, 0.001361, 0.709347, -0, -0, -1,
		-0.722998, 0.013134, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.722998, 0.013134, 0.88203, -0, -0, 1,
		-0.704647, 0.001361, 0.88203, -0, -0, 1,
		-0.704647, 0.001361, 0.709347, -0.502, -0.8649, -0,
		-0.722998, 0.013134, 0.88203, -0.5811, -0.8138, -0,
		-0.722998, 0.013134, 0.709347, -0.5811, -0.8138, -0,
		-0.722998, 0.013134, 0.88203, -0.5811, -0.8138, -0,
		-0.704647, 0.001361, 0.709347, -0.502, -0.8649, -0,
		-0.704647, 0.001361, 0.88203, -0.502, -0.8649, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.722998, 0.013134, 0.709347, -0, -0, -1,
		-0.738169, 0.025296, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.738169, 0.025296, 0.88203, -0, -0, 1,
		-0.722998, 0.013134, 0.88203, -0, -0, 1,
		-0.722998, 0.013134, 0.709347, -0.5811, -0.8138, -0,
		-0.738169, 0.025296, 0.88203, -0.6731, -0.7395, -0,
		-0.738169, 0.025296, 0.709347, -0.6731, -0.7395, -0,
		-0.738169, 0.025296, 0.88203, -0.6731, -0.7395, -0,
		-0.722998, 0.013134, 0.709347, -0.5811, -0.8138, -0,
		-0.722998, 0.013134, 0.88203, -0.5811, -0.8138, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.738169, 0.025296, 0.709347, -0, -0, -1,
		-0.750068, 0.037773, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.750068, 0.037773, 0.88203, -0, -0, 1,
		-0.738169, 0.025296, 0.88203, -0, -0, 1,
		-0.738169, 0.025296, 0.709347, -0.6731, -0.7395, -0,
		-0.750068, 0.037773, 0.88203, -0.7764, -0.6303, -0,
		-0.750068, 0.037773, 0.709347, -0.7764, -0.6302, -0,
		-0.750068, 0.037773, 0.88203, -0.7764, -0.6303, -0,
		-0.738169, 0.025296, 0.709347, -0.6731, -0.7395, -0,
		-0.738169, 0.025296, 0.88203, -0.6731, -0.7395, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.750068, 0.037773, 0.709347, -0, -0, -1,
		-0.758619, 0.050487, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.758619, 0.050487, 0.88203, -0, -0, 1,
		-0.750068, 0.037773, 0.88203, -0, -0, 1,
		-0.750068, 0.037773, 0.709347, -0.7764, -0.6302, -0,
		-0.758619, 0.050487, 0.88203, -0.8815, -0.4721, -0,
		-0.758619, 0.050487, 0.709347, -0.8815, -0.4721, -0,
		-0.758619, 0.050487, 0.88203, -0.8815, -0.4721, -0,
		-0.750068, 0.037773, 0.709347, -0.7764, -0.6302, -0,
		-0.750068, 0.037773, 0.88203, -0.7764, -0.6303, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.758619, 0.050487, 0.709347, -0, -0, -1,
		-0.763771, 0.063361, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.763771, 0.063361, 0.88203, -0, -0, 1,
		-0.758619, 0.050487, 0.88203, -0, -0, 1,
		-0.758619, 0.050487, 0.709347, -0.8815, -0.4721, -0,
		-0.763771, 0.063361, 0.88203, -0.9664, -0.2572, -0,
		-0.763771, 0.063361, 0.709347, -0.9664, -0.2572, -0,
		-0.763771, 0.063361, 0.88203, -0.9664, -0.2572, -0,
		-0.758619, 0.050487, 0.709347, -0.8815, -0.4721, -0,
		-0.758619, 0.050487, 0.88203, -0.8815, -0.4721, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.763771, 0.063361, 0.709347, -0, -0, -1,
		-0.765492, 0.076314, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.765492, 0.076314, 0.88203, -0, -0, 1,
		-0.763771, 0.063361, 0.88203, -0, -0, 1,
		-0.763771, 0.063361, 0.709347, -0.9664, -0.2572, -0,
		-0.765492, 0.076314, 0.88203, -1, -0, -0,
		-0.765492, 0.076314, 0.709347, -1, -0, -0,
		-0.765492, 0.076314, 0.88203, -1, -0, -0,
		-0.763771, 0.063361, 0.709347, -0.9664, -0.2572, -0,
		-0.763771, 0.063361, 0.88203, -0.9664, -0.2572, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.765492, 0.076314, 0.709347, -0, -0, -1,
		-0.763771, 0.089268, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.763771, 0.089268, 0.88203, -0, -0, 1,
		-0.765492, 0.076314, 0.88203, -0, -0, 1,
		-0.765492, 0.076314, 0.709347, -1, -0, -0,
		-0.763771, 0.089268, 0.88203, -0.9664, 0.2572, -0,
		-0.763771, 0.089268, 0.709347, -0.9664, 0.2572, -0,
		-0.763771, 0.089268, 0.88203, -0.9664, 0.2572, -0,
		-0.765492, 0.076314, 0.709347, -1, -0, -0,
		-0.765492, 0.076314, 0.88203, -1, -0, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.763771, 0.089268, 0.709347, -0, -0, -1,
		-0.758619, 0.102142, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.758619, 0.102142, 0.88203, -0, -0, 1,
		-0.763771, 0.089268, 0.88203, -0, -0, 1,
		-0.763771, 0.089268, 0.709347, -0.9664, 0.2572, -0,
		-0.758619, 0.102142, 0.88203, -0.8815, 0.4721, -0,
		-0.758619, 0.102142, 0.709347, -0.8815, 0.4721, -0,
		-0.758619, 0.102142, 0.88203, -0.8815, 0.4721, -0,
		-0.763771, 0.089268, 0.709347, -0.9664, 0.2572, -0,
		-0.763771, 0.089268, 0.88203, -0.9664, 0.2572, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.758619, 0.102142, 0.709347, -0, -0, -1,
		-0.750068, 0.114856, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.750068, 0.114856, 0.88203, -0, -0, 1,
		-0.758619, 0.102142, 0.88203, -0, -0, 1,
		-0.758619, 0.102142, 0.709347, -0.8815, 0.4721, -0,
		-0.750068, 0.114856, 0.88203, -0.7764, 0.6302, -0,
		-0.750068, 0.114856, 0.709347, -0.7764, 0.6303, -0,
		-0.750068, 0.114856, 0.88203, -0.7764, 0.6302, -0,
		-0.758619, 0.102142, 0.709347, -0.8815, 0.4721, -0,
		-0.758619, 0.102142, 0.88203, -0.8815, 0.4721, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.750068, 0.114856, 0.709347, -0, -0, -1,
		-0.738169, 0.127333, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.738169, 0.127333, 0.88203, -0, -0, 1,
		-0.750068, 0.114856, 0.88203, -0, -0, 1,
		-0.750068, 0.114856, 0.709347, -0.7764, 0.6303, -0,
		-0.738169, 0.127333, 0.88203, -0.6731, 0.7395, -0,
		-0.738169, 0.127333, 0.709347, -0.6731, 0.7395, -0,
		-0.738169, 0.127333, 0.88203, -0.6731, 0.7395, -0,
		-0.750068, 0.114856, 0.709347, -0.7764, 0.6303, -0,
		-0.750068, 0.114856, 0.88203, -0.7764, 0.6302, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.738169, 0.127333, 0.709347, -0, -0, -1,
		-0.722998, 0.139495, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.722998, 0.139495, 0.88203, -0, -0, 1,
		-0.738169, 0.127333, 0.88203, -0, -0, 1,
		-0.738169, 0.127333, 0.709347, -0.6731, 0.7395, -0,
		-0.722998, 0.139495, 0.88203, -0.5811, 0.8138, -0,
		-0.722998, 0.139495, 0.709347, -0.5811, 0.8138, -0,
		-0.722998, 0.139495, 0.88203, -0.5811, 0.8138, -0,
		-0.738169, 0.127333, 0.709347, -0.6731, 0.7395, -0,
		-0.738169, 0.127333, 0.88203, -0.6731, 0.7395, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.722998, 0.139495, 0.709347, -0, -0, -1,
		-0.704647, 0.151268, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.704647, 0.151268, 0.88203, -0, -0, 1,
		-0.722998, 0.139495, 0.88203, -0, -0, 1,
		-0.722998, 0.139495, 0.709347, -0.5811, 0.8138, -0,
		-0.704647, 0.151268, 0.88203, -0.502, 0.8649, -0,
		-0.704647, 0.151268, 0.709347, -0.502, 0.8649, -0,
		-0.704647, 0.151268, 0.88203, -0.502, 0.8649, -0,
		-0.722998, 0.139495, 0.709347, -0.5811, 0.8138, -0,
		-0.722998, 0.139495, 0.88203, -0.5811, 0.8138, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.704647, 0.151268, 0.709347, -0, -0, -1,
		-0.683229, 0.162579, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.683229, 0.162579, 0.88203, -0, -0, 1,
		-0.704647, 0.151268, 0.88203, -0, -0, 1,
		-0.704647, 0.151268, 0.709347, -0.502, 0.8649, -0,
		-0.683229, 0.162579, 0.88203, -0.4346, 0.9006, -0,
		-0.683229, 0.162579, 0.709347, -0.4346, 0.9006, -0,
		-0.683229, 0.162579, 0.88203, -0.4346, 0.9006, -0,
		-0.704647, 0.151268, 0.709347, -0.502, 0.8649, -0,
		-0.704647, 0.151268, 0.88203, -0.502, 0.8649, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.683229, 0.162579, 0.709347, -0, -0, -1,
		-0.658877, 0.173357, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.658877, 0.173357, 0.88203, -0, -0, 1,
		-0.683229, 0.162579, 0.88203, -0, -0, 1,
		-0.683229, 0.162579, 0.709347, -0.4346, 0.9006, -0,
		-0.658877, 0.173357, 0.88203, -0.377, 0.9262, -0,
		-0.658877, 0.173357, 0.709347, -0.377, 0.9262, -0,
		-0.658877, 0.173357, 0.88203, -0.377, 0.9262, -0,
		-0.683229, 0.162579, 0.709347, -0.4346, 0.9006, -0,
		-0.683229, 0.162579, 0.88203, -0.4346, 0.9006, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.658877, 0.173357, 0.709347, -0, -0, -1,
		-0.63174, 0.183538, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.63174, 0.183538, 0.88203, -0, -0, 1,
		-0.658877, 0.173357, 0.88203, -0, -0, 1,
		-0.658877, 0.173357, 0.709347, -0.377, 0.9262, -0,
		-0.63174, 0.183538, 0.88203, -0.3272, 0.945, -0,
		-0.63174, 0.183538, 0.709347, -0.3272, 0.945, -0,
		-0.63174, 0.183538, 0.88203, -0.3272, 0.945, -0,
		-0.658877, 0.173357, 0.709347, -0.377, 0.9262, -0,
		-0.658877, 0.173357, 0.88203, -0.377, 0.9262, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.63174, 0.183538, 0.709347, -0, -0, -1,
		-0.601986, 0.193057, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.601986, 0.193057, 0.88203, -0, -0, 1,
		-0.63174, 0.183538, 0.88203, -0, -0, 1,
		-0.63174, 0.183538, 0.709347, -0.3272, 0.945, -0,
		-0.601986, 0.193057, 0.88203, -0.2836, 0.9589, -0,
		-0.601986, 0.193057, 0.709347, -0.2836, 0.9589, -0,
		-0.601986, 0.193057, 0.88203, -0.2836, 0.9589, -0,
		-0.63174, 0.183538, 0.709347, -0.3272, 0.945, -0,
		-0.63174, 0.183538, 0.88203, -0.3272, 0.945, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.601986, 0.193057, 0.709347, -0, -0, -1,
		-0.569798, 0.201857, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.569798, 0.201857, 0.88203, -0, -0, 1,
		-0.601986, 0.193057, 0.88203, -0, -0, 1,
		-0.601986, 0.193057, 0.709347, -0.2836, 0.9589, -0,
		-0.569798, 0.201857, 0.88203, -0.2449, 0.9695, -0,
		-0.569798, 0.201857, 0.709347, -0.2449, 0.9695, -0,
		-0.569798, 0.201857, 0.88203, -0.2449, 0.9695, -0,
		-0.601986, 0.193057, 0.709347, -0.2836, 0.9589, -0,
		-0.601986, 0.193057, 0.88203, -0.2836, 0.9589, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.569798, 0.201857, 0.709347, -0, -0, -1,
		-0.535375, 0.209883, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.535375, 0.209883, 0.88203, -0, -0, 1,
		-0.569798, 0.201857, 0.88203, -0, -0, 1,
		-0.569798, 0.201857, 0.709347, -0.2449, 0.9695, -0,
		-0.535375, 0.209883, 0.88203, -0.2101, 0.9777, -0,
		-0.535375, 0.209883, 0.709347, -0.2101, 0.9777, -0,
		-0.535375, 0.209883, 0.88203, -0.2101, 0.9777, -0,
		-0.569798, 0.201857, 0.709347, -0.2449, 0.9695, -0,
		-0.569798, 0.201857, 0.88203, -0.2449, 0.9695, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.535375, 0.209883, 0.709347, -0, -0, -1,
		-0.498929, 0.217085, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.498929, 0.217085, 0.88203, -0, -0, 1,
		-0.535375, 0.209883, 0.88203, -0, -0, 1,
		-0.535375, 0.209883, 0.709347, -0.2101, 0.9777, -0,
		-0.498929, 0.217085, 0.88203, -0.1783, 0.984, -0,
		-0.498929, 0.217085, 0.709347, -0.1783, 0.984, -0,
		-0.498929, 0.217085, 0.88203, -0.1783, 0.984, -0,
		-0.535375, 0.209883, 0.709347, -0.2101, 0.9777, -0,
		-0.535375, 0.209883, 0.88203, -0.2101, 0.9777, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.498929, 0.217085, 0.709347, -0, -0, -1,
		-0.460685, 0.223419, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.460685, 0.223419, 0.88203, -0, -0, 1,
		-0.498929, 0.217085, 0.88203, -0, -0, 1,
		-0.498929, 0.217085, 0.709347, -0.1783, 0.984, -0,
		-0.460685, 0.223419, 0.88203, -0.149, 0.9888, -0,
		-0.460685, 0.223419, 0.709347, -0.149, 0.9888, -0,
		-0.460685, 0.223419, 0.88203, -0.149, 0.9888, -0,
		-0.498929, 0.217085, 0.709347, -0.1783, 0.984, -0,
		-0.498929, 0.217085, 0.88203, -0.1783, 0.984, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.460685, 0.223419, 0.709347, -0, -0, -1,
		-0.420878, 0.228846, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.420878, 0.228846, 0.88203, -0, -0, 1,
		-0.460685, 0.223419, 0.88203, -0, -0, 1,
		-0.460685, 0.223419, 0.709347, -0.149, 0.9888, -0,
		-0.420878, 0.228846, 0.88203, -0.1216, 0.9926, -0,
		-0.420878, 0.228846, 0.709347, -0.1216, 0.9926, -0,
		-0.420878, 0.228846, 0.88203, -0.1216, 0.9926, -0,
		-0.460685, 0.223419, 0.709347, -0.149, 0.9888, -0,
		-0.460685, 0.223419, 0.88203, -0.149, 0.9888, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.420878, 0.228846, 0.709347, -0, -0, -1,
		-0.379755, 0.233333, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.379755, 0.233333, 0.88203, -0, -0, 1,
		-0.420878, 0.228846, 0.88203, -0, -0, 1,
		-0.420878, 0.228846, 0.709347, -0.1216, 0.9926, -0,
		-0.379755, 0.233333, 0.88203, -0.0957, 0.9954, -0,
		-0.379755, 0.233333, 0.709347, -0.0957, 0.9954, -0,
		-0.379755, 0.233333, 0.88203, -0.0957, 0.9954, -0,
		-0.420878, 0.228846, 0.709347, -0.1216, 0.9926, -0,
		-0.420878, 0.228846, 0.88203, -0.1216, 0.9926, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.379755, 0.233333, 0.709347, -0, -0, -1,
		-0.337567, 0.236852, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.337567, 0.236852, 0.88203, -0, -0, 1,
		-0.379755, 0.233333, 0.88203, -0, -0, 1,
		-0.379755, 0.233333, 0.709347, -0.0957, 0.9954, -0,
		-0.337567, 0.236852, 0.88203, -0.0708, 0.9975, -0,
		-0.337567, 0.236852, 0.709347, -0.0708, 0.9975, -0,
		-0.337567, 0.236852, 0.88203, -0.0708, 0.9975, -0,
		-0.379755, 0.233333, 0.709347, -0.0957, 0.9954, -0,
		-0.379755, 0.233333, 0.88203, -0.0957, 0.9954, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.337567, 0.236852, 0.709347, -0, -0, -1,
		-0.294576, 0.239381, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.294576, 0.239381, 0.88203, -0, -0, 1,
		-0.337567, 0.236852, 0.88203, -0, -0, 1,
		-0.337567, 0.236852, 0.709347, -0.0708, 0.9975, -0,
		-0.294576, 0.239381, 0.88203, -0.0468, 0.9989, -0,
		-0.294576, 0.239381, 0.709347, -0.0468, 0.9989, -0,
		-0.294576, 0.239381, 0.88203, -0.0468, 0.9989, -0,
		-0.337567, 0.236852, 0.709347, -0.0708, 0.9975, -0,
		-0.337567, 0.236852, 0.88203, -0.0708, 0.9975, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.294576, 0.239381, 0.709347, -0, -0, -1,
		-0.251047, 0.240905, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.251047, 0.240905, 0.88203, -0, -0, 1,
		-0.294576, 0.239381, 0.88203, -0, -0, 1,
		-0.294576, 0.239381, 0.709347, -0.0468, 0.9989, -0,
		-0.251047, 0.240905, 0.88203, -0.0233, 0.9997, -0,
		-0.251047, 0.240905, 0.709347, -0.0233, 0.9997, -0,
		-0.251047, 0.240905, 0.88203, -0.0233, 0.9997, -0,
		-0.294576, 0.239381, 0.709347, -0.0468, 0.9989, -0,
		-0.294576, 0.239381, 0.88203, -0.0468, 0.9989, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.251047, 0.240905, 0.709347, -0, -0, -1,
		-0.207248, 0.241414, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.207248, 0.241414, 0.88203, -0, -0, 1,
		-0.251047, 0.240905, 0.88203, -0, -0, 1,
		-0.251047, 0.240905, 0.709347, -0.0233, 0.9997, -0,
		-0.207248, 0.241414, 0.88203, -0, 1, -0,
		-0.207248, 0.241414, 0.709347, -0, 1, -0,
		-0.207248, 0.241414, 0.88203, -0, 1, -0,
		-0.251047, 0.240905, 0.709347, -0.0233, 0.9997, -0,
		-0.251047, 0.240905, 0.88203, -0.0233, 0.9997, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.207248, 0.241414, 0.709347, -0, -0, -1,
		-0.163448, 0.240905, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.163448, 0.240905, 0.88203, -0, -0, 1,
		-0.207248, 0.241414, 0.88203, -0, -0, 1,
		-0.207248, 0.241414, 0.709347, -0, 1, -0,
		-0.163448, 0.240905, 0.88203, 0.0233, 0.9997, -0,
		-0.163448, 0.240905, 0.709347, 0.0233, 0.9997, -0,
		-0.163448, 0.240905, 0.88203, 0.0233, 0.9997, -0,
		-0.207248, 0.241414, 0.709347, -0, 1, -0,
		-0.207248, 0.241414, 0.88203, -0, 1, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.163448, 0.240905, 0.709347, -0, -0, -1,
		-0.119919, 0.239381, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.119919, 0.239381, 0.88203, -0, -0, 1,
		-0.163448, 0.240905, 0.88203, -0, -0, 1,
		-0.163448, 0.240905, 0.709347, 0.0233, 0.9997, -0,
		-0.119919, 0.239381, 0.88203, 0.0468, 0.9989, -0,
		-0.119919, 0.239381, 0.709347, 0.0468, 0.9989, -0,
		-0.119919, 0.239381, 0.88203, 0.0468, 0.9989, -0,
		-0.163448, 0.240905, 0.709347, 0.0233, 0.9997, -0,
		-0.163448, 0.240905, 0.88203, 0.0233, 0.9997, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.119919, 0.239381, 0.709347, -0, -0, -1,
		-0.076928, 0.236852, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.076928, 0.236852, 0.88203, -0, -0, 1,
		-0.119919, 0.239381, 0.88203, -0, -0, 1,
		-0.119919, 0.239381, 0.709347, 0.0468, 0.9989, -0,
		-0.076928, 0.236852, 0.88203, 0.0708, 0.9975, -0,
		-0.076928, 0.236852, 0.709347, 0.0708, 0.9975, -0,
		-0.076928, 0.236852, 0.88203, 0.0708, 0.9975, -0,
		-0.119919, 0.239381, 0.709347, 0.0468, 0.9989, -0,
		-0.119919, 0.239381, 0.88203, 0.0468, 0.9989, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.076928, 0.236852, 0.709347, -0, -0, -1,
		-0.034741, 0.233333, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		-0.034741, 0.233333, 0.88203, -0, -0, 1,
		-0.076928, 0.236852, 0.88203, -0, -0, 1,
		-0.076928, 0.236852, 0.709347, 0.0708, 0.9975, -0,
		-0.034741, 0.233333, 0.88203, 0.0957, 0.9954, -0,
		-0.034741, 0.233333, 0.709347, 0.0957, 0.9954, -0,
		-0.034741, 0.233333, 0.88203, 0.0957, 0.9954, -0,
		-0.076928, 0.236852, 0.709347, 0.0708, 0.9975, -0,
		-0.076928, 0.236852, 0.88203, 0.0708, 0.9975, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		-0.034741, 0.233333, 0.709347, -0, -0, -1,
		0.006383, 0.228846, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		0.006383, 0.228846, 0.88203, -0, -0, 1,
		-0.034741, 0.233333, 0.88203, -0, -0, 1,
		-0.034741, 0.233333, 0.709347, 0.0957, 0.9954, -0,
		0.006383, 0.228846, 0.88203, 0.1216, 0.9926, -0,
		0.006383, 0.228846, 0.709347, 0.1216, 0.9926, -0,
		0.006383, 0.228846, 0.88203, 0.1216, 0.9926, -0,
		-0.034741, 0.233333, 0.709347, 0.0957, 0.9954, -0,
		-0.034741, 0.233333, 0.88203, 0.0957, 0.9954, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		0.006383, 0.228846, 0.709347, -0, -0, -1,
		0.04619, 0.223419, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		0.04619, 0.223419, 0.88203, -0, -0, 1,
		0.006383, 0.228846, 0.88203, -0, -0, 1,
		0.006383, 0.228846, 0.709347, 0.1216, 0.9926, -0,
		0.04619, 0.223419, 0.88203, 0.149, 0.9888, -0,
		0.04619, 0.223419, 0.709347, 0.149, 0.9888, -0,
		0.04619, 0.223419, 0.88203, 0.149, 0.9888, -0,
		0.006383, 0.228846, 0.709347, 0.1216, 0.9926, -0,
		0.006383, 0.228846, 0.88203, 0.1216, 0.9926, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		0.04619, 0.223419, 0.709347, -0, -0, -1,
		0.084434, 0.217085, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		0.084434, 0.217085, 0.88203, -0, -0, 1,
		0.04619, 0.223419, 0.88203, -0, -0, 1,
		0.04619, 0.223419, 0.709347, 0.149, 0.9888, -0,
		0.084434, 0.217085, 0.88203, 0.1783, 0.984, -0,
		0.084434, 0.217085, 0.709347, 0.1783, 0.984, -0,
		0.084434, 0.217085, 0.88203, 0.1783, 0.984, -0,
		0.04619, 0.223419, 0.709347, 0.149, 0.9888, -0,
		0.04619, 0.223419, 0.88203, 0.149, 0.9888, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		0.084434, 0.217085, 0.709347, -0, -0, -1,
		0.12088, 0.209883, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		0.12088, 0.209883, 0.88203, -0, -0, 1,
		0.084434, 0.217085, 0.88203, -0, -0, 1,
		0.084434, 0.217085, 0.709347, 0.1783, 0.984, -0,
		0.12088, 0.209883, 0.88203, 0.2101, 0.9777, -0,
		0.12088, 0.209883, 0.709347, 0.2101, 0.9777, -0,
		0.12088, 0.209883, 0.88203, 0.2101, 0.9777, -0,
		0.084434, 0.217085, 0.709347, 0.1783, 0.984, -0,
		0.084434, 0.217085, 0.88203, 0.1783, 0.984, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		0.12088, 0.209883, 0.709347, -0, -0, -1,
		0.155303, 0.201857, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		0.155303, 0.201857, 0.88203, -0, -0, 1,
		0.12088, 0.209883, 0.88203, -0, -0, 1,
		0.12088, 0.209883, 0.709347, 0.2101, 0.9777, -0,
		0.155303, 0.201857, 0.88203, 0.2449, 0.9695, -0,
		0.155303, 0.201857, 0.709347, 0.2449, 0.9695, -0,
		0.155303, 0.201857, 0.88203, 0.2449, 0.9695, -0,
		0.12088, 0.209883, 0.709347, 0.2101, 0.9777, -0,
		0.12088, 0.209883, 0.88203, 0.2101, 0.9777, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		0.155303, 0.201857, 0.709347, -0, -0, -1,
		0.187491, 0.193057, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		0.187491, 0.193057, 0.88203, -0, -0, 1,
		0.155303, 0.201857, 0.88203, -0, -0, 1,
		0.155303, 0.201857, 0.709347, 0.2449, 0.9695, -0,
		0.187491, 0.193057, 0.88203, 0.2836, 0.9589, -0,
		0.187491, 0.193057, 0.709347, 0.2836, 0.9589, -0,
		0.187491, 0.193057, 0.88203, 0.2836, 0.9589, -0,
		0.155303, 0.201857, 0.709347, 0.2449, 0.9695, -0,
		0.155303, 0.201857, 0.88203, 0.2449, 0.9695, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		0.187491, 0.193057, 0.709347, -0, -0, -1,
		0.217245, 0.183538, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		0.217245, 0.183538, 0.88203, -0, -0, 1,
		0.187491, 0.193057, 0.88203, -0, -0, 1,
		0.187491, 0.193057, 0.709347, 0.2836, 0.9589, -0,
		0.217245, 0.183538, 0.88203, 0.3272, 0.945, -0,
		0.217245, 0.183538, 0.709347, 0.3272, 0.945, -0,
		0.217245, 0.183538, 0.88203, 0.3272, 0.945, -0,
		0.187491, 0.193057, 0.709347, 0.2836, 0.9589, -0,
		0.187491, 0.193057, 0.88203, 0.2836, 0.9589, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		0.217245, 0.183538, 0.709347, -0, -0, -1,
		0.244382, 0.173357, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		0.244382, 0.173357, 0.88203, -0, -0, 1,
		0.217245, 0.183538, 0.88203, -0, -0, 1,
		0.217245, 0.183538, 0.709347, 0.3272, 0.945, -0,
		0.244382, 0.173357, 0.88203, 0.377, 0.9262, -0,
		0.244382, 0.173357, 0.709347, 0.377, 0.9262, -0,
		0.244382, 0.173357, 0.88203, 0.377, 0.9262, -0,
		0.217245, 0.183538, 0.709347, 0.3272, 0.945, -0,
		0.217245, 0.183538, 0.88203, 0.3272, 0.945, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		0.244382, 0.173357, 0.709347, -0, -0, -1,
		0.268734, 0.162579, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		0.268734, 0.162579, 0.88203, -0, -0, 1,
		0.244382, 0.173357, 0.88203, -0, -0, 1,
		0.244382, 0.173357, 0.709347, 0.377, 0.9262, -0,
		0.268734, 0.162579, 0.88203, 0.4346, 0.9006, -0,
		0.268734, 0.162579, 0.709347, 0.4346, 0.9006, -0,
		0.268734, 0.162579, 0.88203, 0.4346, 0.9006, -0,
		0.244382, 0.173357, 0.709347, 0.377, 0.9262, -0,
		0.244382, 0.173357, 0.88203, 0.377, 0.9262, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		0.268734, 0.162579, 0.709347, -0, -0, -1,
		0.290152, 0.151268, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		0.290152, 0.151268, 0.88203, -0, -0, 1,
		0.268734, 0.162579, 0.88203, -0, -0, 1,
		0.268734, 0.162579, 0.709347, 0.4346, 0.9006, -0,
		0.290152, 0.151268, 0.88203, 0.502, 0.8649, -0,
		0.290152, 0.151268, 0.709347, 0.502, 0.8649, -0,
		0.290152, 0.151268, 0.88203, 0.502, 0.8649, -0,
		0.268734, 0.162579, 0.709347, 0.4346, 0.9006, -0,
		0.268734, 0.162579, 0.88203, 0.4346, 0.9006, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		0.290152, 0.151268, 0.709347, -0, -0, -1,
		0.308503, 0.139495, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		0.308503, 0.139495, 0.88203, -0, -0, 1,
		0.290152, 0.151268, 0.88203, -0, -0, 1,
		0.290152, 0.151268, 0.709347, 0.502, 0.8649, -0,
		0.308503, 0.139495, 0.88203, 0.5811, 0.8138, -0,
		0.308503, 0.139495, 0.709347, 0.5811, 0.8138, -0,
		0.308503, 0.139495, 0.88203, 0.5811, 0.8138, -0,
		0.290152, 0.151268, 0.709347, 0.502, 0.8649, -0,
		0.290152, 0.151268, 0.88203, 0.502, 0.8649, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		0.308503, 0.139495, 0.709347, -0, -0, -1,
		0.323674, 0.127333, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		0.323674, 0.127333, 0.88203, -0, -0, 1,
		0.308503, 0.139495, 0.88203, -0, -0, 1,
		0.308503, 0.139495, 0.709347, 0.5811, 0.8138, -0,
		0.323674, 0.127333, 0.88203, 0.6731, 0.7395, -0,
		0.323674, 0.127333, 0.709347, 0.6731, 0.7395, -0,
		0.323674, 0.127333, 0.88203, 0.6731, 0.7395, -0,
		0.308503, 0.139495, 0.709347, 0.5811, 0.8138, -0,
		0.308503, 0.139495, 0.88203, 0.5811, 0.8138, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		0.323674, 0.127333, 0.709347, -0, -0, -1,
		0.335572, 0.114856, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		0.335572, 0.114856, 0.88203, -0, -0, 1,
		0.323674, 0.127333, 0.88203, -0, -0, 1,
		0.323674, 0.127333, 0.709347, 0.6731, 0.7395, -0,
		0.335572, 0.114856, 0.88203, 0.7764, 0.6303, -0,
		0.335572, 0.114856, 0.709347, 0.7764, 0.6303, -0,
		0.335572, 0.114856, 0.88203, 0.7764, 0.6303, -0,
		0.323674, 0.127333, 0.709347, 0.6731, 0.7395, -0,
		0.323674, 0.127333, 0.88203, 0.6731, 0.7395, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		0.335572, 0.114856, 0.709347, -0, -0, -1,
		0.344124, 0.102142, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		0.344124, 0.102142, 0.88203, -0, -0, 1,
		0.335572, 0.114856, 0.88203, -0, -0, 1,
		0.335572, 0.114856, 0.709347, 0.7764, 0.6303, -0,
		0.344124, 0.102142, 0.88203, 0.8815, 0.4721, -0,
		0.344124, 0.102142, 0.709347, 0.8815, 0.4721, -0,
		0.344124, 0.102142, 0.88203, 0.8815, 0.4721, -0,
		0.335572, 0.114856, 0.709347, 0.7764, 0.6303, -0,
		0.335572, 0.114856, 0.88203, 0.7764, 0.6303, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		0.344124, 0.102142, 0.709347, -0, -0, -1,
		0.349276, 0.089268, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		0.349276, 0.089268, 0.88203, -0, -0, 1,
		0.344124, 0.102142, 0.88203, -0, -0, 1,
		0.344124, 0.102142, 0.709347, 0.8815, 0.4721, -0,
		0.349276, 0.089268, 0.88203, 0.9664, 0.2572, -0,
		0.349276, 0.089268, 0.709347, 0.9664, 0.2572, -0,
		0.349276, 0.089268, 0.88203, 0.9664, 0.2572, -0,
		0.344124, 0.102142, 0.709347, 0.8815, 0.4721, -0,
		0.344124, 0.102142, 0.88203, 0.8815, 0.4721, -0,
		-0.207248, 0.076314, 0.709347, -0, -0, -1,
		0.349276, 0.089268, 0.709347, -0, -0, -1,
		0.350997, 0.076314, 0.709347, -0, -0, -1,
		-0.207248, 0.076314, 0.88203, -0, -0, 1,
		0.350997, 0.076314, 0.88203, -0, -0, 1,
		0.349276, 0.089268, 0.88203, -0, -0, 1,
		0.349276, 0.089268, 0.709347, 0.9664, 0.2572, -0,
		0.350997, 0.076314, 0.88203, 1, -0, -0,
		0.350997, 0.076314, 0.709347, 1, -0, -0,
		0.350997, 0.076314, 0.88203, 1, -0, -0,
		0.349276, 0.089268, 0.709347, 0.9664, 0.2572, -0,
		0.349276, 0.089268, 0.88203, 0.9664, 0.2572, -0,
	};
	float * tank2 = new float[24 * 6]
	{
		-0.961451, 0.24133, 1.51476, -0, -0, 1,
		-0.728066, 0.062759, 1.51476, -0, -0, 1,
		-0.72626, 0.388434, 1.51476, -0, -0, 1,
		-0.72626, 0.388434, 0.806152, -0, -0, -1,
		-0.728066, 0.062759, 0.806152, -0, -0, -1,
		-0.961451, 0.24133, 0.806152, -0, -0, -1,
		-0.961451, 0.24133, 1.51476, -0.5303, 0.8478, -0,
		-0.72626, 0.388434, 0.806152, -0.5303, 0.8478, -0,
		-0.961451, 0.24133, 0.806152, -0.5303, 0.8478, -0,
		-0.961451, 0.24133, 1.51476, -0.5303, 0.8478, -0,
		-0.72626, 0.388434, 1.51476, -0.5303, 0.8478, -0,
		-0.72626, 0.388434, 0.806152, -0.5303, 0.8478, -0,
		-0.726216, 0.388997, 1.51427, 1, -0.0056, -0,
		-0.728066, 0.062759, 0.806152, 1, -0.0057, -0,
		-0.72626, 0.388434, 0.806152, 1, -0.0055, -0.0001,
		-0.726216, 0.388997, 1.51427, 1, -0.0056, -0,
		-0.728066, 0.062759, 1.51476, 1, -0.0057, -0,
		-0.728066, 0.062759, 0.806152, 1, -0.0057, -0,
		-0.728066, 0.062759, 1.51476, -0.6077, -0.7942, -0,
		-0.961451, 0.24133, 0.806152, -0.6077, -0.7942, -0,
		-0.728066, 0.062759, 0.806152, -0.6077, -0.7942, -0,
		-0.728066, 0.062759, 1.51476, -0.6077, -0.7942, -0,
		-0.961451, 0.24133, 1.51476, -0.6077, -0.7942, -0,
		-0.961451, 0.24133, 0.806152, -0.6077, -0.7942, -0,
	};
	float * tank3 = new float[36 * 6]
	{
		0.3121, 0.563065, 0.882477, -0, -0, -1,
		-0.44809, 0.393064, 0.882477, -0, -0, -1,
		-0.44809, 0.563065, 0.882477, -0, -0, -1,
		0.3121, 0.563065, 0.882477, -0, -0, -1,
		0.3121, 0.393064, 0.882477, -0, -0, -1,
		-0.44809, 0.393064, 0.882477, -0, -0, -1,
		-0.44809, 0.563065, 1.426, -0, -0, 1,
		0.3121, 0.393064, 1.426, -0, -0, 1,
		0.3121, 0.563065, 1.426, -0, -0, 1,
		-0.44809, 0.563065, 1.426, -0, -0, 1,
		-0.44809, 0.393064, 1.426, -0, -0, 1,
		0.3121, 0.393064, 1.426, -0, -0, 1,
		0.3121, 0.563065, 1.426, 1, -0, -0,
		0.3121, 0.393064, 0.882477, 1, -0, -0,
		0.3121, 0.563065, 0.882477, 1, -0, -0,
		0.3121, 0.563065, 1.426, 1, -0, -0,
		0.3121, 0.393064, 1.426, 1, -0, -0,
		0.3121, 0.393064, 0.882477, 1, -0, -0,
		-0.44809, 0.563065, 0.882477, -1, -0, -0,
		-0.44809, 0.393064, 1.426, -1, -0, -0,
		-0.44809, 0.563065, 1.426, -1, -0, -0,
		-0.44809, 0.563065, 0.882477, -1, -0, -0,
		-0.44809, 0.393064, 0.882477, -1, -0, -0,
		-0.44809, 0.393064, 1.426, -1, -0, -0,
		-0.44809, 0.563065, 0.882477, -0, 1, -0,
		0.3121, 0.563065, 1.426, -0, 1, -0,
		0.3121, 0.563065, 0.882477, -0, 1, -0,
		-0.44809, 0.563065, 0.882477, -0, 1, -0,
		-0.44809, 0.563065, 1.426, -0, 1, -0,
		0.3121, 0.563065, 1.426, -0, 1, -0,
		-0.44809, 0.393064, 1.426, -0, -1, -0,
		0.3121, 0.393064, 0.882477, -0, -1, -0,
		0.3121, 0.393064, 1.426, -0, -1, -0,
		-0.44809, 0.393064, 1.426, -0, -1, -0,
		-0.44809, 0.393064, 0.882477, -0, -1, -0,
		0.3121, 0.393064, 0.882477, -0, -1, -0,
	};
	float * tank4 = new float[24 * 6]
	{
		-0.447534, 0.563273, 1.42617, -0, -0, 1,
		-0.674076, 0.392822, 1.42617, -0, -0, 1,
		-0.446634, 0.393378, 1.42617, -0, -0, 1,
		-0.446634, 0.393378, 0.880718, -0, -0, -1,
		-0.674076, 0.392822, 0.880718, -0, -0, -1,
		-0.447534, 0.563273, 0.880718, -0, -0, -1,
		-0.447534, 0.563273, 1.42617, 1, 0.0053, -0,
		-0.446634, 0.393378, 0.880718, 1, 0.0053, -0,
		-0.447534, 0.563273, 0.880718, 1, 0.0053, -0,
		-0.447534, 0.563273, 1.42617, 1, 0.0053, -0,
		-0.446634, 0.393378, 1.42617, 1, 0.0053, -0,
		-0.446634, 0.393378, 0.880718, 1, 0.0053, -0,
		-0.446634, 0.393378, 1.42617, 0.0024, -1, -0,
		-0.674076, 0.392822, 0.880718, 0.0024, -1, -0,
		-0.446634, 0.393378, 0.880718, 0.0024, -1, -0,
		-0.446634, 0.393378, 1.42617, 0.0024, -1, -0,
		-0.674076, 0.392822, 1.42617, 0.0024, -1, -0,
		-0.674076, 0.392822, 0.880718, 0.0024, -1, -0,
		-0.674076, 0.392822, 1.42617, -0.6012, 0.7991, -0,
		-0.447534, 0.563273, 0.880718, -0.6012, 0.7991, -0,
		-0.674076, 0.392822, 0.880718, -0.6012, 0.7991, -0,
		-0.674076, 0.392822, 1.42617, -0.6012, 0.7991, -0,
		-0.447534, 0.563273, 1.42617, -0.6012, 0.7991, -0,
		-0.447534, 0.563273, 0.880718, -0.6012, 0.7991, -0,
	};
	float * tank5 = new float[1920 * 6]
	{
		-1.16679, 0.662624, 1.18547, -0.9628, 0.2702, -0,
		-1.16698, 0.661927, 1.17627, -0.9628, 0.2702, -0,
		-1.16757, 0.659834, 1.17635, -0.9628, 0.2702, -0,
		-1.16757, 0.659834, 1.18559, -0.9628, 0.2702, -0,
		-1.16679, 0.662624, 1.18547, -0.9628, 0.2702, -0,
		-1.16757, 0.659834, 1.17635, -0.9628, 0.2702, -0,
		-0.502035, 0.473038, 1.17635, 0.9628, -0.2702, -0,
		-0.501448, 0.47513, 1.17627, 0.9628, -0.2702, -0,
		-0.501252, 0.475828, 1.18547, 0.9628, -0.2702, -0,
		-0.502035, 0.473038, 1.17635, 0.9628, -0.2702, -0,
		-0.501252, 0.475828, 1.18547, 0.9628, -0.2702, -0,
		-0.502035, 0.473038, 1.18559, 0.9628, -0.2702, -0,
		-1.16757, 0.659834, 1.18559, -0, -0, 1,
		-0.501252, 0.475828, 1.18547, 0.0212, 0.0755, 0.9969,
		-1.16679, 0.662624, 1.18547, 0.0212, 0.0755, 0.9969,
		-0.501252, 0.475828, 1.18547, 0.0212, 0.0755, 0.9969,
		-1.16757, 0.659834, 1.18559, -0, -0, 1,
		-0.502035, 0.473038, 1.18559, -0, -0, 1,
		-1.16698, 0.661927, 1.17627, -0.0212, -0.0755, -0.9969,
		-0.501448, 0.47513, 1.17627, -0.0212, -0.0755, -0.9969,
		-1.16757, 0.659834, 1.17635, -0, -0, -1,
		-0.502035, 0.473038, 1.17635, -0, -0, -1,
		-1.16757, 0.659834, 1.17635, -0, -0, -1,
		-0.501448, 0.47513, 1.17627, -0.0212, -0.0755, -0.9969,
		-1.16601, 0.665397, 1.18513, -0.9628, 0.2702, -0,
		-1.1664, 0.664006, 1.17601, -0.9628, 0.2702, -0,
		-1.16698, 0.661927, 1.17627, -0.9628, 0.2702, -0,
		-1.16679, 0.662624, 1.18547, -0.9628, 0.2702, -0,
		-1.16601, 0.665397, 1.18513, -0.9628, 0.2702, -0,
		-1.16698, 0.661927, 1.17627, -0.9628, 0.2702, -0,
		-0.501448, 0.47513, 1.17627, 0.9628, -0.2702, -0,
		-0.500864, 0.47721, 1.17601, 0.9628, -0.2702, -0,
		-0.500474, 0.478601, 1.18513, 0.9628, -0.2702, -0,
		-0.501448, 0.47513, 1.17627, 0.9628, -0.2702, -0,
		-0.500474, 0.478601, 1.18513, 0.9628, -0.2702, -0,
		-0.501252, 0.475828, 1.18547, 0.9628, -0.2702, -0,
		-1.16679, 0.662624, 1.18547, 0.0212, 0.0755, 0.9969,
		-0.500474, 0.478601, 1.18513, 0.0423, 0.1506, 0.9877,
		-1.16601, 0.665397, 1.18513, 0.0423, 0.1506, 0.9877,
		-0.500474, 0.478601, 1.18513, 0.0423, 0.1506, 0.9877,
		-1.16679, 0.662624, 1.18547, 0.0212, 0.0755, 0.9969,
		-0.501252, 0.475828, 1.18547, 0.0212, 0.0755, 0.9969,
		-1.1664, 0.664006, 1.17601, -0.0423, -0.1506, -0.9877,
		-0.500864, 0.47721, 1.17601, -0.0423, -0.1506, -0.9877,
		-1.16698, 0.661927, 1.17627, -0.0212, -0.0755, -0.9969,
		-0.501448, 0.47513, 1.17627, -0.0212, -0.0755, -0.9969,
		-1.16698, 0.661927, 1.17627, -0.0212, -0.0755, -0.9969,
		-0.500864, 0.47721, 1.17601, -0.0423, -0.1506, -0.9877,
		-1.16524, 0.668136, 1.18457, -0.9628, 0.2702, -0,
		-1.16582, 0.66606, 1.17559, -0.9628, 0.2702, -0,
		-1.1664, 0.664006, 1.17601, -0.9628, 0.2702, -0,
		-1.16601, 0.665397, 1.18513, -0.9628, 0.2702, -0,
		-1.16524, 0.668136, 1.18457, -0.9628, 0.2702, -0,
		-1.1664, 0.664006, 1.17601, -0.9628, 0.2702, -0,
		-0.500864, 0.47721, 1.17601, 0.9628, -0.2702, -0,
		-0.500288, 0.479264, 1.17559, 0.9628, -0.2702, -0,
		-0.499705, 0.481339, 1.18457, 0.9628, -0.2702, -0,
		-0.500864, 0.47721, 1.17601, 0.9628, -0.2702, -0,
		-0.499705, 0.481339, 1.18457, 0.9628, -0.2702, -0,
		-0.500474, 0.478601, 1.18513, 0.9628, -0.2702, -0,
		-1.16601, 0.665397, 1.18513, 0.0423, 0.1506, 0.9877,
		-0.499705, 0.481339, 1.18457, 0.0631, 0.2248, 0.9724,
		-1.16524, 0.668136, 1.18457, 0.0631, 0.2248, 0.9724,
		-0.499705, 0.481339, 1.18457, 0.0631, 0.2248, 0.9724,
		-1.16601, 0.665397, 1.18513, 0.0423, 0.1506, 0.9877,
		-0.500474, 0.478601, 1.18513, 0.0423, 0.1506, 0.9877,
		-1.16582, 0.66606, 1.17559, -0.0631, -0.2248, -0.9724,
		-0.500288, 0.479264, 1.17559, -0.0631, -0.2248, -0.9724,
		-1.1664, 0.664006, 1.17601, -0.0423, -0.1506, -0.9877,
		-0.500864, 0.47721, 1.17601, -0.0423, -0.1506, -0.9877,
		-1.1664, 0.664006, 1.17601, -0.0423, -0.1506, -0.9877,
		-0.500288, 0.479264, 1.17559, -0.0631, -0.2248, -0.9724,
		-1.16449, 0.670823, 1.18378, -0.9628, 0.2702, -0,
		-1.16526, 0.668076, 1.175, -0.9628, 0.2702, -0,
		-1.16582, 0.66606, 1.17559, -0.9628, 0.2702, -0,
		-1.16524, 0.668136, 1.18457, -0.9628, 0.2702, -0,
		-1.16449, 0.670823, 1.18378, -0.9628, 0.2702, -0,
		-1.16582, 0.66606, 1.17559, -0.9628, 0.2702, -0,
		-0.500288, 0.479264, 1.17559, 0.9628, -0.2702, -0,
		-0.499722, 0.481279, 1.175, 0.9628, -0.2702, -0,
		-0.498951, 0.484027, 1.18378, 0.9628, -0.2702, -0,
		-0.500288, 0.479264, 1.17559, 0.9628, -0.2702, -0,
		-0.498951, 0.484027, 1.18378, 0.9628, -0.2702, -0,
		-0.499705, 0.481339, 1.18457, 0.9628, -0.2702, -0,
		-1.16524, 0.668136, 1.18457, 0.0631, 0.2248, 0.9724,
		-0.498951, 0.484027, 1.18378, 0.0835, 0.2975, 0.9511,
		-1.16449, 0.670823, 1.18378, 0.0835, 0.2975, 0.9511,
		-0.498951, 0.484027, 1.18378, 0.0835, 0.2975, 0.9511,
		-1.16524, 0.668136, 1.18457, 0.0631, 0.2248, 0.9724,
		-0.499705, 0.481339, 1.18457, 0.0631, 0.2248, 0.9724,
		-1.16526, 0.668076, 1.175, -0.0835, -0.2975, -0.9511,
		-0.499722, 0.481279, 1.175, -0.0835, -0.2975, -0.9511,
		-1.16582, 0.66606, 1.17559, -0.0631, -0.2248, -0.9724,
		-0.500288, 0.479264, 1.17559, -0.0631, -0.2248, -0.9724,
		-1.16582, 0.66606, 1.17559, -0.0631, -0.2248, -0.9724,
		-0.499722, 0.481279, 1.175, -0.0835, -0.2975, -0.9511,
		-1.16375, 0.673443, 1.18278, -0.9628, 0.2702, -0,
		-1.16471, 0.670041, 1.17425, -0.9628, 0.2702, -0,
		-1.16526, 0.668076, 1.175, -0.9628, 0.2702, -0,
		-1.16449, 0.670823, 1.18378, -0.9628, 0.2702, -0,
		-1.16375, 0.673443, 1.18278, -0.9628, 0.2702, -0,
		-1.16526, 0.668076, 1.175, -0.9628, 0.2702, -0,
		-0.499722, 0.481279, 1.175, 0.9628, -0.2702, -0,
		-0.499171, 0.483244, 1.17425, 0.9628, -0.2702, -0,
		-0.498216, 0.486646, 1.18278, 0.9628, -0.2702, -0,
		-0.499722, 0.481279, 1.175, 0.9628, -0.2702, -0,
		-0.498216, 0.486646, 1.18278, 0.9628, -0.2702, -0,
		-0.498951, 0.484027, 1.18378, 0.9628, -0.2702, -0,
		-1.16449, 0.670823, 1.18378, 0.0835, 0.2975, 0.9511,
		-0.498216, 0.486646, 1.18278, 0.1034, 0.3684, 0.9239,
		-1.16375, 0.673443, 1.18278, 0.1034, 0.3684, 0.9239,
		-0.498216, 0.486646, 1.18278, 0.1034, 0.3684, 0.9239,
		-1.16449, 0.670823, 1.18378, 0.0835, 0.2975, 0.9511,
		-0.498951, 0.484027, 1.18378, 0.0835, 0.2975, 0.9511,
		-1.16471, 0.670041, 1.17425, -0.1034, -0.3684, -0.9239,
		-0.499171, 0.483244, 1.17425, -0.1034, -0.3684, -0.9239,
		-1.16526, 0.668076, 1.175, -0.0835, -0.2975, -0.9511,
		-0.499722, 0.481279, 1.175, -0.0835, -0.2975, -0.9511,
		-1.16526, 0.668076, 1.175, -0.0835, -0.2975, -0.9511,
		-0.499171, 0.483244, 1.17425, -0.1034, -0.3684, -0.9239,
		-1.16304, 0.675978, 1.18156, -0.9628, 0.2702, -0,
		-1.16417, 0.671942, 1.17333, -0.9628, 0.2702, -0,
		-1.16471, 0.670041, 1.17425, -0.9628, 0.2702, -0,
		-1.16375, 0.673443, 1.18278, -0.9628, 0.2702, -0,
		-1.16304, 0.675978, 1.18156, -0.9628, 0.2702, -0,
		-1.16471, 0.670041, 1.17425, -0.9628, 0.2702, -0,
		-0.499171, 0.483244, 1.17425, 0.9628, -0.2702, -0,
		-0.498637, 0.485146, 1.17333, 0.9628, -0.2702, -0,
		-0.497504, 0.489182, 1.18156, 0.9628, -0.2702, -0,
		-0.499171, 0.483244, 1.17425, 0.9628, -0.2702, -0,
		-0.497504, 0.489182, 1.18156, 0.9628, -0.2702, -0,
		-0.498216, 0.486646, 1.18278, 0.9628, -0.2702, -0,
		-1.16375, 0.673443, 1.18278, 0.1034, 0.3684, 0.9239,
		-0.497504, 0.489182, 1.18156, 0.1227, 0.4371, 0.891,
		-1.16304, 0.675978, 1.18156, 0.1227, 0.4371, 0.891,
		-0.497504, 0.489182, 1.18156, 0.1227, 0.4371, 0.891,
		-1.16375, 0.673443, 1.18278, 0.1034, 0.3684, 0.9239,
		-0.498216, 0.486646, 1.18278, 0.1034, 0.3684, 0.9239,
		-1.16417, 0.671942, 1.17333, -0.1227, -0.4371, -0.891,
		-0.498637, 0.485146, 1.17333, -0.1227, -0.4371, -0.891,
		-1.16471, 0.670041, 1.17425, -0.1034, -0.3684, -0.9239,
		-0.499171, 0.483244, 1.17425, -0.1034, -0.3684, -0.9239,
		-1.16471, 0.670041, 1.17425, -0.1034, -0.3684, -0.9239,
		-0.498637, 0.485146, 1.17333, -0.1227, -0.4371, -0.891,
		-1.16235, 0.678415, 1.18015, -0.9628, 0.2702, -0,
		-1.16366, 0.67377, 1.17227, -0.9628, 0.2702, -0,
		-1.16417, 0.671942, 1.17333, -0.9628, 0.2702, -0,
		-1.16304, 0.675978, 1.18156, -0.9628, 0.2702, -0,
		-1.16235, 0.678415, 1.18015, -0.9628, 0.2702, -0,
		-1.16417, 0.671942, 1.17333, -0.9628, 0.2702, -0,
		-0.498637, 0.485146, 1.17333, 0.9628, -0.2702, -0,
		-0.498124, 0.486973, 1.17227, 0.9628, -0.2702, -0,
		-0.49682, 0.491618, 1.18015, 0.9628, -0.2702, -0,
		-0.498637, 0.485146, 1.17333, 0.9628, -0.2702, -0,
		-0.49682, 0.491618, 1.18015, 0.9628, -0.2702, -0,
		-0.497504, 0.489182, 1.18156, 0.9628, -0.2702, -0,
		-1.16304, 0.675978, 1.18156, 0.1227, 0.4371, 0.891,
		-0.49682, 0.491618, 1.18015, 0.1412, 0.5031, 0.8526,
		-1.16235, 0.678415, 1.18015, 0.1412, 0.5031, 0.8526,
		-0.49682, 0.491618, 1.18015, 0.1412, 0.5031, 0.8526,
		-1.16304, 0.675978, 1.18156, 0.1227, 0.4371, 0.891,
		-0.497504, 0.489182, 1.18156, 0.1227, 0.4371, 0.891,
		-1.16366, 0.67377, 1.17227, -0.1412, -0.5031, -0.8526,
		-0.498124, 0.486973, 1.17227, -0.1412, -0.5031, -0.8526,
		-1.16417, 0.671942, 1.17333, -0.1227, -0.4371, -0.891,
		-0.498637, 0.485146, 1.17333, -0.1227, -0.4371, -0.891,
		-1.16417, 0.671942, 1.17333, -0.1227, -0.4371, -0.891,
		-0.498124, 0.486973, 1.17227, -0.1412, -0.5031, -0.8526,
		-1.1617, 0.680736, 1.17853, -0.9628, 0.2702, -0,
		-1.16317, 0.675511, 1.17106, -0.9628, 0.2702, -0,
		-1.16366, 0.67377, 1.17227, -0.9628, 0.2702, -0,
		-1.16235, 0.678415, 1.18015, -0.9628, 0.2702, -0,
		-1.1617, 0.680736, 1.17853, -0.9628, 0.2702, -0,
		-1.16366, 0.67377, 1.17227, -0.9628, 0.2702, -0,
		-0.498124, 0.486973, 1.17227, 0.9628, -0.2702, -0,
		-0.497635, 0.488714, 1.17106, 0.9628, -0.2702, -0,
		-0.496169, 0.49394, 1.17853, 0.9628, -0.2702, -0,
		-0.498124, 0.486973, 1.17227, 0.9628, -0.2702, -0,
		-0.496169, 0.49394, 1.17853, 0.9628, -0.2702, -0,
		-0.49682, 0.491618, 1.18015, 0.9628, -0.2702, -0,
		-1.16235, 0.678415, 1.18015, 0.1412, 0.5031, 0.8526,
		-0.496169, 0.49394, 1.17853, 0.1588, 0.5659, 0.809,
		-1.1617, 0.680736, 1.17853, 0.1588, 0.5659, 0.809,
		-0.496169, 0.49394, 1.17853, 0.1588, 0.5659, 0.809,
		-1.16235, 0.678415, 1.18015, 0.1412, 0.5031, 0.8526,
		-0.49682, 0.491618, 1.18015, 0.1412, 0.5031, 0.8526,
		-1.16317, 0.675511, 1.17106, -0.1588, -0.5659, -0.809,
		-0.497635, 0.488714, 1.17106, -0.1588, -0.5659, -0.809,
		-1.16366, 0.67377, 1.17227, -0.1412, -0.5031, -0.8526,
		-0.498124, 0.486973, 1.17227, -0.1412, -0.5031, -0.8526,
		-1.16366, 0.67377, 1.17227, -0.1412, -0.5031, -0.8526,
		-0.497635, 0.488714, 1.17106, -0.1588, -0.5659, -0.809,
		-1.16109, 0.682929, 1.17674, -0.9628, 0.2702, -0,
		-1.16271, 0.677155, 1.16972, -0.9628, 0.2702, -0,
		-1.16317, 0.675511, 1.17106, -0.9628, 0.2702, -0,
		-1.1617, 0.680736, 1.17853, -0.9628, 0.2702, -0,
		-1.16109, 0.682929, 1.17674, -0.9628, 0.2702, -0,
		-1.16317, 0.675511, 1.17106, -0.9628, 0.2702, -0,
		-0.497635, 0.488714, 1.17106, 0.9628, -0.2702, -0,
		-0.497174, 0.490359, 1.16972, 0.9628, -0.2702, -0,
		-0.495553, 0.496133, 1.17674, 0.9628, -0.2702, -0,
		-0.497635, 0.488714, 1.17106, 0.9628, -0.2702, -0,
		-0.495553, 0.496133, 1.17674, 0.9628, -0.2702, -0,
		-0.496169, 0.49394, 1.17853, 0.9628, -0.2702, -0,
		-1.1617, 0.680736, 1.17853, 0.1588, 0.5659, 0.809,
		-0.495553, 0.496133, 1.17674, 0.1755, 0.6253, 0.7604,
		-1.16109, 0.682929, 1.17674, 0.1755, 0.6253, 0.7604,
		-0.495553, 0.496133, 1.17674, 0.1755, 0.6253, 0.7604,
		-1.1617, 0.680736, 1.17853, 0.1588, 0.5659, 0.809,
		-0.496169, 0.49394, 1.17853, 0.1588, 0.5659, 0.809,
		-1.16271, 0.677155, 1.16972, -0.1755, -0.6253, -0.7604,
		-0.497174, 0.490359, 1.16972, -0.1755, -0.6253, -0.7604,
		-1.16317, 0.675511, 1.17106, -0.1588, -0.5659, -0.809,
		-0.497635, 0.488714, 1.17106, -0.1588, -0.5659, -0.809,
		-1.16317, 0.675511, 1.17106, -0.1588, -0.5659, -0.809,
		-0.497174, 0.490359, 1.16972, -0.1755, -0.6253, -0.7604,
		-1.16051, 0.684979, 1.17477, -0.9628, 0.2702, -0,
		-1.16228, 0.678693, 1.16824, -0.9628, 0.2702, -0,
		-1.16271, 0.677155, 1.16972, -0.9628, 0.2702, -0,
		-1.16109, 0.682929, 1.17674, -0.9628, 0.2702, -0,
		-1.16051, 0.684979, 1.17477, -0.9628, 0.2702, -0,
		-1.16271, 0.677155, 1.16972, -0.9628, 0.2702, -0,
		-0.497174, 0.490359, 1.16972, 0.9628, -0.2702, -0,
		-0.496742, 0.491897, 1.16824, 0.9628, -0.2702, -0,
		-0.494978, 0.498183, 1.17477, 0.9628, -0.2702, -0,
		-0.497174, 0.490359, 1.16972, 0.9628, -0.2702, -0,
		-0.494978, 0.498183, 1.17477, 0.9628, -0.2702, -0,
		-0.495553, 0.496133, 1.17674, 0.9628, -0.2702, -0,
		-1.16109, 0.682929, 1.17674, 0.1755, 0.6253, 0.7604,
		-0.494978, 0.498183, 1.17477, 0.1911, 0.6808, 0.7071,
		-1.16051, 0.684979, 1.17477, 0.1911, 0.6808, 0.7071,
		-0.494978, 0.498183, 1.17477, 0.1911, 0.6808, 0.7071,
		-1.16109, 0.682929, 1.17674, 0.1755, 0.6253, 0.7604,
		-0.495553, 0.496133, 1.17674, 0.1755, 0.6253, 0.7604,
		-1.16228, 0.678693, 1.16824, -0.1911, -0.6808, -0.7071,
		-0.496742, 0.491897, 1.16824, -0.1911, -0.6808, -0.7071,
		-1.16271, 0.677155, 1.16972, -0.1755, -0.6253, -0.7604,
		-0.497174, 0.490359, 1.16972, -0.1755, -0.6253, -0.7604,
		-1.16271, 0.677155, 1.16972, -0.1755, -0.6253, -0.7604,
		-0.496742, 0.491897, 1.16824, -0.1911, -0.6808, -0.7071,
		-1.15998, 0.686875, 1.17264, -0.9628, 0.2702, -0,
		-1.16188, 0.680115, 1.16664, -0.9628, 0.2702, -0,
		-1.16228, 0.678693, 1.16824, -0.9628, 0.2702, -0,
		-1.16051, 0.684979, 1.17477, -0.9628, 0.2702, -0,
		-1.15998, 0.686875, 1.17264, -0.9628, 0.2702, -0,
		-1.16228, 0.678693, 1.16824, -0.9628, 0.2702, -0,
		-0.496742, 0.491897, 1.16824, 0.9628, -0.2702, -0,
		-0.496343, 0.493318, 1.16664, 0.9628, -0.2702, -0,
		-0.494446, 0.500078, 1.17264, 0.9628, -0.2702, -0,
		-0.496742, 0.491897, 1.16824, 0.9628, -0.2702, -0,
		-0.494446, 0.500078, 1.17264, 0.9628, -0.2702, -0,
		-0.494978, 0.498183, 1.17477, 0.9628, -0.2702, -0,
		-1.16051, 0.684979, 1.17477, 0.1911, 0.6808, 0.7071,
		-0.494446, 0.500078, 1.17264, 0.2055, 0.7321, 0.6494,
		-1.15998, 0.686875, 1.17264, 0.2055, 0.7321, 0.6494,
		-0.494446, 0.500078, 1.17264, 0.2055, 0.7321, 0.6494,
		-1.16051, 0.684979, 1.17477, 0.1911, 0.6808, 0.7071,
		-0.494978, 0.498183, 1.17477, 0.1911, 0.6808, 0.7071,
		-1.16188, 0.680115, 1.16664, -0.2055, -0.7321, -0.6494,
		-0.496343, 0.493318, 1.16664, -0.2055, -0.7321, -0.6494,
		-1.16228, 0.678693, 1.16824, -0.1911, -0.6808, -0.7071,
		-0.496742, 0.491897, 1.16824, -0.1911, -0.6808, -0.7071,
		-1.16228, 0.678693, 1.16824, -0.1911, -0.6808, -0.7071,
		-0.496343, 0.493318, 1.16664, -0.2055, -0.7321, -0.6494,
		-1.15949, 0.688603, 1.17036, -0.9628, 0.2702, -0,
		-1.16151, 0.681411, 1.16494, -0.9628, 0.2702, -0,
		-1.16188, 0.680115, 1.16664, -0.9628, 0.2702, -0,
		-1.15998, 0.686875, 1.17264, -0.9628, 0.2702, -0,
		-1.15949, 0.688603, 1.17036, -0.9628, 0.2702, -0,
		-1.16188, 0.680115, 1.16664, -0.9628, 0.2702, -0,
		-0.496343, 0.493318, 1.16664, 0.9628, -0.2702, -0,
		-0.495979, 0.494615, 1.16494, 0.9628, -0.2702, -0,
		-0.493961, 0.501807, 1.17036, 0.9628, -0.2702, -0,
		-0.496343, 0.493318, 1.16664, 0.9628, -0.2702, -0,
		-0.493961, 0.501807, 1.17036, 0.9628, -0.2702, -0,
		-0.494446, 0.500078, 1.17264, 0.9628, -0.2702, -0,
		-1.15998, 0.686875, 1.17264, 0.2055, 0.7321, 0.6494,
		-0.493961, 0.501807, 1.17036, 0.2186, 0.7789, 0.5878,
		-1.15949, 0.688603, 1.17036, 0.2186, 0.7789, 0.5878,
		-0.493961, 0.501807, 1.17036, 0.2186, 0.7789, 0.5878,
		-1.15998, 0.686875, 1.17264, 0.2055, 0.7321, 0.6494,
		-0.494446, 0.500078, 1.17264, 0.2055, 0.7321, 0.6494,
		-1.16151, 0.681411, 1.16494, -0.2186, -0.7789, -0.5878,
		-0.495979, 0.494615, 1.16494, -0.2186, -0.7789, -0.5878,
		-1.16188, 0.680115, 1.16664, -0.2055, -0.7321, -0.6494,
		-0.496343, 0.493318, 1.16664, -0.2055, -0.7321, -0.6494,
		-1.16188, 0.680115, 1.16664, -0.2055, -0.7321, -0.6494,
		-0.495979, 0.494615, 1.16494, -0.2186, -0.7789, -0.5878,
		-1.15906, 0.690155, 1.16795, -0.9628, 0.2702, -0,
		-1.16119, 0.682575, 1.16313, -0.9628, 0.2702, -0,
		-1.16151, 0.681411, 1.16494, -0.9628, 0.2702, -0,
		-1.15949, 0.688603, 1.17036, -0.9628, 0.2702, -0,
		-1.15906, 0.690155, 1.16795, -0.9628, 0.2702, -0,
		-1.16151, 0.681411, 1.16494, -0.9628, 0.2702, -0,
		-0.495979, 0.494615, 1.16494, 0.9628, -0.2702, -0,
		-0.495653, 0.495778, 1.16313, 0.9628, -0.2702, -0,
		-0.493525, 0.503358, 1.16795, 0.9628, -0.2702, -0,
		-0.495979, 0.494615, 1.16494, 0.9628, -0.2702, -0,
		-0.493525, 0.503358, 1.16795, 0.9628, -0.2702, -0,
		-0.493961, 0.501807, 1.17036, 0.9628, -0.2702, -0,
		-1.15949, 0.688603, 1.17036, 0.2186, 0.7789, 0.5878,
		-0.493525, 0.503358, 1.16795, 0.2304, 0.8209, 0.5225,
		-1.15906, 0.690155, 1.16795, 0.2304, 0.8209, 0.5225,
		-0.493525, 0.503358, 1.16795, 0.2304, 0.8209, 0.5225,
		-1.15949, 0.688603, 1.17036, 0.2186, 0.7789, 0.5878,
		-0.493961, 0.501807, 1.17036, 0.2186, 0.7789, 0.5878,
		-1.16119, 0.682575, 1.16313, -0.2304, -0.8209, -0.5225,
		-0.495653, 0.495778, 1.16313, -0.2304, -0.8209, -0.5225,
		-1.16151, 0.681411, 1.16494, -0.2186, -0.7789, -0.5878,
		-0.495979, 0.494615, 1.16494, -0.2186, -0.7789, -0.5878,
		-1.16151, 0.681411, 1.16494, -0.2186, -0.7789, -0.5878,
		-0.495653, 0.495778, 1.16313, -0.2304, -0.8209, -0.5225,
		-1.15868, 0.691519, 1.16542, -0.9628, 0.2702, -0,
		-1.1609, 0.683598, 1.16123, -0.9628, 0.2702, -0,
		-1.16119, 0.682575, 1.16313, -0.9628, 0.2702, -0,
		-1.15906, 0.690155, 1.16795, -0.9628, 0.2702, -0,
		-1.15868, 0.691519, 1.16542, -0.9628, 0.2702, -0,
		-1.16119, 0.682575, 1.16313, -0.9628, 0.2702, -0,
		-0.495653, 0.495778, 1.16313, 0.9628, -0.2702, -0,
		-0.495366, 0.496801, 1.16123, 0.9628, -0.2702, -0,
		-0.493142, 0.504723, 1.16542, 0.9628, -0.2702, -0,
		-0.495653, 0.495778, 1.16313, 0.9628, -0.2702, -0,
		-0.493142, 0.504723, 1.16542, 0.9628, -0.2702, -0,
		-0.493525, 0.503358, 1.16795, 0.9628, -0.2702, -0,
		-1.15906, 0.690155, 1.16795, 0.2304, 0.8209, 0.5225,
		-0.493142, 0.504723, 1.16542, 0.2408, 0.8579, 0.454,
		-1.15868, 0.691519, 1.16542, 0.2408, 0.8579, 0.454,
		-0.493142, 0.504723, 1.16542, 0.2408, 0.8579, 0.454,
		-1.15906, 0.690155, 1.16795, 0.2304, 0.8209, 0.5225,
		-0.493525, 0.503358, 1.16795, 0.2304, 0.8209, 0.5225,
		-1.1609, 0.683598, 1.16123, -0.2408, -0.8579, -0.454,
		-0.495366, 0.496801, 1.16123, -0.2408, -0.8579, -0.454,
		-1.16119, 0.682575, 1.16313, -0.2304, -0.8209, -0.5225,
		-0.495653, 0.495778, 1.16313, -0.2304, -0.8209, -0.5225,
		-1.16119, 0.682575, 1.16313, -0.2304, -0.8209, -0.5225,
		-0.495366, 0.496801, 1.16123, -0.2408, -0.8579, -0.454,
		-1.15835, 0.692688, 1.16279, -0.9628, 0.2702, -0,
		-1.16065, 0.684475, 1.15925, -0.9628, 0.2702, -0,
		-1.1609, 0.683598, 1.16123, -0.9628, 0.2702, -0,
		-1.15868, 0.691519, 1.16542, -0.9628, 0.2702, -0,
		-1.15835, 0.692688, 1.16279, -0.9628, 0.2702, -0,
		-1.1609, 0.683598, 1.16123, -0.9628, 0.2702, -0,
		-0.495366, 0.496801, 1.16123, 0.9628, -0.2702, -0,
		-0.49512, 0.497678, 1.15925, 0.9628, -0.2702, -0,
		-0.492814, 0.505892, 1.16279, 0.9628, -0.2702, -0,
		-0.495366, 0.496801, 1.16123, 0.9628, -0.2702, -0,
		-0.492814, 0.505892, 1.16279, 0.9628, -0.2702, -0,
		-0.493142, 0.504723, 1.16542, 0.9628, -0.2702, -0,
		-1.15868, 0.691519, 1.16542, 0.2408, 0.8579, 0.454,
		-0.492814, 0.505892, 1.16279, 0.2497, 0.8895, 0.3827,
		-1.15835, 0.692688, 1.16279, 0.2497, 0.8895, 0.3827,
		-0.492814, 0.505892, 1.16279, 0.2497, 0.8895, 0.3827,
		-1.15868, 0.691519, 1.16542, 0.2408, 0.8579, 0.454,
		-0.493142, 0.504723, 1.16542, 0.2408, 0.8579, 0.454,
		-1.16065, 0.684475, 1.15925, -0.2497, -0.8895, -0.3827,
		-0.49512, 0.497678, 1.15925, -0.2497, -0.8895, -0.3827,
		-1.1609, 0.683598, 1.16123, -0.2408, -0.8579, -0.454,
		-0.495366, 0.496801, 1.16123, -0.2408, -0.8579, -0.454,
		-1.1609, 0.683598, 1.16123, -0.2408, -0.8579, -0.454,
		-0.49512, 0.497678, 1.15925, -0.2497, -0.8895, -0.3827,
		-1.15808, 0.693654, 1.16007, -0.9628, 0.2702, -0,
		-1.16045, 0.685199, 1.15721, -0.9628, 0.2702, -0,
		-1.16065, 0.684475, 1.15925, -0.9628, 0.2702, -0,
		-1.15835, 0.692688, 1.16279, -0.9628, 0.2702, -0,
		-1.15808, 0.693654, 1.16007, -0.9628, 0.2702, -0,
		-1.16065, 0.684475, 1.15925, -0.9628, 0.2702, -0,
		-0.49512, 0.497678, 1.15925, 0.9628, -0.2702, -0,
		-0.494916, 0.498403, 1.15721, 0.9628, -0.2702, -0,
		-0.492543, 0.506858, 1.16007, 0.9628, -0.2702, -0,
		-0.49512, 0.497678, 1.15925, 0.9628, -0.2702, -0,
		-0.492543, 0.506858, 1.16007, 0.9628, -0.2702, -0,
		-0.492814, 0.505892, 1.16279, 0.9628, -0.2702, -0,
		-1.15835, 0.692688, 1.16279, 0.2497, 0.8895, 0.3827,
		-0.492543, 0.506858, 1.16007, 0.257, 0.9157, 0.309,
		-1.15808, 0.693654, 1.16007, 0.257, 0.9157, 0.309,
		-0.492543, 0.506858, 1.16007, 0.257, 0.9157, 0.309,
		-1.15835, 0.692688, 1.16279, 0.2497, 0.8895, 0.3827,
		-0.492814, 0.505892, 1.16279, 0.2497, 0.8895, 0.3827,
		-1.16045, 0.685199, 1.15721, -0.257, -0.9157, -0.309,
		-0.494916, 0.498403, 1.15721, -0.257, -0.9157, -0.309,
		-1.16065, 0.684475, 1.15925, -0.2497, -0.8895, -0.3827,
		-0.49512, 0.497678, 1.15925, -0.2497, -0.8895, -0.3827,
		-1.16065, 0.684475, 1.15925, -0.2497, -0.8895, -0.3827,
		-0.494916, 0.498403, 1.15721, -0.257, -0.9157, -0.309,
		-1.15787, 0.694412, 1.15728, -0.9628, 0.2702, -0,
		-1.16029, 0.685768, 1.15512, -0.9628, 0.2702, -0,
		-1.16045, 0.685199, 1.15721, -0.9628, 0.2702, -0,
		-1.15808, 0.693654, 1.16007, -0.9628, 0.2702, -0,
		-1.15787, 0.694412, 1.15728, -0.9628, 0.2702, -0,
		-1.16045, 0.685199, 1.15721, -0.9628, 0.2702, -0,
		-0.494916, 0.498403, 1.15721, 0.9628, -0.2702, -0,
		-0.494757, 0.498971, 1.15512, 0.9628, -0.2702, -0,
		-0.49233, 0.507616, 1.15728, 0.9628, -0.2702, -0,
		-0.494916, 0.498403, 1.15721, 0.9628, -0.2702, -0,
		-0.49233, 0.507616, 1.15728, 0.9628, -0.2702, -0,
		-0.492543, 0.506858, 1.16007, 0.9628, -0.2702, -0,
		-1.15808, 0.693654, 1.16007, 0.257, 0.9157, 0.309,
		-0.49233, 0.507616, 1.15728, 0.2628, 0.9362, 0.2334,
		-1.15787, 0.694412, 1.15728, 0.2628, 0.9362, 0.2334,
		-0.49233, 0.507616, 1.15728, 0.2628, 0.9362, 0.2334,
		-1.15808, 0.693654, 1.16007, 0.257, 0.9157, 0.309,
		-0.492543, 0.506858, 1.16007, 0.257, 0.9157, 0.309,
		-1.16029, 0.685768, 1.15512, -0.2628, -0.9362, -0.2334,
		-0.494757, 0.498971, 1.15512, -0.2628, -0.9362, -0.2334,
		-1.16045, 0.685199, 1.15721, -0.257, -0.9157, -0.309,
		-0.494916, 0.498403, 1.15721, -0.257, -0.9157, -0.309,
		-1.16045, 0.685199, 1.15721, -0.257, -0.9157, -0.309,
		-0.494757, 0.498971, 1.15512, -0.2628, -0.9362, -0.2334,
		-1.15771, 0.694957, 1.15443, -0.9628, 0.2702, -0,
		-1.16018, 0.686176, 1.15299, -0.9628, 0.2702, -0,
		-1.16029, 0.685768, 1.15512, -0.9628, 0.2702, -0,
		-1.15787, 0.694412, 1.15728, -0.9628, 0.2702, -0,
		-1.15771, 0.694957, 1.15443, -0.9628, 0.2702, -0,
		-1.16029, 0.685768, 1.15512, -0.9628, 0.2702, -0,
		-0.494757, 0.498971, 1.15512, 0.9628, -0.2702, -0,
		-0.494642, 0.49938, 1.15299, 0.9628, -0.2702, -0,
		-0.492177, 0.508161, 1.15443, 0.9628, -0.2702, -0,
		-0.494757, 0.498971, 1.15512, 0.9628, -0.2702, -0,
		-0.492177, 0.508161, 1.15443, 0.9628, -0.2702, -0,
		-0.49233, 0.507616, 1.15728, 0.9628, -0.2702, -0,
		-1.15787, 0.694412, 1.15728, 0.2628, 0.9362, 0.2334,
		-0.492177, 0.508161, 1.15443, 0.2669, 0.9509, 0.1564,
		-1.15771, 0.694957, 1.15443, 0.2669, 0.9509, 0.1564,
		-0.492177, 0.508161, 1.15443, 0.2669, 0.9509, 0.1564,
		-1.15787, 0.694412, 1.15728, 0.2628, 0.9362, 0.2334,
		-0.49233, 0.507616, 1.15728, 0.2628, 0.9362, 0.2334,
		-1.16018, 0.686176, 1.15299, -0.2669, -0.9509, -0.1564,
		-0.494642, 0.49938, 1.15299, -0.2669, -0.9509, -0.1564,
		-1.16029, 0.685768, 1.15512, -0.2628, -0.9362, -0.2334,
		-0.494757, 0.498971, 1.15512, -0.2628, -0.9362, -0.2334,
		-1.16029, 0.685768, 1.15512, -0.2628, -0.9362, -0.2334,
		-0.494642, 0.49938, 1.15299, -0.2669, -0.9509, -0.1564,
		-1.15762, 0.695285, 1.15155, -0.9628, 0.2702, -0,
		-1.16011, 0.686423, 1.15083, -0.9628, 0.2702, -0,
		-1.16018, 0.686176, 1.15299, -0.9628, 0.2702, -0,
		-1.15771, 0.694957, 1.15443, -0.9628, 0.2702, -0,
		-1.15762, 0.695285, 1.15155, -0.9628, 0.2702, -0,
		-1.16018, 0.686176, 1.15299, -0.9628, 0.2702, -0,
		-0.494642, 0.49938, 1.15299, 0.9628, -0.2702, -0,
		-0.494573, 0.499626, 1.15083, 0.9628, -0.2702, -0,
		-0.492085, 0.508489, 1.15155, 0.9628, -0.2702, -0,
		-0.494642, 0.49938, 1.15299, 0.9628, -0.2702, -0,
		-0.492085, 0.508489, 1.15155, 0.9628, -0.2702, -0,
		-0.492177, 0.508161, 1.15443, 0.9628, -0.2702, -0,
		-1.15771, 0.694957, 1.15443, 0.2669, 0.9509, 0.1564,
		-0.492085, 0.508489, 1.15155, 0.2694, 0.9598, 0.0785,
		-1.15762, 0.695285, 1.15155, 0.2694, 0.9598, 0.0785,
		-0.492085, 0.508489, 1.15155, 0.2694, 0.9598, 0.0785,
		-1.15771, 0.694957, 1.15443, 0.2669, 0.9509, 0.1564,
		-0.492177, 0.508161, 1.15443, 0.2669, 0.9509, 0.1564,
		-1.16011, 0.686423, 1.15083, -0.2694, -0.9598, -0.0785,
		-0.494573, 0.499626, 1.15083, -0.2694, -0.9598, -0.0785,
		-1.16018, 0.686176, 1.15299, -0.2669, -0.9509, -0.1564,
		-0.494642, 0.49938, 1.15299, -0.2669, -0.9509, -0.1564,
		-1.16018, 0.686176, 1.15299, -0.2669, -0.9509, -0.1564,
		-0.494573, 0.499626, 1.15083, -0.2694, -0.9598, -0.0785,
		-1.15759, 0.695395, 1.14865, -0.9628, 0.2702, -0,
		-1.16008, 0.686505, 1.14865, -0.9628, 0.2702, -0,
		-1.16011, 0.686423, 1.15083, -0.9628, 0.2702, -0,
		-1.15762, 0.695285, 1.15155, -0.9628, 0.2702, -0,
		-1.15759, 0.695395, 1.14865, -0.9628, 0.2702, -0,
		-1.16011, 0.686423, 1.15083, -0.9628, 0.2702, -0,
		-0.494573, 0.499626, 1.15083, 0.9628, -0.2702, -0,
		-0.49455, 0.499708, 1.14865, 0.9628, -0.2702, -0,
		-0.492055, 0.508599, 1.14865, 0.9628, -0.2702, -0,
		-0.494573, 0.499626, 1.15083, 0.9628, -0.2702, -0,
		-0.492055, 0.508599, 1.14865, 0.9628, -0.2702, -0,
		-0.492085, 0.508489, 1.15155, 0.9628, -0.2702, -0,
		-1.15762, 0.695285, 1.15155, 0.2694, 0.9598, 0.0785,
		-0.492055, 0.508599, 1.14865, 0.2702, 0.9628, -0,
		-1.15759, 0.695395, 1.14865, 0.2702, 0.9628, -0,
		-0.492055, 0.508599, 1.14865, 0.2702, 0.9628, -0,
		-1.15762, 0.695285, 1.15155, 0.2694, 0.9598, 0.0785,
		-0.492085, 0.508489, 1.15155, 0.2694, 0.9598, 0.0785,
		-1.16008, 0.686505, 1.14865, -0.2702, -0.9628, -0,
		-0.49455, 0.499708, 1.14865, -0.2702, -0.9628, -0,
		-1.16011, 0.686423, 1.15083, -0.2694, -0.9598, -0.0785,
		-0.494573, 0.499626, 1.15083, -0.2694, -0.9598, -0.0785,
		-1.16011, 0.686423, 1.15083, -0.2694, -0.9598, -0.0785,
		-0.49455, 0.499708, 1.14865, -0.2702, -0.9628, -0,
		-1.15762, 0.695285, 1.14576, -0.9628, 0.2702, -0,
		-1.16011, 0.686423, 1.14648, -0.9628, 0.2702, -0,
		-1.16008, 0.686505, 1.14865, -0.9628, 0.2702, -0,
		-1.15759, 0.695395, 1.14865, -0.9628, 0.2702, -0,
		-1.15762, 0.695285, 1.14576, -0.9628, 0.2702, -0,
		-1.16008, 0.686505, 1.14865, -0.9628, 0.2702, -0,
		-0.49455, 0.499708, 1.14865, 0.9628, -0.2702, -0,
		-0.494573, 0.499626, 1.14648, 0.9628, -0.2702, -0,
		-0.492085, 0.508489, 1.14576, 0.9628, -0.2702, -0,
		-0.49455, 0.499708, 1.14865, 0.9628, -0.2702, -0,
		-0.492085, 0.508489, 1.14576, 0.9628, -0.2702, -0,
		-0.492055, 0.508599, 1.14865, 0.9628, -0.2702, -0,
		-1.15759, 0.695395, 1.14865, 0.2702, 0.9628, -0,
		-0.492085, 0.508489, 1.14576, 0.2694, 0.9598, -0.0785,
		-1.15762, 0.695285, 1.14576, 0.2694, 0.9598, -0.0785,
		-0.492085, 0.508489, 1.14576, 0.2694, 0.9598, -0.0785,
		-1.15759, 0.695395, 1.14865, 0.2702, 0.9628, -0,
		-0.492055, 0.508599, 1.14865, 0.2702, 0.9628, -0,
		-1.16011, 0.686423, 1.14648, -0.2694, -0.9598, 0.0785,
		-0.494573, 0.499626, 1.14648, -0.2694, -0.9598, 0.0785,
		-1.16008, 0.686505, 1.14865, -0.2702, -0.9628, -0,
		-0.49455, 0.499708, 1.14865, -0.2702, -0.9628, -0,
		-1.16008, 0.686505, 1.14865, -0.2702, -0.9628, -0,
		-0.494573, 0.499626, 1.14648, -0.2694, -0.9598, 0.0785,
		-1.15771, 0.694957, 1.14288, -0.9628, 0.2702, -0,
		-1.16018, 0.686176, 1.14432, -0.9628, 0.2702, -0,
		-1.16011, 0.686423, 1.14648, -0.9628, 0.2702, -0,
		-1.15762, 0.695285, 1.14576, -0.9628, 0.2702, -0,
		-1.15771, 0.694957, 1.14288, -0.9628, 0.2702, -0,
		-1.16011, 0.686423, 1.14648, -0.9628, 0.2702, -0,
		-0.494573, 0.499626, 1.14648, 0.9628, -0.2702, -0,
		-0.494642, 0.49938, 1.14432, 0.9628, -0.2702, -0,
		-0.492177, 0.508161, 1.14288, 0.9628, -0.2702, -0,
		-0.494573, 0.499626, 1.14648, 0.9628, -0.2702, -0,
		-0.492177, 0.508161, 1.14288, 0.9628, -0.2702, -0,
		-0.492085, 0.508489, 1.14576, 0.9628, -0.2702, -0,
		-1.15762, 0.695285, 1.14576, 0.2694, 0.9598, -0.0785,
		-0.492177, 0.508161, 1.14288, 0.2669, 0.9509, -0.1564,
		-1.15771, 0.694957, 1.14288, 0.2669, 0.9509, -0.1564,
		-0.492177, 0.508161, 1.14288, 0.2669, 0.9509, -0.1564,
		-1.15762, 0.695285, 1.14576, 0.2694, 0.9598, -0.0785,
		-0.492085, 0.508489, 1.14576, 0.2694, 0.9598, -0.0785,
		-1.16018, 0.686176, 1.14432, -0.2669, -0.9509, 0.1564,
		-0.494642, 0.49938, 1.14432, -0.2669, -0.9509, 0.1564,
		-1.16011, 0.686423, 1.14648, -0.2694, -0.9598, 0.0785,
		-0.494573, 0.499626, 1.14648, -0.2694, -0.9598, 0.0785,
		-1.16011, 0.686423, 1.14648, -0.2694, -0.9598, 0.0785,
		-0.494642, 0.49938, 1.14432, -0.2669, -0.9509, 0.1564,
		-1.15787, 0.694412, 1.14003, -0.9628, 0.2702, -0,
		-1.16029, 0.685768, 1.14219, -0.9628, 0.2702, -0,
		-1.16018, 0.686176, 1.14432, -0.9628, 0.2702, -0,
		-1.15771, 0.694957, 1.14288, -0.9628, 0.2702, -0,
		-1.15787, 0.694412, 1.14003, -0.9628, 0.2702, -0,
		-1.16018, 0.686176, 1.14432, -0.9628, 0.2702, -0,
		-0.494642, 0.49938, 1.14432, 0.9628, -0.2702, -0,
		-0.494757, 0.498971, 1.14219, 0.9628, -0.2702, -0,
		-0.49233, 0.507616, 1.14003, 0.9628, -0.2702, -0,
		-0.494642, 0.49938, 1.14432, 0.9628, -0.2702, -0,
		-0.49233, 0.507616, 1.14003, 0.9628, -0.2702, -0,
		-0.492177, 0.508161, 1.14288, 0.9628, -0.2702, -0,
		-1.15771, 0.694957, 1.14288, 0.2669, 0.9509, -0.1564,
		-0.49233, 0.507616, 1.14003, 0.2628, 0.9362, -0.2334,
		-1.15787, 0.694412, 1.14003, 0.2628, 0.9362, -0.2334,
		-0.49233, 0.507616, 1.14003, 0.2628, 0.9362, -0.2334,
		-1.15771, 0.694957, 1.14288, 0.2669, 0.9509, -0.1564,
		-0.492177, 0.508161, 1.14288, 0.2669, 0.9509, -0.1564,
		-1.16029, 0.685768, 1.14219, -0.2628, -0.9362, 0.2334,
		-0.494757, 0.498971, 1.14219, -0.2628, -0.9362, 0.2334,
		-1.16018, 0.686176, 1.14432, -0.2669, -0.9509, 0.1564,
		-0.494642, 0.49938, 1.14432, -0.2669, -0.9509, 0.1564,
		-1.16018, 0.686176, 1.14432, -0.2669, -0.9509, 0.1564,
		-0.494757, 0.498971, 1.14219, -0.2628, -0.9362, 0.2334,
		-1.15808, 0.693654, 1.13724, -0.9628, 0.2702, -0,
		-1.16045, 0.685199, 1.14009, -0.9628, 0.2702, -0,
		-1.16029, 0.685768, 1.14219, -0.9628, 0.2702, -0,
		-1.15787, 0.694412, 1.14003, -0.9628, 0.2702, -0,
		-1.15808, 0.693654, 1.13724, -0.9628, 0.2702, -0,
		-1.16029, 0.685768, 1.14219, -0.9628, 0.2702, -0,
		-0.494757, 0.498971, 1.14219, 0.9628, -0.2702, -0,
		-0.494916, 0.498403, 1.14009, 0.9628, -0.2702, -0,
		-0.492543, 0.506858, 1.13724, 0.9628, -0.2702, -0,
		-0.494757, 0.498971, 1.14219, 0.9628, -0.2702, -0,
		-0.492543, 0.506858, 1.13724, 0.9628, -0.2702, -0,
		-0.49233, 0.507616, 1.14003, 0.9628, -0.2702, -0,
		-1.15787, 0.694412, 1.14003, 0.2628, 0.9362, -0.2334,
		-0.492543, 0.506858, 1.13724, 0.257, 0.9157, -0.309,
		-1.15808, 0.693654, 1.13724, 0.257, 0.9157, -0.309,
		-0.492543, 0.506858, 1.13724, 0.257, 0.9157, -0.309,
		-1.15787, 0.694412, 1.14003, 0.2628, 0.9362, -0.2334,
		-0.49233, 0.507616, 1.14003, 0.2628, 0.9362, -0.2334,
		-1.16045, 0.685199, 1.14009, -0.257, -0.9157, 0.309,
		-0.494916, 0.498403, 1.14009, -0.257, -0.9157, 0.309,
		-1.16029, 0.685768, 1.14219, -0.2628, -0.9362, 0.2334,
		-0.494757, 0.498971, 1.14219, -0.2628, -0.9362, 0.2334,
		-1.16029, 0.685768, 1.14219, -0.2628, -0.9362, 0.2334,
		-0.494916, 0.498403, 1.14009, -0.257, -0.9157, 0.309,
		-1.15835, 0.692688, 1.13452, -0.9628, 0.2702, -0,
		-1.16065, 0.684475, 1.13805, -0.9628, 0.2702, -0,
		-1.16045, 0.685199, 1.14009, -0.9628, 0.2702, -0,
		-1.15808, 0.693654, 1.13724, -0.9628, 0.2702, -0,
		-1.15835, 0.692688, 1.13452, -0.9628, 0.2702, -0,
		-1.16045, 0.685199, 1.14009, -0.9628, 0.2702, -0,
		-0.494916, 0.498403, 1.14009, 0.9628, -0.2702, -0,
		-0.49512, 0.497678, 1.13805, 0.9628, -0.2702, -0,
		-0.492814, 0.505892, 1.13452, 0.9628, -0.2702, -0,
		-0.494916, 0.498403, 1.14009, 0.9628, -0.2702, -0,
		-0.492814, 0.505892, 1.13452, 0.9628, -0.2702, -0,
		-0.492543, 0.506858, 1.13724, 0.9628, -0.2702, -0,
		-1.15808, 0.693654, 1.13724, 0.257, 0.9157, -0.309,
		-0.492814, 0.505892, 1.13452, 0.2497, 0.8895, -0.3827,
		-1.15835, 0.692688, 1.13452, 0.2497, 0.8895, -0.3827,
		-0.492814, 0.505892, 1.13452, 0.2497, 0.8895, -0.3827,
		-1.15808, 0.693654, 1.13724, 0.257, 0.9157, -0.309,
		-0.492543, 0.506858, 1.13724, 0.257, 0.9157, -0.309,
		-1.16065, 0.684475, 1.13805, -0.2497, -0.8895, 0.3827,
		-0.49512, 0.497678, 1.13805, -0.2497, -0.8895, 0.3827,
		-1.16045, 0.685199, 1.14009, -0.257, -0.9157, 0.309,
		-0.494916, 0.498403, 1.14009, -0.257, -0.9157, 0.309,
		-1.16045, 0.685199, 1.14009, -0.257, -0.9157, 0.309,
		-0.49512, 0.497678, 1.13805, -0.2497, -0.8895, 0.3827,
		-1.15868, 0.691519, 1.13189, -0.9628, 0.2702, -0,
		-1.1609, 0.683598, 1.13608, -0.9628, 0.2702, -0,
		-1.16065, 0.684475, 1.13805, -0.9628, 0.2702, -0,
		-1.15835, 0.692688, 1.13452, -0.9628, 0.2702, -0,
		-1.15868, 0.691519, 1.13189, -0.9628, 0.2702, -0,
		-1.16065, 0.684475, 1.13805, -0.9628, 0.2702, -0,
		-0.49512, 0.497678, 1.13805, 0.9628, -0.2702, -0,
		-0.495366, 0.496801, 1.13608, 0.9628, -0.2702, -0,
		-0.493142, 0.504723, 1.13189, 0.9628, -0.2702, -0,
		-0.49512, 0.497678, 1.13805, 0.9628, -0.2702, -0,
		-0.493142, 0.504723, 1.13189, 0.9628, -0.2702, -0,
		-0.492814, 0.505892, 1.13452, 0.9628, -0.2702, -0,
		-1.15835, 0.692688, 1.13452, 0.2497, 0.8895, -0.3827,
		-0.493142, 0.504723, 1.13189, 0.2408, 0.8579, -0.454,
		-1.15868, 0.691519, 1.13189, 0.2408, 0.8579, -0.454,
		-0.493142, 0.504723, 1.13189, 0.2408, 0.8579, -0.454,
		-1.15835, 0.692688, 1.13452, 0.2497, 0.8895, -0.3827,
		-0.492814, 0.505892, 1.13452, 0.2497, 0.8895, -0.3827,
		-1.1609, 0.683598, 1.13608, -0.2408, -0.8579, 0.454,
		-0.495366, 0.496801, 1.13608, -0.2408, -0.8579, 0.454,
		-1.16065, 0.684475, 1.13805, -0.2497, -0.8895, 0.3827,
		-0.49512, 0.497678, 1.13805, -0.2497, -0.8895, 0.3827,
		-1.16065, 0.684475, 1.13805, -0.2497, -0.8895, 0.3827,
		-0.495366, 0.496801, 1.13608, -0.2408, -0.8579, 0.454,
		-1.15906, 0.690155, 1.12935, -0.9628, 0.2702, -0,
		-1.16119, 0.682575, 1.13418, -0.9628, 0.2702, -0,
		-1.1609, 0.683598, 1.13608, -0.9628, 0.2702, -0,
		-1.15868, 0.691519, 1.13189, -0.9628, 0.2702, -0,
		-1.15906, 0.690155, 1.12935, -0.9628, 0.2702, -0,
		-1.1609, 0.683598, 1.13608, -0.9628, 0.2702, -0,
		-0.495366, 0.496801, 1.13608, 0.9628, -0.2702, -0,
		-0.495653, 0.495778, 1.13418, 0.9628, -0.2702, -0,
		-0.493525, 0.503358, 1.12935, 0.9628, -0.2702, -0,
		-0.495366, 0.496801, 1.13608, 0.9628, -0.2702, -0,
		-0.493525, 0.503358, 1.12935, 0.9628, -0.2702, -0,
		-0.493142, 0.504723, 1.13189, 0.9628, -0.2702, -0,
		-1.15868, 0.691519, 1.13189, 0.2408, 0.8579, -0.454,
		-0.493525, 0.503358, 1.12935, 0.2304, 0.8209, -0.5225,
		-1.15906, 0.690155, 1.12935, 0.2304, 0.8209, -0.5225,
		-0.493525, 0.503358, 1.12935, 0.2304, 0.8209, -0.5225,
		-1.15868, 0.691519, 1.13189, 0.2408, 0.8579, -0.454,
		-0.493142, 0.504723, 1.13189, 0.2408, 0.8579, -0.454,
		-1.16119, 0.682575, 1.13418, -0.2304, -0.8209, 0.5225,
		-0.495653, 0.495778, 1.13418, -0.2304, -0.8209, 0.5225,
		-1.1609, 0.683598, 1.13608, -0.2408, -0.8579, 0.454,
		-0.495366, 0.496801, 1.13608, -0.2408, -0.8579, 0.454,
		-1.1609, 0.683598, 1.13608, -0.2408, -0.8579, 0.454,
		-0.495653, 0.495778, 1.13418, -0.2304, -0.8209, 0.5225,
		-1.15949, 0.688603, 1.12694, -0.9628, 0.2702, -0,
		-1.16151, 0.681411, 1.13237, -0.9628, 0.2702, -0,
		-1.16119, 0.682575, 1.13418, -0.9628, 0.2702, -0,
		-1.15906, 0.690155, 1.12935, -0.9628, 0.2702, -0,
		-1.15949, 0.688603, 1.12694, -0.9628, 0.2702, -0,
		-1.16119, 0.682575, 1.13418, -0.9628, 0.2702, -0,
		-0.495653, 0.495778, 1.13418, 0.9628, -0.2702, -0,
		-0.495979, 0.494615, 1.13237, 0.9628, -0.2702, -0,
		-0.493961, 0.501807, 1.12694, 0.9628, -0.2702, -0,
		-0.495653, 0.495778, 1.13418, 0.9628, -0.2702, -0,
		-0.493961, 0.501807, 1.12694, 0.9628, -0.2702, -0,
		-0.493525, 0.503358, 1.12935, 0.9628, -0.2702, -0,
		-1.15906, 0.690155, 1.12935, 0.2304, 0.8209, -0.5225,
		-0.493961, 0.501807, 1.12694, 0.2186, 0.7789, -0.5878,
		-1.15949, 0.688603, 1.12694, 0.2186, 0.7789, -0.5878,
		-0.493961, 0.501807, 1.12694, 0.2186, 0.7789, -0.5878,
		-1.15906, 0.690155, 1.12935, 0.2304, 0.8209, -0.5225,
		-0.493525, 0.503358, 1.12935, 0.2304, 0.8209, -0.5225,
		-1.16151, 0.681411, 1.13237, -0.2186, -0.7789, 0.5878,
		-0.495979, 0.494615, 1.13237, -0.2186, -0.7789, 0.5878,
		-1.16119, 0.682575, 1.13418, -0.2304, -0.8209, 0.5225,
		-0.495653, 0.495778, 1.13418, -0.2304, -0.8209, 0.5225,
		-1.16119, 0.682575, 1.13418, -0.2304, -0.8209, 0.5225,
		-0.495979, 0.494615, 1.13237, -0.2186, -0.7789, 0.5878,
		-1.15998, 0.686875, 1.12467, -0.9628, 0.2702, -0,
		-1.16188, 0.680115, 1.13066, -0.9628, 0.2702, -0,
		-1.16151, 0.681411, 1.13237, -0.9628, 0.2702, -0,
		-1.15949, 0.688603, 1.12694, -0.9628, 0.2702, -0,
		-1.15998, 0.686875, 1.12467, -0.9628, 0.2702, -0,
		-1.16151, 0.681411, 1.13237, -0.9628, 0.2702, -0,
		-0.495979, 0.494615, 1.13237, 0.9628, -0.2702, -0,
		-0.496343, 0.493318, 1.13066, 0.9628, -0.2702, -0,
		-0.494446, 0.500078, 1.12467, 0.9628, -0.2702, -0,
		-0.495979, 0.494615, 1.13237, 0.9628, -0.2702, -0,
		-0.494446, 0.500078, 1.12467, 0.9628, -0.2702, -0,
		-0.493961, 0.501807, 1.12694, 0.9628, -0.2702, -0,
		-1.15949, 0.688603, 1.12694, 0.2186, 0.7789, -0.5878,
		-0.494446, 0.500078, 1.12467, 0.2055, 0.7321, -0.6494,
		-1.15998, 0.686875, 1.12467, 0.2055, 0.7321, -0.6494,
		-0.494446, 0.500078, 1.12467, 0.2055, 0.7321, -0.6494,
		-1.15949, 0.688603, 1.12694, 0.2186, 0.7789, -0.5878,
		-0.493961, 0.501807, 1.12694, 0.2186, 0.7789, -0.5878,
		-1.16188, 0.680115, 1.13066, -0.2055, -0.7321, 0.6494,
		-0.496343, 0.493318, 1.13066, -0.2055, -0.7321, 0.6494,
		-1.16151, 0.681411, 1.13237, -0.2186, -0.7789, 0.5878,
		-0.495979, 0.494615, 1.13237, -0.2186, -0.7789, 0.5878,
		-1.16151, 0.681411, 1.13237, -0.2186, -0.7789, 0.5878,
		-0.496343, 0.493318, 1.13066, -0.2055, -0.7321, 0.6494,
		-1.16051, 0.684979, 1.12254, -0.9628, 0.2702, -0,
		-1.16228, 0.678693, 1.12907, -0.9628, 0.2702, -0,
		-1.16188, 0.680115, 1.13066, -0.9628, 0.2702, -0,
		-1.15998, 0.686875, 1.12467, -0.9628, 0.2702, -0,
		-1.16051, 0.684979, 1.12254, -0.9628, 0.2702, -0,
		-1.16188, 0.680115, 1.13066, -0.9628, 0.2702, -0,
		-0.496343, 0.493318, 1.13066, 0.9628, -0.2702, -0,
		-0.496742, 0.491897, 1.12907, 0.9628, -0.2702, -0,
		-0.494978, 0.498183, 1.12254, 0.9628, -0.2702, -0,
		-0.496343, 0.493318, 1.13066, 0.9628, -0.2702, -0,
		-0.494978, 0.498183, 1.12254, 0.9628, -0.2702, -0,
		-0.494446, 0.500078, 1.12467, 0.9628, -0.2702, -0,
		-1.15998, 0.686875, 1.12467, 0.2055, 0.7321, -0.6494,
		-0.494978, 0.498183, 1.12254, 0.1911, 0.6808, -0.7071,
		-1.16051, 0.684979, 1.12254, 0.1911, 0.6808, -0.7071,
		-0.494978, 0.498183, 1.12254, 0.1911, 0.6808, -0.7071,
		-1.15998, 0.686875, 1.12467, 0.2055, 0.7321, -0.6494,
		-0.494446, 0.500078, 1.12467, 0.2055, 0.7321, -0.6494,
		-1.16228, 0.678693, 1.12907, -0.1911, -0.6808, 0.7071,
		-0.496742, 0.491897, 1.12907, -0.1911, -0.6808, 0.7071,
		-1.16188, 0.680115, 1.13066, -0.2055, -0.7321, 0.6494,
		-0.496343, 0.493318, 1.13066, -0.2055, -0.7321, 0.6494,
		-1.16188, 0.680115, 1.13066, -0.2055, -0.7321, 0.6494,
		-0.496742, 0.491897, 1.12907, -0.1911, -0.6808, 0.7071,
		-1.16109, 0.682929, 1.12057, -0.9628, 0.2702, -0,
		-1.16271, 0.677155, 1.12759, -0.9628, 0.2702, -0,
		-1.16228, 0.678693, 1.12907, -0.9628, 0.2702, -0,
		-1.16051, 0.684979, 1.12254, -0.9628, 0.2702, -0,
		-1.16109, 0.682929, 1.12057, -0.9628, 0.2702, -0,
		-1.16228, 0.678693, 1.12907, -0.9628, 0.2702, -0,
		-0.496742, 0.491897, 1.12907, 0.9628, -0.2702, -0,
		-0.497174, 0.490359, 1.12759, 0.9628, -0.2702, -0,
		-0.495553, 0.496133, 1.12057, 0.9628, -0.2702, -0,
		-0.496742, 0.491897, 1.12907, 0.9628, -0.2702, -0,
		-0.495553, 0.496133, 1.12057, 0.9628, -0.2702, -0,
		-0.494978, 0.498183, 1.12254, 0.9628, -0.2702, -0,
		-1.16051, 0.684979, 1.12254, 0.1911, 0.6808, -0.7071,
		-0.495553, 0.496133, 1.12057, 0.1755, 0.6253, -0.7604,
		-1.16109, 0.682929, 1.12057, 0.1755, 0.6253, -0.7604,
		-0.495553, 0.496133, 1.12057, 0.1755, 0.6253, -0.7604,
		-1.16051, 0.684979, 1.12254, 0.1911, 0.6808, -0.7071,
		-0.494978, 0.498183, 1.12254, 0.1911, 0.6808, -0.7071,
		-1.16271, 0.677155, 1.12759, -0.1755, -0.6253, 0.7604,
		-0.497174, 0.490359, 1.12759, -0.1755, -0.6253, 0.7604,
		-1.16228, 0.678693, 1.12907, -0.1911, -0.6808, 0.7071,
		-0.496742, 0.491897, 1.12907, -0.1911, -0.6808, 0.7071,
		-1.16228, 0.678693, 1.12907, -0.1911, -0.6808, 0.7071,
		-0.497174, 0.490359, 1.12759, -0.1755, -0.6253, 0.7604,
		-1.1617, 0.680736, 1.11877, -0.9628, 0.2702, -0,
		-1.16317, 0.675511, 1.12624, -0.9628, 0.2702, -0,
		-1.16271, 0.677155, 1.12759, -0.9628, 0.2702, -0,
		-1.16109, 0.682929, 1.12057, -0.9628, 0.2702, -0,
		-1.1617, 0.680736, 1.11877, -0.9628, 0.2702, -0,
		-1.16271, 0.677155, 1.12759, -0.9628, 0.2702, -0,
		-0.497174, 0.490359, 1.12759, 0.9628, -0.2702, -0,
		-0.497635, 0.488714, 1.12624, 0.9628, -0.2702, -0,
		-0.496169, 0.49394, 1.11877, 0.9628, -0.2702, -0,
		-0.497174, 0.490359, 1.12759, 0.9628, -0.2702, -0,
		-0.496169, 0.49394, 1.11877, 0.9628, -0.2702, -0,
		-0.495553, 0.496133, 1.12057, 0.9628, -0.2702, -0,
		-1.16109, 0.682929, 1.12057, 0.1755, 0.6253, -0.7604,
		-0.496169, 0.49394, 1.11877, 0.1588, 0.5659, -0.809,
		-1.1617, 0.680736, 1.11877, 0.1588, 0.5659, -0.809,
		-0.496169, 0.49394, 1.11877, 0.1588, 0.5659, -0.809,
		-1.16109, 0.682929, 1.12057, 0.1755, 0.6253, -0.7604,
		-0.495553, 0.496133, 1.12057, 0.1755, 0.6253, -0.7604,
		-1.16317, 0.675511, 1.12624, -0.1588, -0.5659, 0.809,
		-0.497635, 0.488714, 1.12624, -0.1588, -0.5659, 0.809,
		-1.16271, 0.677155, 1.12759, -0.1755, -0.6253, 0.7604,
		-0.497174, 0.490359, 1.12759, -0.1755, -0.6253, 0.7604,
		-1.16271, 0.677155, 1.12759, -0.1755, -0.6253, 0.7604,
		-0.497635, 0.488714, 1.12624, -0.1588, -0.5659, 0.809,
		-1.16235, 0.678415, 1.11716, -0.9628, 0.2702, -0,
		-1.16366, 0.67377, 1.12503, -0.9628, 0.2702, -0,
		-1.16317, 0.675511, 1.12624, -0.9628, 0.2702, -0,
		-1.1617, 0.680736, 1.11877, -0.9628, 0.2702, -0,
		-1.16235, 0.678415, 1.11716, -0.9628, 0.2702, -0,
		-1.16317, 0.675511, 1.12624, -0.9628, 0.2702, -0,
		-0.497635, 0.488714, 1.12624, 0.9628, -0.2702, -0,
		-0.498124, 0.486973, 1.12503, 0.9628, -0.2702, -0,
		-0.49682, 0.491618, 1.11716, 0.9628, -0.2702, -0,
		-0.497635, 0.488714, 1.12624, 0.9628, -0.2702, -0,
		-0.49682, 0.491618, 1.11716, 0.9628, -0.2702, -0,
		-0.496169, 0.49394, 1.11877, 0.9628, -0.2702, -0,
		-1.1617, 0.680736, 1.11877, 0.1588, 0.5659, -0.809,
		-0.49682, 0.491618, 1.11716, 0.1412, 0.5031, -0.8526,
		-1.16235, 0.678415, 1.11716, 0.1412, 0.503, -0.8526,
		-0.49682, 0.491618, 1.11716, 0.1412, 0.5031, -0.8526,
		-1.1617, 0.680736, 1.11877, 0.1588, 0.5659, -0.809,
		-0.496169, 0.49394, 1.11877, 0.1588, 0.5659, -0.809,
		-1.16366, 0.67377, 1.12503, -0.1412, -0.5031, 0.8526,
		-0.498124, 0.486973, 1.12503, -0.1412, -0.5031, 0.8526,
		-1.16317, 0.675511, 1.12624, -0.1588, -0.5659, 0.809,
		-0.497635, 0.488714, 1.12624, -0.1588, -0.5659, 0.809,
		-1.16317, 0.675511, 1.12624, -0.1588, -0.5659, 0.809,
		-0.498124, 0.486973, 1.12503, -0.1412, -0.5031, 0.8526,
		-1.16304, 0.675978, 1.11574, -0.9628, 0.2702, -0,
		-1.16417, 0.671942, 1.12397, -0.9628, 0.2702, -0,
		-1.16366, 0.67377, 1.12503, -0.9628, 0.2702, -0,
		-1.16235, 0.678415, 1.11716, -0.9628, 0.2702, -0,
		-1.16304, 0.675978, 1.11574, -0.9628, 0.2702, -0,
		-1.16366, 0.67377, 1.12503, -0.9628, 0.2702, -0,
		-0.498124, 0.486973, 1.12503, 0.9628, -0.2702, -0,
		-0.498637, 0.485146, 1.12397, 0.9628, -0.2702, -0,
		-0.497504, 0.489182, 1.11574, 0.9628, -0.2702, -0,
		-0.498124, 0.486973, 1.12503, 0.9628, -0.2702, -0,
		-0.497504, 0.489182, 1.11574, 0.9628, -0.2702, -0,
		-0.49682, 0.491618, 1.11716, 0.9628, -0.2702, -0,
		-1.16235, 0.678415, 1.11716, 0.1412, 0.5031, -0.8526,
		-0.497504, 0.489182, 1.11574, 0.1227, 0.4371, -0.891,
		-1.16304, 0.675978, 1.11574, 0.1227, 0.4371, -0.891,
		-0.497504, 0.489182, 1.11574, 0.1227, 0.4371, -0.891,
		-1.16235, 0.678415, 1.11716, 0.1412, 0.503, -0.8526,
		-0.49682, 0.491618, 1.11716, 0.1412, 0.5031, -0.8526,
		-1.16417, 0.671942, 1.12397, -0.1227, -0.4371, 0.891,
		-0.498637, 0.485146, 1.12397, -0.1227, -0.4371, 0.891,
		-1.16366, 0.67377, 1.12503, -0.1412, -0.5031, 0.8526,
		-0.498124, 0.486973, 1.12503, -0.1412, -0.5031, 0.8526,
		-1.16366, 0.67377, 1.12503, -0.1412, -0.5031, 0.8526,
		-0.498637, 0.485146, 1.12397, -0.1227, -0.4371, 0.891,
		-1.16375, 0.673443, 1.11453, -0.9628, 0.2702, -0,
		-1.16471, 0.670041, 1.12306, -0.9628, 0.2702, -0,
		-1.16417, 0.671942, 1.12397, -0.9628, 0.2702, -0,
		-1.16304, 0.675978, 1.11574, -0.9628, 0.2702, -0,
		-1.16375, 0.673443, 1.11453, -0.9628, 0.2702, -0,
		-1.16417, 0.671942, 1.12397, -0.9628, 0.2702, -0,
		-0.498637, 0.485146, 1.12397, 0.9628, -0.2702, -0,
		-0.499171, 0.483244, 1.12306, 0.9628, -0.2702, -0,
		-0.498216, 0.486646, 1.11453, 0.9628, -0.2702, -0,
		-0.498637, 0.485146, 1.12397, 0.9628, -0.2702, -0,
		-0.498216, 0.486646, 1.11453, 0.9628, -0.2702, -0,
		-0.497504, 0.489182, 1.11574, 0.9628, -0.2702, -0,
		-1.16304, 0.675978, 1.11574, 0.1227, 0.4371, -0.891,
		-0.498216, 0.486646, 1.11453, 0.1034, 0.3684, -0.9239,
		-1.16375, 0.673443, 1.11453, 0.1034, 0.3684, -0.9239,
		-0.498216, 0.486646, 1.11453, 0.1034, 0.3684, -0.9239,
		-1.16304, 0.675978, 1.11574, 0.1227, 0.4371, -0.891,
		-0.497504, 0.489182, 1.11574, 0.1227, 0.4371, -0.891,
		-1.16471, 0.670041, 1.12306, -0.1034, -0.3684, 0.9239,
		-0.499171, 0.483244, 1.12306, -0.1034, -0.3684, 0.9239,
		-1.16417, 0.671942, 1.12397, -0.1227, -0.4371, 0.891,
		-0.498637, 0.485146, 1.12397, -0.1227, -0.4371, 0.891,
		-1.16417, 0.671942, 1.12397, -0.1227, -0.4371, 0.891,
		-0.499171, 0.483244, 1.12306, -0.1034, -0.3684, 0.9239,
		-1.16449, 0.670823, 1.11353, -0.9628, 0.2702, -0,
		-1.16526, 0.668076, 1.12231, -0.9628, 0.2702, -0,
		-1.16471, 0.670041, 1.12306, -0.9628, 0.2702, -0,
		-1.16375, 0.673443, 1.11453, -0.9628, 0.2702, -0,
		-1.16449, 0.670823, 1.11353, -0.9628, 0.2702, -0,
		-1.16471, 0.670041, 1.12306, -0.9628, 0.2702, -0,
		-0.499171, 0.483244, 1.12306, 0.9628, -0.2702, -0,
		-0.499722, 0.481279, 1.12231, 0.9628, -0.2702, -0,
		-0.498951, 0.484027, 1.11353, 0.9628, -0.2702, -0,
		-0.499171, 0.483244, 1.12306, 0.9628, -0.2702, -0,
		-0.498951, 0.484027, 1.11353, 0.9628, -0.2702, -0,
		-0.498216, 0.486646, 1.11453, 0.9628, -0.2702, -0,
		-1.16375, 0.673443, 1.11453, 0.1034, 0.3684, -0.9239,
		-0.498951, 0.484027, 1.11353, 0.0835, 0.2975, -0.9511,
		-1.16449, 0.670823, 1.11353, 0.0835, 0.2975, -0.9511,
		-0.498951, 0.484027, 1.11353, 0.0835, 0.2975, -0.9511,
		-1.16375, 0.673443, 1.11453, 0.1034, 0.3684, -0.9239,
		-0.498216, 0.486646, 1.11453, 0.1034, 0.3684, -0.9239,
		-1.16526, 0.668076, 1.12231, -0.0835, -0.2975, 0.9511,
		-0.499722, 0.481279, 1.12231, -0.0835, -0.2975, 0.9511,
		-1.16471, 0.670041, 1.12306, -0.1034, -0.3684, 0.9239,
		-0.499171, 0.483244, 1.12306, -0.1034, -0.3684, 0.9239,
		-1.16471, 0.670041, 1.12306, -0.1034, -0.3684, 0.9239,
		-0.499722, 0.481279, 1.12231, -0.0835, -0.2975, 0.9511,
		-1.16524, 0.668136, 1.11274, -0.9628, 0.2702, -0,
		-1.16582, 0.66606, 1.12172, -0.9628, 0.2702, -0,
		-1.16526, 0.668076, 1.12231, -0.9628, 0.2702, -0,
		-1.16449, 0.670823, 1.11353, -0.9628, 0.2702, -0,
		-1.16524, 0.668136, 1.11274, -0.9628, 0.2702, -0,
		-1.16526, 0.668076, 1.12231, -0.9628, 0.2702, -0,
		-0.499722, 0.481279, 1.12231, 0.9628, -0.2702, -0,
		-0.500288, 0.479264, 1.12172, 0.9628, -0.2702, -0,
		-0.499705, 0.481339, 1.11274, 0.9628, -0.2702, -0,
		-0.499722, 0.481279, 1.12231, 0.9628, -0.2702, -0,
		-0.499705, 0.481339, 1.11274, 0.9628, -0.2702, -0,
		-0.498951, 0.484027, 1.11353, 0.9628, -0.2702, -0,
		-1.16449, 0.670823, 1.11353, 0.0835, 0.2975, -0.9511,
		-0.499705, 0.481339, 1.11274, 0.0631, 0.2248, -0.9724,
		-1.16524, 0.668136, 1.11274, 0.0631, 0.2248, -0.9724,
		-0.499705, 0.481339, 1.11274, 0.0631, 0.2248, -0.9724,
		-1.16449, 0.670823, 1.11353, 0.0835, 0.2975, -0.9511,
		-0.498951, 0.484027, 1.11353, 0.0835, 0.2975, -0.9511,
		-1.16582, 0.66606, 1.12172, -0.0631, -0.2248, 0.9724,
		-0.500288, 0.479264, 1.12172, -0.0631, -0.2248, 0.9724,
		-1.16526, 0.668076, 1.12231, -0.0835, -0.2975, 0.9511,
		-0.499722, 0.481279, 1.12231, -0.0835, -0.2975, 0.9511,
		-1.16526, 0.668076, 1.12231, -0.0835, -0.2975, 0.9511,
		-0.500288, 0.479264, 1.12172, -0.0631, -0.2248, 0.9724,
		-1.16601, 0.665397, 1.11217, -0.9628, 0.2702, -0,
		-1.1664, 0.664006, 1.12129, -0.9628, 0.2702, -0,
		-1.16582, 0.66606, 1.12172, -0.9628, 0.2702, -0,
		-1.16524, 0.668136, 1.11274, -0.9628, 0.2702, -0,
		-1.16601, 0.665397, 1.11217, -0.9628, 0.2702, -0,
		-1.16582, 0.66606, 1.12172, -0.9628, 0.2702, -0,
		-0.500288, 0.479264, 1.12172, 0.9628, -0.2702, -0,
		-0.500864, 0.47721, 1.12129, 0.9628, -0.2702, -0,
		-0.500474, 0.478601, 1.11217, 0.9628, -0.2702, -0,
		-0.500288, 0.479264, 1.12172, 0.9628, -0.2702, -0,
		-0.500474, 0.478601, 1.11217, 0.9628, -0.2702, -0,
		-0.499705, 0.481339, 1.11274, 0.9628, -0.2702, -0,
		-1.16524, 0.668136, 1.11274, 0.0631, 0.2248, -0.9724,
		-0.500474, 0.478601, 1.11217, 0.0423, 0.1506, -0.9877,
		-1.16601, 0.665397, 1.11217, 0.0423, 0.1506, -0.9877,
		-0.500474, 0.478601, 1.11217, 0.0423, 0.1506, -0.9877,
		-1.16524, 0.668136, 1.11274, 0.0631, 0.2248, -0.9724,
		-0.499705, 0.481339, 1.11274, 0.0631, 0.2248, -0.9724,
		-1.1664, 0.664006, 1.12129, -0.0423, -0.1506, 0.9877,
		-0.500864, 0.47721, 1.12129, -0.0423, -0.1506, 0.9877,
		-1.16582, 0.66606, 1.12172, -0.0631, -0.2248, 0.9724,
		-0.500288, 0.479264, 1.12172, -0.0631, -0.2248, 0.9724,
		-1.16582, 0.66606, 1.12172, -0.0631, -0.2248, 0.9724,
		-0.500864, 0.47721, 1.12129, -0.0423, -0.1506, 0.9877,
		-1.16679, 0.662624, 1.11183, -0.9628, 0.2702, -0,
		-1.16698, 0.661927, 1.12104, -0.9628, 0.2702, -0,
		-1.1664, 0.664006, 1.12129, -0.9628, 0.2702, -0,
		-1.16601, 0.665397, 1.11217, -0.9628, 0.2702, -0,
		-1.16679, 0.662624, 1.11183, -0.9628, 0.2702, -0,
		-1.1664, 0.664006, 1.12129, -0.9628, 0.2702, -0,
		-0.500864, 0.47721, 1.12129, 0.9628, -0.2702, -0,
		-0.501448, 0.47513, 1.12104, 0.9628, -0.2702, -0,
		-0.501252, 0.475828, 1.11183, 0.9628, -0.2702, -0,
		-0.500864, 0.47721, 1.12129, 0.9628, -0.2702, -0,
		-0.501252, 0.475828, 1.11183, 0.9628, -0.2702, -0,
		-0.500474, 0.478601, 1.11217, 0.9628, -0.2702, -0,
		-1.16601, 0.665397, 1.11217, 0.0423, 0.1506, -0.9877,
		-0.501252, 0.475828, 1.11183, 0.0212, 0.0755, -0.9969,
		-1.16679, 0.662624, 1.11183, 0.0212, 0.0755, -0.9969,
		-0.501252, 0.475828, 1.11183, 0.0212, 0.0755, -0.9969,
		-1.16601, 0.665397, 1.11217, 0.0423, 0.1506, -0.9877,
		-0.500474, 0.478601, 1.11217, 0.0423, 0.1506, -0.9877,
		-1.16698, 0.661927, 1.12104, -0.0212, -0.0755, 0.9969,
		-0.501448, 0.47513, 1.12104, -0.0212, -0.0755, 0.9969,
		-1.1664, 0.664006, 1.12129, -0.0423, -0.1506, 0.9877,
		-0.500864, 0.47721, 1.12129, -0.0423, -0.1506, 0.9877,
		-1.1664, 0.664006, 1.12129, -0.0423, -0.1506, 0.9877,
		-0.501448, 0.47513, 1.12104, -0.0212, -0.0755, 0.9969,
		-1.16757, 0.659834, 1.11172, -0.9628, 0.2702, -0,
		-1.16757, 0.659834, 1.12095, -0.9628, 0.2702, -0,
		-1.16698, 0.661927, 1.12104, -0.9628, 0.2702, -0,
		-1.16679, 0.662624, 1.11183, -0.9628, 0.2702, -0,
		-1.16757, 0.659834, 1.11172, -0.9628, 0.2702, -0,
		-1.16698, 0.661927, 1.12104, -0.9628, 0.2702, -0,
		-0.501448, 0.47513, 1.12104, 0.9628, -0.2702, -0,
		-0.502035, 0.473038, 1.12095, 0.9628, -0.2702, -0,
		-0.502035, 0.473038, 1.11172, 0.9628, -0.2702, -0,
		-0.501448, 0.47513, 1.12104, 0.9628, -0.2702, -0,
		-0.502035, 0.473038, 1.11172, 0.9628, -0.2702, -0,
		-0.501252, 0.475828, 1.11183, 0.9628, -0.2702, -0,
		-1.16679, 0.662624, 1.11183, 0.0212, 0.0755, -0.9969,
		-0.502035, 0.473038, 1.11172, -0, -0, -1,
		-1.16757, 0.659834, 1.11172, -0, -0, -1,
		-0.502035, 0.473038, 1.11172, -0, -0, -1,
		-1.16679, 0.662624, 1.11183, 0.0212, 0.0755, -0.9969,
		-0.501252, 0.475828, 1.11183, 0.0212, 0.0755, -0.9969,
		-1.16757, 0.659834, 1.12095, -0, -0, 1,
		-0.502035, 0.473038, 1.12095, -0, -0, 1,
		-1.16698, 0.661927, 1.12104, -0.0212, -0.0755, 0.9969,
		-0.501448, 0.47513, 1.12104, -0.0212, -0.0755, 0.9969,
		-1.16698, 0.661927, 1.12104, -0.0212, -0.0755, 0.9969,
		-0.502035, 0.473038, 1.12095, -0, -0, 1,
		-1.16835, 0.657044, 1.11183, -0.9628, 0.2702, -0,
		-1.16816, 0.657742, 1.12104, -0.9628, 0.2702, -0,
		-1.16757, 0.659834, 1.12095, -0.9628, 0.2702, -0,
		-1.16757, 0.659834, 1.11172, -0.9628, 0.2702, -0,
		-1.16835, 0.657044, 1.11183, -0.9628, 0.2702, -0,
		-1.16757, 0.659834, 1.12095, -0.9628, 0.2702, -0,
		-0.502035, 0.473038, 1.12095, 0.9628, -0.2702, -0,
		-0.502623, 0.470945, 1.12104, 0.9628, -0.2702, -0,
		-0.502818, 0.470248, 1.11183, 0.9628, -0.2702, -0,
		-0.502035, 0.473038, 1.12095, 0.9628, -0.2702, -0,
		-0.502818, 0.470248, 1.11183, 0.9628, -0.2702, -0,
		-0.502035, 0.473038, 1.11172, 0.9628, -0.2702, -0,
		-1.16757, 0.659834, 1.11172, -0, -0, -1,
		-0.502818, 0.470248, 1.11183, -0.0212, -0.0755, -0.9969,
		-1.16835, 0.657044, 1.11183, -0.0212, -0.0755, -0.9969,
		-0.502818, 0.470248, 1.11183, -0.0212, -0.0755, -0.9969,
		-1.16757, 0.659834, 1.11172, -0, -0, -1,
		-0.502035, 0.473038, 1.11172, -0, -0, -1,
		-1.16816, 0.657742, 1.12104, 0.0212, 0.0755, 0.9969,
		-0.502623, 0.470945, 1.12104, 0.0212, 0.0755, 0.9969,
		-1.16757, 0.659834, 1.12095, -0, -0, 1,
		-0.502035, 0.473038, 1.12095, -0, -0, 1,
		-1.16757, 0.659834, 1.12095, -0, -0, 1,
		-0.502623, 0.470945, 1.12104, 0.0212, 0.0755, 0.9969,
		-1.16913, 0.654271, 1.11217, -0.9628, 0.2702, -0,
		-1.16874, 0.655662, 1.12129, -0.9628, 0.2702, -0,
		-1.16816, 0.657742, 1.12104, -0.9628, 0.2702, -0,
		-1.16835, 0.657044, 1.11183, -0.9628, 0.2702, -0,
		-1.16913, 0.654271, 1.11217, -0.9628, 0.2702, -0,
		-1.16816, 0.657742, 1.12104, -0.9628, 0.2702, -0,
		-0.502623, 0.470945, 1.12104, 0.9628, -0.2702, -0,
		-0.503206, 0.468866, 1.12129, 0.9628, -0.2702, -0,
		-0.503597, 0.467475, 1.11217, 0.9628, -0.2702, -0,
		-0.502623, 0.470945, 1.12104, 0.9628, -0.2702, -0,
		-0.503597, 0.467475, 1.11217, 0.9628, -0.2702, -0,
		-0.502818, 0.470248, 1.11183, 0.9628, -0.2702, -0,
		-1.16835, 0.657044, 1.11183, -0.0212, -0.0755, -0.9969,
		-0.503597, 0.467475, 1.11217, -0.0423, -0.1506, -0.9877,
		-1.16913, 0.654271, 1.11217, -0.0423, -0.1506, -0.9877,
		-0.503597, 0.467475, 1.11217, -0.0423, -0.1506, -0.9877,
		-1.16835, 0.657044, 1.11183, -0.0212, -0.0755, -0.9969,
		-0.502818, 0.470248, 1.11183, -0.0212, -0.0755, -0.9969,
		-1.16874, 0.655662, 1.12129, 0.0423, 0.1506, 0.9877,
		-0.503206, 0.468866, 1.12129, 0.0423, 0.1506, 0.9877,
		-1.16816, 0.657742, 1.12104, 0.0212, 0.0755, 0.9969,
		-0.502623, 0.470945, 1.12104, 0.0212, 0.0755, 0.9969,
		-1.16816, 0.657742, 1.12104, 0.0212, 0.0755, 0.9969,
		-0.503206, 0.468866, 1.12129, 0.0423, 0.1506, 0.9877,
		-1.1699, 0.651533, 1.11274, -0.9628, 0.2702, -0,
		-1.16932, 0.653608, 1.12172, -0.9628, 0.2702, -0,
		-1.16874, 0.655662, 1.12129, -0.9628, 0.2702, -0,
		-1.16913, 0.654271, 1.11217, -0.9628, 0.2702, -0,
		-1.1699, 0.651533, 1.11274, -0.9628, 0.2702, -0,
		-1.16874, 0.655662, 1.12129, -0.9628, 0.2702, -0,
		-0.503206, 0.468866, 1.12129, 0.9628, -0.2702, -0,
		-0.503783, 0.466812, 1.12172, 0.9628, -0.2702, -0,
		-0.504365, 0.464736, 1.11274, 0.9628, -0.2702, -0,
		-0.503206, 0.468866, 1.12129, 0.9628, -0.2702, -0,
		-0.504365, 0.464736, 1.11274, 0.9628, -0.2702, -0,
		-0.503597, 0.467475, 1.11217, 0.9628, -0.2702, -0,
		-1.16913, 0.654271, 1.11217, -0.0423, -0.1506, -0.9877,
		-0.504365, 0.464736, 1.11274, -0.0631, -0.2248, -0.9724,
		-1.1699, 0.651533, 1.11274, -0.0631, -0.2248, -0.9724,
		-0.504365, 0.464736, 1.11274, -0.0631, -0.2248, -0.9724,
		-1.16913, 0.654271, 1.11217, -0.0423, -0.1506, -0.9877,
		-0.503597, 0.467475, 1.11217, -0.0423, -0.1506, -0.9877,
		-1.16932, 0.653608, 1.12172, 0.0631, 0.2248, 0.9724,
		-0.503783, 0.466812, 1.12172, 0.0631, 0.2248, 0.9724,
		-1.16874, 0.655662, 1.12129, 0.0423, 0.1506, 0.9877,
		-0.503206, 0.468866, 1.12129, 0.0423, 0.1506, 0.9877,
		-1.16874, 0.655662, 1.12129, 0.0423, 0.1506, 0.9877,
		-0.503783, 0.466812, 1.12172, 0.0631, 0.2248, 0.9724,
		-1.17065, 0.648845, 1.11353, -0.9628, 0.2702, -0,
		-1.16988, 0.651593, 1.12231, -0.9628, 0.2702, -0,
		-1.16932, 0.653608, 1.12172, -0.9628, 0.2702, -0,
		-1.1699, 0.651533, 1.11274, -0.9628, 0.2702, -0,
		-1.17065, 0.648845, 1.11353, -0.9628, 0.2702, -0,
		-1.16932, 0.653608, 1.12172, -0.9628, 0.2702, -0,
		-0.503783, 0.466812, 1.12172, 0.9628, -0.2702, -0,
		-0.504349, 0.464796, 1.12231, 0.9628, -0.2702, -0,
		-0.50512, 0.462049, 1.11353, 0.9628, -0.2702, -0,
		-0.503783, 0.466812, 1.12172, 0.9628, -0.2702, -0,
		-0.50512, 0.462049, 1.11353, 0.9628, -0.2702, -0,
		-0.504365, 0.464736, 1.11274, 0.9628, -0.2702, -0,
		-1.1699, 0.651533, 1.11274, -0.0631, -0.2248, -0.9724,
		-0.50512, 0.462049, 1.11353, -0.0835, -0.2975, -0.9511,
		-1.17065, 0.648845, 1.11353, -0.0835, -0.2975, -0.9511,
		-0.50512, 0.462049, 1.11353, -0.0835, -0.2975, -0.9511,
		-1.1699, 0.651533, 1.11274, -0.0631, -0.2248, -0.9724,
		-0.504365, 0.464736, 1.11274, -0.0631, -0.2248, -0.9724,
		-1.16988, 0.651593, 1.12231, 0.0835, 0.2975, 0.9511,
		-0.504349, 0.464796, 1.12231, 0.0835, 0.2975, 0.9511,
		-1.16932, 0.653608, 1.12172, 0.0631, 0.2248, 0.9724,
		-0.503783, 0.466812, 1.12172, 0.0631, 0.2248, 0.9724,
		-1.16932, 0.653608, 1.12172, 0.0631, 0.2248, 0.9724,
		-0.504349, 0.464796, 1.12231, 0.0835, 0.2975, 0.9511,
		-1.17139, 0.646226, 1.11453, -0.9628, 0.2702, -0,
		-1.17043, 0.649628, 1.12306, -0.9628, 0.2702, -0,
		-1.16988, 0.651593, 1.12231, -0.9628, 0.2702, -0,
		-1.17065, 0.648845, 1.11353, -0.9628, 0.2702, -0,
		-1.17139, 0.646226, 1.11453, -0.9628, 0.2702, -0,
		-1.16988, 0.651593, 1.12231, -0.9628, 0.2702, -0,
		-0.504349, 0.464796, 1.12231, 0.9628, -0.2702, -0,
		-0.5049, 0.462831, 1.12306, 0.9628, -0.2702, -0,
		-0.505855, 0.459429, 1.11453, 0.9628, -0.2702, -0,
		-0.504349, 0.464796, 1.12231, 0.9628, -0.2702, -0,
		-0.505855, 0.459429, 1.11453, 0.9628, -0.2702, -0,
		-0.50512, 0.462049, 1.11353, 0.9628, -0.2702, -0,
		-1.17065, 0.648845, 1.11353, -0.0835, -0.2975, -0.9511,
		-0.505855, 0.459429, 1.11453, -0.1034, -0.3684, -0.9239,
		-1.17139, 0.646226, 1.11453, -0.1034, -0.3684, -0.9239,
		-0.505855, 0.459429, 1.11453, -0.1034, -0.3684, -0.9239,
		-1.17065, 0.648845, 1.11353, -0.0835, -0.2975, -0.9511,
		-0.50512, 0.462049, 1.11353, -0.0835, -0.2975, -0.9511,
		-1.17043, 0.649628, 1.12306, 0.1034, 0.3684, 0.9239,
		-0.5049, 0.462831, 1.12306, 0.1034, 0.3684, 0.9239,
		-1.16988, 0.651593, 1.12231, 0.0835, 0.2975, 0.9511,
		-0.504349, 0.464796, 1.12231, 0.0835, 0.2975, 0.9511,
		-1.16988, 0.651593, 1.12231, 0.0835, 0.2975, 0.9511,
		-0.5049, 0.462831, 1.12306, 0.1034, 0.3684, 0.9239,
		-1.1721, 0.64369, 1.11574, -0.9628, 0.2702, -0,
		-1.17097, 0.647726, 1.12397, -0.9628, 0.2702, -0,
		-1.17043, 0.649628, 1.12306, -0.9628, 0.2702, -0,
		-1.17139, 0.646226, 1.11453, -0.9628, 0.2702, -0,
		-1.1721, 0.64369, 1.11574, -0.9628, 0.2702, -0,
		-1.17043, 0.649628, 1.12306, -0.9628, 0.2702, -0,
		-0.5049, 0.462831, 1.12306, 0.9628, -0.2702, -0,
		-0.505434, 0.46093, 1.12397, 0.9628, -0.2702, -0,
		-0.506567, 0.456894, 1.11574, 0.9628, -0.2702, -0,
		-0.5049, 0.462831, 1.12306, 0.9628, -0.2702, -0,
		-0.506567, 0.456894, 1.11574, 0.9628, -0.2702, -0,
		-0.505855, 0.459429, 1.11453, 0.9628, -0.2702, -0,
		-1.17139, 0.646226, 1.11453, -0.1034, -0.3684, -0.9239,
		-0.506567, 0.456894, 1.11574, -0.1227, -0.4371, -0.891,
		-1.1721, 0.64369, 1.11574, -0.1227, -0.4371, -0.891,
		-0.506567, 0.456894, 1.11574, -0.1227, -0.4371, -0.891,
		-1.17139, 0.646226, 1.11453, -0.1034, -0.3684, -0.9239,
		-0.505855, 0.459429, 1.11453, -0.1034, -0.3684, -0.9239,
		-1.17097, 0.647726, 1.12397, 0.1227, 0.4371, 0.891,
		-0.505434, 0.46093, 1.12397, 0.1227, 0.4371, 0.891,
		-1.17043, 0.649628, 1.12306, 0.1034, 0.3684, 0.9239,
		-0.5049, 0.462831, 1.12306, 0.1034, 0.3684, 0.9239,
		-1.17043, 0.649628, 1.12306, 0.1034, 0.3684, 0.9239,
		-0.505434, 0.46093, 1.12397, 0.1227, 0.4371, 0.891,
		-1.17279, 0.641254, 1.11716, -0.9628, 0.2702, -0,
		-1.17148, 0.645899, 1.12503, -0.9628, 0.2702, -0,
		-1.17097, 0.647726, 1.12397, -0.9628, 0.2702, -0,
		-1.1721, 0.64369, 1.11574, -0.9628, 0.2702, -0,
		-1.17279, 0.641254, 1.11716, -0.9628, 0.2702, -0,
		-1.17097, 0.647726, 1.12397, -0.9628, 0.2702, -0,
		-0.505434, 0.46093, 1.12397, 0.9628, -0.2702, -0,
		-0.505947, 0.459103, 1.12503, 0.9628, -0.2702, -0,
		-0.50725, 0.454457, 1.11716, 0.9628, -0.2702, -0,
		-0.505434, 0.46093, 1.12397, 0.9628, -0.2702, -0,
		-0.50725, 0.454457, 1.11716, 0.9628, -0.2702, -0,
		-0.506567, 0.456894, 1.11574, 0.9628, -0.2702, -0,
		-1.1721, 0.64369, 1.11574, -0.1227, -0.4371, -0.891,
		-0.50725, 0.454457, 1.11716, -0.1412, -0.5031, -0.8526,
		-1.17279, 0.641254, 1.11716, -0.1412, -0.5031, -0.8526,
		-0.50725, 0.454457, 1.11716, -0.1412, -0.5031, -0.8526,
		-1.1721, 0.64369, 1.11574, -0.1227, -0.4371, -0.891,
		-0.506567, 0.456894, 1.11574, -0.1227, -0.4371, -0.891,
		-1.17148, 0.645899, 1.12503, 0.1412, 0.5031, 0.8526,
		-0.505947, 0.459103, 1.12503, 0.1412, 0.5031, 0.8526,
		-1.17097, 0.647726, 1.12397, 0.1227, 0.4371, 0.891,
		-0.505434, 0.46093, 1.12397, 0.1227, 0.4371, 0.891,
		-1.17097, 0.647726, 1.12397, 0.1227, 0.4371, 0.891,
		-0.505947, 0.459103, 1.12503, 0.1412, 0.5031, 0.8526,
		-1.17344, 0.638932, 1.11877, -0.9628, 0.2702, -0,
		-1.17197, 0.644158, 1.12624, -0.9628, 0.2702, -0,
		-1.17148, 0.645899, 1.12503, -0.9628, 0.2702, -0,
		-1.17279, 0.641254, 1.11716, -0.9628, 0.2702, -0,
		-1.17344, 0.638932, 1.11877, -0.9628, 0.2702, -0,
		-1.17148, 0.645899, 1.12503, -0.9628, 0.2702, -0,
		-0.505947, 0.459103, 1.12503, 0.9628, -0.2702, -0,
		-0.506435, 0.457361, 1.12624, 0.9628, -0.2702, -0,
		-0.507902, 0.452136, 1.11877, 0.9628, -0.2702, -0,
		-0.505947, 0.459103, 1.12503, 0.9628, -0.2702, -0,
		-0.507902, 0.452136, 1.11877, 0.9628, -0.2702, -0,
		-0.50725, 0.454457, 1.11716, 0.9628, -0.2702, -0,
		-1.17279, 0.641254, 1.11716, -0.1412, -0.5031, -0.8526,
		-0.507902, 0.452136, 1.11877, -0.1588, -0.5659, -0.809,
		-1.17344, 0.638932, 1.11877, -0.1588, -0.5659, -0.809,
		-0.507902, 0.452136, 1.11877, -0.1588, -0.5659, -0.809,
		-1.17279, 0.641254, 1.11716, -0.1412, -0.5031, -0.8526,
		-0.50725, 0.454457, 1.11716, -0.1412, -0.5031, -0.8526,
		-1.17197, 0.644158, 1.12624, 0.1588, 0.5659, 0.809,
		-0.506435, 0.457361, 1.12624, 0.1588, 0.5659, 0.809,
		-1.17148, 0.645899, 1.12503, 0.1412, 0.5031, 0.8526,
		-0.505947, 0.459103, 1.12503, 0.1412, 0.5031, 0.8526,
		-1.17148, 0.645899, 1.12503, 0.1412, 0.5031, 0.8526,
		-0.506435, 0.457361, 1.12624, 0.1588, 0.5659, 0.809,
		-1.17405, 0.636739, 1.12057, -0.9628, 0.2702, -0,
		-1.17243, 0.642513, 1.12759, -0.9628, 0.2702, -0,
		-1.17197, 0.644158, 1.12624, -0.9628, 0.2702, -0,
		-1.17344, 0.638932, 1.11877, -0.9628, 0.2702, -0,
		-1.17405, 0.636739, 1.12057, -0.9628, 0.2702, -0,
		-1.17197, 0.644158, 1.12624, -0.9628, 0.2702, -0,
		-0.506435, 0.457361, 1.12624, 0.9628, -0.2702, -0,
		-0.506897, 0.455717, 1.12759, 0.9628, -0.2702, -0,
		-0.508517, 0.449943, 1.12057, 0.9628, -0.2702, -0,
		-0.506435, 0.457361, 1.12624, 0.9628, -0.2702, -0,
		-0.508517, 0.449943, 1.12057, 0.9628, -0.2702, -0,
		-0.507902, 0.452136, 1.11877, 0.9628, -0.2702, -0,
		-1.17344, 0.638932, 1.11877, -0.1588, -0.5659, -0.809,
		-0.508517, 0.449943, 1.12057, -0.1755, -0.6253, -0.7604,
		-1.17405, 0.636739, 1.12057, -0.1755, -0.6253, -0.7604,
		-0.508517, 0.449943, 1.12057, -0.1755, -0.6253, -0.7604,
		-1.17344, 0.638932, 1.11877, -0.1588, -0.5659, -0.809,
		-0.507902, 0.452136, 1.11877, -0.1588, -0.5659, -0.809,
		-1.17243, 0.642513, 1.12759, 0.1755, 0.6253, 0.7604,
		-0.506897, 0.455717, 1.12759, 0.1755, 0.6253, 0.7604,
		-1.17197, 0.644158, 1.12624, 0.1588, 0.5659, 0.809,
		-0.506435, 0.457361, 1.12624, 0.1588, 0.5659, 0.809,
		-1.17197, 0.644158, 1.12624, 0.1588, 0.5659, 0.809,
		-0.506897, 0.455717, 1.12759, 0.1755, 0.6253, 0.7604,
		-1.17463, 0.634689, 1.12254, -0.9628, 0.2702, -0,
		-1.17286, 0.640975, 1.12907, -0.9628, 0.2702, -0,
		-1.17243, 0.642513, 1.12759, -0.9628, 0.2702, -0,
		-1.17405, 0.636739, 1.12057, -0.9628, 0.2702, -0,
		-1.17463, 0.634689, 1.12254, -0.9628, 0.2702, -0,
		-1.17243, 0.642513, 1.12759, -0.9628, 0.2702, -0,
		-0.506897, 0.455717, 1.12759, 0.9628, -0.2702, -0,
		-0.507329, 0.454179, 1.12907, 0.9628, -0.2702, -0,
		-0.509093, 0.447893, 1.12254, 0.9628, -0.2702, -0,
		-0.506897, 0.455717, 1.12759, 0.9628, -0.2702, -0,
		-0.509093, 0.447893, 1.12254, 0.9628, -0.2702, -0,
		-0.508517, 0.449943, 1.12057, 0.9628, -0.2702, -0,
		-1.17405, 0.636739, 1.12057, -0.1755, -0.6253, -0.7604,
		-0.509093, 0.447893, 1.12254, -0.1911, -0.6808, -0.7071,
		-1.17463, 0.634689, 1.12254, -0.1911, -0.6808, -0.7071,
		-0.509093, 0.447893, 1.12254, -0.1911, -0.6808, -0.7071,
		-1.17405, 0.636739, 1.12057, -0.1755, -0.6253, -0.7604,
		-0.508517, 0.449943, 1.12057, -0.1755, -0.6253, -0.7604,
		-1.17286, 0.640975, 1.12907, 0.1911, 0.6808, 0.7071,
		-0.507329, 0.454179, 1.12907, 0.1911, 0.6808, 0.7071,
		-1.17243, 0.642513, 1.12759, 0.1755, 0.6253, 0.7604,
		-0.506897, 0.455717, 1.12759, 0.1755, 0.6253, 0.7604,
		-1.17243, 0.642513, 1.12759, 0.1755, 0.6253, 0.7604,
		-0.507329, 0.454179, 1.12907, 0.1911, 0.6808, 0.7071,
		-1.17516, 0.632794, 1.12467, -0.9628, 0.2702, -0,
		-1.17326, 0.639554, 1.13066, -0.9628, 0.2702, -0,
		-1.17286, 0.640975, 1.12907, -0.9628, 0.2702, -0,
		-1.17463, 0.634689, 1.12254, -0.9628, 0.2702, -0,
		-1.17516, 0.632794, 1.12467, -0.9628, 0.2702, -0,
		-1.17286, 0.640975, 1.12907, -0.9628, 0.2702, -0,
		-0.507329, 0.454179, 1.12907, 0.9628, -0.2702, -0,
		-0.507728, 0.452757, 1.13066, 0.9628, -0.2702, -0,
		-0.509625, 0.445997, 1.12467, 0.9628, -0.2702, -0,
		-0.507329, 0.454179, 1.12907, 0.9628, -0.2702, -0,
		-0.509625, 0.445997, 1.12467, 0.9628, -0.2702, -0,
		-0.509093, 0.447893, 1.12254, 0.9628, -0.2702, -0,
		-1.17463, 0.634689, 1.12254, -0.1911, -0.6808, -0.7071,
		-0.509625, 0.445997, 1.12467, -0.2055, -0.7321, -0.6494,
		-1.17516, 0.632794, 1.12467, -0.2055, -0.7321, -0.6494,
		-0.509625, 0.445997, 1.12467, -0.2055, -0.7321, -0.6494,
		-1.17463, 0.634689, 1.12254, -0.1911, -0.6808, -0.7071,
		-0.509093, 0.447893, 1.12254, -0.1911, -0.6808, -0.7071,
		-1.17326, 0.639554, 1.13066, 0.2055, 0.7321, 0.6494,
		-0.507728, 0.452757, 1.13066, 0.2055, 0.7321, 0.6494,
		-1.17286, 0.640975, 1.12907, 0.1911, 0.6808, 0.7071,
		-0.507329, 0.454179, 1.12907, 0.1911, 0.6808, 0.7071,
		-1.17286, 0.640975, 1.12907, 0.1911, 0.6808, 0.7071,
		-0.507728, 0.452757, 1.13066, 0.2055, 0.7321, 0.6494,
		-1.17564, 0.631065, 1.12694, -0.9628, 0.2702, -0,
		-1.17363, 0.638257, 1.13237, -0.9628, 0.2702, -0,
		-1.17326, 0.639554, 1.13066, -0.9628, 0.2702, -0,
		-1.17516, 0.632794, 1.12467, -0.9628, 0.2702, -0,
		-1.17564, 0.631065, 1.12694, -0.9628, 0.2702, -0,
		-1.17326, 0.639554, 1.13066, -0.9628, 0.2702, -0,
		-0.507728, 0.452757, 1.13066, 0.9628, -0.2702, -0,
		-0.508091, 0.451461, 1.13237, 0.9628, -0.2702, -0,
		-0.51011, 0.444269, 1.12694, 0.9628, -0.2702, -0,
		-0.507728, 0.452757, 1.13066, 0.9628, -0.2702, -0,
		-0.51011, 0.444269, 1.12694, 0.9628, -0.2702, -0,
		-0.509625, 0.445997, 1.12467, 0.9628, -0.2702, -0,
		-1.17516, 0.632794, 1.12467, -0.2055, -0.7321, -0.6494,
		-0.51011, 0.444269, 1.12694, -0.2186, -0.7789, -0.5878,
		-1.17564, 0.631065, 1.12694, -0.2186, -0.7789, -0.5878,
		-0.51011, 0.444269, 1.12694, -0.2186, -0.7789, -0.5878,
		-1.17516, 0.632794, 1.12467, -0.2055, -0.7321, -0.6494,
		-0.509625, 0.445997, 1.12467, -0.2055, -0.7321, -0.6494,
		-1.17363, 0.638257, 1.13237, 0.2186, 0.7789, 0.5878,
		-0.508091, 0.451461, 1.13237, 0.2186, 0.7789, 0.5878,
		-1.17326, 0.639554, 1.13066, 0.2055, 0.7321, 0.6494,
		-0.507728, 0.452757, 1.13066, 0.2055, 0.7321, 0.6494,
		-1.17326, 0.639554, 1.13066, 0.2055, 0.7321, 0.6494,
		-0.508091, 0.451461, 1.13237, 0.2186, 0.7789, 0.5878,
		-1.17608, 0.629514, 1.12935, -0.9628, 0.2702, -0,
		-1.17395, 0.637094, 1.13418, -0.9628, 0.2702, -0,
		-1.17363, 0.638257, 1.13237, -0.9628, 0.2702, -0,
		-1.17564, 0.631065, 1.12694, -0.9628, 0.2702, -0,
		-1.17608, 0.629514, 1.12935, -0.9628, 0.2702, -0,
		-1.17363, 0.638257, 1.13237, -0.9628, 0.2702, -0,
		-0.508091, 0.451461, 1.13237, 0.9628, -0.2702, -0,
		-0.508418, 0.450298, 1.13418, 0.9628, -0.2702, -0,
		-0.510545, 0.442717, 1.12935, 0.9628, -0.2702, -0,
		-0.508091, 0.451461, 1.13237, 0.9628, -0.2702, -0,
		-0.510545, 0.442717, 1.12935, 0.9628, -0.2702, -0,
		-0.51011, 0.444269, 1.12694, 0.9628, -0.2702, -0,
		-1.17564, 0.631065, 1.12694, -0.2186, -0.7789, -0.5878,
		-0.510545, 0.442717, 1.12935, -0.2304, -0.8209, -0.5225,
		-1.17608, 0.629514, 1.12935, -0.2304, -0.8209, -0.5225,
		-0.510545, 0.442717, 1.12935, -0.2304, -0.8209, -0.5225,
		-1.17564, 0.631065, 1.12694, -0.2186, -0.7789, -0.5878,
		-0.51011, 0.444269, 1.12694, -0.2186, -0.7789, -0.5878,
		-1.17395, 0.637094, 1.13418, 0.2304, 0.8209, 0.5225,
		-0.508418, 0.450298, 1.13418, 0.2304, 0.8209, 0.5225,
		-1.17363, 0.638257, 1.13237, 0.2186, 0.7789, 0.5878,
		-0.508091, 0.451461, 1.13237, 0.2186, 0.7789, 0.5878,
		-1.17363, 0.638257, 1.13237, 0.2186, 0.7789, 0.5878,
		-0.508418, 0.450298, 1.13418, 0.2304, 0.8209, 0.5225,
		-1.17646, 0.62815, 1.13189, -0.9628, 0.2702, -0,
		-1.17424, 0.636071, 1.13608, -0.9628, 0.2702, -0,
		-1.17395, 0.637094, 1.13418, -0.9628, 0.2702, -0,
		-1.17608, 0.629514, 1.12935, -0.9628, 0.2702, -0,
		-1.17646, 0.62815, 1.13189, -0.9628, 0.2702, -0,
		-1.17395, 0.637094, 1.13418, -0.9628, 0.2702, -0,
		-0.508418, 0.450298, 1.13418, 0.9628, -0.2702, -0,
		-0.508705, 0.449274, 1.13608, 0.9628, -0.2702, -0,
		-0.510928, 0.441353, 1.13189, 0.9628, -0.2702, -0,
		-0.508418, 0.450298, 1.13418, 0.9628, -0.2702, -0,
		-0.510928, 0.441353, 1.13189, 0.9628, -0.2702, -0,
		-0.510545, 0.442717, 1.12935, 0.9628, -0.2702, -0,
		-1.17608, 0.629514, 1.12935, -0.2304, -0.8209, -0.5225,
		-0.510928, 0.441353, 1.13189, -0.2408, -0.8579, -0.454,
		-1.17646, 0.62815, 1.13189, -0.2408, -0.8579, -0.454,
		-0.510928, 0.441353, 1.13189, -0.2408, -0.8579, -0.454,
		-1.17608, 0.629514, 1.12935, -0.2304, -0.8209, -0.5225,
		-0.510545, 0.442717, 1.12935, -0.2304, -0.8209, -0.5225,
		-1.17424, 0.636071, 1.13608, 0.2408, 0.8579, 0.454,
		-0.508705, 0.449274, 1.13608, 0.2408, 0.8579, 0.454,
		-1.17395, 0.637094, 1.13418, 0.2304, 0.8209, 0.5225,
		-0.508418, 0.450298, 1.13418, 0.2304, 0.8209, 0.5225,
		-1.17395, 0.637094, 1.13418, 0.2304, 0.8209, 0.5225,
		-0.508705, 0.449274, 1.13608, 0.2408, 0.8579, 0.454,
		-1.17679, 0.62698, 1.13452, -0.9628, 0.2702, -0,
		-1.17449, 0.635194, 1.13805, -0.9628, 0.2702, -0,
		-1.17424, 0.636071, 1.13608, -0.9628, 0.2702, -0,
		-1.17646, 0.62815, 1.13189, -0.9628, 0.2702, -0,
		-1.17679, 0.62698, 1.13452, -0.9628, 0.2702, -0,
		-1.17424, 0.636071, 1.13608, -0.9628, 0.2702, -0,
		-0.508705, 0.449274, 1.13608, 0.9628, -0.2702, -0,
		-0.508951, 0.448398, 1.13805, 0.9628, -0.2702, -0,
		-0.511256, 0.440184, 1.13452, 0.9628, -0.2702, -0,
		-0.508705, 0.449274, 1.13608, 0.9628, -0.2702, -0,
		-0.511256, 0.440184, 1.13452, 0.9628, -0.2702, -0,
		-0.510928, 0.441353, 1.13189, 0.9628, -0.2702, -0,
		-1.17646, 0.62815, 1.13189, -0.2408, -0.8579, -0.454,
		-0.511256, 0.440184, 1.13452, -0.2497, -0.8895, -0.3827,
		-1.17679, 0.62698, 1.13452, -0.2497, -0.8895, -0.3827,
		-0.511256, 0.440184, 1.13452, -0.2497, -0.8895, -0.3827,
		-1.17646, 0.62815, 1.13189, -0.2408, -0.8579, -0.454,
		-0.510928, 0.441353, 1.13189, -0.2408, -0.8579, -0.454,
		-1.17449, 0.635194, 1.13805, 0.2497, 0.8895, 0.3827,
		-0.508951, 0.448398, 1.13805, 0.2497, 0.8895, 0.3827,
		-1.17424, 0.636071, 1.13608, 0.2408, 0.8579, 0.454,
		-0.508705, 0.449274, 1.13608, 0.2408, 0.8579, 0.454,
		-1.17424, 0.636071, 1.13608, 0.2408, 0.8579, 0.454,
		-0.508951, 0.448398, 1.13805, 0.2497, 0.8895, 0.3827,
		-1.17706, 0.626014, 1.13724, -0.9628, 0.2702, -0,
		-1.17469, 0.634469, 1.14009, -0.9628, 0.2702, -0,
		-1.17449, 0.635194, 1.13805, -0.9628, 0.2702, -0,
		-1.17679, 0.62698, 1.13452, -0.9628, 0.2702, -0,
		-1.17706, 0.626014, 1.13724, -0.9628, 0.2702, -0,
		-1.17449, 0.635194, 1.13805, -0.9628, 0.2702, -0,
		-0.508951, 0.448398, 1.13805, 0.9628, -0.2702, -0,
		-0.509155, 0.447673, 1.14009, 0.9628, -0.2702, -0,
		-0.511528, 0.439218, 1.13724, 0.9628, -0.2702, -0,
		-0.508951, 0.448398, 1.13805, 0.9628, -0.2702, -0,
		-0.511528, 0.439218, 1.13724, 0.9628, -0.2702, -0,
		-0.511256, 0.440184, 1.13452, 0.9628, -0.2702, -0,
		-1.17679, 0.62698, 1.13452, -0.2497, -0.8895, -0.3827,
		-0.511528, 0.439218, 1.13724, -0.257, -0.9157, -0.309,
		-1.17706, 0.626014, 1.13724, -0.257, -0.9157, -0.309,
		-0.511528, 0.439218, 1.13724, -0.257, -0.9157, -0.309,
		-1.17679, 0.62698, 1.13452, -0.2497, -0.8895, -0.3827,
		-0.511256, 0.440184, 1.13452, -0.2497, -0.8895, -0.3827,
		-1.17469, 0.634469, 1.14009, 0.257, 0.9157, 0.309,
		-0.509155, 0.447673, 1.14009, 0.257, 0.9157, 0.309,
		-1.17449, 0.635194, 1.13805, 0.2497, 0.8895, 0.3827,
		-0.508951, 0.448398, 1.13805, 0.2497, 0.8895, 0.3827,
		-1.17449, 0.635194, 1.13805, 0.2497, 0.8895, 0.3827,
		-0.509155, 0.447673, 1.14009, 0.257, 0.9157, 0.309,
		-1.17727, 0.625256, 1.14003, -0.9628, 0.2702, -0,
		-1.17485, 0.633901, 1.14219, -0.9628, 0.2702, -0,
		-1.17469, 0.634469, 1.14009, -0.9628, 0.2702, -0,
		-1.17706, 0.626014, 1.13724, -0.9628, 0.2702, -0,
		-1.17727, 0.625256, 1.14003, -0.9628, 0.2702, -0,
		-1.17469, 0.634469, 1.14009, -0.9628, 0.2702, -0,
		-0.509155, 0.447673, 1.14009, 0.9628, -0.2702, -0,
		-0.509314, 0.447104, 1.14219, 0.9628, -0.2702, -0,
		-0.51174, 0.43846, 1.14003, 0.9628, -0.2702, -0,
		-0.509155, 0.447673, 1.14009, 0.9628, -0.2702, -0,
		-0.51174, 0.43846, 1.14003, 0.9628, -0.2702, -0,
		-0.511528, 0.439218, 1.13724, 0.9628, -0.2702, -0,
		-1.17706, 0.626014, 1.13724, -0.257, -0.9157, -0.309,
		-0.51174, 0.43846, 1.14003, -0.2628, -0.9362, -0.2334,
		-1.17727, 0.625256, 1.14003, -0.2628, -0.9362, -0.2334,
		-0.51174, 0.43846, 1.14003, -0.2628, -0.9362, -0.2334,
		-1.17706, 0.626014, 1.13724, -0.257, -0.9157, -0.309,
		-0.511528, 0.439218, 1.13724, -0.257, -0.9157, -0.309,
		-1.17485, 0.633901, 1.14219, 0.2628, 0.9362, 0.2334,
		-0.509314, 0.447104, 1.14219, 0.2628, 0.9362, 0.2334,
		-1.17469, 0.634469, 1.14009, 0.257, 0.9157, 0.309,
		-0.509155, 0.447673, 1.14009, 0.257, 0.9157, 0.309,
		-1.17469, 0.634469, 1.14009, 0.257, 0.9157, 0.309,
		-0.509314, 0.447104, 1.14219, 0.2628, 0.9362, 0.2334,
		-1.17743, 0.624711, 1.14288, -0.9628, 0.2702, -0,
		-1.17496, 0.633492, 1.14432, -0.9628, 0.2702, -0,
		-1.17485, 0.633901, 1.14219, -0.9628, 0.2702, -0,
		-1.17727, 0.625256, 1.14003, -0.9628, 0.2702, -0,
		-1.17743, 0.624711, 1.14288, -0.9628, 0.2702, -0,
		-1.17485, 0.633901, 1.14219, -0.9628, 0.2702, -0,
		-0.509314, 0.447104, 1.14219, 0.9628, -0.2702, -0,
		-0.509429, 0.446696, 1.14432, 0.9628, -0.2702, -0,
		-0.511893, 0.437915, 1.14288, 0.9628, -0.2702, -0,
		-0.509314, 0.447104, 1.14219, 0.9628, -0.2702, -0,
		-0.511893, 0.437915, 1.14288, 0.9628, -0.2702, -0,
		-0.51174, 0.43846, 1.14003, 0.9628, -0.2702, -0,
		-1.17727, 0.625256, 1.14003, -0.2628, -0.9362, -0.2334,
		-0.511893, 0.437915, 1.14288, -0.2669, -0.9509, -0.1564,
		-1.17743, 0.624711, 1.14288, -0.2669, -0.9509, -0.1564,
		-0.511893, 0.437915, 1.14288, -0.2669, -0.9509, -0.1564,
		-1.17727, 0.625256, 1.14003, -0.2628, -0.9362, -0.2334,
		-0.51174, 0.43846, 1.14003, -0.2628, -0.9362, -0.2334,
		-1.17496, 0.633492, 1.14432, 0.2669, 0.9509, 0.1564,
		-0.509429, 0.446696, 1.14432, 0.2669, 0.9509, 0.1564,
		-1.17485, 0.633901, 1.14219, 0.2628, 0.9362, 0.2334,
		-0.509314, 0.447104, 1.14219, 0.2628, 0.9362, 0.2334,
		-1.17485, 0.633901, 1.14219, 0.2628, 0.9362, 0.2334,
		-0.509429, 0.446696, 1.14432, 0.2669, 0.9509, 0.1564,
		-1.17752, 0.624383, 1.14576, -0.9628, 0.2702, -0,
		-1.17503, 0.633246, 1.14648, -0.9628, 0.2702, -0,
		-1.17496, 0.633492, 1.14432, -0.9628, 0.2702, -0,
		-1.17743, 0.624711, 1.14288, -0.9628, 0.2702, -0,
		-1.17752, 0.624383, 1.14576, -0.9628, 0.2702, -0,
		-1.17496, 0.633492, 1.14432, -0.9628, 0.2702, -0,
		-0.509429, 0.446696, 1.14432, 0.9628, -0.2702, -0,
		-0.509498, 0.44645, 1.14648, 0.9628, -0.2702, -0,
		-0.511985, 0.437587, 1.14576, 0.9628, -0.2702, -0,
		-0.509429, 0.446696, 1.14432, 0.9628, -0.2702, -0,
		-0.511985, 0.437587, 1.14576, 0.9628, -0.2702, -0,
		-0.511893, 0.437915, 1.14288, 0.9628, -0.2702, -0,
		-1.17743, 0.624711, 1.14288, -0.2669, -0.9509, -0.1564,
		-0.511985, 0.437587, 1.14576, -0.2694, -0.9598, -0.0785,
		-1.17752, 0.624383, 1.14576, -0.2694, -0.9598, -0.0785,
		-0.511985, 0.437587, 1.14576, -0.2694, -0.9598, -0.0785,
		-1.17743, 0.624711, 1.14288, -0.2669, -0.9509, -0.1564,
		-0.511893, 0.437915, 1.14288, -0.2669, -0.9509, -0.1564,
		-1.17503, 0.633246, 1.14648, 0.2694, 0.9598, 0.0785,
		-0.509498, 0.44645, 1.14648, 0.2694, 0.9598, 0.0785,
		-1.17496, 0.633492, 1.14432, 0.2669, 0.9509, 0.1564,
		-0.509429, 0.446696, 1.14432, 0.2669, 0.9509, 0.1564,
		-1.17496, 0.633492, 1.14432, 0.2669, 0.9509, 0.1564,
		-0.509498, 0.44645, 1.14648, 0.2694, 0.9598, 0.0785,
		-1.17755, 0.624274, 1.14865, -0.9628, 0.2702, -0,
		-1.17506, 0.633164, 1.14865, -0.9628, 0.2702, -0,
		-1.17503, 0.633246, 1.14648, -0.9628, 0.2702, -0,
		-1.17752, 0.624383, 1.14576, -0.9628, 0.2702, -0,
		-1.17755, 0.624274, 1.14865, -0.9628, 0.2702, -0,
		-1.17503, 0.633246, 1.14648, -0.9628, 0.2702, -0,
		-0.509498, 0.44645, 1.14648, 0.9628, -0.2702, -0,
		-0.509521, 0.446367, 1.14865, 0.9628, -0.2702, -0,
		-0.512016, 0.437477, 1.14865, 0.9628, -0.2702, -0,
		-0.509498, 0.44645, 1.14648, 0.9628, -0.2702, -0,
		-0.512016, 0.437477, 1.14865, 0.9628, -0.2702, -0,
		-0.511985, 0.437587, 1.14576, 0.9628, -0.2702, -0,
		-1.17752, 0.624383, 1.14576, -0.2694, -0.9598, -0.0785,
		-0.512016, 0.437477, 1.14865, -0.2702, -0.9628, -0,
		-1.17755, 0.624274, 1.14865, -0.2702, -0.9628, -0,
		-0.512016, 0.437477, 1.14865, -0.2702, -0.9628, -0,
		-1.17752, 0.624383, 1.14576, -0.2694, -0.9598, -0.0785,
		-0.511985, 0.437587, 1.14576, -0.2694, -0.9598, -0.0785,
		-1.17506, 0.633164, 1.14865, 0.2702, 0.9628, -0,
		-0.509521, 0.446367, 1.14865, 0.2702, 0.9628, -0,
		-1.17503, 0.633246, 1.14648, 0.2694, 0.9598, 0.0785,
		-0.509498, 0.44645, 1.14648, 0.2694, 0.9598, 0.0785,
		-1.17503, 0.633246, 1.14648, 0.2694, 0.9598, 0.0785,
		-0.509521, 0.446367, 1.14865, 0.2702, 0.9628, -0,
		-1.17752, 0.624383, 1.15155, -0.9628, 0.2702, -0,
		-1.17503, 0.633246, 1.15083, -0.9628, 0.2702, -0,
		-1.17506, 0.633164, 1.14865, -0.9628, 0.2702, -0,
		-1.17755, 0.624274, 1.14865, -0.9628, 0.2702, -0,
		-1.17752, 0.624383, 1.15155, -0.9628, 0.2702, -0,
		-1.17506, 0.633164, 1.14865, -0.9628, 0.2702, -0,
		-0.509521, 0.446367, 1.14865, 0.9628, -0.2702, -0,
		-0.509498, 0.44645, 1.15083, 0.9628, -0.2702, -0,
		-0.511985, 0.437587, 1.15155, 0.9628, -0.2702, -0,
		-0.509521, 0.446367, 1.14865, 0.9628, -0.2702, -0,
		-0.511985, 0.437587, 1.15155, 0.9628, -0.2702, -0,
		-0.512016, 0.437477, 1.14865, 0.9628, -0.2702, -0,
		-1.17755, 0.624274, 1.14865, -0.2702, -0.9628, -0,
		-0.511985, 0.437587, 1.15155, -0.2694, -0.9598, 0.0784,
		-1.17752, 0.624383, 1.15155, -0.2694, -0.9598, 0.0785,
		-0.511985, 0.437587, 1.15155, -0.2694, -0.9598, 0.0785,
		-1.17755, 0.624274, 1.14865, -0.2702, -0.9628, -0,
		-0.512016, 0.437477, 1.14865, -0.2702, -0.9628, -0,
		-1.17503, 0.633246, 1.15083, 0.2694, 0.9598, -0.0785,
		-0.509498, 0.44645, 1.15083, 0.2694, 0.9598, -0.0785,
		-1.17506, 0.633164, 1.14865, 0.2702, 0.9628, -0,
		-0.509521, 0.446367, 1.14865, 0.2702, 0.9628, -0,
		-1.17506, 0.633164, 1.14865, 0.2702, 0.9628, -0,
		-0.509498, 0.44645, 1.15083, 0.2694, 0.9598, -0.0785,
		-1.17743, 0.624711, 1.15443, -0.9628, 0.2702, -0,
		-1.17496, 0.633492, 1.15299, -0.9628, 0.2702, -0,
		-1.17503, 0.633246, 1.15083, -0.9628, 0.2702, -0,
		-1.17752, 0.624383, 1.15155, -0.9628, 0.2702, -0,
		-1.17743, 0.624711, 1.15443, -0.9628, 0.2702, -0,
		-1.17503, 0.633246, 1.15083, -0.9628, 0.2702, -0,
		-0.509498, 0.44645, 1.15083, 0.9628, -0.2702, -0,
		-0.509429, 0.446696, 1.15299, 0.9628, -0.2702, -0,
		-0.511893, 0.437915, 1.15443, 0.9628, -0.2702, -0,
		-0.509498, 0.44645, 1.15083, 0.9628, -0.2702, -0,
		-0.511893, 0.437915, 1.15443, 0.9628, -0.2702, -0,
		-0.511985, 0.437587, 1.15155, 0.9628, -0.2702, -0,
		-1.17752, 0.624383, 1.15155, -0.2694, -0.9598, 0.0785,
		-0.511893, 0.437915, 1.15443, -0.2669, -0.9509, 0.1564,
		-1.17743, 0.624711, 1.15443, -0.2669, -0.9509, 0.1564,
		-0.511893, 0.437915, 1.15443, -0.2669, -0.9509, 0.1564,
		-1.17752, 0.624383, 1.15155, -0.2694, -0.9598, 0.0785,
		-0.511985, 0.437587, 1.15155, -0.2694, -0.9598, 0.0784,
		-1.17496, 0.633492, 1.15299, 0.2669, 0.9509, -0.1564,
		-0.509429, 0.446696, 1.15299, 0.2669, 0.9509, -0.1564,
		-1.17503, 0.633246, 1.15083, 0.2694, 0.9598, -0.0785,
		-0.509498, 0.44645, 1.15083, 0.2694, 0.9598, -0.0785,
		-1.17503, 0.633246, 1.15083, 0.2694, 0.9598, -0.0785,
		-0.509429, 0.446696, 1.15299, 0.2669, 0.9509, -0.1564,
		-1.17727, 0.625256, 1.15728, -0.9628, 0.2702, -0,
		-1.17485, 0.633901, 1.15512, -0.9628, 0.2702, -0,
		-1.17496, 0.633492, 1.15299, -0.9628, 0.2702, -0,
		-1.17743, 0.624711, 1.15443, -0.9628, 0.2702, -0,
		-1.17727, 0.625256, 1.15728, -0.9628, 0.2702, -0,
		-1.17496, 0.633492, 1.15299, -0.9628, 0.2702, -0,
		-0.509429, 0.446696, 1.15299, 0.9628, -0.2702, -0,
		-0.509314, 0.447104, 1.15512, 0.9628, -0.2702, -0,
		-0.51174, 0.43846, 1.15728, 0.9628, -0.2702, -0,
		-0.509429, 0.446696, 1.15299, 0.9628, -0.2702, -0,
		-0.51174, 0.43846, 1.15728, 0.9628, -0.2702, -0,
		-0.511893, 0.437915, 1.15443, 0.9628, -0.2702, -0,
		-1.17743, 0.624711, 1.15443, -0.2669, -0.9509, 0.1564,
		-0.51174, 0.43846, 1.15728, -0.2628, -0.9362, 0.2334,
		-1.17727, 0.625256, 1.15728, -0.2628, -0.9362, 0.2334,
		-0.51174, 0.43846, 1.15728, -0.2628, -0.9362, 0.2334,
		-1.17743, 0.624711, 1.15443, -0.2669, -0.9509, 0.1564,
		-0.511893, 0.437915, 1.15443, -0.2669, -0.9509, 0.1564,
		-1.17485, 0.633901, 1.15512, 0.2628, 0.9362, -0.2334,
		-0.509314, 0.447104, 1.15512, 0.2628, 0.9362, -0.2335,
		-1.17496, 0.633492, 1.15299, 0.2669, 0.9509, -0.1564,
		-0.509429, 0.446696, 1.15299, 0.2669, 0.9509, -0.1564,
		-1.17496, 0.633492, 1.15299, 0.2669, 0.9509, -0.1564,
		-0.509314, 0.447104, 1.15512, 0.2628, 0.9362, -0.2334,
		-1.17706, 0.626014, 1.16007, -0.9628, 0.2702, -0,
		-1.17469, 0.634469, 1.15721, -0.9628, 0.2702, -0,
		-1.17485, 0.633901, 1.15512, -0.9628, 0.2702, -0,
		-1.17727, 0.625256, 1.15728, -0.9628, 0.2702, -0,
		-1.17706, 0.626014, 1.16007, -0.9628, 0.2702, -0,
		-1.17485, 0.633901, 1.15512, -0.9628, 0.2702, -0,
		-0.509314, 0.447104, 1.15512, 0.9628, -0.2702, -0,
		-0.509155, 0.447673, 1.15721, 0.9628, -0.2702, -0,
		-0.511528, 0.439218, 1.16007, 0.9628, -0.2702, -0,
		-0.509314, 0.447104, 1.15512, 0.9628, -0.2702, -0,
		-0.511528, 0.439218, 1.16007, 0.9628, -0.2702, -0,
		-0.51174, 0.43846, 1.15728, 0.9628, -0.2702, -0,
		-1.17727, 0.625256, 1.15728, -0.2628, -0.9362, 0.2334,
		-0.511528, 0.439218, 1.16007, -0.257, -0.9157, 0.309,
		-1.17706, 0.626014, 1.16007, -0.257, -0.9157, 0.309,
		-0.511528, 0.439218, 1.16007, -0.257, -0.9157, 0.309,
		-1.17727, 0.625256, 1.15728, -0.2628, -0.9362, 0.2334,
		-0.51174, 0.43846, 1.15728, -0.2628, -0.9362, 0.2334,
		-1.17469, 0.634469, 1.15721, 0.257, 0.9157, -0.309,
		-0.509155, 0.447673, 1.15721, 0.257, 0.9157, -0.309,
		-1.17485, 0.633901, 1.15512, 0.2628, 0.9362, -0.2334,
		-0.509314, 0.447104, 1.15512, 0.2628, 0.9362, -0.2335,
		-1.17485, 0.633901, 1.15512, 0.2628, 0.9362, -0.2334,
		-0.509155, 0.447673, 1.15721, 0.257, 0.9157, -0.309,
		-1.17679, 0.62698, 1.16279, -0.9628, 0.2702, -0,
		-1.17449, 0.635194, 1.15925, -0.9628, 0.2702, -0,
		-1.17469, 0.634469, 1.15721, -0.9628, 0.2702, -0,
		-1.17706, 0.626014, 1.16007, -0.9628, 0.2702, -0,
		-1.17679, 0.62698, 1.16279, -0.9628, 0.2702, -0,
		-1.17469, 0.634469, 1.15721, -0.9628, 0.2702, -0,
		-0.509155, 0.447673, 1.15721, 0.9628, -0.2702, -0,
		-0.508951, 0.448398, 1.15925, 0.9628, -0.2702, -0,
		-0.511256, 0.440184, 1.16279, 0.9628, -0.2702, -0,
		-0.509155, 0.447673, 1.15721, 0.9628, -0.2702, -0,
		-0.511256, 0.440184, 1.16279, 0.9628, -0.2702, -0,
		-0.511528, 0.439218, 1.16007, 0.9628, -0.2702, -0,
		-1.17706, 0.626014, 1.16007, -0.257, -0.9157, 0.309,
		-0.511256, 0.440184, 1.16279, -0.2497, -0.8895, 0.3827,
		-1.17679, 0.62698, 1.16279, -0.2497, -0.8895, 0.3827,
		-0.511256, 0.440184, 1.16279, -0.2497, -0.8895, 0.3827,
		-1.17706, 0.626014, 1.16007, -0.257, -0.9157, 0.309,
		-0.511528, 0.439218, 1.16007, -0.257, -0.9157, 0.309,
		-1.17449, 0.635194, 1.15925, 0.2497, 0.8895, -0.3827,
		-0.508951, 0.448398, 1.15925, 0.2497, 0.8895, -0.3827,
		-1.17469, 0.634469, 1.15721, 0.257, 0.9157, -0.309,
		-0.509155, 0.447673, 1.15721, 0.257, 0.9157, -0.309,
		-1.17469, 0.634469, 1.15721, 0.257, 0.9157, -0.309,
		-0.508951, 0.448398, 1.15925, 0.2497, 0.8895, -0.3827,
		-1.17646, 0.62815, 1.16542, -0.9628, 0.2702, -0,
		-1.17424, 0.636071, 1.16123, -0.9628, 0.2702, -0,
		-1.17449, 0.635194, 1.15925, -0.9628, 0.2702, -0,
		-1.17679, 0.62698, 1.16279, -0.9628, 0.2702, -0,
		-1.17646, 0.62815, 1.16542, -0.9628, 0.2702, -0,
		-1.17449, 0.635194, 1.15925, -0.9628, 0.2702, -0,
		-0.508951, 0.448398, 1.15925, 0.9628, -0.2702, -0,
		-0.508705, 0.449274, 1.16123, 0.9628, -0.2702, -0,
		-0.510928, 0.441353, 1.16542, 0.9628, -0.2702, -0,
		-0.508951, 0.448398, 1.15925, 0.9628, -0.2702, -0,
		-0.510928, 0.441353, 1.16542, 0.9628, -0.2702, -0,
		-0.511256, 0.440184, 1.16279, 0.9628, -0.2702, -0,
		-1.17679, 0.62698, 1.16279, -0.2497, -0.8895, 0.3827,
		-0.510928, 0.441353, 1.16542, -0.2408, -0.8579, 0.454,
		-1.17646, 0.62815, 1.16542, -0.2408, -0.8579, 0.454,
		-0.510928, 0.441353, 1.16542, -0.2408, -0.8579, 0.454,
		-1.17679, 0.62698, 1.16279, -0.2497, -0.8895, 0.3827,
		-0.511256, 0.440184, 1.16279, -0.2497, -0.8895, 0.3827,
		-1.17424, 0.636071, 1.16123, 0.2408, 0.8579, -0.454,
		-0.508705, 0.449274, 1.16123, 0.2408, 0.8579, -0.454,
		-1.17449, 0.635194, 1.15925, 0.2497, 0.8895, -0.3827,
		-0.508951, 0.448398, 1.15925, 0.2497, 0.8895, -0.3827,
		-1.17449, 0.635194, 1.15925, 0.2497, 0.8895, -0.3827,
		-0.508705, 0.449274, 1.16123, 0.2408, 0.8579, -0.454,
		-1.17608, 0.629514, 1.16795, -0.9628, 0.2702, -0,
		-1.17395, 0.637094, 1.16313, -0.9628, 0.2702, -0,
		-1.17424, 0.636071, 1.16123, -0.9628, 0.2702, -0,
		-1.17646, 0.62815, 1.16542, -0.9628, 0.2702, -0,
		-1.17608, 0.629514, 1.16795, -0.9628, 0.2702, -0,
		-1.17424, 0.636071, 1.16123, -0.9628, 0.2702, -0,
		-0.508705, 0.449274, 1.16123, 0.9628, -0.2702, -0,
		-0.508418, 0.450298, 1.16313, 0.9628, -0.2702, -0,
		-0.510545, 0.442717, 1.16795, 0.9628, -0.2702, -0,
		-0.508705, 0.449274, 1.16123, 0.9628, -0.2702, -0,
		-0.510545, 0.442717, 1.16795, 0.9628, -0.2702, -0,
		-0.510928, 0.441353, 1.16542, 0.9628, -0.2702, -0,
		-1.17646, 0.62815, 1.16542, -0.2408, -0.8579, 0.454,
		-0.510545, 0.442717, 1.16795, -0.2304, -0.8209, 0.5225,
		-1.17608, 0.629514, 1.16795, -0.2304, -0.8209, 0.5225,
		-0.510545, 0.442717, 1.16795, -0.2304, -0.8209, 0.5225,
		-1.17646, 0.62815, 1.16542, -0.2408, -0.8579, 0.454,
		-0.510928, 0.441353, 1.16542, -0.2408, -0.8579, 0.454,
		-1.17395, 0.637094, 1.16313, 0.2304, 0.8209, -0.5225,
		-0.508418, 0.450298, 1.16313, 0.2304, 0.8209, -0.5225,
		-1.17424, 0.636071, 1.16123, 0.2408, 0.8579, -0.454,
		-0.508705, 0.449274, 1.16123, 0.2408, 0.8579, -0.454,
		-1.17424, 0.636071, 1.16123, 0.2408, 0.8579, -0.454,
		-0.508418, 0.450298, 1.16313, 0.2304, 0.8209, -0.5225,
		-1.17564, 0.631065, 1.17036, -0.9628, 0.2702, -0,
		-1.17363, 0.638257, 1.16494, -0.9628, 0.2702, -0,
		-1.17395, 0.637094, 1.16313, -0.9628, 0.2702, -0,
		-1.17608, 0.629514, 1.16795, -0.9628, 0.2702, -0,
		-1.17564, 0.631065, 1.17036, -0.9628, 0.2702, -0,
		-1.17395, 0.637094, 1.16313, -0.9628, 0.2702, -0,
		-0.508418, 0.450298, 1.16313, 0.9628, -0.2702, -0,
		-0.508091, 0.451461, 1.16494, 0.9628, -0.2702, -0,
		-0.51011, 0.444269, 1.17036, 0.9628, -0.2702, -0,
		-0.508418, 0.450298, 1.16313, 0.9628, -0.2702, -0,
		-0.51011, 0.444269, 1.17036, 0.9628, -0.2702, -0,
		-0.510545, 0.442717, 1.16795, 0.9628, -0.2702, -0,
		-1.17608, 0.629514, 1.16795, -0.2304, -0.8209, 0.5225,
		-0.51011, 0.444269, 1.17036, -0.2186, -0.7789, 0.5878,
		-1.17564, 0.631065, 1.17036, -0.2186, -0.7789, 0.5878,
		-0.51011, 0.444269, 1.17036, -0.2186, -0.7789, 0.5878,
		-1.17608, 0.629514, 1.16795, -0.2304, -0.8209, 0.5225,
		-0.510545, 0.442717, 1.16795, -0.2304, -0.8209, 0.5225,
		-1.17363, 0.638257, 1.16494, 0.2186, 0.7789, -0.5878,
		-0.508091, 0.451461, 1.16494, 0.2186, 0.7789, -0.5878,
		-1.17395, 0.637094, 1.16313, 0.2304, 0.8209, -0.5225,
		-0.508418, 0.450298, 1.16313, 0.2304, 0.8209, -0.5225,
		-1.17395, 0.637094, 1.16313, 0.2304, 0.8209, -0.5225,
		-0.508091, 0.451461, 1.16494, 0.2186, 0.7789, -0.5878,
		-1.17516, 0.632794, 1.17264, -0.9628, 0.2702, -0,
		-1.17326, 0.639554, 1.16664, -0.9628, 0.2702, -0,
		-1.17363, 0.638257, 1.16494, -0.9628, 0.2702, -0,
		-1.17564, 0.631065, 1.17036, -0.9628, 0.2702, -0,
		-1.17516, 0.632794, 1.17264, -0.9628, 0.2702, -0,
		-1.17363, 0.638257, 1.16494, -0.9628, 0.2702, -0,
		-0.508091, 0.451461, 1.16494, 0.9628, -0.2702, -0,
		-0.507728, 0.452757, 1.16664, 0.9628, -0.2702, -0,
		-0.509625, 0.445997, 1.17264, 0.9628, -0.2702, -0,
		-0.508091, 0.451461, 1.16494, 0.9628, -0.2702, -0,
		-0.509625, 0.445997, 1.17264, 0.9628, -0.2702, -0,
		-0.51011, 0.444269, 1.17036, 0.9628, -0.2702, -0,
		-1.17564, 0.631065, 1.17036, -0.2186, -0.7789, 0.5878,
		-0.509625, 0.445997, 1.17264, -0.2055, -0.7321, 0.6494,
		-1.17516, 0.632794, 1.17264, -0.2055, -0.7321, 0.6494,
		-0.509625, 0.445997, 1.17264, -0.2055, -0.7321, 0.6494,
		-1.17564, 0.631065, 1.17036, -0.2186, -0.7789, 0.5878,
		-0.51011, 0.444269, 1.17036, -0.2186, -0.7789, 0.5878,
		-1.17326, 0.639554, 1.16664, 0.2055, 0.7321, -0.6494,
		-0.507728, 0.452757, 1.16664, 0.2055, 0.7321, -0.6494,
		-1.17363, 0.638257, 1.16494, 0.2186, 0.7789, -0.5878,
		-0.508091, 0.451461, 1.16494, 0.2186, 0.7789, -0.5878,
		-1.17363, 0.638257, 1.16494, 0.2186, 0.7789, -0.5878,
		-0.507728, 0.452757, 1.16664, 0.2055, 0.7321, -0.6494,
		-1.17463, 0.634689, 1.17477, -0.9628, 0.2702, -0,
		-1.17286, 0.640975, 1.16824, -0.9628, 0.2702, -0,
		-1.17326, 0.639554, 1.16664, -0.9628, 0.2702, -0,
		-1.17516, 0.632794, 1.17264, -0.9628, 0.2702, -0,
		-1.17463, 0.634689, 1.17477, -0.9628, 0.2702, -0,
		-1.17326, 0.639554, 1.16664, -0.9628, 0.2702, -0,
		-0.507728, 0.452757, 1.16664, 0.9628, -0.2702, -0,
		-0.507329, 0.454179, 1.16824, 0.9628, -0.2702, -0,
		-0.509093, 0.447893, 1.17477, 0.9628, -0.2702, -0,
		-0.507728, 0.452757, 1.16664, 0.9628, -0.2702, -0,
		-0.509093, 0.447893, 1.17477, 0.9628, -0.2702, -0,
		-0.509625, 0.445997, 1.17264, 0.9628, -0.2702, -0,
		-1.17516, 0.632794, 1.17264, -0.2055, -0.7321, 0.6494,
		-0.509093, 0.447893, 1.17477, -0.1911, -0.6808, 0.7071,
		-1.17463, 0.634689, 1.17477, -0.1911, -0.6808, 0.7071,
		-0.509093, 0.447893, 1.17477, -0.1911, -0.6808, 0.7071,
		-1.17516, 0.632794, 1.17264, -0.2055, -0.7321, 0.6494,
		-0.509625, 0.445997, 1.17264, -0.2055, -0.7321, 0.6494,
		-1.17286, 0.640975, 1.16824, 0.1911, 0.6808, -0.7071,
		-0.507329, 0.454179, 1.16824, 0.1911, 0.6808, -0.7071,
		-1.17326, 0.639554, 1.16664, 0.2055, 0.7321, -0.6494,
		-0.507728, 0.452757, 1.16664, 0.2055, 0.7321, -0.6494,
		-1.17326, 0.639554, 1.16664, 0.2055, 0.7321, -0.6494,
		-0.507329, 0.454179, 1.16824, 0.1911, 0.6808, -0.7071,
		-1.17405, 0.636739, 1.17674, -0.9628, 0.2702, -0,
		-1.17243, 0.642513, 1.16972, -0.9628, 0.2702, -0,
		-1.17286, 0.640975, 1.16824, -0.9628, 0.2702, -0,
		-1.17463, 0.634689, 1.17477, -0.9628, 0.2702, -0,
		-1.17405, 0.636739, 1.17674, -0.9628, 0.2702, -0,
		-1.17286, 0.640975, 1.16824, -0.9628, 0.2702, -0,
		-0.507329, 0.454179, 1.16824, 0.9628, -0.2702, -0,
		-0.506897, 0.455717, 1.16972, 0.9628, -0.2702, -0,
		-0.508517, 0.449943, 1.17674, 0.9628, -0.2702, -0,
		-0.507329, 0.454179, 1.16824, 0.9628, -0.2702, -0,
		-0.508517, 0.449943, 1.17674, 0.9628, -0.2702, -0,
		-0.509093, 0.447893, 1.17477, 0.9628, -0.2702, -0,
		-1.17463, 0.634689, 1.17477, -0.1911, -0.6808, 0.7071,
		-0.508517, 0.449943, 1.17674, -0.1755, -0.6253, 0.7604,
		-1.17405, 0.636739, 1.17674, -0.1755, -0.6253, 0.7604,
		-0.508517, 0.449943, 1.17674, -0.1755, -0.6253, 0.7604,
		-1.17463, 0.634689, 1.17477, -0.1911, -0.6808, 0.7071,
		-0.509093, 0.447893, 1.17477, -0.1911, -0.6808, 0.7071,
		-1.17243, 0.642513, 1.16972, 0.1755, 0.6253, -0.7604,
		-0.506897, 0.455717, 1.16972, 0.1755, 0.6253, -0.7604,
		-1.17286, 0.640975, 1.16824, 0.1911, 0.6808, -0.7071,
		-0.507329, 0.454179, 1.16824, 0.1911, 0.6808, -0.7071,
		-1.17286, 0.640975, 1.16824, 0.1911, 0.6808, -0.7071,
		-0.506897, 0.455717, 1.16972, 0.1755, 0.6253, -0.7604,
		-1.17344, 0.638932, 1.17853, -0.9628, 0.2702, -0,
		-1.17197, 0.644158, 1.17106, -0.9628, 0.2702, -0,
		-1.17243, 0.642513, 1.16972, -0.9628, 0.2702, -0,
		-1.17405, 0.636739, 1.17674, -0.9628, 0.2702, -0,
		-1.17344, 0.638932, 1.17853, -0.9628, 0.2702, -0,
		-1.17243, 0.642513, 1.16972, -0.9628, 0.2702, -0,
		-0.506897, 0.455717, 1.16972, 0.9628, -0.2702, -0,
		-0.506435, 0.457361, 1.17106, 0.9628, -0.2702, -0,
		-0.507902, 0.452136, 1.17853, 0.9628, -0.2702, -0,
		-0.506897, 0.455717, 1.16972, 0.9628, -0.2702, -0,
		-0.507902, 0.452136, 1.17853, 0.9628, -0.2702, -0,
		-0.508517, 0.449943, 1.17674, 0.9628, -0.2702, -0,
		-1.17405, 0.636739, 1.17674, -0.1755, -0.6253, 0.7604,
		-0.507902, 0.452136, 1.17853, -0.1588, -0.5659, 0.809,
		-1.17344, 0.638932, 1.17853, -0.1588, -0.5659, 0.809,
		-0.507902, 0.452136, 1.17853, -0.1588, -0.5659, 0.809,
		-1.17405, 0.636739, 1.17674, -0.1755, -0.6253, 0.7604,
		-0.508517, 0.449943, 1.17674, -0.1755, -0.6253, 0.7604,
		-1.17197, 0.644158, 1.17106, 0.1588, 0.5659, -0.809,
		-0.506435, 0.457361, 1.17106, 0.1588, 0.5659, -0.809,
		-1.17243, 0.642513, 1.16972, 0.1755, 0.6253, -0.7604,
		-0.506897, 0.455717, 1.16972, 0.1755, 0.6253, -0.7604,
		-1.17243, 0.642513, 1.16972, 0.1755, 0.6253, -0.7604,
		-0.506435, 0.457361, 1.17106, 0.1588, 0.5659, -0.809,
		-1.17279, 0.641254, 1.18015, -0.9628, 0.2702, -0,
		-1.17148, 0.645899, 1.17227, -0.9628, 0.2702, -0,
		-1.17197, 0.644158, 1.17106, -0.9628, 0.2702, -0,
		-1.17344, 0.638932, 1.17853, -0.9628, 0.2702, -0,
		-1.17279, 0.641254, 1.18015, -0.9628, 0.2702, -0,
		-1.17197, 0.644158, 1.17106, -0.9628, 0.2702, -0,
		-0.506435, 0.457361, 1.17106, 0.9628, -0.2702, -0,
		-0.505947, 0.459103, 1.17227, 0.9628, -0.2702, -0,
		-0.50725, 0.454457, 1.18015, 0.9628, -0.2702, -0,
		-0.506435, 0.457361, 1.17106, 0.9628, -0.2702, -0,
		-0.50725, 0.454457, 1.18015, 0.9628, -0.2702, -0,
		-0.507902, 0.452136, 1.17853, 0.9628, -0.2702, -0,
		-1.17344, 0.638932, 1.17853, -0.1588, -0.5659, 0.809,
		-0.50725, 0.454457, 1.18015, -0.1412, -0.5031, 0.8526,
		-1.17279, 0.641254, 1.18015, -0.1412, -0.5031, 0.8526,
		-0.50725, 0.454457, 1.18015, -0.1412, -0.5031, 0.8526,
		-1.17344, 0.638932, 1.17853, -0.1588, -0.5659, 0.809,
		-0.507902, 0.452136, 1.17853, -0.1588, -0.5659, 0.809,
		-1.17148, 0.645899, 1.17227, 0.1412, 0.5031, -0.8526,
		-0.505947, 0.459103, 1.17227, 0.1412, 0.5031, -0.8526,
		-1.17197, 0.644158, 1.17106, 0.1588, 0.5659, -0.809,
		-0.506435, 0.457361, 1.17106, 0.1588, 0.5659, -0.809,
		-1.17197, 0.644158, 1.17106, 0.1588, 0.5659, -0.809,
		-0.505947, 0.459103, 1.17227, 0.1412, 0.5031, -0.8526,
		-1.1721, 0.64369, 1.18156, -0.9628, 0.2702, -0,
		-1.17097, 0.647726, 1.17333, -0.9628, 0.2702, -0,
		-1.17148, 0.645899, 1.17227, -0.9628, 0.2702, -0,
		-1.17279, 0.641254, 1.18015, -0.9628, 0.2702, -0,
		-1.1721, 0.64369, 1.18156, -0.9628, 0.2702, -0,
		-1.17148, 0.645899, 1.17227, -0.9628, 0.2702, -0,
		-0.505947, 0.459103, 1.17227, 0.9628, -0.2702, -0,
		-0.505434, 0.46093, 1.17333, 0.9628, -0.2702, -0,
		-0.506567, 0.456894, 1.18156, 0.9628, -0.2702, -0,
		-0.505947, 0.459103, 1.17227, 0.9628, -0.2702, -0,
		-0.506567, 0.456894, 1.18156, 0.9628, -0.2702, -0,
		-0.50725, 0.454457, 1.18015, 0.9628, -0.2702, -0,
		-1.17279, 0.641254, 1.18015, -0.1412, -0.5031, 0.8526,
		-0.506567, 0.456894, 1.18156, -0.1227, -0.4371, 0.891,
		-1.1721, 0.64369, 1.18156, -0.1227, -0.4371, 0.891,
		-0.506567, 0.456894, 1.18156, -0.1227, -0.4371, 0.891,
		-1.17279, 0.641254, 1.18015, -0.1412, -0.5031, 0.8526,
		-0.50725, 0.454457, 1.18015, -0.1412, -0.5031, 0.8526,
		-1.17097, 0.647726, 1.17333, 0.1227, 0.4371, -0.891,
		-0.505434, 0.46093, 1.17333, 0.1227, 0.4371, -0.891,
		-1.17148, 0.645899, 1.17227, 0.1412, 0.5031, -0.8526,
		-0.505947, 0.459103, 1.17227, 0.1412, 0.5031, -0.8526,
		-1.17148, 0.645899, 1.17227, 0.1412, 0.5031, -0.8526,
		-0.505434, 0.46093, 1.17333, 0.1227, 0.4371, -0.891,
		-1.17139, 0.646226, 1.18278, -0.9628, 0.2702, -0,
		-1.17043, 0.649628, 1.17425, -0.9628, 0.2702, -0,
		-1.17097, 0.647726, 1.17333, -0.9628, 0.2702, -0,
		-1.1721, 0.64369, 1.18156, -0.9628, 0.2702, -0,
		-1.17139, 0.646226, 1.18278, -0.9628, 0.2702, -0,
		-1.17097, 0.647726, 1.17333, -0.9628, 0.2702, -0,
		-0.505434, 0.46093, 1.17333, 0.9628, -0.2702, -0,
		-0.5049, 0.462831, 1.17425, 0.9628, -0.2702, -0,
		-0.505855, 0.459429, 1.18278, 0.9628, -0.2702, -0,
		-0.505434, 0.46093, 1.17333, 0.9628, -0.2702, -0,
		-0.505855, 0.459429, 1.18278, 0.9628, -0.2702, -0,
		-0.506567, 0.456894, 1.18156, 0.9628, -0.2702, -0,
		-1.1721, 0.64369, 1.18156, -0.1227, -0.4371, 0.891,
		-0.505855, 0.459429, 1.18278, -0.1034, -0.3684, 0.9239,
		-1.17139, 0.646226, 1.18278, -0.1034, -0.3684, 0.9239,
		-0.505855, 0.459429, 1.18278, -0.1034, -0.3684, 0.9239,
		-1.1721, 0.64369, 1.18156, -0.1227, -0.4371, 0.891,
		-0.506567, 0.456894, 1.18156, -0.1227, -0.4371, 0.891,
		-1.17043, 0.649628, 1.17425, 0.1034, 0.3684, -0.9239,
		-0.5049, 0.462831, 1.17425, 0.1034, 0.3684, -0.9239,
		-1.17097, 0.647726, 1.17333, 0.1227, 0.4371, -0.891,
		-0.505434, 0.46093, 1.17333, 0.1227, 0.4371, -0.891,
		-1.17097, 0.647726, 1.17333, 0.1227, 0.4371, -0.891,
		-0.5049, 0.462831, 1.17425, 0.1034, 0.3684, -0.9239,
		-1.17065, 0.648845, 1.18378, -0.9628, 0.2702, -0,
		-1.16988, 0.651593, 1.175, -0.9628, 0.2702, -0,
		-1.17043, 0.649628, 1.17425, -0.9628, 0.2702, -0,
		-1.17139, 0.646226, 1.18278, -0.9628, 0.2702, -0,
		-1.17065, 0.648845, 1.18378, -0.9628, 0.2702, -0,
		-1.17043, 0.649628, 1.17425, -0.9628, 0.2702, -0,
		-0.5049, 0.462831, 1.17425, 0.9628, -0.2702, -0,
		-0.504349, 0.464796, 1.175, 0.9628, -0.2702, -0,
		-0.50512, 0.462049, 1.18378, 0.9628, -0.2702, -0,
		-0.5049, 0.462831, 1.17425, 0.9628, -0.2702, -0,
		-0.50512, 0.462049, 1.18378, 0.9628, -0.2702, -0,
		-0.505855, 0.459429, 1.18278, 0.9628, -0.2702, -0,
		-1.17139, 0.646226, 1.18278, -0.1034, -0.3684, 0.9239,
		-0.50512, 0.462049, 1.18378, -0.0835, -0.2975, 0.9511,
		-1.17065, 0.648845, 1.18378, -0.0835, -0.2975, 0.9511,
		-0.50512, 0.462049, 1.18378, -0.0835, -0.2975, 0.9511,
		-1.17139, 0.646226, 1.18278, -0.1034, -0.3684, 0.9239,
		-0.505855, 0.459429, 1.18278, -0.1034, -0.3684, 0.9239,
		-1.16988, 0.651593, 1.175, 0.0835, 0.2975, -0.9511,
		-0.504349, 0.464796, 1.175, 0.0835, 0.2975, -0.9511,
		-1.17043, 0.649628, 1.17425, 0.1034, 0.3684, -0.9239,
		-0.5049, 0.462831, 1.17425, 0.1034, 0.3684, -0.9239,
		-1.17043, 0.649628, 1.17425, 0.1034, 0.3684, -0.9239,
		-0.504349, 0.464796, 1.175, 0.0835, 0.2975, -0.9511,
		-1.1699, 0.651533, 1.18457, -0.9628, 0.2702, -0,
		-1.16932, 0.653608, 1.17559, -0.9628, 0.2702, -0,
		-1.16988, 0.651593, 1.175, -0.9628, 0.2702, -0,
		-1.17065, 0.648845, 1.18378, -0.9628, 0.2702, -0,
		-1.1699, 0.651533, 1.18457, -0.9628, 0.2702, -0,
		-1.16988, 0.651593, 1.175, -0.9628, 0.2702, -0,
		-0.504349, 0.464796, 1.175, 0.9628, -0.2702, -0,
		-0.503783, 0.466812, 1.17559, 0.9628, -0.2702, -0,
		-0.504365, 0.464736, 1.18457, 0.9628, -0.2702, -0,
		-0.504349, 0.464796, 1.175, 0.9628, -0.2702, -0,
		-0.504365, 0.464736, 1.18457, 0.9628, -0.2702, -0,
		-0.50512, 0.462049, 1.18378, 0.9628, -0.2702, -0,
		-1.17065, 0.648845, 1.18378, -0.0835, -0.2975, 0.9511,
		-0.504365, 0.464736, 1.18457, -0.0631, -0.2248, 0.9724,
		-1.1699, 0.651533, 1.18457, -0.0631, -0.2248, 0.9724,
		-0.504365, 0.464736, 1.18457, -0.0631, -0.2248, 0.9724,
		-1.17065, 0.648845, 1.18378, -0.0835, -0.2975, 0.9511,
		-0.50512, 0.462049, 1.18378, -0.0835, -0.2975, 0.9511,
		-1.16932, 0.653608, 1.17559, 0.0631, 0.2248, -0.9724,
		-0.503783, 0.466812, 1.17559, 0.0631, 0.2248, -0.9724,
		-1.16988, 0.651593, 1.175, 0.0835, 0.2975, -0.9511,
		-0.504349, 0.464796, 1.175, 0.0835, 0.2975, -0.9511,
		-1.16988, 0.651593, 1.175, 0.0835, 0.2975, -0.9511,
		-0.503783, 0.466812, 1.17559, 0.0631, 0.2248, -0.9724,
		-1.16913, 0.654271, 1.18513, -0.9628, 0.2702, -0,
		-1.16874, 0.655662, 1.17601, -0.9628, 0.2702, -0,
		-1.16932, 0.653608, 1.17559, -0.9628, 0.2702, -0,
		-1.1699, 0.651533, 1.18457, -0.9628, 0.2702, -0,
		-1.16913, 0.654271, 1.18513, -0.9628, 0.2702, -0,
		-1.16932, 0.653608, 1.17559, -0.9628, 0.2702, -0,
		-0.503783, 0.466812, 1.17559, 0.9628, -0.2702, -0,
		-0.503206, 0.468866, 1.17601, 0.9628, -0.2702, -0,
		-0.503597, 0.467475, 1.18513, 0.9628, -0.2702, -0,
		-0.503783, 0.466812, 1.17559, 0.9628, -0.2702, -0,
		-0.503597, 0.467475, 1.18513, 0.9628, -0.2702, -0,
		-0.504365, 0.464736, 1.18457, 0.9628, -0.2702, -0,
		-1.1699, 0.651533, 1.18457, -0.0631, -0.2248, 0.9724,
		-0.503597, 0.467475, 1.18513, -0.0423, -0.1506, 0.9877,
		-1.16913, 0.654271, 1.18513, -0.0423, -0.1506, 0.9877,
		-0.503597, 0.467475, 1.18513, -0.0423, -0.1506, 0.9877,
		-1.1699, 0.651533, 1.18457, -0.0631, -0.2248, 0.9724,
		-0.504365, 0.464736, 1.18457, -0.0631, -0.2248, 0.9724,
		-1.16874, 0.655662, 1.17601, 0.0423, 0.1506, -0.9877,
		-0.503206, 0.468866, 1.17601, 0.0423, 0.1506, -0.9877,
		-1.16932, 0.653608, 1.17559, 0.0631, 0.2248, -0.9724,
		-0.503783, 0.466812, 1.17559, 0.0631, 0.2248, -0.9724,
		-1.16932, 0.653608, 1.17559, 0.0631, 0.2248, -0.9724,
		-0.503206, 0.468866, 1.17601, 0.0423, 0.1506, -0.9877,
		-1.16835, 0.657044, 1.18547, -0.9628, 0.2702, -0,
		-1.16816, 0.657742, 1.17627, -0.9628, 0.2702, -0,
		-1.16874, 0.655662, 1.17601, -0.9628, 0.2702, -0,
		-1.16913, 0.654271, 1.18513, -0.9628, 0.2702, -0,
		-1.16835, 0.657044, 1.18547, -0.9628, 0.2702, -0,
		-1.16874, 0.655662, 1.17601, -0.9628, 0.2702, -0,
		-0.503206, 0.468866, 1.17601, 0.9628, -0.2702, -0,
		-0.502623, 0.470945, 1.17627, 0.9628, -0.2702, -0,
		-0.502818, 0.470248, 1.18547, 0.9628, -0.2702, -0,
		-0.503206, 0.468866, 1.17601, 0.9628, -0.2702, -0,
		-0.502818, 0.470248, 1.18547, 0.9628, -0.2702, -0,
		-0.503597, 0.467475, 1.18513, 0.9628, -0.2702, -0,
		-1.16913, 0.654271, 1.18513, -0.0423, -0.1506, 0.9877,
		-0.502818, 0.470248, 1.18547, -0.0212, -0.0755, 0.9969,
		-1.16835, 0.657044, 1.18547, -0.0212, -0.0755, 0.9969,
		-0.502818, 0.470248, 1.18547, -0.0212, -0.0755, 0.9969,
		-1.16913, 0.654271, 1.18513, -0.0423, -0.1506, 0.9877,
		-0.503597, 0.467475, 1.18513, -0.0423, -0.1506, 0.9877,
		-1.16816, 0.657742, 1.17627, 0.0212, 0.0755, -0.9969,
		-0.502623, 0.470945, 1.17627, 0.0212, 0.0755, -0.9969,
		-1.16874, 0.655662, 1.17601, 0.0423, 0.1506, -0.9877,
		-0.503206, 0.468866, 1.17601, 0.0423, 0.1506, -0.9877,
		-1.16874, 0.655662, 1.17601, 0.0423, 0.1506, -0.9877,
		-0.502623, 0.470945, 1.17627, 0.0212, 0.0755, -0.9969,
		-1.16757, 0.659834, 1.18559, -0.9628, 0.2702, -0,
		-1.16757, 0.659834, 1.17635, -0.9628, 0.2702, -0,
		-1.16816, 0.657742, 1.17627, -0.9628, 0.2702, -0,
		-1.16835, 0.657044, 1.18547, -0.9628, 0.2702, -0,
		-1.16757, 0.659834, 1.18559, -0.9628, 0.2702, -0,
		-1.16816, 0.657742, 1.17627, -0.9628, 0.2702, -0,
		-0.502623, 0.470945, 1.17627, 0.9628, -0.2702, -0,
		-0.502035, 0.473038, 1.17635, 0.9628, -0.2702, -0,
		-0.502035, 0.473038, 1.18559, 0.9628, -0.2702, -0,
		-0.502623, 0.470945, 1.17627, 0.9628, -0.2702, -0,
		-0.502035, 0.473038, 1.18559, 0.9628, -0.2702, -0,
		-0.502818, 0.470248, 1.18547, 0.9628, -0.2702, -0,
		-1.16835, 0.657044, 1.18547, -0.0212, -0.0755, 0.9969,
		-0.502035, 0.473038, 1.18559, -0, -0, 1,
		-1.16757, 0.659834, 1.18559, -0, -0, 1,
		-0.502035, 0.473038, 1.18559, -0, -0, 1,
		-1.16835, 0.657044, 1.18547, -0.0212, -0.0755, 0.9969,
		-0.502818, 0.470248, 1.18547, -0.0212, -0.0755, 0.9969,
		-1.16757, 0.659834, 1.17635, -0, -0, -1,
		-0.502035, 0.473038, 1.17635, -0, -0, -1,
		-1.16816, 0.657742, 1.17627, 0.0212, 0.0755, -0.9969,
		-0.502623, 0.470945, 1.17627, 0.0212, 0.0755, -0.9969,
		-1.16816, 0.657742, 1.17627, 0.0212, 0.0755, -0.9969,
		-0.502035, 0.473038, 1.17635, -0, -0, -1,
	};
	float * tank6 = new float[36 * 6]
	{
		-0.487355, 0.567722, 1.21622, 1, -0, -0,
		-0.487355, 0.444923, 1.0792, 1, -0, -0,
		-0.487355, 0.567722, 1.0792, 1, -0, -0,
		-0.487355, 0.567722, 1.21622, 1, -0, -0,
		-0.487355, 0.444923, 1.21622, 1, -0, -0,
		-0.487355, 0.444923, 1.0792, 1, -0, -0,
		-0.610153, 0.567722, 1.0792, -1, -0, -0,
		-0.610153, 0.444923, 1.21622, -1, -0, -0,
		-0.610153, 0.567722, 1.21622, -1, -0, -0,
		-0.610153, 0.567722, 1.0792, -1, -0, -0,
		-0.610153, 0.444923, 1.0792, -1, -0, -0,
		-0.610153, 0.444923, 1.21622, -1, -0, -0,
		-0.610153, 0.567722, 1.21622, -0, -0, 1,
		-0.487355, 0.444923, 1.21622, -0, -0, 1,
		-0.487355, 0.567722, 1.21622, -0, -0, 1,
		-0.610153, 0.567722, 1.21622, -0, -0, 1,
		-0.610153, 0.444923, 1.21622, -0, -0, 1,
		-0.487355, 0.444923, 1.21622, -0, -0, 1,
		-0.487355, 0.567722, 1.0792, -0, -0, -1,
		-0.610153, 0.444923, 1.0792, -0, -0, -1,
		-0.610153, 0.567722, 1.0792, -0, -0, -1,
		-0.487355, 0.567722, 1.0792, -0, -0, -1,
		-0.487355, 0.444923, 1.0792, -0, -0, -1,
		-0.610153, 0.444923, 1.0792, -0, -0, -1,
		-0.487355, 0.567722, 1.0792, -0, 1, -0,
		-0.610153, 0.567722, 1.21622, -0, 1, -0,
		-0.487355, 0.567722, 1.21622, -0, 1, -0,
		-0.487355, 0.567722, 1.0792, -0, 1, -0,
		-0.610153, 0.567722, 1.0792, -0, 1, -0,
		-0.610153, 0.567722, 1.21622, -0, 1, -0,
		-0.610153, 0.444923, 1.0792, -0, -1, -0,
		-0.487355, 0.444923, 1.21622, -0, -1, -0,
		-0.610153, 0.444923, 1.21622, -0, -1, -0,
		-0.610153, 0.444923, 1.0792, -0, -1, -0,
		-0.487355, 0.444923, 1.0792, -0, -1, -0,
		-0.487355, 0.444923, 1.21622, -0, -1, -0,

	};

	std::vector<float> tank0Vec(tank0, tank0 + (36 * 6));
	std::vector<float> tank1Vec(tank1, tank1 + (960 * 6));
	std::vector<float> tank2Vec(tank2, tank2 + (24 * 6));
	std::vector<float> tank3Vec(tank3, tank3 + (36 * 6));
	std::vector<float> tank4Vec(tank4, tank4 + (24 * 6));
	std::vector<float> tank5Vec(tank5, tank5 + (1920 * 6));
	std::vector<float> tank6Vec(tank6, tank6 + (36 * 6));
	
	tank.push_back(tank0Vec);
	tank.push_back(tank1Vec);
	tank.push_back(tank2Vec);
	tank.push_back(tank3Vec);
	tank.push_back(tank4Vec);
	tank.push_back(tank5Vec);
	tank.push_back(tank6Vec);

	delete[] tank0;
	delete[] tank1;
	delete[] tank2;
	delete[] tank3;
	delete[] tank4;
	delete[] tank5;
	delete[] tank6;
}

int main(int argc, char** argv)
{
	glfwInit();

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

	int numSphereVerts;
	float* sphere = createCircle(numSphereVerts, false);
	float* invSphere = createCircle(numSphereVerts, true);
	float floor[] = {
			-1.f,  1.f, -1.f,  	0.0f, 0.0f,   0.f, 1.f, 0.f,
			1.f,  1.f, -1.f,  	50.f, 0.0f,   0.f, 1.f, 0.f,
			1.f,  1.f,  1.f,  	50.f, 50.f,   0.f, 1.f, 0.f,
			1.f,  1.f,  1.f,  	50.f, 50.f,   0.f, 1.f, 0.f,
			-1.f,  1.f,  1.f,  	0.0f, 50.f,   0.f, 1.f, 0.f,
			-1.f,  1.f, -1.f, 	0.0f, 0.0f,   0.f, 1.f, 0.f,
	};

	const char* files[6] = {
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
	textures[2] = setup_mipmaps(files, 6, true);
	textures[3] = setup_texture("Assets/sun.bmp", false);
	textures[4] = setup_texture("Assets/14054_Pirate_Ship_Cannon_on_Cart_wheel_diff.bmp", false);
	textures[5] = setup_texture("Assets/14054_Pirate_Ship_Cannon_on_Cart_cart_diff.bmp", false);
	textures[6] = setup_texture("Assets/14054_Pirate_Ship_Cannon_on_Cart_barrel_diff.bmp", false);

	std::vector<std::vector<float>> cannonObject;
	objParser("Assets/pirate_cannon.obj", cannonObject, true);
	
	std::vector<std::vector<float>> lampObject;
	objParser("Assets/Street_Lamp.obj", lampObject, false);

	std::vector<std::vector<float>> tank;
	loadTankVerts(tank);

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
	glNamedBufferStorage(Buffers[index], numSphereVerts * 8 * sizeof(float), sphere, 0);
	glBindVertexArray(VAOs[index]);
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[index]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(2);
	verts.push_back(numSphereVerts * 8);

	// Sky = 1
	index++;
	glNamedBufferStorage(Buffers[index], numSphereVerts * 8 * sizeof(float), invSphere, 0);
	glBindVertexArray(VAOs[index]);
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[index]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(2);
	verts.push_back(numSphereVerts * 8);

	// Floor = 2
	index++;
	glNamedBufferStorage(Buffers[index], 6 * 8 * sizeof(float), floor, 0);
	glBindVertexArray(VAOs[index]);
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[index]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(2);
	verts.push_back(6 * 8);

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

	glEnable(GL_DEPTH_TEST);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

	glm::mat4 lightModel = glm::mat4(1.f);
	lightModel = glm::rotate(lightModel, (float)M_PI / 2, glm::vec3(0.f, 1.f, 0.f));
	lightModel = glm::translate(lightModel, glm::vec3(0.f, 6.5f, -8.6f));
	lightPos[1] = glm::vec3(lightModel * glm::vec4(0.f, 0.f, 0.f, 1.f));
	lightDirection[1] = glm::vec3(-0.05f, -1.f, 0.f);

	while (!glfwWindowShouldClose(window))
	{
		glm::mat4 model = glm::mat4(1.f);
		float angle = (float)glfwGetTime() / 20;
		model = glm::rotate(model, angle, sunRotation);
		model = glm::translate(model, -initSunPos);

		lightPos[0] = glm::vec3(model * glm::vec4(0.f, 0.f, 0.f, -1.f));
		lightDirection[0] = glm::normalize(glm::vec3(0.f, 1.f, 0.f) - lightPos[0]);

		if (cannonBallPos > 0) {
			cannonBallPos += CANNONBALL_SPEED;
		} if (cannonBallPos >= 100) {
			cannonBallPos = -1;
		}

		glm::mat4 projectedLightSpaceMatrix[NUM_LIGHTS];
		for (int lightIndex = 0; lightIndex < NUM_LIGHTS; lightIndex++)
		{
			glm::mat4 lightProjection;
			glm::mat4 lightView;
			// Spotlight and positional
			if (lightType[lightIndex] == 1 || lightType[lightIndex] == 2) {
				float near_plane = 5.f; float far_plane = 50.f;
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

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}