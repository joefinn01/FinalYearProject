#include "GameObject.h"
#include "Managers/ObjectManager.h"
#include "Shaders/Vertices.h"
#include "Apps/App.h"

using namespace DirectX;

GameObject::GameObject()
{
}

GameObject::~GameObject()
{
}

bool GameObject::Init(std::string sName, DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 rotation, DirectX::XMFLOAT3 scale, Mesh* pMesh)
{
	m_sName = sName;

	m_Position = position;

	Rotate(rotation.x, rotation.y, rotation.z);

	m_Scale = scale;

	m_eType = GameObjectType::BASE;

	m_pMesh = pMesh;

	ObjectManager::GetInstance()->AddGameObject(this);

	return true;
}

void GameObject::Update(const Timer& kTimer)
{
	Rotate(20.0f * kTimer.DeltaTime(), 0, 0);
}

void GameObject::Destroy()
{
	ObjectManager::GetInstance()->RemoveGameObject(m_sName);
}

std::string GameObject::GetName() const
{
	return m_sName;
}

GameObjectType GameObject::GetType() const
{
	return m_eType;
}

void GameObject::SetRotation(const DirectX::XMFLOAT4X4& rotationMatrix)
{
	XMStoreFloat4(&m_Rotation, XMQuaternionNormalize(XMQuaternionRotationMatrix(XMLoadFloat4x4(&rotationMatrix))));

	UpdateAxisVectors();
}

void GameObject::SetRotation(const DirectX::XMFLOAT4& rotationQuat)
{
	XMStoreFloat4(&m_Rotation, XMQuaternionNormalize(XMLoadFloat4(&rotationQuat)));

	UpdateAxisVectors();
}

void GameObject::SetRotation(float fRoll, float fPitch, float fYaw)
{
	XMStoreFloat4(&m_Rotation, XMQuaternionNormalize(XMQuaternionRotationRollPitchYaw((fPitch * XM_PI) / 180.0f, (fYaw * XM_PI) / 180.0f, (fRoll * XM_PI) / 180.0f)));

	UpdateAxisVectors();
}

void GameObject::Rotate(const DirectX::XMFLOAT4X4& rotationMatrix)
{
	XMStoreFloat4(&m_Rotation, XMQuaternionNormalize(XMQuaternionMultiply(XMLoadFloat4(&m_Rotation), XMQuaternionRotationMatrix(XMLoadFloat4x4(&rotationMatrix)))));

	UpdateAxisVectors();
}

void GameObject::Rotate(const DirectX::XMFLOAT4& rotationQuat)
{
	XMStoreFloat4(&m_Rotation, XMQuaternionNormalize(XMQuaternionMultiply(XMLoadFloat4(&m_Rotation), XMLoadFloat4(&rotationQuat))));

	UpdateAxisVectors();
}

void GameObject::Rotate(float fX, float fY, float fZ)
{
	XMStoreFloat4(&m_Rotation, XMQuaternionNormalize(XMQuaternionMultiply(XMLoadFloat4(&m_Rotation), XMQuaternionRotationRollPitchYaw((fX * XM_PI) / 180.0f, (fY * XM_PI) / 180.0f, (fZ * XM_PI) / 180.0f))));

	UpdateAxisVectors();
}

DirectX::XMFLOAT3 GameObject::GetEulerAngles() const
{
	XMFLOAT4X4 rotationMatrix;
	XMStoreFloat4x4(&rotationMatrix, XMMatrixRotationQuaternion(XMLoadFloat4(&m_Rotation)));

	float Roll;
	float Pitch;
	float Yaw;

	if (rotationMatrix._11 == 1.0f)
	{
		Yaw = atan2f(rotationMatrix._13, rotationMatrix._34);
		Pitch = 0.0f;
		Roll = 0.0f;

	}
	else if (rotationMatrix._11 == -1.0f)
	{
		Yaw = atan2f(rotationMatrix._13, rotationMatrix._34);
		Pitch = 0.0f;
		Roll = 0.0f;
	}
	else
	{

		Yaw = atan2f(-rotationMatrix._31, rotationMatrix._11);
		Pitch = asinf(rotationMatrix._21);
		Roll = atan2f(-rotationMatrix._23, rotationMatrix._22);
	}
	return XMFLOAT3(Pitch, Yaw, Roll);
}

DirectX::XMFLOAT4 GameObject::GetOrientation() const
{
	return m_Rotation;
}

void GameObject::SetPosition(DirectX::XMFLOAT3 position)
{
	m_Position = position;
}

void GameObject::SetPosition(float fX, float fY, float fZ)
{
	m_Position = XMFLOAT3(fX, fY, fZ);
}

void GameObject::Translate(DirectX::XMFLOAT3 translation)
{
	XMStoreFloat3(&m_Position, XMLoadFloat3(&m_Position) + XMLoadFloat3(&translation));
}

void GameObject::Translate(float fX, float fY, float fZ)
{
	XMStoreFloat3(&m_Position, XMLoadFloat3(&m_Position) + XMVectorSet(fX, fY, fZ, 0));
}

DirectX::XMFLOAT3 GameObject::GetPosition() const
{
	return m_Position;
}

void GameObject::SetScale(DirectX::XMFLOAT3 scale)
{
	m_Scale = scale;
}

void GameObject::SetScale(float fX, float fY, float fZ)
{
	m_Scale = XMFLOAT3(fX, fY, fZ);
}

void GameObject::AdjustScale(DirectX::XMFLOAT3 scaleDifference)
{
	XMStoreFloat3(&m_Scale, XMLoadFloat3(&m_Scale) + XMLoadFloat3(&scaleDifference));
}

void GameObject::AdjustScale(float fX, float fY, float fZ)
{
	XMStoreFloat3(&m_Scale, XMLoadFloat3(&m_Scale) + XMVectorSet(fX, fY, fZ, 0));
}

DirectX::XMFLOAT3 GameObject::GetScale() const
{
	return m_Scale;
}

DirectX::XMFLOAT4 GameObject::GetUpVector() const
{
	return m_Up;
}

DirectX::XMFLOAT4 GameObject::GetForwardVector() const
{
	XMFLOAT4 forward;
	XMStoreFloat4(&forward, XMVector3Cross(XMLoadFloat4(&m_Up), XMLoadFloat4(&m_Right)));

	return forward;
}

DirectX::XMFLOAT4 GameObject::GetRightVector() const
{
	return m_Right;
}

DirectX::XMFLOAT4X4 GameObject::GetWorldMatrix() const
{
	XMFLOAT4X4 world;

	XMStoreFloat4x4(&world, XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z) * XMMatrixRotationQuaternion(XMLoadFloat4(&m_Rotation)) * XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z));

	return world;
}

Mesh* GameObject::GetMesh() const
{
	return m_pMesh;
}

void GameObject::SetMesh(Mesh* pMesh)
{
	m_pMesh = pMesh;
}

XMFLOAT3X4 GameObject::Get3X4WorldMatrix()
{
	XMFLOAT4X4 world = GetWorldMatrix();
	XMFLOAT3X4 matrix;

	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			matrix.m[i][j] = world.m[j][i];
		}
	}

	return matrix;
}

void GameObject::UpdateAxisVectors()
{
	XMStoreFloat4(&m_Up, XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), XMMatrixRotationQuaternion(XMLoadFloat4(&m_Rotation))));
	XMStoreFloat4(&m_Right, XMVector3TransformNormal(XMVectorSet(1, 0, 0, 0), XMMatrixRotationQuaternion(XMLoadFloat4(&m_Rotation))));
}
