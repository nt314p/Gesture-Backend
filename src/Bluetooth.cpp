#include "pch.h"
#include "Bluetooth.h"
#include "CircularBuffer.h"
#include <ppltasks.h>

namespace BluetoothLE
{
	using namespace Windows::Devices;
	using namespace Windows::Foundation;
	using namespace Platform;

	static CircularBuffer buffer;

	static unsigned int bleDeviceInitializationTries = 0;
	static bool isConnected;

	std::function<void()> Connected;
	std::function<void()> Disconnected;
	std::function<void()> ReceivedData;

	static Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher^ bleWatcher;
	static Bluetooth::BluetoothLEDevice^ bleDevice;
	static Bluetooth::GenericAttributeProfile::GattCharacteristic^ customCharacteristic;

	bool IsConnected()
	{
		return isConnected;
	}

	void OnCharacteristicValueChanged(Bluetooth::GenericAttributeProfile::GattCharacteristic^ sender,
		Bluetooth::GenericAttributeProfile::GattValueChangedEventArgs^ args)
	{
		// std::cout << "Characteristic value changed!" << std::endl;
		auto reader = Windows::Storage::Streams::DataReader::FromBuffer(args->CharacteristicValue);
		while (reader->UnconsumedBufferLength > 0)
		{
			buffer.WriteBuffer(reader->ReadByte());
		}

		ReceivedData();
	}

	void PrintPairingInfo()
	{
		std::cout << "Is paired: " << bleDevice->DeviceInformation->Pairing->IsPaired << std::endl;
		std::cout << "Can pair: " << bleDevice->DeviceInformation->Pairing->CanPair << std::endl;
		std::cout << "Protection level: " << (int)bleDevice->DeviceInformation->Pairing->ProtectionLevel << std::endl;
	}

	concurrency::task<Bluetooth::GenericAttributeProfile::GattDeviceServicesResult^> FetchServices()
	{
		Bluetooth::GenericAttributeProfile::GattDeviceServicesResult^ servicesResult = nullptr;
		try
		{
			auto x = bleDevice->GetGattServicesForUuidAsync(ServiceUUID, Bluetooth::BluetoothCacheMode::Uncached);
			return co_await x;
		}
		catch (...)
		{
			std::cout << "Exception!" << std::endl;
			co_return nullptr;
		}
	}

	concurrency::task<bool> InitializeBLEDevice()
	{
		using namespace Bluetooth::GenericAttributeProfile;
		constexpr auto SuccessStatus = GattCommunicationStatus::Success;
		constexpr auto NotifyDescriptorValue = GattClientCharacteristicConfigurationDescriptorValue::Notify;

		std::cout << "Initializing device..." << std::endl;

		/*
		std::cout << (bleDevice == nullptr) << std::endl;

		Sleep(1000);

		auto getServicesTask =
			concurrency::create_task(
				bleDevice->GetGattServicesForUuidAsync(serviceUUID, Bluetooth::BluetoothCacheMode::Uncached));

		GattDeviceServicesResult^ servicesResult = nullptr;

		getServicesTask.then([](GattDeviceServicesResult^ result)
			{
				std::cout << (int)result->Status << std::endl;
			})
			.then([](concurrency::task<void> t)
				{
					try
					{
						t.get();
						std::cout << "Success I think" << std::endl;
					}
					catch (...)
					{
						std::cout << "Exception!" << std::endl;
					}
				});*/

		auto servicesResult =
			co_await bleDevice->GetGattServicesForUuidAsync(ServiceUUID, Bluetooth::BluetoothCacheMode::Uncached);

		if (servicesResult->Status != SuccessStatus)
		{
			std::cout << "Unable to fetch service" << std::endl;
			std::cout << (int)servicesResult->Status << std::endl;
			co_return false;
		}

		std::cout << "Service status: Success" << std::endl;

		auto customService = servicesResult->Services->GetAt(0);

		auto characteristicsResult = co_await customService->GetCharacteristicsForUuidAsync(CharacteristicUUID,
			Bluetooth::BluetoothCacheMode::Uncached);

		if (characteristicsResult->Status != SuccessStatus)
		{
			std::cout << "Unable to fetch characteristics" << std::endl;
			std::cout << (int)characteristicsResult->Status << std::endl;
			co_return false;
		}

		std::cout << "Characteristic status: Success" << std::endl;

		customCharacteristic = characteristicsResult->Characteristics->GetAt(0);
		auto descriptorResult =
			co_await customCharacteristic->GetDescriptorsAsync(Bluetooth::BluetoothCacheMode::Uncached);

		if (descriptorResult->Status != SuccessStatus) co_return false;

		std::cout << "Descriptor status: Success" << std::endl;

		auto writeConfig =
			co_await customCharacteristic->WriteClientCharacteristicConfigurationDescriptorWithResultAsync(
				NotifyDescriptorValue
			);

		if (writeConfig->Status != SuccessStatus) co_return false;

		std::cout << "Write characteristic config status: Success" << std::endl;

		auto characteristicConfig =
			co_await customCharacteristic->ReadClientCharacteristicConfigurationDescriptorAsync();

		if (characteristicConfig->Status != SuccessStatus) co_return false;

		std::cout << "Read characteristic config status: Success " << std::endl;

		if (characteristicConfig->ClientCharacteristicConfigurationDescriptor != NotifyDescriptorValue)
			co_return false;

		std::cout << "Characteristic notifications successfully enabled" << std::endl;

		using namespace Bluetooth::GenericAttributeProfile;
		customCharacteristic->ValueChanged +=
			ref new TypedEventHandler<GattCharacteristic^, GattValueChangedEventArgs^>(&OnCharacteristicValueChanged);

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
		args->Accept(BLEPin);
	}

	void OnConnectionChanged(Bluetooth::BluetoothLEDevice^ sender, Platform::Object^ args)
	{
		if (sender->ConnectionStatus == Bluetooth::BluetoothConnectionStatus::Disconnected)
		{
			isConnected = false;
			customCharacteristic = nullptr;
			bleDevice = nullptr;
			Disconnected();
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
		auto index = 0U;

		if (!serviceUUIDs->IndexOf(ServiceUUID, &index)) return;

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
				ref new TypedEventHandler<DeviceInformationCustomPairing^,
				DevicePairingRequestedEventArgs^>(&OnPairingRequested);

			bleDeviceInitializationTries++;
			Sleep(3000);
		}

		bleDevice->ConnectionStatusChanged +=
			ref new TypedEventHandler<Bluetooth::BluetoothLEDevice^, Platform::Object^>(&OnConnectionChanged);

		isConnected = true;
		Connected();
	}

	void InitializeWatcher()
	{
		using namespace Bluetooth::Advertisement;
		bleWatcher = ref new BluetoothLEAdvertisementWatcher();
		bleWatcher->ScanningMode = BluetoothLEScanningMode::Active;

		bleWatcher->Received += ref new TypedEventHandler<BluetoothLEAdvertisementWatcher^,
			BluetoothLEAdvertisementReceivedEventArgs^>(&OnAdvertisementReceived);
	}

	void AttemptConnection()
	{
		bleWatcher->Stop();
		bleWatcher->Start();
		std::cout << "Watching for BLE device advertisements..." << std::endl;
	}

	void Disconnect()
	{
		// TODO: also unsubscribe from callbacks?
		isConnected = false;
		customCharacteristic = nullptr;
		bleDevice = nullptr;
	}
}