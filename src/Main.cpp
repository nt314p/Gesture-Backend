#include "pch.h"
#include <chrono>
#include "InputHandler.h"
#include "BluetoothHandler.h"
#include "Main.h"

constexpr auto Signature = 0b10101000;
constexpr auto BufferLength = 128;
constexpr auto PacketAlignmentAttemptsThreshold = 10000;
constexpr auto SequentialValidPacketsToAlign = 5;

Packet currentPacket;

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

bool TryProcessPacket()
{
	uint8_t* byteBuffer = (uint8_t*)&currentPacket;

	for (int i = 0; i < sizeof(Packet); i++)
	{
		byteBuffer[i] = ReadBuffer();
	}

	if ((currentPacket.ButtonData & 0b11111000) == Signature) return true;

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

void PrintByte(uint8_t byte)
{
	// 0b [8 bits] \0
	char str[2 + 8 + 1];
	str[0] = '0';
	str[1] = 'b';
	str[10] = '\0';

	for (int i = 0; i < 8; i++)
	{
		str[9 - i] = byte & 1 << i ? '1' : '0';
	}

	std::cout << str << std::endl;
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

	while (BufferCount() >= sizeof(Packet))
	{
		if (!TryProcessPacket()) return;
		ProcessInput(currentPacket);
	}

	auto t = std::chrono::system_clock::now();
	auto us = std::chrono::duration_cast<std::chrono::microseconds>(t - previousTime).count();

	auto elapsed = std::chrono::system_clock::now().time_since_epoch();
	auto elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();

	//std::cout << elapsedUs << "\t" << us << std::endl;
	previousTime = t;
}

void OnBluetoothConnected()
{
	std::cout << "Device connected!" << std::endl;
}

void OnBluetoothDisconnected()
{
	std::cout << "Device disconnected!" << std::endl;
}

int main(Platform::Array<Platform::String^>^ args)
{
	std::cout << "Starting GestureBackend..." << std::endl;

	HRESULT hresult = RoInitialize(RO_INIT_MULTITHREADED);

	if (hresult != S_OK)
	{
		if (hresult == S_FALSE)
			RoUninitialize();
		else
			return 0;
	}

	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);

	InitializeInput();
	InitializeBLEWatcher();
	AttemptBluetoothConnection();

	while (true)
	{
		Sleep(1000);
	}
}