#include "pch.h"
#include "NotificationHandler.h"
#include "BluetoothHandler.h"

namespace NotificationHandler
{
	constexpr GUID guid = { 0x53d7aa31, 0x8ac9, 0x4661, {0x92, 0x35, 0xd9, 0x29, 0x87, 0x76, 0xc7, 0x2e} };
	constexpr auto WM_APP_NOTIFYCALLBACK = WM_APP + 1U;
	const wchar_t ClassName[] = L"WindowClassName";

	bool autoReconnect = false;
	bool DEBUG_localConnection = false;

	HMODULE hInstance;
	HICON balloonIcon;
	HICON notificationIcon;

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
		NOTIFYICONDATA nid;
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

		std::wstring statusStr = std::wstring(L"Status: ") + (BluetoothHandler::IsConnected() ? L"Connected" : L"Disconnected");
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

	LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		std::cout << message << std::endl;

		switch (message)
		{
		case WM_CREATE:
			std::cout << "Adding notification icon..." << std::endl;
			AddNotificationIcon(hWnd);
			break;
		case WM_APP_NOTIFYCALLBACK:
		{
			switch (LOWORD(lParam))
			{
			case NIN_SELECT:
				ShowBalloon(hWnd, L"Remote connected!", L"Remote was connected");
				break;
			case WM_CONTEXTMENU:
				POINT p = { LOWORD(wParam), HIWORD(wParam) };
				ShowContextMenu(hWnd, p);
				break;
			}
			break;
		}
		case WM_COMMAND:
		{
			WORD id = LOWORD(wParam);
			switch (id)
			{
			case ContextMenuItem::ExitButton:
				DeleteNotificationIcon();
				PostQuitMessage(0);
				break;
			case ContextMenuItem::AutomaticConnectionCheckbox:
				autoReconnect = !autoReconnect;
				break;
			case ContextMenuItem::ConnectionActionButton:
				DEBUG_localConnection = !DEBUG_localConnection;
				break;
			}
			break;
		}
		case WM_DESTROY:
			DeleteNotificationIcon();
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}

		return 0;
	}

	void Initialize()
	{
		std::cout << "Initializing notification handler..." << std::endl;

		hInstance = GetModuleHandle(L"");

		WNDCLASS wc = { 0 };
		wc.lpfnWndProc = WindowProc;
		wc.hInstance = hInstance;
		wc.lpszClassName = ClassName;

		ATOM atom = RegisterClass(&wc);
		if (!atom)
		{
			DWORD error = GetLastError();
			std::cout << "RegisterClass failed: " << error << std::endl;
			return;
		}

		HWND hWnd = CreateWindowEx(0, ClassName, L"", WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

		if (hWnd == NULL)
		{
			DWORD error = GetLastError();
			std::cout << "CreateWindowEx failed: " << error << std::endl;
			return;
		}

		if (LoadIconMetric(NULL, L"remy1.ico", LIM_LARGE, &balloonIcon) != S_OK)
			std::cout << "Failed to load balloon icon" << std::endl;

		if (LoadIconMetric(NULL, MAKEINTRESOURCE(IDI_QUESTION), LIM_LARGE, &notificationIcon) != S_OK)
			std::cout << "Failed to load notification icon" << std::endl;

		MSG msg;
		while (GetMessage(&msg, hWnd, 0, 0) > 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}