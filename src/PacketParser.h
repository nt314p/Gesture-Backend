#pragma once
#include "Main.h"
#include "CircularBuffer.h"

namespace PacketParser
{
	static constexpr auto Signature = 0b10101000;
	static constexpr auto PacketAlignmentAttemptsThreshold = 1000;
	static constexpr auto MaxPacketBacklog = 3U;
	static constexpr auto SequentialValidPacketsToAlign = 5;
	static constexpr auto DataTimeoutThresholdMs = 200;
	static constexpr auto DataTimeoutDisconnectThresholdMs = 3000;

	extern std::function<void(Packet)> PacketReady;

	void SetBuffer(CircularBuffer* circularBuffer);
	bool DataTimeoutExceeded();
	void ResetLastReceivedDataTime();
	void OnReceivedData();
	bool TryAlignData();
	void ResetDataAlignment();
}