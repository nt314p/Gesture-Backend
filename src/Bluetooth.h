#pragma once
#include "pch.h"
#include "Main.h"

namespace BluetoothLE
{
	using namespace Windows::Devices;
	using namespace Platform;

	static constexpr auto BluetoothScanningTimeoutMinutes = 5; // TODO: implement timeout

	static const auto ServiceUUID = Bluetooth::BluetoothUuidHelper::FromShortId(0xffe0);
	static const auto CharacteristicUUID = Bluetooth::BluetoothUuidHelper::FromShortId(0xffe1);
	static const auto BLEPin = ref new String(L"802048");

	extern std::function<void()> Connected;
	extern std::function<void()> Disconnected;
	extern std::function<void()> ReceivedData;

	void InitializeWatcher();
	void AttemptConnection();
	bool IsConnected();
	void Disconnect();
}