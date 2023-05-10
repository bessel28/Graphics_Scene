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
#include <string.h>

#include <vector>
#define _USE_MATH_DEFINES

#include <math.h>

SCamera camera;

#define NUM_BUFFERS 3
#define NUM_VAOS 3
#define NUMTEXTURES 3
GLuint Buffers[NUM_BUFFERS];
GLuint VAOs[NUM_VAOS];

int WIDTH = 1024;
int HEIGHT = 768;

#define SH_MAP_WIDTH 2048
#define SH_MAP_HEIGHT 2048

#define NUM_LIGHTS 2

glm::vec3 lightDirection[NUM_LIGHTS];
glm::vec3 lightPos[NUM_LIGHTS];

glm::vec3 initSunPos = glm::vec3(0.f, 0.f, 20.f);
glm::vec3 sunRotation = glm::vec3(-2.f, 0.f, 0.f);

int lightType[] = { 0 , 2 };

#define RADIUS 1
#define NUM_SECTORS 30
#define NUM_STACKS 30
#define SPHERE_VERTS (8 * 3 * 2 * NUM_SECTORS * (NUM_STACKS - 1))

int cameraType = 0;

int flashLight = 0;

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
		lightPos[1] = camera.Position;
		lightDirection[1] = camera.Front;
	}
	if (key == GLFW_KEY_LEFT_ALT && action == GLFW_PRESS) {
		lightType[0] = (lightType[0] + 1) % 3;
		printf("Light Type: %d %d\n", lightType[0], lightType[1]);
	}
	if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
		cameraType = (cameraType + 1) % 2;
		if (cameraType == 0) {
			InitCamera(camera);
			cam_dist = 5.f;
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

float* objParser(const char* filename) {
	char* obj = read_file(filename);
	const char* delim = "\n";
	char* saveptr;
	char* saveptr2;
	rsize_t strmax = sizeof obj;

	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> texCoords;
	std::vector<float> object;

	char* line = strtok_s(obj, delim, &saveptr);
	while (line != NULL)
	{
		char* words = strtok_s(line, " ", &saveptr2);
		std::string word(words);
		printf("%s\n", words);
		if (word == "#") {
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
			glm::vec2 v;
			words = strtok_s(NULL, " ", &saveptr2);
			v.x = std::strtof(words, nullptr);
			words = strtok_s(NULL, " ", &saveptr2);
			v.y = std::strtof(words, nullptr);
			texCoords.push_back(v);
		}
		else if (word == "f") {

		}
		line = strtok_s(NULL, delim, &saveptr);
	}
	printf("done");




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

float* createCircle(int& num_Verts)
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
			vertices[index++] = (nx);
			vertices[index++] = (ny);
			vertices[index++] = (nz);
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
	model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	int modelType = 0;
	glUniform1i(glGetUniformLocation(shaderProgram, "modelType"), modelType);

	glDrawArrays(GL_TRIANGLES, 0, SPHERE_VERTS);
}

void drawSky(unsigned int shaderProgram, GLuint skyTex, int index)
{
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
}

void drawSun(unsigned int shaderProgram, GLuint sunTex, int index)
{
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[index]);
	glBindVertexArray(VAOs[index]);
	glBindTexture(GL_TEXTURE_2D, sunTex);

	glm::mat4 model = glm::mat4(1.f);
	model = glm::rotate(model, (float)glfwGetTime() / 20, sunRotation);
	model = glm::translate(model, initSunPos);
	model = glm::translate(model, glm::vec3(5.f) * glm::normalize(initSunPos));
	model = glm::rotate(model, (float)glfwGetTime() / 20, -sunRotation);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	int modelType = 3;
	glUniform1i(glGetUniformLocation(shaderProgram, "modelType"), modelType);

	glDrawArrays(GL_TRIANGLES, 0, SPHERE_VERTS);
}

void drawObjects(unsigned int shaderProgram, GLuint* textures) {
	drawCannonball(shaderProgram, textures[0], 0);
	drawSky(shaderProgram, textures[1], 0);
	drawFloor(shaderProgram, textures[2], 1);
	drawSun(shaderProgram, textures[3], 0);
}

void renderWithShadow(unsigned int renderShaderProgram, ShadowStruct shadow[NUM_LIGHTS], glm::mat4 projectedLightSpaceMatrix[], GLuint* textures)
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

	drawObjects(renderShaderProgram, textures);
}

