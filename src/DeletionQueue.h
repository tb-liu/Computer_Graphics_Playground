#pragma once
#include<vector>
#include <deque>
#include <functional>

// make my life easier
struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void pushFunction(std::function<void()> && function);
	void flush();
};