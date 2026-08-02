# Naui
Naui is an entire UI engine using C built for [our software](https://github.com/DevDynastyStudios).<br>
It has the following features:
- Custom [UI library](https://github.com/boxDev2008/Leaf) (by boxDev).
- Custom renderer using [mgfx](https://github.com/boxDev2008/magma) (rect/text/image rendering, supports GL/VK/D3D11).
- All of your convenient utilities for C development (arenas, [lists, hashmaps](https://github.com/nothings/stb/tree/master/stb_ds.h), strings...).
- Panels.
- Actions.
- Themes.
- Shortcuts.
- Archives (makes writing project file formats much easier).
- Filesystem API.
- Iterator API.
- Localization.

# Building
Naui comes with a simple Lua CLI tool you can use:
```bash
lua build.lua                   # brings up the help menu
lua build.lua debug             # builds project in debug mode
lua build.lua debug run_debug   # build and runs project in debug mode
```

## Windows:
You need to install the following:
- [clang](https://github.com/llvm/llvm-project/releases/download/llvmorg-22.1.8/clang+llvm-22.1.8-x86_64-pc-windows-msvc.tar.xz)
- [Visual Studio](https://visualstudio.microsoft.com/thank-you-downloading-visual-studio/?sku=Community&channel=Stable&version=VS18&source=VSLandingPage&cid=2500&passive=false) (eww)
![vsstudio](https://raw.githubusercontent.com/DevDynastyStudios/Naui/refs/heads/main/screenshots/vs.png)

## Linux:
You need to install the following:
- clang
- libx11-dev
- libegl-dev
- xorg-xwayland (if on Wayland)
