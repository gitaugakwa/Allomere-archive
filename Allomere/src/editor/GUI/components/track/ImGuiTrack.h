#pragma once
#include "ImGuiTrack.h"

#include "editor/GUI/components/timeline/ImGuiTimeline.h"
#include "editor/GUI/components/clip/ImGuiClip.h"

#include "editor/context/TrackContext.h"
#include "editor/context/ContextManager.h"


namespace Allomere {
	namespace GUI {
		class ImGuiTimeline;

		class BaseImGuiTrack {
		public:
			BaseImGuiTrack() :mId{ sId++ } {};
			~BaseImGuiTrack() {};


			inline size_t id() const { return mId; }

			// Special member functions
#pragma region
			BaseImGuiTrack(const BaseImGuiTrack&)
				: mId(sId++) {};
			BaseImGuiTrack(BaseImGuiTrack&& arg) noexcept
				: mId(std::exchange(arg.mId, 0)) {};
			BaseImGuiTrack& operator=(const BaseImGuiTrack&) {
				//mId = sId++;
				return *this;
			};
			BaseImGuiTrack& operator=(BaseImGuiTrack&& arg) noexcept {
				mId = std::move(arg.mId);
				return *this;
			};
#pragma endregion

		protected:

			inline static size_t sId = 1;
			size_t mId;
		};

		class ImGuiTrack : public BaseImGuiTrack, public EventEmmiter, public std::enable_shared_from_this<ImGuiTrack>
		{
		public:
			using TrackContext = Context::ContextManager<Context::TrackContext>;
		public:
			ImGuiTrack(std::weak_ptr<TrackContext> wContext, std::weak_ptr<ImGuiTimeline> timeline);
			//ImGuiTrack(std::weak_ptr<Track> track);

			static void setTrackTop(float val) {
				trackTop = val;
			}

			void renderContent();
			void renderContext();

			ImGuiTimeline& timeline() { return *pTimeline.lock(); }

		private:
			void SetupSubscriptions();
		private:
			friend class ImGuiTimeline;
			friend class ImGuiClip;

			using ImGuiClips = std::vector<std::shared_ptr<ImGuiClip>>;

			std::weak_ptr<ImGuiTimeline> pTimeline;
			std::weak_ptr<TrackContext> pContext;

			ImGuiClips mImGuiClips;

			static size_t trackGap;
			static size_t textPadding;
			static size_t trackContextWidth;
			static float trackTop;
		};
	}
}