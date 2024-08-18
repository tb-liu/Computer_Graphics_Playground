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

class InputManager: public SystemBase
{
public:
	void init() override;
	void update(float) override;
	void shutdown() override;
	SystemType Type() const override;
	inline const bool* getQuitState();
	inline const std::map<SDL_Keycode, KeyStates>& getKeyboardStates();

	InputManager();
	~InputManager();

private:
	// record exit event
	bool bQuit;
	std::map<SDL_Keycode, KeyStates> keyboardStates;
};

