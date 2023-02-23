#pragma once

namespace BluetoothLE
{
	constexpr auto BluetoothScanningTimeoutMins = 5; // TODO: implement timeout

	void InitializeWatcher();
	void AttemptConnection();
	bool IsConnected();
	void Disconnect();
}