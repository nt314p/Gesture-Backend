#pragma once

constexpr auto WM_APP_NOTIFYCALLBACK = WM_APP + 1U;
constexpr GUID guid = { 0x53d7aa31, 0x8ac9, 0x4661, {0x92, 0x35, 0xd9, 0x29, 0x87, 0x76, 0xc7, 0x2e} };

struct Vector3
{
	float X;
	float Y;
	float Z;
};

struct Vector3Int16
{
	int16_t X;
	int16_t Y;
	int16_t Z;
};

#pragma pack(push, 1)
struct Packet
{
	Vector3Int16 Gyro;
	uint8_t ButtonData;
};
#pragma pack(pop)

void OnBluetoothConnected();
void OnBluetoothDisconnected();