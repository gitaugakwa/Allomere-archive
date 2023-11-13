#include "allomerepch.h"
#include "Clip.h"

#include <valarray>

#include <cmath>
#include "events/timeline/ClipEvent.h"


#include "editor/audio/Audio.h"
#include "editor/audio/track/Track.h"
#pragma warning(disable: 4100 4554)
#include "essentia/algorithmfactory.h"
//#include "algorithms/rhythm/beattrackerdegara.h"
#include "essentia.h"
#include "essentia/pool.h"
#include "essentia/streaming/algorithms/poolstorage.h"
#include "essentia/scheduler/network.h"
#include "essentia/streaming/algorithms/vectorinput.h"
#include "essentia/streaming/algorithms/vectoroutput.h"
#pragma warning(default: 4100 4554)

#if defined(ALLOMERE_SIMILARITY) || defined(ESSENTIA_SIMILARITY)
#else
#define ALLOMERE_SIMILARITY 1
#endif

#if defined(ESSENTIA_FEATURE_SPECTRUM) || defined(ESSENTIA_FEATURE_HPCP)
#else
#define ESSENTIA_FEATURE_SPECTRUM 1
#endif

#if defined(ALLOMERE_SIMILARITY)
	#if defined(ALLOMERE_SIMILARITY_MSE)
	#else
	#define ALLOMERE_SIMILARITY_MSE 1
	#endif
#elif defined(ESSENTIA_SIMILARITY)
	#if defined(ESSENTIA_SIMILARITY_CSS)
	#else
	#define ESSENTIA_SIMILARITY_CSS 1
	#endif
#endif

namespace Allomere {
	namespace Audio {

		bool Clip::initialized{ false };

		void Clip::initialize()
		{
			if (!initialized)
			{
				initialized = true;
			}
		}

