#pragma once
// This should be the main loop runing the program.
// Store each component and updates
#include "SystemBase.h"
#include <array>
#include <chrono>
class Engine
{
public:
	void init();
	void load();
	void update(float dt);
	void unload();
	void shutdown();

	
	// delete copy ctor and assignment
	Engine(const Engine& engine) = delete;
	Engine& operator=(const Engine&) = delete;

	void addSystem(SystemBase * sys, SystemType type); /* This will add the system base on its register order, look at SystemType for detail order */
	SystemBase* getSystem(SystemType type);
	static Engine* getInstance();

private:
	Engine();
	~Engine();
	static Engine* instancePtr;
	const bool* bQuit;
	std::array<SystemBase*, static_cast<size_t>(SystemType::MAX)> systems;

#pragma region FrameRate
	std::chrono::high_resolution_clock::time_point frameBegin;
	std::chrono::high_resolution_clock::time_point frameEnd;
	const float FrameCap = 1.0f / 240.f;

	void getDT(float& dt);
#pragma endregion FrameRate
};