void generateDepthMap(unsigned int shadowShaderProgram, ShadowStruct shadow[NUM_LIGHTS], glm::mat4 projectedLightSpaceMatrix[], GLuint* textures)
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

		drawObjects(shadowShaderProgram, textures);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int main(int argc, char** argv)
{
	glfwInit();

	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Assignment 3", NULL, NULL);
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetWindowSizeCallback(window, SizeCallback);
	glfwSetCursorPosCallback(window, MouseMoveCallback);

	gl3wInit();

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(DebguMessageCallback, 0);


	GLuint program = CompileShader("phong.vert", "phong.frag");
	GLuint shadowProgram = CompileShader("shadow.vert", "shadow.frag");


	ShadowStruct shadow[NUM_LIGHTS];
	for (int i = 0; i < NUM_LIGHTS; i++) {
		shadow[i] = setup_shadowmap(SH_MAP_WIDTH, SH_MAP_HEIGHT);
	}

	int numSphereVerts;
	float* sphere = createCircle(numSphereVerts);
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

	GLuint cannonballTex = setup_texture("Assets/cannonball.bmp", false);
	GLuint skyTex = setup_texture("Assets/sky_texture2.bmp", false);
	GLuint floorTex = setup_mipmaps(files, 6, true);
	GLuint sunTex = setup_texture("Assets/sun.bmp", false);

	GLuint* textures = (GLuint*)calloc(NUMTEXTURES, sizeof(GLuint));

	textures[0] = cannonballTex;
	textures[1] = skyTex;
	textures[2] = floorTex;
	textures[3] = sunTex;

	objParser("Assets/pirate_cannon.obj");

	InitCamera(camera);
	cam_dist = 10.f;
	MoveAndOrientCamera(camera, glm::vec3(0, 0, 0), cam_dist, 0.f, 0.f);

	// Create Buffers and VAOs
	glCreateBuffers(NUM_BUFFERS, Buffers);
	glGenVertexArrays(NUM_VAOS, VAOs);

	// Cannonball = 0
	// Sky = 0
	// Sun = 0
	glNamedBufferStorage(Buffers[0], numSphereVerts * 8 * sizeof(float), sphere, 0);
	glBindVertexArray(VAOs[0]);
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[0]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(2);

	// Floor = 1
	glNamedBufferStorage(Buffers[1], 6 * 8 * sizeof(float), floor, 0);
	glBindVertexArray(VAOs[1]);
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[1]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, (8 * sizeof(float)), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glEnable(GL_DEPTH_TEST);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

	lightPos[1] = camera.Position;
	lightDirection[1] = camera.Front;
	while (!glfwWindowShouldClose(window))
	{
		glm::mat4 model = glm::mat4(1.f);
		float angle = (float)glfwGetTime() / 20;
		model = glm::rotate(model, angle, sunRotation);
		model = glm::translate(model, -initSunPos);

		lightPos[0] = glm::vec3(model * glm::vec4(0.f, 0.f, 0.f, -1.f));
		lightDirection[0] = glm::normalize(glm::vec3(0.f, 1.f, 0.f) - lightPos[0]);



		glm::mat4 projectedLightSpaceMatrix[NUM_LIGHTS];
		for (int lightIndex = 0; lightIndex < NUM_LIGHTS; lightIndex++)
		{
			glm::mat4 lightProjection;
			glm::mat4 lightView;
			// Spotlight and positional
			if (lightType[lightIndex] == 0 || lightType[lightIndex] == 2) {
				float near_plane = 1.0f; float far_plane = 50.f;
				lightProjection = glm::perspective(glm::radians(45.f), (float)WIDTH / (float)HEIGHT, near_plane, far_plane);
				lightView = glm::lookAt(lightPos[lightIndex], lightPos[lightIndex] + lightDirection[lightIndex], glm::vec3(0.0f, 1.0f, 0.0f));
			}
			// Directional
			else if (lightType[lightIndex] == 1) {
				float near_plane = 1.f, far_plane = 50.f;
				lightProjection = glm::ortho(-10.f, 10.f, -10.f, 10.f, near_plane, far_plane);
				lightView = glm::lookAt(lightPos[lightIndex], lightPos[lightIndex] + lightDirection[lightIndex], glm::vec3(0.0f, 1.0f, 0.f));
			}
			projectedLightSpaceMatrix[lightIndex] = lightProjection * lightView;
		}

		generateDepthMap(shadowProgram, shadow, projectedLightSpaceMatrix, textures);

		//saveShadowMapToBitmap(shadow[1].Texture, SH_MAP_WIDTH, SH_MAP_HEIGHT);

		renderWithShadow(program, shadow, projectedLightSpaceMatrix, textures);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}