#include "allomerepch.h"
#include "Application.h"

#include "core/Base.h"

#include "Events/Event.h"
#include "Events/ApplicationEvent.h"

namespace Allomere {

//#define BIND_EVENT_FN(x) std::bind(&Application::x, this, std::placeholders::_1)

	Application::~Application()
	{
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		//dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClosed));
		//for (auto it = m_LayerStack.end(); it != m_LayerStack.begin();)
		//{
		//	(*--it)->OnEvent(e);
		//	if (e.Handled)
		//		break;
		//}
	}
}
