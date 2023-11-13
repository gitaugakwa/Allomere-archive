#include "allomerepch.h"

#include "Timeline.h"

#include "miniaudio.h"
#include "essentia.h"

//#include "events/imgui/ImGuiTrackEvent.h"
#include "events/timeline/TimelineEvent.h"

#include "boost/atomic.hpp"

//"C:\\Torrents\\REDACTED\\Flume - Hi This Is Flume (Mixtape) (2019) [WEB FLAC]\\11 - MUD.flac"

namespace Allomere {
	namespace Audio {

		Timeline::Timeline(std::weak_ptr<Timeline::TimelineContext> wContext) : pContext(wContext), Attachable()
		{
			SetupSubscriptions();
			// Initialize Essentia
			essentia::init();
#ifdef ALLOMERE_DEBUG
			essentia::setDebugLevel(essentia::ENone);
			essentia::infoLevelActive = false;
			//essentia::setDebugLevel(essentia::EAll ^ essentia::EExecution ^ essentia::EScheduler ^ essentia::EAlgorithm ^ essentia::ENetwork ^ essentia::EMemory ^ essentia::EConnectors);
#endif // ALLOMERE_DEBUG

		}

		Timeline::~Timeline() {
			essentia::shutdown();
		};

		void Timeline::SetupSubscriptions()
		{

		}

		template<typename T>
		bool Timeline::Drop(T event)
		{
			if (Emit(event))
				return true;
			for (auto& track : mTracks)
			{
				if (track->Drop(event))
				{
					return true;
				}
			}
			return false;
		}

		void Timeline::setPlayState(bool newState) {
			auto context = pContext.lock()->write();
			context->playing = newState;
		}

		void Timeline::togglePlayState() {
			bool playing;
			{
				const auto context = pContext.lock()->read();
				playing = context->playing;
			}
			setPlayState(!playing);
		}

		ma_result Timeline::read(void* pFramesOut, ma_uint64 frameCount, ma_uint64* pFramesRead)
		{
			bool playing;
			size_t time;
			{
				const auto context = pContext.lock()->read();
				playing = context->playing;
				time = context->time;
			}
			auto sampleRate = pDevice.lock()->sampleRate();
			float* pOutputF32 = (float*)pFramesOut;
			if (mTracks.size() == 0) {
				return silence(pFramesOut, frameCount, pFramesRead);
			}

			/*for (auto track : *pTracks) {
				track->read();
			}*/


			if (playing) {

				read_and_mix_pcm_frames_f32(pOutputF32, frameCount, pFramesRead);
				//ma_data_source_read_pcm_frames(pDecoder, pOutput, frameCount, NULL);
				//ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, NULL);
	//#pragma warning( disable : 4244)
				if (*pFramesRead) {
					time += sampleRate / ((*pFramesRead) * 10);
				}
				//time += ((float)frameCount / (pDevice->sampleRate));
	//#pragma warning( default : 4244)
			}
			else {
				return silence(pFramesOut, frameCount, pFramesRead);
			}
			return MA_SUCCESS;


			//ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, NULL);
		}

		ma_result Timeline::read_and_mix_pcm_frames_f32(float* pOutputF32, ma_uint32 frameCount, ma_uint64* pFramesRead)
		{
			/*
			The way mixing works is that we just read into a temporary buffer, then take the contents of that buffer and mix it with the
			contents of the output buffer by simply adding the samples together. You could also clip the samples to -1..+1, but I'm not
			doing that in this example.
			*/
			auto channels = pDevice.lock()->channels();
			auto trackCount = mTracks.size();

			silence(pOutputF32, frameCount, nullptr);

			ma_result result;
			//std::array<float, 1 << 12> buffer;
			//float* temp = buffer.data();
			float* temp = readBuffer.data();
			ma_uint32 tempCapInFrames = readBuffer.size() / channels;
			ma_uint64* trackTotalFramesRead = new ma_uint64[trackCount];

			for (size_t i = 0; i < trackCount; i++) {
				Track& track = *((mTracks)[i]);
				ma_uint64& totalFramesRead = trackTotalFramesRead[i] = 0;

				if (track.getClips().size() == 0) {
					continue;
				}
				while (totalFramesRead < frameCount) {
					ma_uint64 iSample;
					ma_uint64 framesReadThisIteration;
					ma_uint32 totalFramesRemaining = frameCount - totalFramesRead;
					ma_uint32 framesToReadThisIteration = tempCapInFrames;
					if (framesToReadThisIteration > totalFramesRemaining) {
						framesToReadThisIteration = totalFramesRemaining;
					}

					result = track.read(temp, framesToReadThisIteration, &framesReadThisIteration);
					//result = ma_decoder_read_pcm_frames(pDecoder, temp, framesToReadThisIteration, &framesReadThisIteration);
					if (result != MA_SUCCESS || framesReadThisIteration == 0) {
						break;
					}

					/* Mix the frames together. */
					for (iSample = 0; iSample < framesReadThisIteration * channels; ++iSample) {
						pOutputF32[totalFramesRead * channels + iSample] += temp[iSample];
					}

					totalFramesRead += framesReadThisIteration;

					if (framesReadThisIteration < (ma_uint32)framesToReadThisIteration) {
						break;  /* Reached EOF. */
					}
				}
				;

			}

			ma_uint64 maxTotalFramesRead = *std::max_element(trackTotalFramesRead, trackTotalFramesRead + trackCount);
			*pFramesRead = maxTotalFramesRead;

			delete[] trackTotalFramesRead;

			return MA_SUCCESS;
		}

		/*std::weak_ptr<Clip> Timeline::addClipToTrack(const char* pFilePath, size_t trackIndex, size_t initialStartFrame) {
			auto clip = mTracks[trackIndex]->addClip(pFilePath, initialStartFrame);
			clips[clip.lock()->id()] = clip;
			return clip;
		}*/

		/*void Timeline::addTrack() {
			auto imguiTrack = mImGuiTracks.emplace_back(
				std::make_shared<ImGuiTrack>(
					mTracks.emplace_back(
						std::make_shared<Track>(weak_from_this())
					)
				)
			);
			imguiTrack->Subscribe<ImGuiTrackClipFocusedEvent>(
				[&](Event& event) {
					auto& focusedEvent = dynamic_cast<ImGuiTrackClipFocusedEvent&>(event);
					Emit(focusedEvent);
					return true;
				}
			);
		}*/

		ma_result Timeline::seek(size_t ms) {
			bool playing;
			{
				const auto context = pContext.lock()->read();
				playing = context->playing;
			}
			auto prevState = playing;
			if (prevState) {
				setPlayState(false);
			}
			auto sampleRate = pDevice.lock()->sampleRate();
			size_t frame = (size_t)(((float)(ms) / 1000) * sampleRate);
			std::for_each(mTracks.begin(), mTracks.end(),
				[&](std::shared_ptr<Track>& track) {
					track->seek(frame);
				}
			);

			{
				auto context = pContext.lock()->write();
				context->time = ms;
			}

			TimelineSeekEvent event;
			Emit(event);

			setPlayState(prevState);
			return MA_SUCCESS;
		}


		std::shared_ptr<ma_resource_manager_data_source> Timeline::load(const char* pFilePath, ma_resource_manager_data_source_flags flags) const
		{
			std::shared_ptr<ma_resource_manager_data_source> dataSource(new ma_resource_manager_data_source);
			auto& device = *pDevice.lock();

			ma_resource_manager_data_source_init(
				&device.resourceManager(),
				pFilePath,
				flags,
				NULL,
				dataSource.get());

			return dataSource;
		}
	}
}