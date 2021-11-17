#pragma once

template<class T>
class Singleton
{
public:
	static T* GetInstance()
	{
		if (s_pInstance == nullptr)
		{
			s_pInstance = new T();
		}

		return s_pInstance;
	}

protected:

private:
	static T* s_pInstance;
};

template <class T>
T* Singleton<T>::s_pInstance = nullptr;