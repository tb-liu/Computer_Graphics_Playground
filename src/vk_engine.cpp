
#define VMA_IMPLEMENTATION
#include "vk_engine.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <vk_types.h>
#include <vk_initializers.h>

#include "VkBootstrap.h"

#include <iostream>
#include <fstream>
#include "Defines.h"
#include "vk_pipeline.h"

#include "engine.h"


const int MAX_FRAMES_IN_FLIGHT = 2;
int CURRENT_FRAME = 0;

const int GraphicsGlobal::MAX_SHADER_COUNT = 3;
int GraphicsGlobal::SELECTED_SHADER = 2;



void VulkanEngine::init()
{
	// We initialize SDL and create a window with it. 
	SDL_Init(SDL_INIT_VIDEO);

	SDL_WindowFlags windowFlags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);
	
	window = SDL_CreateWindow(
		"Playground",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		windowExtent.width,
		windowExtent.height,
		windowFlags
	);
	
	// vulkan init
	initVulkan();
	initSwapchain();
	initDefaultRenderpass();
	initFrameBuffers();
	initComputeBuffer();
	initSyncStructures();
	initDescriptors();
	initPipeline();
	loadMeshes();
	initScene();
	//everything went fine
	isInitialized = true;

	// grab the pointer to camera system
	cameraPtr = dynamic_cast<Camera*>(Engine::getInstance()->getSystem(SystemType::CAMERA));
	ubo.model = glm::mat4(1.f);
	ubo.proj = glm::perspective(glm::radians(45.f), windowExtent.width / (float)windowExtent.height, 0.1f, 200.f);
	ubo.proj[1][1] *= -1;
}
void VulkanEngine::shutdown()
{	
	if (isInitialized) 
	{
		// wait for all things to finish
		vkDeviceWaitIdle(device);
		// destroy sync objects
		graphicsQueueRingBuffer.cleanUpSyncObjects();
		computeQueueRingBuffer.cleanUpSyncObjects();

		deletionQueue.flush();

		// destory allocator
		vmaDestroyAllocator(allocator);
		vkDestroyDevice(device, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkb::destroy_debug_utils_messenger(instance, debugMessenger);
		vkDestroyInstance(instance, nullptr);

		SDL_DestroyWindow(window);
	}
}

void VulkanEngine::update(float dt)
{
	CURRENT_FRAME = CURRENT_FRAME++ % MAX_FRAMES_IN_FLIGHT;
	// acqure next sync objects
	SyncObject * nextSync = graphicsQueueRingBuffer.getNextObject();
	SyncObject* nextComputeSync = computeQueueRingBuffer.getNextObject();

	

	// compute pipeline
	// wait for previous compute done
	VK_CHECK(vkWaitForFences(device, 1, &nextComputeSync->renderFence, true, ONE_SECOND));
	VK_CHECK(vkResetFences(device, 1, &nextComputeSync->renderFence));

	VkCommandBuffer computeCmd = nextComputeSync->mainCommandBuffer;
	VkCommandBufferBeginInfo computeCmdBeginInfo = {};
	computeCmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	computeCmdBeginInfo.pNext = nullptr;
	computeCmdBeginInfo.pInheritanceInfo = nullptr;
	computeCmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK(vkBeginCommandBuffer(computeCmd, &computeCmdBeginInfo));

	vkCmdBindPipeline(computeCmd, VK_PIPELINE_BIND_POINT_COMPUTE, getPipelineSet("ComputePipeline")->pipeline);
	vkCmdBindDescriptorSets(computeCmd, VK_PIPELINE_BIND_POINT_COMPUTE, getPipelineSet("ComputePipeline")->pipelineLayout, 0, 1, &computeDescriptors, 0, nullptr);
	//upload the matrix to the GPU via push constants
	vkCmdPushConstants(computeCmd, getPipelineSet("ComputePipeline")->pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float), &dt);

	// Dispatch the compute shader
	vkCmdDispatch(computeCmd, MAX_INSTANCE / THREADS_PER_GROUP + 1, 1, 1);
	


	VK_CHECK(vkEndCommandBuffer(computeCmd));
	std::array<VkSemaphore, 1> computeSignalSemaphores = { nextComputeSync->renderSemaphore };
	VkPipelineStageFlags computeWaitStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	VkSubmitInfo computeSubmit = {};
	computeSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	computeSubmit.pWaitDstStageMask = &computeWaitStage;
	computeSubmit.signalSemaphoreCount = computeSignalSemaphores.size();
	computeSubmit.pSignalSemaphores = computeSignalSemaphores.data();
	computeSubmit.commandBufferCount = 1;
	computeSubmit.pCommandBuffers = &computeCmd;

	vkQueueSubmit(computeQueue, 1, &computeSubmit, nextComputeSync->renderFence);

	// graphics pipeline
	// wait until the GPU has finished rendering the last frame. Timeout of 1 second
	VK_CHECK(vkWaitForFences(device, 1, &nextSync->renderFence, true, ONE_SECOND));
	VK_CHECK(vkResetFences(device, 1, &nextSync->renderFence));
	//request image from the swapchain, one second timeout
	uint32_t swapchainImageIndex;
	VK_CHECK(vkAcquireNextImageKHR(device, swapchain, ONE_SECOND, nextSync->renderSemaphore, nullptr, &swapchainImageIndex));
	VK_CHECK(vkResetCommandBuffer(nextSync->mainCommandBuffer, 0));

	//naming it cmd for shorter writing
	VkCommandBuffer cmd = nextSync->mainCommandBuffer;

	// TODO: move this to compute shader?
	if (resetParticle)
	{
		resetParticle = false;
		resetParticleInfo(nextSync->commandPool, graphicsQueue);
	}

	//begin the command buffer recording. We will use this command buffer exactly once, so we want to let Vulkan know that
	VkCommandBufferBeginInfo cmdBeginInfo = {};
	cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBeginInfo.pNext = nullptr;
	cmdBeginInfo.pInheritanceInfo = nullptr;
	cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	//make a clear-color from frame number. This will flash with a 120*pi frame period.
	VkClearValue clearValue;
	float flash = abs(sin(frameNumber / 120.f));
	clearValue.color = { { 0.0f, 0.0f, flash, 1.0f } };

	//clear depth at 1
	VkClearValue depthClear;
	depthClear.depthStencil.depth = 1.f;

	VkClearValue clearValues[] = { clearValue, depthClear };
	//start the main renderpass.
	//We will use the clear color from above, and the framebuffer of the index the swapchain gave us
	VkRenderPassBeginInfo rpInfo = {};
	rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpInfo.pNext = nullptr;

	rpInfo.renderPass = renderPass;
	rpInfo.renderArea.offset.x = 0;
	rpInfo.renderArea.offset.y = 0;
	rpInfo.renderArea.extent = windowExtent;
	rpInfo.framebuffer = framebuffers[swapchainImageIndex];
	//connect clear values
	rpInfo.clearValueCount = 2;
	rpInfo.pClearValues = &clearValues[0];

	// get transform matrix
	ubo.model = glm::rotate(glm::mat4{ 1.0f }, glm::radians(frameNumber * 0.4f), glm::vec3(0, 1, 0));
	ubo.view = cameraPtr->getViewMatrix();


	// begin render pass
	vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

	
	{
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, renderObjects[0].pipelineSet->pipeline);
		//bind the mesh vertex buffer with offset 0
		VkDeviceSize offset = 0;
		//vkCmdBindVertexBuffers(cmd, 0, 1, &renderObjects[0].mesh->vertexBuffer.buffer, &offset);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, renderObjects[0].pipelineSet->pipelineLayout, 0, 1, &vertexShaderDescriptors[CURRENT_FRAME], 0, nullptr);

		//and copy it to the buffer
		void* data;
		vmaMapMemory(allocator, buffers[CURRENT_FRAME].allocation, &data);

		memcpy(data, &ubo, sizeof(UniformBuffer));

		vmaUnmapMemory(allocator, buffers[CURRENT_FRAME].allocation);

		//we can now draw the mesh
		// vkCmdDraw(cmd, renderObjects[0].mesh->vertices.size(), 1, 0, 0);

		// draw the sphere
		vkCmdBindVertexBuffers(cmd, 0, 1, &renderObjects[1].mesh->vertexBuffer.buffer, &offset);
		vkCmdBindIndexBuffer(cmd, renderObjects[1].mesh->indiceBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(cmd, static_cast<uint32_t>(renderObjects[1].mesh->indices.size()), MAX_INSTANCE, 0, 0, 0);

	}
	
	//finalize the render pass
	vkCmdEndRenderPass(cmd);
	//finalize the command buffer (we can no longer add commands, but it can now be executed)
	VK_CHECK(vkEndCommandBuffer(cmd));

	//prepare the submission to the queue.
	//we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
	//we will signal the _renderSemaphore, to signal that rendering has finished

	//nextSync->presentSemaphore we wait for get the current framebuffer, nextComputeSync->renderSemaphore wair for compute is done
	std::array<VkSemaphore, 2> waitSemaphores = { nextComputeSync->renderSemaphore, nextSync->renderSemaphore };
	std::array<VkSemaphore, 1> signalSemaphores = { nextSync->presentSemaphore };
	VkPipelineStageFlags graphicsWaitStage[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submit = {};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.pNext = nullptr;
	submit.pWaitDstStageMask = graphicsWaitStage;
	submit.waitSemaphoreCount = waitSemaphores.size();
	submit.pWaitSemaphores = waitSemaphores.data();
	submit.signalSemaphoreCount = signalSemaphores.size();
	submit.pSignalSemaphores = signalSemaphores.data();
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &cmd;

	//submit command buffer to the queue and execute it.
	// _renderFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submit, nextSync->renderFence));
	// this will put the image we just rendered into the visible window.
	// we want to wait on the _renderSemaphore for that,
	// as it's necessary that drawing commands have finished before the image is displayed to the user
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.swapchainCount = 1;
	presentInfo.pWaitSemaphores = &nextSync->presentSemaphore;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pImageIndices = &swapchainImageIndex;

	VK_CHECK(vkQueuePresentKHR(graphicsQueue, &presentInfo));
	
	//increase the number of frames drawn
	frameNumber++;
}

SystemType VulkanEngine::Type() const
{
	return SystemType::GRAPHICS;
}

bool VulkanEngine::loadShaderModule(const char* filePath, VkShaderModule* outShaderModule)
{
	//open the file. With cursor at the end
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		return false;
	}

	//find what the size of the file is by looking up the location of the cursor
	//because the cursor is at the end, it gives the size directly in bytes
	size_t fileSize = (size_t)file.tellg();

	//spirv expects the buffer to be on uint32, so make sure to reserve an int vector big enough for the entire file
	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

	//put file cursor at beginning
	file.seekg(0);

	//load the entire file into the buffer
	file.read((char*)buffer.data(), fileSize);

	//now that the file is loaded into the buffer, we can close it
	file.close();

	//create a new shader module, using the buffer we loaded
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext = nullptr;

	//codeSize has to be in bytes, so multiply the ints in the buffer by size of int to know the real size of the buffer
	createInfo.codeSize = buffer.size() * sizeof(uint32_t);
	createInfo.pCode = buffer.data();

	//check that the creation goes well.
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) 
		return false;
	*outShaderModule = shaderModule;
	return true;
}

