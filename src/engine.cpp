#include "engine.h"
#include "Camera.h"
#include "vk_engine.h"
#include <SDL.h>
#include <chrono>

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

    // Add the above systems to the systems array of the Engine
    addSystem(camera, SystemType::CAMERA);
    //addSystem(graphics, SystemType::GRAPHICS);
}

void Engine::update(float dt)
{
}

void Engine::unload()
{
}

void Engine::exit()
{
}

void Engine::addSystem(SystemBase* sys, SystemType type)
{
}

SystemBase* Engine::getSystem(SystemType type)
{
    return nullptr;
}

Engine* Engine::getInstance()
{
    return nullptr;
}

Engine::Engine():systems()
{
}

Engine::~Engine()
{
}

void Engine::getDT(float& dt)
{
}
