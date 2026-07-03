#!/usr/bin/env bash
set -euo pipefail

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "package-macos.sh must run on macOS" >&2
    exit 1
fi

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${MIRU_BUILD_DIR:-"$repo_root/build-macos"}"
app_bundle="${MIRU_APP_BUNDLE:-"$build_dir/Miru.app"}"
output_dir="${MIRU_OUTPUT_DIR:-"$build_dir/package"}"
artifact_name="${MIRU_ARTIFACT_NAME:-Miru-macOS}"
qmldir="${MIRU_QML_DIR:-"$repo_root/qml"}"

if [[ ! -d "$app_bundle" ]]; then
    echo "App bundle not found: $app_bundle" >&2
    exit 1
fi

if ! command -v macdeployqt >/dev/null 2>&1; then
    echo "macdeployqt was not found on PATH. Use the same Qt install used to build Miru." >&2
    exit 1
fi

mkdir -p "$output_dir"

echo "Deploying Qt runtime"
macdeployqt "$app_bundle" -qmldir="$qmldir" -always-overwrite

mpv_binary="$app_bundle/Contents/Resources/mpv/mpv"
if [[ -x "$mpv_binary" ]]; then
    echo "Bundling non-system mpv libraries"
    lib_dir="$app_bundle/Contents/Frameworks/mpv"
    mkdir -p "$lib_dir"

    declare -a queue=("$mpv_binary")
    declare -a visited=()
    declare -a copied=()

    contains_value() {
        local needle="$1"
        shift

        local item
        for item in "$@"; do
            if [[ "$item" == "$needle" ]]; then
                return 0
            fi
        done

        return 1
    }

    is_system_library() {
        local dep="$1"
        [[ "$dep" == /System/Library/* || "$dep" == /usr/lib/* ]]
    }

    rewrite_dependency() {
        local binary="$1"
        local dep="$2"
        local base="$3"
        local replacement

        if [[ "$binary" == "$mpv_binary" ]]; then
            replacement="@executable_path/../../Frameworks/mpv/$base"
        else
            replacement="@loader_path/$base"
        fi

        install_name_tool -change "$dep" "$replacement" "$binary" 2>/dev/null || true
    }

    while ((${#queue[@]} > 0)); do
        binary="${queue[0]}"
        queue=("${queue[@]:1}")

        if contains_value "$binary" "${visited[@]}" || [[ ! -e "$binary" ]]; then
            continue
        fi
        visited+=("$binary")

        while IFS= read -r dep; do
            [[ -n "$dep" ]] || continue

            if [[ "$dep" != /* ]] || is_system_library "$dep"; then
                continue
            fi

            base="$(basename "$dep")"
            dest="$lib_dir/$base"

            if ! contains_value "$dest" "${copied[@]}"; then
                cp -fL "$dep" "$dest"
                chmod u+w "$dest"
                copied+=("$dest")
                queue+=("$dest")
                install_name_tool -id "@loader_path/$base" "$dest" 2>/dev/null || true
            fi

            rewrite_dependency "$binary" "$dep" "$base"
        done < <(otool -L "$binary" | awk 'NR > 1 {print $1}')
    done
else
    echo "Bundled mpv not found; packaged app will fall back to mpv on PATH"
fi

echo "Applying ad-hoc signature"
codesign --force --deep --sign - "$app_bundle"

dmg_path="$output_dir/$artifact_name.dmg"
zip_path="$output_dir/$artifact_name.zip"
rm -f "$dmg_path" "$zip_path"

echo "Creating $dmg_path"
hdiutil create -volname "Miru" -srcfolder "$app_bundle" -ov -format UDZO "$dmg_path"

echo "Creating $zip_path"
ditto -c -k --keepParent "$app_bundle" "$zip_path"

echo "Packaged:"
echo "$dmg_path"
echo "$zip_path"