void VulkanEngine::loadShaderWrapper(std::string shaderName, VkShaderModule* outShaderModule)
{
	std::string path = "../../shaders/" + shaderName + ".spv";

	if (!loadShaderModule(path.c_str(), outShaderModule))
	{
		std::cout << "Error when building the " + shaderName + "shader module" << std::endl;
	}
	else {
		std::cout << shaderName + " successfully loaded" << std::endl;
	}
}

void VulkanEngine::loadMeshes()
{
	meshes["Monkey"].loadFromOBJ("../../assets/monkey_smooth.obj");
	generateSphere(meshes["Sphere"], 20);
	// upload the mesh to GPU
	uploadMesh(meshes["Monkey"]);
	uploadMesh(meshes["Sphere"], true);
}

void VulkanEngine::uploadMesh(Mesh& mesh, bool indices)
{
	//allocate vertex buffer
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = mesh.vertices.size() * sizeof(Vertex);
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;


	// TODO: change this to GPU only and pass the info use staging buffer.
	VmaAllocationCreateInfo vmaAllocInfo = {};
	vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

	// allocate the buffer
	VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaAllocInfo, &mesh.vertexBuffer.buffer, &mesh.vertexBuffer.allocation, nullptr));

	// add the destruction of triangle mesh buffer to the deletion queue
	deletionQueue.pushFunction([=]() { vmaDestroyBuffer(allocator, mesh.vertexBuffer.buffer, mesh.vertexBuffer.allocation); });

	// copy the data to gpu
	// It is possible to keep the pointer mapped and not unmap it immediately, but that is an advanced technique mostly used for streaming data, which we don’t need right now.
	void * data;
	vmaMapMemory(allocator, mesh.vertexBuffer.allocation, &data);
	memcpy(data, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));
	vmaUnmapMemory(allocator, mesh.vertexBuffer.allocation);

	// if has indices buffer
	if (indices)
	{
		//allocate vertex buffer
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = mesh.indices.size() * sizeof(uint32_t);
		bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

		// allocate the buffer
		VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaAllocInfo, &mesh.indiceBuffer.buffer, &mesh.indiceBuffer.allocation, nullptr));

		// add the destruction of triangle mesh buffer to the deletion queue
		deletionQueue.pushFunction([=]() { vmaDestroyBuffer(allocator, mesh.indiceBuffer.buffer, mesh.indiceBuffer.allocation); });

		// copy the data to gpu
		void* data;
		vmaMapMemory(allocator, mesh.indiceBuffer.allocation, &data);
		memcpy(data, mesh.indices.data(), mesh.indices.size() * sizeof(uint32_t));
		vmaUnmapMemory(allocator, mesh.indiceBuffer.allocation);
	}
}

