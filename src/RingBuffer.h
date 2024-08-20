#pragma once
#include <vector>
#include <vulkan/vulkan.h>

struct SyncObject
{
	VkSemaphore presentSemaphore, renderSemaphore;
	VkFence renderFence;

	VkCommandPool commandPool;
	VkCommandBuffer mainCommandBuffer;
};

class RingSyncObjects
{
public:
	RingSyncObjects();
	void init(int max, VkDevice device, uint32_t graphicsQueueFamily);
	void cleanUp();
	~RingSyncObjects();
	SyncObject * getNextObject();
private:
	int currentIndex;
	int maxObjectNum;
	VkDevice device;
	std::vector<SyncObject> syncObjects;
};
