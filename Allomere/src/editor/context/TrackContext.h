#pragma once
#include <vector>
#include <memory>

#include "ClipContext.h"

namespace Allomere {
	namespace Context {

		class BaseTrackContext
		{
		public:
			BaseTrackContext() :mId{ sId++ } {};
			~BaseTrackContext() {};

			inline size_t id() const { return mId; }

			// Special member functions
#pragma region
			BaseTrackContext(const BaseTrackContext&)
				: mId(sId++) {};
			BaseTrackContext(BaseTrackContext&& arg) noexcept
				: mId(std::exchange(arg.mId, 0)) {};
			BaseTrackContext& operator=(const BaseTrackContext&) {
				//mId = sId++;
				return *this;
			};
			BaseTrackContext& operator=(BaseTrackContext&& arg) noexcept {
				mId = std::move(arg.mId);
				return *this;
			};
#pragma endregion

		protected:

			inline static size_t sId = 0;
			size_t mId;
		};

		struct TrackClipContext: public ClipContext
		{
			size_t startFrame;
		};

		struct TrackContext : public BaseTrackContext
		{
			size_t id;

			std::vector<std::shared_ptr<TrackClipContext>> clips;
			bool muted;

			// GUI
			size_t height{ 150 };
		};
	}
}