void VulkanEngine::initVulkan()
{
	vkb::InstanceBuilder builder;

	// instance
	auto instRet = builder.set_app_name("Graphics Playground")
						  .request_validation_layers(true) // change it when not debug mode?
						  .require_api_version(1, 3, 0)
						  .use_default_debug_messenger()
						  .build();

	vkb::Instance vkbInst = instRet.value();

	instance = vkbInst.instance;
	debugMessenger = vkbInst.debug_messenger;


	// device
	SDL_Vulkan_CreateSurface(window, instance, &surface);

	//use vkbootstrap to select a GPU.
	//We want a GPU that can write to the SDL surface and supports Vulkan 1.3
	vkb::PhysicalDeviceSelector selector{ vkbInst };
	vkb::PhysicalDevice physicalDevice = selector.set_minimum_version(1, 3)
												 .set_surface(surface)
												 .select()
												 .value();

	//create the final Vulkan device
	vkb::DeviceBuilder deviceBuilder{ physicalDevice };

	vkb::Device vkbDevice = deviceBuilder.build().value();

	// Get the VkDevice handle used in the rest of a Vulkan application
	device = vkbDevice.device;
	gpuDevice = physicalDevice.physical_device;

	// use vkbootstrap to get a Graphics queue
	graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

	// get a compute queue, this will error out if does not support compute shader
	computeQueue = vkbDevice.get_queue(vkb::QueueType::compute).value();
	computeQueueFramily = vkbDevice.get_queue_index(vkb::QueueType::compute).value();

	// initialize the memory allocator
	VmaAllocatorCreateInfo allocatorInfo{};
	allocatorInfo.physicalDevice = gpuDevice;
	allocatorInfo.device = device;
	allocatorInfo.instance = instance;
	vmaCreateAllocator(&allocatorInfo, &allocator);
}

