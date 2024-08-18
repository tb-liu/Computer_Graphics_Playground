#include "engine.h"
#include "Camera.h"
#include "InputManager.h"
#include "vk_engine.h"
#include <SDL.h>
#include <chrono>

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
    //Graphics* graphics = new Graphics();
    Camera* camera = new Camera(glm::vec3(9.0f, 20.0f, 45.f), glm::vec3(0, 1, 0), -90, 0);
    // END: Add all the systems you want here
	InputManager* inputManager = new InputManager();
    // Add the above systems to the systems array of the Engine
    addSystem(camera, SystemType::CAMERA);
	addSystem(inputManager, SystemType::INPUT);
    //addSystem(graphics, SystemType::GRAPHICS);


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
}

void Engine::exit()
{
}

void Engine::addSystem(SystemBase* sys, SystemType type)
{
	assert(static_cast<size_t>(type) < static_cast<size_t>(SystemType::MAX));
	systems[static_cast<size_t>(type)] = sys;
}

SystemBase* Engine::getSystem(SystemType type)
{
    return nullptr;
}

Engine* Engine::getInstance()
{
    return nullptr;
}

Engine::Engine():bQuit(nullptr), systems()
{
}

Engine::~Engine()
{
}

void Engine::getDT(float& dt)
{
}
