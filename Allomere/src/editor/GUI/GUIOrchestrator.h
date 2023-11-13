#pragma once

#include "editor/orchestrator/Orchestrator.h"

#include "editor/GUI/panels/Panel.h"

namespace Allomere {
	namespace GUI {

		class GUIOrchestrator : public Orchestrator {
		public:

			GUIOrchestrator() = default;
			GUIOrchestrator(std::weak_ptr<Orchestrator> wOrchestrator);

			void AddPanel(std::shared_ptr<Panel> sPanel);

		private:
			void SetupSubscriptions();

		private:
			using ImGuiPanels = std::vector<std::shared_ptr<Panel>>;

			ImGuiPanels mImGuiPanels;
		};
	}
}