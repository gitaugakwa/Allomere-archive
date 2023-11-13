#pragma once

#include "events/Event.h"

#include "editor/audio/interfaces/Playable.h"
#include "Device.h"

#include "miniaudio.h"


namespace Allomere {
	namespace Audio {

		class Audio : public EventEmmiter {
		public:
			Audio(const Audio&) = delete;

			static Audio& Get() { return s_Instance; }
			//static ma_resource_manager& GetResourceManager() { return Get().maResourceManager; }

			//static std::shared_ptr<ma_resource_manager_data_source> Load(const char* pFilePath, ma_resource_manager_data_source_flags flags = MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_WAIT_INIT);

			static std::vector<ma_device_info> GetDevices();
			static std::weak_ptr<Device> GetDevice(const wchar_t* id = nullptr);

		private:
			Audio();
			~Audio();

			void SetupSubscriptions();

		private:

			ma_context maContext;

			//ma_resource_manager maResourceManager;
			//ma_resource_manager_config maResourceManagerConfig;

			static std::atomic<size_t> time;
			static std::atomic<bool> playing;

			std::map<std::wstring, std::shared_ptr<Device>> mDevices;

			static Audio s_Instance;

		};
	}
}