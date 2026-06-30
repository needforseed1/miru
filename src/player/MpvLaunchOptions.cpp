#include "MpvLaunchOptions.h"

namespace {
bool shouldForwardPlaybackHeader(const QString &key, const QString &value)
{
    return !key.trimmed().isEmpty()
        && !value.trimmed().isEmpty()
        && key.compare(QStringLiteral("Range"), Qt::CaseInsensitive) != 0;
}
} // namespace

QStringList buildMpvArguments(const MpvLaunchOptions &options)
{
    QStringList args;
    args << QStringLiteral("--input-ipc-server=%1").arg(options.socketPath);
    args << QStringLiteral("--save-position-on-quit=no");
    if (options.startFullscreen) {
        args << QStringLiteral("--fullscreen=yes");
    }
    if (options.startSeconds > 0.0) {
        args << QStringLiteral("--start=%1").arg(options.startSeconds, 0, 'f', 1);
    } else if (options.startPercent > 0.0 && options.startPercent < 100.0) {
        args << QStringLiteral("--start=%1%").arg(options.startPercent, 0, 'f', 1);
    }

    args << QStringLiteral("--ytdl=no");
    args << QStringLiteral("--cache=yes");
    args << QStringLiteral("--demuxer-readahead-secs=20");
    args << QStringLiteral("--network-timeout=60");
    args << QStringLiteral("--audio-buffer=1.0");

    if (options.asahiLinux) {
        args << QStringLiteral("--hwdec=no");
    } else if (options.enableHwdec) {
        args << QStringLiteral("--hwdec=auto-safe");
    }
    if (options.enableGpuNext) {
        args << QStringLiteral("--vo=gpu-next");
    }
    if (options.enableHdrHint) {
        args << QStringLiteral("--gpu-api=vulkan");
        args << QStringLiteral("--target-colorspace-hint=yes");
    }
    if (!options.title.trimmed().isEmpty()) {
        args << QStringLiteral("--force-media-title=%1").arg(options.title.trimmed());
    }
    if (options.enableModernz && !options.modernzScriptPath.isEmpty()) {
        args << QStringLiteral("--osc=no");
        args << QStringLiteral("--osd-bar=no");
        args << QStringLiteral("--script-opts=modernz-download_button=no,modernz-persistent_progress=no,modernz-persistent_buffer=no");
        args << QStringLiteral("--script=%1").arg(options.modernzScriptPath);
        if (!options.modernzFontsPath.isEmpty()) {
            args << QStringLiteral("--osd-fonts-dir=%1").arg(options.modernzFontsPath);
        }
    }
    const QString preferredSubtitleLanguage = options.subtitleLanguage.trimmed();
    if (!preferredSubtitleLanguage.isEmpty() && preferredSubtitleLanguage != QStringLiteral("off")) {
        args << QStringLiteral("--slang=%1").arg(preferredSubtitleLanguage);
    }
    for (auto it = options.headers.constBegin(); it != options.headers.constEnd(); ++it) {
        const QString value = it.value().toString();
        if (shouldForwardPlaybackHeader(it.key(), value)) {
            args << QStringLiteral("--http-header-fields-append=%1: %2")
                        .arg(it.key().trimmed(), value.trimmed());
        }
    }

    args << options.extraArgs;
    args << options.url.trimmed();
    return args;
}
