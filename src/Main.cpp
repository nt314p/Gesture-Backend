#include "pch.h"
#include "BluetoothHelper.h"

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

Vector3 ToVector3(Vector3Short v, float range)
{
	return {
		v.X * range / INT16_MAX,
		v.Y * range / INT16_MAX,
		v.Z * range / INT16_MAX,
	};
}

bool TryProcessPacket(UINT8* bytes)
{
	Packet packet = *(Packet*)bytes;

	return true;
}

int main(Platform::Array<Platform::String^>^ args)
{
	HRESULT hresult = RoInitialize(RO_INIT_SINGLETHREADED);

	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);

	POINT cursor;

	HDC primary = GetDC(NULL); // get dc for entire screen
	int screenWidth = GetDeviceCaps(primary, HORZRES) - 1;
	int screenHeight = GetDeviceCaps(primary, VERTRES) - 1;
	ReleaseDC(NULL, primary);

	std::cout << screenWidth << ", " << screenHeight << std::endl;

	Initialize();

	std::cin.get();

	while (true)
	{
		GetCursorPos(&cursor);
		std::cout << cursor.x << ", " << cursor.y << std::endl;
		Sleep(10);
	}
}