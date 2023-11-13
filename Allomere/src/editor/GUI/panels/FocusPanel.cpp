#include "allomerepch.h"
#include "FocusPanel.h"

#include "TimelinePanel.h"

#include "events/imgui/ImGuiClipEvent.h"
#include "events/imgui/ImGuiTrackEvent.h"
#include "events/imgui/ImGuiTimelineEvent.h"

#include <imgui.h>


namespace Allomere {
	namespace GUI {
		FocusPanel::FocusPanel(std::weak_ptr<GUIOrchestrator> wOrchestrator)
			: Panel(wOrchestrator)
		{
			SetupSubscriptions();
		}

		void FocusPanel::SetupSubscriptions()
		{
			Subscribe<ImGuiClipClickedEvent>(
				[&](Event& e) {
					auto& clickedEvent = dynamic_cast<ImGuiClipClickedEvent&>(e);
					focused = clickedEvent.GetClip().lock()->getFocused();

					size_t startFrame;
					{
						auto& context = std::dynamic_pointer_cast<Context::ContextManager<Context::TrackClipContext>>(clickedEvent.GetClip().lock()->context().lock())->read();
						startFrame = context->startFrame;
					}
					event.reset(new ImGuiClipFocusedEvent(clickedEvent.GetID(), focused, startFrame));
					focused.lock()->refresh(*event);
					return true;
				}
			);

			Subscribe<ImGuiTimelineSeekEvent>(
				[&](Event& e) {
					if (!focused.expired()) {
						auto& seekEvent = dynamic_cast<ImGuiTimelineSeekEvent&>(e);
						seekEvent;
						focused.lock()->refresh(*event);

					}
					//delete event;
					//event = new ImGuiTimelineSeekEvent(seekEvent);
					//focused = focusedEvent.GetFocused();
					return true;
				}
			);
		}


		void FocusPanel::render()
		{
			ImGui::Begin("FocusPanel");
			if (!focused.expired())
			{
				auto pFocused = focused.lock();
				pFocused->renderFocused(*event);
			}
			ImGui::End();
		}
	}
}