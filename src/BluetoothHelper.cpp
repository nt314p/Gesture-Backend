#include "pch.h"
#include "BluetoothHelper.h"

using namespace Windows::Devices;
using namespace Windows::Foundation;
using namespace Platform;

auto serviceUUID = Bluetooth::BluetoothUuidHelper::FromShortId(0xffe0);
auto characteristicUUID = Bluetooth::BluetoothUuidHelper::FromShortId(0xffe1);

static void OnAdvertisementReceived(Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher^ watcher,
	Bluetooth::Advertisement::BluetoothLEAdvertisementReceivedEventArgs^ eventArgs)
{
	printf("Got advertisement. Address: %llx\n", eventArgs->BluetoothAddress);

	auto serviceUUIDs = eventArgs->Advertisement->ServiceUuids;
	unsigned int index = -1;
	if (serviceUUIDs->IndexOf(serviceUUID, &index))
	{
		printf("Found device with service. Address: %llx\n", eventArgs->BluetoothAddress);

		watcher->Stop();
		std::wcout << "Name: " << eventArgs->Advertisement->LocalName->Data() << std::endl;

		auto bleDevice = ConnectToDevice(eventArgs->BluetoothAddress).get();
		auto result = Initialize(bleDevice, serviceUUID, characteristicUUID).get();
	}
}

void CharacteristicOnValueChanged(Bluetooth::GenericAttributeProfile::GattCharacteristic^ sender,
	Bluetooth::GenericAttributeProfile::GattValueChangedEventArgs^ args)
{
	std::cout << "Value changed!" << std::endl;
	std::cout << args->CharacteristicValue->Length << std::endl;
}

concurrency::task<Bluetooth::BluetoothLEDevice^> ConnectToDevice(unsigned long long address)
{
	std::cout << "Connecting..." << std::endl;

	auto bleDevice = co_await Bluetooth::BluetoothLEDevice::FromBluetoothAddressAsync(address);
	// implement
	co_return bleDevice;
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
	std::cout << "Characteristic status: " << characteristicsResult->Status.ToString()->Data() << std::endl;

	if (characteristicsResult->Status == Bluetooth::GenericAttributeProfile::GattCommunicationStatus::AccessDenied) 
		co_return false;

	auto customCharacteristic = characteristicsResult->Characteristics->GetAt(0);
	auto descriptorResult = co_await customCharacteristic->GetDescriptorsAsync(Bluetooth::BluetoothCacheMode::Uncached);
	std::cout << "Descriptor status: " << descriptorResult->Status.ToString()->Data() << std::endl;

	auto writeConfig = co_await customCharacteristic->WriteClientCharacteristicConfigurationDescriptorWithResultAsync(
		Bluetooth::GenericAttributeProfile::GattClientCharacteristicConfigurationDescriptorValue::Notify
	);
	std::cout << "Write characteristic config status: " << writeConfig->Status.ToString()->Data() << std::endl;

	auto characteristicConfig = co_await customCharacteristic->ReadClientCharacteristicConfigurationDescriptorAsync();
	std::cout << "Read characteristic config status: " << characteristicConfig->Status.ToString()->Data() << std::endl;

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
bool StartWatcher(/*std::string targetName, std::string serviceGuid, std::string characteristicGuid, int maxRetries*/)
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

	return true;
}