#include "allomerepch.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <string>

#include "events/imgui/ImGuiClipEvent.h"
#include "events/imgui/ImGuiTrackEvent.h"
#include "events/imgui/ImGuiTimelineEvent.h"
#include "events/timeline/TimelineEvent.h"
#include "events/timeline/TrackEvent.h"
#include "events/imgui/ImGuiClipEvent.h"

#include "ImGuiTimeline.h"


namespace Allomere {
    namespace GUI {

        //std::weak_ptr<Focusable> ImGuiTimeline::focus{};

        ImGuiTimeline::ImGuiTimeline(std::weak_ptr<Context::ContextManager<Context::TimelineContext>> wContext, std::weak_ptr<TimelinePanel> wPanel) : pContext(wContext), pPanel(wPanel)
        {
            //SetParent(wPanel);
            SetupSubscriptions();
        }

        void ImGuiTimeline::SetupSubscriptions()
        {
            Subscribe<TrackCreatedEvent>([this](Event& e) {
                auto& addedEvent = dynamic_cast<TrackCreatedEvent&>(e);

                auto id = pContext.lock()->read()->id();

                if (addedEvent.GetTimeline() == id)
                {
                    auto context = addedEvent.GetContext().lock();
                    auto imGuiClip = mImGuiTracks.emplace_back(new ImGuiTrack(context, weak_from_this()));
                    ALLOMERE_CORE_INFO("Track Added to Timeline[{0}]", id);
                    return true;
                }
                return false;
                });
        }

        void ImGuiTimeline::renderContext() {
            const ImVec2 windowPos = ImGui::GetWindowPos();
            const ImVec2 windowSize = ImGui::GetWindowSize();

            const ImVec2 canvasPos = ImGui::GetCursorScreenPos();

            ImDrawList* drawList = ImGui::GetWindowDrawList();

            size_t time;

            {
                const auto context = pContext.lock()->read();
                time = context->time;
            }

            std::chrono::milliseconds duration{ time };

           /* drawList->AddRectFilled(
                ImVec2(windowPos.x, canvasPos.y),
                ImVec2(windowPos.x + 200, canvasPos.y + topBarHeight + (ImGuiTrack::trackHeight + ImGuiTrack::trackGap) * mImGuiTracks.size()),
                IM_COL32(15, 15, 15, 255)
            );*/

            drawList->AddRectFilled(
                ImVec2(windowPos.x, canvasPos.y),
                ImVec2(windowPos.x + 200, canvasPos.y + topBarHeight),
                IM_COL32(15, 23, 42, 255)
            );

            {
                ImGui::SetCursorScreenPos(ImVec2(windowPos.x + 10, canvasPos.y + 5));
                auto locked = cursorLock;
                if (locked)
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(217, 119, 6, 255));
                }
                if (ImGui::Button(">|<"))
                {
                    cursorLock = !cursorLock;
                }
                if (locked)
                {
                    ImGui::PopStyleColor();
                }
                ImGui::SetCursorScreenPos(canvasPos);
            }

            {
                auto h = std::chrono::duration_cast<std::chrono::hours>(duration);
                duration -= h;
                auto m = std::chrono::duration_cast<std::chrono::minutes>(duration);
                duration -= m;
                auto s = std::chrono::duration_cast<std::chrono::seconds>(duration);
                duration -= s;
                std::ostringstream ss;
                ss.fill('0');
                ss << std::setw(2) << h.count() << ":"
                    << std::setw(2) << m.count() << ":"
                    << std::setw(2) << s.count() << "."
                    << std::setw(3) << duration.count();
                drawList->AddText(ImVec2(windowPos.x + textPadding, canvasPos.y + textPadding + (topBarHeight - tickHeight)), IM_COL32(241, 245, 249, 255), ss.str().c_str());
            }

            ImGuiTrack::setTrackTop(canvasPos.y + topBarHeight + topBarMargin + 0);

