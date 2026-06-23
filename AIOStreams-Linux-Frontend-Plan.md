# AIOStreams Linux Frontend Plan

## Goal

Build a beautiful Linux desktop frontend for AIOStreams that uses Cinemeta for discovery and `mpv` for high-quality playback. The app will be Usenet/HTTP-stream focused only, with no torrent support in the MVP.

## Recommended Stack

- UI: Qt 6/QML
- Core: C++
- Player MVP: external `mpv` process
- Player later: embedded `libmpv`
- Metadata: Cinemeta/Stremio addon API
- Streams: AIOStreams only
- Storage: SQLite
- Packaging first target: AppImage
- Later packages: Flatpak, `.deb`, Arch/AUR

Qt/QML is the best fit because it supports a polished native Linux UI today and gives a clean path to embedded `libmpv` later. Tauri or Electron would be easier for web-style UI work, but native embedded video and HDR handling are more awkward there.

## Product Scope

The app should feel like a premium Linux media client: fast browsing, cinematic artwork, clean stream selection, and excellent playback through `mpv`.

### MVP Includes

- Cinemeta home catalogs
- Popular movies
- Popular shows
- Search
- Movie details
- Show details
- Season and episode picker
- AIOStreams addon URL configuration
- Stream fetching from AIOStreams
- Stream filtering for Usenet/HTTP-compatible links only
- External `mpv` playback
- Resume tracking through `mpv` IPC
- Continue Watching
- Local watch state in SQLite
- Basic settings page

### Not In MVP

- Torrent support
- Trakt sync
- Embedded player
- Account system
- Recommendation engine
- Multi-provider addon marketplace

## Architecture

```text
QML Frontend
  |
  v
C++ App Core
  |
  |-- CinemetaClient
  |-- AIOStreamsClient
  |-- LibraryStore SQLite
  |-- SettingsStore
  |-- PlayerController
          |
          |-- ExternalMpvPlayer first
          |-- EmbeddedLibmpvPlayer later
```

## Core Modules

### CinemetaClient

Responsibilities:

- Fetch Cinemeta manifest
- Fetch movie and series catalogs
- Fetch movie and series metadata
- Search movies and series
- Normalize Stremio addon responses for the UI
- Cache metadata and artwork references

Default Cinemeta manifest:

```text
https://v3-cinemeta.strem.io/manifest.json
```

Useful endpoint patterns:

```text
/catalog/{type}/{id}.json
/meta/{type}/{id}.json
```

### AIOStreamsClient

Responsibilities:

- Store configured AIOStreams addon URL
- Fetch stream results for movies and episodes
- Normalize stream responses
- Reject unsupported stream types
- Sort streams by quality, HDR support, size, source, and language

Useful endpoint patterns:

```text
/stream/movie/{id}.json
/stream/series/{id}:{season}:{episode}.json
```

Allowed streams:

- Direct HTTPS URLs
- HLS/DASH URLs playable by `mpv`
- Resolved Usenet/debrid-style HTTP links

Rejected streams:

- `infoHash`
- Magnet links
- Torrent-only streams
- Streams without a playable URL

### PlayerController

Expose one stable interface regardless of backend:

```text
play(url, metadata)
pause()
resume()
seek(seconds)
stop()
getPosition()
getDuration()
```

Initial backend:

```text
ExternalMpvPlayer
```

Later backend:

```text
EmbeddedLibmpvPlayer
```

This keeps the rest of the app independent from the playback implementation.

### LibraryStore

Responsibilities:

- Continue Watching
- Resume points
- Watch history
- Favorites/watchlist
- Watched episodes
- Local SQLite persistence

Suggested tables:

- `addons`
- `catalog_cache`
- `meta_cache`
- `watch_history`
- `resume_points`
- `favorites`
- `settings`
- `stream_cache`

Resume fields:

```text
content_id
type
season
episode
position_seconds
duration_seconds
updated_at
watched
```

## mpv Integration

Use external `mpv` first. This is the most reliable route for Linux playback quality and HDR experimentation.

Initial command shape:

```bash
mpv "<stream-url>" \
  --input-ipc-server=/tmp/aiostreams-mpv.sock \
  --force-media-title="<title>" \
  --save-position-on-quit=no \
  --hwdec=auto-safe \
  --vo=gpu-next \
  --gpu-api=vulkan \
  --target-colorspace-hint=yes
```

Use `mpv` JSON IPC to track:

- Playback position
- Duration
- Pause state
- End-of-file events
- Errors

Keep external `mpv` available even after embedded `libmpv` exists, because HDR behavior can vary across compositors, drivers, and GPUs.

## HDR Strategy

HDR should be a first-class feature, but Linux HDR support is still environment-dependent.

