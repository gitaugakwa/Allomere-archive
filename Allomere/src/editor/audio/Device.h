#pragma once

#include "editor/audio/interfaces/Attachable.h"

namespace Allomere {

	class Device: public std::enable_shared_from_this<Device>
	{
	public:
		Device(const ma_device_id* id = nullptr);
		~Device();

		void attach(std::weak_ptr<Attachable> playable);

		ma_uint32 sampleRate() const { return maDeviceConfig.sampleRate; }
		ma_uint32 channels() const { return maDeviceConfig.playback.channels; }
		ma_format format() const { return maDeviceConfig.playback.format; }

		ma_resource_manager& resourceManager() { return maResourceManager; }

	private:
		static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
		static ma_uint64 read_and_mix_pcm_frames_f32(ma_device* playables, float* pOutputF32, ma_uint64 frameCount);

		static ma_result silence(void* pFramesOut, Device* pDevice, ma_uint64 frameCount, ma_uint64* pFramesRead);

	private:
		std::array<float, 1 << 12> readBuffer; // 4096

		ma_resource_manager maResourceManager;
		ma_resource_manager_config maResourceManagerConfig;

		ma_device_config maDeviceConfig;
		ma_device maDevice;

		std::vector<std::weak_ptr<Attachable>> mPlayables;
	};
}