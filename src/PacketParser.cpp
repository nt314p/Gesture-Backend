#include "pch.h"
#include "PacketParser.h"
#include "Input.h"
#include "CircularBuffer.h"
#include <chrono>

namespace PacketParser
{
	static Packet currentPacket;

	// byteValidCount[i] stores how many times the ith byte had a valid signature
	static uint8_t byteValidCount[sizeof(Packet)] = {}; 
	static int byteIndex = 0;
	static bool isDataAligned = false;
	static int attemptedPacketAlignments = 0;

	std::function<void(Packet)> PacketReady;

	static auto previousTime = std::chrono::system_clock::now();
	static auto lastReceivedDataTime = std::chrono::system_clock::now();

	static CircularBuffer* buffer;

	static void SetBuffer(CircularBuffer* circularBuffer)
	{
		PacketParser::buffer = circularBuffer;
	}

	inline static bool HasValidSignature(uint8_t byte)
	{
		return (byte & 0b11111000) == Signature;
	}

	static bool TryProcessPacket()
	{
		uint8_t* byteBuffer = (uint8_t*)&currentPacket;

		for (int i = 0; i < sizeof(Packet); i++) // read data into packet struct
		{
			byteBuffer[i] = buffer->ReadBuffer();
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

		while (buffer->BufferCount() > 0)
		{
			if (HasValidSignature(buffer->ReadBuffer()))
			{
				byteValidCount[byteIndex]++;
			}
			else
			{
				byteValidCount[byteIndex] = 0;
			}

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

	static void CorrectPacketBacklog()
	{
		auto packetBacklog = buffer->BufferCount() / sizeof(Packet);

		if (packetBacklog <= MaxPacketBacklog) return;
		
		for (unsigned int i = 0; i < (packetBacklog - MaxPacketBacklog) * sizeof(Packet); i++)
		{
			buffer->ReadBuffer();
		}

		std::cout << packetBacklog << std::endl;
	}

	void OnReceivedData()
	{
		using namespace std::chrono;
		lastReceivedDataTime = system_clock::now();

		if (!isDataAligned)
		{
			TryAlignData();
			return;
		}

		CorrectPacketBacklog();

		while (buffer->BufferCount() >= sizeof(Packet))
		{
			if (!TryProcessPacket()) return;

			PacketReady(currentPacket);
		}

		auto t = system_clock::now();
		auto us = duration_cast<microseconds>(t - previousTime).count();

		auto elapsed = system_clock::now().time_since_epoch();
		auto elapsedUs = duration_cast<microseconds>(elapsed).count();

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