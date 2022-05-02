#include "pch.h"
#include <chrono>
#include "BluetoothHelper.h"
#include "Main.h"

constexpr auto Signature = 0b10101000;
constexpr auto BufferLength = 128;
constexpr auto PacketAlignmentAttemptsThreshold = 10000;
constexpr auto SequentialValidPacketsToAlign = 5;
constexpr auto DegreeRange = 500.0f;

constexpr auto MouseSensitivity = 10.0f;
constexpr auto ScrollSensitivity = 15.0f;
constexpr auto MousePowerFactor = 1.4f;
constexpr auto ScrollPowerFactor = 1.5f;
constexpr auto MouseDeadZone = 0.3f;
constexpr auto ScrollDeadZone = 3.0f;

constexpr auto ScrollTolerance = 25;
constexpr auto DragTolerance = 20;

INPUT input;
POINT cursor;
Packet currentPacket;

int screenWidth;
int screenHeight;

float mouseX = 0;
float mouseY = 0;
uint8_t previousButtonData;

MiddleMouseAction middleMouseAction = MiddleMouseAction::Undetermined;

uint8_t buffer[BufferLength] = {};
int readIndex = 0;
int writeIndex = 0;

uint8_t byteValidCount[sizeof(Packet)] = {}; // byteValidCount[i] stores how many times the ith byte had a valid signature
int byteIndex = 0;
bool isDataAligned = false;
int attemptedPacketAlignments = 0;

auto previousTime = std::chrono::system_clock::now();

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

Vector3 ToVector3(Vector3Short v, float range)
{
	return {
		v.X * range / INT16_MAX,
		v.Y * range / INT16_MAX,
		v.Z * range / INT16_MAX,
	};
}

bool TryProcessPacket()
{
	static uint8_t byteBuffer[sizeof(Packet)];

	for (int i = 0; i < sizeof(Packet); i++)
	{
		byteBuffer[i] = ReadBuffer();
	}

	Packet p = *(Packet*)byteBuffer;
	currentPacket.Gyro = p.Gyro;
	currentPacket.ScrollData = p.ScrollData;
	currentPacket.ButtonData = p.ButtonData;

	if ((p.ButtonData & 0b11111000) == Signature) return true;

	isDataAligned = false;
	attemptedPacketAlignments = 0;
	std::cout << "Data misaligned!" << std::endl;

	return false;
}

