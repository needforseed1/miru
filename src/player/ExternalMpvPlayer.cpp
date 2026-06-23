#include "ExternalMpvPlayer.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>

namespace {
bool isAsahiLinux()
{
#if defined(Q_OS_LINUX) && (defined(__aarch64__) || defined(__arm64__))
    const QStringList paths = {
        QStringLiteral("/proc/device-tree/model"),
        QStringLiteral("/proc/device-tree/compatible"),
    };
    for (const QString &path : paths) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            continue;
        }
        const QByteArray data = file.readAll().toLower();
        if (data.contains("apple")) {
            return true;
        }
    }
#endif
    return false;
}
} // namespace

ExternalMpvPlayer::ExternalMpvPlayer(QObject *parent)
    : QObject(parent)
{
}

void ExternalMpvPlayer::play(const QString &url, const QString &title,
                             const QVariantMap &headers, const QStringList &subtitleUrls,
                             const QStringList &extraArgs)
{
    if (url.trimmed().isEmpty()) {
        emit errorOccurred(QStringLiteral("Cannot play an empty stream URL"));
        return;
    }

    const QString mpv = QStandardPaths::findExecutable(QStringLiteral("mpv"));
    if (mpv.isEmpty()) {
        emit errorOccurred(QStringLiteral("mpv was not found on PATH"));
        return;
    }

    QStringList args;
    args << QStringLiteral("--input-ipc-server=%1").arg(QDir::temp().absoluteFilePath(QStringLiteral("aiostreams-mpv.sock")));
    args << QStringLiteral("--save-position-on-quit=no");

    // We only ever hand mpv a direct HTTP URL, so disable the youtube-dl
    // hook. It does not run for a stream that opens natively, but it is
    // mpv's fallback when an open fails -- and debrid/Usenet links do
    // expire. Disabling it turns a dead link into an instant failure
    // instead of a multi-second yt-dlp probe that cannot help.
    args << QStringLiteral("--ytdl=no");

    // Keep a healthy read-ahead buffer for smooth network playback and
    // fail faster on dead sources (default network-timeout is 60s).
    // (These do not speed up a normal start: that is bounded by the
    // server's first-byte latency and the seeks mpv makes to read the
    // container index up front -- which is also what makes seeking fast.)
    args << QStringLiteral("--cache=yes");
    args << QStringLiteral("--demuxer-readahead-secs=20");
    args << QStringLiteral("--network-timeout=15");

    // Asahi Linux currently lacks the Vulkan video decode extension mpv probes
    // for HEVC. Avoid repeated failed hwaccel setup before software fallback.
    args << (isAsahiLinux() ? QStringLiteral("--hwdec=no") : QStringLiteral("--hwdec=auto-safe"));
    args << QStringLiteral("--vo=gpu-next");
    args << QStringLiteral("--gpu-api=vulkan");
    args << QStringLiteral("--target-colorspace-hint=yes");
    if (!title.trimmed().isEmpty()) {
        args << QStringLiteral("--force-media-title=%1").arg(title.trimmed());
    }
    for (auto it = headers.constBegin(); it != headers.constEnd(); ++it) {
        const QString value = it.value().toString();
        if (!value.isEmpty()) {
            args << QStringLiteral("--http-header-fields-append=%1: %2").arg(it.key(), value);
        }
    }

    // External subtitles (OpenSubtitles). The first one is auto-selected by mpv.
    for (const QString &subtitle : subtitleUrls) {
        if (!subtitle.trimmed().isEmpty()) {
            args << QStringLiteral("--sub-file=%1").arg(subtitle.trimmed());
        }
    }

    args << extraArgs;
    args << url.trimmed();

    if (!QProcess::startDetached(mpv, args)) {
        emit errorOccurred(QStringLiteral("Failed to start mpv"));
        return;
    }

    emit playbackStarted();
}
