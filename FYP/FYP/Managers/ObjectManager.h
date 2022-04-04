#pragma once
#include "Commons\Singleton.h"
#include "Include/json/json.hpp"

#include <string>
#include <unordered_map>
#include <Windows.h>

class Timer;
class GameObject;
class Camera;

struct ID3D12GraphicsCommandList4;

class ObjectManager : public Singleton<ObjectManager>
{
public:
	//GameObject methods
	bool AddGameObject(GameObject* pGameObject, UINT& uiIndex);

	bool RemoveGameObject(const std::string& ksName);
	bool RemoveGameObject(GameObject* pGameObject);

	GameObject* GetGameObject(const std::string& ksName);

	std::unordered_map<std::string, GameObject*>* GetGameObjects();

	int GetNumGameObjects();

	//Camera methods
	bool AddCamera(Camera* pCamera);

	bool RemoveCamera(const std::string& ksName);
	bool RemoveCamera(Camera* pCamera);

	Camera* GetActiveCamera() const;
	void SetActiveCamera(const std::string& ksName);

	Camera* GetCamera(const std::string& ksName) const;

	void Update(const Timer& kTimer);

	void Save(nlohmann::json& data);

protected:

private:
	std::unordered_map<std::string, GameObject*> m_GameObjects;
	std::unordered_map<std::string, Camera*> m_Cameras;

	UINT m_uiCurrentIndex = 0;

	Camera* m_pActiveCamera = nullptr;
};

