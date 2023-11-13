#include "allomerepch.h"

#include <imgui.h>
#include <imgui_internal.h> 
#include <string>

#include "Events/Event.h"

#include "events/timeline/TrackEvent.h"
#include "events/timeline/ClipEvent.h"

#include "events/imgui/ImGuiClipEvent.h"
#include "events/imgui/ImGuiTrackEvent.h"

#include "ImGuiTrack.h"


namespace Allomere {
    namespace GUI {

        size_t ImGuiTrack::trackGap = { 5 };
        size_t ImGuiTrack::textPadding = { 10 };
        size_t ImGuiTrack::trackContextWidth = { 200 };
        float ImGuiTrack::trackTop = { 0 };

        ImGuiTrack::ImGuiTrack(std::weak_ptr<Context::ContextManager<Context::TrackContext>> wContext, std::weak_ptr<ImGuiTimeline> wTimeline) : pContext(wContext), pTimeline(wTimeline) {
            //SetParent(wTimeline);
            SetupSubscriptions();
        }

        void ImGuiTrack::SetupSubscriptions()
        {
            Subscribe<ClipAddedEvent>([this](Event& e) {
                auto& addedEvent = dynamic_cast<ClipAddedEvent&>(e);
                auto context = addedEvent.GetContext().lock();
                if (addedEvent.GetTrack() == mId)
                {
                    // ImGuiClip
                    auto imGuiClip = mImGuiClips.emplace_back(new ImGuiClip(context, weak_from_this()));
                    // Create ImGui Clip
                    ALLOMERE_CORE_INFO("Clip Added to Track[{0}]", mId);
                    return true;
                }
                return false;
                });
            /*Subscribe<ImGuiClipClickedEvent>(
                [&, clip, wImguiClip](Event& event) {
                    event;
                    auto& trackClip = *clip.lock();
                    wImguiClip.lock()->prepareFocused();
                    ImGuiTrackClipFocusedEvent focusedEvent(wImguiClip.lock()->getFocused(), trackClip.getStartP());
                    Emit(focusedEvent);
                    return true;
                }
            );*/
            Subscribe<ImGuiClipDraggedEvent>(
                [this](Event& e) {
                    e;
                    /*auto& imGuiTimeline = pTrack.lock()->timeline().imGui();
                    auto& io = ImGui::GetIO();
                    const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
                    const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
                    const float scrollX = ImGui::GetScrollX();
                    const float trackStartPos = canvasPos.x + scrollX;
                    const float trackEndPos = trackStartPos + canvasSize.x;

                    auto& imguiClip = *wImguiClip.lock();

                    auto delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x;
                    auto& trackClip = *clip.lock();
                    const auto currStart = trackClip.getStart();
                    auto start = std::max<long long>(0, (long long)currStart + (long long)imGuiTimeline.posToFrame(delta));
                    auto draggableStart = trackStartPos + imguiClip.getCenterClickedPos().x;
                    if (!(io.MousePos.x > trackStartPos + imguiClip.getCenterClickedPos().x))
                    {
                        trackClip.setStart(0);
                        return true;
                    }
                    if (!(trackEndPos > io.MousePos.x))
                    {
                        trackClip.setStart(imGuiTimeline.deltaToFrame(trackEndPos - draggableStart));
                        return true;
                    }
                    if (trackEndPos > io.MousePos.x)
                    {
                        trackClip.setStart(start);
                        ImGui::ResetMouseDragDelta();
                    }*/
                    return false;
                }
            );
            Subscribe<ImGuiClipReleasedEvent>(
                [this](Event& e) {
                    e;
                    ImGuiTrackRefreshEvent event(weak_from_this());
                    Emit(event);
                    return false;
                }
            );
            Subscribe<ImGuiClipResizedEvent>(
                [this](Event& e) {
                    e;
                    /*auto& imGuiTimeline = wTrack.lock()->timeline().imGui();
                    auto& event = dynamic_cast<ImGuiClipResizedEvent&>(e);
                    const auto left = event.GetLeft();
                    if (left) {
                        auto& trackClip = *clip.lock();
                        const auto currStart = trackClip.getStart();
                        auto start = std::max<long long>(0, (long long)currStart + (long long)imGuiTimeline.posToFrame(left));
                        trackClip.setStart(start);
                    }*/
                    return false;
                }
            );

        }

        void ImGuiTrack::renderContent() {
            auto& imGuiTimeline = *pTimeline.lock();
            std::vector<std::shared_ptr<Context::TrackClipContext>> clips;
            size_t height;
            {
                const auto context = pContext.lock()->read();
                clips = context->clips;
                height = context->height;
            }

            //ImDrawList* drawList = ImGui::GetWindowDrawList();
            const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
            const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
            const float scrollX = ImGui::GetScrollX();

            const ImVec2 trackPos = canvasPos;

            const float trackStartPos = trackPos.x + scrollX;
            const float trackEndPos = trackStartPos + canvasSize.x;

            const float& trackMinXPos = trackPos.x;
            trackMinXPos;

            for (size_t i = 0; i < mImGuiClips.size(); i++)
            {
                auto startPos = imGuiTimeline.frameToPos(clips[i]->startFrame);
                if (startPos <= trackEndPos) {
                    mImGuiClips[i]->renderContent(startPos);
                }
            }
            /*for (auto& imGuiClip : mImGuiClips) {
                auto startPos = imGuiClip->getContext();
                if (startPos <= trackEndPos) {
                    imGuiClip->renderContent(startPos);
                }
            }*/

            trackTop += height + trackGap;
            ImGui::SetCursorScreenPos(canvasPos + ImVec2(0, (float)(height + trackGap)));

        }

        void ImGuiTrack::renderContext() {
            const ImVec2 windowPos = ImGui::GetWindowPos();

            ImDrawList* drawList = ImGui::GetWindowDrawList();

            size_t id;
            bool muted;
            size_t height;
            {
                const auto context = pContext.lock()->read();
                id = context->id;
                muted = context->muted;
                height = context->height;
            }

            ImRect trackContextRect(
                ImVec2(windowPos.x, 0), ImVec2(windowPos.x + ImGuiTrack::trackContextWidth, (float)(height))
            );
            //ImGui::SetCursorScreenPos(ImVec2(windowPos.x, trackTop));
            //ImGui::SetCursorScreenPos();
            //ImGui::Dummy(trackContextRect.GetSize());

            {
                std::string trackName("Track " + std::to_string(id));
                ImVec2 diff(0, trackTop);
                drawList->AddRectFilled(trackContextRect.Min + diff, trackContextRect.Max + diff, IM_COL32(15, 23, 42, 255));
                drawList->AddText(ImVec2(windowPos.x + textPadding, trackTop + textPadding), IM_COL32(241, 245, 249, 255), trackName.c_str());
            }
            {
                ImGui::SetCursorScreenPos(ImVec2(windowPos.x + textPadding, trackTop + height - 30 - textPadding));
                std::string checkboxName("Muted##" + std::to_string(id));
                std::string buttonName("M##" + std::to_string(id));
                if (muted) {
                    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(217, 119, 6, 255));
                }
                if (ImGui::Button(buttonName.c_str())) {
                    ImGuiTrackSetMuteStateEvent event(!muted);
                    Emit(event);
                    //track.muted = !track.muted;
                }
                if (muted) {
                    ImGui::PopStyleColor();
                }
            }
            trackTop += height + trackGap;

        }
    }
}