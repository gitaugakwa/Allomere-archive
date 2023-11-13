#include "allomerepch.h"
#include "Track.h"

#include "events/timeline/TrackEvent.h"
#include "events/timeline/ClipEvent.h"

namespace Allomere {
	namespace Audio {

		float* Track::pSilenceData = NULL;
		ma_audio_buffer_config Track::maSilenceBufferConfig;
		ma_audio_buffer Track::maSilenceBuffer;

		Track::Track(std::weak_ptr<Track::TrackContext> wContext,std::weak_ptr<Timeline> wTimeline) : pContext(wContext), pTimeline(wTimeline) {
			SetupSubscriptions();

			auto& t = *wTimeline.lock();

			auto sampleRate = t.sampleRate();
			auto channels = t.channels();
			auto format = t.format();

			if (pSilenceData == NULL) {
				pSilenceData = new float[sampleRate * channels];
				std::fill(pSilenceData, pSilenceData + (sampleRate * channels), 0.0f);
				maSilenceBufferConfig = ma_audio_buffer_config_init(format, channels, sampleRate, pSilenceData, NULL);
				ma_audio_buffer_init(&maSilenceBufferConfig, &maSilenceBuffer);
			}

			/*Clip::GlobalSubscribe([&](Event& event) {
				event.
				Emit(event);
				return false;
				});*/
		}

		void Track::SetupSubscriptions()
		{
			Subscribe<ClipCutEvent>(
				[&](Event& event) {
					auto& cutEvent = dynamic_cast<ClipCutEvent&>(event);

					auto cutClipIt = std::find_if(mClips.begin(), mClips.end(),
						[&](const std::shared_ptr<Clip>& clip) {
							return clip->id() == cutEvent.GetID();
						}
					);

					if (cutClipIt == mClips.end()) {
						return true;
					}

					return refreshState(**cutClipIt);
				}
			);
			Subscribe<ClipAddedEvent>(
				[&, this](Event& e) {
					auto& addedEvent = dynamic_cast<ClipAddedEvent&>(e);

					if (addedEvent.GetTrack() == mId)
					{
						auto clip = mClips.emplace_back(
							new Clip(addedEvent.GetContext(), weak_from_this(), addedEvent.GetStartFrame())
						);
						std::sort(mClips.begin(), mClips.end(), sortByStartFrame);
						if (mClips[0]->startFrame == 0) {
							inClip = true;
						}
						return true;
					}

					return false;
				}
			);
		}


		bool Track::sortByStartFrame(const std::shared_ptr<Clip>& clip1, const std::shared_ptr<Clip>& clip2)
		{
			return (*clip1) < (*clip2);
		}

		bool Track::refreshState()
		{
			size_t sampleRate;
			size_t time;
			{
				const auto context = pTimeline.lock()->pContext.lock()->read();
				sampleRate = context->sampleRate;
				time = context->time;
			}

			const size_t frame = (size_t)(((float)time / 1000) * sampleRate);

			return refreshState(frame);
		}

		bool Track::refreshState(size_t frame)
		{
			auto currentClipIt = std::find_if(mClips.rbegin(), mClips.rend(),
				[&](std::shared_ptr<Clip>& clip) {
					return clip->getStart() <= frame;
				}
			);
			if (currentClipIt == mClips.rend())
			{
				auto endClipIt = mClips.end();
				if (endClipIt != mClips.begin())
				{
					(*prev(endClipIt))->seek(0);
				}
				inClip = false;
				return true;
			}
			auto& currentClip = **currentClipIt;
			size_t startFrame = currentClip.getStart();
			size_t samplesIn = frame - startFrame;
			if (samplesIn <= currentClip.length()) {
				currentClip.seek(samplesIn);
				inClip = true;
			}
			else {
				inClip = false;
			}
			return true;
		}

		bool Track::refreshState(Clip& clip)
		{
			size_t sampleRate;
			size_t time;
			bool playing;
			{
				const auto context = pTimeline.lock()->pContext.lock()->read();
				sampleRate = context->sampleRate;
				time = context->time;
				playing = context->playing;
			}
			const size_t frame = (size_t)(((float)time / 1000) * sampleRate);

			const auto startFrame = clip.getStart();

			if (startFrame <= frame && frame <= startFrame + clip.length())
			{
				if (playing) {
					//timeline.playing = false;
				}
				clip.seek(frame - startFrame);
				inClip = true;
				//timeline.playing = playing;
			}
			return true;
		}

		//std::weak_ptr<Track::Clip> Track::addClip(const char* pFilePath, size_t initialStartFrame) {
		//	// Clip
		//	auto clip = clips.emplace_back(std::make_shared<Clip>(pFilePath, weak_from_this(), initialStartFrame));

		//	// Track
		//	std::sort(clips.begin(), clips.end(), sortByStartFrame);
		//	if (clips[0]->startFrame == 0) {
		//		inClip = true;
		//	}

		//	ClipAddedEvent event(clip);
		//	Emit(event);

		//	return clip;
		//}

		ma_result Track::read(void* pFramesOut, ma_uint64 frameCount, ma_uint64* pFramesRead)
		{
			size_t channels;
			{
				const auto context = pTimeline.lock()->pContext.lock()->read();
				channels = context->channels;
			}
			bool muted;
			{
				const auto context = pContext.lock()->read();
				muted = context->muted;
			}
			ma_result result = MA_SUCCESS;
			ma_uint64 framesRead = 0;
			ma_uint64 silenceCount = frameCount;
			float* pOut = (float*)pFramesOut;
			
			if (muted) {
				pOut = NULL;
			}

			if (inClip) {
				result = mClips[0]->read(pOut, frameCount, &framesRead);
				if (framesRead < frameCount) {
					inClip = false;
					if (pOut != NULL) {
						pOut += framesRead * channels;
						silenceCount = frameCount - framesRead;
					}
					else {
						silenceCount = 0;
					}
				}
			}
			if (!inClip && silenceCount > 0) {
				auto startClip = mClips.begin();
				for (size_t i = 0; i < mClips.size(); i++) {
					long long diff = mClips[i]->startFrame - currentFrameCount;
					if (diff >= 0 && diff <= (long long)silenceCount) {
						clipIndex = i;
						silenceCount = diff;
						break;
					}
					if (diff > (long long)silenceCount) {
						break;
					}
				}
				(framesRead) += ma_audio_buffer_read_pcm_frames(&maSilenceBuffer, pOut, silenceCount, true);
				if (framesRead < frameCount) {
					inClip = true;
					pOut += framesRead * channels;

					result = read(pOut, frameCount - framesRead, &framesRead);
				}

			}

			*pFramesRead = framesRead;

			currentFrameCount += *pFramesRead;

			return result;

		}

		size_t Track::length() {
			auto clipSize = mClips.size();
			if (clipSize == 0) {
				return 0;
			}
			std::vector<size_t> clipLengths(mClips.size());
			std::transform(
				mClips.begin(),
				mClips.end(),
				clipLengths.begin(), [](std::shared_ptr<Clip>& clip) {
					return clip->startFrame + clip->length();
				});
			return *std::max_element(clipLengths.begin(), clipLengths.end());
		}

		/*void Track::toggleMuteState()
		{
			muted = !muted;
		}*/

		ma_result Track::seek(size_t frame)
		{
			refreshState(frame);
			currentFrameCount = frame;
			return MA_SUCCESS;
		}
	}
}