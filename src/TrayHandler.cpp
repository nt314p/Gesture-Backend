#include "pch.h"
#include "TrayHandler.h"

namespace TrayHandler
{
	constexpr GUID guid = { 0x23977b55, 0x10e0, 0x4041, {0xb8, 0x62, 0xb1, 0x95, 0x41, 0x96, 0x36, 0x69} };
	constexpr auto tipMessage = "Hello, world!";
	const wchar_t ClassName[] = L"Sample Window Class";

	LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		std::cout << "I got a callback" << std::endl;
		return 1;
		//return DefWindowProc(hWnd, message, wParam, lParam);
	}

	void Initialize()
	{
		std::cout << "Initializing tray handler..." << std::endl;

		HMODULE hInstance = GetModuleHandle(L"");

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

		NOTIFYICONDATA nid;
		nid.cbSize = sizeof(nid);
		nid.hWnd = hWnd;
		nid.guidItem = guid;
		nid.uID = 100;
		nid.uCallbackMessage = WM_APP + 1;
		nid.uFlags = NIF_ICON | NIF_TIP | NIF_SHOWTIP | NIF_MESSAGE;

		wcscpy_s(nid.szTip, L"Hello, world!");
		wcscpy_s(nid.szInfo, L"You've got mail!");


		HRESULT hr = LoadIconMetric(NULL, MAKEINTRESOURCE(IDI_EXCLAMATION), LIM_LARGE, &nid.hIcon);

		if (hr != S_OK)
			std::cout << "Error loading icon!" << std::endl;

		bool result = Shell_NotifyIcon(NIM_ADD, &nid);
		std::cout << result << std::endl;

		nid.uVersion = NOTIFYICON_VERSION_4;
		Shell_NotifyIcon(NIM_SETVERSION, &nid);

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