#pragma once
#include "pch.h"
#include "CircularBuffer.h"
#include "Main.h"

namespace BluetoothLE
{
	using namespace Windows::Devices;
	using namespace Platform;

	static constexpr auto BluetoothScanningTimeoutMinutes = 5; // TODO: implement timeout

	class BLEDevice
	{
	private:
		Guid ServiceUUID; // TODO: test if moving these back throws errors
		Guid CharacteristicUUID;
		String^ Pin;// = ref new String(L"802048");
		bool isConnected;
		CircularBuffer buffer;

		Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher^ bleWatcher;
		Bluetooth::BluetoothLEDevice^ bleDevice;
		Bluetooth::GenericAttributeProfile::GattCharacteristic^ customCharacteristic;

		unsigned int bleDeviceInitializationTries = 0;

		concurrency::task<Bluetooth::GenericAttributeProfile::GattDeviceServicesResult^> FetchServices();
		concurrency::task<bool> InitializeBLEDevice();
		concurrency::task<Bluetooth::BluetoothLEDevice^> ConnectToDevice(unsigned long long address);

		concurrency::task<bool> PairToDevice();
		concurrency::task<void> UnpairFromDevice();

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

		void InitializeWatcher();
		void AttemptConnection();
		bool IsConnected();
		void Disconnect();

		BLEDevice(unsigned int serviceId, unsigned int characteristicId, const wchar_t* pin);
	};
}