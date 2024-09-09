#pragma once

#include <vector>
#include <array>
#include <glm/glm.hpp>
#include "vk_types.h"

const int MAX_INSTANCE = 4096; // max particles for now

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

struct Material
{
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
};

struct RenderObject
{
    Mesh* mesh;

    Material* material;

    glm::mat4 transformMatrix;
};

// for particles information
struct StorageBuffer
{
    std::array<glm::vec4, MAX_INSTANCE> pos;
    std::array<glm::vec4, MAX_INSTANCE> vel;

    // store the info need to allocate buffer for this struct
    // also not contribute to the class size
    static AllocatedBuffer storageBuffer;
};

void generateSphere(Mesh& mesh, int numDivisions);