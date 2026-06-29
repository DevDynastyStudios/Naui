workspace "NauiApp"
    configurations { "Debug", "Release" }

project "Naui"
    kind "StaticLib"
    language "C"
    architecture "x64"
    targetdir "build/%{cfg.buildcfg}"

    files {
        "naui/**.c",
        "naui/vendor/**.c"
    }

    includedirs {
        "naui",
        "naui/vendor"
    }

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

    filter "system:windows"
        defines { "MGFX_D3D11" }
    filter "system:linux"
        defines { "MGFX_OPENGL" }

project "NauiApp"
    language "C"
    architecture "x64"
    targetdir "build/%{cfg.buildcfg}"

    files {
        "app/src/**.c",
        "app/vendor/**.c"
    }

    links { "Naui" }

    includedirs {
        ".",
        "app/src",
        "app/vendor",
        "naui",
        "naui/vendor"
    }

    filter "configurations:Debug"
        kind "ConsoleApp"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        kind "WindowedApp"
        defines { "NDEBUG" }
        optimize "On"

    filter "system:linux"
        links { "X11", "EGL", "m" }


project "UnitTest"
	kind "ConsoleApp"
	language "C"
	architecture "x64"
	targetdir "build/unit_test/%{cfg.buildcfg}"

	files {
		"unit_test/**.c"
	}

	links { "Naui" }
	includedirs {
		".",
		"unit_test",
		"naui",
        "naui/vendor"
	}

	filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

    filter "system:linux"
        links { "X11", "EGL", "m" }

