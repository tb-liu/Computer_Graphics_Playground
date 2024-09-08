﻿// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once
#include <vk_types.h>
#include <vector>
#include <vk_mem_alloc.h>
#include <unordered_map>
#include <String>

#include "DeletionQueue.h"
#include "RingBuffer.h"
#include "Mesh.h"
#include "SystemBase.h"
#include "Camera.h"

namespace GraphicsGlobal 
{
	extern const int MAX_SHADER_COUNT;
	extern int SELECTED_SHADER;
}



struct UniformBuffer
{
	glm::mat4 proj;
	glm::mat4 view;
	glm::mat4 model;
};

// TODO: change this to graphics class and only respones for rendering
class VulkanEngine :public SystemBase {
public:

	bool isInitialized{ false };
	int frameNumber {0};

	VkExtent2D windowExtent{ 800 , 600 };

	struct SDL_Window* window{ nullptr };

	//initializes everything in the engine
	void init() override;

	//shuts down the engine
	void shutdown() override;

	//draw loop
	void update(float) override;

	SystemType Type() const override;
private:
	
	Camera* cameraPtr;
	UniformBuffer ubo;

	// basic vulkan
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkPhysicalDevice gpuDevice;
	VkDevice device;
	VkSurfaceKHR surface;

	// vulkan swapchain
	VkSwapchainKHR swapchain;
	VkFormat swapchainImageFormat;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	VkImageView depthImageView;
	AllocatedImage depthImage;

	VkDescriptorSetLayout globalSetLayout;
	VkDescriptorPool descriptorPool;

	//the format for the depth image
	VkFormat depthFormat;
	// vulkan command buffer & graphics queue
	VkQueue graphicsQueue; //queue we will submit to
	uint32_t graphicsQueueFamily; //family of that queue

	// render pass
	VkRenderPass renderPass;
	std::vector<VkFramebuffer> framebuffers;
	// one buffer per frame
	std::vector<AllocatedBuffer> buffers;
	std::vector<VkDescriptorSet> globalDescriptors;

	// Sync Object
	RingBuffer ringBuffer;

	// pipeline related things
	// TODO: all these will go into render objects array
	VkPipelineLayout trianglePipelineLayout;
	VkPipeline trianglePipeline;
	VkPipeline redTrianglePipeline;


	// deletion queue
	DeletionQueue deletionQueue;

	// memory allocator
	VmaAllocator allocator;

	// mesh objects
	std::vector<RenderObject> renderObjects;
	std::unordered_map<std::string, Material> materials;
	std::unordered_map<std::string, Mesh> meshes;

	//create material and add it to the map
	Material* createMaterial(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name);
	Material* getMaterial(const std::string& name);
	Mesh* getMesh(const std::string& name);

	// loads a shader module from a spir-v file. Returns false if it errors
	bool loadShaderModule(const char* filePath, VkShaderModule* outShaderModule);

	// a wrapper function for loading shader
	void loadShaderWrapper(std::string shaderName, VkShaderModule* outShaderModule);

	// mesh functions
	void loadMeshes();
	void uploadMesh(Mesh & mesh);

	void initVulkan();
	void initSwapchain();
	void initDefaultRenderpass();
	void initFrameBuffers();
	void initSyncStructures();
	void initPipeline();
	void initDescriptors();
	void initScene();
};
