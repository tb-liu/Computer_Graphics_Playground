#pragma once
#include <vector>
#include <vulkan/vulkan.h>

struct SyncObject
{
	VkSemaphore presentSemaphore, renderSemaphore;
	VkFence renderFence;
};

class RingSyncObjects
{
public:
	RingSyncObjects();
	void init(int max, VkDevice device);
	void cleanUp();
	~RingSyncObjects();
	SyncObject * getNextObject();
private:
	int currentIndex;
	int maxObjectNum;
	VkDevice device;
	std::vector<SyncObject> syncObjects;
	// TODO: maybe add command buffer to here as well
};
