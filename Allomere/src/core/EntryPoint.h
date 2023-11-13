#pragma once
#include "core/Base.h"
#include "core/Application.h"

#ifdef MATTHREADS
#else
#define MATTHREADS 0
#endif // MATTHREADS

#ifdef ALLOMERE_PLATFORM_WINDOWS

//extern Allomere::Application* Allomere::CreateApplication(ApplicationCommandLineArgs args);

int main(int argc, char** argv)
{
	argc;
	argv;
	Allomere::Log::Init();

	Eigen::initParallel();
	ALLOMERE_CORE_INFO("Initialized Eigen Parallel!");
	Eigen::setNbThreads(MATTHREADS);
	ALLOMERE_CORE_INFO("Eigen Threads: {0}", Eigen::nbThreads());

	ALLOMERE_PROFILE_BEGIN_SESSION("Startup", "AllomereProfile-Startup.json");
	//ALLOMERE_CORE_INFO("Initializing Essentia");
	//ALLOMERE_CORE_INFO("Essentia Initialized");

	//auto app = Allomere::CreateApplication({ argc, argv });
	auto app = Allomere::CreateApplication();
	ALLOMERE_PROFILE_END_SESSION();

	ALLOMERE_PROFILE_BEGIN_SESSION("Runtime", "AllomereProfile-Runtime.json");
	app->Init();
	app->Run();
	ALLOMERE_PROFILE_END_SESSION();

	ALLOMERE_PROFILE_BEGIN_SESSION("Shutdown", "AllomereProfile-Shutdown.json");
	delete app;
	ALLOMERE_PROFILE_END_SESSION();
}

#endif
