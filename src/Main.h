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

struct Packet
{
	Vector3Short Gyro;
	int8_t ScrollData;
	uint8_t ButtonData;
};

enum MiddleMouseAction
{
	None, // Middle mouse is not pressed
	Undetermined, // Middle mouse has been pressed but we cannot determine an action
	Scroll,
	Drag
};

void ProcessCharacteristicValue(Windows::Storage::Streams::IBuffer^ characteristicValue);

bool TryAlignData();