		Clip::Clip(std::weak_ptr<Clip::ClipContext> wContext, std::weak_ptr<Track> wTrack) : pContext(wContext), pTrack(wTrack)
		{
			initialize();
			SetupSubscriptions();
			auto& timeline = wTrack.lock()->timeline();
			std::string filePath;
			{
				const auto context = pContext.lock()->read();
				filePath = context->filePath;
			}

			auto source = timeline.load(filePath.c_str());

			//auto sampleRate = timeline.sampleRate();

			mDataSources.push_back(source);
			rawDataSource = timeline.load(filePath.c_str());



			//mBeatsFuture = std::async(std::launch::async,
			//	[=, this]() {
			//		std::vector<essentia::Real> audio;
			//		std::vector<essentia::Real> ticks;
			//		std::vector<size_t> beats;

			//		auto* ticksOutput = new essentia::streaming::VectorOutput<essentia::Real>();

			//		auto& factory = essentia::streaming::AlgorithmFactory::instance();

			//		auto* sMonoLoader = factory.create("MonoLoader", "filename", filePath.c_str(), "sampleRate", sampleRate);
			//		auto* sBTDegara = factory.create("BeatTrackerDegara", "maxTempo", 208, "minTempo", 40);

			//		sMonoLoader->output("audio") >> sBTDegara->input("signal");
			//		sBTDegara->output("ticks") >> *ticksOutput;

			//		auto* ticksNetwork = new essentia::scheduler::Network(sMonoLoader);

			//		ticksOutput->setVector(&ticks);

			//		ticksNetwork->run();

			//		ALLOMERE_CORE_INFO("TickCount: {0} BPM: {1:.2f}", ticks.size(), ticks.size() / ((float)(originalLength()) / (sampleRate * 60)));
			//		beats.resize(ticks.size());

			//		std::transform(std::execution::par, ticks.begin(), ticks.end(), beats.begin(),
			//			[sampleRate](essentia::Real val) {
			//				return val * sampleRate;
			//			}
			//		);

			//		delete sMonoLoader;
			//		delete sBTDegara;

			//		mBeats.swap(beats);
			//		mBeatsInitialized = true;

			//		ClipBeatsGeneratedEvent event(mId);
			//		Emit(event);

			//		return;
			//		//return beats;
			//	}
			//);


//			mSimilarityFuture = std::async(std::launch::async,
//				[=, this]() {
//					while (!mBeatsInitialized) {}
//					auto& beats = mBeats;
//
//					essentia::infoLevelActive = false;
//					essentia::warningLevelActive = false;
//
//					auto& factory = essentia::streaming::AlgorithmFactory::instance();
//
//					Timer::Stopwatch stopwatch;
//
//					const size_t padding = sampleRate / 2;
//					const auto lengthInFrames = originalLength();
//					const size_t beatsCount = beats.size();
//#ifdef ALLOMERE_DEBUG
//					const size_t minGap = 2;
//					const size_t similaritySize = 1;
//					const size_t maxGap = minGap + similaritySize;
//#else
//					const size_t minGap = 15;
//					const size_t similaritySize = 100;
//					const size_t maxGap = minGap + similaritySize;
//#endif // ALLOMERE_DEBUG
//
//					stopwatch.Start();
//
//					std::vector<size_t>::iterator firstLargerThanPadding = std::find_if(beats.begin(), beats.end(),
//						[&](const size_t& beat) {
//							return beat > padding;
//						}
//					);
//					std::vector<size_t>::reverse_iterator lastLargerThanPadding = std::find_if(beats.rbegin(), beats.rend(),
//						[&](const size_t& beat) {
//							return (lengthInFrames - beat) > padding;
//						}
//					);
//					std::vector<essentia::Real> startTimes(beats.size(), 0);
//					std::vector<essentia::Real> endTimes(beats.size());
//
//					auto startBegin = startTimes.begin();
//					auto startActual = startBegin + std::distance(beats.begin(), firstLargerThanPadding);
//					//std::fill(startBegin, startActual, 0);
//					std::transform(firstLargerThanPadding, beats.end(), startActual,
//						[&](size_t beat) {
//							return beat - padding;
//						}
//					);
//
//					auto endBegin = endTimes.rbegin();
//					auto endActual = endBegin + std::distance(beats.rbegin(), lastLargerThanPadding);
//					std::fill(endBegin, endActual, lengthInFrames);
//					std::transform(lastLargerThanPadding, beats.rend(), endActual,
//						[&](size_t beat) {
//							return beat + padding;
//						}
//					);
//
//					stopwatch.Lap();
//
//					auto* sMonoLoader = factory.create("MonoLoader", "filename", pFilePath, "sampleRate", sampleRate);
//					auto* sSlicer = factory.create("Slicer", "startTimes", startTimes, "endTimes", endTimes, "timeUnits", "samples");
//
//					std::vector<std::vector<essentia::Real>> slices;
//					auto* slicesOutput = new essentia::streaming::VectorOutput<std::vector<essentia::Real>>();
//
//					sMonoLoader->output("audio") >> sSlicer->input("audio");
//					sSlicer->output("frame") >> *slicesOutput;
//
//					auto* sliceNetwork = new essentia::scheduler::Network(sMonoLoader);
//
//					slicesOutput->setVector(&slices);
//
//					sliceNetwork->run();
//
//					delete sliceNetwork;
//
//					stopwatch.Lap();
//
//					// Add silence to start of initial slices that are not of size (padding * 2)
//					// And silence to the end of last slices that are not of size (padding * 2)
//
//					std::vector<std::vector<essentia::Real>>::iterator firstSliceEqualToSize = std::find_if(slices.begin(), slices.end(),
//						[padding](const std::vector<essentia::Real>& slice) {
//							return slice.size() == padding * 2;
//						}
//					);
//					std::vector<std::vector<essentia::Real>>::reverse_iterator lastSliceEqualToSize = std::find_if(slices.rbegin(), slices.rend(),
//						[padding](const std::vector<essentia::Real>& slice) {
//							return slice.size() == padding * 2;
//						}
//					);
//
//					std::transform(slices.begin(), firstSliceEqualToSize, slices.begin(),
//						[padding](std::vector<essentia::Real>& slice) {
//							slice.insert(slice.begin(), (padding * 2) - slice.size(), 0);
//							return slice;
//						}
//					);
//					std::transform(slices.rbegin(), lastSliceEqualToSize, slices.rbegin(),
//						[padding](std::vector<essentia::Real>& slice) {
//							slice.insert(slice.end(), (padding * 2) - slice.size(), 0);
//							return slice;
//						}
//					);
//
//					stopwatch.Lap();
//					ALLOMERE_CORE_INFO("Slices Complete");
//
//					std::vector<std::vector<std::vector<float>>> featuresSlices(slices.size());
//					{
//#ifdef ESSENTIA_FEATURE_SPECTRUM
//						auto* sliceInput = new essentia::streaming::VectorInput<essentia::Real>();
//						auto* spectrumOutput = new essentia::streaming::VectorOutput<std::vector<essentia::Real>>();
//
//						auto* sFrameCutter = factory.create("FrameCutter", "frameSize", 256, "hopSize", 128, "silentFrames", "keep", "startFromZero", true);
//						auto* sWindowing = factory.create("Windowing", "type", "hann", "size", 256);
//						auto* sSpectrum = factory.create("Spectrum");
//						//auto* sSpectralPeaks = factory.create("SpectralPeaks", "sampleRate", sampleRate);
//						//auto* sSpectralWhitening = factory.create("SpectralWhitening", "maxFrequency", 3500, "sampleRate", sampleRate);
//						//auto* sHPCP = factory.create("HPCP", "sampleRate", sampleRate, "minFrequency", 100, "maxFrequency", 3500, "size", 12);
//
//						*sliceInput >> sFrameCutter->input("signal");
//						sFrameCutter->output("frame") >> sWindowing->input("frame");
//						sWindowing->output("frame") >> sSpectrum->input("frame");
//						sSpectrum->output("spectrum") >> *spectrumOutput;
//
//						auto* spectrumNetwork = new essentia::scheduler::Network(sliceInput);
//
//						std::vector<std::vector<essentia::Real>> spectrum;
//
//						std::transform(slices.begin(), slices.end(), featuresSlices.begin(),
//							[&](const std::vector<essentia::Real>& slice) {
//
//								spectrum.clear();
//								spectrumOutput->setVector(&spectrum);
//								sliceInput->setVector(&slice);
//								spectrumNetwork->run();
//								spectrumNetwork->reset();
//
//								//delete hpcpNetwork;
//								return spectrum;
//							}
//						);
//						delete spectrumNetwork;
//#endif
//#ifdef ESSENTIA_FEATURE_HPCP
//						auto* sliceInput = new essentia::streaming::VectorInput<essentia::Real>();
//						auto* hpcpOutput = new essentia::streaming::VectorOutput<std::vector<essentia::Real>>();
//
//						auto* sFrameCutter = factory.create("FrameCutter", "frameSize", 1024, "hopSize", 512, "silentFrames", "keep", "startFromZero", true);
//						auto* sWindowing = factory.create("Windowing", "type", "blackmanharris62");
//						auto* sSpectrum = factory.create("Spectrum");
//						auto* sSpectralPeaks = factory.create("SpectralPeaks", "sampleRate", sampleRate);
//						auto* sSpectralWhitening = factory.create("SpectralWhitening", "maxFrequency", 3500, "sampleRate", sampleRate);
//						auto* sHPCP = factory.create("HPCP", "sampleRate", sampleRate, "minFrequency", 100, "maxFrequency", 3500, "size", 12);
//
//
//						*sliceInput >> sFrameCutter->input("signal");
//						sFrameCutter->output("frame") >> sWindowing->input("frame");
//						sWindowing->output("frame") >> sSpectrum->input("frame");
//						sSpectrum->output("spectrum") >> sSpectralPeaks->input("spectrum");
//						sSpectrum->output("spectrum") >> sSpectralWhitening->input("spectrum");
//						sSpectralPeaks->output("magnitudes") >> sSpectralWhitening->input("magnitudes");
//						sSpectralPeaks->output("frequencies") >> sSpectralWhitening->input("frequencies");
//						sSpectralPeaks->output("frequencies") >> sHPCP->input("frequencies");
//						sSpectralWhitening->output("magnitudes") >> sHPCP->input("magnitudes");
//						sHPCP->output("hpcp") >> *hpcpOutput;
//
//						auto* hpcpNetwork = new essentia::scheduler::Network(sliceInput);
//
//						std::vector<std::vector<essentia::Real>> hpcp;
//
//						std::transform(slices.begin(), slices.end(), featuresSlices.begin(),
//							[&](const std::vector<essentia::Real>& slice) {
//
//								hpcp.clear();
//								hpcpOutput->setVector(&hpcp);
//								sliceInput->setVector(&slice);
//								hpcpNetwork->run();
//								hpcpNetwork->reset();
//
//								//delete hpcpNetwork;
//								return hpcp;
//							}
//						);
//						delete hpcpNetwork;
//#endif
//					}
//
//					stopwatch.Lap();
//					ALLOMERE_CORE_INFO("Features Complete");
//
//
//
//#ifdef ALLOMERE_SIMILARITY_MSE
//
//
//
//#endif // ALLOMERE_SIMILARITY_MSE
//
//					auto* simMat = new Similarity[beatsCount * beatsCount];
//					auto* simMatSorted = new Similarity[beatsCount * beatsCount];
//
//					{
//						auto references = std::ranges::views::iota(minGap, beatsCount);
//
//#ifdef ESSENTIA_SIMILARITY_CSS
//						std::vector<float> distances;
//						std::vector<TNT::Array2D<float>> scoreMatrix;
//
//						std::for_each(references.begin(), references.end(),
//							[&, maxGap, minGap](size_t reference) {
//								auto& referenceHPCP = hpcpSlices[reference];
//
//								auto* queryInput = new essentia::streaming::VectorInput<std::vector<essentia::Real>>();
//								auto* distanceOutput = new essentia::streaming::VectorOutput<essentia::Real>();
//								auto* scoreMatrixOutput = new essentia::streaming::VectorOutput<TNT::Array2D<essentia::Real>>();
//
//								auto* sCCS = factory.create("ChromaCrossSimilarity", "oti", 0, "referenceFeature", referenceHPCP);
//								auto* sCSS = factory.create("CoverSongSimilarity", "pipeDistance", true);
//
//								*queryInput >> sCCS->input("queryFeature");
//								sCCS->output("csm") >> sCSS->input("inputArray");
//								sCSS->output("scoreMatrix") >> *scoreMatrixOutput;
//								sCSS->output("distance") >> *distanceOutput;
//
//								auto* similarityNetwork = new essentia::scheduler::Network(queryInput);
//
//								auto queries = std::ranges::views::iota(std::max<long long>(0, (long long)(reference + 1) - (long long)maxGap), (long long)(reference + 1) - (long long)minGap);
//
//								std::for_each(queries.begin(), queries.end(),
//									[&, reference, beatsCount](size_t query) {
//
//										auto& similarity = simMat[reference * beatsCount + query];
//
//										queryInput->setVector(&(hpcpSlices[query]));
//										distanceOutput->setVector(&(distances));
//										scoreMatrixOutput->setVector(&(scoreMatrix));
//										similarityNetwork->run();
//										similarityNetwork->reset();
//
//										similarity.calculated = true;
//										similarity.distance = distances.back();
//										similarity.reference = reference;
//										similarity.query = query;
//										distances.clear();
//										scoreMatrix.clear();
//									}
//								);
//
//								delete similarityNetwork;
//								//ALLOMERE_CORE_INFO("Reference {0}", reference);
//							}
//						);
//#elif ALLOMERE_SIMILARITY_MSE
//
//						std::for_each(references.begin(), references.end(),
//							[&, maxGap, minGap](size_t reference) {
//								auto& referenceHPCP = featuresSlices[reference];
//
//								Eigen::MatrixXf referenceMat(referenceHPCP.size(), referenceHPCP[0].size());
//								for (int i = 0; i < referenceHPCP.size(); i++)
//									referenceMat.row(i) = Eigen::VectorXf::Map(&referenceHPCP[i][0], referenceHPCP[i].size());
//
//								auto queries = std::ranges::views::iota(std::max<long long>(0, (long long)(reference + 1) - (long long)maxGap), (long long)(reference + 1) - (long long)minGap);
//
//								std::for_each(queries.begin(), queries.end(),
//									[&, reference, beatsCount](size_t query) {
//										auto& similarity = simMat[reference * beatsCount + query];
//
//										auto& queryHPCP = featuresSlices[query];
//
//										Eigen::MatrixXf queryMat(queryHPCP.size(), queryHPCP[0].size());
//										for (int i = 0; i < referenceHPCP.size(); i++)
//											queryMat.row(i) = Eigen::VectorXf::Map(&queryHPCP[i][0], queryHPCP[i].size());
//
//										auto distance = (referenceMat - queryMat).array().square().mean();
//
//										similarity.calculated = true;
//										similarity.distance = distance;
//										similarity.reference = reference;
//										similarity.query = query;
//									}
//								);
//
//								//ALLOMERE_CORE_INFO("Reference {0}", reference);
//							}
//						);
//
//#endif // ALLOMERE_SIMILARITY_CSS
//
//
//					}
//
//					std::copy(simMat, simMat + beatsCount * beatsCount, simMatSorted);
//					std::sort(simMatSorted, simMatSorted + beatsCount * beatsCount);
//
//					similarityMatrix = simMat;
//					similarityMatrixSorted = simMatSorted;
//					//essentia::warningLevelActive = true;
//
//					auto firstUncalculated = std::find_if(similarityMatrixSorted, similarityMatrixSorted + beatsCount * beatsCount,
//						[](Similarity& sim) {
//							return !sim.calculated;
//						}
//					);
//
//					stopwatch.Lap();
//
//					ALLOMERE_CORE_INFO("TotalDur: {0} ms Count: {1} Sim: {2} ms", stopwatch.FromStart().milliseconds(), std::distance(similarityMatrixSorted, firstUncalculated), stopwatch.laps().back().milliseconds());
//
//					ClipSimilarityGeneratedEvent event(mId);
//					Emit(event);
//
//					//mSimilarity = Similarity{ distance, stopwatch };
//
//					return;
//				}
//			);
		}

