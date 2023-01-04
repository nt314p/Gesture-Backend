#pragma once

constexpr auto Signature = 0b10101000;
constexpr auto BufferLength = 128;
constexpr auto PacketAlignmentAttemptsThreshold = 1000;
constexpr auto MaxPacketBacklog = 3U;
constexpr auto SequentialValidPacketsToAlign = 5;
constexpr auto DataTimeoutThresholdMs = 300;
constexpr auto DataTimeoutDisconnectThresholdMs = 3000;

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

void ProcessCharacteristicValue(Windows::Storage::Streams::IBuffer^ characteristicValue);
bool TryAlignData();
void OnBluetoothConnected();
void OnBluetoothDisconnected();