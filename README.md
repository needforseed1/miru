# Miru

Miru is a native Qt 6 desktop frontend for AIOStreams. It provides
Stremio-style browsing, metadata from Cinemeta or a compatible metadata addon,
real IMDb ratings, OpenSubtitles integration, Trakt resume sync, and playback
through `mpv`.

The app is focused on direct HTTP streams from Usenet/debrid-style providers.
It is not a torrent client.

## Status

Miru is still an early prototype, but the main workflow is usable:

- Browse home rails from the active metadata addon.
- Search movies and series.
- Open detail pages with artwork, genres, metadata, episodes, and IMDb scores.
- Configure an AIOStreams manifest URL for playable HTTP streams.
- Filter out torrent-style streams, magnet links, and `infoHash` entries.
- Show ColorfulAndConcise release badges for quality, HDR/DV, audio, and source.
- Load subtitles from the OpenSubtitles Stremio addon.
- Track local Continue Watching progress through `mpv` IPC.
- Optionally sync resume progress and Next Up through Trakt.
- Play in external `mpv`, with experimental embedded mpv available on X11/XWayland.

Planned or incomplete:

- Better stream sorting and filtering.
- More playback profiles for HDR, HDR-to-SDR, and low-power playback.
- Embedded playback polish, while keeping external `mpv` as the reliable fallback.

## Clone

```bash
git clone https://github.com/needforseed1/miru.git
cd miru
```

## Dependencies

Build dependencies:

| Requirement | Why |
| --- | --- |
| CMake 3.21+ and a C++17 compiler | Build system and C++ app code |
| Qt 6.5+ Base | Core, GUI, networking, SQL |
| Qt 6.5+ Declarative | Qt Quick, Quick Controls 2, Layouts, Effects |
| zlib | Decompress IMDb public TSV datasets |

Runtime dependencies:

| Requirement | Why |
| --- | --- |
| Qt WebP image plugin | Cinemeta artwork is often WebP |
| `mpv` on `PATH`, or bundled `mpv` | Video playback |

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

On Fedora and Arch, the Qt Quick Controls, Layouts, and Effects modules ship
with the Qt declarative packages.

## Build

```bash
cmake -S . -B build
cmake --build build
```

Run the development build:

```bash
./build/miru
```

The executable target and app display name are `Miru`/`miru`.

### macOS Bundle

For macOS app bundle builds, use a separate build directory:

```bash
cmake -S . -B build-macos -DCMAKE_BUILD_TYPE=Release -DMIRU_BUNDLED_MPV=/path/to/mpv
cmake --build build-macos
```

This produces `Miru.app` with bundled mpv resources. See `docs/macos-build.md`
for packaging and verification notes.

## Settings

### AIOStreams Addon

Paste your AIOStreams manifest URL in Settings. Miru stores it locally and uses
it to fetch playable HTTP streams for each movie or episode.

The app accepts a base addon URL or a `manifest.json` URL, for example:

```text
https://example.invalid/manifest.json
```

### Metadata Addon

Leave the metadata URL blank to use Cinemeta. You can also use AIOMetadata or
another Stremio-compatible metadata addon for richer details, artwork, episode
stills, and episode ratings.

### Subtitles

Subtitles are loaded from the OpenSubtitles Stremio addon. Choose a preferred
language in Settings, or turn subtitles off.

### Trakt

Trakt is optional. Add a Trakt API app client ID and secret, then connect your
account to sync resume progress and Next Up across devices.

### IMDb Ratings

Miru downloads IMDb's public ratings datasets, builds a local SQLite cache, and
uses it for poster scores and episode scores. Refreshes are local after the
dataset download.

### Display

Miru honors desktop scaling, including fractional Wayland scales. The Settings
page also has an interface zoom option, applied via `QT_SCALE_FACTOR` on the
next launch.

For a native Wayland run:

```bash
QT_QPA_PLATFORM=wayland ./build/miru
```

For a manual scale override:

```bash
QT_SCALE_FACTOR=1.25 ./build/miru
```

## Playback

Miru launches direct `http` and `https` streams in `mpv`. It passes the stream
URL, required provider auth headers, subtitles, an IPC socket, and network-safe
direct-stream options:

```text
--ytdl=no
--cache=yes
--demuxer-readahead-secs=20
--network-timeout=60
```

Playback options in Settings:

- External or embedded mpv mode.
- ModernZ mpv control overlay.
- Start external mpv fullscreen.
- Hardware decoding with `--hwdec=auto-safe`.
- gpu-next renderer with `--vo=gpu-next`.
- HDR/Vulkan hint with `--gpu-api=vulkan --target-colorspace-hint=yes`.
- Extra mpv arguments appended last, so they can override defaults.

Embedded playback currently requires X11/XWayland. If embedded playback is not
available, Miru falls back to external `mpv`.

## Continue Watching

Local progress is captured through `mpv` IPC and stored in a small JSON watch
history file. Continue Watching resumes by movie or episode identity, not by
stream URL, so a fresh debrid/Usenet link can resume from the same point.

When Trakt is connected, resume rows and Next Up come from Trakt instead.

## Project Layout

```text
src/app/        Application controller exposed to QML
src/services/   Cinemeta, AIOStreams, subtitles, IMDb, and Trakt clients
src/player/     External and embedded mpv helpers
qml/            Qt Quick UI
resources/      Release badge data and bundled mpv UI resources
docs/           Planning and dependency notes
```

## Notes

- Miru is not affiliated with Stremio, Cinemeta, AIOStreams, OpenSubtitles,
  Trakt, IMDb, or mpv.
- AIOStreams is the stream provider target for now.
- Usenet/debrid-style HTTP streams are the intended source type.
- External `mpv` remains supported even as embedded playback improves.