		Clip::~Clip()
		{
			//ma_resource_manager_data_source_uninit(&dataSource);
			ma_resource_manager_data_source_uninit(rawDataSource.get());
			std::for_each(mDataSources.begin(), mDataSources.end(),
				[](std::shared_ptr<ma_resource_manager_data_source>& dataSource) {
					ma_resource_manager_data_source_uninit(dataSource.get());
				}
			);
		}

		void Clip::SetupSubscriptions()
		{

		}

		ma_result Clip::read(void* pFramesOut, ma_uint64 frameCount, ma_uint64* pFramesRead)
		{
			return ma_data_source_read_pcm_frames(mDataSources.front().get(), pFramesOut, frameCount, pFramesRead);
		}

		ma_result Clip::getFrames(void* pFramesOut, ma_uint64 startFrame, ma_uint64 endFrame, ma_uint64* framesRead)
		{
			int result = MA_SUCCESS;
			if (startFrame > endFrame)
			{
				return MA_INVALID_OPERATION;
			}
			ma_uint64 frameCount{ endFrame - startFrame };
			result |= ma_data_source_seek_to_pcm_frame(&rawDataSource, startFrame);
			result |= ma_data_source_read_pcm_frames(&rawDataSource, pFramesOut, frameCount, framesRead);
			return (ma_result)result;
		}

