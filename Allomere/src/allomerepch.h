#pragma once

#include "core/PlatformDetection.h"

#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>
#include <variant>

#include <map>
#include <exception>
#include <execution>
#include <string>
#include <sstream>
#include <fstream>
#include <cmath>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <chrono>
#include <vector>
#include <random>
#include <limits>
#include <mutex>
#include <future>
#include <numeric>
#include <ranges>
#include <concepts>
#include <coroutine>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>

// Eigen
#pragma warning(disable : 5054) // Disable Warning C5054: operator '&': deprecated between enumerations of different types
#include "Eigen/Core"
#include "unsupported/Eigen/Splines"
// #include "Eigen/StdVector"
// #pragma warning( disable : 4554 ) // Disable Warning C4554: '&': check operator precedence for possible error; use parentheses to clarify precedence
// #include "unsupported/Eigen/CXX11/Tensor"
// #pragma warning( default : 4554 ) // Reenable Warning C4554: '&': check operator precedence for possible error; use parentheses to clarify precedence
#pragma warning(default : 5054) // Reenable Warning C5054: operator '&': deprecated between enumerations of different types

//	---oneAPI---------------
// ThreadPool
// #include "oneapi/tbb.h"

// MKL
// #include "mkl.h"
//	---oneAPI---------------

// #include <D3d12.h>

// Allomere+Neural


// Base
#include "core/Base.h"

// Log
#include "core/Log.h"

// // Random
// #include "Random/Random.h"

// // Activation
// #include "Activation/Activation.h"

// // Loss
// #include "Loss/Loss.h"

// Timer
//#include "timer.h"

// Miniaudio
#include "miniaudio.h"

#include "debug/Instrumentor.h"

// JSON
#include "nlohmann/json.hpp"

#ifdef ALLOMERE_PLATFORM_WINDOWS
#include <Windows.h>
#endif // ALLOMERE_PLATFORM_WINDOWS
