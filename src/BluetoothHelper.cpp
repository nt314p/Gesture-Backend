#include "pch.h"
#include "BluetoothHelper.h"
#include "Main.h"

using namespace Windows::Devices;
using namespace Windows::Foundation;
using namespace Platform;

auto serviceUUID = Bluetooth::BluetoothUuidHelper::FromShortId(0xffe0);
auto characteristicUUID = Bluetooth::BluetoothUuidHelper::FromShortId(0xffe1);

void OnAdvertisementReceived(Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher^ watcher,
	Bluetooth::Advertisement::BluetoothLEAdvertisementReceivedEventArgs^ eventArgs)
{
	if (watcher->Status == Bluetooth::Advertisement::BluetoothLEAdvertisementWatcherStatus::Stopped)
	{
		return;
	}

	auto address = eventArgs->BluetoothAddress;

	printf("Got advertisement. Address: %llx\n", address);

	auto serviceUUIDs = eventArgs->Advertisement->ServiceUuids;
	unsigned int index = -1;
	if (serviceUUIDs->IndexOf(serviceUUID, &index))
	{
		std::cout << "Found device with matching service!" << std::endl;

		watcher->Stop();

		auto bleDevice = ConnectToDevice(address).get();

		while (!Initialize(bleDevice, serviceUUID, characteristicUUID).get())
		{
			std::cout << "Failed to initialize device, retrying..." << std::endl;
		}
	}
}

void CharacteristicOnValueChanged(Bluetooth::GenericAttributeProfile::GattCharacteristic^ sender,
	Bluetooth::GenericAttributeProfile::GattValueChangedEventArgs^ args)
{
	ProcessCharacteristicValue(args->CharacteristicValue);
}

concurrency::task<Bluetooth::BluetoothLEDevice^> ConnectToDevice(unsigned long long address)
{
	co_return co_await Bluetooth::BluetoothLEDevice::FromBluetoothAddressAsync(address);
}

concurrency::task<bool> Initialize(Bluetooth::BluetoothLEDevice^ bleDevice, Guid serviceGuid, Guid characteristicGuid)
{
	std::cout << "Initializing device..." << std::endl;

	auto servicesResult = co_await bleDevice->GetGattServicesForUuidAsync(serviceGuid, Bluetooth::BluetoothCacheMode::Uncached);

	if (servicesResult->Status != Bluetooth::GenericAttributeProfile::GattCommunicationStatus::Success)
	{
		std::cout << "Unable to fetch service" << std::endl;
		co_return false;
	}

	std::cout << "Service status: Success" << std::endl;

	auto customService = servicesResult->Services->GetAt(0);

	auto characteristicsResult = co_await customService->GetCharacteristicsForUuidAsync(characteristicGuid, 
		Bluetooth::BluetoothCacheMode::Uncached);


	if (characteristicsResult->Status != Bluetooth::GenericAttributeProfile::GattCommunicationStatus::Success) 
		co_return false;

	std::cout << "Characteristic status: Success" << std::endl;

	auto customCharacteristic = characteristicsResult->Characteristics->GetAt(0);
	auto descriptorResult = co_await customCharacteristic->GetDescriptorsAsync(Bluetooth::BluetoothCacheMode::Uncached);

	if (descriptorResult->Status != Bluetooth::GenericAttributeProfile::GattCommunicationStatus::Success)
		co_return false;

	std::cout << "Descriptor status: Success" << std::endl;

	auto writeConfig = co_await customCharacteristic->WriteClientCharacteristicConfigurationDescriptorWithResultAsync(
		Bluetooth::GenericAttributeProfile::GattClientCharacteristicConfigurationDescriptorValue::Notify
	);

	if (writeConfig->Status != Bluetooth::GenericAttributeProfile::GattCommunicationStatus::Success)
		co_return false;

	std::cout << "Write characteristic config status: Success" << std::endl;

	auto characteristicConfig = co_await customCharacteristic->ReadClientCharacteristicConfigurationDescriptorAsync();

	if (characteristicConfig->Status != Bluetooth::GenericAttributeProfile::GattCommunicationStatus::Success)
		co_return false;

	std::cout << "Read characteristic config status: Success " << std::endl;

	if (characteristicConfig->ClientCharacteristicConfigurationDescriptor ==
		Bluetooth::GenericAttributeProfile::GattClientCharacteristicConfigurationDescriptorValue::Notify)
	{
		std::cout << "Characteristic notifications successfully enabled" << std::endl;
	}

	customCharacteristic->ValueChanged += ref new TypedEventHandler<Bluetooth::GenericAttributeProfile::GattCharacteristic^,
		Bluetooth::GenericAttributeProfile::GattValueChangedEventArgs^>(
			[customCharacteristic](Bluetooth::GenericAttributeProfile::GattCharacteristic^ sender,
				Bluetooth::GenericAttributeProfile::GattValueChangedEventArgs^ args)
			{
				CharacteristicOnValueChanged(sender, args);
			}
	);

	co_return true;
}

// Some code from: https://github.com/urish/win-ble-cpp
void StartWatcher()
{
	std::cout << "Initializing bluetooth watcher" << std::endl;

	auto watcher = ref new Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher();
	watcher->ScanningMode = Bluetooth::Advertisement::BluetoothLEScanningMode::Active;

	watcher->Received += ref new TypedEventHandler<Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher^,
		Bluetooth::Advertisement::BluetoothLEAdvertisementReceivedEventArgs^>(
			[watcher](Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher^ watcher,
				Bluetooth::Advertisement::BluetoothLEAdvertisementReceivedEventArgs^ eventArgs)
			{
				// apparently just passing in the function doesn't work
				// so I had to create a lambda to call the function
				OnAdvertisementReceived(watcher, eventArgs); 
			}
	);

	watcher->Start();
}