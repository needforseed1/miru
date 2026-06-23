# miru

`miru` (見る, "to watch") is an experimental native Linux desktop frontend for AIOStreams, with Cinemeta/AIOMetadata discovery, real IMDb ratings, OpenSubtitles, and external `mpv` playback.

The goal is a native Linux media app with Stremio/Nuvio-style browsing, Usenet/HTTP stream resolution through AIOStreams, and high-quality playback through `mpv`.

## Status

This is an early Qt 6/QML prototype.

Working now:

- Loads Cinemeta popular movie and series catalogs
- Searches Cinemeta movie and series metadata
- Details page with poster, metadata, genres, and IMDb rating
- Real IMDb ratings on posters and per episode, from IMDb's public dataset dumps cached locally in SQLite (~64 MB)
- Manifest-driven home rails (Popular/Trending/Top Rated/… discovered from the metadata addon)
- Optional self-hosted AIOMetadata as the metadata + catalog source
- Stores an AIOStreams addon URL locally (with remove/re-add)
- Fetches streams from the configured AIOStreams addon per movie/episode
- ColorfulAndConcise release badges (resolution, quality, HDR/DV, audio, source)
- Filters out torrent-style streams
- Automatic subtitles via the OpenSubtitles addon (configurable language)
- Launches direct `http`/`https` streams in external `mpv` with auth headers

Planned next:

- Resume tracking through `mpv` IPC
- SQLite watch history and Continue Watching
- Stream sorting
- Player profiles for HDR, HDR-to-SDR tonemapping, and low-power mode
- Embedded `libmpv` later, while keeping external `mpv` available

## Clone

```bash
git clone https://github.com/fjordnode/miru.git
cd miru
```

## Dependencies

What the app needs:

| Requirement | Why |
| --- | --- |
| CMake 3.21+ and a C++17 compiler | Build the project |
| Qt 6.5+ Base (Core, Gui, Network, SQL) | Core app + networking |
| Qt 6.5+ Declarative (Qt Quick, Quick Controls 2, Quick Layouts, **QtQuick.Effects**) | QML UI (Effects/`MultiEffect` is used for poster shadows/rounding) |
| Qt image format plugins (**WebP**) | Cinemeta serves WebP posters/backgrounds; without it artwork falls back to placeholders |
| zlib | Decompress the IMDb dataset dumps for episode ratings |
| `mpv` on `PATH` | Playback |

The build-time packages (`-dev`/`-devel`) are only needed where you compile.
The WebP image plugin and `mpv` are runtime requirements and must also be
present on the machine that actually runs the app.

### Debian 13 / Ubuntu

```bash
sudo apt update
sudo apt install \
  cmake g++ \
  qt6-base-dev qt6-declarative-dev \
  qml6-module-qtquick qml6-module-qtquick-controls \
  qml6-module-qtquick-layouts qml6-module-qtquick-effects \
  qt6-image-formats-plugins \
  zlib1g-dev \
  mpv
```

### Fedora

```bash
sudo dnf install \
  cmake gcc-c++ \
  qt6-qtbase-devel qt6-qtdeclarative-devel \
  qt6-qtimageformats \
  zlib-devel \
  mpv
```

### Arch Linux

```bash
sudo pacman -S \
  cmake gcc \
  qt6-base qt6-declarative \
  qt6-imageformats \
  zlib \
  mpv
```

On Fedora and Arch, the Qt Quick Controls/Layouts/Effects QML modules ship
inside `qt6-qtdeclarative-devel` / `qt6-declarative`, so no extra packages are
needed for them.

## Build

```bash
cmake -S . -B build
cmake --build build
```

Run from the build directory output:

```bash
./build/stremio-linux
```

## Display scaling

The app honours your desktop's scaling, including fractional Wayland scales
(1.25, 1.5, …). For the crispest result, run it natively on Wayland:

```bash
QT_QPA_PLATFORM=wayland ./build/stremio-linux
```

For a manual override, use `Settings → Display` to pick an interface zoom
(applied via `QT_SCALE_FACTOR` on the next launch), or set it yourself:

```bash
QT_SCALE_FACTOR=1.25 ./build/stremio-linux
```

## Configure AIOStreams

Open `Settings` in the app and paste your AIOStreams addon URL.

The app accepts either the addon base URL or a `manifest.json` URL. For example:

```text
https://example.invalid/manifest.json
```

The URL is stored locally using Qt settings.

## Playback

The app currently launches external `mpv` with a Vulkan/gpu-next profile:

```text
--hwdec=auto-safe
--vo=gpu-next
--gpu-api=vulkan
--target-colorspace-hint=yes
```

On Asahi Linux / Apple Silicon, the app uses `--hwdec=no` because mpv cannot use Vulkan HEVC video decode unless the GPU driver exposes `VK_KHR_video_decode_queue`.

Only streams with direct `http` or `https` URLs are passed to `mpv`.

Torrent streams are intentionally ignored:

- `infoHash` streams are rejected
- magnet links are rejected
- streams without a playable URL are rejected

## Project Layout

```text
src/app/        Application controller exposed to QML
src/services/   Cinemeta and AIOStreams network clients
src/player/     External mpv launcher
qml/            Qt Quick UI
docs/           Planning and dependency notes
```

## Notes

- This is not affiliated with Stremio, Cinemeta, AIOStreams, or mpv.
- Cinemeta is used for discovery metadata.
- AIOStreams is the only stream provider target for now.
- Usenet/debrid-style HTTP streams are the intended source type.
- Embedded `libmpv` is planned later, but external `mpv` remains the quality fallback.
