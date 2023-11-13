#pragma once
#include "Panel.h"
#include "editor/GUI/interfaces/ImGuiFocusable.h"

#include "events/Event.h"
#include "events/imgui/ImGuiFocusedEvent.h"

#include "editor/GUI/GUIOrchestrator.h"


namespace Allomere {
	namespace GUI {
		class FocusPanel : public Panel, public std::enable_shared_from_this<FocusPanel> {

		public:
			FocusPanel(std::weak_ptr<GUIOrchestrator> wOrchestrator);
			virtual void render() override;


		private:
			void SetupSubscriptions();
		private:
			std::weak_ptr<Focusable> focused;
			std::shared_ptr<ImGuiFocusedEvent> event = nullptr;
		};
	}
}