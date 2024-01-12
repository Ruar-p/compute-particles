
/**** Basic template for compute shaders on the GPU. ****/

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <computeParticles.h>

#include <utils/camera.h>
#include <utils/shaders/Shader.h>
#include <utils/shaders/ComputeShader.h>
#include <utils/noise/FastNoiseLite.h>
#include <utils/asset-loading/stb_image.h>

#include <iostream>
#include <map>
#include <thread>
#include <vector>


// Screen Settings
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

// Camera variables
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
const float CAM_FAR = 10000.0f;
const float CAM_NEAR = 0.1f;

// Timing/fps counter variables
float deltaTime = 0.0f;
float lastFrame = 0.0f;
int frameCounter = 0;

// Misc.
bool paused = true;


// Compute Shader constants
const int WORK_GROUP_SIZE = 1024;


struct Particle {
	glm::vec4 position;
	glm::vec4 velocity;
	glm::vec4 accel;

	Particle(glm::vec4 p1, glm::vec4 p2, glm::vec4 p3)
		: 
		position(p1),
		velocity(p2),
		accel(p3)
	{}
};


int main() {
	//// SETUP ////
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	glfwWindowHint(GLFW_SAMPLES, 4);						// Anti-aliasing

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Particles", NULL, NULL);

	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	//glfwSwapInterval(0);	// VSync, I think. 0 = off, 1 = on

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "failed to initialize GLAD" << std::endl;
		return -1;
	}
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);	// Viewport dimensions
	// Input Mode
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	// Register callbacks here
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);


	// Various state settings
	glEnable(GL_DEPTH_TEST);								// Depth test
	glDepthFunc(GL_LESS);

	glEnable(GL_MULTISAMPLE);								// Anti-aliasing

	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	// Alpha-blending
	//glEnable(GL_BLEND);


	//// SHADERS ////	Shader shader("path/to/vert.vert", "path/to/frag.frag");
	Shader particleShader("data/shaders/particleShader.vert", "data/shaders/particleShader.frag");

	// Compute shader	Shader shader("path/to/compute.comp");
	ComputeShader computeShader("data/shaders/computeShader.comp");
	// Display max params for compute shader
	/*
	GLint maxWorkGroupSize;
	GLint maxWorkGroupCount[3];
	GLint maxInvocationsPerWorkGroup;
	GLint maxSharedMemorySize;
	GLint maxImageUnits;
	GLint maxImageUniforms;
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &maxWorkGroupSize);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &maxWorkGroupCount[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &maxWorkGroupCount[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &maxWorkGroupCount[2]);
	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxInvocationsPerWorkGroup);
	glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, &maxSharedMemorySize);
	glGetIntegerv(GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS, &maxImageUnits);
	glGetIntegerv(GL_MAX_COMPUTE_IMAGE_UNIFORMS, &maxImageUniforms);
	std::cout << "GL_MAX_COMPUTE_WORK_GROUP_SIZE: " << maxWorkGroupSize << std::endl;
	std::cout << "GL_MAX_COMPUTE_WORK_GROUP_COUNT 0: " << maxWorkGroupCount[0] << std::endl;
	std::cout << "GL_MAX_COMPUTE_WORK_GROUP_COUNT 1: " << maxWorkGroupCount[1] << std::endl;
	std::cout << "GL_MAX_COMPUTE_WORK_GROUP_COUNT 2: " << maxWorkGroupCount[2] << std::endl;
	std::cout << "GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS: " << maxInvocationsPerWorkGroup << std::endl;
	std::cout << "GL_MAX_COMPUTE_SHARED_MEMORY_SIZE: " << maxSharedMemorySize << std::endl;
	std::cout << "GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS: " << maxImageUnits << std::endl;
	std::cout << "GL_MAX_COMPUTE_IMAGE_UNIFORMS: " << maxImageUniforms << std::endl;
	*/

	//// VERTICES ////
	float vertices[] = {
		// first triangle
		 0.0f,  1.0f, 0.0f,	 0.0f, 1.0f,  // top left 
		 0.0f,  0.0f, 0.0f,  0.0f, 0.0f,  // bottom left
		 1.0f,  0.0f, 0.0f,	 1.0f, 0.0f,  // bottom right

		 // second triangle
		 0.0f,  1.0f, 0.0f, 0.0f, 1.0f,  // top left 
		 1.0f,  0.0f, 0.0f,	 1.0f, 0.0f,  // bottom right
		 1.0f,  1.0f, 0.0f,	 1.0f, 1.0f   // top right
	};


	// Offsets, Noise, Particle, ParticleSystem setup
	// --------------------------------
	// 
	//// NOISE SETUP ////
	FastNoiseLite noise = FastNoiseLite();
	noise.SetNoiseType(noise.NoiseType_OpenSimplex2S);
	noise.SetFrequency(0.005f);
	float inc = 0.0f;

	const long int NUM_PARTICLES = WORK_GROUP_SIZE * 2000; // 1024 * 1000 = 1024000

	std::vector<Particle> particles;
	particles.reserve(NUM_PARTICLES);
	
	for (int i = 0; i < NUM_PARTICLES; ++i) {
		// Pos Offsets and particle size
		//float x = 0.0f;
		//float y = 0.0f;
		//float z = 0.0f;
		//float w = 10.0f;	// Particle size
		float x = (noise.GetNoise(inc, inc, inc))			* SCR_WIDTH;
		float y = (noise.GetNoise(x, inc, inc))				* SCR_WIDTH;
		float z = (noise.GetNoise(x, y, 0.0f))				* SCR_WIDTH;
		float w = ((noise.GetNoise(x, y, z) + 1.0f) / 2.0f)	* 3.0f;		// Scale factor
		glm::vec4 posAndSize = glm::vec4(x, y, z, w);

		// Velocities and mass
		float velX = (noise.GetNoise(inc, inc))					* 0.015f;
		float velY = (noise.GetNoise(velX, inc))				* 0.015f;
		float velZ = (noise.GetNoise(inc, velY))				* 0.015f;
		float mass = (noise.GetNoise(velX, velY, velZ) + 1.0f)	* 0.075f;
		glm::vec4 velandMass = glm::vec4(velX, velY, velZ, mass);

		//glm::vec4 accel = glm::vec4(0.0f, -0.00001, 0.0f, 1.0f);	// Gravity
		glm::vec4 accel = glm::vec4(velX/1000, velX / 1000, 
									velX / 1000, mass);	// Weird accel

		// Add a particle to particles
		particles.emplace_back(posAndSize, velandMass, accel);
		inc += 0.005f;
	}


	//// VBOs, VAOs ////
	// Square billboard
	unsigned int VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3*sizeof(float))); // Circle texture attrib pointer
	glEnableVertexAttribArray(1);

	// SSBO : Bind a large amount of particle info to a shader storage buffer object
	GLuint SSBO;
	glGenBuffers(1, &SSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Particle) * NUM_PARTICLES, particles.data(), GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, SSBO);	// binding = 2 in the shader layouts for this SSBO 

	//// TEXTURES & SOME UNIFORMS ////
	unsigned int circleTexture = loadTexture("data/textures/circle1.png");

	particleShader.use();
	glActiveTexture(GL_TEXTURE0);
	particleShader.setInt("texture1", 0);

	computeShader.use();
	computeShader.setInt("X_BOUNDS", SCR_WIDTH);
	computeShader.setInt("Y_BOUNDS", SCR_WIDTH);
	computeShader.setInt("Z_BOUNDS", SCR_WIDTH);

	
	//// RENDER LOOP ////
	while (!glfwWindowShouldClose(window))
	{
		//// TIME ////
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		//// INPUT ////
		processInput(window);
		//// BACKGROUND ////
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);		// Background
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//// SHADERS ////
		// Compute shader
		GLuint SSBOIdx = 2;
		if (!paused) {
			computeShader.use();
			computeShader.setFloat("dt", currentFrame);

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBOIdx, SSBO);

			glDispatchCompute(NUM_PARTICLES / WORK_GROUP_SIZE, 1, 1);
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT); // Ensures writing has finished before read
		}

		
		// Shader for drawing circles
		//glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);	// Draw texture points (turn off discard fragments in shader)
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);	// Draw line
		particleShader.use();
		glm::mat4 model = glm::mat4(1.0f);
		glm::mat4 view = camera.getViewMatrix();
		glm::mat4 projection = glm::perspective(glm::radians(camera.zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, CAM_NEAR, CAM_FAR);

		particleShader.setMat4("model", model);
		particleShader.setMat4("view", view);
		particleShader.setMat4("projection", projection);
		particleShader.setVec3("cameraPos", camera.position);

		//glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
		//glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBOIdx, SSBO);
		glBindVertexArray(VAO);
		glBindTexture(GL_TEXTURE_2D, circleTexture);
		glDrawArraysInstanced(GL_TRIANGLES, 0, 6, NUM_PARTICLES);
		glBindVertexArray(0);


		// FPS Counter
		if (frameCounter > 1000) {
			std::cout << "FPS: " << 1 / deltaTime << std::endl;
			frameCounter = 0;
		}
		else {
			frameCounter++;
		}
		
		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	
	// De-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &SSBO);

	// GLFW: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}


// Process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		camera.processKeyboard(SPEED_CAM, deltaTime);
	
	// Q -> Pause
	// E -> Resume
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		paused = true;
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		paused = false;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.processKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.processKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.processKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.processKeyboard(RIGHT, deltaTime);
}

// GLFW: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// GLFW: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	camera.processMouseMovement(xoffset, yoffset);
}

// GLFW: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.processMouseScroll(static_cast<float>(yoffset));
}

// Utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const* path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;
		else {
			std::cout << "Issue with loading textures! Check loadTexture()." << std::endl;
			return -1;
		}
			

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		// Default
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		
		// For transparent textures - prevents weird border interpolation issues
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}