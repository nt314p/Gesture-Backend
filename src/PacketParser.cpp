#include "pch.h"
#include "PacketParser.h"
#include "CircularBuffer.h"
#include <bitset>
#include <chrono>

namespace PacketParser
{
	static Packet currentPacket;
	static CircularBuffer* buffer;

	static bool isDataAligned = false;
	static int attemptedPacketAlignments = 0;

	std::function<void(Packet)> PacketReady;

	void SetBuffer(CircularBuffer* circularBuffer)
	{
		PacketParser::buffer = circularBuffer;
	}

	inline static bool HasValidSignature(uint8_t byte)
	{
		return (byte & SignatureMask) == Signature;
	}

	static bool TryProcessPacket()
	{
		uint8_t* byteBuffer = (uint8_t*)&currentPacket;

		for (int i = 0; i < sizeof(Packet); i++) // read data into packet struct
		{
			byteBuffer[i] = buffer->ReadBuffer();
		}

		return HasValidSignature(currentPacket.ButtonData);
	}

	// Attempts to align data currently in the buffer
	bool TryAlignData()
	{
		// byteValidCount[i] stores how many times the ith byte had a valid signature
		static uint8_t byteValidCount[sizeof(Packet)] = {};
		static int byteIndex = 0;

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
		auto packetBacklog = buffer->BufferCount() / (int)sizeof(Packet);

		if (packetBacklog <= MaxPacketBacklog) return;
		
		for (unsigned int i = 0; i < (packetBacklog - MaxPacketBacklog) * sizeof(Packet); i++)
		{
			buffer->ReadBuffer();
		}
	}

	void OnReceivedData()
	{
		if (!isDataAligned)
		{
			if (attemptedPacketAlignments > PacketAlignmentAttemptsThreshold)
			{
				std::cout << "Tried to align " << PacketAlignmentAttemptsThreshold << " packets with no success" << std::endl;
				return;
			}

			if (TryAlignData())
			{
				std::cout << "Data aligned!" << std::endl;
			}
			return;
		}

		CorrectPacketBacklog(); // TODO: is backlog correction necessary now that we consume all available packets?

		while (buffer->BufferCount() >= sizeof(Packet))
		{
			if (!TryProcessPacket())
			{
				isDataAligned = false;
				attemptedPacketAlignments = 0;

				std::cout << "Data misaligned! Attempting to realign..." << std::endl;
				return;
			}

			//std::cout << currentPacket.Gyro.X << "\t" << currentPacket.Gyro.Y << "\t" << currentPacket.Gyro.Z << std::endl;

			if (PacketReady) PacketReady(currentPacket);
		}
	}

	// Marks data as not aligned
	void ResetDataAlignment()
	{
		isDataAligned = false;
	}
}