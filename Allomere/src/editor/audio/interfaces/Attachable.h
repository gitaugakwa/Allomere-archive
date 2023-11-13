#pragma once

#include "miniaudio.h"

#include "Playable.h"

namespace Allomere {
	class Device;

	class Attachable: public Playable {
	public:
		virtual ma_result setDevice(std::weak_ptr<Device> device) {
			pDevice = device;
			return MA_SUCCESS;
		};

		virtual std::shared_ptr<ma_resource_manager_data_source> load(const char* pFilePath, ma_resource_manager_data_source_flags flags = MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_WAIT_INIT) const = 0;

		ma_result silence(void* pFramesOut, ma_uint64 frameCount, ma_uint64* pFramesRead);
		

	protected:
		std::weak_ptr<Device> pDevice;

	};
}