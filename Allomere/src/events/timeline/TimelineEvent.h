#pragma once

#include "events/Event.h"

#include "editor/context/TimelineContext.h"
#include "editor/context/ContextManager.h"

namespace Allomere {

	using TimelineContext = Context::ContextManager<Context::TimelineContext>;

	class TimelineCreatedEvent : public Event
	{
	public:
		TimelineCreatedEvent(std::weak_ptr<TimelineContext> wContext)
			: m_Context(wContext) {}

		std::weak_ptr<TimelineContext> GetContext() const { return m_Context; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "TimelineCreatedEvent";
			return ss.str();
		}

		EVENT_CLASS_TYPE(TimelineCreated)
		EVENT_CLASS_CATEGORY(EventCategoryTimeline)
	private:
		std::weak_ptr<TimelineContext> m_Context;
	};

	class TimelineSeekEvent : public Event
	{
	public:
		TimelineSeekEvent()
			 {}


		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "TimelineSeekEvent";
			return ss.str();
		}

		EVENT_CLASS_TYPE(TimelineSeek)
		EVENT_CLASS_CATEGORY(EventCategoryTimeline)
	private:

	};
}