Player profiles:

- Auto
- HDR experimental
- HDR-to-SDR tonemapping
- Low-power laptop
- Custom mpv args

Recommended HDR experimental profile:

```bash
--vo=gpu-next
--gpu-api=vulkan
--hwdec=auto-safe
--target-colorspace-hint=yes
```

Recommended HDR-to-SDR tonemapping profile:

```bash
--vo=gpu-next
--gpu-api=vulkan
--tone-mapping=bt.2390
--hdr-compute-peak=yes
```

Settings should clearly explain that true HDR output depends on Wayland/X11, compositor, GPU, driver, display, and `mpv` version.

## Frontend Design Direction

The UI should be cinematic and premium rather than a direct Stremio clone.

Visual direction:

- Dark cinematic interface
- Large backdrop hero sections
- Poster rails with smooth navigation
- Frosted or blurred detail panels
- Strong typography
- Quality-focused stream cards
- Keyboard-first navigation
- Living-room friendly spacing
- Responsive behavior for small laptop displays and large TVs

Primary screens:

- Home
- Movies
- Shows
- Search
- Details
- Season/Episode view
- Stream picker
- Continue Watching
- Settings

Home sections:

- Continue Watching
- Popular Movies
- Popular Shows
- New Movies
- New Shows
- Search shortcut
- Watchlist later

Details page layout:

```text
Backdrop hero
Title or logo
Year | runtime | genres | IMDb rating
Description
Play or Resume button
Season selector for shows
Cast row
Related titles later
```

Stream picker card example:

```text
2160p HDR10 | 24.1 GB | Usenet | English
1080p WEB-DL | 8.4 GB | Usenet | English
720p WEB | 2.1 GB | Usenet | English
```

## Settings

Initial settings:

- AIOStreams addon URL
- Preferred quality
- Prefer HDR
- Preferred language
- Maximum file size
- `mpv` executable path
- Player profile
- Custom `mpv` arguments
- Resume tracking enabled
- Clear metadata cache
- Clear image cache

Startup diagnostics:

- Detect `mpv`
- Show `mpv --version`
- Warn if `mpv` is missing
- Warn if Vulkan/HDR profile is selected but likely unsupported

## Build Phases

### Phase 1: Prototype

- Create Qt/QML app shell
- Add navigation structure
- Fetch Cinemeta popular movies and shows
- Render poster rails
- Add basic details page
- Configure one AIOStreams URL
- Fetch streams for selected movie
- Launch external `mpv`

### Phase 2: MVP

- Add search
- Add show, season, and episode support
- Add SQLite storage
- Add Continue Watching
- Add `mpv` IPC resume tracking
- Add settings page
- Add stream sorting and filtering
- Add basic error states

### Phase 3: Polish

- Improve home page composition
- Add skeleton loading states
- Add artwork caching
- Add stream badges
- Add keyboard navigation
- Add empty states
- Add app icon and branding
- Add AppImage packaging

### Phase 4: Player Quality

- Add player profiles
- Add HDR/SDR mode selection
- Add custom `mpv` args
- Add subtitle preferences
- Add audio preferences
- Add Wayland/X11 detection
- Add basic player diagnostics

### Phase 5: Embedded Player

- Add `libmpv` dependency
- Create QML video surface
- Implement `EmbeddedLibmpvPlayer`
- Keep `ExternalMpvPlayer` as an option
- Compare HDR behavior between external and embedded playback
- Decide whether embedded should become default

### Phase 6: Later Features

- Trakt sync
- Better recommendations
- Watchlist sync
- Flatpak package
- `.deb` package
- Arch/AUR package
- Optional remote control API

## Main Risks

- AIOStreams responses may vary depending on provider configuration.
- Some streams may require headers, cookies, or tokens that must be passed to `mpv`.
- Linux HDR support varies heavily by compositor, GPU, driver, display, and `mpv` version.
- Cinemeta availability depends on external Stremio infrastructure.
- Series metadata and episode IDs may require careful Stremio protocol handling.
- Embedded `libmpv` may not match external `mpv` quality on every Linux setup.

## Recommended MVP Definition

The first public MVP is successful when a user can:

- Add an AIOStreams URL
- Browse popular movies and shows from Cinemeta
- Search for a title
- Open a details page
- Pick an episode for a show
- See AIOStreams Usenet/HTTP streams
- Launch playback in external `mpv`
- Quit playback and resume later
- Use the app comfortably with keyboard navigation

## Final Direction

Build a Qt 6/QML Linux desktop app with external `mpv` first, planned embedded `libmpv` later, Cinemeta for browsing, AIOStreams for streams, SQLite for local state, and Trakt saved for a later release.
