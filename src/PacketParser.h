#pragma once

namespace PacketParser
{
	static constexpr auto Signature = 0b10101000;
	static constexpr auto BufferLength = 128;
	static constexpr auto PacketAlignmentAttemptsThreshold = 1000;
	static constexpr auto MaxPacketBacklog = 3U;
	static constexpr auto SequentialValidPacketsToAlign = 5;
	static constexpr auto DataTimeoutThresholdMs = 200;
	static constexpr auto DataTimeoutDisconnectThresholdMs = 3000;

	bool DataTimeoutExceeded();
	void ResetLastReceivedDataTime();
	void ProcessCharacteristicValue(Windows::Storage::Streams::IBuffer^ characteristicValue);
	bool TryAlignData();
	void ResetDataAlignment();
}