#include "allomerepch.h"

#include <implot.h>

#include <imgui.h>
#include <imgui_internal.h>

#include "events/imgui/ImGuiClipEvent.h"

#include "ImGuiClipFocused.h"

namespace Allomere {
	namespace GUI {
		using Focused = ImGuiClip::Focused;

		ma_resource_manager Focused::maResourceManager{};
		ma_resource_manager_config Focused::maResourceManagerConfig{};

		bool Focused::initialized{ false };

		Focused::Focused(std::weak_ptr<Context::ContextManager<Context::ClipContext>> wContext, std::weak_ptr<ImGuiClip> clip) : pContext(wContext), pClip(clip) {
			std::string filePath;
			{
				const auto context = pContext.lock()->read();
				filePath = context->filePath;
			}
			size_t channels;
			{
				auto context = clip.lock()->pTrack.lock()->pTimeline.lock()->pContext.lock()->read();
				channels = context->channels;
			}

			audioSamples.reserve(audioFrames * channels);

			ma_resource_manager_data_source_flags flags = MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_WAIT_INIT;
			ma_resource_manager_data_source_init(
				&maResourceManager,
				filePath.c_str(),
				flags,
				NULL,
				&dataSource);
		}

		Focused::~Focused() {
			ma_resource_manager_data_source_uninit(&dataSource);
		}

		void Focused::SetupSubscriptions()
		{

		}

		void Focused::initialize() {
			if (!initialized)
			{
				maResourceManagerConfig = ma_resource_manager_config_init();

				maResourceManagerConfig.decodedFormat = ma_format_f32;
				maResourceManagerConfig.decodedChannels = 2;
				maResourceManagerConfig.decodedSampleRate = 44100;
			}
		}

