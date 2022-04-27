#pragma once

struct Vector3
{
	float X;
	float Y;
	float Z;
};

struct Vector3Short
{
	INT16 X;
	INT16 Y;
	INT16 Z;
};

struct Packet
{
	Vector3Short Gyro;
	INT8 ScrollData;
	UINT8 ButtonData;
};

void ProcessCharacteristicValue(Windows::Storage::Streams::IBuffer^ characteristicValue);

bool TryAlignData();
