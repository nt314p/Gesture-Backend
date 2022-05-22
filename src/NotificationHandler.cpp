#include "pch.h"
#include "NotificationHandler.h"

namespace NotificationHandler
{
	constexpr auto uID = 100U;
	constexpr GUID guid = { 0x53d7aa32, 0x8ac9, 0x4661, {0x92, 0x35, 0xd9, 0x29, 0x87, 0x76, 0xc7, 0x2e} };
	constexpr auto WM_APP_NOTIFYCALLBACK = WM_APP + 1U;
	constexpr auto tipMessage = "Hello, world!";
	const wchar_t ClassName[] = L"Sample Window Class";

	bool autoReconnect = false;

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

		HRESULT hr = LoadIconMetric(NULL, MAKEINTRESOURCE(IDI_QUESTION), LIM_LARGE, &nid.hIcon);

		if (hr != S_OK)
			std::cout << "Error loading icon!" << std::endl;

		bool result = Shell_NotifyIcon(NIM_ADD, &nid);
		std::cout << result << std::endl;

		nid.uVersion = NOTIFYICON_VERSION_4;
		Shell_NotifyIcon(NIM_SETVERSION, &nid);
	}

	void DeleteNotificationIcon()
	{
		NOTIFYICONDATA nid = { 0 };
		nid.cbSize = sizeof(nid);
		nid.guidItem = guid;
		if (!Shell_NotifyIcon(NIM_DELETE, &nid))
			std::cout << "Failed to delete icon" << std::endl;
	}

	void ShowBalloon(HWND hWnd, LPCWSTR title, LPCWSTR message)
	{
		NOTIFYICONDATA nid;
		nid.cbSize = sizeof(nid);
		nid.hWnd = hWnd;
		nid.guidItem = guid;
		nid.uFlags = NIF_INFO | NIF_GUID | NIF_ICON | NIF_REALTIME;
		nid.dwInfoFlags = NIIF_USER | NIIF_LARGE_ICON;
		nid.hIcon = notificationIcon;
		nid.hBalloonIcon = balloonIcon;
		wcscpy_s(nid.szInfo, message);
		wcscpy_s(nid.szInfoTitle, title);

		if (!Shell_NotifyIcon(NIM_MODIFY, &nid))
		{
			DWORD err = GetLastError();
			std::cout << "Failed to show balloon: " << err << std::endl;
		}
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

		InsertMenuItemString(hMenu, ContextMenuItem::StatusTitle, MFS_DEFAULT, L"Status: Connected");
		AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
		InsertMenuItemString(hMenu, ContextMenuItem::ConnectionActionButton, 0, L"Disconnect");
		InsertMenuItemString(hMenu, ContextMenuItem::EditInputSettingsButton, 0, L"Edit input settings");
		InsertMenuItemString(hMenu, ContextMenuItem::AutomaticConnectionCheckbox, 
			autoReconnect ? MFS_CHECKED : 0, L"Auto reconnect");
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
			AddNotificationIcon(hWnd);
			break;
		case WM_APP_NOTIFYCALLBACK:
		{
			std::cout << LOWORD(lParam) << std::endl;
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
			}
			std::cout << "Got command" << std::endl;
			std::cout << LOWORD(wParam) << std::endl;
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
			std::cout << "RegisterClass failed: " << GetLastError() << std::endl;
			return;
		}

		HWND hWnd = CreateWindowEx(0, ClassName, L"Test window", WS_OVERLAPPEDWINDOW,
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
			std::cout << "Wheeeee0!" << std::endl;
			TranslateMessage(&msg);
			std::cout << "Wheeeee1!" << std::endl;
			DispatchMessage(&msg);
			std::cout << "Wheeeee2!" << std::endl;
		}
	}
}