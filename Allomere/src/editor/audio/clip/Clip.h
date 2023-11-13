#pragma once
#include "miniaudio.h"

#include "editor/audio/interfaces/Playable.h"

#include "Events/Event.h"
#include "core/Timer.h"

#include "tnt/tnt.h"

#include "editor/context/ClipContext.h"
#include "editor/context/ContextManager.h"


namespace Allomere {
	namespace Audio {
		class Track;

		class BaseClip {
		public:
			BaseClip() :mId{ sId++ } {};
			~BaseClip() {};


			inline size_t id() const { return mId; }

			// Special member functions
#pragma region
			BaseClip(const BaseClip&)
				: mId(sId++) {};
			BaseClip(BaseClip&& arg) noexcept
				: mId(std::exchange(arg.mId, 0)) {};
			BaseClip& operator=(const BaseClip&) {
				//mId = sId++;
				return *this;
			};
			BaseClip& operator=(BaseClip&& arg) noexcept {
				mId = std::move(arg.mId);
				return *this;
			};
#pragma endregion

		protected:

			inline static size_t sId = 1;
			size_t mId;
		};


		class Clip : public BaseClip, public EventEmmiter, public Playable
		{
		public:
			using ClipContext = Context::ContextManager<Context::ClipContext>;

		public:
			Clip(std::weak_ptr<ClipContext> pFilePath, std::weak_ptr<Track> track);
			virtual ~Clip();


			virtual ma_result read(void* pFramesOut, ma_uint64 frameCount, ma_uint64* pFramesRead) override;
			virtual ma_result seek(size_t frame) override;

			ma_result getFrames(void* pFramesOut, ma_uint64 startFrame, ma_uint64 endFrame, ma_uint64* framesRead);

			ma_result cut(ma_uint64 startFrame, ma_uint64 endFrame);
			ma_result bridge(ma_uint64 fromFrame, ma_uint64 toFrame, bool loop = false);	// infinite looping or not
			// Will have to find a way to keep track of the loop count
			// Might be metadata per datasource
			// Cause if we are going to use loopCount number of dataSources, we'll need to know which dataSources are part of the loop
			// And also if we're going to have bridges in a loop, then we'll have to find a way to manage that
			ma_result bridge(ma_uint64 fromFrame, ma_uint64 toFrame, size_t loopCount);		// no of loops
			ma_result bridgeBeat(ma_uint64 fromBeat, ma_uint64 toBeat, bool loop = false);

			std::pair<ma_uint64, ma_uint64> getCut() const;
			std::pair<ma_uint64, ma_uint64> getRange();

			//std::getParts();

			const std::vector<size_t>& beats() const;
			//const Similarity* similarity() const { return similarityMatrix; }
			//const Similarity* similaritySorted() const { return similarityMatrixSorted; }


			size_t originalLength();
			size_t length();

			//const Track& track() const { return *pTrack.lock(); }

		private:
			void initialize();
			void SetupSubscriptions();

			ClipContext& getContext() { return *pContext.lock(); }

		private:
			std::weak_ptr<Track> pTrack;

			static bool initialized;

			std::weak_ptr<ClipContext> pContext;

			//Similarity* similarityMatrix = nullptr;
			//Similarity* similarityMatrixSorted = nullptr;

			std::shared_ptr<ma_resource_manager_data_source> rawDataSource;
			std::vector<std::shared_ptr<ma_resource_manager_data_source>> mDataSources;


			float mDistance;
			//float distance;

			mutable std::atomic<bool> mBeatsInitialized{ false };
			mutable std::future<void> mBeatsFuture;

			mutable std::future<void> mSimilarityFuture;
		};
	}
}