void VulkanEngine::initSwapchain()
{
	vkb::SwapchainBuilder swapchainBuilder{gpuDevice, device, surface};

	vkb::Swapchain vkbSwapchain = swapchainBuilder.use_default_format_selection()
												  .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
												  .set_desired_extent(windowExtent.width, windowExtent.height)
												  .build()
												  .value();
	
	//store swapchain and its related images
	swapchain = vkbSwapchain.swapchain;
	swapchainImages = vkbSwapchain.get_images().value();
	swapchainImageViews = vkbSwapchain.get_image_views().value();

	swapchainImageFormat = vkbSwapchain.image_format;

	//depth image size will match the window
	VkExtent3D depthImageExtent = { windowExtent.width, windowExtent.height, 1 };

	//hardcoding the depth format to 32 bit float
	depthFormat = VK_FORMAT_D32_SFLOAT;

	//the depth image will be an image with the format we selected and Depth Attachment usage flag
	VkImageCreateInfo dimgInfo = vkinit::imageCreateInfo(depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);

	//for the depth image, we want to allocate it from GPU local memory
	VmaAllocationCreateInfo dimgAllocinfo = {};
	dimgAllocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	dimgAllocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//allocate and create the image
	vmaCreateImage(allocator, &dimgInfo, &dimgAllocinfo, &depthImage.image, &depthImage.allocation, nullptr);

	//build an image-view for the depth image to use for rendering
	VkImageViewCreateInfo dviewInfo = vkinit::imageviewCreateInfo(depthFormat, depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

	VK_CHECK(vkCreateImageView(device, &dviewInfo, nullptr, &depthImageView));

	// destory function
	deletionQueue.pushFunction([=]() { vkDestroyImageView(device, depthImageView, nullptr);
									   vmaDestroyImage(allocator, depthImage.image, depthImage.allocation);
									   vkDestroySwapchainKHR(device, swapchain, nullptr); });
}



void VulkanEngine::initDefaultRenderpass()
{
	// the renderpass will use this color attachment.
	VkAttachmentDescription colorAttachment = {};
	//the attachment will have the format needed by the swapchain
	colorAttachment.format = swapchainImageFormat;
	//1 sample, we won't be doing MSAA
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	// we Clear when this attachment is loaded
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	// we keep the attachment stored when the renderpass ends
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	//we don't care about stencil
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	//we don't know or care about the starting layout of the attachment
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	//after the renderpass ends, the image has to be on a layout ready for display
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	//attachment number will index into the pAttachments array in the parent renderpass itself
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// create depth attachment
	VkAttachmentDescription depthAttachment = {};
	depthAttachment.flags = 0;
	depthAttachment.format = depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	//attachment number will index into the pAttachments array in the parent renderpass itself
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	//we are going to create 1 subpass, which is the minimum you can do
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkAttachmentDescription attachments[2] = { colorAttachment, depthAttachment };

	VkSubpassDependency dependecy = {};
	dependecy.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependecy.dstSubpass = 0;
	dependecy.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependecy.srcAccessMask = 0;
	dependecy.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependecy.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkSubpassDependency depthDependency = {};
	depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	depthDependency.dstSubpass = 0;
	depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	depthDependency.srcAccessMask = 0;
	depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkSubpassDependency dependencies[2] = { dependecy, depthDependency };

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	//connect the color attachment to the info
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = &attachments[0];
	//connect the subpass to the info
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 2;
	renderPassInfo.pDependencies = &dependencies[0];


	VK_CHECK(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
	deletionQueue.pushFunction([=]() { vkDestroyRenderPass(device, renderPass, nullptr); });
}

void VulkanEngine::initFrameBuffers()
{
	//create the framebuffers for the swapchain images. This will connect the render-pass to the images for rendering
	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.pNext = nullptr;

	framebufferInfo.renderPass = renderPass;
	framebufferInfo.attachmentCount = 1;
	framebufferInfo.width = windowExtent.width;
	framebufferInfo.height = windowExtent.height;
	framebufferInfo.layers = 1;

	//grab how many images we have in the swapchain
	const uint32_t swapchainImagecount = swapchainImages.size();
	framebuffers = std::vector<VkFramebuffer>(swapchainImagecount);

	//create framebuffers for each of the swapchain image views
	for (int i = 0; i < swapchainImagecount; i++) 
	{
		VkImageView attachments[2] = { swapchainImageViews[i], depthImageView };
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.attachmentCount = 2;

		VK_CHECK(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers[i]));
		deletionQueue.pushFunction([=]() { vkDestroyFramebuffer(device, framebuffers[i], nullptr);
										   vkDestroyImageView(device, swapchainImageViews[i], nullptr); });
	}

}

void VulkanEngine::initSyncStructures()
{
	// this also init the command buffer stuff
	graphicsQueueRingBuffer.initSyncObjects(MAX_FRAMES_IN_FLIGHT, device, graphicsQueueFamily);
	computeQueueRingBuffer.initSyncObjects(1, device, computeQueueFramily); // currently only one buffer for compute pipeline

}

PipelineSet* VulkanEngine::recordPipelineSet(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name)
{
	PipelineSet mat;
	mat.pipeline = pipeline;
	mat.pipelineLayout = layout;
	pipelineSets[name] = mat;
	return &pipelineSets[name];
}

PipelineSet* VulkanEngine::getPipelineSet(const std::string& name)
{
	//search for the object, and return nullptr if not found
	auto it = pipelineSets.find(name);
	if (it == pipelineSets.end())
		return nullptr;
	else
		return &(*it).second;
}


Mesh* VulkanEngine::getMesh(const std::string& name)
{
	auto it = meshes.find(name);
	if (it == meshes.end())
		return nullptr;
	else
		return &(*it).second;
}


void VulkanEngine::initPipeline()
{

	VkShaderModule redTriangleFragShader;
	loadShaderWrapper("colorTriangle.frag", &redTriangleFragShader);

	// load mesh vertex shader
	VkShaderModule meshVertShader;
	loadShaderWrapper("triMesh.vert", &meshVertShader);


	VkPipelineLayout meshPipelineLayout;
	VkPipeline meshPipeline;
	// build the mesh pipeline
	VkPipelineLayoutCreateInfo meshPipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo();

	meshPipelineLayoutInfo.setLayoutCount = 1;
	meshPipelineLayoutInfo.pSetLayouts = &graphicsSetLayout;

	VK_CHECK(vkCreatePipelineLayout(device, &meshPipelineLayoutInfo, nullptr, &meshPipelineLayout));



	//build the stage-create-info for both vertex and fragment stages. This lets the pipeline know the shader modules per stage
	PipelineBuilder pipelineBuilder;

	//vertex input controls how to read vertices from vertex buffers
	pipelineBuilder.vertexInputInfo = vkinit::vertexInputStateCreateInfo();
	pipelineBuilder.inputAssembly = vkinit::inputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	//build viewport and scissor from the swapchain extents
	pipelineBuilder.viewport.x = 0.0f;
	pipelineBuilder.viewport.y = 0.0f;
	pipelineBuilder.viewport.width = (float)windowExtent.width;
	pipelineBuilder.viewport.height = (float)windowExtent.height;
	pipelineBuilder.viewport.minDepth = 0.0f;
	pipelineBuilder.viewport.maxDepth = 1.0f;
	pipelineBuilder.scissor.offset = { 0, 0 };
	pipelineBuilder.scissor.extent = windowExtent;
	//configure the rasterizer to draw filled triangles
	pipelineBuilder.rasterizer = vkinit::rasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
	//we don't use multisampling, so just run the default one
	pipelineBuilder.multisampling = vkinit::multisamplingStateCreateInfo();
	//a single blend attachment with no blending and writing to RGBA
	pipelineBuilder.colorBlendAttachment = vkinit::colorBlendAttachmentState();
	// enable depth test
	pipelineBuilder.depthStencil = vkinit::depthStencilCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
	VertexInputDescription vertexDescription = Vertex::getVertexDescription();
	// connect the pipeline builder vertex input info to the one we get from Vertex
	pipelineBuilder.vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
	pipelineBuilder.vertexInputInfo.vertexAttributeDescriptionCount = vertexDescription.attributes.size();

	pipelineBuilder.vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
	pipelineBuilder.vertexInputInfo.vertexBindingDescriptionCount = vertexDescription.bindings.size();

	// add the shaders
	pipelineBuilder.shaderStages.push_back(vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, meshVertShader));
	pipelineBuilder.shaderStages.push_back(vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, redTriangleFragShader));

	pipelineBuilder.pipelineLayout = meshPipelineLayout;

	//build the mesh triangle pipeline
	meshPipeline = pipelineBuilder.buildPipeline(device, renderPass);
	// save this pair here
	recordPipelineSet(meshPipeline, meshPipelineLayout, "GraphicsPipeline");

	// pipeline for compute shader
	VkShaderModule densityComputeShader;
	loadShaderWrapper("densityCompute.comp", &densityComputeShader);

	VkPipelineLayout computePipelineLayout;
	VkPipeline computePipeline;

	// build the compute pipeline
	VkPipelineLayoutCreateInfo computePipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo();
	// push the delta time to compute shader
	VkPushConstantRange pushConstant;
	pushConstant.offset = 0;
	pushConstant.size = sizeof(float);
	pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	computePipelineLayoutInfo.setLayoutCount = 1;
	computePipelineLayoutInfo.pSetLayouts = &computeSetLayout;
	computePipelineLayoutInfo.pPushConstantRanges = &pushConstant;
	computePipelineLayoutInfo.pushConstantRangeCount = 1;

	VK_CHECK(vkCreatePipelineLayout(device, &computePipelineLayoutInfo, nullptr, &computePipelineLayout));

	// compute pipeline info
	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	pipelineInfo.stage.module = densityComputeShader;
	pipelineInfo.stage.pName = "main"; // Entry point in the shader
	pipelineInfo.layout = computePipelineLayout; // Pipeline layout

	VK_CHECK(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline));
	recordPipelineSet(computePipeline, computePipelineLayout, "ComputePipeline");


	//deleting all of the vulkan shaders
	vkDestroyShaderModule(device, meshVertShader, nullptr);
	vkDestroyShaderModule(device, redTriangleFragShader, nullptr);
	vkDestroyShaderModule(device, densityComputeShader, nullptr);

	// destroy the pipelines we have created
	deletionQueue.pushFunction([=]() { vkDestroyPipeline(device, meshPipeline, nullptr);
									   vkDestroyPipeline(device, computePipeline, nullptr);
									   vkDestroyPipelineLayout(device, meshPipelineLayout, nullptr);
									   vkDestroyPipelineLayout(device, computePipelineLayout, nullptr); });
}