void OnDataAligned()
{
	std::cout << "Data aligned!" << std::endl;
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
				OnDataAligned();
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

void SendMouseInput()
{
	UINT numEvents = SendInput(1, &input, sizeof(input));
	if (numEvents == 0)
	{
		std::cout << "SendMouse failed: " << HRESULT_FROM_WIN32(GetLastError()) << std::endl;
	}
}

void MouseScroll(int scrollAmount)
{
	input.mi.dx = 0;
	input.mi.dy = 0;
	input.mi.dwFlags = MOUSEEVENTF_WHEEL;
	input.mi.mouseData = scrollAmount;
	SendMouseInput();
}

// Moves the mouse to the position (mouseX, mouseY)
void MouseMove()
{
	input.mi.dx = (int)round(mouseX * 0xffff / (float)screenWidth);
	input.mi.dy = (int)round(mouseY * 0xffff / (float)screenHeight);
	input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
	input.mi.mouseData = 0;
	SendMouseInput();
}

void MouseClick(DWORD flags)
{
	input.mi.dx = 0;
	input.mi.dy = 0;
	input.mi.dwFlags = flags;
	input.mi.mouseData = 0;
	SendMouseInput();
}

// Handles mouse input (click, move, and scroll)
void ProcessInput(Vector3 gyro)
{
	uint8_t currentButtonData = currentPacket.ButtonData;
	uint8_t buttonChanges = currentButtonData ^ previousButtonData;

	if (buttonChanges & 1 << 0)
		MouseClick(currentButtonData & 1 << 0 ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP);

	if (buttonChanges & 1 << 1)
	{
		bool isMiddlePressed = currentButtonData & 1 << 1;
		MouseClick(isMiddlePressed ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP);
		middleMouseAction = isMiddlePressed ? MiddleMouseAction::Undetermined : MiddleMouseAction::None;
	}

	if (buttonChanges & 1 << 2)
		MouseClick(currentButtonData & 1 << 1 ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP);

	previousButtonData = currentButtonData;

	auto dx = gyro.Z;
	auto dy = gyro.X;
	auto scroll = gyro.Y;
	
	if (abs(dx) < MouseDeadZone) dx = 0;
	if (abs(dy) < MouseDeadZone) dy = 0;
	if (abs(scroll) < ScrollDeadZone) scroll = 0;

	auto cookedDx = (float)(-dx * pow(abs(dx), MousePowerFactor - 1) / MouseSensitivity);
	auto cookedDy = (float)(dy * pow(abs(dy), MousePowerFactor - 1) / MouseSensitivity);
	auto cookedScroll = (float)(scroll * pow(abs(scroll), ScrollPowerFactor - 1) / ScrollSensitivity);

	if (middleMouseAction == MiddleMouseAction::Undetermined)
	{
		if (abs(scroll) > ScrollTolerance)
		{
			middleMouseAction = MiddleMouseAction::Scroll;
		}

		if (abs(dx) > DragTolerance || abs(dy) > DragTolerance)
		{
			middleMouseAction = MiddleMouseAction::Drag;
		}
	}

	if (middleMouseAction == MiddleMouseAction::Scroll)
	{
		MouseScroll((int)roundf(cookedScroll));
	}

	if (cookedDx == 0 && cookedDy == 0)
	{
		GetCursorPos(&cursor);
		mouseX = (float)cursor.x;
		mouseY = (float)cursor.y;
		return;
	}

	mouseX += cookedDx;
	mouseY += cookedDy;
	mouseX = std::clamp(mouseX, 0.0f, (float)screenWidth);
	mouseY = std::clamp(mouseY, 0.0f, (float)screenHeight);

	MouseMove();
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

	if (BufferCount() < sizeof(Packet)) return;

	if (!TryProcessPacket()) return;

	Vector3 gyro = ToVector3(currentPacket.Gyro, DegreeRange);

	ProcessInput(gyro);

	auto t = std::chrono::system_clock::now();
	auto us = std::chrono::duration_cast<std::chrono::microseconds>(t - previousTime).count();

	auto elapsed = std::chrono::system_clock::now().time_since_epoch();
	auto elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();

	std::cout << elapsedUs << "\t" << us << std::endl;
	previousTime = t;
}

int main(Platform::Array<Platform::String^>^ args)
{
	std::cout << "Starting GestureBackend..." << std::endl;

	HRESULT hresult = RoInitialize(RO_INIT_MULTITHREADED);

	if (hresult != S_OK)
	{
		if (hresult == S_FALSE) RoUninitialize();
		return 0;
	}

	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);

	GetCursorPos(&cursor);
	mouseX = (float)cursor.x;
	mouseY = (float)cursor.y;

	HDC primary = GetDC(NULL);
	screenWidth = GetDeviceCaps(primary, HORZRES) - 1;
	screenHeight = GetDeviceCaps(primary, VERTRES) - 1;
	ReleaseDC(NULL, primary);

	MOUSEINPUT mouseInput;
	mouseInput.dx = 0;
	mouseInput.dy = 0;
	mouseInput.time = 0;
	mouseInput.mouseData = 0;
	mouseInput.dwExtraInfo = NULL;

	input.type = INPUT_MOUSE;
	input.mi = mouseInput;

	StartWatcher();

	std::cin.get();

	while (true)
	{
		GetCursorPos(&cursor);
		std::cout << cursor.x << ", " << cursor.y << std::endl;
		Sleep(10);
	}
}