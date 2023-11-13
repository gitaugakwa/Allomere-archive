#pragma once
#include <imgui.h>
#include <imgui_internal.h>

#include <filesystem>

#include "events/Event.h"

#include "editor/GUI/components/track/ImGuiTrack.h"

#include "editor/context/ClipContext.h"
#include "editor/context/ContextManager.h"

namespace Allomere {
	namespace GUI {
		class ImGuiTrack;

		class BaseImGuiClip {
		public:
			BaseImGuiClip() :mId{ sId++ } {};
			~BaseImGuiClip() {};


			inline size_t id() const { return mId; }

			// Special member functions
#pragma region
			BaseImGuiClip(const BaseImGuiClip&)
				: mId(sId++) {};
			BaseImGuiClip(BaseImGuiClip&& arg) noexcept
				: mId(std::exchange(arg.mId, 0)) {};
			BaseImGuiClip& operator=(const BaseImGuiClip&) {
				//mId = sId++;
				return *this;
			};
			BaseImGuiClip& operator=(BaseImGuiClip&& arg) noexcept {
				mId = std::move(arg.mId);
				return *this;
			};
#pragma endregion

		protected:

			inline static size_t sId = 1;
			size_t mId;
		};

		enum class ResizeableHandle {
			None = 0,
			Left = BIT(0),
			Right = BIT(1)
		};

		class ImGuiClip : public BaseImGuiClip, public EventEmmiter, public std::enable_shared_from_this<ImGuiClip>
		{
		public:
			class Focused;
			friend Focused;

			using ClipContext = Context::ContextManager<Context::ClipContext>;

		public:
			ImGuiClip(std::weak_ptr<ClipContext> context, std::weak_ptr<ImGuiTrack> track);
			~ImGuiClip();

			ImVec2 getCenterClickedPos() const { return centerClickedPos; }

			static void setClipTop(float val) {
				clipTop = val;
			}

			std::weak_ptr<ClipContext> context() { return pContext; }

			std::weak_ptr<Focused> getFocused() { return pFocused; }
			//Context::ClipContext getContext();

			void prepareFocused();

			//void render();
			//void renderContext();
			void renderContent(float startPos);
			//void renderFocused(Event& event);
			//void refresh();

		private:
			void SetupSubscriptions();

			void cleanPart();

			void preparePart(const Context::PartContext& part, float partDuration = FLT_MAX);
			void renderPart(int id);

		private:
			std::weak_ptr<ImGuiTrack> pTrack;
			std::weak_ptr<ClipContext> pContext;

			static size_t clipHeight;
			static size_t clipGap;
			static size_t textPadding;
			static float clipTop;


			bool focusInitialized = false;
			//ma_resource_manager_data_source dataSource;


			ResizeableHandle handle{ ResizeableHandle::None };
			bool isCenterClicked{ false };
			bool isCenterDragging{ false };
			bool isCenterHovered{ false };
			ImVec2 centerClickedPos;


			ImRect clipRect;
			ImRect clipDrawRect;
			ImRect clipButtonRect;

			ma_uint64 partLength;
			ImRect partRect;
			ImRect partDrawRect;
			ImRect partButtonRect;


			std::shared_ptr<Focused> pFocused = nullptr;

			bool isHovered{ false };
			bool isHeld{ false };

			std::string clipName;

		};
	}
}

