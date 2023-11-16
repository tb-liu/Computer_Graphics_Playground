//we will be using glsl version 4.5 syntax
#version 450

void main()
{
	//const array of positions for the triangle
	// changed this to counter clock wise order, since my program set
	// ccw and backface culling on
	const vec3 positions[3] = vec3[3](
		vec3(1.f,1.f, 0.0f),
		vec3(0.f,-1.f, 0.0f),
		vec3(-1.f,1.f, 0.0f)
	);

	//output the position of each vertex
	gl_Position = vec4(positions[gl_VertexIndex], 1.0f);
}