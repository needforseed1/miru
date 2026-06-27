# macOS and Windows Port Plan

## Goal

Port the app to macOS and Windows using external bundled mpv as the playback backend. Users should not need to install mpv manually.

## Strategy

Port one platform at a time.

Start with macOS because it is closest to the current Linux runtime model: Qt works well, bundled external processes are normal, and mpv IPC can likely keep using Unix-style local sockets.

After macOS is working, port Windows with the same architecture plus Windows-specific mpv named-pipe IPC.

## Non-Goals

Do not replace external mpv with libmpv yet.

Do not prioritize embedded playback on macOS or Windows.

Do not require users to install mpv separately.

## Phase 1: Platform-Neutral App Cleanup

Rename Linux-specific identifiers where user-visible or packaging-related.

Change app name from `AIOStreams Linux` to a platform-neutral name.

Change CMake target/package naming from `stremio-linux` when appropriate.

Keep settings migration optional unless existing user settings need to be preserved.

## Phase 2: Bundled mpv Discovery

Add mpv resolution logic.

Search bundled mpv first.

Fall back to `PATH` mpv second.

Expected locations:

- macOS: inside the `.app` bundle, for example `Contents/Resources/mpv/mpv`.
- Windows: next to the executable, for example `mpv.exe` or `mpv/mpv.exe`.
- Linux: keep existing `PATH` behavior, optionally support bundled mpv later.

Update error messages to mention bundled mpv and PATH fallback.

## Phase 3: mpv IPC Portability

Abstract mpv IPC server path creation.

Linux/macOS: continue using a temp Unix socket path.

Windows: use a named pipe path such as `\\.\pipe\aiostreams-mpv-<pid>-<timestamp>`.

Verify `QLocalSocket` connects correctly to mpv on each platform.

Keep playback progress, pause, seek, and finish tracking working across platforms.

## Phase 4: macOS Port

Build the app with Qt 6 on macOS.

Package as a `.app` bundle.

Bundle Qt dependencies using `macdeployqt`.

Bundle mpv inside the app.

Verify bundled mpv launches from the app bundle.

Verify HTTP streams, request headers, subtitles, seek/resume, IMDb cache, and poster images.

Add app icon and bundle metadata.

Decide later whether to sign/notarize for public distribution.

## Phase 5: Windows Port

Build the app with Qt 6 on Windows.

Package Qt dependencies using `windeployqt`.

Bundle `mpv.exe` and required mpv DLLs.

Implement and verify Windows named-pipe IPC.

Verify playback, headers, subtitles, seek/resume, IMDb cache, and poster images.

Add app icon and Windows metadata.

Decide installer format later: zip first, installer/MSIX later.

## Phase 6: Packaging and Distribution

Include license notices for bundled mpv and related libraries.

Document mpv binary source and version.

Keep custom mpv args supported.

Prefer simple dev packages first.

Only add signing/notarization after playback and packaging are stable.

## Verification Checklist

- App launches.
- Home/catalog metadata loads.
- Search works.
- AIOStreams streams load.
- External bundled mpv launches without system mpv installed.
- Provider request headers are passed to mpv.
- Subtitles are passed to mpv.
- Playback progress updates.
- Pause, seek, stop, and resume work.
- IMDb ratings cache downloads and builds.
- Settings persist correctly.
- Qt image plugins load WebP artwork.

## Recommended Order

1. macOS external bundled mpv.
2. Windows external bundled mpv.
3. Optional Linux bundled mpv support.
4. Optional embedded/libmpv research later.

---

# Trakt Integration Plan

## Goal

Add Trakt support for cross-device resume states and playback scrobbling while keeping local watch history as the app's immediate source of truth.

Trakt credentials must be supplied by the user in Settings. Do not hardcode or bundle Trakt API credentials in the source tree or build system.

## Reference

Nuvio uses:

- Build-time Trakt credentials from external config.
- Browser OAuth redirect auth.
- Stored access and refresh tokens.
- Token refresh before API calls.
- `/sync/playback/movies` and `/sync/playback/episodes` for resume state.
- `/scrobble/start` and `/scrobble/stop` for playback updates.
- Percentage-based remote progress that is converted to seconds once player duration is known.

For this app, use a smaller implementation focused on resume and scrobbling.

## Non-Goals

- Do not implement Trakt lists, comments, recommendations, or library sync initially.
- Do not replace local `WatchHistory`.
- Do not store stream URLs from Trakt; Trakt only supplies content identity and progress.
- Do not add custom desktop URI scheme registration unless device-code auth is not sufficient.

## Phase 1: Settings And Auth

Add Trakt settings fields:

- Trakt client ID.
- Trakt client secret.
- Connect button.
- Disconnect button.
- Manual refresh button.
- Connection status.
- Optional toggle: Use Trakt for Continue Watching.

Suggested `QSettings` keys:

- `trakt/clientId`
- `trakt/clientSecret`
- `trakt/accessToken`
- `trakt/refreshToken`
- `trakt/tokenCreatedAt`
- `trakt/tokenExpiresIn`
- `trakt/username`
- `trakt/useForContinueWatching`

Preferred auth flow:

1. Use Trakt device-code auth if available for the app type.
2. Show the verification URL and user code in Settings.
3. Poll until approved, expired, or cancelled.
4. Exchange approval for access and refresh tokens.

