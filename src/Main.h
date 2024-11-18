#pragma once

constexpr auto WM_APP_NOTIFYCALLBACK = WM_APP + 1U;

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