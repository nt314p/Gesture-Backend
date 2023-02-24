#pragma once

namespace BluetoothLE
{
	static constexpr auto BluetoothScanningTimeoutMins = 5; // TODO: implement timeout
	static constexpr auto BufferLength = 128;

	uint8_t ReadBuffer();
	int BufferCount();

	void InitializeWatcher();
	void AttemptConnection();
	bool IsConnected();
	void Disconnect();
}