		ma_result Clip::seek(size_t frame)
		{
			// If is chained, look through entire chain and get the first data source that includes the frame
			auto* ds = mDataSources.front().get();
			//ALLOMERE_CORE_INFO("Current: {0} Next: {1}", ds->backend.buffer.ds.pCurrent, ds->backend.buffer.ds.pNext);
			int result = MA_SUCCESS;
			if (ds->backend.buffer.ds.pNext) {
				bool found{ false };
				ma_uint64 totalLength{ 0 };
				while (!found && ds)
				{
					ma_uint64 length;
					ma_data_source_get_length_in_pcm_frames(ds, &length);
					if (ma_data_source_is_looping(ds)) {
						auto nOfLoops = (frame - totalLength) / length;
						totalLength += (length * nOfLoops);
					}
					if (frame <= totalLength + length)
					{
						result |= ma_data_source_seek_to_pcm_frame(ds, frame - totalLength);
						result |= ma_data_source_set_current(mDataSources.front().get(), ds);
						ma_uint64 cursor;
						ma_data_source_get_cursor_in_pcm_frames(ds, &cursor);
						found = true;
					}
					else
					{
						totalLength += length;
						ds = (ma_resource_manager_data_source*)(ds->backend.buffer.ds.pNext);
					}
				}

				if (!found) {
					return MA_DOES_NOT_EXIST;
				}
			}
			else
			{
				result |= ma_data_source_seek_to_pcm_frame(ds, frame);
			}

			return (ma_result)result;
		}

