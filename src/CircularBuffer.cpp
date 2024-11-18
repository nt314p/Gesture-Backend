#include <cstdint>
#include "CircularBuffer.h"

void CircularBuffer::WriteBuffer(uint8_t byte)
{
	buffer[writeIndex] = byte;
	writeIndex++;
	if (writeIndex >= BufferLength)	writeIndex = 0;
}

uint8_t CircularBuffer::ReadBuffer()
{
	auto byte = buffer[readIndex];
	readIndex++;
	if (readIndex >= BufferLength) readIndex = 0;
	return byte;
}

int CircularBuffer::BufferCount() const
{
	int difference = writeIndex - readIndex;
	if (difference < 0) difference += BufferLength;
	return difference;
}