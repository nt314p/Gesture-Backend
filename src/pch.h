#pragma once
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32.lib")

#include <iostream>
#include <string>
#include <Windows.h>
#include <winrt/Windows.Foundation.h>
#include <Windows.Devices.Bluetooth.h>
#include <winrt/Windows.Devices.Bluetooth.Advertisement.h>
#include <wrl/wrappers/corewrappers.h>
#include <wrl/event.h>
#include <collection.h>
#include <shellapi.h>
#include <commctrl.h>
#include <roapi.h>
#include <pplawait.h>