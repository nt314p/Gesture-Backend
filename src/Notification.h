#pragma once

namespace Notification
{
	namespace ContextMenuItem
	{
		static constexpr UINT StatusTitle = 0;
		// separator at index 1
		static constexpr UINT ConnectionActionButton = 2; // connect/disconnect
		static constexpr UINT EditInputSettingsButton = 3;
		static constexpr UINT AutomaticConnectionCheckbox = 4;
		// separator at index 5
		static constexpr UINT ExitButton = 6;
	}

	static constexpr GUID guid = { 0x53d7aa31, 0x8ac9, 0x4661, {0x92, 0x35, 0xd9, 0x29, 0x87, 0x76, 0xc7, 0x2e} };

	static constexpr auto BalloonIconName = L"remy1.ico";
	static constexpr auto NotificationIconName = L"remy3.ico";
	
	void Initialize();
	void AddNotificationIcon(HWND hWnd);
	void DeleteNotificationIcon();
	void ShowBalloon(HWND hWnd, LPCWSTR title, LPCWSTR message);
	void ShowContextMenu(HWND hWnd, POINT pt, bool bleConnected, bool autoReconnect);
	void SetNotificationIconTooltip(const wchar_t* tooltip);
}