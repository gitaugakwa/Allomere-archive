#include "allomerepch.h"

#include "Attachable.h"

#include "editor/audio/Device.h"

namespace Allomere {

	ma_result Attachable::silence(void* pFramesOut, ma_uint64 frameCount, ma_uint64* pFramesRead)
	{
		{
			// not sure, you might have to zero the pOutput memory here
			// this is untested, but it would be something along the lines of: 
			auto& device = *pDevice.lock();
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
}