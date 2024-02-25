#pragma once
#include "vk_engine.h"
#include "SystemBase.h"
#include <array>

// Forward Declarations
//enum class SystemType;
//class SystemBase;
typedef SystemBase* SystemPtr;

class Engine
{
public:
	void init();
	void load();
	void update(float dt);
	void unload();
	void exit();

	Engine();
	~Engine();
	Engine(const Engine& Engine) = delete;


	// Helper functions
	void addSystem(SystemPtr sys, SystemType type);
	SystemPtr getSystem(SystemType type);
	static Engine* getInstance();

private:
	static Engine* instancePtr;
	std::array<SystemPtr, static_cast<size_t>(SystemType::MAX)> systems;

	VkExtent2D windowExtent{ 800 , 600 };
	struct SDL_Window* window{ nullptr };

#pragma region FrameRate
	float frameBegin = 0.0f;
	float frameEnd = 0.0f;
	const float FrameCap = 1.0f / 240.f;

	void getDT(float& dt);
#pragma endregion FrameRate
};