		/*const std::vector<size_t>& Clip::beats() const
		{
			if (!mBeatsInitialized)
			{
				if (mBeatsFuture.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
				{
					return mBeats;
				};
			}
			mSimilarityFuture.wait_for(std::chrono::milliseconds(0));
			return mBeats;
		}*/

		ma_result Clip::cut(ma_uint64 cutStartFrame, ma_uint64 cutEndFrame)
		{
			const auto result = ma_data_source_set_range_in_pcm_frames(mDataSources.front().get(), cutStartFrame, cutEndFrame);
			ClipCutEvent event(mId);
			Emit(event);
			return result;
		}

		ma_result Clip::bridge(ma_uint64 startFrame, ma_uint64 endFrame, bool loop)
		{
			auto& timeline = pTrack.lock()->timeline();
			auto range = getRange();
			ma_uint64 rangeStart = range.first, rangeEnd = range.second;

			std::string filePath;
			{
				const auto context = pContext.lock()->read();
				filePath = context->filePath;
			}

			int result = MA_SUCCESS;

			if (startFrame == std::clamp(startFrame, rangeStart, rangeEnd) &&
				endFrame == std::clamp(endFrame, rangeStart, rangeEnd))
			{
				ma_resource_manager_data_source* source = mDataSources.front().get();

				auto sourceIt = std::find_if(mDataSources.begin(), mDataSources.end(),
					[startFrame](std::shared_ptr<ma_resource_manager_data_source>& source) {
						ma_uint64 rangeStart, rangeEnd;
						ma_data_source_get_range_in_pcm_frames(source.get(), &rangeStart, &rangeEnd);
						return startFrame == std::clamp(startFrame, rangeStart, rangeEnd);
					}
				);
				auto endIt = std::find_if(mDataSources.begin(), mDataSources.end(),
					[endFrame](std::shared_ptr<ma_resource_manager_data_source>& source) {
						ma_uint64 rangeStart, rangeEnd;
						ma_data_source_get_range_in_pcm_frames(source.get(), &rangeStart, &rangeEnd);
						return endFrame == std::clamp(endFrame, rangeStart, rangeEnd);
					}
				);

				if (sourceIt != mDataSources.end() && endIt != mDataSources.end()) {
					source = sourceIt->get();
				}
				else {
					return MA_INVALID_OPERATION;
				}

				if (startFrame < endFrame)
				{
					ma_resource_manager_data_source* landingDataSource = nullptr;

					ma_uint64 sourceRangeStart, sourceRangeEnd;
					ma_data_source_get_range_in_pcm_frames(source, &sourceRangeStart, &sourceRangeEnd);
					if (sourceRangeEnd == SIZE_MAX)
					{
						ma_data_source_get_length_in_pcm_frames(source, &sourceRangeEnd);
						sourceRangeEnd += sourceRangeStart;
					}

					if (sourceRangeStart == startFrame) {
						result |= ma_data_source_set_range_in_pcm_frames(source, endFrame, sourceRangeEnd);
					}
					else if (endFrame == sourceRangeEnd)
					{
						result |= ma_data_source_set_range_in_pcm_frames(source, sourceRangeStart, startFrame);
					}
					else {
						auto pLanding = timeline.load(filePath.c_str());
						auto landingIt = mDataSources.insert(sourceIt + 1, pLanding);
						landingDataSource = pLanding.get();

						/*result |= ma_resource_manager_data_source_init(
							&timeline.maResourceManager,
							filePath.c_str(),
							MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_WAIT_INIT,
							NULL,
							landingDataSource
						);*/

						result |= ma_data_source_set_range_in_pcm_frames(landingDataSource, endFrame, sourceRangeEnd);

						result |= ma_data_source_set_range_in_pcm_frames(source, sourceRangeStart, startFrame);

						result |= ma_data_source_set_next(source, landingDataSource);

						if (landingIt != mDataSources.end() - 1)
						{
							auto nextIt = landingIt + 1;
							auto* pNext = (*nextIt).get();
							ma_uint64 nextRangeStart, nextRangeEnd;

							ma_data_source_get_range_in_pcm_frames(pNext, &nextRangeStart, &nextRangeEnd);
							//result |= ma_data_source_set_range_in_pcm_frames(landingDataSource, endFrame, nextRangeStart);

							result |= ma_data_source_set_next(landingDataSource, pNext);
						}
					}
				}
				else
				{
					if (loop)
					{
						auto pBridge = timeline.load(filePath.c_str());
						auto pLanding = timeline.load(filePath.c_str());
						mDataSources.insert(mDataSources.end(), { pBridge, pLanding });
						ma_resource_manager_data_source* bridgeLoopDataSource = pBridge.get();
						ma_resource_manager_data_source* landingDataSource = pLanding.get();

						/*result |= ma_resource_manager_data_source_init(
							&timeline.maResourceManager,
							filePath.c_str(),
							MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_WAIT_INIT,
							NULL,
							bridgeLoopDataSource
						);
						result |= ma_resource_manager_data_source_init(
							&timeline.maResourceManager,
							filePath.c_str(),
							MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_WAIT_INIT,
							NULL,
							landingDataSource
						);*/

						result |= ma_data_source_set_range_in_pcm_frames(source, rangeStart, startFrame);
						result |= ma_data_source_set_range_in_pcm_frames(bridgeLoopDataSource, endFrame, startFrame);
						result |= ma_data_source_set_range_in_pcm_frames(landingDataSource, startFrame, rangeEnd);

						result |= ma_data_source_set_looping(bridgeLoopDataSource, true);

						result |= ma_data_source_set_next(source, bridgeLoopDataSource);
						result |= ma_data_source_set_next(bridgeLoopDataSource, bridgeLoopDataSource);
					}
					else {
						auto pBridge = timeline.load(filePath.c_str());
						auto pLanding = timeline.load(filePath.c_str());
						auto bridgeIt = mDataSources.insert(sourceIt + 1, { pBridge, pLanding });
						auto landingIt = bridgeIt + 1;
						ma_resource_manager_data_source* bridgeDataSource = (*bridgeIt).get();
						ma_resource_manager_data_source* landingDataSource = (*landingIt).get();

						/*result |= ma_resource_manager_data_source_init(
							&timeline.maResourceManager,
							filePath.c_str(),
							MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_WAIT_INIT,
							NULL,
							bridgeDataSource
						);
						result |= ma_resource_manager_data_source_init(
							&timeline.maResourceManager,
							filePath.c_str(),
							MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_WAIT_INIT,
							NULL,
							landingDataSource
						);*/

						ma_uint64 sourceRangeStart, sourceRangeEnd;
						ma_data_source_get_range_in_pcm_frames(source, &sourceRangeStart, &sourceRangeEnd);
						if (sourceRangeEnd == SIZE_MAX)
						{
							ma_data_source_get_length_in_pcm_frames(source, &sourceRangeEnd);
							sourceRangeEnd += sourceRangeStart;
						}

						result |= ma_data_source_set_range_in_pcm_frames(source, sourceRangeStart, startFrame);
						result |= ma_data_source_set_range_in_pcm_frames(bridgeDataSource, endFrame, startFrame);
						result |= ma_data_source_set_range_in_pcm_frames(landingDataSource, startFrame, sourceRangeEnd);


						result |= ma_data_source_set_next(source, bridgeDataSource);
						result |= ma_data_source_set_next(bridgeDataSource, landingDataSource);

						if (landingIt != mDataSources.end() - 1)
						{
							auto nextIt = landingIt + 1;
							auto* pNext = (*nextIt).get();
							ma_uint64 nextRangeStart, nextRangeEnd;

							ma_data_source_get_range_in_pcm_frames(pNext, &nextRangeStart, &nextRangeEnd);
							result |= ma_data_source_set_range_in_pcm_frames(landingDataSource, endFrame, nextRangeStart);

							result |= ma_data_source_set_next(landingDataSource, pNext);
						}
					}
				}

				ClipBridgedEvent event(mId);
				Emit(event);
			}
			else
			{
				return MA_INVALID_OPERATION;
			}
			return (ma_result)result;
		}

