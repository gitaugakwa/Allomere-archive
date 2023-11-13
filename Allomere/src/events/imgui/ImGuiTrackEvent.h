#pragma once

#include "events/Event.h"
#include "editor/GUI/components/clip/ImGuiClipFocused.h"
#include "editor/GUI/components/track/ImGuiTrack.h"

#include "ImGuiFocusedEvent.h"

namespace Allomere {

	class ImGuiTrackRefreshEvent : public Event
	{
	public:

		ImGuiTrackRefreshEvent(std::weak_ptr<GUI::ImGuiTrack> track)
			: m_Track(track) {}

		size_t GetID() const { return m_Track.lock()->id(); }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "ImGuiTrackRefreshEvent: TrackID=" << GetID();
			return ss.str();
		}

		EVENT_CLASS_TYPE(ImGuiTrackRefresh)
		EVENT_CLASS_CATEGORY(EventCategoryTrack)
	private:
		std::weak_ptr<GUI::ImGuiTrack> m_Track;

	};

	class ImGuiTrackSetMuteStateEvent : public Event
	{
	public:

		ImGuiTrackSetMuteStateEvent(bool newState)
			: m_State(newState) {}

		//size_t GetID() const { return m_Track.lock()->id(); }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "ImGuiTrackSetMuteStateEvent: TrackID=";
			return ss.str();
		}

		EVENT_CLASS_TYPE(ImGuiTrackRefresh)
		EVENT_CLASS_CATEGORY(EventCategoryTrack)
	private:
		bool m_State;

	};

}