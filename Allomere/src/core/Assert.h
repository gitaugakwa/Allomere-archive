#pragma once

#include "core/Base.h"
#include "core/Log.h"
#include <filesystem>

#ifdef ALLOMERE_ENABLE_ASSERTS

// Alteratively we could use the same "default" message for both "WITH_MSG" and "NO_MSG" and
// provide support for custom formatting by concatenating the formatting string instead of having the format inside the default message
#define ALLOMERE_INTERNAL_ASSERT_IMPL(type, check, msg, ...) { if(!(check)) { ALLOMERE##type##ERROR(msg, __VA_ARGS__); ALLOMERE_DEBUGBREAK(); } }
#define ALLOMERE_INTERNAL_ASSERT_WITH_MSG(type, check, ...) ALLOMERE_INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
#define ALLOMERE_INTERNAL_ASSERT_NO_MSG(type, check) ALLOMERE_INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", ALLOMERE_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)

#define ALLOMERE_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
#define ALLOMERE_INTERNAL_ASSERT_GET_MACRO(...) ALLOMERE_EXPAND_MACRO( ALLOMERE_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, ALLOMERE_INTERNAL_ASSERT_WITH_MSG, ALLOMERE_INTERNAL_ASSERT_NO_MSG) )

// Currently accepts at least the condition and one additional parameter (the message) being optional
#define ALLOMERE_ASSERT(...) ALLOMERE_EXPAND_MACRO( ALLOMERE_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__) )
#define ALLOMERE_CORE_ASSERT(...) ALLOMERE_EXPAND_MACRO( ALLOMERE_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_CORE_, __VA_ARGS__) )
#else
#define ALLOMERE_ASSERT(...)
#define ALLOMERE_CORE_ASSERT(...)
#endif