#pragma once

#include "events/Event.h"

namespace Allomere {

	class Focusable {

	public:
		virtual void renderFocused(Event& event) = 0;
		virtual void refresh(Event& event) = 0;
		std::string title(Event& event) { event; return "Focus"; }
	};
} 