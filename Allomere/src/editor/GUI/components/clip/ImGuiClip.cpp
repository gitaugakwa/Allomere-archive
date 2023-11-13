#include "allomerepch.h"

#include <imgui.h>
#include <imgui_internal.h>

#include "miniaudio.h"

#include "events/imgui/ImGuiClipEvent.h"
#include "events/imgui/ImGuiTrackEvent.h"

#include "ImGuiClip.h"
#include "ImGuiClipFocused.h"

namespace Allomere {
	namespace GUI {

		size_t ImGuiClip::clipHeight = { 150 };
		size_t ImGuiClip::clipGap = { 5 };
		size_t ImGuiClip::textPadding = { 5 };
		float ImGuiClip::clipTop = { 0 };

		ImGuiClip::ImGuiClip(std::weak_ptr<ImGuiClip::ClipContext> wContext,std::weak_ptr<ImGuiTrack> wTrack) : pContext(wContext), pTrack(wTrack)
		{
			//SetParent(wTrack);
			SetupSubscriptions();
			//clipName = std::filesystem::path(clip.lock()->filePath).stem().string();

			//auto& timeline = Timeline::Get();
			//ma_resource_manager_data_source_init(&timeline.maResourceManager, clip.lock()->filePath.c_str(), MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_WAIT_INIT, NULL, &dataSource);
		}

		void ImGuiClip::SetupSubscriptions()
		{}

		ImGuiClip::~ImGuiClip()
		{
			//ma_resource_manager_data_source_uninit(&dataSource);
		}

