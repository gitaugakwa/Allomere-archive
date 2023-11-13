#pragma once

#include "events/Event.h"

#include "editor/audio/track/Track.h"

#include "editor/context/TrackContext.h"
#include "editor/context/ContextManager.h"

namespace Allomere {

	using TrackContext = Context::ContextManager<Context::TrackContext>;

	class TrackCreatedEvent : public Event
	{
	public:
		TrackCreatedEvent(std::weak_ptr<TrackContext> wContext, size_t timeline)
			: m_Context(wContext), m_Timeline(timeline) {}

		std::weak_ptr<TrackContext> GetContext() const { return m_Context; }
		size_t GetTimeline() const { return m_Timeline; }
		//size_t GetStartFrame() const { return m_StartFrame; }
		//std::weak_ptr<Audio::Track::Clip> GetClip() { return m_Clip; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "TrackCreatedEvent: ClipID=" << GetContext().lock()->read()->id;
			return ss.str();
		}

		EVENT_CLASS_TYPE(TrackCreated)
		EVENT_CLASS_CATEGORY(EventCategoryTrack)
	private:
		std::weak_ptr<TrackContext> m_Context;
		size_t m_Timeline;
	};
}