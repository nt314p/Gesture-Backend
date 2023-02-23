#include "pch.h"
#include "Main.h"
#include "Notification.h"
#include "Bluetooth.h"

namespace Notification
{
	static bool autoReconnect = false;
	bool DEBUG_localConnection = false;

	static HICON balloonIcon;
	static HICON notificationIcon;

	void AddNotificationIcon(HWND hWnd)
	{
		NOTIFYICONDATA nid = { 0 };
		nid.cbSize = sizeof(nid);
		nid.hWnd = hWnd;
		nid.guidItem = guid;
		nid.uCallbackMessage = WM_APP_NOTIFYCALLBACK;
		nid.uFlags = NIF_ICON | NIF_TIP | NIF_SHOWTIP | NIF_MESSAGE | NIF_GUID;

		nid.hIcon = notificationIcon;
		nid.hBalloonIcon = balloonIcon;

		wcscpy_s(nid.szTip, L"Hello, world!");

		if (LoadIconMetric(NULL, MAKEINTRESOURCE(IDI_QUESTION), LIM_LARGE, &nid.hIcon) != S_OK)
			std::cout << "Error loading icon!" << std::endl;

		if(!Shell_NotifyIcon(NIM_ADD, &nid))
			std::cout << "Failed to add icon!" << std::endl;

		nid.uVersion = NOTIFYICON_VERSION_4;
		if (!Shell_NotifyIcon(NIM_SETVERSION, &nid))
			std::cout << "Failed to set icon version!" << std::endl;;
	}

	void DeleteNotificationIcon()
	{
		NOTIFYICONDATA nid = { 0 };
		nid.cbSize = sizeof(nid);
		nid.guidItem = guid;
		nid.uFlags = NIF_GUID;
		
		if (!Shell_NotifyIcon(NIM_DELETE, &nid))
			std::cout << "Failed to delete icon!" << std::endl;
	}

	void SetNotificationIconTooltip(const wchar_t* tooltip)
	{
		std::cout << "Setting tooltip: " << tooltip << std::endl;
		NOTIFYICONDATA nid = { 0 };
		nid.cbSize = sizeof(nid);
		nid.guidItem = guid;
		nid.uFlags = NIF_GUID | NIF_TIP | NIF_SHOWTIP;

		wcscpy_s(nid.szTip, tooltip);

		if (!Shell_NotifyIcon(NIM_MODIFY, &nid))
			std::cout << "Failed to set tooltip!" << std::endl;
	}

	void ShowBalloon(HWND hWnd, LPCWSTR title, LPCWSTR message)
	{
		NOTIFYICONDATA nid{};
		nid.cbSize = sizeof(nid);
		nid.hWnd = hWnd;
		nid.guidItem = guid;
		nid.uFlags = NIF_INFO | NIF_GUID | NIF_ICON | NIF_REALTIME | NIF_SHOWTIP;
		nid.dwInfoFlags = NIIF_USER | NIIF_LARGE_ICON;
		nid.hIcon = notificationIcon;
		nid.hBalloonIcon = balloonIcon;
		wcscpy_s(nid.szInfo, message);
		wcscpy_s(nid.szInfoTitle, title);
		wcscpy_s(nid.szTip, L"hi");

		if (!Shell_NotifyIcon(NIM_MODIFY, &nid))
			std::cout << "Failed to show balloon!" << std::endl;
	}

	void InitializeMenuItemInfo(UINT id, UINT mask, UINT flags, const wchar_t* text, MENUITEMINFO* pMii)
	{
		memset(pMii, 0, sizeof(MENUITEMINFO));
		pMii->cbSize = sizeof(MENUITEMINFO);
		pMii->fMask = MIIM_ID | mask;
		pMii->fState = flags;
		pMii->wID = id;
		pMii->dwTypeData = (LPWSTR)text;
	}

	void InsertMenuItemString(HMENU hMenu, UINT id, UINT flags, const wchar_t* text)
	{
		MENUITEMINFO mii;
		InitializeMenuItemInfo(id, MIIM_STATE | MIIM_STRING, flags, text, &mii);
		InsertMenuItem(hMenu, id, true, &mii);
	}

	void ModifyMenuItemState(HMENU hMenu, UINT id, UINT flags)
	{
		MENUITEMINFO mii;
		InitializeMenuItemInfo(id, MIIM_STATE, flags, NULL, &mii);
		SetMenuItemInfo(hMenu, id, true, &mii);
	}

	void ModifyMenuItemText(HMENU hMenu, UINT id, const wchar_t* text)
	{
		MENUITEMINFO mii;
		InitializeMenuItemInfo(id, MIIM_STRING, 0, text, &mii);
		SetMenuItemInfo(hMenu, id, true, &mii);
	}

	void ShowContextMenu(HWND hWnd, POINT pt)
	{
		HMENU hMenu = CreatePopupMenu();
		if (hMenu == NULL) return;

		std::wstring statusStr = std::wstring(L"Status: ") + (BluetoothLE::IsConnected() ? L"Connected" : L"Disconnected");
		InsertMenuItemString(hMenu, ContextMenuItem::StatusTitle, MFS_DEFAULT, statusStr.c_str());
		AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
		const wchar_t* connectionActionStr = DEBUG_localConnection ? L"Connect" : L"Disconnect";
		InsertMenuItemString(hMenu, ContextMenuItem::ConnectionActionButton, 0, connectionActionStr);
		InsertMenuItemString(hMenu, ContextMenuItem::EditInputSettingsButton, 0, L"Edit input settings");
		InsertMenuItemString(hMenu, ContextMenuItem::AutomaticConnectionCheckbox,
			autoReconnect ? (UINT)MFS_CHECKED : 0, L"Auto reconnect");
		AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
		InsertMenuItemString(hMenu, ContextMenuItem::ExitButton, 0, L"Exit");

		SetForegroundWindow(hWnd);

		UINT uFlags = TPM_RIGHTBUTTON;
		if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0)
		{
			uFlags |= TPM_RIGHTALIGN;
		}
		else
		{
			uFlags |= TPM_LEFTALIGN;
		}

		TrackPopupMenuEx(hMenu, uFlags, pt.x, pt.y, hWnd, NULL);
	}
}