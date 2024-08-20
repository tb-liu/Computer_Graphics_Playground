#include "InputManager.h"
#include "vk_engine.h"


std::map<SDL_Keycode, KeyStates> InputGlobal::keyboardStates;

void InputManager::init()
{
}

void InputManager::update(float)
{
	// clear the states from previous update
	InputGlobal::keyboardStates.clear();
	SDL_Event e;

	//Handle events on queue
	while (SDL_PollEvent(&e) != 0)
	{
		//close the window when user alt-f4s or clicks the X button			
		if (e.type == SDL_QUIT) bQuit = true;
		else if (e.type == SDL_KEYDOWN)
		{
			// esacpe key to exit program
			if (e.key.keysym.sym == SDLK_ESCAPE) bQuit = true;
			InputGlobal::keyboardStates[e.key.keysym.sym].pressed = true;
			printf("Key pressed detected (%c)\n", e.key.keysym.sym);
		}
		else if (e.type == SDL_KEYUP)
			InputGlobal::keyboardStates[e.key.keysym.sym].released = true;
	}
	// if space pressed then change shader
	if (InputGlobal::isKeyPressed(SDLK_SPACE))
	{
		GraphicsGlobal::SELECTED_SHADER += 1;
		GraphicsGlobal::SELECTED_SHADER %= GraphicsGlobal::MAX_SHADER_COUNT;
	}
}

void InputManager::shutdown()
{
}

SystemType InputManager::Type() const
{
    return SystemType::INPUT;
}


InputManager::InputManager():bQuit(false)
{
}

InputManager::~InputManager()
{
}


KeyStates::KeyStates(bool press, bool release):pressed(press), released(release)
{
}
