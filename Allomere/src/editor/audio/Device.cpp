#include "allomerepch.h"
#include "Device.h"

namespace Allomere {

	Device::Device(const ma_device_id* id)
	{
		// If devices have the same configs, use the same resource manager
		maDeviceConfig = ma_device_config_init(ma_device_type_playback);
		maDeviceConfig.playback.pDeviceID = id;
		maDeviceConfig.playback.format = ma_format_f32;
		maDeviceConfig.playback.channels = 2;
		maDeviceConfig.sampleRate = 44100;
		maDeviceConfig.dataCallback = data_callback;
		maDeviceConfig.pUserData = this; // timelines

		maResourceManagerConfig = ma_resource_manager_config_init();

		maResourceManagerConfig.decodedFormat = maDeviceConfig.playback.format;
		maResourceManagerConfig.decodedChannels = maDeviceConfig.playback.channels;
		maResourceManagerConfig.decodedSampleRate = maDeviceConfig.sampleRate;

		if (ma_resource_manager_init(&maResourceManagerConfig, &maResourceManager) != MA_SUCCESS) {
			ALLOMERE_CORE_ERROR("Failed to initialize resource manager.\n");
			return;
		}

		if (ma_device_init(NULL, &maDeviceConfig, &maDevice) != MA_SUCCESS) {
			ALLOMERE_CORE_ERROR("Failed to open playback device.\n");
			ma_resource_manager_uninit(&maResourceManager);
			return;
		}

		if (ma_device_start(&maDevice) != MA_SUCCESS) {
			ALLOMERE_CORE_ERROR("Failed to start playback device.\n");
			ma_resource_manager_uninit(&maResourceManager);
			ma_device_uninit(&maDevice);
			return;
		}
	}

	Device::~Device()
	{
		ma_resource_manager_uninit(&maResourceManager);
		ma_device_uninit(&maDevice);
	}

	void Device::attach(std::weak_ptr<Attachable> playable)
	{
		playable.lock()->setDevice(weak_from_this());
		mPlayables.push_back(playable);
	}

	void Device::data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
	{
		pInput;
		Device* device = (Device*)pDevice->pUserData;

		if (device->mPlayables.size() == 0) {
			return;
		}

		float* pOutputF32 = (float*)pOutput;
		read_and_mix_pcm_frames_f32(pDevice, pOutputF32, frameCount);
	}

	ma_uint64 Device::read_and_mix_pcm_frames_f32(ma_device* pMaDevice, float* pOutputF32, ma_uint64 frameCount)
	{
		/*
		The way mixing works is that we just read into a temporary buffer, then take the contents of that buffer and mix it with the
		contents of the output buffer by simply adding the samples together. You could also clip the samples to -1..+1, but I'm not
		doing that in this example.
		*/
		Device* pDevice = (Device*)pMaDevice->pUserData;

		silence(pOutputF32, pDevice, frameCount, nullptr);

		Device& device = *pDevice;
		ma_uint64 channels = device.channels();
		size_t playables = device.mPlayables.size();

		ma_result result;
		float* temp = device.readBuffer.data();
		ma_uint64 tempCapInFrames = device.readBuffer.size() / channels;
		ma_uint64* playableTotalFramesRead = new ma_uint64[playables];

		for (size_t i = 0; i < playables; i++) {
			ma_uint64& totalFramesRead = playableTotalFramesRead[i] = 0;

			Playable& playable = *(device.mPlayables[i].lock());

			while (totalFramesRead < frameCount) {
				ma_uint64 iSample;
				ma_uint64 framesReadThisIteration;
				ma_uint64 totalFramesRemaining = frameCount - totalFramesRead;
				ma_uint64 framesToReadThisIteration = tempCapInFrames;
				if (framesToReadThisIteration > totalFramesRemaining) {
					framesToReadThisIteration = totalFramesRemaining;
				}

				result = playable.read(temp, framesToReadThisIteration, &framesReadThisIteration);
				//result = ma_decoder_read_pcm_frames(pDecoder, temp, framesToReadThisIteration, &framesReadThisIteration);
				if (result != MA_SUCCESS || framesReadThisIteration == 0) {
					break;
				}

				/* Mix the frames together. */
				for (iSample = 0; iSample < framesReadThisIteration * channels; ++iSample) {
					pOutputF32[totalFramesRead * channels + iSample] += temp[iSample];
				}

				totalFramesRead += framesReadThisIteration;

				if (framesReadThisIteration < framesToReadThisIteration) {
					break;  /* Reached EOF. */
				}
			}
			memset(temp, 0, sizeof(float) * device.readBuffer.size());
		}


		ma_uint64 maxTotalFramesRead = *std::max_element(playableTotalFramesRead, playableTotalFramesRead + playables);
		delete[] playableTotalFramesRead;

		return maxTotalFramesRead;
	}

	ma_result Device::silence(void* pFramesOut, Device* pDevice, ma_uint64 frameCount, ma_uint64* pFramesRead)
	{
		// not sure, you might have to zero the pOutput memory here
		// this is untested, but it would be something along the lines of: 
		auto& device = *pDevice;
		auto channels = device.channels();

		size_t len = channels * frameCount;
		switch (device.format())
		{
		case ma_format_unknown: 0; break;  // what to do??
		case ma_format_u8: memset(pFramesOut, 127, len * 1); break; // not sure
		case ma_format_s16: memset(pFramesOut, 0, len * 2); break;
		case ma_format_s24: memset(pFramesOut, 0, len * 3); break;
		case ma_format_s32: memset(pFramesOut, 0, len * 4); break;
		case ma_format_f32: memset(pFramesOut, 0, len * 4); break;
		};
		if (pFramesRead != nullptr)
		{
			(*pFramesRead) = frameCount;
		}
		return MA_SUCCESS;
	};
}