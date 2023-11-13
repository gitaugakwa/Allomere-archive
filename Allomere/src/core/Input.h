#pragma once

#include "core/KeyCodes.h"
#include "core/MouseCodes.h"

#include <glm/glm.hpp>

namespace Allomere {

	class Input
	{
	public:
		static bool IsKeyPressed(KeyCode key);

		static bool IsMouseButtonPressed(MouseCode button);
		static glm::vec2 GetMousePosition();
		static float GetMouseX();
		static float GetMouseY();
	};
}