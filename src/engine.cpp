#include "engine.h"
#include "Camera.h"
#include "InputManager.h"
#include "vk_engine.h"
#include <SDL.h>
#include <algorithm>

Engine* Engine::instancePtr = nullptr;

void Engine::init()
{
    for (SystemBase * sys : systems)
        if (sys != nullptr)
            sys->init();
}

void Engine::load()
{
    // BEGIN: Add all the systems you want here
    Camera* camera = new Camera(glm::vec3(9.0f, 20.0f, 45.f), glm::vec3(0, 1, 0), -90, 0);
	InputManager* inputManager = new InputManager();
	VulkanEngine* graphics = new VulkanEngine();
    // END: Add all the systems you want here
	
    // Add the above systems to the systems array of the Engine
    addSystem(camera, camera->Type());
	addSystem(graphics, graphics->Type());
	addSystem(inputManager, inputManager->Type());


	// read the quit state from input class
	bQuit = inputManager->getQuitState();
}

void Engine::update(float dt)
{
	SDL_Event e;

	//main loop
	do
	{
		getDT(dt);
		for (SystemBase * sys : systems)
		{
			if (sys != nullptr)
				sys->update(dt);
		}
	} 
	while (!*bQuit);
}

void Engine::unload()
{
	// shutdown all in reverse order
	int lastSys = (int)systems.size() - 1;
	for (int i = lastSys; i >= 0; --i)
	{
		if (systems[i] != nullptr)
		{
			systems[i]->shutdown();
		}
	}
}

void Engine::shutdown()
{
	// Once all systems are shutdown
	// delete them all in reverse order
	int lastSys = (int)systems.size() - 1;
	for (int i = lastSys; i >= 0; --i)
	{
		if (systems[i] != nullptr)
		{
			delete systems[i];
			systems[i] = nullptr;
		}
	}
}

void Engine::addSystem(SystemBase* sys, SystemType type)
{
	assert(static_cast<size_t>(type) < static_cast<size_t>(SystemType::MAX));
	systems[static_cast<size_t>(type)] = sys;
}

SystemBase* Engine::getSystem(SystemType type)
{
	return systems[static_cast<size_t>(type)];
}

Engine* Engine::getInstance()
{
	if (instancePtr == nullptr)
		instancePtr = new Engine();

	return instancePtr;
}

Engine::Engine():bQuit(nullptr), systems()
{
	frameBegin = std::chrono::high_resolution_clock::now();
}

Engine::~Engine()
{
	delete instancePtr;
	instancePtr = nullptr;
}

void Engine::getDT(float& dt)
{
	// Get the end time of the last frame
	frameEnd = std::chrono::high_resolution_clock::now();

	// Calculate the duration between frames
	std::chrono::duration<float> duration = frameEnd - frameBegin;
	dt = duration.count();

	// Clamp the delta time to the frame cap
	dt = std::clamp(dt, 0.0f, FrameCap);

	// Get the start time of the current frame (for the next call)
	frameBegin = frameEnd;
}
