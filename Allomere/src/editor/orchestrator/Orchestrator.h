#pragma once

#include "events/Event.h"

namespace Allomere {

	class Orchestrator : public EventEmmiter {
	public:
		Orchestrator() = default;
		Orchestrator(std::weak_ptr<Orchestrator> wOrchestrator);

		void AddChild(Orchestrator* pOrchestrator)
		{
			mOrchestrators.emplace_back(pOrchestrator);
		}


		template<typename T>
		bool Emit(T event)
		{
			return EventEmmiter::Emit<T>(event);
		}

	protected:

	private:
		void SetupSubscriptions();

	private:
		std::vector<std::shared_ptr<Orchestrator>> mOrchestrators;
	};
}