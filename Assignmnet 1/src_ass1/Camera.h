#pragma once
#include <algorithm>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

enum Camera_Movement
{
	Right_Pos,
	Right_Neg,
	Up_Pos,
	Up_Neg,
	Front_Pos,
	Front_Neg
};

class Camera
{
public:
	// Default values:
	float const NEAR_PLANE = 0.1f;
	float const FAR_PLANE = 100.0f;
	float const PERSP = 45.0f;
	float POS_X = 0;
	float POS_Y = 0;
	float POS_Z = -10;
	float DIST = -10;

	// Local camera values
	float perspective;
	int width, height;
	float nearPlane, farPlane;
	glm::vec3 localPos;
	glm::vec3 axisX, axisY, axisZ, localUp; // Left/Right, Up/Down, Forward/Backward respectively
	float RotX, RotY, RotZ;

	// Constructor
	Camera(int width_ = 1200, int height_ = 700)
	{
		width = width_;
		height = height_;
		perspective = PERSP;
		nearPlane = NEAR_PLANE;
		farPlane = FAR_PLANE;

		// initialLoc = glm::vec3(0, 0, -10);
		localPos = glm::vec3(0, 0, -10);
		// objectLoc = glm::vec3(0, 0, 0);

		axisY = glm::vec3(0, 1, 0);
		axisX = glm::vec3(1, 0, 0);
		axisZ = glm::vec3(0, 0, 1);

		localUp = glm::vec3(0, 1, 0);

		RotX = 0;
		RotY = 0;
		RotZ = 0;
	}

	glm::mat4 getMVPMatrix()
	{
		// Projection matrix : 45 degree Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
		glm::mat4 Projection = glm::perspective(glm::radians(perspective), (float)width / height, nearPlane, farPlane);

		glm::vec3 pointVector = glm::vec3(0, 0, 1);
		glm::vec3 defaultUp = glm::vec3(0, 1, 0);

		// Rotate Up:
		glm::mat4 rot = glm::mat4(1.0f);
		glm::vec4 rVec = glm::vec4(pointVector.x, pointVector.y, pointVector.z, 1);
		rot = glm::rotate(rot, glm::radians(RotY), glm::vec3(0, 1, 0));
		rVec = rVec * rot;
		pointVector = glm::vec3(rVec.x, rVec.y, rVec.z);

		// Rotate along Forward:
		rot = glm::mat4(1.0f);
		glm::vec4 tempUp = glm::vec4(defaultUp.x, defaultUp.y, defaultUp.z, 1);
		rot = glm::rotate(rot, glm::radians(RotZ), glm::vec3(0, 0, 1));
		tempUp = tempUp * rot;
		axisY = glm::vec3(tempUp.x, tempUp.y, tempUp.z);

		// Rotate along Right
		rot = glm::mat4(1.0f);
		rot = glm::rotate(rot, glm::radians(RotX), glm::vec3(1, 0, 0));
		rVec = rVec * rot;
		tempUp = tempUp * rot;
		axisY = glm::vec3(tempUp.x, tempUp.y, tempUp.z);
		pointVector = glm::vec3(rVec.x, rVec.y, rVec.z);

		// Set Camera View (where initalLoc is the camera origin)
		glm::mat4 View = glm::lookAt(localPos, pointVector + localPos, axisY); // This should act as the new origin

		// Model matrix : Model is at origin
		glm::mat4 Model = glm::mat4(1.0f);

		// ModelViewProjection
		glm::mat4 mvp = Projection * View * Model; // Remember, matrix multiplication is the other way around

		return mvp;
	}

	/* Centers the model with an object center and dist from the object. */
	void setModelCenter(float center, float dist)
	{
		DIST = dist;
		POS_X = 0;
		POS_Y = center;
		POS_Z = -1 * DIST;

		localPos = glm::vec3(POS_X, POS_Y, POS_Z);
	}

	/* Set camera to default settings. */
	void reset()
	{
		perspective = PERSP;
		nearPlane = NEAR_PLANE;
		farPlane = FAR_PLANE;

		// initialLoc = glm::vec3(0, 0, -10);
		localPos = glm::vec3(POS_X, POS_Y, POS_Z);
		// objectLoc = glm::vec3(0, 0, 0);

		axisY = glm::vec3(0, 1, 0);
		axisX = glm::vec3(1, 0, 0);
		axisZ = glm::vec3(0, 0, 1);

		localUp = glm::vec3(0, 1, 0);

		RotX = 0;
		RotY = 0;
		RotZ = 0;
	}

	/* Implement camera movement. */
	void rotateByVal(Camera_Movement rotType, double rotValue)
	{
		if (rotType == Front_Neg)
		{
			RotZ += rotValue;
		}
		else if (rotType == Front_Pos)
		{
			RotZ -= rotValue;
		}
		else if (rotType == Right_Neg)
		{
			RotX += rotValue;
		}
		else if (rotType == Right_Pos)
		{
			RotX -= rotValue;
		}
		else if (rotType == Up_Neg)
		{
			RotY += rotValue;
		}
		else if (rotType == Up_Pos)
		{
			RotY -= rotValue;
		}
	}
};
