#pragma once

#include "core/PlatformDetection.h"

#include <memory>

#ifdef ALLOMERE_DEBUG
#if defined(ALLOMERE_PLATFORM_WINDOWS)
#define ALLOMERE_DEBUGBREAK() __debugbreak()
#elif defined(ALLOMERE_PLATFORM_LINUX)
#include <signal.h>
#define ALLOMERE_DEBUGBREAK() raise(SIGTRAP)
#else
#error "Platform doesn't support debugbreak yet!"
#endif
#define ALLOMERE_ENABLE_ASSERTS
#else
#define ALLOMERE_DEBUGBREAK()
#endif

#define ALLOMERE_EXPAND_MACRO(x) x
#define ALLOMERE_STRINGIFY_MACRO(x) #x

#define BIT(x) (1 << x)

#define ALLOMERE_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

namespace Allomere {

	template<typename T>
	using Scope = std::unique_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Scope<T> CreateScope(Args&& ... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	using Ref = std::shared_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Ref<T> CreateRef(Args&& ... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

}

#include "core/Log.h"
#include "core/Assert.h"