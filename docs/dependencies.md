# Dependencies

The app is a Qt 6/QML C++ project and needs Qt development packages at configure time.

## Required

- CMake 3.21 or newer
- C++17 compiler
- Qt 6.5 or newer
  - Qt Base: Core, Gui, Network, SQL
  - Qt Declarative: Qt Quick, Quick Controls 2, Quick Layouts, QtQuick.Effects
- Qt image format plugins (WebP) — Cinemeta serves WebP poster/background art; without this plugin posters fall back to placeholders
- zlib — decompresses the IMDb dataset dumps used for per-episode ratings
- `mpv`

The `-dev`/`-devel` packages are only needed where you compile. The WebP image
plugin and `mpv` are runtime requirements and must also be installed on the
machine that runs the app.

## Debian/Ubuntu

```bash
sudo apt install \
  cmake g++ \
  qt6-base-dev qt6-declarative-dev \
  qml6-module-qtquick qml6-module-qtquick-controls \
  qml6-module-qtquick-layouts qml6-module-qtquick-effects \
  qt6-image-formats-plugins \
  zlib1g-dev \
  mpv
```

## Fedora

```bash
sudo dnf install \
  cmake gcc-c++ \
  qt6-qtbase-devel qt6-qtdeclarative-devel \
  qt6-qtimageformats \
  zlib-devel \
  mpv
```

## Arch Linux

```bash
sudo pacman -S \
  cmake gcc \
  qt6-base qt6-declarative \
  qt6-imageformats \
  zlib \
  mpv
```

On Fedora and Arch, the Qt Quick Controls/Layouts/Effects QML modules ship inside
`qt6-qtdeclarative-devel` / `qt6-declarative`.

## Configure

```bash
cmake -S . -B build
cmake --build build
```

If CMake cannot find Qt, set `CMAKE_PREFIX_PATH` to the Qt install prefix or install the distro Qt development packages.
