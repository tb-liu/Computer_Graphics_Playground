#pragma once

#include <vector>
#include <array>
#include <glm/glm.hpp>
#include "vk_types.h"

const int MAX_INSTANCE = 1024*32; // max particles for now
const int THREADS_PER_GROUP = 256;

struct Particle
{
    glm::vec4 pos, velocity, force;
    float density;
    float pad[3];
};

struct VertexInputDescription
{
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    VkPipelineVertexInputStateCreateFlags flags = 0;
};

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;

    // TODO: change this to take a binding number?
    static VertexInputDescription getVertexDescription();
};

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    AllocatedBuffer vertexBuffer;
    AllocatedBuffer indiceBuffer;
    bool loadFromOBJ(const char * filename);
};

struct PipelineSet
{
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
};

struct RenderObject
{
    Mesh* mesh;

    PipelineSet* pipelineSet;

    glm::mat4 transformMatrix;
};

// for particles information
struct StorageBuffer
{
    std::array<Particle, MAX_INSTANCE> particles;

    // store the info need to allocate buffer for this struct
    // also not contribute to the class size
    static AllocatedBuffer storageBuffer;
};

void generateSphere(Mesh& mesh, int numDivisions);