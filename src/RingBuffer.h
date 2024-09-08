#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include <vk_types.h>
struct SyncObject
{
	VkSemaphore presentSemaphore, renderSemaphore;
	VkFence renderFence;

	VkCommandPool commandPool;
	VkCommandBuffer mainCommandBuffer;
};

class RingBuffer
{
public:
	RingBuffer();
	void initSyncObjects(int max, VkDevice device, uint32_t graphicsQueueFamily);
	void cleanUpSyncObjects();
	~RingBuffer();
	SyncObject * getNextObject();
private:
	int currentIndex;
	int maxObjectNum;
	VkDevice device;
	std::vector<SyncObject> syncObjects;

};