		void Focused::renderFocused(Event& event) {
			std::vector<size_t> beats;
			Context::Similarity* similarityMatrix;
			{
				const auto context = pContext.lock()->read();
				beats = context->beats;
				similarityMatrix = context->similarityMatrix.get();
			}
			size_t time;
			bool playing;
			{
				const auto context = pClip.lock()->pTrack.lock()->pTimeline.lock()->pContext.lock()->read();
				playing = context->playing;
				time = context->time;
			}

			auto& focusedEvent = dynamic_cast<ImGuiClipFocusedEvent&>(event);

			const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
			const ImVec2 canvasSize = ImGui::GetContentRegionAvail();


			const ImVec2 trackPos = ImGui::GetCursorScreenPos();

			ImDrawList* drawList = ImGui::GetWindowDrawList();

			float cursorPos = (static_cast<float>(time) / 1000);
			//auto timelineStart = 0;

			size_t startFrame = focusedEvent.GetStartFrame();
			//size_t startFrame = clip.startFrame;


			auto beatsCount = beats.size();

			if (playing || !beatsInitialized) {
				refreshLive(focusedEvent);
			}
			//auto beat{ beats.begin()};


			const ImVec2 audioGraphSize(canvasSize.x, (canvasSize.y - 5) * 0.6f);
			const ImVec2 beatGraphSize(canvasSize.x, (canvasSize.y - 5) * 0.4f);
			const size_t stride = (size_t)(20 * scale);
			const size_t count = audioFrames / stride;
			ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(5, 5));
			if (ImPlot::BeginPlot("Clip Audio Graph", audioGraphSize, ImPlotFlags_NoChild | ImPlotFlags_NoMouseText | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoLegend | ImPlotFlags_NoTitle))
			{
				ImPlot::SetupAxisLimits(ImAxis_Y1, -1, 1, ImPlotCond_Always);
				ImPlot::SetupAxisLimits(ImAxis_X1, 0, (double)count);
				ImPlot::SetupAxisLimitsConstraints(ImAxis_X1, 0, (double)count);
				//ImPlot::SetupAxisLimits(ImAxis_X1, -1, 1, ImPlotCond_Always);
				//ImPlot::SetupAxes("Time", "Sample");
				ImPlotAxisFlags axesFlags = ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_Lock;
				ImPlot::SetupAxes("Sample", "Value", axesFlags, axesFlags);
				/*auto offsetterLeft = [](int i, void* data) {
					auto& val = *((std::array<float, frameCount * 2>*)data);
					return ImPlotPoint(val[i], i);
				};
				auto offsetterRight = [](int i, void* data) {
					auto& val = *((std::array<float, frameCount * 2>*)data);
					return ImPlotPoint(val[i+1], i);
				};*/

				//ImPlot::PlotLineG("Left", offsetterLeft, &audioPoints, frameCount / 50);
				//ImPlot::PlotLineG("Right", offsetterRight, &audioPoints, frameCount / 50);
				ImPlot::PlotLine("Left", audioSamples.data(), (int)(count), 1, 0, 0, 0, (int)(sizeof(float) * stride * 2));
				ImPlot::PlotLine("Right", audioSamples.data() + 1, (int)(count), 1, 0, 0, 0, (int)(sizeof(float) * stride * 2));
				ImPlot::EndPlot();
			}
			const ImVec2 newCanvasPos = ImGui::GetCursorScreenPos();
			if (ImPlot::BeginPlot("Clip Beat Graph", beatGraphSize, ImPlotFlags_NoChild | ImPlotFlags_NoMouseText | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoLegend | ImPlotFlags_NoTitle))
			{
				ImPlot::SetupAxisLimits(ImAxis_Y1, -1, 1, ImPlotCond_Always);
				ImPlot::SetupAxisLimits(ImAxis_X1, 0, (double)audioFrames);
				ImPlot::SetupAxisLimitsConstraints(ImAxis_X1, 0, (double)audioFrames);

				ImPlotAxisFlags axesFlags = ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_Lock; //  | ImPlotAxisFlags_AutoFit;
				ImPlot::SetupAxes("Beat Time", "Beat Map", axesFlags, axesFlags);

				float mk_size = ImPlot::GetStyle().MarkerSize;
				float ys{ 0 };
				size_t beatId = 0;

				if (beatsInitialized) {
					for (std::vector<size_t>::const_iterator beat = focusedStartBeat; beat != focusedEndBeat; beat++, beatId++) {
						ImGui::PushID((int)beatId);
						float xs{ (float)(*beat) - focusedStartFrame };
						size_t beatIndex = std::distance(beats.cbegin(), beat);
						const float thickness = 1.5f;
						if (similarityMatrix != nullptr) {
							auto* refSim = similarityMatrix + (beatIndex * beatsCount);
							auto querySim = Context::Similarity::iterator(similarityMatrix + beatIndex, beatsCount);
							auto lerpIota = std::ranges::views::iota(0, 10);
							// Refernece
							auto* refStart = std::find_if(refSim, refSim + beatsCount, [](const auto& sim) {return sim.calculated; });
							auto* refEnd = std::find_if(refStart, refSim + beatsCount, [](const auto& sim) {return !sim.calculated; });

							// Query
							auto queryStart = std::find_if(querySim, querySim + beatsCount, [](const auto& sim) { return sim.calculated; });
							auto queryEnd = std::find_if(queryStart, querySim + beatsCount, [](const auto& sim) { return !sim.calculated; });
							std::for_each(refStart, refEnd,
								[&, this, xs](const auto& similarity) {
									float ps[] = {
										((float)beats[similarity.query] - (float)focusedStartFrame), 0, xs,
										0,1,0
									};
									ps[1] = std::midpoint(ps[0], ps[2]);

									auto pointsCount = lerpIota.size();

									Eigen::Matrix<float, 2, 3, Eigen::RowMajor> points = Eigen::Map<Eigen::Matrix<float, 2, 3, Eigen::RowMajor>>(ps);
									Eigen::Spline2f spline = Eigen::SplineFitting<Eigen::Spline2f>::Interpolate(points, 2);
									Eigen::Array<float, 2, -1, Eigen::RowMajor> values;
									values.resize(Eigen::NoChange, pointsCount);

									std::for_each(lerpIota.begin(), lerpIota.end(),
										[&, pointsCount](auto interp) {
											//auto lerpValue = std::lerp(ps[0], ps[2], (float)interp / 20);
											auto lerpValue = (float)interp / (pointsCount - 1);
											values.col(interp) = spline(lerpValue);
										}
									);

									float* xsp = values.row(0).data();
									float* ysp = values.row(1).data();

									const auto color = ImVec4(255, 255, 255, 1);
									ImPlot::SetNextLineStyle(color, thickness);
									ImPlot::PlotLine("Beat Similarity", xsp, ysp, pointsCount);
								}
							);
							std::for_each(queryStart, queryEnd,
								[&, this, xs](const auto& similarity) {
									float ps[] = {
										xs, 0, ((float)beats[similarity.reference] - (float)focusedStartFrame),
										0,1,0
									};
									ps[1] = std::midpoint(ps[0], ps[2]);

									auto pointsCount = lerpIota.size();

									Eigen::Matrix<float, 2, 3, Eigen::RowMajor> points = Eigen::Map<Eigen::Matrix<float, 2, 3, Eigen::RowMajor>>(ps);
									Eigen::Spline2f spline = Eigen::SplineFitting<Eigen::Spline2f>::Interpolate(points, 2);
									Eigen::Array<float, 2, -1, Eigen::RowMajor> values;
									values.resize(Eigen::NoChange, pointsCount);

									std::for_each(lerpIota.begin(), lerpIota.end(),
										[&, pointsCount](auto interp) {
											//auto lerpValue = std::lerp(ps[0], ps[2], (float)interp / 20);
											auto lerpValue = (float)interp / (pointsCount - 1);
											values.col(interp) = spline(lerpValue);
										}
									);

									float* xsp = values.row(0).data();
									float* ysp = values.row(1).data();

									const auto color = ImVec4(255, 255, 255, 1);
									ImPlot::SetNextLineStyle(color, thickness);
									ImPlot::PlotLine("Beat Similarity", xsp, ysp, pointsCount);
								}
							);

						}
						ImPlot::SetNextMarkerStyle(ImPlotMarker_Diamond, mk_size);
						ImPlot::PlotLine("Beat", &xs, &ys, 1);
						ImGui::PopID();
					}
				}
				ImPlot::EndPlot();
			}
			ImPlot::PopStyleVar();
			auto canvasScale = cursorPos - ((float)(startFrame) / 44100) - ((float)focusedStartFrame / 44100);
			if (canvasScale >= 0 && canvasScale <= 1) {
				drawList->AddLine(ImVec2(canvasPos.x + 5 + ((canvasSize.x - 10) * canvasScale), canvasPos.y + 5), ImVec2(canvasPos.x + 5 + ((canvasSize.x - 10) * canvasScale), canvasPos.y + audioGraphSize.y - 5), IM_COL32(220, 38, 38, 255));
				drawList->AddLine(ImVec2(newCanvasPos.x + 5 + ((canvasSize.x - 10) * canvasScale), newCanvasPos.y + 5), ImVec2(newCanvasPos.x + 5 + ((canvasSize.x - 10) * canvasScale), newCanvasPos.y + beatGraphSize.y - 5), IM_COL32(220, 38, 38, 255));
			}
		}

