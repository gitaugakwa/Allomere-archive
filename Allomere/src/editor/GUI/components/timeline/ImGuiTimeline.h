#pragma once
#include "core/Timer.h"

#include "events/Event.h"

#include "editor/GUI/components/track/ImGuiTrack.h"
#include "editor/GUI/panels/TimelinePanel.h"

#include "editor/context/TimelineContext.h"
#include "editor/context/ContextManager.h"


namespace Allomere {
	namespace GUI {

		class TimelinePanel;
		class ImGuiTrack;

		class ImGuiTimeline : public EventEmmiter, public std::enable_shared_from_this<ImGuiTimeline>
		{
			using ImGuiTracks = std::vector<std::shared_ptr<ImGuiTrack>>;
		public:
			ImGuiTimeline(std::weak_ptr<Context::ContextManager<Context::TimelineContext>> wContext, std::weak_ptr<TimelinePanel> wPanel);

			void renderContext();
			void renderContent();

			float frameToPos(size_t frame) const;
			size_t posToFrame(float pos) const;
			long long deltaToFrame(float pos) const;

			//static void setFocus(std::weak_ptr<Focusable> clip);

		private:
			void SetupSubscriptions();

		private:
			friend class ImGuiClip;

			ImGuiTracks mImGuiTracks;
			std::weak_ptr<TimelinePanel> pPanel;
			std::weak_ptr<Context::ContextManager<Context::TimelineContext>> pContext;

			unsigned int topBarHeight = 60;
			unsigned int tickHeight = 25;
			unsigned int textPadding = 10;
			unsigned int topBarMargin = 3;

			Timer::Stopwatch spaceBarToggleStopwatch;

			bool cursorLock{ true };

		};
	}
}