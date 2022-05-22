#include "pch.h"
#include <chrono>
#include "NotificationHandler.h"
#include "InputHandler.h"
#include "BluetoothHandler.h"
#include "Main.h"

Packet currentPacket;

uint8_t buffer[BufferLength] = {};
int readIndex = 0;
int writeIndex = 0;

uint8_t byteValidCount[sizeof(Packet)] = {}; // byteValidCount[i] stores how many times the ith byte had a valid signature
int byteIndex = 0;
bool isDataAligned = false;
bool isConnected = false;
int attemptedPacketAlignments = 0;

auto previousTime = std::chrono::system_clock::now();
auto lastReceivedPacketTime = std::chrono::system_clock::now();

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
	std::cout << "Data misaligned! Attempting to realign..." << std::endl;

	return false;
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
				std::cout << "Data aligned!" << std::endl;
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

	unsigned int packetBacklog = BufferCount() / sizeof(Packet);
	if (packetBacklog > MaxPacketBacklog)
	{
		for (unsigned int i = 0; i < (packetBacklog - MaxPacketBacklog) * sizeof(Packet); i++)
		{
			ReadBuffer();
		}
	}

	while (BufferCount() >= sizeof(Packet))
	{
		lastReceivedPacketTime = std::chrono::system_clock::now();
		if (!TryProcessPacket()) return;
		InputHandler::ProcessPacket(currentPacket);
	}

	auto t = std::chrono::system_clock::now();
	auto us = std::chrono::duration_cast<std::chrono::microseconds>(t - previousTime).count();

	auto elapsed = std::chrono::system_clock::now().time_since_epoch();
	auto elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();

	/*
	int ms = us / 1000;

	std::cout << elapsedUs << "\t";

	for (int i = 0; i < ms; i++)
	{
		std::cout << "#";
	}

	std::cout << std::endl;*/

	previousTime = t;
}

void OnBluetoothConnected()
{
	std::cout << "Device connected!" << std::endl;
	lastReceivedPacketTime = std::chrono::system_clock::now();
	isConnected = true;
}

void OnBluetoothDisconnected()
{
	std::cout << "Device disconnected!" << std::endl;
	std::cout << "Attempting to reconnect..." << std::endl;
	isDataAligned = false;
	isConnected = false;
	BluetoothHandler::AttemptConnection();
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
		{
			std::cout << "RoInitialize failed" << std::endl;
			return 0;
		}
	}

	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
	
	NotificationHandler::Initialize();

	while (true)
		Sleep(100);

	return 0;

	InputHandler::Initialize();
	BluetoothHandler::InitializeWatcher();
	BluetoothHandler::AttemptConnection();

	auto currentTime = std::chrono::system_clock::now();

	while (true)
	{
		using namespace std::chrono;
		Sleep(200);

		if (!isConnected) continue;

		currentTime = system_clock::now();

		auto msElapsed = duration_cast<milliseconds>(currentTime - lastReceivedPacketTime).count();
		if (msElapsed > DataTimeoutThresholdMs)
		{
			std::cout << "Have not received data for " << msElapsed << " ms" << std::endl;
		}
	}
}