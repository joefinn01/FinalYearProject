#pragma once

#include "Helpers/MathHelper.h"

#include <string>

class Timer;

class Camera
{
public:
	Camera(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 at, DirectX::XMFLOAT3 up, float fNearDepth, float fFarDepth, std::string sName);
	Camera();
	~Camera();

	virtual void Update(const Timer& kTimer);

	void SetPosition(DirectX::XMFLOAT3 position);
	DirectX::XMFLOAT3 GetPosition() const;

	void Translate(DirectX::XMFLOAT3 vec);

	void SetLookAt(DirectX::XMFLOAT3 at);
	DirectX::XMFLOAT3 GetLookAt() const;

	void SetUpVector(DirectX::XMFLOAT3 up);
	DirectX::XMFLOAT3 GetUpVector() const;

	DirectX::XMFLOAT3 GetRightVector() const;
	DirectX::XMFLOAT3 GetForwardVector() const;

	DirectX::XMFLOAT4X4 GetViewMatrix() const;
	DirectX::XMFLOAT4X4 GetProjectionMatrix() const;
	DirectX::XMFLOAT4X4 GetViewProjectionMatrix() const;

	std::string GetName() const;

	void Reshape(float fNearDepth, float fFarDepth);

protected:
	DirectX::XMFLOAT3 m_Eye = DirectX::XMFLOAT3();
	DirectX::XMFLOAT3 m_At = DirectX::XMFLOAT3();

	DirectX::XMFLOAT3 m_Up = DirectX::XMFLOAT3(0, 1, 0);
	DirectX::XMFLOAT3 m_Forward = DirectX::XMFLOAT3(0, 0, 1);

	DirectX::XMFLOAT4X4 m_RotationMatrix = MathHelper::Identity();

	DirectX::XMFLOAT4X4 m_View = MathHelper::Identity();
	DirectX::XMFLOAT4X4 m_Projection = MathHelper::Identity();

	float m_fNearDepth;
	float m_fFarDepth;

	std::string m_sName = "";

private:

};

