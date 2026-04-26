workspace "NauiApp"
    configurations { "Debug", "Release" }

project "Naui"
    kind "StaticLib"
    language "C"
    targetdir "build/%{cfg.buildcfg}"

    files {
        "naui/**.c",
        "naui/vendor/**.c"
    }

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

project "NauiApp"
    kind "ConsoleApp"
    language "C"
    targetdir "build/%{cfg.buildcfg}"

    files {
        "app/src/**.c",
        "app/vendor/**.c"
    }

    links { "Naui" }
    includedirs {
        ".",
        "app/vendor"
    }

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"