#include "Apps/App.h"
#include "Helpers/DebugHelper.h"

Tag tag = L"Main";

App* pApp = nullptr;

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)

{
	pApp = new App(hInstance);

	if (App::GetApp()->Init() == false)
	{
		LOG_ERROR(tag, L"Failed to Init App!");

		return 0;
	}

	return App::GetApp()->Run();
}