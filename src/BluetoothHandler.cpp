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

	unsigned int bleDeviceInitializationTries = 0;

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

	void PrintPairingInfo()
	{
		std::cout << "Is paired: " << bleDevice->DeviceInformation->Pairing->IsPaired << std::endl;
		std::cout << "Can pair: " << bleDevice->DeviceInformation->Pairing->CanPair << std::endl;
		std::cout << "Protection level: " << (int)bleDevice->DeviceInformation->Pairing->ProtectionLevel << std::endl;
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

		if (characteristicsResult->Status != SuccessStatus)
		{
			std::cout << "Unable to fetch characteristics" << std::endl;
			std::cout << (int)characteristicsResult->Status << std::endl;
			co_return false;
		}

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
		std::cout << "Received pairing request!" << std::endl;
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

	concurrency::task<bool> PairToDevice()
	{
		std::cout << "Triggered pairing..." << std::endl;

		using namespace Windows::Devices::Enumeration;

		std::cout << "Protection level of device is " << 
			(int)bleDevice->DeviceInformation->Pairing->ProtectionLevel << std::endl;

		auto pairResult = co_await bleDevice->DeviceInformation->Pairing->Custom->PairAsync(
			DevicePairingKinds::ProvidePin);

		if (pairResult->Status != DevicePairingResultStatus::Paired)
		{
			std::cout << "Pairing unsuccessful!" << std::endl;
			std::cout << (int)pairResult->Status << std::endl;
			co_return false;
		}
		
		if (pairResult->ProtectionLevelUsed != DevicePairingProtectionLevel::EncryptionAndAuthentication)
		{
			std::cout << "Paired with uncorrect protection level. Likely unable to receive data..." << std::endl;
		}

		co_return true;
	}
	
	concurrency::task<void> UnpairFromDevice()
	{
		using namespace Windows::Devices::Enumeration;
		auto unpairingResult = co_await bleDevice->DeviceInformation->Pairing->UnpairAsync();
		if (unpairingResult->Status == DeviceUnpairingResultStatus::Unpaired)
			std::cout << "Successfully unpaired" << std::endl;
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

		PrintPairingInfo();

		if (bleDevice->DeviceInformation->Pairing->IsPaired && false)
		{
			std::cout << "Device is already paired, attempting to unpair..." << std::endl;
			UnpairFromDevice().get();
			bleDevice = nullptr;
			AttemptConnection();
			return;
		}

		while (!bleDevice->DeviceInformation->Pairing->IsPaired)
		{
			std::cout << "Device is not paired, attempting to pair..." << std::endl;
			if (PairToDevice().get()) break;
			
			bleDevice = nullptr;
			Sleep(3000);
			bleDevice = ConnectToDevice(address).get();
			bleDevice->DeviceInformation->Pairing->Custom->PairingRequested +=
				ref new TypedEventHandler<DeviceInformationCustomPairing^, DevicePairingRequestedEventArgs^>(&OnPairingRequested);
		}

		PrintPairingInfo();

		while (!InitializeBLEDevice().get())
		{
			std::cout << "Failed to initialize device, retrying..." << std::endl;
			bleDevice = nullptr;
			bleDevice = ConnectToDevice(address).get();
			bleDevice->DeviceInformation->Pairing->Custom->PairingRequested +=
				ref new TypedEventHandler<DeviceInformationCustomPairing^, DevicePairingRequestedEventArgs^>(&OnPairingRequested);
			bleDeviceInitializationTries++;
			Sleep(3000);
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

	void Disconnect()
	{
		isConnected = false;
		customCharacteristic = nullptr;
		bleDevice = nullptr;
	}
}