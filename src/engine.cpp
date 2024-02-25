#include "Engine.h"
#include "glm/glm.hpp"

#include <SDL.h>
#include <SDL_vulkan.h>

// All systems here
#include "Camera.h"
// Initialization of the static window variable & the engine Instance pointer
Engine* Engine::instancePtr = nullptr;


void Engine::init()
{

    for (SystemPtr sys : systems)
    {
        if (sys != nullptr)
        {
            sys->init();
        }
    }
}

void Engine::load()
{
    // We initialize SDL and create a window with it. 
    SDL_Init(SDL_INIT_VIDEO);

    SDL_WindowFlags windowFlags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    window = SDL_CreateWindow(
        "Playground",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        windowExtent.width,
        windowExtent.height,
        windowFlags
    );

    // Add the above systems to the systems array of the Engine
    addSystem(camera, SystemType::CAMERA);
    addSystem(graphics, SystemType::GRAPHICS);

}

void Engine::update(float dt)
{
    SDL_Event e;
    bool bQuit = false;

    //main loop
    while (!bQuit)
    {
        //Handle events on queue
        while (SDL_PollEvent(&e) != 0)
        {
            //close the window when user alt-f4s or clicks the X button			
            if (e.type == SDL_QUIT) bQuit = true;
            else if (e.type == SDL_KEYDOWN)
            {
                // esacpe key to exit program
                if (e.key.keysym.sym == SDLK_ESCAPE) bQuit = true;
                // use space key to switch shader
                // TODO: add a event system so other class can react to key press
                /*if (e.key.keysym.sym == SDLK_SPACE)
                {
                    SELECTED_SHADER += 1;
                    SELECTED_SHADER %= MAX_SHADER_COUNT;
                }*/

            }
        }

        for (SystemPtr sys : systems)
        {
            if (sys != nullptr)
            {
                sys->update(dt);
            }
        }
    }
}

void Engine::getDT(float& dt)
{
    // Get the end time of the last frame
    frameEnd = static_cast<float>(glfwGetTime());
    dt = frameEnd - frameBegin;

    //Get the start time of the current frame
    frameBegin = static_cast<float>(glfwGetTime());

    dt = glm::clamp(dt, 0.0f, FrameCap);
}

void Engine::unload()
{
    // shutdown all in reverse order
    int lastSys = (int)systems.max_size() - 1;
    for (int i = lastSys; i >= 0; --i)
    {
        if (systems[i] != nullptr)
        {
            systems[i]->shutdown();
        }
    }

    Log::trace("Engine: Shutdown");
}

void Engine::exit()
{
    // Once all systems are shutdown
    // free them all in reverse order
    int lastSys = (int)systems.max_size() - 1;
    for (int i = lastSys; i >= 0; --i)
    {
        if (systems[i] != nullptr)
        {
            free(systems[i]);
            systems[i] = nullptr;
        }
    }
    Log::shutdown();
    SDL_DestroyWindow(window);
}

Engine::Engine() :systems()
{}

Engine::~Engine()
{
    free(instancePtr);
    instancePtr = nullptr;
}

// Used to set the window from anywhere
void Engine::setWindow(GLFWwindow* win)
{
    window = win;
}

void Engine::addSystem(SystemPtr sys, SystemType type)
{
    // Will do something better in the future here
    assert(static_cast<size_t>(type) < static_cast<size_t>(SystemType::MAX));
    systems[static_cast<size_t>(type)] = sys;
}

SystemPtr Engine::getSystem(SystemType type)
{
    return systems[static_cast<size_t>(type)];
}

Engine* Engine::getInstance()
{
    if (instancePtr == nullptr)
    {
        instancePtr = new Engine();
    }

    return instancePtr;
}

GLFWwindow* Engine::getWindow()
{
    return window;
}
