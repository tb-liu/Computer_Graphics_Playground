#include "RingBuffer.h"
#include "Defines.h"


RingSyncObjects::RingSyncObjects()
{
}

void RingSyncObjects::init(int max, VkDevice device)
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

	for (size_t i = 0; i < maxObjectNum; i++)
	{
		VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &syncObjects[i].renderFence));
		VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &syncObjects[i].presentSemaphore));
		VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &syncObjects[i].renderSemaphore));
	}
	
}

void RingSyncObjects::cleanUp()
{
	for (int i = 0; i < maxObjectNum; i++)
	{
		vkDestroyFence(device, syncObjects[i].renderFence, nullptr);
		vkDestroySemaphore(device, syncObjects[i].renderSemaphore, nullptr);
		vkDestroySemaphore(device, syncObjects[i].presentSemaphore, nullptr);
	}
}

RingSyncObjects::~RingSyncObjects()
{	
}


SyncObject* RingSyncObjects::getNextObject()
{
	int index = currentIndex;
	currentIndex = currentIndex++ % maxObjectNum;
	return &syncObjects[index];
}

