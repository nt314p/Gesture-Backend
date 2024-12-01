#pragma once
#include "pch.h"
#include "CircularBuffer.h"
#include "Main.h"

namespace BluetoothLE
{
	using namespace Windows::Devices;
	using namespace Platform;

	static constexpr auto BluetoothScanningTimeoutMinutes = 5; // TODO: implement timeout
	static constexpr auto DataTimeoutThresholdMs = 200;
	static constexpr auto DataTimeoutDisconnectThresholdMs = 3000; // Disconnects after exceeding this threshold
	// TODO: implement disconnection. ideally object drives its own timer for disconnection?

	class BLEDevice
	{
	private:
		Guid ServiceUUID;
		Guid CharacteristicUUID;
		String^ Pin;

		unsigned int bleDeviceInitializationTries = 0;
		bool isConnected;

		std::chrono::time_point<std::chrono::system_clock> lastReceivedDataTime;

		Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher^ bleWatcher;
		Bluetooth::BluetoothLEDevice^ bleDevice;
		Bluetooth::GenericAttributeProfile::GattCharacteristic^ customCharacteristic;

		concurrency::task<bool> InitializeDevice();
		bool DataTimeoutExceeded() const;

		void OnAdvertisementReceived(Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher^ watcher,
			Bluetooth::Advertisement::BluetoothLEAdvertisementReceivedEventArgs^ eventArgs);

		void OnPairingRequested(Enumeration::DeviceInformationCustomPairing^ sender,
			Enumeration::DevicePairingRequestedEventArgs^ args);

		void OnConnectionChanged(Bluetooth::BluetoothLEDevice^ sender, Platform::Object^ args);

		void OnCharacteristicValueChanged(Bluetooth::GenericAttributeProfile::GattCharacteristic^ sender,
			Bluetooth::GenericAttributeProfile::GattValueChangedEventArgs^ args);

	public:
		std::function<void()> Connected;
		std::function<void()> Disconnected;
		std::function<void()> ReceivedData;

		CircularBuffer buffer;

		void AttemptConnection();
		bool IsConnected() const;
		void Disconnect();

		BLEDevice(unsigned int serviceId, unsigned int characteristicId, const wchar_t* pin);
	};
}