#pragma warning( disable : 4244 ) // Disable Warning C4244: conversion from * to *, possible loss of data


#include "allomere.h"
//#include "oneapi/dnnl/dnnl.h"
//#include "oneapi/dnnl/dnnl.hpp"
//#include "oneapi/dnnl/dnnl_debug.h"
//#include "oneapi/dnnl/dnnl_config.h"

//#include "ThreadPool.h"
//#include "Timer.h"
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <winbase.h>

#include <filesystem>

#include "imgui.h"
#include "implot.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}


class Sandbox : public Allomere::Application
{

public:
	Sandbox()
	{
        std::vector<float> data;
        std::span test(&data, 2);
        test;

        test = std::span(&data, 4);
	}

    void Renederer()
    {
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit())
            return;

        // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
        const char* glsl_version = "#version 100";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
        const char* glsl_version = "#version 150";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
        const char* glsl_version = "#version 130";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
        //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

        int numberOfMonitors;
        GLFWmonitor** monitors = glfwGetMonitors(&numberOfMonitors);
        int width = 1280;
        int height = 720;
        GLFWmonitor* monitor = NULL;
        if (numberOfMonitors) {
            monitor = monitors[0];
            const GLFWvidmode* desktopMode = glfwGetVideoMode(monitor);
            height = desktopMode->height * 0.90;
            width = desktopMode->width * 0.90;
        }

        // Create window with graphics context
        GLFWwindow* window = glfwCreateWindow(width, height, "Dear ImGui GLFW+OpenGL3 example", NULL, NULL);
        if (window == NULL)
            return;
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // Enable vsync

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
        //io.ConfigViewportsNoAutoMerge = true;
        //io.ConfigViewportsNoTaskBarIcon = true;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsLight();

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);


        // Load Fonts
        // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
        // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
        // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
        // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
        // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
        // - Read 'docs/FONTS.md' for more instructions and details.
        // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
        // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
        //io.Fonts->AddFontDefault();
        //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
        //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
        //IM_ASSERT(font != NULL);

        // Our state
        bool show_demo_window = false;
        bool show_another_window = false;
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        // Main loop
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
        io.IniFilename = NULL;
        EMSCRIPTEN_MAINLOOP_BEGIN
#else
        while (!glfwWindowShouldClose(window))
#endif
        {
            // Poll and handle events (inputs, window resize, etc.)
            // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
            // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
            // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
            // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
            glfwPollEvents();

            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            {
                // Note: Switch this to true to enable dockspace
                static bool dockspaceOpen = true;
                static bool opt_fullscreen_persistant = true;
                bool opt_fullscreen = opt_fullscreen_persistant;
                static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

                // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
                // because it would be confusing to have two docking targets within each others.
                ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
                ImGuiViewport* viewport = ImGui::GetMainViewport();
                if (opt_fullscreen)
                {
                    ImGui::SetNextWindowPos(viewport->Pos);
                    ImGui::SetNextWindowSize(viewport->Size);
                    ImGui::SetNextWindowViewport(viewport->ID);
                    //ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                    //ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
                    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
                }

                // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
                if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
                    window_flags |= ImGuiWindowFlags_NoBackground;

                ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);

                ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");

                ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
                static bool firstRender = true;

                //if (!ImGui::DockBuilderGetNode(dockspace_id)) {
                if (firstRender) {
                    firstRender = false;
                    ImGui::DockBuilderRemoveNode(dockspace_id);
                    ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
                    ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

                    ImGuiID dock_id_up;
                    ImGuiID dock_id_up_left;
                    ImGuiID dock_id_down_left;
                    auto dock_id_down = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.3f, nullptr, &dock_id_up);
                    auto dock_id_up_right = ImGui::DockBuilderSplitNode(dock_id_up, ImGuiDir_Right, 0.3f, nullptr, &dock_id_up_left);
                    //auto dock_id_down = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.7f, nullptr, &dockspace_id);
                    auto dock_id_down_right = ImGui::DockBuilderSplitNode(dock_id_down, ImGuiDir_Right, 0.5f, nullptr, &dock_id_down_left);
                    //auto dock_id_down_left = ImGui::DockBuilderSplitNode(dock_id_down, ImGuiDir_Left, 0.5f, nullptr, &dock_id_down);
                    //auto dock_id_down = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.25f, nullptr, &dockspace_id);
                    ImGui::DockBuilderDockWindow("Hello, world!", dock_id_up_right);
                    ImGui::DockBuilderDockWindow("Timeline", dock_id_up_left);
                    ImGui::DockBuilderDockWindow("DownRight", dock_id_down_right);
                    ImGui::DockBuilderDockWindow("FocusPanel", dock_id_down_left);
                    //ImGui::DockBuilderDockWindow("Down", dock_id_down);

                    ImGui::DockBuilderFinish(dockspace_id);
                }

                // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
                // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive, 
                // all active windows docked into it will lose their parent and become undocked.
                // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise 
                // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
                //ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                //ImGui::PopStyleVar();
                ImGui::End();
            }


            // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
            if (show_demo_window)
                ImGui::ShowDemoWindow(&show_demo_window);

            {
                pTimelinePanel->render();
            }
            {
                ImGui::Begin("DownRight");
                ImGui::End();
            }
            {
                pFocusPanel->render();
                //ImGui::Begin("DownLeft");
                //ImGui::End();
            }

            // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
            {
                static float f = 0.0f;
                static int counter = 0;

                ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

                ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
                ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
                ImGui::Checkbox("Another Window", &show_another_window);

                ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
                ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

                if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                    counter++;
                ImGui::SameLine();
                ImGui::Text("counter = %d", counter);

                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                ImGui::End();
            }

            // 3. Show another simple window.
            if (show_another_window)
            {
                ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
                ImGui::Text("Hello from another window!");
                if (ImGui::Button("Close Me"))
                    show_another_window = false;
                ImGui::End();
            }

            // Rendering
            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            // Update and Render additional Platform Windows
            // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
            //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                GLFWwindow* backup_current_context = glfwGetCurrentContext();
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
                glfwMakeContextCurrent(backup_current_context);
            }

            glfwSwapBuffers(window);
        }

