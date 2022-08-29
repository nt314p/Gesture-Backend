#include "pch.h"
#include "BluetoothHandler.h"
#include "Main.h"

using namespace Windows::Devices;
using namespace Windows::Foundation;
using namespace Platform;

namespace BluetoothHandler
{
	auto serviceUUID = Bluetooth::BluetoothUuidHelper::FromShortId(0xffe0);
	auto characteristicUUID = Bluetooth::BluetoothUuidHelper::FromShortId(0xffe1);
	auto blePin = ref new String(L"802048");

	bool isConnected;

	Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher^ bleWatcher;
	Bluetooth::BluetoothLEDevice^ bleDevice;
	Bluetooth::GenericAttributeProfile::GattCharacteristic^ customCharacteristic;

	bool IsConnected()
	{
		return isConnected;
	}

	void OnCharacteristicValueChanged(Bluetooth::GenericAttributeProfile::GattCharacteristic^ sender,
		Bluetooth::GenericAttributeProfile::GattValueChangedEventArgs^ args)
	{
		ProcessCharacteristicValue(args->CharacteristicValue);
	}

	concurrency::task<bool> InitializeBLEDevice()
	{
		const auto SuccessStatus = Bluetooth::GenericAttributeProfile::GattCommunicationStatus::Success;
		const auto NotifyDescriptorValue =
			Bluetooth::GenericAttributeProfile::GattClientCharacteristicConfigurationDescriptorValue::Notify;

		std::cout << "Initializing device..." << std::endl;

		auto servicesResult = co_await bleDevice->GetGattServicesForUuidAsync(serviceUUID, Bluetooth::BluetoothCacheMode::Uncached);

		if (servicesResult->Status != SuccessStatus)
		{
			std::cout << "Unable to fetch service" << std::endl;
			std::cout << (int)servicesResult->Status << std::endl;
			co_return false;
		}

		std::cout << "Service status: Success" << std::endl;

		auto customService = servicesResult->Services->GetAt(0);

		auto characteristicsResult = co_await customService->GetCharacteristicsForUuidAsync(characteristicUUID,
			Bluetooth::BluetoothCacheMode::Uncached);

		if (characteristicsResult->Status != SuccessStatus) co_return false;

		std::cout << "Characteristic status: Success" << std::endl;

		customCharacteristic = characteristicsResult->Characteristics->GetAt(0);
		auto descriptorResult = co_await customCharacteristic->GetDescriptorsAsync(Bluetooth::BluetoothCacheMode::Uncached);

		if (descriptorResult->Status != SuccessStatus) co_return false;

		std::cout << "Descriptor status: Success" << std::endl;

		auto writeConfig = co_await customCharacteristic->WriteClientCharacteristicConfigurationDescriptorWithResultAsync(
			NotifyDescriptorValue
		);

		if (writeConfig->Status != SuccessStatus) co_return false;

		std::cout << "Write characteristic config status: Success" << std::endl;

		auto characteristicConfig = co_await customCharacteristic->ReadClientCharacteristicConfigurationDescriptorAsync();

		if (characteristicConfig->Status != SuccessStatus) co_return false;

		std::cout << "Read characteristic config status: Success " << std::endl;

		if (characteristicConfig->ClientCharacteristicConfigurationDescriptor != NotifyDescriptorValue) co_return false;

		std::cout << "Characteristic notifications successfully enabled" << std::endl;

		customCharacteristic->ValueChanged += ref new TypedEventHandler<Bluetooth::GenericAttributeProfile::GattCharacteristic^,
			Bluetooth::GenericAttributeProfile::GattValueChangedEventArgs^>(&OnCharacteristicValueChanged);

		co_return true;
	}

	concurrency::task<Bluetooth::BluetoothLEDevice^> ConnectToDevice(unsigned long long address)
	{
		co_return co_await Bluetooth::BluetoothLEDevice::FromBluetoothAddressAsync(address);
	}

	void OnPairingRequested(Windows::Devices::Enumeration::DeviceInformationCustomPairing^ sender,
		Windows::Devices::Enumeration::DevicePairingRequestedEventArgs^ args)
	{
		std::cout << "Received pairing request!";
		args->Accept(blePin);
	}

	void OnConnectionChanged(Bluetooth::BluetoothLEDevice^ sender, Platform::Object^ args)
	{
		if (sender->ConnectionStatus == Bluetooth::BluetoothConnectionStatus::Disconnected)
		{
			isConnected = false;
			OnBluetoothDisconnected();
			customCharacteristic = nullptr;
			bleDevice = nullptr;
		}
	}

	concurrency::task<void> PairToDevice()
	{
		using namespace Windows::Devices::Enumeration;
		co_await bleDevice->DeviceInformation->Pairing->Custom->PairAsync(DevicePairingKinds::ProvidePin);
		std::cout << "Triggered pairing..." << std::endl;
	}

	void OnAdvertisementReceived(Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher^ watcher,
		Bluetooth::Advertisement::BluetoothLEAdvertisementReceivedEventArgs^ eventArgs)
	{
		if (watcher->Status == Bluetooth::Advertisement::BluetoothLEAdvertisementWatcherStatus::Stopped)
			return;

		auto address = eventArgs->BluetoothAddress;

		printf("Received advertisement. Address: %llx\n", address);

		auto serviceUUIDs = eventArgs->Advertisement->ServiceUuids;
		unsigned int index = 0;

		if (!serviceUUIDs->IndexOf(serviceUUID, &index)) return;

		std::cout << "Found device with matching service!" << std::endl;

		watcher->Stop();

		bleDevice = ConnectToDevice(address).get();

		using namespace Windows::Devices::Enumeration;

		bleDevice->DeviceInformation->Pairing->Custom->PairingRequested +=
			ref new TypedEventHandler<DeviceInformationCustomPairing^, DevicePairingRequestedEventArgs^>(&OnPairingRequested);

		if (!bleDevice->DeviceInformation->Pairing->IsPaired)
		{
			std::cout << "Device is not paired, attempting to pair...";
			//bleDevice->DeviceInformation->Pairing->Custom->PairAsync(DevicePairingKinds::ProvidePin);
			PairToDevice().get();
		}

		std::cout << "Is paired: " << bleDevice->DeviceInformation->Pairing->IsPaired << std::endl;
		std::cout << "Can pair: " << bleDevice->DeviceInformation->Pairing->CanPair << std::endl;
		std::cout << "Protection level: " << (int)bleDevice->DeviceInformation->Pairing->ProtectionLevel << std::endl;

		while (!InitializeBLEDevice().get())
		{
			std::cout << "Failed to initialize device, retrying..." << std::endl;
			Sleep(100);
		}

		bleDevice->ConnectionStatusChanged +=
			ref new TypedEventHandler<Bluetooth::BluetoothLEDevice^, Platform::Object^>(&OnConnectionChanged);

		isConnected = true;
		OnBluetoothConnected();
	}

	void InitializeWatcher()
	{
		bleWatcher = ref new Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher();
		bleWatcher->ScanningMode = Bluetooth::Advertisement::BluetoothLEScanningMode::Active;

		bleWatcher->Received += ref new TypedEventHandler<Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher^,
			Bluetooth::Advertisement::BluetoothLEAdvertisementReceivedEventArgs^>(&OnAdvertisementReceived);
	}

	void AttemptConnection()
	{
		bleWatcher->Stop();
		bleWatcher->Start();
		std::cout << "Watching for BLE device advertisements..." << std::endl;
	}
}