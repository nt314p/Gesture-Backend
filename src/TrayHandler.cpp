#include "pch.h"
#include "TrayHandler.h"

namespace TrayHandler
{
	constexpr auto uID = 100U;
	constexpr GUID guid = { 0x53d7aa32, 0x8ac9, 0x4661, {0x92, 0x35, 0xd9, 0x29, 0x87, 0x76, 0xc7, 0x2e} };
	constexpr auto WM_APP_NOTIFYCALLBACK = WM_APP + 1U;
	constexpr auto tipMessage = "Hello, world!";
	const wchar_t ClassName[] = L"Sample Window Class";

	HMODULE hInstance;

	void AddNotificationIcon(HWND hWnd)
	{
		NOTIFYICONDATA nid = { 0 };
		nid.cbSize = sizeof(nid);
		nid.hWnd = hWnd;
		nid.guidItem = guid;
		nid.uCallbackMessage = WM_APP_NOTIFYCALLBACK;
		nid.uFlags = NIF_ICON | NIF_TIP | NIF_SHOWTIP | NIF_MESSAGE | NIF_GUID;

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

		HRESULT hr = LoadIconMetric(NULL, L"remy1.ico", LIM_LARGE, &nid.hBalloonIcon);

		nid.uFlags = NIF_INFO | NIF_GUID | NIF_ICON;
		nid.dwInfoFlags = NIIF_USER | NIIF_LARGE_ICON;
		wcscpy_s(nid.szInfo, message);
		wcscpy_s(nid.szInfoTitle, title);

		if (!Shell_NotifyIcon(NIM_MODIFY, &nid))
		{
			DWORD err = GetLastError();
			std::cout << "Failed to show balloon: " << err << std::endl;
		}
	}

	void ShowContextMenu(HWND hwnd, POINT pt)
	{
		HMENU hMenu = CreatePopupMenu();
		if (hMenu == NULL) return;

		InsertMenu(hMenu, ContextMenuItem::StatusTitle, MF_BYPOSITION | MF_STRING, NULL, L"Status: Connected");
		AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
		InsertMenu(hMenu, ContextMenuItem::ConnectionActionButton, MF_BYPOSITION | MF_STRING, NULL, L"Disconnect");
		InsertMenu(hMenu, ContextMenuItem::EditInputSettingsButton, MF_BYPOSITION | MF_STRING, NULL, L"Edit input settings");
		InsertMenu(hMenu, ContextMenuItem::AutomaticConnectionCheckbox, MF_BYPOSITION | MF_STRING | MF_CHECKED, NULL, L"Auto reconnect");
		AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
		InsertMenu(hMenu, ContextMenuItem::ExitButton, MF_BYPOSITION | MF_STRING, NULL, L"Exit");

		SetMenuDefaultItem(hMenu, ContextMenuItem::StatusTitle, true);

		SetForegroundWindow(hwnd);

		UINT uFlags = TPM_RIGHTBUTTON;
		if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0)
		{
			uFlags |= TPM_RIGHTALIGN;
		}
		else
		{
			uFlags |= TPM_LEFTALIGN;
		}

		TrackPopupMenuEx(hMenu, uFlags, pt.x, pt.y, hwnd, NULL);
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
		std::cout << "Initializing tray handler..." << std::endl;

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

		//ShowWindow(hWnd, 0);

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