# Naui

Naui is an application engine written in C for [our software](https://github.com/DevDynastyStudios).

It has the following features:

* Custom [UI library](https://github.com/boxDev2008/Leaf)
* Custom renderer using [mgfx](https://github.com/boxDev2008/magma) (rect/text/image rendering, supports GL/VK/D3D11).
* All of your convenient utilities for C development (arenas, [lists, hashmaps](https://github.com/nothings/stb/tree/master/stb_ds.h), strings...).
* Panels.
* Actions.
* Themes.
* Shortcuts.
* Archives (makes writing project file formats much easier).
* Filesystem API.
* Iterator API.
* Localization.

# Building

Naui comes with a simple Lua CLI tool you can use:

```bash
lua build.lua                   # brings up the help menu
lua build.lua debug             # builds project in debug mode
lua build.lua debug run_debug   # builds and runs the project in debug mode
```

## Windows

Install the following:

* [clang](https://github.com/llvm/llvm-project/releases/download/llvmorg-22.1.8/clang+llvm-22.1.8-x86_64-pc-windows-msvc.tar.xz)
* [Lua](https://sourceforge.net/projects/luabinaries/files/5.5.0/)
* [Visual Studio](https://visualstudio.microsoft.com/thank-you-downloading-visual-studio/?sku=Community&channel=Stable&version=VS18&source=VSLandingPage&cid=2500&passive=false) (eww)

![vsstudio](https://raw.githubusercontent.com/DevDynastyStudios/Naui/refs/heads/main/content/screenshots/vs.png)

---

## Linux

Install the required packages for your distribution.

### Ubuntu / Debian

```bash
sudo apt update
sudo apt install clang lua5.4 libx11-dev libegl-dev xwayland
```

### Fedora

```bash
sudo dnf install clang lua libX11-devel mesa-libEGL-devel xorg-x11-server-Xwayland
```

### Arch Linux

```bash
sudo pacman -S clang lua libx11 libegl xorg-xwayland
```

### openSUSE

```bash
sudo zypper install clang lua54 libX11-devel Mesa-libEGL-devel xwayland
```

> **Note:** `xwayland` is only required if you're running a Wayland session.

---

## macOS

Support coming soon.
