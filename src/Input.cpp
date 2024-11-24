#include "pch.h"
#include "Input.h"

namespace Input
{
	static INPUT input;
	static POINT cursor;

	static int screenWidth;
	static int screenHeight;

	static float mouseX = 0;
	static float mouseY = 0;
	static uint8_t previousButtonData;

	static MiddleMouseAction middleMouseAction = MiddleMouseAction::Undetermined;

	static Vector3 ToVector3(Vector3Int16 v, float range)
	{
		return {
			v.X * range / INT16_MAX,
			v.Y * range / INT16_MAX,
			v.Z * range / INT16_MAX,
		};
	}

	void Initialize()
	{
		GetCursorPos(&cursor);
		mouseX = (float)cursor.x;
		mouseY = (float)cursor.y;

		HDC primary = GetDC(NULL);
		screenWidth = GetDeviceCaps(primary, HORZRES) - 1;
		screenHeight = GetDeviceCaps(primary, VERTRES) - 1;
		ReleaseDC(NULL, primary);

		MOUSEINPUT mouseInput{};
		mouseInput.dx = 0;
		mouseInput.dy = 0;
		mouseInput.time = 0;
		mouseInput.mouseData = 0;
		mouseInput.dwExtraInfo = NULL;

		input.type = INPUT_MOUSE;
		input.mi = mouseInput;
	}

	static void SendMouseInput()
	{
		UINT numEvents = SendInput(1, &input, sizeof(input));
		if (numEvents == 0)
		{
			std::cout << "SendMouse failed: " << HRESULT_FROM_WIN32(GetLastError()) << std::endl;
		}
	}

	void Scroll(int scrollAmount)
	{
		input.mi.dx = 0;
		input.mi.dy = 0;
		input.mi.dwFlags = MOUSEEVENTF_WHEEL;
		input.mi.mouseData = (DWORD)scrollAmount;
		SendMouseInput();
	}

	// Moves the mouse to the position (mouseX, mouseY)
	void MouseMove()
	{
		input.mi.dx = (int)round(mouseX * 0xffff / (float)screenWidth);
		input.mi.dy = (int)round(mouseY * 0xffff / (float)screenHeight);
		input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE_NOCOALESCE;
		input.mi.mouseData = 0;
		SendMouseInput();
	}

	void MouseClick(int flags)
	{
		input.mi.dx = 0;
		input.mi.dy = 0;
		input.mi.dwFlags = (DWORD)flags;
		input.mi.mouseData = 0;
		SendMouseInput();
	}

	// Handles mouse input (click, move, and scroll)
	void ProcessPacket(Packet packet)
	{
		auto gyro = ToVector3(packet.Gyro, DegreeRange);

		auto currentButtonData = packet.ButtonData;
		uint8_t buttonChanges = (uint8_t)(currentButtonData ^ previousButtonData);

		int rightChanged = buttonChanges & RightMask;
		int rightDown = currentButtonData & RightMask;
		int middleChanged = buttonChanges & MiddleMask;
		int middleDown = currentButtonData & MiddleMask;
		int leftChanged = buttonChanges & LeftMask;
		int leftDown = currentButtonData & LeftMask;

		if (rightChanged)
		{
			MouseClick(rightDown ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP);
		}


		if (leftChanged)
		{
			MouseClick(leftDown ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP);
		}

		if (middleChanged)
		{
			MouseClick(middleDown ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP);
			middleMouseAction = middleDown ? MiddleMouseAction::Undetermined : MiddleMouseAction::None;
		}

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

			if (dx * dx + dy * dy > DragTolerance)
			{
				middleMouseAction = MiddleMouseAction::Drag;
			}
		}

		if (middleMouseAction == MiddleMouseAction::Scroll)
		{
			Scroll((int)roundf(cookedScroll));
		}

		bool noMovement = cookedDx == 0 && cookedDy == 0;

		// allow free mouse movement when no input is given or when scrolling
		if (noMovement || middleMouseAction == MiddleMouseAction::Scroll)
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
}