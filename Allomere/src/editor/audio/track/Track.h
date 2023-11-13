#pragma once
#include "editor/audio/clip/Clip.h"

#include "editor/audio/timeline/Timeline.h"

#include "Events/Event.h"

#include <vector>

#include "miniaudio.h"

#include "editor/context/TrackContext.h"
#include "editor/context/ContextManager.h"

namespace Allomere {
	namespace Audio {
		class Timeline;

		class BaseTrack {
		public:
			BaseTrack() :mId{ sId++ } {};
			~BaseTrack() {};


			inline size_t id() const { return mId; }

			// Special member functions
#pragma region
			BaseTrack(const BaseTrack&)
				: mId(sId++) {};
			BaseTrack(BaseTrack&& arg) noexcept
				: mId(std::exchange(arg.mId, 0)) {};
			BaseTrack& operator=(const BaseTrack&) {
				//mId = sId++;
				return *this;
			};
			BaseTrack& operator=(BaseTrack&& arg) noexcept {
				mId = std::move(arg.mId);
				return *this;
			};
#pragma endregion

		protected:

			inline static size_t sId = 1;
			size_t mId;
		};

		class Track : public BaseTrack, public EventEmmiter, public Playable, public std::enable_shared_from_this<Track>
		{
		public:
			friend class ImGuiTrack;
			friend class ImGuiClip;

			using TrackContext = Context::ContextManager<Context::TrackContext>;
		public:
			class Clip : public Allomere::Audio::Clip {
			public:
				friend class Track;
				friend class ImGuiTrack;

				using ClipContext = Context::ContextManager<Context::ClipContext>;
			public:

				Clip(std::weak_ptr<ClipContext> pContext, std::weak_ptr<Track> track, size_t initialStartFrame = 0) : Allomere::Audio::Clip(pContext, track), startFrame(initialStartFrame) {}

				const size_t* getStartP() const { return &startFrame; }
				size_t getStart() const { return startFrame; }
				size_t setStart(size_t newStart) { return startFrame = newStart; }

				bool operator < (const Clip& clip) const
				{
					return (startFrame < clip.startFrame);
				}
			private:
				size_t startFrame{ 0 };

			};

			using Clips = std::vector<std::shared_ptr<Clip>>;

		public:
			Track(std::weak_ptr<TrackContext> pContext, std::weak_ptr<Timeline> timeline);

			//std::weak_ptr<Clip> addClip(const char* pFilePath, size_t initialStartFrame = 0);

			size_t length();

			virtual ma_result seek(size_t pos) override;
			virtual ma_result read(void* pFramesOut, ma_uint64 frameCount, ma_uint64* pFramesRead) override;

			Clips getClips() const { return mClips; }

			bool refreshState();
			bool refreshState(size_t frame);
			bool refreshState(Clip& clip);

			const Timeline& timeline() const { return *pTimeline.lock(); }


		private:
			static bool sortByStartFrame(const std::shared_ptr<Clip>& clip1, const std::shared_ptr<Clip>& clip2);

			void SetupSubscriptions();

			TrackContext& getContext() { return *pContext.lock(); }

		private:

			static ma_audio_buffer_config maSilenceBufferConfig;
			static ma_audio_buffer maSilenceBuffer;
			static float* pSilenceData;


			std::weak_ptr<Timeline> pTimeline;
			std::weak_ptr<TrackContext> pContext;

			Clips mClips{};

			ma_uint64 currentFrameCount{ 0 };

			bool inClip = { false };
			size_t clipIndex = { 0 };

		};
	}
}