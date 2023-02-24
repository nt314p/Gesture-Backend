#pragma once
#include "Main.h"

namespace Input
{
	static constexpr auto MouseSensitivity = 10.0f;
	static constexpr auto ScrollSensitivity = 15.0f;
	static constexpr auto MousePowerFactor = 1.4f;
	static constexpr auto ScrollPowerFactor = 1.5f;
	static constexpr auto MouseDeadZone = 0.3f;
	static constexpr auto ScrollDeadZone = 3.0f;

	static constexpr auto ScrollTolerance = 25;
	static constexpr auto DragTolerance = 20;

	static constexpr auto DegreeRange = 500.0f;

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