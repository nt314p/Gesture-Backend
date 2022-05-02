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
		return;

	auto address = eventArgs->BluetoothAddress;

	printf("Got advertisement. Address: %llx\n", address);

	auto serviceUUIDs = eventArgs->Advertisement->ServiceUuids;
	unsigned int index = 0;

	if (!serviceUUIDs->IndexOf(serviceUUID, &index)) return;

	std::cout << "Found device with matching service!" << std::endl;

	watcher->Stop();

	auto bleDevice = ConnectToDevice(address).get();

	while (!Initialize(bleDevice, serviceUUID, characteristicUUID).get())
	{
		std::cout << "Failed to initialize device, retrying..." << std::endl;
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
	const auto SuccessStatus = Bluetooth::GenericAttributeProfile::GattCommunicationStatus::Success;
	const auto NotifyDescriptorValue =
		Bluetooth::GenericAttributeProfile::GattClientCharacteristicConfigurationDescriptorValue::Notify;

	std::cout << "Initializing device..." << std::endl;

	auto servicesResult = co_await bleDevice->GetGattServicesForUuidAsync(serviceGuid, Bluetooth::BluetoothCacheMode::Uncached);

	if (servicesResult->Status != SuccessStatus)
	{
		std::cout << "Unable to fetch service" << std::endl;
		co_return false;
	}

	std::cout << "Service status: Success" << std::endl;

	auto customService = servicesResult->Services->GetAt(0);

	auto characteristicsResult = co_await customService->GetCharacteristicsForUuidAsync(characteristicGuid,
		Bluetooth::BluetoothCacheMode::Uncached);

	if (characteristicsResult->Status != SuccessStatus) co_return false;

	std::cout << "Characteristic status: Success" << std::endl;

	auto customCharacteristic = characteristicsResult->Characteristics->GetAt(0);
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

	if (characteristicConfig->ClientCharacteristicConfigurationDescriptor == NotifyDescriptorValue) co_return false;
	
	std::cout << "Characteristic notifications successfully enabled" << std::endl;

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
	auto bleWatcher = ref new Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher();
	bleWatcher->ScanningMode = Bluetooth::Advertisement::BluetoothLEScanningMode::Active;

	bleWatcher->Received += ref new TypedEventHandler<Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher^,
		Bluetooth::Advertisement::BluetoothLEAdvertisementReceivedEventArgs^>(
			[bleWatcher](Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher^ watcher,
				Bluetooth::Advertisement::BluetoothLEAdvertisementReceivedEventArgs^ eventArgs)
			{
				OnAdvertisementReceived(watcher, eventArgs);
			}
	);

	bleWatcher->Start();

	std::cout << "Watching for bluetooth advertisements..." << std::endl;
}