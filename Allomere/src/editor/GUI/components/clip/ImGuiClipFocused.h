#pragma once
#include "events/Event.h"

#include "editor/GUI/interfaces/ImGuiFocusable.h"
#include "ImGuiClip.h"

namespace Allomere {
	namespace GUI {
		class ImGuiClip::Focused : public Focusable, public EventEmmiter
		{
		public:
			Focused(std::weak_ptr<Context::ContextManager<Context::ClipContext>> context, std::weak_ptr<ImGuiClip> clip);
			~Focused();

			void renderFocused(Event& event);
			void refresh(Event& event);
			void refreshLive(Event& event);
		private:
			void SetupSubscriptions();
			ma_result getFrames(void* pFramesOut, ma_uint64 startFrame, ma_uint64 endFrame, ma_uint64* framesRead);

			void initialize();

		private:
			std::weak_ptr<ImGuiClip> pClip;
			std::weak_ptr<Context::ContextManager<Context::ClipContext>> pContext;

			static bool initialized;

			static ma_resource_manager maResourceManager;
			static ma_resource_manager_config maResourceManagerConfig;

			float scale = 1;
			size_t audioFrames = (size_t)(44100 * scale);
			std::vector<float> audioSamples{};

			size_t focusedStartFrame{ (size_t)(-1) };
			size_t focusedEndFrame{ (size_t)(-1) };

			std::vector<size_t>::const_iterator focusedStartBeat;
			std::vector<size_t>::const_iterator focusedEndBeat;

			ma_resource_manager_data_source dataSource;

			bool beatsInitialized{ false };
		};
	}
}