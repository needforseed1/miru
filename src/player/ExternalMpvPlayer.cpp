#include "ExternalMpvPlayer.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLocalSocket>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>

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
                               bool enableHwdec, bool enableGpuNext, bool enableHdrHint,
                               const QStringList &extraArgs, double startSeconds)
{
    resetWatcher(true);

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
    const QString socketPath = QDir::temp().absoluteFilePath(
        QStringLiteral("aiostreams-mpv-%1-%2.sock")
            .arg(QCoreApplication::applicationPid())
            .arg(QDateTime::currentMSecsSinceEpoch()));
    args << QStringLiteral("--input-ipc-server=%1").arg(socketPath);
    args << QStringLiteral("--save-position-on-quit=no");
    if (startSeconds > 0.0) {
        args << QStringLiteral("--start=%1").arg(startSeconds, 0, 'f', 1);
    }

    // We only ever hand mpv a direct HTTP URL, so disable the youtube-dl
    // hook. It does not run for a stream that opens natively, but it is
    // mpv's fallback when an open fails -- and debrid/Usenet links do
    // expire. Disabling it turns a dead link into an instant failure
    // instead of a multi-second yt-dlp probe that cannot help.
    args << QStringLiteral("--ytdl=no");

    // Keep a healthy read-ahead buffer for smooth network playback. Debrid
    // REMUX links can be slow to return the first HTTP response when cold, so
    // keep mpv's default 60s network timeout instead of failing too early.
    // (These do not speed up a normal start: that is bounded by the
    // server's first-byte latency and the seeks mpv makes to read the
    // container index up front -- which is also what makes seeking fast.)
    args << QStringLiteral("--cache=yes");
    args << QStringLiteral("--demuxer-readahead-secs=20");
    args << QStringLiteral("--network-timeout=60");

    // Large REMUX streams can briefly starve PipeWire while demux/decode catches
    // up. A modest audio buffer smooths those dips without hiding real stalls.
    args << QStringLiteral("--audio-buffer=1.0");

    if (isAsahiLinux()) {
        // Asahi Linux currently lacks the Vulkan video decode extension mpv
        // probes for HEVC. Avoid repeated failed hwaccel setup before software
        // fallback. User custom args are appended last and can still override.
        args << QStringLiteral("--hwdec=no");
    } else if (enableHwdec) {
        args << QStringLiteral("--hwdec=auto-safe");
    }
    if (enableGpuNext) {
        args << QStringLiteral("--vo=gpu-next");
    }
    if (enableHdrHint) {
        args << QStringLiteral("--gpu-api=vulkan");
        args << QStringLiteral("--target-colorspace-hint=yes");
    }
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

    startWatcher(socketPath);
    emit playbackStarted();
}

void ExternalMpvPlayer::resetWatcher(bool emitFinished)
{
    ++m_generation;
    if (emitFinished) {
        finishPlayback();
    }

    if (m_socket) {
        m_socket->disconnect(this);
        m_socket->abort();
        m_socket->deleteLater();
        m_socket = nullptr;
    }

    m_readBuffer.clear();
    m_socketPath.clear();
    m_connectAttempts = 0;
    m_lastPosition = 0.0;
    m_lastDuration = 0.0;
    m_finishEmitted = false;
    m_progressTimer.invalidate();
}

void ExternalMpvPlayer::startWatcher(const QString &socketPath)
{
    m_socketPath = socketPath;
    m_connectAttempts = 0;
    m_finishEmitted = false;
    m_progressTimer.invalidate();
    retryConnect();
}

void ExternalMpvPlayer::retryConnect()
{
    const int generation = m_generation;
    if (m_socketPath.isEmpty()) {
        return;
    }

    if (m_socket) {
        m_socket->disconnect(this);
        m_socket->abort();
        m_socket->deleteLater();
    }

    m_socket = new QLocalSocket(this);
    connect(m_socket, &QLocalSocket::connected, this, [this, generation]() {
        if (generation != m_generation || !m_socket) {
            return;
        }
        m_socket->write("{\"command\":[\"observe_property\",1,\"time-pos\"]}\n");
        m_socket->write("{\"command\":[\"observe_property\",2,\"duration\"]}\n");
        m_socket->flush();
    });
    connect(m_socket, &QLocalSocket::readyRead, this, &ExternalMpvPlayer::handleReadyRead);
    connect(m_socket, &QLocalSocket::disconnected, this, &ExternalMpvPlayer::finishPlayback);
    connect(m_socket, &QLocalSocket::readChannelFinished, this, &ExternalMpvPlayer::finishPlayback);
    connect(m_socket, &QLocalSocket::errorOccurred, this, [this, generation](QLocalSocket::LocalSocketError) {
        if (generation != m_generation || m_socketPath.isEmpty()) {
            return;
        }
        if (++m_connectAttempts >= 30) {
            if (m_socket) {
                m_socket->deleteLater();
                m_socket = nullptr;
            }
            m_socketPath.clear();
            return;
        }
        QTimer::singleShot(100, this, [this, generation]() {
            if (generation == m_generation) {
                retryConnect();
            }
        });
    });

    m_socket->connectToServer(m_socketPath);
}

void ExternalMpvPlayer::handleReadyRead()
{
    if (!m_socket) {
        return;
    }

    m_readBuffer.append(m_socket->readAll());
    int newline = m_readBuffer.indexOf('\n');
    while (newline >= 0) {
        const QByteArray line = m_readBuffer.left(newline).trimmed();
        m_readBuffer.remove(0, newline + 1);

        const QJsonDocument doc = QJsonDocument::fromJson(line);
        const QJsonObject object = doc.object();
        if (object.value(QStringLiteral("event")).toString() == QStringLiteral("property-change")) {
            const int id = object.value(QStringLiteral("id")).toInt();
            const QJsonValue data = object.value(QStringLiteral("data"));
            if (data.isDouble()) {
                if (id == 1) {
                    m_lastPosition = data.toDouble();
                } else if (id == 2) {
                    m_lastDuration = data.toDouble();
                }
                emitProgressIfDue();
            }
        }

        newline = m_readBuffer.indexOf('\n');
    }
}

void ExternalMpvPlayer::emitProgressIfDue(bool force)
{
    if (m_lastDuration <= 0.0) {
        return;
    }
    if (!m_progressTimer.isValid()) {
        m_progressTimer.start();
        emit positionChanged(m_lastPosition, m_lastDuration);
        return;
    }
    if (force || m_progressTimer.elapsed() >= 5000) {
        m_progressTimer.restart();
        emit positionChanged(m_lastPosition, m_lastDuration);
    }
}

void ExternalMpvPlayer::finishPlayback()
{
    if (m_finishEmitted) {
        return;
    }
    m_finishEmitted = true;
    emitProgressIfDue(true);
    emit playbackFinished(m_lastPosition, m_lastDuration);
}
