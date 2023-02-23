#include "pch.h"
#include <chrono>
#include "Notification.h"
#include "Input.h"
#include "Bluetooth.h"
#include "PacketParser.h"
#include "Main.h"

static const wchar_t ClassName[] = L"WindowClassName";

bool autoReconnect = false;
HMODULE hInstance;

void OnBluetoothConnected()
{
	std::cout << "Device connected!" << std::endl;
	PacketParser::ResetLastReceivedDataTime();
}

void OnBluetoothDisconnected()
{
	std::cout << "Device disconnected!" << std::endl;
	PacketParser::ResetDataAlignment();
	PacketParser::ResetLastReceivedDataTime();

	if (!autoReconnect) return;
	std::cout << "Attempting to reconnect..." << std::endl;
	BluetoothLE::AttemptConnection();
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	std::cout << message << std::endl;

	switch (message)
	{
	case WM_CREATE:
		std::cout << "Adding notification icon..." << std::endl;
		Notification::AddNotificationIcon(hWnd);
		break;
	case WM_APP_NOTIFYCALLBACK:
	{
		switch (LOWORD(lParam))
		{
		case NIN_SELECT:
			Notification::ShowBalloon(hWnd, L"Remote connected!", L"Remote was connected");
			break;
		case WM_CONTEXTMENU:
			POINT p = { LOWORD(wParam), HIWORD(wParam) };
			Notification::ShowContextMenu(hWnd, p);
			break;
		}
		break;
	}
	case WM_COMMAND:
	{
		WORD id = LOWORD(wParam);
		switch (id)
		{
		case Notification::ContextMenuItem::ExitButton:
			Notification::DeleteNotificationIcon();
			PostQuitMessage(0);
			break;
		case Notification::ContextMenuItem::AutomaticConnectionCheckbox:
			autoReconnect = !autoReconnect;
			break;
		case Notification::ContextMenuItem::ConnectionActionButton:
			Notification::DEBUG_localConnection = !Notification::DEBUG_localConnection;
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

// TODO: make this method non blocking
// perhaps return a method to pump?
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

int main(Platform::Array<Platform::String^>^ args)
{
	std::cout << "Starting GestureBackend..." << std::endl;

	HRESULT hresult = RoInitialize(RO_INIT_MULTITHREADED);

	if (hresult != S_OK)
	{
		if (hresult == S_FALSE)
			RoUninitialize();
		else
		{
			std::cout << "RoInitialize failed" << std::endl;
			return 0;
		}
	}

	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);

	//Notification::Initialize();

	Input::Initialize();
	BluetoothLE::InitializeWatcher();
	BluetoothLE::AttemptConnection();

	while (true)
	{

		Sleep(200);
		if (!BluetoothLE::IsConnected()) continue;

		if (PacketParser::DataTimeoutExceeded())
		{
			std::cout << "Disconnecting..." << std::endl;
			Notification::ShowBalloon(L"Remote disconnected!", L"Remote was disconnected");
			BluetoothLE::Disconnect();
			Sleep(500);
			BluetoothLE::AttemptConnection();
		}
	}
}