#pragma once
#include "Commons/Singleton.h"

#include <Windows.h>
#include <dxgitype.h>

typedef DXGI_MODE_DESC WindowSettings;

class WindowManager : public Singleton<WindowManager>
{
public:

	//Getters and setters
	void SetHWND(HWND hwnd);
	HWND GetHWND() const;

	void SetRefreshRate(UINT uiNumerator, UINT uiDenominator);
	void GetRefreshRate(UINT& uiNumerator, UINT& uiDenominator) const;

	void SetWindowDimensions(UINT uiWidth, UINT uiHeight);
	void UpdateWindowDimensions();

	void SetWindowWidth(UINT uiWidth);
	UINT GetWindowWidth() const;

	void SetWindowHeight(UINT uiHeight);
	UINT GetWindowHeight() const;

	void SetWindowSettings(WindowSettings settings);

	float GetAspectRatio() const;

protected:

private:
	HWND m_HWND = nullptr;

	WindowSettings m_WindowSettings = WindowSettings();
};