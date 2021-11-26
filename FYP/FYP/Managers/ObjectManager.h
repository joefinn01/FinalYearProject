#pragma once
#include "Commons\Singleton.h"

#include <string>
#include <unordered_map>

class Timer;
class GameObject;
class Camera;

struct ID3D12GraphicsCommandList4;

class ObjectManager : public Singleton<ObjectManager>
{
public:
	//GameObject methods
	bool AddGameObject(GameObject* pGameObject);

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

protected:

private:
	std::unordered_map<std::string, GameObject*> m_GameObjects;
	std::unordered_map<std::string, Camera*> m_Cameras;

	Camera* m_pActiveCamera = nullptr;
};

