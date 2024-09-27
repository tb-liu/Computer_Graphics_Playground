#version 450
#extension GL_GOOGLE_include_directive : require
#include "header.glsl"
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

layout(std430, binding = 1) buffer storageBuffer {
	Particle particles[MAX_INSTANCE];
} ObjectData;

void main()
{
	vec4 pos = ObjectData.particles[gl_InstanceIndex].pos + cameraData.model * vec4(vPosition, 1);
	gl_Position = cameraData.proj * cameraData.view * pos;
	float length = length(ObjectData.particles[gl_InstanceIndex].velocity);
	float t = 0;
    t = length / 30.f;
	outColor = mix(vec3(0,0,1), vec3(1,1,1), t);
}