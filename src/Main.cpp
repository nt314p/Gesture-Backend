#include "pch.h"
#include <chrono>
#include "BluetoothHelper.h"
#include "Main.h"

constexpr auto Signature = 0b10101000;
constexpr auto BufferLength = 128;
constexpr auto PacketAlignmentAttemptsThreshold = 10000;
constexpr auto SequentialValidPacketsToAlign = 5;
constexpr auto DegreeRange = 500.0f;

INPUT input;

float mouseX = 0;
float mouseY = 0;

Packet currentPacket = Packet();

uint8_t buffer[BufferLength] = {};
int readIndex = 0;
int writeIndex = 0;

uint8_t byteValidCount[sizeof(Packet)] = {}; // byteValidCount[i] stores how many times the ith byte had a valid signature
int byteIndex = 0;
bool isDataAligned = false;
int attemptedPacketAlignments = 0;

auto previousTime = std::chrono::system_clock::now();

void WriteBuffer(uint8_t byte)
{
	buffer[writeIndex] = byte;
	writeIndex++;
	if (writeIndex >= BufferLength)
		writeIndex = 0;
}

uint8_t ReadBuffer()
{
	uint8_t byte = buffer[readIndex];
	readIndex++;
	if (readIndex >= BufferLength) 
		readIndex = 0;

	return byte;
}

int BufferCount()
{
	int difference = writeIndex - readIndex;
	if (difference < 0) difference += BufferLength;
	return difference;
}

Vector3 ToVector3(Vector3Short v, float range)
{
	return {
		v.X * range / INT16_MAX,
		v.Y * range / INT16_MAX,
		v.Z * range / INT16_MAX,
	};
}

bool TryProcessPacket()
{
	static uint8_t byteBuffer[sizeof(Packet)];

	for (int i = 0; i < sizeof(Packet); i++)
	{
		byteBuffer[i] = ReadBuffer();
	}

	Packet p = *(Packet*)byteBuffer;
	currentPacket.Gyro = p.Gyro;
	currentPacket.ScrollData = p.ScrollData;
	currentPacket.ButtonData = p.ButtonData;

	if ((p.ButtonData & 0b11111000) == Signature) return true;

	isDataAligned = false;
	attemptedPacketAlignments = 0;
	std::cout << "Data misaligned!" << std::endl;

	return false;
}

void OnDataAligned()
{
	std::cout << "Data aligned!" << std::endl;
}

// Attempts to align data currently in the buffer
bool TryAlignData()
{
	if (attemptedPacketAlignments > PacketAlignmentAttemptsThreshold)
	{
		std::cout << "Tried to align " << PacketAlignmentAttemptsThreshold << " packets with no success" << std::endl;
		return false;
	}

	while (BufferCount() > 0)
	{
		if ((ReadBuffer() & 0b11111000) == Signature)
		{
			byteValidCount[byteIndex]++;
			if (byteValidCount[byteIndex] >= SequentialValidPacketsToAlign)
			{
				isDataAligned = true;
				OnDataAligned();
				return true;
			}
		}
		else
		{
			byteValidCount[byteIndex] = 0;
		}

		byteIndex++;

		if (byteIndex >= sizeof(Packet))
		{
			byteIndex = 0;
			attemptedPacketAlignments++;
		}
	}

	return false;
}

void ProcessCharacteristicValue(Windows::Storage::Streams::IBuffer^ characteristicValue)
{
	auto reader = Windows::Storage::Streams::DataReader::FromBuffer(characteristicValue);
	while (reader->UnconsumedBufferLength > 0)
	{
		WriteBuffer(reader->ReadByte());
	}

	if (!isDataAligned)
	{
		TryAlignData();
		return;
	}

	if (BufferCount() < sizeof(Packet)) return;

	if (!TryProcessPacket()) return;

	Vector3 gyro = ToVector3(currentPacket.Gyro, DegreeRange);

	auto t = std::chrono::system_clock::now();
	auto us = std::chrono::duration_cast<std::chrono::microseconds>(t - previousTime).count();

	auto elapsed = std::chrono::system_clock::now().time_since_epoch();
	auto elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();

	std::cout << elapsedUs << "\t" << us << std::endl;
	previousTime = t;

	//std::cout << gyro.X << "\t\t" << gyro.Y << "\t\t" << gyro.Z << std::endl;
}

void MouseClick(DWORD flags)
{
	input.mi.dx = 0;
	input.mi.dy = 0;
	input.mi.dwFlags = flags;
	input.mi.mouseData = 0;

	UINT numEvents = SendInput(1, &input, sizeof(input));
	if (numEvents == 0)
	{
		std::cout << "SendInput MouseClick failed: " << HRESULT_FROM_WIN32(GetLastError()) << std::endl;
	}
}

int main(Platform::Array<Platform::String^>^ args)
{
	HRESULT hresult = RoInitialize(RO_INIT_MULTITHREADED);

	if (hresult != S_OK)
	{
		if (hresult == S_FALSE) RoUninitialize();
		return 0;		
	}

	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);

	POINT cursor;

	HDC primary = GetDC(NULL); // get dc for entire screen
	int screenWidth = GetDeviceCaps(primary, HORZRES) - 1;
	int screenHeight = GetDeviceCaps(primary, VERTRES) - 1;
	ReleaseDC(NULL, primary);

	std::cout << screenWidth << ", " << screenHeight << std::endl;

	MOUSEINPUT mouseInput;
	mouseInput.dx = 0;
	mouseInput.dy = 0;
	mouseInput.time = 0;
	mouseInput.mouseData = 0;
	mouseInput.dwExtraInfo = NULL;

	input.type = INPUT_MOUSE;
	input.mi = mouseInput;

	StartWatcher();

	std::cin.get();

	while (true)
	{
		GetCursorPos(&cursor);
		std::cout << cursor.x << ", " << cursor.y << std::endl;
		Sleep(10);
	}
}