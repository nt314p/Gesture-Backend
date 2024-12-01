#pragma once
#include "Main.h"
#include "CircularBuffer.h"

namespace PacketParser
{
	static constexpr auto Signature = 0b10101000;
	static constexpr auto SignatureMask = 0b11111000;
	static constexpr auto PacketAlignmentAttemptsThreshold = 1000;
	static constexpr auto MaxPacketBacklog = 3;
	static constexpr auto SequentialValidPacketsToAlign = 5;

	extern std::function<void(Packet)> PacketReady;

	void SetBuffer(CircularBuffer* circularBuffer);
	void OnReceivedData();
	bool TryAlignData();
	void ResetDataAlignment();
}