            for (auto& imGuiTrack : mImGuiTracks) {
                imGuiTrack->renderContext();
            }
        }

        void ImGuiTimeline::renderContent() {

            //unsigned int trackHeight = 150;
            //unsigned int trackGap = 5;
            //ImGuiIO& io = ImGui::GetIO();
            const ImVec2 windowPos = ImGui::GetWindowPos();
            const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            const ImVec2 windowSize = ImGui::GetWindowSize();
            const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
            float timelineStart = canvasPos.x + 200;

            float scale;
            size_t time;
            bool playing;
            {
                const auto context = pContext.lock()->read();
                scale = context->scale;
                time = context->time;
                playing = context->playing;
            }

            auto secondScale = 1000 / scale;

            if (ImGui::IsWindowFocused())
            {
                if (ImGui::IsKeyDown(ImGuiKey_Space)) {
                    if (!spaceBarToggleStopwatch.isStarted()) {
                        spaceBarToggleStopwatch.Start();
                        ImGuiTimelineSetPlayStateEvent event(true);
                        Emit(event);
                        //timeline.setPlayState(true);
                    }
                }

                if (ImGui::IsKeyReleased(ImGuiKey_Space)) {
                    auto timestep = spaceBarToggleStopwatch.Lap();
                    auto ms = timestep.milliseconds();
                    if (ms < 150 && spaceBarToggleStopwatch.FromStart().milliseconds() > 150) {
                        spaceBarToggleStopwatch.Stop();
                        ImGuiTimelineSetPlayStateEvent event(false);
                        Emit(event);
                        //timeline.setPlayState(false);
                    }
                    else if (ms > 150) {
                        spaceBarToggleStopwatch.Stop();
                        ImGuiTimelineSetPlayStateEvent event(false);
                        Emit(event);
                        //timeline.setPlayState(false);
                    }
                }

                if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
                    ImGuiTimelineSeekEvent event(time + 1000);
                    Emit(event);
                    //timeline.seek(timeline.time() + 1000);
                }
                if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
                    if (time <= 1000) {
                        ImGuiTimelineSeekEvent event(0);
                        Emit(event);
                        //timeline.seek(0);
                        return;
                    }
                    ImGuiTimelineSeekEvent event(time - 1000);
                    Emit(event);
                    //timeline.seek(time - 1000);
                }
                /*if (ImGui::IsKeyDown(ImGuiKey_RightCtrl) || ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
                {
                    ImGui::SetItemUsingMouseWheel();
                    ALLOMERE_CORE_INFO("Mouse Wheel: {0}", ImGui::GetIO().MouseWheel);
                }*/
            }

            if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl))
            {
                ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
                float wheel = ImGui::GetIO().MouseWheel;
                // min 10px per 1ms
                // max 1px per 10s
                if (wheel) {
                    auto scrollX = ImGui::GetScrollX();
                    auto scalableLength = scrollX - ((scrollX + (canvasSize.x - 400) / 2) * (wheel / scale));
                    scale = std::clamp<float>(scale + wheel, 0.1f, 1000 * 10);

                    ImGui::SetScrollX(scalableLength);
                }
            }



            /*if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Space)) {
                ImGui::IsKeyDown(ImGuiKey_Space);
                timeline.togglePlayState();
            }*/

            float topBarTop = canvasPos.y;

            //float trackTop = canvasPos.y + topBarHeight + topBarMargin;

            ImGuiTrack::setTrackTop(canvasPos.y + topBarHeight + topBarMargin);


            auto scrollY = ImGui::GetScrollY();
            ImRect timelineContent(
                ImVec2(windowPos.x + 208, canvasPos.y + topBarHeight),
                ImVec2(windowPos.x + windowSize.x, canvasPos.y + canvasSize.y + scrollY)
            );

            drawList->PushClipRect(
                timelineContent.Min,
                timelineContent.Max,
                true);
            ImGui::SetCursorScreenPos(canvasPos + ImVec2(200, (float)(topBarHeight + topBarMargin)));
            for (auto& imGuiTrack : mImGuiTracks) {
                imGuiTrack->renderContent();
            }
            drawList->PopClipRect();


            //const auto width = ImGui::GetScrollMaxX();
            const auto height = ImGui::GetScrollMaxY();

            {
                float linePosition = timelineStart;
                /*ImRect timelineBar(
                    ImVec2(timelineStart, topBarTop),
                    ImVec2(canvasPos.x + canvasSize.x + width, topBarTop + topBarHeight)
                );*/
                ImRect tickButton(
                    ImVec2(windowPos.x + 208, canvasPos.y + topBarHeight - tickHeight),
                    ImVec2(windowPos.x + windowSize.x, canvasPos.y + topBarHeight)
                );
                ImRect mapButton(
                    ImVec2(windowPos.x + 208, canvasPos.y),
                    ImVec2(windowPos.x + windowSize.x, canvasPos.y + topBarHeight - tickHeight - 2)
                );
                ImGui::SetCursorScreenPos(tickButton.Min);
                drawList->AddRectFilled(
                    mapButton.Min,
                    mapButton.Max,
                    IM_COL32(15, 23, 42, 255)
                );
                drawList->PushClipRect(
                    tickButton.Min,
                    tickButton.Max,
                    true
                );
                ImGui::InvisibleButton("TimelineBar", tickButton.GetSize());
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    const auto mousePos = ImGui::GetMousePos();
                    // Set Timeline Focus
                    auto cursorSeek = mousePos.x - canvasPos.x - 200;
                    ImGuiTimelineSeekEvent event((size_t)((cursorSeek / secondScale) * 1000));
                    Emit(event);
                    //timeline.seek((size_t)((cursorSeek / secondScale) * 1000));
                }
                ImGui::SetCursorScreenPos(canvasPos);

                drawList->AddRectFilled(
                    tickButton.Min,
                    tickButton.Max,
                    IM_COL32(15, 23, 42, 255)
                );

                // Have a different render for the ticks and cursor and timeline when cursor locked and playing

                const auto scrollX = ImGui::GetScrollX();
                const auto lineTop = topBarTop + topBarHeight - tickHeight;
                const auto timelineTextEnd = (scrollX + canvasSize.x - 200) / secondScale;
                const auto textSize = ImGui::CalcTextSize("00:00:00", NULL, true);
                const auto timeSize = (textSize.x + textPadding * 2);
                const int interval = (int)std::ceil(timeSize / secondScale);
                for (int i = int((scrollX / secondScale) / interval) * interval; i < timelineTextEnd; i += interval) {
                    size_t seconds = i;
                    size_t minutes = seconds / (60);
                    size_t hours = minutes / (60);
                    minutes %= 60;
                    seconds %= 60;
                    std::ostringstream ss;
                    ss.fill('0');
                    ss << std::setw(2) << hours << ":"
                        << std::setw(2) << minutes << ":"
                        << std::setw(2) << seconds;
                    const auto linePos = linePosition + (secondScale * i);
                    const ImVec2 linePosVec(linePos, lineTop);
                    drawList->AddLine(linePosVec, linePosVec + ImVec2(0, (float)(topBarHeight - 2)), IM_COL32(241, 245, 249, 255), 1.5);
                    drawList->AddText(linePosVec + ImVec2(3, 10), IM_COL32(241, 245, 249, 255), ss.str().c_str());
                }
                drawList->PopClipRect();
            }
            float cursorPos = (static_cast<float>(time) / 1000);
            float cursorPosX = timelineStart + (secondScale * cursorPos);

            auto timelineScreenMidpoint = (canvasSize.x - 200) / 2;
            auto cursorTimelinePos = cursorPosX - canvasPos.x - 200;
            if (cursorLock && playing && cursorTimelinePos > timelineScreenMidpoint) {
                ImGui::SetScrollX(cursorTimelinePos - timelineScreenMidpoint);
            }
            if (cursorTimelinePos > timelineScreenMidpoint && cursorLock && playing) {
                drawList->AddLine(ImVec2(windowPos.x + timelineScreenMidpoint + 210, canvasPos.y + topBarHeight - tickHeight), ImVec2(windowPos.x + timelineScreenMidpoint + 210, canvasPos.y + canvasSize.y + height), IM_COL32(220, 38, 38, 255));
            }
            else {
                drawList->AddLine(ImVec2(cursorPosX, canvasPos.y + topBarHeight - tickHeight), ImVec2(cursorPosX, canvasPos.y + canvasSize.y + height), IM_COL32(220, 38, 38, 255));
            }

        }

        /*void ImGuiTimeline::setFocus(std::weak_ptr<Focusable> clip) {
            focus = clip;
        }*/

        float ImGuiTimeline::frameToPos(size_t frame) const
        {
            float scale;
            size_t sampleRate;
            {
                const auto context = pContext.lock()->read();
                scale = context->scale;
                sampleRate = context->sampleRate;
            }
            auto secondScale = 1000 / scale;
            return ((float)frame / sampleRate) * secondScale;
        }
        size_t ImGuiTimeline::posToFrame(float pos) const
        {
            float scale;
            size_t sampleRate;
            {
                const auto context = pContext.lock()->read();
                scale = context->scale;
                sampleRate = context->sampleRate;
            }
            auto secondScale = 1000 / scale;
            return (size_t)((pos / secondScale) * sampleRate);
        }
        long long ImGuiTimeline::deltaToFrame(float pos) const
        {
            float scale;
            size_t sampleRate;
            {
                const auto context = pContext.lock()->read();
                scale = context->scale;
                sampleRate = context->sampleRate;
            }
            auto secondScale = 1000 / scale;
            return (long long)((pos / secondScale) * sampleRate);
        }
    }
}