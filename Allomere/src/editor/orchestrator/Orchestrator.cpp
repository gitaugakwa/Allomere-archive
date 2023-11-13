#include "allomerepch.h"

#include "Orchestrator.h"

namespace Allomere {

	Orchestrator::Orchestrator(std::weak_ptr<Orchestrator> wOrchestrator)
	{
		SetupSubscriptions();
	}

	void Orchestrator::SetupSubscriptions()
	{

	}

}