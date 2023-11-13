#pragma once
#include <vector>
#include <memory>
#include <shared_mutex>

#include "TrackContext.h"
#include "ContextManager.h"

namespace Allomere {
	namespace Context {

		class BaseTimelineContext
		{
		public:
			BaseTimelineContext() :mId{ sId++ } {};
			~BaseTimelineContext() {};

			inline size_t id() const { return mId; }

			// Special member functions
#pragma region
			BaseTimelineContext(const BaseTimelineContext&)
				: mId(sId++) {};
			BaseTimelineContext(BaseTimelineContext&& arg) noexcept
				: mId(std::exchange(arg.mId, 0)) {};
			BaseTimelineContext& operator=(const BaseTimelineContext&) {
				//mId = sId++;
				return *this;
			};
			BaseTimelineContext& operator=(BaseTimelineContext&& arg) noexcept {
				mId = std::move(arg.mId);
				return *this;
			};
#pragma endregion

		protected:

			inline static size_t sId = 0;
			size_t mId;
		};

		struct TimelineContext: public BaseTimelineContext
		{
			std::vector<std::shared_ptr<ContextManager<TrackContext>>> tracks;

			size_t time{ 0 };
			bool playing{ false };
			size_t sampleRate{ 44100 };
			size_t channels{ 2 };

			// GUI
			float scale{ 10 };
		};

	}
}