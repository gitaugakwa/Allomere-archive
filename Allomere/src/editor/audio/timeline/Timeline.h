#pragma once
#include "editor/audio/track/Track.h"

#include "editor/audio/interfaces/Playable.h"

#include "utils/LockFreeQueue.h"

#include "events/Event.h"
#include "editor/audio/Device.h"

#include "miniaudio.h"

#include "editor/context/ContextManager.h"
#include "editor/context/TimelineContext.h"

namespace Allomere {
	namespace Audio {

		class Timeline : public EventEmmiter, public Attachable, public std::enable_shared_from_this<Timeline>
		{
			//friend class ImGuiTimeline;
			//friend class ImGuiTrack;
			friend class Track;
			//friend class ImGuiClip;
			friend class Clip;

			using Tracks = std::vector<std::shared_ptr<Track>>;
			using TimelineContext = Context::ContextManager<Context::TimelineContext>;

		public:
			Timeline(std::weak_ptr<TimelineContext> wContext);
			~Timeline();


			/*static void SetPlayState(bool newState) {
				Get().setPlayState(newState);
			}
			static bool GetPlayState() {
				return Get().playing;
			}*/

			virtual ma_result seek(size_t ms) override;
			virtual ma_result read(void* pFramesOut, ma_uint64 frameCount, ma_uint64* pFramesRead) override;

			virtual std::shared_ptr<ma_resource_manager_data_source> load(const char* pFilePath, ma_resource_manager_data_source_flags flags = MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_WAIT_INIT) const override;

			void setPlayState(bool newState);

			void togglePlayState();
			void addTrack();
			std::weak_ptr<Clip> addClipToTrack(const char* pFilePath, size_t trackIndex, size_t initialStartFrame = 0);

			//Device device() const { return *pDevice.lock(); }

			ma_uint32 sampleRate() const { return pDevice.lock()->sampleRate(); }
			ma_uint32 channels() const { return pDevice.lock()->channels(); }
			ma_format format() const { return pDevice.lock()->format(); }

			const Tracks& tracks() const { return mTracks; }

		private:
			ma_result read_and_mix_pcm_frames_f32(float* pOutputF32, ma_uint32 frameCount, ma_uint64* pFramesRead);

			void SetupSubscriptions();
			template<typename T>
			bool Drop(T event);

		private:
			//std::weak_ptr<Device> pDevice;

			Tracks mTracks;

			std::array<float, 1 << 12> readBuffer{}; // 4096

			std::weak_ptr<TimelineContext> pContext;

			//static std::atomic<float> time;
			//int numOfTracks = 5;

			//static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
			//LockFreeQueue trackEventQueue;
		};
	}
}