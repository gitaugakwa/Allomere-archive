workspace "Allomere"
	architecture "x86_64"
	startproject "Sandbox"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder (solution directory)
IncludeDir = {}

IncludeDir["Base"] = "./"
IncludeDir["GLFW"] = "Allomere/vendor/glfw/include"
IncludeDir["Glad"] = "Allomere/vendor/glad/include"
IncludeDir["ImGui"] = "Allomere/vendor/imgui"
IncludeDir["ImPlot"] = "Allomere/vendor/implot"
IncludeDir["glm"] = "Allomere/vendor/glm"
IncludeDir["miniaudio"] = "Allomere/vendor/miniaudio"
IncludeDir["Eigen"] = "Allomere/vendor/eigen"
IncludeDir["essentia"] = "Allomere/vendor/essentia/src"
IncludeDir["boost"] = "Allomere/vcpkg_installed/x64-windows/include"

group "Dependencies"
	-- include "vendor/premake"
	-- include "Allomere/vendor/Box2D"
	include "Allomere/vendor/glfw"
	include "Allomere/vendor/glad"
	include "Allomere/vendor/imgui"
	include "Allomere/vendor/implot"
	include "Allomere/vendor/essentia"
	-- include "Allomere/vendor/yaml-cpp"
group ""

-- Set working direcctory to where sandbox exports to or something

project "Allomere"
	location "Allomere"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	staticruntime "on"

	warnings "Extra"

	-- openmp "On"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "allomerepch.h"
	pchsource "Allomere/src/allomerepch.cpp"

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.hpp",
		"%{prj.name}/src/**.cpp",
		"Allomere/vendor/glm/glm/**.hpp",
		"Allomere/vendor/glm/glm/**.inl",
		-- "Allomere/vendor/miniaudio/miniaudio.h",
	}

	defines
	{
		"IMGUI_DEFINE_MATH_OPERATORS"
	}

	includedirs
	{
		"%{prj.name}/src",
		"Allomere/vendor/spdlog/include",
		"Allomere/vendor/json/include",
		"%{IncludeDir.miniaudio}",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.ImPlot}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.Eigen}",
		"%{IncludeDir.boost}",
		-- Essentia --
		"Allomere/vendor/essentia/src",
		"Allomere/vendor/essentia/src/algorithms",
		"Allomere/vendor/essentia/src/essentia",
		"Allomere/vendor/essentia/src/essentia/scheduler",
		"Allomere/vendor/essentia/src/essentia/streaming",
		"Allomere/vendor/essentia/src/essentia/streaming/algorithms",
		"Allomere/vendor/essentia/src/essentia/utils",
		-- Essentia --
		-- "Allomere/vendor/oneDNN/cpu_tbb/include",
		-- "Allomere/vendor/oneMKL/include",
		-- "Allomere/vendor/oneTBB/include",
	}

	links
	{
		"GLFW",
		"Glad",
		"ImGui",
		"ImPlot",
		"Essentia",
		"opengl32.lib",
		-- "Allomere/vendor/oneMKL/lib/intel64/mkl_rt.lib",
		-- "Allomere/vendor/oneAPI/windows/lib"
	}

	flags
	{
		"FatalWarnings"
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			-- "ALLOMERE_PLATFORM_WINDOWS",
			"GLFW_INCLUDE_NONE"
		}

		postbuildcommands
		{
			("{COPY} %{cfg.buildtarget.relpath} \"../bin/" .. outputdir .. "/Sandbox/\"")
		}

	filter "configurations:Debug"
		defines "ALLOMERE_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "ALLOMERE_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "ALLOMERE_DIST"
		runtime "Release"
		optimize "on"

