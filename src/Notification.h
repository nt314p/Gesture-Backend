#pragma once

namespace Notification
{
	namespace ContextMenuItem
	{
		constexpr UINT StatusTitle = 0;
		// separator at index 1
		constexpr UINT ConnectionActionButton = 2; // connect/disconnect
		constexpr UINT EditInputSettingsButton = 3;
		constexpr UINT AutomaticConnectionCheckbox = 4;
		// separator at index 5
		constexpr UINT ExitButton = 6;
	}

	void AddNotificationIcon(HWND hWnd);
	void DeleteNotificationIcon();
	void ShowBalloon(HWND hWnd, LPCWSTR title, LPCWSTR message);
	void ShowContextMenu(HWND hWnd, POINT pt);
	void SetNotificationIconTooltip(const wchar_t* tooltip);
}