#pragma once

constexpr auto Signature = 0b10101000;
constexpr auto BufferLength = 128;
constexpr auto PacketAlignmentAttemptsThreshold = 10000;
constexpr auto SequentialValidPacketsToAlign = 5;
constexpr auto DataTimeoutThresholdMs = 400;

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