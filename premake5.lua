workspace "Naui"
    configurations { "Release" }
    startproject "SandboxApp"

project "Naui"
    kind "SharedLib"
    language "C++"
	cppdialect "C++20"
	targetdir "bin/%{cfg.buildcfg}"

    architecture "x64"
    staticruntime "Off"

    files {
        "Naui/**.h",
        "Naui/**.c",
        "Naui/**.cpp",
        "Naui/**.hpp",
    }

    includedirs {
        "Naui",
        "Naui/Vendor",
        "Naui/Vendor/imgui",
        "Naui/Vendor/miniz",
        "Naui/Vendor/nlohmann",
        "Naui/Vendor/stb"
    }

    defines { "NDEBUG", "NAUI_EXPORT" }
    optimize "On"

project "SandboxApp"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
	targetdir "bin/%{cfg.buildcfg}"

    architecture "x64"
    staticruntime "Off"

    files {
        "Sandbox/**.h",
        "Sandbox/**.cpp"
    }

    includedirs {
        ".",
        "Sandbox",
        "Naui",
        "Naui/Vendor",
        "Naui/Vendor/imgui"
    }

    links { "Naui" }

    optimize "On"

    filter { "configurations:Release" }
        defines { "NDEBUG" }