#include "pch.h"
#include "Bluetooth.h"
#include "CircularBuffer.h"
#include <ppltasks.h>

namespace BluetoothLE
{
	using namespace Windows::Devices;
	using namespace Windows::Foundation;

	bool BLEDevice::IsConnected() const
	{
		return isConnected;
	}

	bool BLEDevice::DataTimeoutExceeded() const
	{
		using namespace std::chrono;
		static auto currentTime = system_clock::now();

		currentTime = system_clock::now();

		auto msElapsed = duration_cast<milliseconds>(currentTime - lastReceivedDataTime).count();
		if (msElapsed < DataTimeoutThresholdMs) return false;

		std::cout << "Have not received data for " << msElapsed << " ms" << std::endl;

		return msElapsed >= DataTimeoutDisconnectThresholdMs;
	}

	static void PrintPairingInfo(Bluetooth::BluetoothLEDevice^ bleDevice)
	{
		std::cout << "Is paired: " << bleDevice->DeviceInformation->Pairing->IsPaired << std::endl;
		std::cout << "Can pair: " << bleDevice->DeviceInformation->Pairing->CanPair << std::endl;
		std::cout << "Protection level: " <<
			(int)bleDevice->DeviceInformation->Pairing->ProtectionLevel << std::endl;
	}

	concurrency::task<bool> BLEDevice::InitializeDevice()
	{
		using namespace Bluetooth::GenericAttributeProfile;
		constexpr auto SuccessStatus = GattCommunicationStatus::Success;
		constexpr auto NotifyDescriptorValue = GattClientCharacteristicConfigurationDescriptorValue::Notify;

		std::cout << "Initializing device..." << std::endl;

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
			ref new TypedEventHandler<GattCharacteristic^, GattValueChangedEventArgs^>(
				[this](GattCharacteristic^ characteristic, GattValueChangedEventArgs^ args) {
					this->OnCharacteristicValueChanged(characteristic, args);
				});

		co_return true;
	}

	void BLEDevice::OnCharacteristicValueChanged(Bluetooth::GenericAttributeProfile::GattCharacteristic^ sender,
		Bluetooth::GenericAttributeProfile::GattValueChangedEventArgs^ args)
	{
		auto reader = Windows::Storage::Streams::DataReader::FromBuffer(args->CharacteristicValue);

		while (reader->UnconsumedBufferLength > 0)
		{
			buffer.WriteBuffer(reader->ReadByte());
		}

		lastReceivedDataTime = std::chrono::system_clock::now();

		if (ReceivedData) ReceivedData();
	}

	static concurrency::task<Bluetooth::BluetoothLEDevice^> CoConnectToDevice(uint64_t address)
	{
		co_return co_await Bluetooth::BluetoothLEDevice::FromBluetoothAddressAsync(address);
	}

	static Bluetooth::BluetoothLEDevice^ ConnectToDevice(uint64_t address)
	{
		return CoConnectToDevice(address).get();
	}

	void BLEDevice::OnPairingRequested(Enumeration::DeviceInformationCustomPairing^ sender,
		Enumeration::DevicePairingRequestedEventArgs^ args)
	{
		std::cout << "Received pairing request!" << std::endl;
		args->Accept(Pin);
	}

	void BLEDevice::OnConnectionChanged(Bluetooth::BluetoothLEDevice^ sender, Platform::Object^ args)
	{
		switch (sender->ConnectionStatus)
		{
			
		case Bluetooth::BluetoothConnectionStatus::Connected:
		{
			// This triggers early in the connecting process
			// The connection is not fully verified when it triggers
			// (pairing may not be completed, access to characteristic not verified)
			return;
			isConnected = true;
			if (Connected) Connected();
			break;
		}
		case Bluetooth::BluetoothConnectionStatus::Disconnected:
		{
			isConnected = false;
			customCharacteristic = nullptr;
			bleDevice = nullptr;
			if (Disconnected) Disconnected();
			break;
		}
		}
	}

	static concurrency::task<bool> PairToDevice(Bluetooth::BluetoothLEDevice^ bleDevice)
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
			std::cout << "Paired with incorrect protection level. Likely unable to receive data..." << std::endl;
		}

		co_return true;
	}

	static concurrency::task<void> UnpairFromDevice(Bluetooth::BluetoothLEDevice^ bleDevice)
	{
		using namespace Windows::Devices::Enumeration;
		auto unpairingResult = co_await bleDevice->DeviceInformation->Pairing->UnpairAsync();

		if (unpairingResult->Status == DeviceUnpairingResultStatus::Unpaired)
		{
			std::cout << "Successfully unpaired" << std::endl;
		}
	}

	void BLEDevice::OnAdvertisementReceived(Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher^ watcher,
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

		using namespace Windows::Devices::Enumeration;

		while (true) // TODO: insert delay somewhere
		{
			bleDevice = ConnectToDevice(address);

			bleDevice->ConnectionStatusChanged +=
				ref new TypedEventHandler<Bluetooth::BluetoothLEDevice^, Platform::Object^>(
					[this](Bluetooth::BluetoothLEDevice^ device, Platform::Object^ obj) {
						this->OnConnectionChanged(device, obj);
					});

			bleDevice->DeviceInformation->Pairing->Custom->PairingRequested +=
				ref new TypedEventHandler<DeviceInformationCustomPairing^, DevicePairingRequestedEventArgs^>(
					[this](DeviceInformationCustomPairing^ pairing, DevicePairingRequestedEventArgs^ args) {
						this->OnPairingRequested(pairing, args);
					});

			if (!bleDevice->DeviceInformation->Pairing->IsPaired)
			{
				std::cout << "Device is not paired, attempting to pair..." << std::endl;
				if (!PairToDevice(bleDevice).get())
				{
					std::cout << "Failed to pair!" << std::endl;
					bleDevice = nullptr;
					Sleep(3000);
					continue;
				}
			}

			if (InitializeDevice().get()) break;

			std::cout << "Failed to initialize device!" << std::endl;
			bleDevice = nullptr;
			Sleep(3000);
		}

		isConnected = true;
		if (Connected) Connected();
	}

	void BLEDevice::AttemptConnection()
	{
		bleWatcher->Stop();
		bleWatcher->Start();
		std::cout << "Watching for BLE device advertisements..." << std::endl;
	}

	void BLEDevice::Disconnect()
	{
		// TODO: also unsubscribe from callbacks?
		bleWatcher->Stop();
		isConnected = false;
		customCharacteristic = nullptr;
		bleDevice = nullptr;
	}

	BLEDevice::BLEDevice(unsigned int serviceId, unsigned int characteristicId, const wchar_t* pin)
	{
		ServiceUUID = Bluetooth::BluetoothUuidHelper::FromShortId(serviceId);
		CharacteristicUUID = Bluetooth::BluetoothUuidHelper::FromShortId(characteristicId);
		Pin = ref new String(pin);

		using namespace Bluetooth::Advertisement;
		bleWatcher = ref new BluetoothLEAdvertisementWatcher();
		bleWatcher->ScanningMode = BluetoothLEScanningMode::Active;

		bleWatcher->Received += ref new TypedEventHandler<
			BluetoothLEAdvertisementWatcher^,
			BluetoothLEAdvertisementReceivedEventArgs^>(
				[this](BluetoothLEAdvertisementWatcher^ watcher,
					BluetoothLEAdvertisementReceivedEventArgs^ args) {
						this->OnAdvertisementReceived(watcher, args);
				});
	}
}