project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "on"

	-- toolset "clang"

	-- vectorextensions "AVX2"

	-- openmp "On"
	
	debugdir ("bin/" .. outputdir .. "/%{prj.name}")

	warnings "Extra"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.hpp",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"Allomere/vendor/spdlog/include",
		"Allomere/vendor/json/include",
		"%{IncludeDir.miniaudio}",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.ImPlot}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.Eigen}",
		"%{IncludeDir.boost}",
		-- Essentia --
		"Allomere/vendor/essentia/src",
		"Allomere/vendor/essentia/src/algorithms",
		"Allomere/vendor/essentia/src/essentia",
		"Allomere/vendor/essentia/src/essentia/scheduler",
		"Allomere/vendor/essentia/src/essentia/streaming",
		"Allomere/vendor/essentia/src/essentia/streaming/algorithms",
		"Allomere/vendor/essentia/src/essentia/utils",
		-- Essentia --
		-- "Allomere/vendor/oneDNN/cpu_tbb/include",
		-- "Allomere/vendor/oneMKL/include",
		-- "Allomere/vendor/oneTBB/include",
		"Allomere/vendor",
		"%{prj.name}/src",
		"Allomere/src"
	}

	libdirs
	{
		-- "Allomere/vendor/oneDNN/cpu_tbb/lib",
		-- "Allomere/vendor/oneMKL/lib/intel64",
		-- "Allomere/vendor/imgui",
		-- "Allomere/vendor/oneMKL/lib/intel64/**",
		-- "Allomere/vendor/oneMKL/redist/intel64/**",
		-- "Allomere/vendor/oneTBB/lib/intel64/vc14",
		-- "Allomere/vendor/oneTBB/redist/intel64/vc14"
	}

	links
	{
		"Allomere",
		"GLFW",
		"Glad",
		"ImGui",
		"ImPlot",
		"Essentia",
		"opengl32.lib",
		-- "dnnl",
		-- "mkl_rt"
		-- "tbb12",
	}

	flags
	{
		"FatalWarnings"
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			-- "ALLOMERE_PLATFORM_WINDOWS"
		}

	filter { "Debug", "system:windows" }
		postbuildcommands
		{
			("{COPY} ../Allomere/vendor/essentia/vcpkg_installed/x64-windows/bin/*.dll \"../bin/" .. outputdir .. "/Sandbox/\""),
			-- ("{COPY} ../Allomere/vendor/essentia/packaging/win32_3rdparty/lib/libfftw3f-3.dll \"../bin/" .. outputdir .. "/Sandbox/\""),
			-- ("{COPYDIR} ../Allomere/vendor/oneTBB/redist/intel64/vc14 \"../bin/" .. outputdir .. "/Sandbox/\""),
			-- ("{COPYDIR} ../Allomere/vendor/oneMKL/redist/intel64 \"../bin/" .. outputdir .. "/Sandbox/\""),
			-- ("{COPYDIR} ../Allomere/vendor/oneDNN/cpu_tbb/bin \"../bin/" .. outputdir .. "/Sandbox/\"")
		}
		links
		{
			-- "tbb12_debug"
		}
		buildoptions 
		{
			-- "/bigobj"
		}
		
	filter { "Release", "system:windows" }
		defines
		{
			"EIGEN_NO_DEBUG"
		}
		postbuildcommands
		{
			("{COPY} ../Allomere/vendor/essentia/vcpkg_installed/x64-windows/bin/*.dll \"../bin/" .. outputdir .. "/Sandbox/\""),
			-- ("{COPY} ../Allomere/vendor/essentia/packaging/win32_3rdparty/lib/*.dll \"../bin/" .. outputdir .. "/Sandbox/\""),
			-- ("{COPYDIR} ../Allomere/vendor/oneTBB/redist/intel64/vc14 \"../bin/" .. outputdir .. "/Sandbox/\""),
			-- ("{COPY} ../Allomere/vendor/oneMKL/redist/intel64/mkl_rt.2.dll \"../bin/" .. outputdir .. "/Sandbox/\""),
			-- ("{COPY} ../Allomere/vendor/oneMKL/redist/intel64/mkl_intel_thread.2.dll \"../bin/" .. outputdir .. "/Sandbox/\""),
			-- ("{COPYDIR} ../Allomere/vendor/oneDNN/cpu_tbb/bin \"../bin/" .. outputdir .. "/Sandbox/\"")
		}
		links
		{
			-- "tbb12"
		}
	filter { "Dist", "system:windows" }
		defines
		{
			"EIGEN_NO_DEBUG"
		}
		postbuildcommands
		{
			("{COPY} ../Allomere/vendor/essentia/vcpkg_installed/x64-windows/bin/*.dll \"../bin/" .. outputdir .. "/Sandbox/\""),
			-- ("{COPY} ../Allomere/vendor/essentia/packaging/win32_3rdparty/lib/*.dll \"../bin/" .. outputdir .. "/Sandbox/\""),
			-- ("{COPYDIR} ../Allomere/vendor/oneTBB/redist/intel64/vc14 \"../bin/" .. outputdir .. "/Sandbox/\""),
			-- ("{COPY} ../Allomere/vendor/oneMKL/redist/intel64/mkl_rt.2.dll \"../bin/" .. outputdir .. "/Sandbox/\""),
			-- ("{COPY} ../Allomere/vendor/oneMKL/redist/intel64/mkl_intel_thread.2.dll \"../bin/" .. outputdir .. "/Sandbox/\""),
			-- ("{COPYDIR} ../Allomere/vendor/oneDNN/cpu_tbb/bin \"../bin/" .. outputdir .. "/Sandbox/\"")
		}
		links
		{
			-- "tbb12"
		}

	filter "configurations:Debug"
		defines "ALLOMERE_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "ALLOMERE_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "ALLOMERE_DIST"
		runtime "Release"
		optimize "on"