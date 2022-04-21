#include "pch.h"

using namespace Windows::Devices;

auto serviceUUID = Bluetooth::BluetoothUuidHelper::FromShortId(0xffe5);
auto characteristicUUID = Bluetooth::BluetoothUuidHelper::FromShortId(0xffe9);

HANDLE GetBLEHandle(GUID guid)
{
	return new HANDLE;
}

bool Initialize(std::string targetName, std::string serviceGuid, std::string characteristicGuid, int maxRetries)
{

	Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher^ bleAdvertisementWatcher = ref new Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher();
	return true;
}