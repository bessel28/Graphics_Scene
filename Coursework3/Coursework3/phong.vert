#version 450 core

layout(location = 0) in vec4 vPos;
layout(location = 1) in vec2 vTex;
layout(location = 2) in vec3 vNor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 projectedLightSpaceMatrix[5];

uniform int numLights;

out vec2 tex;
out vec3 nor;
out vec3 FragPosWorldSpace;
out vec4 FragPosProjectedLightSpace[5];

void main()
{
	gl_Position = projection * view * model * vPos;
	tex = vTex;
	nor = mat3(transpose(inverse(model))) * vNor;
	FragPosWorldSpace = vec3(model * vPos);
	for (int i = 0; i < numLights; i++) {
		FragPosProjectedLightSpace[i] = projectedLightSpaceMatrix[i] * model * vPos;
	}
}