		/*ma_result Clip::bridgeBeat(ma_uint64 fromBeat, ma_uint64 toBeat, bool loop)
		{
			if (mBeatsInitialized)
			{
				return bridge(mBeats[fromBeat], mBeats[toBeat], loop);
			}
			return MA_DOES_NOT_EXIST;
		}*/


		std::pair<ma_uint64, ma_uint64> Clip::getCut() const {
			ma_uint64 cutStartFrame, cutEndFrame;
			ma_data_source_get_range_in_pcm_frames(mDataSources.front().get(), &cutStartFrame, &cutEndFrame);
			return std::make_pair(cutStartFrame, cutEndFrame);
		}

		std::pair<ma_uint64, ma_uint64> Clip::getRange() {
			ma_uint64 startFrame, endFrame;
			auto first = mDataSources.begin(), last = mDataSources.end() - 1;

			if (first == last) {
				ma_data_source_get_range_in_pcm_frames(first->get(), &startFrame, &endFrame);
				if (endFrame == SIZE_MAX)
				{
					ma_data_source_get_length_in_pcm_frames(first->get(), &endFrame);
				}
			}
			else
			{
				ma_data_source_get_range_in_pcm_frames(first->get(), &startFrame, NULL);
				ma_data_source_get_range_in_pcm_frames(last->get(), NULL, &endFrame);
			}

			return std::make_pair(startFrame, endFrame);
		}

		// Lengths
		// 
		// OriginalLength -> Length of the Data Source before and modification
		// ActualLength   -> Length of the Data Source through all transitions. If there is a loop, then is SIZE_MAX
		// 


		ma_uint64 Clip::originalLength()
		{
			ma_uint64 length;
			ma_data_source_get_length_in_pcm_frames(rawDataSource.get(), &length);
			return length;
		}

		ma_uint64 Clip::length()
		{
			//ma_uint64 length;
			//ma_data_source_get_length_in_pcm_frames(&dataSource, &length);
			size_t length{ 0 };
			for (auto& dataSource : mDataSources) {
				if (!ma_data_source_is_looping(dataSource.get()))
				{
					size_t currLength{ 0 };
					ma_data_source_get_length_in_pcm_frames(dataSource.get(), &currLength);
					length += currLength;
				}
				else {
					return SIZE_MAX;
				}
			}

			return length;
		}
	}
}