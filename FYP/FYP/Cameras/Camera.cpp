#include "Camera.h"
#include "Managers/WindowManager.h"
#include "Commons/Timer.h"

using namespace DirectX;

Camera::Camera(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 at, DirectX::XMFLOAT3 up, float fNearDepth, float fFarDepth, std::string sName)
{
	m_Eye = position;
	m_At = at;
	m_Up = up;

	m_sName = sName;

	XMStoreFloat3(&m_Forward, XMVector3Normalize(XMLoadFloat3(&m_At) - XMLoadFloat3(&m_Eye)));

	XMStoreFloat4x4(&m_View, XMMatrixLookToLH(XMLoadFloat3(&m_Eye), XMLoadFloat3(&m_Forward), XMLoadFloat3(&m_Up)));

	Reshape(fNearDepth, fFarDepth);
}

Camera::Camera()
{
	XMStoreFloat4x4(&m_View, XMMatrixLookToLH(XMLoadFloat3(&m_Eye), XMLoadFloat3(&m_Forward), XMLoadFloat3(&m_Up)));
}

Camera::~Camera()
{
}

void Camera::Update(const Timer& kTimer)
{
	XMStoreFloat4x4(&m_View, XMMatrixLookToLH(XMLoadFloat3(&m_Eye), XMLoadFloat3(&m_Forward), XMLoadFloat3(&m_Up)));
}

void Camera::SetPosition(DirectX::XMFLOAT3 position)
{
	m_Eye = position;
}

DirectX::XMFLOAT3 Camera::GetPosition() const
{
	return m_Eye;
}

void Camera::Translate(DirectX::XMFLOAT3 vec)
{
	XMStoreFloat3(&m_Eye, XMLoadFloat3(&m_Eye) + XMLoadFloat3(&vec));
}

void Camera::SetLookAt(DirectX::XMFLOAT3 at)
{
	m_At = at;
}

DirectX::XMFLOAT3 Camera::GetLookAt() const
{
	return m_At;
}

void Camera::SetUpVector(DirectX::XMFLOAT3 up)
{
	m_Up = up;
}

DirectX::XMFLOAT3 Camera::GetUpVector() const
{
	return m_Up;
}

DirectX::XMFLOAT3 Camera::GetRightVector() const
{
	XMFLOAT3 right;

	XMStoreFloat3(&right, XMVector3Cross(XMLoadFloat3(&m_Forward), XMLoadFloat3(&m_Up)));

	return right;
}

DirectX::XMFLOAT3 Camera::GetForwardVector() const
{
	return m_Forward;
}

DirectX::XMFLOAT4X4 Camera::GetViewMatrix() const
{
	return m_View;
}

DirectX::XMFLOAT4X4 Camera::GetProjectionMatrix() const
{
	return m_Projection;
}

DirectX::XMFLOAT4X4 Camera::GetViewProjectionMatrix() const
{
	XMFLOAT4X4 viewProj;

	XMStoreFloat4x4(&viewProj, XMMatrixMultiply(XMLoadFloat4x4(&m_View), XMLoadFloat4x4(&m_Projection)));

	return viewProj;
}

std::string Camera::GetName() const
{
	return m_sName;
}

void Camera::Reshape(float fNearDepth, float fFarDepth)
{
	m_fNearDepth = fNearDepth;
	m_fFarDepth = fFarDepth;

	XMStoreFloat4x4(&m_Projection, XMMatrixPerspectiveFovLH(XM_PIDIV2, WindowManager::GetInstance()->GetAspectRatio(), m_fNearDepth, m_fFarDepth));
}
