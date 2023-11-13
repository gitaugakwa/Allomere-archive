#pragma once

#include "events/Event.h"

namespace Allomere {
	namespace GUI {
		class Panel: public EventEmmiter {
		public:

			Panel() = default;
			Panel(std::weak_ptr<EventEmmiter> wParent) {}
			virtual void render() = 0;
		};
	}
}