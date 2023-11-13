#include "allomerepch.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "Audio.h"

namespace Allomere {
	namespace Audio {

		Audio Audio::s_Instance;

		Audio::Audio()
		{

			if (ma_context_init(NULL, 0, NULL, &maContext) != MA_SUCCESS) {
				ALLOMERE_CORE_ERROR("Failed to initialize context.\n");
				return;
			}

			//maResourceManagerConfig = ma_resource_manager_config_init();
			//maResourceManagerConfig.decodedFormat = maDeviceConfig.playback.format;
			//maResourceManagerConfig.decodedChannels = maDeviceConfig.playback.channels;
			//maResourceManagerConfig.decodedSampleRate = maDeviceConfig.sampleRate;


			//if (ma_resource_manager_init(&maResourceManagerConfig, &maResourceManager) != MA_SUCCESS) {
			//	ALLOMERE_CORE_ERROR("Failed to initialize resource manager.\n");
			//	return;
			//}

		}

		void Audio::SetupSubscriptions()
		{

		}

		Audio::~Audio()
		{
			//ma_resource_manager_uninit(&maResourceManager);
			ma_context_uninit(&maContext);
			//essentia::shutdown();
		};

		/*std::shared_ptr<ma_resource_manager_data_source> Audio::Load(const char* pFilePath, ma_resource_manager_data_source_flags flags)
		{
			auto& audio = Audio::Get();
			std::shared_ptr<ma_resource_manager_data_source> dataSource(new ma_resource_manager_data_source);

			ma_resource_manager_data_source_init(
				&audio.maResourceManager,
				pFilePath,
				flags,
				NULL,
				dataSource.get());

			return dataSource;
		}*/

		std::vector<ma_device_info> Audio::GetDevices()
		{
			auto& audio = Audio::Get();

			ma_device_info* pPlaybackInfos;
			ma_uint32 playbackCount;
			ma_device_info* pCaptureInfos;
			ma_uint32 captureCount;

			if (
				ma_context_get_devices(
					&audio.maContext,
					&pPlaybackInfos,
					&playbackCount,
					&pCaptureInfos,
					&captureCount
				) != MA_SUCCESS) {
				ALLOMERE_CORE_ERROR("Failed to get devices.\n");
				// Error.
			}

			return std::vector<ma_device_info>(pPlaybackInfos, pPlaybackInfos + playbackCount);

		}

		std::weak_ptr<Device> Audio::GetDevice(const wchar_t* id)
		{
			auto& audio = Get();
			if (id == nullptr) {
				id = L"DEFAULT";
			}
			auto val = audio.mDevices.find(std::wstring(id));
			if (val == audio.mDevices.end())
			{
				auto devices = GetDevices();
				if (wcscmp(id, L"DEFAULT") == 0)
				{
					return audio.mDevices.emplace(std::make_pair(std::wstring(id), new Device)).first->second;
				}
				auto device = std::find_if(devices.begin(), devices.end(),
					[&](const ma_device_info& info) {
						return wcscmp(id, info.id.wasapi) == 0;
					}
				);
				if (device != devices.end())
				{
					return audio.mDevices.emplace(std::make_pair(std::wstring(id), new Device(&device->id))).first->second;
				}
			}
			return val->second;
		}
	}
}