#version 430

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTex;

struct Particle {
	vec4 posAndSize;    // posX, posY, posZ, size
	vec4 velAndMass;    // velX, velY, velZ, mass
	vec4 accel;         // accX, accY, accZ, empty
};

layout(std430,  binding = 2) buffer pBuffer
{
    Particle particles[];
} input_data;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraPos;

out vec2 texCoords;
out vec3 color;

void main()
{
	vec4 aOffset = vec4(input_data.particles[gl_InstanceID].posAndSize.xyz, 0.0f);				// Calculated pos offset
	vec3 toCamera = normalize(cameraPos - input_data.particles[gl_InstanceID].posAndSize.xyz);	// Direction to camera
	vec3 vel = vec3(input_data.particles[gl_InstanceID].velAndMass.xyz);
	float scale = input_data.particles[gl_InstanceID].posAndSize.w;

	mat4 scaledModel = model * mat4(scale, 0.0f, 0.0f, 0.0f,	// Scale the x and y axes
									 0.0f, scale, 0.0f, 0.0f,
									 0.0f, 0.0f, 1.0f, 0.0f,
									 0.0f, 0.0f, 0.0f, 1.0f);

	// Create a rotation matrix to align the model with the camera direction
    mat3 rotationMatrix = mat3(
        cross(vec3(0.0, 1.0, 0.0), toCamera),	// cross(up, toCam) should give the new x-axis
        vec3(0.0, 1.0, 0.0),					// New y-axis
        -toCamera								// New z-axis
    );

    // Apply the rotation to the scaled model matrix
    mat4 rotatedModel = mat4(rotationMatrix) * scaledModel;

	//gl_Position =  projection * view * model * vec4(aPos + aOffset, 1.0f);			// Just offsets
	gl_Position =  projection * view * ((rotatedModel * vec4(aPos, 1.0f)) + aOffset);
	texCoords = aTex;
	color = aPos + (abs(vel.x) + abs(vel.y) + abs(vel.z));
}

// local	. model mat			-> world space
// world	. view mat			-> view space
// view		. projection mat	-> clip space
// clip		. viewport trans	-> screen space