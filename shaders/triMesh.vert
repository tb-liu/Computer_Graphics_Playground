#version 450

const int MAX_INSTANCE = 4096;

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;

layout (location = 0) out vec3 outColor;

//push constants block
layout( set = 0, binding = 0 ) uniform CameraBuffer
{
	mat4 proj;
	mat4 view;
	mat4 model;
} cameraData;

layout(std140, binding = 1) buffer storageBuffer {
	vec4 pos[MAX_INSTANCE];
    vec4 velocity[MAX_INSTANCE];
} ObjectData;

void main()
{
	vec4 pos = ObjectData.pos[gl_InstanceIndex] + cameraData.model * vec4(vPosition, 1);
	gl_Position = cameraData.proj * cameraData.view * pos;
	outColor = vColor;
}