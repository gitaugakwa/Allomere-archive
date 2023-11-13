#pragma once
#include "core/Base.h"

#include "Events/Event.h"
//#include "Events/ApplicationEvent.h"

//#include "Soren/LayerStack.h"


namespace Allomere {

	class Application
	{
	public:
		virtual ~Application();

		virtual void Init() = 0;
		virtual void Run() = 0;
		virtual void Deinit() = 0;

		virtual void OnEvent(Event& e);

		//void PushLayer(Layer* layer);
		//void PushOverlay(Layer* layer);
		
	protected:
		bool m_Running = true;

		// not sure how layers will be implemented but we'll see.
		// incase we decide on it being one application, then we can have different layers that then are able to pass data
		//LayerStack m_LayerStack;
	private:

	};

	//To be defined in client
	Application* CreateApplication();

}
