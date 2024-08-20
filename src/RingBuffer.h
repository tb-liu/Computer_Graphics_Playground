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

class FrameData
{
public:
	FrameData();
	void initSyncObjects(int max, VkDevice device, uint32_t graphicsQueueFamily);
	void initBuffers(VmaAllocator allocator, size_t bufferSize);
	void cleanUpSyncObjects();
	void cleanUpBuffers();
	~FrameData();
	SyncObject * getNextObject();
private:
	int currentIndex;
	int maxObjectNum;
	VkDevice device;
	VmaAllocator allocator;
	// TODO: make the sync object and buffer two classes?
	std::vector<SyncObject> syncObjects;
	std::vector<AllocatedBuffer> buffers;
	std::vector<VkDescriptorSet> globalDescriptors;
};
