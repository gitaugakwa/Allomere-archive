#pragma once

#include "core/Buffer.h"

namespace Allomere
{

	class FileSystem
	{
	public:
		// TODO: move to FileSystem class
		static Buffer ReadFileBinary(const std::filesystem::path &filepath);
	};

}
