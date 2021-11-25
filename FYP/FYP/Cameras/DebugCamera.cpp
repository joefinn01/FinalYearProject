#include "DebugCamera.h"
#include "Commons/Timer.h"
#include "Managers/WindowManager.h"
#include "Helpers/DebugHelper.h"

#include <Windows.h>

using namespace DirectX;

Tag tag = L"DebugCamera";

DebugCamera::DebugCamera(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 at, DirectX::XMFLOAT3 up, float fNearDepth, float fFarDepth, std::string sName) : Camera(position, at, up, fNearDepth, fFarDepth, sName)
{
	if (m_bLockMouse)
	{
		//center the mouse.
		SetCursorPos(WindowManager::GetInstance()->GetWindowWidth() / 2, WindowManager::GetInstance()->GetWindowHeight() / 2);
	}

	m_InputObserver.Object = this;
	m_InputObserver.OnKeyDown = OnKeyDown;
	m_InputObserver.OnKeyUp = OnKeyUp;
	m_InputObserver.OnKeyHeld = OnKeyHeld;

	InputManager::GetInstance()->Subscribe(m_InputObserver);
}

void DebugCamera::Update(const Timer& kTimer)
{
	if (m_bLockMouse == true)
	{
		POINT mousePosition;
		GetCursorPos(&mousePosition);

		int iCentreX = WindowManager::GetInstance()->GetWindowWidth() / 2;
		int iCentreY = WindowManager::GetInstance()->GetWindowHeight() / 2;

		XMFLOAT2 deltaMousePosition = XMFLOAT2((float)(iCentreX - mousePosition.x), (float)(iCentreY - mousePosition.y));

		//Calculate the change in yaw
		XMMATRIX rotationMatrix = XMMatrixRotationY(deltaMousePosition.x * kTimer.DeltaTime() * -m_fMouseSensitivity);

		XMStoreFloat3(&m_Forward, XMVector3TransformNormal(XMLoadFloat3(&m_Forward), rotationMatrix));
		XMStoreFloat3(&m_Up, XMVector3TransformNormal(XMLoadFloat3(&m_Up), rotationMatrix));

		XMFLOAT3 right = GetRightVector();

		//Calculate the change in pitch
		rotationMatrix = XMMatrixRotationAxis(XMLoadFloat3(&right), deltaMousePosition.y * kTimer.DeltaTime() * m_fMouseSensitivity);

		XMStoreFloat3(&m_Forward, XMVector3TransformNormal(XMVector3Normalize(XMLoadFloat3(&m_Forward)), rotationMatrix));
		XMStoreFloat3(&m_Up, XMVector3TransformNormal(XMVector3Normalize(XMLoadFloat3(&m_Up)), rotationMatrix));

		//Recenter the mouse for the next frame.
		SetCursorPos(iCentreX, iCentreY);
	}

	Camera::Update(kTimer);
}

void DebugCamera::OnKeyDown(void* pObject, int iKeycode)
{
	DebugCamera* pCamera = (DebugCamera*)pObject;

	switch (iKeycode)
	{
	case VK_CONTROL:
		pCamera->m_bLockMouse = !pCamera->m_bLockMouse;

		//If toggling on then center the mouse so there isn't a big change in mouse position when it's first toggled on.
		if (pCamera->m_bLockMouse == true)
		{
			SetCursorPos(WindowManager::GetInstance()->GetWindowWidth() / 2, WindowManager::GetInstance()->GetWindowHeight() / 2);
		}
		break;

	default:
		break;
	}
}

void DebugCamera::OnKeyUp(void* pObject, int iKeycode)
{
}

void DebugCamera::OnKeyHeld(void* pObject, int iKeycode, const Timer& kTimer)
{
	DebugCamera* pCamera = (DebugCamera*)pObject;

	switch (iKeycode)
	{
	case 87:	//W
	case VK_UP:
		XMStoreFloat3(&pCamera->m_Eye, XMLoadFloat3(&pCamera->m_Eye) + XMLoadFloat3(&pCamera->GetForwardVector()) * kTimer.DeltaTime() * 25.0f);
		break;

	case 83:
	case VK_DOWN:
		XMStoreFloat3(&pCamera->m_Eye, XMLoadFloat3(&pCamera->m_Eye) + XMLoadFloat3(&pCamera->GetForwardVector()) * kTimer.DeltaTime() * -25.0f);
		break;

	case 65:
	case VK_LEFT:
		XMStoreFloat3(&pCamera->m_Eye, XMLoadFloat3(&pCamera->m_Eye) + XMLoadFloat3(&pCamera->GetRightVector()) * kTimer.DeltaTime() * 25.0f);
		break;

	case 68:
	case VK_RIGHT:
		XMStoreFloat3(&pCamera->m_Eye, XMLoadFloat3(&pCamera->m_Eye) + XMLoadFloat3(&pCamera->GetRightVector()) * kTimer.DeltaTime() * -25.0f);
		break;
	default:
		break;
	}
}
