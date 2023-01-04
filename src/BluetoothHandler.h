#pragma once

namespace BluetoothHandler
{
	constexpr auto BluetoothScanningTimeoutMins = 5; // TODO: implement timeout

	void InitializeWatcher();
	void AttemptConnection();
	bool IsConnected();
	void Disconnect();
}