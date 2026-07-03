# AGENTS.md

## Project Shape

- Qt 6/QML + C++17 desktop app; entrypoint is `src/main.cpp`, which exposes `ApplicationController` to QML as `appController` and loads QML module `StremioLinux` / `Main`.
- `src/app/ApplicationController.*` wires the app: metadata catalogs/search, AIOStreams releases, subtitles, IMDb rating cache, settings, and external mpv playback.
- `src/services/CinemetaClient.*` is the metadata/catalog client. Empty metadata URL means Cinemeta; a configured URL can point at AIOMetadata or another Stremio-compatible metadata addon.
- `src/services/AIOStreamsClient.*` is release fetching/filtering. It intentionally rejects `infoHash`, magnet, and non-HTTP streams; keep request headers from `behaviorHints.proxyHeaders.request` because mpv needs them for some providers.
- `src/player/ExternalMpvPlayer.*` launches external `mpv`; playback is not embedded yet.
- QML lives under `qml/`; `qml/Theme.qml` is a CMake-declared singleton and components are imported from `qml/components`.

## Build And Verify

- Configure/build from repo root: `cmake -S . -B build && cmake --build build`.
- Run the Qt test suite after building: `ctest --test-dir build --output-on-failure`.
- Run locally with a desktop session: `./build/miru`.
- Wayland scaling check: `QT_QPA_PLATFORM=wayland ./build/miru`.
- No formatter, CI, or CMake preset config currently exists; use a clean CMake build plus `ctest` as the focused verification step.
- `build/` is ignored. Do not commit generated CMake/QML cache output.

## Dependencies That Are Easy To Miss

- CMake requires Qt 6.5+ components `Quick`, `QuickControls2`, `Network`, `Sql`, and `Concurrent`, plus `ZLIB`.
- Runtime also needs Qt WebP image format plugins; Cinemeta artwork is often WebP and otherwise falls back to placeholders.
- Runtime needs `mpv` on `PATH`; only direct-stream/network args are forced by default. Hardware decode, `gpu-next`, HDR/Vulkan hints, and custom mpv args are persisted settings. Custom args are appended last so they can override app defaults.
- IMDb ratings download public TSV gzip datasets from `datasets.imdbws.com`, build a local SQLite cache via zlib/QtConcurrent, and store it under Qt `AppDataLocation` as `imdb_ratings.sqlite`.

## Persistent Settings

- Settings use `QSettings` with organization `AIOStreamsLinux` and app `AIOStreams Linux`.
- Important keys: `addons/aioStreamsUrl`, `addons/metadataUrl`, `subtitles/language`, `ui/scaleFactor`, `imdb/lastRefresh`, `mpv/hardwareDecoding`, `mpv/gpuNext`, `mpv/hdrHint`, `mpv/extraArgs`.
- UI zoom is applied before `QGuiApplication` starts via `QT_SCALE_FACTOR`; changing it in settings requires restart.

## Product Constraints

- The app is AIOStreams/Usenet/debrid HTTP-first, not a torrent client.
- Keep external `mpv` available even if embedded `libmpv` is added later.
- Preserve full release labels/badges where possible; `resources/Sterzeck_badge.json` drives ColorfulAndConcise release badges.