		void Focused::refresh(Event& event) {
			size_t length;
			{
				const auto context = pContext.lock()->read();
				length = context->length();
			}
			size_t time;
			size_t sampleRate;
			{
				const auto context = pClip.lock()->pTrack.lock()->pTimeline.lock()->pContext.lock()->read();
				time = context->time;
				sampleRate = context->sampleRate;
			}
			//auto& imGuiTimeline = timeline.imGui();

			float cursorPos = (static_cast<float>(time) / 1000);

			auto& focusedEvent = dynamic_cast<ImGuiClipFocusedEvent&>(event);
			size_t startFrame = focusedEvent.GetStartFrame();

			size_t currentTimelineFrame = (size_t)(cursorPos * sampleRate);
			size_t currentFrame = std::max<long long>(0, (long long)currentTimelineFrame - (long long)startFrame);


			//const auto beatsCount = beats.size();

			if (focusedStartFrame > currentFrame || currentFrame > focusedEndFrame) {
				size_t framesRead;
				auto readStart = std::min<long long>(currentFrame, (long long)length - (long long)audioFrames);
				{
					getFrames(audioSamples.data(), readStart, readStart + audioFrames, &framesRead);

					if (framesRead < audioFrames) {
						auto startBlank = audioSamples.begin() + framesRead * 2;
						std::fill(startBlank, audioSamples.end() - 1, 0.0f);
					}
				}
				focusedStartFrame = readStart;
				focusedEndFrame = currentFrame + framesRead;
			}

			{
				std::vector<size_t> beats;
				{
					const auto context = pContext.lock()->read();
					beats = context->beats;
				}
				focusedStartBeat = std::find_if(beats.begin(), beats.end(),
					[&](size_t beat) {
						return focusedStartFrame <= beat;
					}
				);
				focusedEndBeat = std::find_if(focusedStartBeat, beats.cend(),
					[&](size_t beat) {
						return focusedEndFrame <= beat;
					}
				);
				beatsInitialized = true;
			}
		}

