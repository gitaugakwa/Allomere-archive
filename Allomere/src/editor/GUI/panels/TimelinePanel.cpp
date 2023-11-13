#include "allomerepch.h"
#include "TimelinePanel.h"

#include "events/imgui/ImGuiClipEvent.h"
#include "events/imgui/ImGuiTrackEvent.h"
#include "events/imgui/ImGuiTimelineEvent.h"
#include "events/timeline/TimelineEvent.h"

#include <imgui.h>

namespace Allomere {
	namespace GUI {
		TimelinePanel::TimelinePanel( std::weak_ptr<GUIOrchestrator> wOrchestrator)
			: Panel(wOrchestrator)
		{
			SetupSubscriptions();
		}

		void TimelinePanel::SetupSubscriptions()
		{
			Subscribe<TimelineCreatedEvent>(
				[this](Event& e) {
					auto& event = dynamic_cast<TimelineCreatedEvent&>(e);
					ALLOMERE_CORE_INFO("TimelineCreated Event");
					pImGuiTimeline = std::make_shared<ImGuiTimeline>(event.GetContext(), weak_from_this());
					return false;
				}
			);
		}

		void TimelinePanel::render()
		{
			ImGui::Begin("Timeline");
			if (pImGuiTimeline)
			{
				auto& imGuiTimeline = *pImGuiTimeline;

				imGuiTimeline.renderContent();
				imGuiTimeline.renderContext();
			}
			ImGui::End();
		}

	}
}