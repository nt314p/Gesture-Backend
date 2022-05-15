#include "pch.h"
#include "TrayHandler.h"

namespace TrayHandler
{
	constexpr GUID guid = { 0x23977b55, 0x10e0, 0x4041, {0xb8, 0x62, 0xb1, 0x95, 0x41, 0x96, 0x36, 0x69} };
	constexpr auto tipMessage = "Hello, world!";


	void Initialize()
	{
		std::cout << "Initializing tray handler..." << std::endl;

		NOTIFYICONDATA nid;
		nid.cbSize = sizeof(nid);
		nid.hWnd = GetConsoleWindow();

		std::cout << (nid.hWnd == NULL) << std::endl;

		nid.guidItem = guid;
		nid.uID = 100;
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
	}
}