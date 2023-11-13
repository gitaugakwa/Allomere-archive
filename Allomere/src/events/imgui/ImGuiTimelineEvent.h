#pragma once

#include "events/Event.h"

namespace Allomere {

	class ImGuiTimelineSeekEvent : public Event
	{
	public:
		ImGuiTimelineSeekEvent(size_t frame)
			: m_Frame(frame) {}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "ImGuiTimelineSeekEvent";
			return ss.str();
		}

		EVENT_CLASS_TYPE(ImGuiTimelineSeek)
		EVENT_CLASS_CATEGORY(EventCategoryTimeline | EventCategoryUI)
	private:
		size_t m_Frame;
	};

	class ImGuiTimelineSetPlayStateEvent : public Event
	{
	public:
		ImGuiTimelineSetPlayStateEvent(bool newState)
			: m_State(newState) {}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "ImGuiTimelineSetPlayStateEvent";
			return ss.str();
		}

		EVENT_CLASS_TYPE(ImGuiTimelineSetPlayState)
		EVENT_CLASS_CATEGORY(EventCategoryTimeline | EventCategoryUI)
	private:
		bool m_State;
	};
}