		void ImGuiClip::renderContent(float startPos) {
			/*if (pClip.expired()) {
				return;
			}*/
			//ImGuiWindow* window = ImGui::GetCurrentWindow();
			auto& track = *pTrack.lock();
			auto& imGuiTimeline = track.timeline();

			size_t id;
			size_t length;
			std::pair<ma_uint64, ma_uint64> clipPoints;
			std::vector<Context::PartContext> parts;
			//size_t partLength;
			{
				const auto context = pContext.lock()->read();
				id = context->id;
				length = context->length();
				clipPoints = context->clipPoints();
				partLength = context->partLength(0);
				parts = context->parts;
			}


			auto& io = ImGui::GetIO();
			io;

			const ImVec2 windowPos = ImGui::GetWindowPos();

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
			const ImVec2 trackPos = canvasPos + ImVec2(0, 0);

			//const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
			const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
			const float scrollX = ImGui::GetScrollX();

			//ma_uint64 durationInFrames;

			const float trackStartPos = trackPos.x + scrollX;
			const float trackEndPos = trackStartPos + canvasSize.x;

			std::string clipId("Clip##" + std::to_string(id));
			ImGui::PushID(clipId.c_str());

			if (isCenterClicked) {
				if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
					ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);

					isCenterDragging = true;

					ImGuiClipDraggedEvent event(id);
					Emit(event);
				}
				if (isCenterDragging) {
					if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
						isCenterClicked = false;
						isCenterDragging = false;
						ImGuiClipReleasedEvent event(id);
						Emit(event);
					}
				}
				else {
					if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
						isCenterClicked = false;
					}
				}
			}

			//if (handle != ResizeableHandle::None) {
			//	if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
			//		auto delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x;
			//		switch (handle) {
			//		case(ResizeableHandle::Left): {
			//			clipStartXPos = std::clamp(clipStartXPos + delta, clipStartXPos - ImGuiTimeline::frameToPos(cutPoints.first), clipEndXPos);
			//			auto clipStartXFrame = ImGuiTimeline::posToFrame(clipStartXPos - trackPos.x - startPos) + cutPoints.first;
			//			clip.cut(clipStartXFrame, cutPoints.second);
			//			ImGui::ResetMouseDragDelta();
			//			ImGuiClipResizedEvent event(clip.mId, delta, 0);
			//			Emit(event);
			//			break;
			//		}
			//		case(ResizeableHandle::Right): {
			//			clipEndXPos = std::clamp(clipEndXPos + delta, clipStartXPos, clipStartXPos + ImGuiTimeline::frameToPos(originalLength - cutPoints.first));
			//			auto clipEndXFrame = ImGuiTimeline::posToFrame(clipEndXPos - trackPos.x - startPos) + cutPoints.first;
			//			clip.cut(cutPoints.first, clipEndXFrame);
			//			ImGui::ResetMouseDragDelta();
			//			ImGuiClipResizedEvent event(clip.mId, 0, delta);
			//			Emit(event);
			//			break;
			//		}
			//		}
			//	}
			//	if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			//		handle = ResizeableHandle::None;
			//	}
			//}

			float clipStartXPos = startPos + trackPos.x;

			float clipEndXPos = length == SIZE_MAX ? FLT_MAX : clipStartXPos + imGuiTimeline.frameToPos(length);
			const ImVec2 height(0, float(clipHeight));

			clipRect = ImRect(
				ImVec2(clipStartXPos, trackPos.y),
				ImVec2(clipEndXPos, trackPos.y + clipHeight)
			);
			clipDrawRect = ImRect(
				ImVec2(std::max(clipStartXPos, trackStartPos - 5), trackPos.y),
				ImVec2(std::min(clipEndXPos, trackEndPos + 10), trackPos.y + clipHeight)
			);
			clipButtonRect = ImRect(
				ImVec2(std::max(clipStartXPos, trackStartPos), clipDrawRect.Min.y),
				ImVec2(std::min(clipEndXPos, trackEndPos), clipDrawRect.Max.y)
			);

			partRect = ImRect(
				clipRect.Min,
				clipRect.Min + ImVec2(0, clipRect.Max.y)
			);
			partDrawRect = ImRect(
				clipDrawRect.Min,
				ImVec2(std::min(partRect.Max.x, clipDrawRect.Max.x), clipDrawRect.Max.y)
			);
			partButtonRect = ImRect(
				clipButtonRect.Min,
				ImVec2(std::min(partRect.Max.x, clipButtonRect.Max.x), clipButtonRect.Max.y)
			);

			//const float clipMinXPos = std::max<float>(0, startPos - ImGuiTimeline::frameToPos(rangePoints.first)) + trackPos.x;
			//const float clipMaxXPos = length == SIZE_MAX ? FLT_MAX : clipMinXPos + ImGuiTimeline::frameToPos(length) - ImGuiTimeline::frameToPos(rangePoints.first);
			drawList->PushClipRect(
				clipDrawRect.Min,
				// Clip is over Clipping
				clipDrawRect.Max + ImVec2(1, 0),
				true
			);

			for (int source = 0; source < parts.size(); source++)
			{
				auto& part = parts[source];

				if (part.looping)
				{
					partLength = part.endFrame - part.startFrame;
					float partDuration = imGuiTimeline.frameToPos(partLength);
					auto skip = std::floor((clipDrawRect.Min.x - clipRect.Min.x - partDuration) / partDuration);
					preparePart(part, partDuration * skip);
					cleanPart();
					/*while (clipDrawRect.Min.x - partDuration > partDrawRect.Max.x)
					{

					}*/
					while (clipDrawRect.Max.x > partDrawRect.Min.x)
					{
						preparePart(part, partDuration);
						renderPart(source);
						cleanPart();

						source++;
					}
					break;
				}

				preparePart(part);

				if (partDrawRect.Max.x < clipDrawRect.Min.x)
				{
					continue;
				}
				if (clipDrawRect.Max.x < partDrawRect.Min.x)
				{
					break;
				}

				renderPart(source);
				cleanPart();

			}

			{
				auto x = std::max(windowPos.x + ImGuiTrack::trackContextWidth + 8, clipDrawRect.Min.x);
				drawList->AddText(
					ImVec2(x, clipDrawRect.Min.y) + ImVec2((float)textPadding, (float)textPadding),
					//ImVec2(trackPos.x + textPadding, trackPos.y + textPadding),
					IM_COL32(241, 245, 249, 255),
					clipName.c_str()
				);
			}

			drawList->PopClipRect();

			ImGui::PopID();

			return;
		}

		void ImGuiClip::preparePart(const Context::PartContext& part, float partDuration)
		{
			auto& track = *pTrack.lock();
			auto& imGuiTimeline = track.timeline();

			if (partDuration == FLT_MAX) {
				partLength = part.endFrame - part.endFrame;
				partDuration = imGuiTimeline.frameToPos(partLength);
			}

			partRect.Max += ImVec2(partDuration, 0);
			partDrawRect.Max += ImVec2(partDuration, 0);
			partButtonRect.Max += ImVec2(partDuration, 0);
		}

		void ImGuiClip::renderPart(int id)
		{
			ImGui::PushID(id);

			size_t clipId;
			{
				const auto context = pContext.lock()->read();
				clipId = context->id;
			}
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			auto& io = ImGui::GetIO();

			auto partLeftXMin = std::max(partButtonRect.Min.x, partRect.Min.x);
			auto partLeftXMax = partRect.Min.x + 5;
			auto partRightXMin = partRect.Max.x - 5;
			auto partRightXMax = std::min(partButtonRect.Max.x, partRect.Max.x);
			auto& partCenterXMin = partLeftXMax;
			auto& partCenterXMax = partRightXMin;

			auto renderLeft = partButtonRect.Min.x < partLeftXMax;
			auto renderRight = partRightXMin < partButtonRect.Max.x;
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			const auto buttonStart = std::min({ partLeftXMin, partCenterXMin, partRightXMin });
			ImGui::SetCursorScreenPos(ImVec2(std::max(buttonStart, partButtonRect.Min.x), partButtonRect.Min.y));
			// partButtonRect.Min.x < partLeftXMax&& partLeftXMin < partButtonRect.Max.x
			if (renderLeft)
			{
				ImRect partLeftButtonRect(
					ImVec2(partLeftXMin, partButtonRect.Min.y),
					ImVec2(partLeftXMax, partButtonRect.Max.y)
				);
				const auto size = partLeftButtonRect.GetSize();
				if (size.x > 0) {
					ImGui::InvisibleButton("Left", size);
					ImGui::SameLine();
					if (ImGui::IsItemHovered()) {
						ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
					}
					if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
						//ALLOMERE_CORE_INFO("Left Clicked Pos: {0}", centerClickedPos.x);
						handle = ResizeableHandle::Left;
					}
				}
			}
			if (renderLeft || renderRight)
			{
				ImRect partCenterButtonRect(
					ImVec2(std::max(partCenterXMin, partButtonRect.Min.x), partButtonRect.Min.y),
					ImVec2(std::min(partCenterXMax, partButtonRect.Max.x), partButtonRect.Max.y)
				);
				const auto size = partCenterButtonRect.GetSize();
				if (size.x > 0) {
					ImGui::InvisibleButton("Center", size);
					ImGui::SameLine();
					if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
						centerClickedPos = io.MouseClickedPos[0] - clipRect.Min;
						//ALLOMERE_CORE_INFO("Center Clicked Pos: {0}", centerClickedPos.x);
						ImGuiClipClickedEvent event(clipId, weak_from_this());
						Emit(event);
						isCenterClicked = true;
					}
				}
			}
			if (renderRight)
			{
				ImRect partRightButtonRect(
					ImVec2(partRightXMin, partButtonRect.Min.y),
					ImVec2(partRightXMax, partButtonRect.Max.y)
				);
				const auto size = partRightButtonRect.GetSize();
				if (size.x > 0) {
					ImGui::InvisibleButton("Right", size);
					if (ImGui::IsItemHovered()) {
						ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
					}
					if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
						//ALLOMERE_CORE_INFO("Right Clicked Pos: {0}", centerClickedPos.x);
						handle = ResizeableHandle::Right;
					}
				}
			}
			ImGui::PopStyleVar();

			drawList->AddRectFilled(
				partDrawRect.Min,
				partDrawRect.Max,
				IM_COL32(245, 158, 11, 255),
				5
			);

			ImGui::PopID();
		}

		void ImGuiClip::cleanPart()
		{
			const ImVec2 height(0, float(clipHeight));

			partRect.Min = partRect.Max - height;
			partDrawRect.Min = partDrawRect.Max - height;
			partButtonRect.Min = ImVec2(std::max(clipButtonRect.Min.x, partButtonRect.Max.x), partButtonRect.Max.y - clipHeight);
		}

		void ImGuiClip::prepareFocused()
		{
			if (pFocused == nullptr) {
				pFocused.reset(new Focused(pContext, weak_from_this()));
			}
		}
	}

}