void VulkanEngine::initScene()
{
	RenderObject monkey;
	monkey.mesh = getMesh("Monkey");
	monkey.pipelineSet = getPipelineSet("GraphicsPipeline");
	monkey.transformMatrix = glm::mat4(1.f);

	renderObjects.push_back(monkey);

	RenderObject sphere;
	sphere.mesh = getMesh("Sphere");
	sphere.pipelineSet = getPipelineSet("GraphicsPipeline");
	sphere.transformMatrix = glm::mat4(1.f);

	renderObjects.push_back(sphere);
}

void VulkanEngine::initComputeBuffer() 
{
	// this will used for storing all particles information
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = sizeof(StorageBuffer);
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
						VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | 
						VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; // Used in compute and graphics pipelines

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	// allocate the buffer
	VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &StorageBuffer::storageBuffer.buffer, &StorageBuffer::storageBuffer.allocation, nullptr));

	// add the destruction of buffer to the deletion queue
	deletionQueue.pushFunction([=]() { vmaDestroyBuffer(allocator, StorageBuffer::storageBuffer.buffer, StorageBuffer::storageBuffer.allocation); });

	// init partiles info and upload to the buffer
	// resetParticleInfo();
}
void VulkanEngine::resetParticleInfo(VkCommandPool cmdPool, VkQueue queue)
{
	// TODO: move the init particles pos to GPU?
	StorageBuffer buffer = {};
	float goldenRatio = (1.0f + std::sqrt(5.0f)) / 2.0f;
	float angleIncrement = 2.0f * M_PI * goldenRatio;

	for (size_t i = 0; i < MAX_INSTANCE; i++)
	{
		float t = float(i) / float(MAX_INSTANCE);
		float inclination = std::acos(1.0f - 2.0f * t) * 5;
		float azimuth = angleIncrement * i;

		buffer.particles[i].pos.x = std::sin(inclination) * std::cos(azimuth);
		buffer.particles[i].pos.y = std::sin(inclination) * std::sin(azimuth);
		buffer.particles[i].pos.z = std::cos(inclination);
		buffer.particles[i].pos.w = 1.f;

		buffer.particles[i].velocity = glm::vec4(0, -9.8f, 0, 0);
	}

	// create a staging buffer
	VkBuffer stagingBuffer;
	VmaAllocation stagingBufferAllocation;

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = sizeof(StorageBuffer);
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU; // Allows CPU access

	VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &stagingBuffer, &stagingBufferAllocation, nullptr));

	// Map the buffer and copy the initial data
	void* data;
	vmaMapMemory(allocator, stagingBufferAllocation, &data);
	memcpy(data, &buffer, sizeof(StorageBuffer));
	vmaUnmapMemory(allocator, stagingBufferAllocation);

	// upload the data to gpu
	copyBuffer(cmdPool, queue, stagingBuffer, buffer.storageBuffer.buffer, sizeof(StorageBuffer));

	vmaDestroyBuffer(allocator, stagingBuffer, stagingBufferAllocation);
}

