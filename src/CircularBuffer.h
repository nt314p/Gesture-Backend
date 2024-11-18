#pragma once

static constexpr auto BufferLength = 256;

class CircularBuffer
{
private:
	uint8_t buffer[BufferLength] = {};
	int readIndex = 0;
	int writeIndex = 0;
public:
	void WriteBuffer(uint8_t byte);
	uint8_t ReadBuffer();
	int BufferCount() const;
};