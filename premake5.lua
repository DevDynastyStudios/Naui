workspace "Naui"
    configurations { "Release" }
    startproject "Application"

project "Naui"
    kind "SharedLib"
    language "C++"
	cppdialect "C++20"
	targetdir "bin/%{cfg.buildcfg}"

    architecture "x64"
    staticruntime "Off"

    files {
        "naui/**.h",
        "naui/**.cpp",
        "naui/vendor/stb/**.h",
        "naui/vendor/stb/**.c",
        "naui/vendor/imgui/**.h",
        "naui/vendor/imgui/**.cpp",
		"naui/vendor/miniz/**.c",
		"naui/vendor/miniz/**.h"
    }

    includedirs {
        "naui",
        "naui/vendor",
        "naui/vendor/stb",
        "naui/vendor/imgui",
        "naui/vendor/nlohmann",
		"naui/vendor/miniz"
    }

    defines { "NDEBUG", "NAUI_EXPORT", "IMGUI_BUILD_DLL" }
    optimize "On"

project "Application"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
	targetdir "bin/%{cfg.buildcfg}"

    architecture "x64"
    staticruntime "Off"

    files {
        "main/**.h",
        "main/**.cpp"
    }

    includedirs {
        ".",
        "main",
        "naui",
        "naui/vendor",
        "naui/vendor/imgui"
    }

    links { "Naui" }

    optimize "On"

    filter { "configurations:Release" }
        defines { "NDEBUG" }
