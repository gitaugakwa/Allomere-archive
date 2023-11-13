#pragma once

#include "miniaudio.h"


namespace Allomere {

	class Playable {
	public:

		virtual ma_result read(void* pFramesOut, ma_uint64 frameCount, ma_uint64* pFramesRead) = 0;
		virtual ma_result seek(size_t frame) = 0;

	private:
		
	};

}