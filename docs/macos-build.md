# macOS Build

Use the GitHub Actions workflow for end-user builds. Use a dedicated
`build-macos/` directory only for local packaging or debugging.

## End-User Package

The `macOS App` workflow builds downloadable packages on GitHub:

- It runs automatically for tags that match `v*` and attaches packages to the
  matching GitHub Release.
- It can also be started manually from the GitHub Actions tab.
- It uploads `.dmg` and `.zip` packages for Apple Silicon and Intel Macs.

These artifacts include the Qt runtime, bundled mpv resources, and a bundled
`mpv` executable with non-system Homebrew libraries copied into the app bundle.
They are ad-hoc signed, but not notarized. Public distribution without the
Gatekeeper warning still requires a Developer ID certificate and notarization.

## Prerequisites

- macOS with a C++17-capable compiler.
- CMake 3.21 or newer.
- Qt 6.5 or newer with Quick, Quick Controls 2, Quick Layouts, QtQuick.Effects, Network, SQL, Concurrent, and image format plugins.
- Qt WebP image format plugin for Cinemeta artwork.
- zlib.
- Homebrew `openssl@3`; the packaging script bundles `libssl`/`libcrypto` so
  Qt can use its OpenSSL TLS backend (Secure Transport has no TLS 1.3).
- An `mpv` executable to bundle.

For local development, a Homebrew `mpv` can be used if its runtime libraries are also available on the machine. For distribution, use a self-contained/signable mpv build and include the required license notices.

## Configure

```bash
cmake -S . -B build-macos \
  -DCMAKE_BUILD_TYPE=Release \
  -DMIRU_BUNDLED_MPV=/path/to/mpv
```

If you omit `MIRU_BUNDLED_MPV`, the app still builds and falls back to finding `mpv` on `PATH`.

## Build

```bash
cmake --build build-macos
```

With Makefiles or Ninja, the app bundle is expected at:

```text
build-macos/Miru.app
```

With Xcode, check the selected configuration directory, for example
`build-macos/Release/Miru.app`.

## Deploy Qt Runtime

Run `macdeployqt` from the Qt installation used to build the app:

```bash
macdeployqt build-macos/Miru.app -qmldir="$PWD/qml"
```

Or run the repository packaging helper, which deploys Qt, copies non-system
`mpv` libraries when bundled mpv is present, applies an ad-hoc signature, and
creates `.dmg` and `.zip` outputs:

```bash
scripts/package-macos.sh
```

## Verify

- Launch `build-macos/Miru.app`.
- Confirm the app name and window title are Miru.
- Confirm the app starts without system `mpv` on `PATH`.
- Confirm a stream opens in bundled external mpv.
- Confirm provider request headers work.
- Confirm subtitles are passed to mpv.
- Confirm playback progress, pause, seek, stop, and resume work.
- Confirm WebP poster artwork loads.
- Confirm IMDb ratings cache downloads and builds.
- Confirm a TLS 1.3-only addon URL loads (verifies the bundled OpenSSL
  backend; Secure Transport would fail with handshake error -9836).

## Notes

- Embedded playback is not a macOS target for now; use external mpv.
- Public distribution will also need app signing and notarization.
- The bundled mpv path inside the app is `Contents/Resources/mpv/mpv`.
- Bundled ModernZ resources are copied into `Contents/Resources/mpv/modernz`.
