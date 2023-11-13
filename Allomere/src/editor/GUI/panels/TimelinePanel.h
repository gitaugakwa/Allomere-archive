#pragma once
#include "Panel.h"

#include "events/Event.h"

#include "editor/GUI/components/timeline/ImGuiTimeline.h"

#include "editor/GUI/GUIOrchestrator.h"

namespace Allomere {
	namespace GUI {
		class ImGuiTimeline;

		class TimelinePanel : public Panel, public std::enable_shared_from_this<TimelinePanel> {
		public:
			TimelinePanel(std::weak_ptr<GUIOrchestrator> wOrchestrator);
			virtual void render() override;

		private:
			void SetupSubscriptions();

		private:
			std::shared_ptr<ImGuiTimeline> pImGuiTimeline;
			//std::weak_ptr<GUIOrchestrator> pOrchestrator;
		};
	}
}