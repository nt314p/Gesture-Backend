#include "pch.h"
#include "PacketParser.h"
#include "Bluetooth.h"
#include "Input.h"
#include <chrono>

namespace PacketParser
{
	static Packet currentPacket;

	// byteValidCount[i] stores how many times the ith byte had a valid signature
	static uint8_t byteValidCount[sizeof(Packet)] = {}; 
	static int byteIndex = 0;
	static bool isDataAligned = false;
	static int attemptedPacketAlignments = 0;

	static auto previousTime = std::chrono::system_clock::now();
	static auto lastReceivedDataTime = std::chrono::system_clock::now();

	inline bool HasValidSignature(uint8_t byte)
	{
		return (byte & 0b11111000) == Signature;
	}

	bool TryProcessPacket()
	{
		uint8_t* byteBuffer = (uint8_t*)&currentPacket;

		for (int i = 0; i < sizeof(Packet); i++) // read data into packet struct
		{
			byteBuffer[i] = BluetoothLE::ReadBuffer();
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

		while (BluetoothLE::BufferCount() > 0)
		{
			if (HasValidSignature(BluetoothLE::ReadBuffer()))
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

	void CorrectPacketBacklog()
	{
		auto packetBacklog = BluetoothLE::BufferCount() / sizeof(Packet);

		if (packetBacklog <= MaxPacketBacklog) return;
		
		for (unsigned int i = 0; i < (packetBacklog - MaxPacketBacklog) * sizeof(Packet); i++)
		{
			BluetoothLE::ReadBuffer();
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

		while (BluetoothLE::BufferCount() >= sizeof(Packet))
		{
			if (!TryProcessPacket()) return;
			//std::cout << packetBacklog << "\t" <<
			//	currentPacket.Gyro.X << "\t" << currentPacket.Gyro.Y << "\t" << currentPacket.Gyro.Z << std::endl;
			Input::ProcessPacket(currentPacket);
		}

		auto t = system_clock::now();
		auto us = duration_cast<microseconds>(t - previousTime).count();

		auto elapsed = system_clock::now().time_since_epoch();
		auto elapsedUs = duration_cast<microseconds>(elapsed).count();

		/*
		int ms = (int)(us / 1000);

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