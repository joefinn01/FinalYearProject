#include "WindowManager.h"

void WindowManager::SetHWND(HWND hwnd)
{
	m_HWND = hwnd;
}

HWND WindowManager::GetHWND() const
{
	return m_HWND;
}

void WindowManager::SetRefreshRate(UINT uiNumerator, UINT uiDenominator)
{
	m_WindowSettings.RefreshRate.Numerator = uiNumerator;
	m_WindowSettings.RefreshRate.Denominator = uiDenominator;
}

void WindowManager::GetRefreshRate(UINT& uiNumerator, UINT& uiDenominator) const
{
	uiNumerator = m_WindowSettings.RefreshRate.Numerator;
	uiDenominator = m_WindowSettings.RefreshRate.Denominator;
}

void WindowManager::SetWindowDimensions(UINT uiWidth, UINT uiHeight)
{
	m_WindowSettings.Width = uiWidth;
	m_WindowSettings.Height = uiHeight;
}

void WindowManager::UpdateWindowDimensions()
{
	RECT rect;
	if (GetClientRect(m_HWND, &rect))
	{
		UINT dpi = GetDpiForWindow(GetDesktopWindow());
		float fDPIScale = dpi / 96.0f;

		m_WindowSettings.Width = (UINT)((rect.right - rect.left) * fDPIScale);
		m_WindowSettings.Height = (UINT)((rect.bottom - rect.top) * fDPIScale);
	}
}

void WindowManager::SetWindowWidth(UINT uiWidth)
{
	m_WindowSettings.Width = uiWidth;
}

UINT WindowManager::GetWindowWidth() const
{
	return m_WindowSettings.Width;
}

void WindowManager::SetWindowHeight(UINT uiHeight)
{
	m_WindowSettings.Height = uiHeight;
}

UINT WindowManager::GetWindowHeight() const
{
	return m_WindowSettings.Height;
}

void WindowManager::SetWindowSettings(WindowSettings settings)
{
	m_WindowSettings = settings;
}

float WindowManager::GetAspectRatio() const
{
	return m_WindowSettings.Width / (float)m_WindowSettings.Height;
}
