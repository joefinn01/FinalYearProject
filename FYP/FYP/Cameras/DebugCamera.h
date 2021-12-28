#pragma once
#include "Camera.h"
#include "Managers/InputManager.h"

class DebugCamera : public Camera
{
public:
	DebugCamera(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 at, DirectX::XMFLOAT3 up, float fNearDepth, float fFarDepth, std::string sName);

	virtual void Update(const Timer& kTimer) override;

protected:
	bool m_bLockMouse = false;

	float m_fMouseSensitivity = 1.0f;
	float m_fMovementSpeed = 3.0f;

	InputObserver m_InputObserver;

	static void OnKeyDown(void* pObject, int iKeycode);
	static void OnKeyUp(void* pObject, int iKeycode);
	static void OnKeyHeld(void* pObject, int iKeycode, const Timer& kTimer);

private:

};

