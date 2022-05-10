#pragma once

struct Vector3
{
	float X;
	float Y;
	float Z;
};

struct Vector3Short
{
	int16_t X;
	int16_t Y;
	int16_t Z;
};

#pragma pack(push, 1)
struct Packet
{
	Vector3Short Gyro;
	uint8_t ButtonData;
};
#pragma pack(pop)

void ProcessCharacteristicValue(Windows::Storage::Streams::IBuffer^ characteristicValue);
bool TryAlignData();
void OnBluetoothConnected();
void OnBluetoothDisconnected();