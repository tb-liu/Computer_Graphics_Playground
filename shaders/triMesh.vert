#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;

layout (location = 0) out vec3 outColor;

//push constants block
layout( push_constant ) uniform constants
{
	mat4 proj;
	mat4 view;
	mat4 model;
} PushConstants;

void main()
{
	gl_Position = PushConstants.proj * PushConstants.view * PushConstants.model * vec4(vPosition, 1.0f);
	outColor = vColor;
}