Fallback auth flow:

1. Open browser OAuth URL.
2. Use localhost callback to receive the authorization code.
3. Exchange code for access and refresh tokens.

Avoid a custom URI scheme for the first implementation because it adds desktop packaging and registration work.

## Phase 2: Trakt Client

Add `src/services/TraktClient.*` using Qt networking.

Responsibilities:

- Load user-supplied credentials from settings.
- Store auth state in settings.
- Refresh tokens when they are close to expiry.
- Provide authorized request headers.
- Fetch remote playback progress.
- Send scrobble events.

Required headers for authenticated API calls:

- `trakt-api-version: 2`
- `trakt-api-key: <client_id>`
- `Authorization: Bearer <access_token>`

Token refresh rule:

- Refresh if expiry is unknown or within 60 seconds.

## Phase 3: Data Model Changes

Extend local watch-history entries with optional fields:

- `progressPercent`
- `source`, for example `local` or `trakt`
- `traktUpdatedAt`

Keep existing fields:

- `position`
- `duration`
- `stream`
- `key`

For Trakt remote progress:

- If Trakt provides percentage only, store `progressPercent` and leave `position`/`duration` as zero.
- If duration is known locally, convert percent to seconds.
- If duration is unknown, keep the percent and resolve seek position after mpv reports duration.

## Phase 4: Pull Remote Resume

On app startup, manual refresh, and after successful scrobble stop:

- Fetch `/sync/playback/movies`.
- Fetch `/sync/playback/episodes`.
- Convert Trakt progress entries into local watch-history-shaped entries.
- Merge local and Trakt entries by content key, preferring the newest timestamp.

Key mapping:

- Movie key: `movie:<imdb-or-trakt-id>`.
- Episode key: `series:<parent-id>:<season>:<episode>`.

Metadata behavior:

- Prefer existing local metadata when present.
- If a Trakt entry has no local metadata, resolve via the configured metadata addon when possible.
- If metadata cannot be resolved, keep the entry hidden until it can be resolved.

## Phase 5: Apply Trakt Resume To Playback

For direct local resume:

- Use saved local stream URL if it is still safe to reuse.
- Resume using saved `position`, or `progressPercent * duration` after mpv reports duration.

For Trakt-only resume:

- Open details or stream selection for that content.
- Carry the Trakt resume percent/position into the next playback launch.
- Seek once mpv reports duration if only percent is available.

This may require adding an optional pending initial progress percent alongside the existing `--start` seconds flow.

## Phase 6: Scrobbling

On playback actually starts:

- Send `/scrobble/start`.

On pause, stop, close, or completion:

- Send `/scrobble/stop`.

Progress calculation:

```text
progress = playbackPosition / playbackDuration * 100
```

Rules:

- Skip scrobble when Trakt is disconnected.
- Skip scrobble when duration is unknown.
- Clamp progress to `0..100`.
- Suppress duplicate scrobbles within a short time window.
- Retry `stop` once or twice because it is the event that preserves remote resume state.

Content body mapping:

- Movies: send title/year when known plus IDs.
- Episodes: send show IDs plus season and episode.
- Prefer IMDb IDs when available.
- Avoid title-only fuzzy scrobbles unless there is no safe ID and the user explicitly enables it later.

## Phase 7: Expiring URL Safe Resume

Add helper:

```text
hasLikelyExpiringPlaybackCredentials(url)
```

Flag query keys/fragments such as:

- `token`
- `access_token`
- `auth`
- `jwt`
- `sig`
- `signature`
- `hmac`
- `policy`
- `exp`
- `expires`
- `expires_in`
- `expiresAt`
- `keyPairId`

Resume behavior:

- Safe saved URL: play directly.
- Expiring saved URL: reload streams/details and apply local/Trakt resume state to the newly selected stream.
- No saved URL: load details/streams and apply Trakt resume state.

## Phase 8: UI Updates

Settings:

- Add Trakt credentials fields.
- Add connect/disconnect controls.
- Add status text for connected user or current auth step.
- Add manual sync button.

Continue Watching:

- Show merged local and Trakt resume entries.
- Prefer local stream-backed entries when duplicate content exists.
- Indicate Trakt-only entries subtly if they require stream selection.

Playback:

- If starting from Trakt percentage, seek after duration is known.
- Keep current local resume backoff behavior for second-based resumes.

## Verification Checklist

- App builds cleanly.
- User can enter Trakt client ID and client secret.
- Connect flow succeeds.
- Disconnect clears tokens but keeps credentials unless user clears them.
- Token refresh works.
- Remote movie playback progress appears in Continue Watching.
- Remote episode playback progress appears in Continue Watching.
- Local progress still works with Trakt disabled.
- Playback scrobble start is sent once playback actually begins.
- Playback scrobble stop is sent on pause/quit/end.
- Resume from Trakt percentage seeks correctly after duration is known.
- Expiring saved URLs are not blindly reused.
- Existing AIOStreams playback headers and mpv resume still work.

## Recommended Order

1. Add settings fields and persistent credential/token storage.
2. Implement `TraktClient` auth and token refresh.
3. Implement `/sync/playback/*` pull and merge into Continue Watching.
4. Implement scrobble start/stop.
5. Add percent-based resume seek after duration is known.
6. Add expiring URL detection for saved stream resume.
