#include "pch.h"
#include "BluetoothHelper.h"

using namespace Windows::Devices;
using namespace Windows::Foundation;
using namespace Platform;

auto serviceUUID = Bluetooth::BluetoothUuidHelper::FromShortId(0xffe0);
auto characteristicUUID = Bluetooth::BluetoothUuidHelper::FromShortId(0xffe1);

HANDLE GetBLEHandle(GUID guid)
{
	return new HANDLE;
}

bool ConnectToDevice(unsigned long long address)
{
	std::cout << "Attempted connection!" << std::endl;
	// implement
	return true;
}

// Some code from: https://github.com/urish/win-ble-cpp
bool Initialize(/*std::string targetName, std::string serviceGuid, std::string characteristicGuid, int maxRetries*/)
{
	std::cout << "Initializing bluetooth watcher" << std::endl;

	auto watcher = ref new Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher();
	watcher->ScanningMode = Bluetooth::Advertisement::BluetoothLEScanningMode::Active;
	watcher->Received += ref new TypedEventHandler<Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher^, Bluetooth::Advertisement::BluetoothLEAdvertisementReceivedEventArgs^>(
		[watcher](Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher^ watcher, Bluetooth::Advertisement::BluetoothLEAdvertisementReceivedEventArgs^ eventArgs) {
			printf("Got advertisement. Address: %llx\n", eventArgs->BluetoothAddress);
			
			auto serviceUUIDs = eventArgs->Advertisement->ServiceUuids;
			unsigned int index = -1;
			if (serviceUUIDs->IndexOf(serviceUUID, &index)) 
			{
				printf("Found device with service. Address: %llx\n", eventArgs->BluetoothAddress);

				watcher->Stop();
				std::wcout << "Name: " << eventArgs->Advertisement->LocalName->Data() << std::endl;
				ConnectToDevice(eventArgs->BluetoothAddress);
			}
		}
	);

	watcher->Start();

	return true;
}