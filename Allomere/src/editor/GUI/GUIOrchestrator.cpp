#include "allomerepch.h"

#include "GUIOrchestrator.h"

namespace Allomere {
	namespace GUI {

		GUIOrchestrator::GUIOrchestrator(std::weak_ptr<Orchestrator> wOrchestrator) : Orchestrator(wOrchestrator)
		{
			auto parent = wOrchestrator.lock();
			parent->AddChild(this);
		}

		void GUIOrchestrator::SetupSubscriptions()
		{}


	}
}