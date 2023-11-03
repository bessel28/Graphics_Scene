#pragma once

#include <stdio.h>
#include <glm/glm.hpp>

#define _USE_MATH_DEFINES

#include <math.h>
#include <cmath>
#define DEG2RAD(n) n*(M_PI/180)

struct SCamera
{
	enum Camera_Movement
	{
		FORWARD,
		BACKWARD,
		LEFT,
		RIGHT
	};

	glm::vec3 Position;
	glm::vec3 Front;
	glm::vec3 Up;
	glm::vec3 Right;

	glm::vec3 WorldUp;

	float Yaw;
	float Pitch;

	const float MovementSpeed = 5.5f;
	const float FlySpeed = 1.f;
	const float MouseSensitivity = 0.02f;

};

float cam_dist = 2.f;

void InitCamera(SCamera& in)
{
	in.Front = glm::vec3(0.0f, 5.0f, -1.0f);
	in.Position = glm::vec3(0.0f, 5.0f, 0.0f);
	in.Up = glm::vec3(0.0f, 1.0f, 0.0f);
	in.WorldUp = in.Up;
	in.Right = glm::normalize(glm::cross(in.Front, in.WorldUp));

	in.Yaw = -45.f;
	in.Pitch = 0.f;
}


void MoveAndOrientCamera(SCamera& in, glm::vec3 target, float distance, float xoffset, float yoffset)
{
	in.Yaw -= xoffset * in.MovementSpeed;
	in.Pitch -= yoffset * in.MovementSpeed;

	if (in.Pitch > 89.f) in.Pitch = 89.f;
	if (in.Pitch <= 5) in.Pitch = 5;

	if (distance >= 24) cam_dist = distance = 24;

	in.Position = glm::vec3(cosf(DEG2RAD(in.Yaw)) * cosf(DEG2RAD(in.Pitch)), sinf(DEG2RAD(in.Pitch)), sinf(DEG2RAD(in.Yaw)) * cosf(DEG2RAD(in.Pitch))) * distance;

	in.Front = glm::normalize(target - in.Position);

	in.Right = glm::normalize(glm::cross(in.Front, in.WorldUp));
	in.Up = glm::normalize(glm::cross(in.Right, in.Front));
}

void MoveFlyThruCamera(SCamera& in, float foffset, float roffset)
{
	if (in.Pitch > 89.f) in.Pitch = 89.f;
	if (in.Pitch < -89.f) in.Pitch = -89.f;	
	
	in.Front = glm::vec3(cosf(DEG2RAD(in.Pitch)) * -cosf(DEG2RAD(in.Yaw)), -sinf(DEG2RAD(in.Pitch)), cosf(DEG2RAD(in.Pitch)) * sinf(DEG2RAD(-in.Yaw)));

	in.Right = glm::normalize(glm::cross(in.Front, in.WorldUp));
	in.Up = glm::normalize(glm::cross(in.Right, in.Front));

	in.Position += foffset * 0.5f * in.Front ;
	in.Position += roffset * 0.5f * in.Right ;

	if (in.Position.y <= 2) in.Position.y = 2;

	float distance = std::sqrt(pow(in.Position.x, 2) + pow(in.Position.y, 2) + pow(in.Position.z, 2));

	if (distance >= 24.5f) {
		glm::vec3 direction = glm::normalize(in.Position);
		in.Position = direction * glm::vec3(24.5f);
	}

}

void OrientFlyThruCamera(SCamera& in, float xoffset, float yoffset)
{
	in.Yaw -= xoffset * in.MouseSensitivity;
	in.Pitch -= yoffset * in.MouseSensitivity;

	if (in.Pitch > 89.f) in.Pitch = 89.f;
	if (in.Pitch < -89.f) in.Pitch = -89.f;

	in.Front = glm::vec3(cosf(DEG2RAD(in.Pitch)) * -cosf(DEG2RAD(in.Yaw)), -sinf(DEG2RAD(in.Pitch)), cosf(DEG2RAD(in.Pitch)) * sinf(DEG2RAD(-in.Yaw)));

	in.Right = glm::normalize(glm::cross(in.Front, in.WorldUp));
	in.Up = glm::normalize(glm::cross(in.Right, in.Front));
}