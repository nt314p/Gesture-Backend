#include "pch.h"
#include "PacketParser.h"
#include "Input.h"
#include <chrono>

namespace PacketParser
{
	static Packet currentPacket;

	static uint8_t buffer[BufferLength] = {};
	static int readIndex = 0;
	static int writeIndex = 0;

	static uint8_t byteValidCount[sizeof(Packet)] = {}; // byteValidCount[i] stores how many times the ith byte had a valid signature
	static int byteIndex = 0;
	static bool isDataAligned = false;
	static int attemptedPacketAlignments = 0;

	static auto previousTime = std::chrono::system_clock::now();
	static auto lastReceivedDataTime = std::chrono::system_clock::now();

	void WriteBuffer(uint8_t byte)
	{
		buffer[writeIndex] = byte;
		writeIndex++;
		if (writeIndex >= BufferLength)	writeIndex = 0;
	}

	uint8_t ReadBuffer()
	{
		uint8_t byte = buffer[readIndex];
		readIndex++;
		if (readIndex >= BufferLength)	readIndex = 0;
		return byte;
	}

	int BufferCount()
	{
		int difference = writeIndex - readIndex;
		if (difference < 0) difference += BufferLength;
		return difference;
	}

	inline bool HasValidSignature(uint8_t byte)
	{
		return (byte & 0b11111000) == Signature;
	}

	bool TryProcessPacket()
	{
		uint8_t* byteBuffer = (uint8_t*)&currentPacket;

		for (int i = 0; i < sizeof(Packet); i++) // read data into packet struct
		{
			byteBuffer[i] = ReadBuffer();
		}

		if (HasValidSignature(currentPacket.ButtonData)) return true;

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
			if (HasValidSignature(ReadBuffer()))
				byteValidCount[byteIndex]++;
			else
				byteValidCount[byteIndex] = 0;

			if (byteValidCount[byteIndex] >= SequentialValidPacketsToAlign)
			{
				isDataAligned = true;
				std::cout << "Data aligned!" << std::endl;
				return true;
			}

			byteIndex++;

			if (byteIndex < sizeof(Packet)) continue;

			byteIndex = 0;
			attemptedPacketAlignments++;
		}

		return false;
	}

	void ProcessCharacteristicValue(Windows::Storage::Streams::IBuffer^ characteristicValue)
	{
		lastReceivedDataTime = std::chrono::system_clock::now();

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
			std::cout << packetBacklog << std::endl;
		}

		while (BufferCount() >= sizeof(Packet))
		{
			if (!TryProcessPacket()) return;
			//std::cout << packetBacklog << "\t" <<
			//	currentPacket.Gyro.X << "\t" << currentPacket.Gyro.Y << "\t" << currentPacket.Gyro.Z << std::endl;
			Input::ProcessPacket(currentPacket);
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

	void ResetLastReceivedDataTime()
	{
		lastReceivedDataTime = std::chrono::system_clock::now();
	}

	// Marks data as not aligned
	void ResetDataAlignment()
	{
		isDataAligned = false;
	}

	bool DataTimeoutExceeded()
	{
		using namespace std::chrono;
		static auto currentTime = system_clock::now();

		currentTime = system_clock::now();

		auto msElapsed = duration_cast<milliseconds>(currentTime - lastReceivedDataTime).count();
		if (msElapsed < DataTimeoutThresholdMs) return false;

		std::cout << "Have not received data for " << msElapsed << " ms" << std::endl;

		return msElapsed >= DataTimeoutDisconnectThresholdMs;
	}
}