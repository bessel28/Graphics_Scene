#version 450 core

layout (location = 0) out vec4 fColour;

in vec2 tex;
in vec3 nor;
in vec3 FragPosWorldSpace;
in vec4 FragPosProjectedLightSpace[5];

uniform sampler2D shadowMap0;
uniform sampler2D shadowMap1;
uniform sampler2D shadowMap[5];
uniform sampler2D Texture;

uniform int numLights;
uniform vec3 lightDirection[5];
uniform vec3 lightColour;
uniform vec3 lightPos[5];
uniform int lightType[5];

uniform vec3 camPos;

uniform int modelType;

float attConst = 1.f;
float attLin = 0.01;
float attQuad = 0.001f;

float ambient = 0.1f;

sampler2D getShadowMap(int lightIndex) {
	return shadowMap[lightIndex];
	switch (lightIndex) {
		case 0:
			return shadowMap0;
		case 1:
			return shadowMap1;
	}
}

float shadowOnFragment(int lightIndex)
{
	vec3 ndc = FragPosProjectedLightSpace[lightIndex].xyz / FragPosProjectedLightSpace[lightIndex].w;
	vec3 ss = (ndc+1)*0.5;

	float fragDepth = ss.z;

	vec3 Nnor = normalize(nor);
	vec3 NtoLight = normalize(-lightDirection[lightIndex]);

	vec2 texelSize = 1.0 / textureSize(getShadowMap(lightIndex), 0);

	float shadow = 0.f;
	float bias = max(0.01 * (1.0 - dot(Nnor, NtoLight)), 0.001);
	int samplesqrt = 2;
	int samples = 0;
	for (int x = -samplesqrt; x <= samplesqrt; ++x) {
		for (int y = -samplesqrt; y <= samplesqrt; ++y) {
			samples++;
			float litDepth = texture(getShadowMap(lightIndex), ss.xy + vec2(x, y) * texelSize).r;
			shadow += fragDepth > (litDepth + bias) ? 1.0 : 0.0;
		}
	}

	//float litDepth = texture(getShadowMap(lightIndex), ss.xy).r;

	// float bias = max(0.01 * (1.0 - dot(Nnor, NtoLight)), 0.001);
	// shadow = fragDepth > (litDepth + bias) ? 1.0 : 0.0;

	if (fragDepth > 1)
		shadow = 0.f;

	return shadow / samples;
}

float CalculateDirectionalIllumination(int lightIndex, float shadow)
{
	vec3 Nnor = normalize(nor);
	vec3 NToLight = - normalize(lightDirection[lightIndex]);
	float diffuse = dot(Nnor, NToLight);

	diffuse = max(diffuse, 0);

	vec3 NFromLight = normalize(lightDirection[lightIndex]);
	vec3 NrefLight = reflect(NFromLight, Nnor);
	
	vec3 camDirection = camPos - FragPosWorldSpace;
	vec3 NcamDirection = normalize(camDirection);
	float specular = dot(NcamDirection, NrefLight);
	specular = max(specular, 0);
	specular = pow(specular, 128.f);

	float phong =  ambient + (shadow * (diffuse + specular));
	return phong;

}

float CalculatePositionalIllumination(int lightIndex, float shadow)
{

	vec3 Nnor = normalize(nor);
	vec3 NToLight = normalize(lightPos[lightIndex] - FragPosWorldSpace);
	float diffuse = dot(Nnor, NToLight);

	diffuse = max(diffuse, 0);

	vec3 NFromLight = - NToLight;
	vec3 NrefLight = reflect(NFromLight, Nnor);
	
	vec3 camDirection = camPos - FragPosWorldSpace;
	vec3 NcamDirection = normalize(camDirection);
	float specular = dot(NcamDirection, NrefLight);
	specular = max(specular, 0);
	specular = pow(specular, 128.f);


	float dist = length(lightPos[lightIndex] - FragPosWorldSpace);
	float attenuation = 1 / (attConst + (attLin * dist) + (attQuad * pow(dist, 2.f)));


	float phong =  (ambient + (shadow * (diffuse + specular))) * attenuation;
	return phong;
}

float CalculateSpotIllumination(int lightIndex, float shadow)
{
	vec3 Nnor = normalize(nor);
	vec3 NToLight = normalize(lightPos[lightIndex] - FragPosWorldSpace);
	float diffuse = dot(Nnor, NToLight);

	diffuse = max(diffuse, 0);

	vec3 NFromLight = - NToLight;
	vec3 NrefLight = reflect(NFromLight, Nnor);
	
	vec3 camDirection = camPos - FragPosWorldSpace;
	vec3 NcamDirection = normalize(camDirection);
	float specular = dot(NcamDirection, NrefLight);
	specular = max(specular, 0);
	specular = pow(specular, 128.f);

	float dist = length(lightPos[lightIndex] - FragPosWorldSpace);
	float attenuation = 1 / (attConst + (attLin * dist) + (attQuad * pow(dist, 2.f)));

	float cut_off = 15.f;
	float phi = cos(radians(cut_off));
	vec3 NSpotDir = normalize(lightDirection[lightIndex]);
	float theta = dot(NFromLight, NSpotDir);

	if (theta > phi) 
	{
		float phong =  (ambient + (shadow * (diffuse + specular))) * attenuation;
		return phong;
	}
	else 
	{
		return ambient * attenuation;
	}

	
}

void main()
{
	float phong = 0;
	float shadow = 0;
	for (int i = 0; i < numLights; i++) {
		shadow = 1 - shadowOnFragment(i);
		switch(lightType[i])
		{
		case 0:
			phong += CalculateDirectionalIllumination(i, shadow);
			break;
		case 1:
			phong += CalculatePositionalIllumination(i, shadow);
			break;
		case 2:
			phong += CalculateSpotIllumination(i, shadow);
			break;
		}
	}

	phong = min(phong, 1);

	switch(modelType)
	{
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
		fColour = texture(Texture, tex) * vec4(phong * lightColour, 1.f);
		break;
	case 5:
		fColour = vec4(phong * vec3(169.f/255,140.f/255,106.f/255), 1.f);
		break;
	case 6:
		fColour = vec4(phong * vec3(.3f, .3f, .3f), 1.f);
		break;
	case 7:
		fColour = vec4(phong * vec3(.6f, .6f, .6f), 1.f);
		break;
	case 8:
		fColour = vec4(phong * vec3(74.f/255, 40.f/255, 9.f/255), 1.f);
		break;
	default:
		fColour = vec4(phong * vec3(1.f, 0.f, 0.f), 1.f);
	};

	// fColour = vec4(phong, phong, phong, 1.f);
	//fColour = vec4(shadow, shadow, shadow, 1.f);
	
}
