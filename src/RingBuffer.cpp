#include "RingBuffer.h"
#include "Defines.h"
#include "vk_initializers.h"

FrameData::FrameData()
{
}

void FrameData::initSyncObjects(int max, VkDevice device, uint32_t graphicsQueueFamily)
{
	this->device = device;
	syncObjects.resize(max);
	currentIndex = 0;
	maxObjectNum = max;
	//create synchronization structures

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.pNext = nullptr;

	//we want to create the fence with the Create Signaled flag, so we can wait on it before using it on a GPU command (for the first frame)
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	//for the semaphores we don't need any flags
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = nullptr;
	semaphoreCreateInfo.flags = 0;

	//create a command pool for commands submitted to the graphics queue.
	//we also want the pool to allow for resetting of individual command buffers
	VkCommandPoolCreateInfo commandPoolInfo = vkinit::commandPoolCreateInfo(graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);



	
	for (size_t i = 0; i < maxObjectNum; i++)
	{
		VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &syncObjects[i].renderFence));
		VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &syncObjects[i].presentSemaphore));
		VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &syncObjects[i].renderSemaphore));

		VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &syncObjects[i].commandPool));
		//allocate the default command buffer that we will use for rendering
		VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::commandBufferAllocateInfo(syncObjects[i].commandPool, 1);

		VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &syncObjects[i].mainCommandBuffer));
	}
	
}

void FrameData::initBuffers(VmaAllocator allocator, size_t bufferSize)
{
	this->allocator = allocator;
	buffers.resize(this->maxObjectNum);
	for (int i = 0; i < maxObjectNum; i++)
	{
		buffers[i] = vkinit::createBuffer(allocator, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	}

}

void FrameData::cleanUpSyncObjects()
{
	for (int i = 0; i < maxObjectNum; i++)
	{
		vkDestroyFence(device, syncObjects[i].renderFence, nullptr);
		vkDestroySemaphore(device, syncObjects[i].renderSemaphore, nullptr);
		vkDestroySemaphore(device, syncObjects[i].presentSemaphore, nullptr);
		vkDestroyCommandPool(device, syncObjects[i].commandPool, nullptr);
	}
}

void FrameData::cleanUpBuffers()
{
	for (int i = 0; i < maxObjectNum; i++)
	{
		vmaDestroyBuffer(allocator, buffers[i].buffer, buffers[i].allocation);
	}
}

FrameData::~FrameData()
{	
}


SyncObject* FrameData::getNextObject()
{
	int index = currentIndex;
	currentIndex = currentIndex++ % maxObjectNum;
	return &syncObjects[index];
}

