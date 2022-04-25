#include "pch.h"
#include "BluetoothHelper.h"

constexpr auto BUFFER_LENGTH = 128;

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

uint8_t buffer[BUFFER_LENGTH];
unsigned int readIndex;
unsigned int writeIndex;

void WriteBuffer(uint8_t byte)
{
	buffer[writeIndex] = byte;
	writeIndex++;
	if (writeIndex >= BUFFER_LENGTH)
	{
		writeIndex = 0;
	}
}

uint8_t ReadBuffer()
{
	uint8_t byte = buffer[readIndex];
	readIndex++;
	if (readIndex >= BUFFER_LENGTH)
	{
		readIndex = 0;
	}

	return byte;
}

int BufferCount()
{
	int difference = writeIndex - readIndex;
	if (difference < 0) difference += BUFFER_LENGTH;
	return difference;
}

void ProcessCharacteristicValue(Windows::Storage::Streams::IBuffer^ characteristicValue)
{
	auto reader = Windows::Storage::Streams::DataReader::FromBuffer(characteristicValue);
	for (unsigned int i = 0; i < characteristicValue->Length; i++)
	{
		WriteBuffer(reader->ReadByte());
	}
}

int main(Platform::Array<Platform::String^>^ args)
{
	WriteBuffer(10);
	ReadBuffer();
	WriteBuffer(10);
	ReadBuffer();
	WriteBuffer(10);
	WriteBuffer(10);
	WriteBuffer(10);
	WriteBuffer(10);

	std::cout << "Read index: " << readIndex << std::endl;
	std::cout << "Write index: " << writeIndex << std::endl;

	std::cout << "Buffer length: " << BufferCount() << std::endl;
	std::cout << (int)ReadBuffer() << std::endl;
	std::cout << "Buffer length: " << BufferCount() << std::endl;

	return 0;

	HRESULT hresult = RoInitialize(RO_INIT_MULTITHREADED);

	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);

	POINT cursor;

	HDC primary = GetDC(NULL); // get dc for entire screen
	int screenWidth = GetDeviceCaps(primary, HORZRES) - 1;
	int screenHeight = GetDeviceCaps(primary, VERTRES) - 1;
	ReleaseDC(NULL, primary);

	std::cout << screenWidth << ", " << screenHeight << std::endl;

	StartWatcher();

	std::cin.get();

	while (true)
	{
		GetCursorPos(&cursor);
		std::cout << cursor.x << ", " << cursor.y << std::endl;
		Sleep(10);
	}
}