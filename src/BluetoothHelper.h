#pragma once

struct BLEDevice
{
	uint8_t* dataBuffer;
	Platform::Guid serviceGuid;
	Platform::Guid characteristicGuid;
};

bool StartWatcher();

concurrency::task<bool> Initialize(Windows::Devices::Bluetooth::BluetoothLEDevice^ bleDevice, 
	Platform::Guid serviceGuid, Platform::Guid characteristicGuid);

concurrency::task<Windows::Devices::Bluetooth::BluetoothLEDevice^> ConnectToDevice(unsigned long long address);