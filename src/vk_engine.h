// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once
#include <vk_types.h>
#include <vector>
#include <vk_mem_alloc.h>

#include "DeletionQueue.h"
#include "RingBuffer.h"
#include "Mesh.h"

struct UniformBuffer
{
	glm::mat4 proj;
	glm::mat4 view;
	glm::mat4 model;
};

class VulkanEngine {
public:

	bool isInitialized{ false };
	int frameNumber {0};

	VkExtent2D windowExtent{ 800 , 600 };

	struct SDL_Window* window{ nullptr };

	//initializes everything in the engine
	void init();

	//shuts down the engine
	void cleanup();

	//draw loop
	void draw();

	//run main loop
	void run();
private:

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

	//the format for the depth image
	VkFormat depthFormat;
	// vulkan command buffer & graphics queue
	VkQueue graphicsQueue; //queue we will submit to
	uint32_t graphicsQueueFamily; //family of that queue

	std::vector<VkCommandPool> commandPool; //the command pool for our commands
	std::vector<VkCommandBuffer> mainCommandBuffer; //the buffer we will record into

	// render pass
	VkRenderPass renderPass;
	std::vector<VkFramebuffer> framebuffers;

	// Sync Object
	RingSyncObjects ringBuffer;

	// pipeline related things
	VkPipelineLayout trianglePipelineLayout;
	VkPipeline trianglePipeline;
	VkPipeline redTrianglePipeline;
	VkPipeline meshPipeline;

	// deletion queue
	DeletionQueue deletionQueue;

	// memory allocator
	VmaAllocator allocator;

	// mesh objects
	Mesh monkeyMesh;

	//loads a shader module from a spir-v file. Returns false if it errors
	bool loadShaderModule(const char* filePath, VkShaderModule* outShaderModule);
	// a wrapper function for loading shader
	void loadShaderWrapper(std::string shaderName, VkShaderModule* outShaderModule);

	// mesh functions
	void loadMeshes();
	void uploadMesh(Mesh & mesh);

	void initVulkan();
	void initSwapchain();
	void initCommands();
	void initDefaultRenderpass();
	void initFrameBuffers();
	void initSyncStructures();
	void initPipeline();
};
