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

HWND globalHWnd;

void OnBluetoothConnected()
{
	std::cout << "Device connected!" << std::endl;
	Notification::ShowBalloon(globalHWnd, L"Remote connected!", L"Remote was connected");
	Notification::SetNotificationIconTooltip(L"Remote is connected");
	PacketParser::ResetLastReceivedDataTime();
}

void OnBluetoothDisconnected()
{
	std::cout << "Device disconnected!" << std::endl;
	Notification::ShowBalloon(globalHWnd, L"Remote disconnected!", L"Remote was disconnected");
	Notification::SetNotificationIconTooltip(L"Remote is disconnected");
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
			//Notification::ShowBalloon(hWnd, L"Remote connected!", L"Remote was connected");
			break;
		case WM_CONTEXTMENU:
			POINT p = { LOWORD(wParam), HIWORD(wParam) };
			Notification::ShowContextMenu(hWnd, p, BluetoothLE::IsConnected(), autoReconnect);
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
			if (BluetoothLE::IsConnected())
			{
				BluetoothLE::Disconnect();
			}
			else {
				BluetoothLE::AttemptConnection();
				Notification::SetNotificationIconTooltip(L"Attempting to connect to remote...");
			}
			break;
		}
		break;
	}
	case WM_TIMER:
		if (!BluetoothLE::IsConnected()) break;
		if (PacketParser::DataTimeoutExceeded())
		{
			std::cout << "Disconnecting..." << std::endl;
			Notification::ShowBalloon(hWnd, L"Remote disconnected!", L"Remote was disconnected");
			BluetoothLE::Disconnect(); // TODO: fix dependency and which function calls which
			// disconnect should call ondisconnected
		}
		break;
	case WM_DESTROY:
		Notification::DeleteNotificationIcon();
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

HWND CreateWindowHandle()
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
		return NULL;
	}

	HWND hWnd = CreateWindowEx(0, ClassName, L"", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

	if (hWnd == NULL)
	{
		DWORD error = GetLastError();
		std::cout << "CreateWindowEx failed: " << error << std::endl;
		return NULL;
	}

	return hWnd;
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

	Notification::Initialize();

	Input::Initialize();
	BluetoothLE::InitializeWatcher();

	HWND hWnd = CreateWindowHandle();
	if (hWnd == NULL) return 0;

	globalHWnd = hWnd;

	SetTimer(hWnd, 0, 1000, NULL);

	MSG msg;
	while (GetMessage(&msg, hWnd, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}