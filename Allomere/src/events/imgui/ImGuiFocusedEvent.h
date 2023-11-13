#pragma once

#include "events/Event.h"

#include "editor/GUI/interfaces/ImGuiFocusable.h"

namespace Allomere {
	class ImGuiFocusedEvent : public Event
	{
	public:
		ImGuiFocusedEvent(std::weak_ptr<Focusable> focused)
			: m_Focused(focused) {}

		std::weak_ptr<Focusable> GetFocused() { return m_Focused; }

		EVENT_CLASS_TYPE(ImGuiFocused);
		EVENT_CLASS_CATEGORY(EventCategoryUI);
	private:
		std::weak_ptr<Focusable> m_Focused;
	};
}