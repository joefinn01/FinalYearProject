#include "InputManager.h"
#include "Helpers/DebugHelper.h"

Tag tag = L"InputManager";

void InputManager::Subscribe(InputObserver observer)
{
	m_Observers.push_back(observer);
}

bool InputManager::Unsubscribe(InputObserver observer)
{
	for (std::vector<InputObserver>::iterator it = m_Observers.begin(); it != m_Observers.end(); ++it)
	{
		if (*it == observer)
		{
			m_Observers.erase(it);

			return true;
		}
	}

	LOG_WARNING(tag, L"Tried to unsubscribe but the input observer doesn't exist!");

	return false;
}

bool InputManager::GetKeyState(int iKeycode)
{
	if (iKeycode < 0 || iKeycode > 254)
	{
		LOG_ERROR(tag, L"Tried to get the state of a key with the keycode %i but that key doesn't exist!", iKeycode);

		return false;
	}

	return m_KeyStates[iKeycode];
}

void InputManager::Update(const Timer& kTimer)
{
	for (int i = 0; i < NUM_KEYS; ++i)
	{
		if (m_KeyStates[i] == true)
		{
			for (int j = 0; j < m_Observers.size(); ++j)
			{
				if (m_Observers[j].OnKeyHeld != nullptr)
				{
					m_Observers[j].OnKeyHeld(m_Observers[j].Object, i, kTimer);
				}
			}
		}
	}
}

void InputManager::KeyDown(int iKeycode)
{
	if (m_KeyStates[iKeycode] == false)
	{
		m_KeyStates[iKeycode] = true;

		for (int i = 0; i < m_Observers.size(); ++i)
		{
			if (m_Observers[i].OnKeyDown != nullptr)
			{
				m_Observers[i].OnKeyDown(m_Observers[i].Object, iKeycode);
			}
		}
	}
}

void InputManager::KeyUp(int iKeycode)
{
	if (m_KeyStates[iKeycode] == true)
	{
		m_KeyStates[iKeycode] = false;

		for (int i = 0; i < m_Observers.size(); ++i)
		{
			if (m_Observers[i].OnKeyUp != nullptr)
			{
				m_Observers[i].OnKeyUp(m_Observers[i].Object, iKeycode);
			}
		}
	}
}

InputManager::InputManager()
{
	for (int i = 0; i < NUM_KEYS; ++i)
	{
		m_KeyStates[i] = false;
	}
}
