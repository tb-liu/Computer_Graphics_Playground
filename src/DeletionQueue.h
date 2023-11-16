#pragma once
#include<vector>
#include <deque>
#include <functional>

// a painless way to destory everything
struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void pushFunction(std::function<void()> && function);
	void flush();
};