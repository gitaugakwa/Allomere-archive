#pragma once

// #define EIGEN_USE_MKL_ALL

#include "allomerepch.h"

#include "core/Base.h"

#include "core/Application.h"
#include "core/Layer.h"
#include "core/Log.h"
#include "core/Assert.h"

#include "core/Timestep.h"
#include "core/Timer.h"

#include "editor/audio/Audio.h"
//#include "editor/audio/Device.h"

#include "editor/audio/timeline/Timeline.h"
#include "editor/GUI/components/timeline/ImGuiTimeline.h"

#include "editor/GUI/panels/TimelinePanel.h"
#include "editor/GUI/panels/FocusPanel.h"

#include "editor/GUI/GUIOrchestrator.h"

#include "editor/orchestrator/Orchestrator.h"

// #include "Input/Input.h"
// #include "Output/Output.h"

// // Events
// #include "Events/ApplicationEvent.h"
// #include "Events/Event.h"
// #include "Events/KeyEvent.h"
// #include "Events/LayerEvent.h"
// #include "Events/LinkEvent.h"
// #include "Events/MouseEvent.h"
// #include "Events/NetworkEvent.h"
// #include "Events/NodeEvent.h"
#include "events/timeline/ClipEvent.h"
#include "events/timeline/TrackEvent.h"
#include "events/timeline/TimelineEvent.h"

#include "core/Input.h"
#include "core/KeyCodes.h"
#include "core/MouseCodes.h"

#include "imgui/ImGuiLayer.h"

// Base Allomere
// #include "Link/Link.h"
// #include "Node/Node.h"
// #include "Layer/Layer.h"
// #include "Network/Network.h"

// // Async Neural
// #include "Layer/LayerAsync.h"
// #include "Network/NetworkAsync.h"

// #include "Model/Model.h"

// #include "Test/Test.h"
// #include "Train/Train.h"
// #include "Train/TrainAsync.h"

// #include "dnnUtils.h"

//	---Entry Point-----------------------
#include "core/EntryPoint.h"
//	-------------------------------------
