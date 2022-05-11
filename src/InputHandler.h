#pragma once
#include "Main.h"

namespace InputHandler
{
	constexpr auto MouseSensitivity = 10.0f;
	constexpr auto ScrollSensitivity = 15.0f;
	constexpr auto MousePowerFactor = 1.4f;
	constexpr auto ScrollPowerFactor = 1.5f;
	constexpr auto MouseDeadZone = 0.3f;
	constexpr auto ScrollDeadZone = 3.0f;

	constexpr auto ScrollTolerance = 25;
	constexpr auto DragTolerance = 20;

	constexpr auto DegreeRange = 500.0f;

	enum class MiddleMouseAction
	{
		None, // Middle mouse is not pressed
		Undetermined, // Middle mouse has been pressed but we cannot determine an action
		Scroll,
		Drag
	};

	void Initialize();
	void Scroll(int scrollAmount);
	void MouseMove();
	void MouseClick(int flags);
	void ProcessPacket(Packet packet);
}