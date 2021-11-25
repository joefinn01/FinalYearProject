#pragma once

// I decided to keep this input manager simple as it isn't vital to the actual application.
// I could improve it by adding an extra level of abstraction between the keycodes and the actual keys allowing for rebinding of keys.

#define NUM_KEYS 255

#include "Commons/Singleton.h"
#include "Commons/Timer.h"

#include <vector>

typedef void (*InputCallback)(void*, int);
typedef void (*UpdateInputCallback)(void*, int, const Timer&);

struct InputObserver
{
	void* Object;
	InputCallback OnKeyDown;
	UpdateInputCallback OnKeyHeld;
	InputCallback OnKeyUp;

	bool operator== (InputObserver rhs)
	{
		return memcmp(this, &rhs, sizeof(InputObserver)) == 0;
	}
};

struct KeyInfo
{
	KeyInfo()
	{
		Pressed = false;
		Observers = std::vector<InputObserver>();
	}

	bool Pressed;
	std::vector<InputObserver> Observers;
};

class InputManager : public Singleton<InputManager>
{
public:
	InputManager();

	void Subscribe(InputObserver observer);
	bool Unsubscribe(InputObserver observer);

	bool GetKeyState(int iKeycode);

	void Update(const Timer& kTimer);

	void KeyDown(int iKeycode);
	void KeyUp(int iKeycode);

protected:

private:
	bool m_KeyStates[NUM_KEYS] = { false };

	std::vector<InputObserver> m_Observers;
};

