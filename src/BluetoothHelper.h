#pragma once

void StartWatcher();

concurrency::task<bool> Initialize(Windows::Devices::Bluetooth::BluetoothLEDevice^ bleDevice, 
	Platform::Guid serviceGuid, Platform::Guid characteristicGuid);

concurrency::task<Windows::Devices::Bluetooth::BluetoothLEDevice^> ConnectToDevice(unsigned long long address);