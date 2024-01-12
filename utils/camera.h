
#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <iostream>

// Defines several possible options for camera movement.
// Used as abstraction to stay away from window-system specific input methods.
enum Camera_Movement {
	SPEED_CAM,
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT
};

// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 10.5f;
const float SPEED_MULT = 75.0f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 90.0f;


// An abstract camera class that processes input 
// and calculates the corresponding Euler Angles,Vectors and Matrices for use in OpenGL
class Camera
{
public:
	// camera Attributes
	glm::vec3 position;
	glm::vec3 front;
	glm::vec3 up;
	glm::vec3 right;
	glm::vec3 worldUp;
	// euler Angles
	float yaw;
	float pitch;
	// camera options
	float movementSpeed;
	float movementMultiplier;
	float mouseSensitivity;
	float zoom;


	// Constructor with Vectors
	Camera(glm::vec3 _position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 _up = glm::vec3(0.0f, 1.0f, 0.0f), float _yaw = YAW, float _pitch = PITCH) : 
		front(glm::vec3(0.0f, 0.0f, -1.0f)), 
		movementSpeed(SPEED), 
		movementMultiplier(1.0f), 
		mouseSensitivity(SENSITIVITY), 
		zoom(ZOOM)
	{
		position = _position;
		worldUp = _up;
		yaw = _yaw;
		pitch = _pitch;
		updateCameraVectors();
	}
	// constructor with scalar values
	// constructor with scalar values
	Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float _yaw, float _pitch) : 
		front(glm::vec3(0.0f, 0.0f, -1.0f)), movementSpeed(SPEED), movementMultiplier(1.0f), mouseSensitivity(SENSITIVITY), zoom(ZOOM)
	{
		position = glm::vec3(posX, posY, posZ);
		worldUp = glm::vec3(upX, upY, upZ);
		yaw = _yaw;
		pitch = _pitch;
		updateCameraVectors();
	}

	// Returns the view matrix calculated using Euler Angles and the LookAt Matrix
	glm::mat4 getViewMatrix()
	{
		return glm::lookAt(position, position + front, up);
	}

	// processes input received from any keyboard-like input system. 
	// Accepts input parameter in the form of camera defined ENUM 
	// (to abstract it from windowing systems)
	void processKeyboard(Camera_Movement input, float deltaTime)
	{
		float velocity = movementSpeed * movementMultiplier * deltaTime;
		if (input == FORWARD)
			position += front * velocity;
		if (input == BACKWARD)
			position -= front * velocity;
		if (input == LEFT)
			position -= right * velocity;
		if (input == RIGHT)
			position += right * velocity;

		// Must be last for some reason
		if (input != SPEED_CAM) {
			movementMultiplier = 1.0f;
		}
		else {
			movementMultiplier = SPEED_MULT;
		}
	}


	// processes input received from a mouse input system. 
	// Expects the offset value in both the x and y direction.
	void processMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
	{
		xoffset *= mouseSensitivity;
		yoffset *= mouseSensitivity;

		yaw += xoffset;
		pitch += yoffset;

		// make sure that when pitch is out of bounds, screen doesn't get flipped
		if (constrainPitch)
		{
			if (pitch > 89.0f)
				pitch = 89.0f;
			if (pitch < -89.0f)
				pitch = -89.0f;
		}

		// update front, right and up Vectors using the updated Euler angles
		updateCameraVectors();
	}


	// processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
	void processMouseScroll(float yoffset)
	{
		zoom -= (float)yoffset;
		if (zoom < 1.0f)
			zoom = 1.0f;
		if (zoom > 90.0f)
			zoom = 90.0f;
	}
	
private:
	// calculates the front vector from the Camera's (updated) Euler Angles
	void updateCameraVectors()
	{
		// calculate the new Front vector
		glm::vec3 newFront;
		newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		newFront.y = sin(glm::radians(pitch));
		newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		front = glm::normalize(newFront);
		// also re-calculate the right and up vector
		right = glm::normalize(glm::cross(front, worldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
		up = glm::normalize(glm::cross(right, front));
	}
};