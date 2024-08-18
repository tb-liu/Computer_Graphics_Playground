#include "InputManager.h"

void InputManager::init()
{
}

void InputManager::update(float)
{
	// clear the states from previous update
	keyboardStates.clear();
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
			keyboardStates[e.key.keysym.sym].pressed = true;
		}
		else if (e.type == SDL_KEYUP)
			keyboardStates[e.key.keysym.sym].released = true;
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
