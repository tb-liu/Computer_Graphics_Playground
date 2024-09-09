#include <tiny_obj_loader.h>
#include <iostream>
#include "Mesh.h"
const float PI = 3.1415926;
const float R = 0.5f;
VertexInputDescription Vertex::getVertexDescription()
{
	VertexInputDescription description;

	//we will have just 1 vertex buffer binding, with a per-vertex rate
	VkVertexInputBindingDescription mainBinding = {};
	mainBinding.binding = 0;
	mainBinding.stride = sizeof(Vertex);
	mainBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	description.bindings.push_back(mainBinding);

	//Position will be stored at Location 0
	VkVertexInputAttributeDescription positionAttribute = {};
	positionAttribute.binding = 0;
	positionAttribute.location = 0;
	positionAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	positionAttribute.offset = offsetof(Vertex, position);

	//Normal will be stored at Location 1
	VkVertexInputAttributeDescription normalAttribute = {};
	normalAttribute.binding = 0;
	normalAttribute.location = 1;
	normalAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	normalAttribute.offset = offsetof(Vertex, normal);

	//Color will be stored at Location 2
	VkVertexInputAttributeDescription colorAttribute = {};
	colorAttribute.binding = 0;
	colorAttribute.location = 2;
	colorAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	colorAttribute.offset = offsetof(Vertex, color);

	description.attributes.push_back(positionAttribute);
	description.attributes.push_back(normalAttribute);
	description.attributes.push_back(colorAttribute);

	return description;
}

bool Mesh::loadFromOBJ(const char* filename)
{
	// contain vertex array of the file
	tinyobj::attrib_t attrib;
	// contain each seperate obect in the file
	std::vector<tinyobj::shape_t> shapes;
	// info about material of each shape
	std::vector<tinyobj::material_t> materials;

	std::string warning, error;

	// load obj file
	tinyobj::LoadObj(&attrib, &shapes, &materials, &warning, &error, filename, nullptr);
	if (!warning.empty())
	{
		std::cout << "WARN: " << warning << std::endl;
	}
	//if we have any error, print it to the console, and break the mesh loading.
	if (!error.empty()) {
		std::cerr << error << std::endl;
		return false;
	}

	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		size_t indexOffset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {

			//hardcode loading to triangles
			int fv = 3;

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t index = shapes[s].mesh.indices[indexOffset + v];

				//vertex position
				tinyobj::real_t vx = attrib.vertices[3 * index.vertex_index + 0];
				tinyobj::real_t vy = attrib.vertices[3 * index.vertex_index + 1];
				tinyobj::real_t vz = attrib.vertices[3 * index.vertex_index + 2];
				//vertex normal
				tinyobj::real_t nx = attrib.normals[3 * index.normal_index + 0];
				tinyobj::real_t ny = attrib.normals[3 * index.normal_index + 1];
				tinyobj::real_t nz = attrib.normals[3 * index.normal_index + 2];

				//copy it into our vertex
				Vertex newVert;
				newVert.position.x = vx;
				newVert.position.y = vy;
				newVert.position.z = vz;

				newVert.normal.x = nx;
				newVert.normal.y = ny;
				newVert.normal.z = nz;

				// setting the vertex color as the vertex normal. This is just for display purposes
				newVert.color = newVert.normal;


				vertices.push_back(newVert);
				// indices.push_back(indexOffset + v);
			}
			indexOffset += fv;
		}
	}

	return true;
}

void generateSphere(Mesh& mesh, int numDivisions)
{
	std::vector<Vertex>& vertices = mesh.vertices;
	std::vector<uint32_t>& indices = mesh.indices;
	float sectorStep = 2 * PI / numDivisions;
	float stackStep = PI / numDivisions;
	float sectorAngle, stackAngle;
	float x, y, z, xy;


	for (int i = 0; i <= numDivisions; i++)
	{
		stackAngle = PI / 2 - i * stackStep;
		xy = R * cosf(stackAngle);
		z = R * sinf(stackAngle);
		for (int j = 0; j <= numDivisions; j++)
		{
			sectorAngle = j * sectorStep;
			y = xy * sinf(sectorAngle);
			x = xy * cosf(sectorAngle);
			// push position and normal, default blue color
			vertices.push_back({ glm::vec3(x, y, z), glm::vec3(x, y, z), glm::vec3(0.f, 0.f, 0.5f) });
		}
	}

	int L1, L2;
	for (int i = 0; i < numDivisions; i++)
	{
		L1 = i * (numDivisions + 1);
		L2 = L1 + numDivisions + 1;

		for (int j = 0; j < numDivisions; j++)
		{
			if (i != 0)
			{
				indices.push_back(L1);
				indices.push_back(L2);
				indices.push_back(L1 + 1);
			}

			if (i != (numDivisions - 1))
			{
				indices.push_back(L1 + 1);
				indices.push_back(L2);
				indices.push_back(L2 + 1);
			}
			L1++;
			L2++;
		}
	}
}

AllocatedBuffer StorageBuffer::storageBuffer;