void VulkanEngine::copyBuffer(VkCommandPool cmdPool, VkQueue queue,VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBufferAllocateInfo allocaInfo{};
	allocaInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocaInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocaInfo.commandBufferCount = 1;
	allocaInfo.commandPool = cmdPool;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocaInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(device, cmdPool, 1, &commandBuffer);
}
void VulkanEngine::initDescriptors()
{
	//information about the binding.
	VkDescriptorSetLayoutBinding camBufferBinding = {};
	camBufferBinding.binding = 0;
	camBufferBinding.descriptorCount = 1;
	camBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	camBufferBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding particleInfoBinding{};
	particleInfoBinding.binding = 1;
	particleInfoBinding.descriptorCount = 1;
	particleInfoBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	particleInfoBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // for graphics pipeline, only read this in vertex shader
	particleInfoBinding.pImmutableSamplers = nullptr;

	std::array<VkDescriptorSetLayoutBinding, 2> graphicsBindings = { camBufferBinding, particleInfoBinding };

	VkDescriptorSetLayoutCreateInfo setinfo = {};
	setinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	setinfo.pNext = nullptr;
	setinfo.bindingCount = static_cast<uint32_t>(graphicsBindings.size());
	setinfo.flags = 0;
	setinfo.pBindings = graphicsBindings.data();

	vkCreateDescriptorSetLayout(device, &setinfo, nullptr, &graphicsSetLayout);

	// add descriptor for compute shader
	VkDescriptorSetLayoutBinding storageBufferBinding = {};
	storageBufferBinding.binding = 0;
	storageBufferBinding.descriptorCount = 1;
	storageBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	storageBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	std::array<VkDescriptorSetLayoutBinding, 1> computeBindings = { storageBufferBinding };

	setinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	setinfo.pNext = nullptr;
	setinfo.bindingCount = static_cast<uint32_t>(computeBindings.size());
	setinfo.flags = 0;
	setinfo.pBindings = computeBindings.data();

	vkCreateDescriptorSetLayout(device, &setinfo, nullptr, &computeSetLayout);


	std::vector<VkDescriptorPoolSize> sizes =
	{
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 },
	};

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = 0;
	poolInfo.maxSets = 10;
	poolInfo.poolSizeCount = (uint32_t)sizes.size();
	poolInfo.pPoolSizes = sizes.data();

	vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);

	buffers.resize(MAX_FRAMES_IN_FLIGHT);
	vertexShaderDescriptors.resize(MAX_FRAMES_IN_FLIGHT);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		buffers[i] = vkinit::createBuffer(allocator, sizeof(UniformBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		// allocate one descriptor set for each frame
		VkDescriptorSetAllocateInfo allocateInfo = {};
		allocateInfo.pNext = nullptr;
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorPool = descriptorPool;
		allocateInfo.descriptorSetCount = 1;
		allocateInfo.pSetLayouts = &graphicsSetLayout;

		vkAllocateDescriptorSets(device, &allocateInfo, &vertexShaderDescriptors[i]);

		// info about the buffer we want to point at in the descriptor
		VkDescriptorBufferInfo bufferInfo;
		bufferInfo.buffer = buffers[i].buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBuffer);

		VkWriteDescriptorSet setWrite = {};
		setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		setWrite.pNext = nullptr;
		setWrite.dstBinding = 0;
		setWrite.dstSet = vertexShaderDescriptors[i];
		setWrite.descriptorCount = 1;
		// type of buffer
		setWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		setWrite.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(device, 1, &setWrite, 0, nullptr);

		// storage buffer info
		bufferInfo.buffer = StorageBuffer::storageBuffer.buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(StorageBuffer);

		setWrite.dstBinding = 1;
		setWrite.dstSet = vertexShaderDescriptors[i];
		setWrite.descriptorCount = 1;
		setWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		setWrite.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(device, 1, &setWrite, 0, nullptr);
	}


	// TODO: create two storage buffer? read from one, wirte to another.
	// for compute shader
	VkDescriptorSetAllocateInfo allocateInfo = {};
	allocateInfo.pNext = nullptr;
	allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfo.descriptorPool = descriptorPool;
	allocateInfo.descriptorSetCount = 1;
	allocateInfo.pSetLayouts = &computeSetLayout;

	vkAllocateDescriptorSets(device, &allocateInfo, &computeDescriptors);

	// info about the buffer we want to point at in the descriptor
	VkDescriptorBufferInfo bufferInfo;
	bufferInfo.buffer = StorageBuffer::storageBuffer.buffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(StorageBuffer);

	VkWriteDescriptorSet setWrite = {};
	setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	setWrite.pNext = nullptr;
	setWrite.dstBinding = 0;
	setWrite.dstSet = computeDescriptors;
	setWrite.descriptorCount = 1;
	setWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	setWrite.pBufferInfo = &bufferInfo;

	vkUpdateDescriptorSets(device, 1, &setWrite, 0, nullptr);

	// add buffers to deletion queues
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		// error out if uses by reference
		deletionQueue.pushFunction([=]() {
			vmaDestroyBuffer(allocator, buffers[i].buffer, buffers[i].allocation);
			});
	}

	// vmaDestroyBuffer(allocator, );

	// add descriptor set layout to deletion queues
	deletionQueue.pushFunction([=]() {
		vkDestroyDescriptorSetLayout(device, graphicsSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, computeSetLayout, nullptr);
		vkDestroyDescriptorPool(device, descriptorPool, nullptr);
		});
}

