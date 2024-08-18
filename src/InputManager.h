#pragma once
// wrapper around SDL input
#include "SystemBase.h"
#include <map>
#include <SDL.h>



struct KeyStates
{
	bool pressed, released;
	KeyStates(bool press = false, bool release = false);
};


namespace InputGlobal
{
	extern std::map<SDL_Keycode, KeyStates> keyboardStates;
	inline bool isKeyPressed(SDL_Keycode key)
	{
		auto it = InputGlobal::keyboardStates.find(key);
		return it != InputGlobal::keyboardStates.end() && it->second.pressed;
	}
}

class InputManager: public SystemBase
{
public:
	void init() override;
	void update(float) override;
	void shutdown() override;
	SystemType Type() const override;

	inline const bool* InputManager::getQuitState()
	{
		return &bQuit;
	}


	InputManager();
	~InputManager();

private:
	// record exit event
	bool bQuit;
};