		void Focused::refreshLive(Event& event) {
			std::vector<size_t> beats;
			size_t length;
			{
				const auto context = pContext.lock()->read();
				beats = context->beats;
				length = context->length();
			}
			size_t time;
			size_t sampleRate;
			{
				const auto context = pClip.lock()->pTrack.lock()->pTimeline.lock()->pContext.lock()->read();
				time = context->time;
				sampleRate = context->sampleRate;
			}
			float cursorPos = (static_cast<float>(time) / 1000);

			auto& focusedEvent = dynamic_cast<ImGuiClipFocusedEvent&>(event);
			size_t startFrame = focusedEvent.GetStartFrame();

			size_t currentTimelineFrame = (size_t)(cursorPos * sampleRate);
			size_t currentFrame = std::max<long long>(0, (long long)currentTimelineFrame - (long long)startFrame);

			const auto isInClip = (std::clamp(currentFrame, startFrame, startFrame + length));

			if (!beatsInitialized) {
				{
					focusedStartBeat = std::find_if(beats.begin(), beats.end(),
						[&](size_t beat) {
							return focusedStartFrame <= beat;
						}
					);
					focusedEndBeat = std::find_if(focusedStartBeat, beats.cend(),
						[&](size_t beat) {
							return focusedEndFrame <= beat;
						}
					);
					beatsInitialized = true;
				}
			}

			if (!isInClip)
			{
				return;
			}
			//const auto beatsCount = beats.size();


			if (focusedStartFrame > currentFrame || currentFrame > focusedEndFrame) {
				size_t framesRead;
				auto readStart = std::min<long long>(currentFrame, (long long)length - (long long)audioFrames);
				{

					getFrames(audioSamples.data(), readStart, readStart + audioFrames, &framesRead);

					if (framesRead < audioFrames) {
						auto startBlank = audioSamples.begin() + framesRead * 2;
						std::fill(startBlank, audioSamples.end() - 1, 0.0f);
					}
				}
				focusedStartFrame = readStart;
				focusedEndFrame = currentFrame + framesRead;


				{
					focusedStartBeat = std::find_if(beats.begin(), beats.end(),
						[&](size_t beat) {
							return focusedStartFrame <= beat;
						}
					);
					focusedEndBeat = std::find_if(focusedStartBeat, beats.cend(),
						[&](size_t beat) {
							return focusedEndFrame <= beat;
						}
					);
					beatsInitialized = true;
				}
			}
		}

		ma_result Focused::getFrames(void* pFramesOut, ma_uint64 startFrame, ma_uint64 endFrame, ma_uint64* framesRead)
		{
			int result = MA_SUCCESS;
			if (startFrame > endFrame)
			{
				return MA_INVALID_OPERATION;
			}
			ma_uint64 frameCount{ endFrame - startFrame };
			result |= ma_data_source_seek_to_pcm_frame(&dataSource, startFrame);
			result |= ma_data_source_read_pcm_frames(&dataSource, pFramesOut, frameCount, framesRead);
			return (ma_result)result;
		}
	}
}