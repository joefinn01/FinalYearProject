#include "Apps/App.h"
#include "Helpers/DebugHelper.h"

Tag tag = L"Main";

App* pApp = nullptr;

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)

{
	pApp = new App(hInstance);

	//Get file path to json file from command line
	std::string filepath = "";
	std::string runNumber = "";

	int iNumArgs = -1;
	LPWSTR* arguments = CommandLineToArgvW(GetCommandLine(), &iNumArgs);

	if (iNumArgs > 1)
	{
		std::wstring tempFilepath = arguments[1];

		filepath = std::string(tempFilepath.begin(), tempFilepath.end());
	}

	if (iNumArgs > 2)
	{
		std::wstring tempRunNumber = arguments[2];

		runNumber = std::string(tempRunNumber.begin(), tempRunNumber.end());
	}

	if (App::GetApp()->Init(filepath, runNumber) == false)
	{
		LOG_ERROR(tag, L"Failed to Init App!");

		return 0;
	}

	return App::GetApp()->Run();
}