#ifdef __EMSCRIPTEN__
        EMSCRIPTEN_MAINLOOP_END;
#endif

        // Cleanup
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);
        glfwTerminate();
    }

	

	virtual void Init() override {
        GUIthread = std::shared_ptr<std::thread>(new std::thread(&Sandbox::Renederer, this));

        Allomere::Timer::Stopwatch init;
        init.Start();

        pTC = std::make_shared<Allomere::Context::ContextManager<Allomere::Context::TimelineContext>>();
        Allomere::TimelineCreatedEvent createdEvent(pTC);

        pOrchestrator->Emit(createdEvent);

        size_t timelineId;
        std::shared_ptr<Allomere::Context::ContextManager<Allomere::Context::TrackContext>> trackContext;
        {
            auto context = pTC->write();
            timelineId = context->id();
            trackContext = context->tracks.emplace_back(new Allomere::Context::ContextManager<Allomere::Context::TrackContext>());
            Allomere::TrackCreatedEvent trackCreatedEvent(trackContext, timelineId);
            pOrchestrator->Emit(trackCreatedEvent);
        }
        //ALLOMERE_CORE_INFO("[CLIP] MUD");

        //pTimeline =std::shared_ptr<Allomere::Timeline>(new Allomere::Timeline);
        //pImGuiTimeline = std::shared_ptr<Allomere::ImGuiTimeline>(new Allomere::ImGuiTimeline(pTimeline));

        //pTimeline->setImgui(pImGuiTimeline);

        //auto device = Allomere::Audio::GetDevice();
        //device.lock()->attach(pTimeline);

        //pTimeline->addTrack();
        //pTimeline->addTrack();
        //pTimeline->addTrack();
        //pTimeline->addTrack();

        //std::weak_ptr<Allomere::Audio::Clip> mudClip = pTimeline->addClipToTrack("C:\\Torrents\\REDACTED\\Flume - Hi This Is Flume (Mixtape) (2019) [WEB FLAC]\\11 - MUD.flac", 0, 44100 * 0);

        //mudClip.lock()->cut(44100 * 2, 44100 * 3);
        //mudClip.lock()->bridge(44100 * 3 - 1000, 44100 * 2 + 1000, true);


        //auto mud = init.Lap();
        //ALLOMERE_CORE_INFO("[CLIP] Jewel");
        //std::weak_ptr<Allomere::Audio::Clip> jewelClip = pTimeline->addClipToTrack("C:\\Torrents\\REDACTED\\Flume - Hi This Is Flume (Mixtape) (2019) [WEB FLAC]\\04 - Jewel.flac", 1, 44100* 2);
        //jewelClip.lock()->Subscribe<Allomere::ClipSimilarityGeneratedEvent>(
        //    [=](Allomere::Event& e) {
        //        e;
        //       /* auto clip = jewelClip.lock();
        //        const auto* sim = clip->similaritySorted();

        //        for (int i = 0; i < 20; i++) {
        //            clip->bridgeBeat(sim[i].query, sim[i].reference);
        //        }
        //        ALLOMERE_CORE_INFO("Bridged Beat");*/
        //        return true;
        //    });
        //auto all = init.Stop();
        //ALLOMERE_CORE_INFO("[ADD] Track 1: {0}", mud);
        //ALLOMERE_CORE_INFO("[ADD] All Clips: {0}", all);
        //Allomere::Timeline::Get().addClipToTrack("C:\\Users\\DOUG\\Downloads\\brooklyn.flac", 2);

	}

	virtual void Run() override {

        while (m_Running)
        {
            auto start = conductorQueue.pop_all();
            start;
        }

        return;
	}

	virtual void Deinit() override {

		
	}

	~Sandbox()
	{

	}
    std::shared_ptr<std::thread> GUIthread;

    std::shared_ptr<Allomere::Orchestrator> pOrchestrator = std::make_shared<Allomere::Orchestrator>();
    
    std::shared_ptr<Allomere::Context::ContextManager<Allomere::Context::TimelineContext>> pTC;
    std::shared_ptr<Allomere::GUI::GUIOrchestrator> pGuiOrchestrator = std::make_shared<Allomere::GUI::GUIOrchestrator>(pOrchestrator);
    std::shared_ptr<Allomere::GUI::TimelinePanel> pTimelinePanel = std::make_shared<Allomere::GUI::TimelinePanel>(pGuiOrchestrator);
    std::shared_ptr<Allomere::GUI::FocusPanel> pFocusPanel = std::make_shared<Allomere::GUI::FocusPanel>(pGuiOrchestrator);

    Allomere::LockFreeQueue<int> conductorQueue;

};

Allomere::Application* Allomere::CreateApplication()
{
	return new Sandbox;
}
