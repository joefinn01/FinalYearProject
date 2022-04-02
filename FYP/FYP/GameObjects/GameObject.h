#pragma once

#include "Helpers/MathHelper.h"
#include "Commons/UploadBuffer.h"
#include "Shaders/ConstantBuffers.h"

#include <string>
#include <unordered_map>

class Timer;

struct Vertex;
struct Mesh;
struct GameObjectCB;
struct RenderInfo;

enum class PrimitiveAttributes : UINT8;

enum class GameObjectType
{
	BASE = 0,
	VISIBLE,
	NUM_GAMEOBJECT_TYPES
};

class GameObject
{
public:
	GameObject();
	~GameObject();

	virtual bool Init(std::string sName, DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 rotationQuat, DirectX::XMFLOAT3 scale, Mesh* pMesh, bool bRender = true, bool bRaytrace = true);
	virtual void Update(const Timer& kTimer);
	virtual void Destroy();

	std::string GetName() const;

	GameObjectType GetType() const;

	void SetRotation(const DirectX::XMFLOAT4X4& rotationMatrix);
	void SetRotation(const DirectX::XMFLOAT4& rotationQuat);
	void SetRotation(float fRoll, float fPitch, float fYaw);

	void Rotate(const DirectX::XMFLOAT4X4& rotationMatrix);
	void Rotate(const DirectX::XMFLOAT4& rotationQuat);
	void Rotate(float fX, float fY, float fZ);

	DirectX::XMFLOAT3 GetEulerAngles() const;
	DirectX::XMFLOAT4 GetOrientation() const;

	void SetPosition(DirectX::XMFLOAT3 position);
	void SetPosition(float fX, float fY, float fZ);
	void Translate(DirectX::XMFLOAT3 translation);
	void Translate(float fX, float fY, float fZ);

	DirectX::XMFLOAT3 GetPosition() const;

	void SetScale(DirectX::XMFLOAT3 scale);
	void SetScale(float fX, float fY, float fZ);
	void AdjustScale(DirectX::XMFLOAT3 scaleDifference);
	void AdjustScale(float fX, float fY, float fZ);

	DirectX::XMFLOAT3 GetScale() const;

	DirectX::XMFLOAT4 GetUpVector() const;
	DirectX::XMFLOAT4 GetForwardVector() const;
	DirectX::XMFLOAT4 GetRightVector() const;

	DirectX::XMFLOAT4X4 GetWorldMatrix() const;
	DirectX::XMFLOAT4X4 GetWorldMatrixNoScale() const;

	Mesh* GetMesh() const;
	void SetMesh(Mesh* pMesh);

	DirectX::XMFLOAT3X4 Get3X4WorldMatrix();

	UINT GetIndex();

	bool IsRendering();
	bool IsRaytraced();

	void SetIsRendering(bool bRender);
	void SetIsRaytracing(bool bRaytrace);

protected:
	GameObjectType m_eType = GameObjectType::BASE;

private:
	void UpdateAxisVectors();

	std::string m_sName = "";

	DirectX::XMFLOAT3 m_Position = DirectX::XMFLOAT3();
	DirectX::XMFLOAT4 m_Rotation = DirectX::XMFLOAT4(0, 0, 0, 1);
	DirectX::XMFLOAT3 m_Scale = DirectX::XMFLOAT3(1, 1, 1);

	DirectX::XMFLOAT4 m_Up = DirectX::XMFLOAT4(0, 1, 0, 0);
	DirectX::XMFLOAT4 m_Right = DirectX::XMFLOAT4(1, 0, 0, 0);

	bool m_bRender = true;
	bool m_bRaytrace = true;

	Mesh* m_pMesh = nullptr;

	UINT m_uiIndex = 0;
};

