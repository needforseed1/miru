#include "ExternalMpvPlayer.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLocalSocket>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>

namespace {
QString firstExecutablePath(const QStringList &candidates)
{
    for (const QString &candidate : candidates) {
        const QFileInfo info(candidate);
        if (info.exists() && info.isFile() && info.isExecutable()) {
            return info.absoluteFilePath();
        }
    }
    return {};
}

QString bundledMpvPath()
{
    const QDir appDir(QCoreApplication::applicationDirPath());
    QStringList candidates;

#if defined(Q_OS_MACOS)
    candidates << appDir.absoluteFilePath(QStringLiteral("../Resources/mpv/mpv"));
    candidates << appDir.absoluteFilePath(QStringLiteral("mpv"));
    candidates << appDir.absoluteFilePath(QStringLiteral("mpv/mpv"));
#elif defined(Q_OS_WIN)
    candidates << appDir.absoluteFilePath(QStringLiteral("mpv.exe"));
    candidates << appDir.absoluteFilePath(QStringLiteral("mpv/mpv.exe"));
#else
    candidates << appDir.absoluteFilePath(QStringLiteral("mpv"));
    candidates << appDir.absoluteFilePath(QStringLiteral("mpv/mpv"));
#endif

    return firstExecutablePath(candidates);
}

QString resolveMpvExecutable()
{
    const QString bundled = bundledMpvPath();
    if (!bundled.isEmpty()) {
        return bundled;
    }
    return QStandardPaths::findExecutable(QStringLiteral("mpv"));
}

QString bundledMpvResourcePath(const QString &relativePath)
{
    const QDir appDir(QCoreApplication::applicationDirPath());
    QStringList candidates;

#if defined(Q_OS_MACOS)
    candidates << appDir.absoluteFilePath(QStringLiteral("../Resources/mpv/%1").arg(relativePath));
#else
    candidates << appDir.absoluteFilePath(QStringLiteral("mpv/%1").arg(relativePath));
    candidates << appDir.absoluteFilePath(QStringLiteral("../share/stremio-linux/mpv/%1").arg(relativePath));
#endif

    for (const QString &candidate : candidates) {
        const QFileInfo info(candidate);
        if (info.exists()) {
            return info.absoluteFilePath();
        }
    }
    return {};
}

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

bool shouldForwardPlaybackHeader(const QString &key, const QString &value)
{
    return !key.trimmed().isEmpty()
        && !value.trimmed().isEmpty()
        && key.compare(QStringLiteral("Range"), Qt::CaseInsensitive) != 0;
}
} // namespace

ExternalMpvPlayer::ExternalMpvPlayer(QObject *parent)
    : QObject(parent)
{
}

bool ExternalMpvPlayer::play(const QString &url, const QString &title,
                                 const QVariantMap &headers, const QStringList &subtitleUrls,
                                 const QString &subtitleLanguage,
                                 bool enableHwdec, bool enableGpuNext, bool enableHdrHint,
                                 bool enableModernz, bool startFullscreen,
                                 const QStringList &extraArgs, double startSeconds, double startPercent,
                                 qulonglong windowId)
{
    resetWatcher(true);

    if (url.trimmed().isEmpty()) {
        emit errorOccurred(QStringLiteral("Cannot play an empty stream URL"));
        return false;
    }

    const QString mpv = resolveMpvExecutable();
    if (mpv.isEmpty()) {
        emit errorOccurred(QStringLiteral("mpv was not found in the app bundle or on PATH"));
        return false;
    }

    QStringList args;
    const QString socketPath = QDir::temp().absoluteFilePath(
        QStringLiteral("aiostreams-mpv-%1-%2.sock")
            .arg(QCoreApplication::applicationPid())
            .arg(QDateTime::currentMSecsSinceEpoch()));
    args << QStringLiteral("--input-ipc-server=%1").arg(socketPath);
    args << QStringLiteral("--save-position-on-quit=no");
    if (startFullscreen && windowId == 0) {
        args << QStringLiteral("--fullscreen=yes");
    }
    if (windowId > 0) {
        args << QStringLiteral("--wid=%1").arg(windowId);
        args << QStringLiteral("--force-window=yes");
    }
    if (startSeconds > 0.0) {
        args << QStringLiteral("--start=%1").arg(startSeconds, 0, 'f', 1);
    } else if (startPercent > 0.0 && startPercent < 100.0) {
        args << QStringLiteral("--start=%1%").arg(startPercent, 0, 'f', 1);
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
    if (enableModernz) {
        const QString modernzScript = bundledMpvResourcePath(QStringLiteral("modernz/modernz.lua"));
        if (!modernzScript.isEmpty()) {
            args << QStringLiteral("--osc=no");
            args << QStringLiteral("--osd-bar=no");
            args << QStringLiteral("--script-opts=modernz-download_button=no");
            args << QStringLiteral("--script=%1").arg(modernzScript);
            const QString modernzFonts = bundledMpvResourcePath(QStringLiteral("modernz/fonts"));
            if (!modernzFonts.isEmpty()) {
                args << QStringLiteral("--osd-fonts-dir=%1").arg(modernzFonts);
            }
        }
    }
    m_preferredSubtitleLanguage = subtitleLanguage.trimmed();
    if (!m_preferredSubtitleLanguage.isEmpty() && m_preferredSubtitleLanguage != QStringLiteral("off")) {
        args << QStringLiteral("--slang=%1").arg(m_preferredSubtitleLanguage);
    }
    for (auto it = headers.constBegin(); it != headers.constEnd(); ++it) {
        const QString value = it.value().toString();
        if (shouldForwardPlaybackHeader(it.key(), value)) {
            args << QStringLiteral("--http-header-fields-append=%1: %2")
                        .arg(it.key().trimmed(), value.trimmed());
        }
    }

    // External subtitles (OpenSubtitles) are NOT passed as --sub-file: mpv loads
    // those during the initial file open and blocks the first frame on them
    // (~1.4s on a slow start). Instead we remember them and add them over IPC
    // once mpv is up, so subtitle fetching runs off the playback-start path.
    m_pendingSubtitles.clear();
    for (const QString &subtitle : subtitleUrls) {
        if (!subtitle.trimmed().isEmpty()) {
            m_pendingSubtitles.append(subtitle.trimmed());
        }
    }

    args << extraArgs;
    args << url.trimmed();

    if (windowId > 0) {
        m_process = new QProcess(this);
        connect(m_process, &QProcess::finished, this, &ExternalMpvPlayer::finishPlayback);
        connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
            if (error == QProcess::FailedToStart) {
                emit errorOccurred(QStringLiteral("Failed to start embedded mpv"));
            }
        });
        m_process->start(mpv, args);
        if (!m_process->waitForStarted(3000)) {
            emit errorOccurred(QStringLiteral("Failed to start embedded mpv"));
            m_process->deleteLater();
            m_process = nullptr;
            return false;
        }
    } else if (!QProcess::startDetached(mpv, args)) {
        emit errorOccurred(QStringLiteral("Failed to start mpv"));
        return false;
    }

    startWatcher(socketPath);
    emit playbackStarted();
    return true;
}

void ExternalMpvPlayer::stop()
{
    sendCommand(QJsonArray{QStringLiteral("quit")});
    resetWatcher(true);
}

void ExternalMpvPlayer::setPaused(bool paused)
{
    sendCommand(QJsonArray{QStringLiteral("set_property"), QStringLiteral("pause"), paused});
}

void ExternalMpvPlayer::seek(double seconds)
{
    sendCommand(QJsonArray{QStringLiteral("seek"), seconds, QStringLiteral("absolute")});
}

void ExternalMpvPlayer::seekRelative(double seconds)
{
    sendCommand(QJsonArray{QStringLiteral("seek"), seconds, QStringLiteral("relative")});
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

    if (m_process) {
        QProcess *process = m_process;
        m_process = nullptr;
        process->disconnect(this);
        if (process->state() != QProcess::NotRunning) {
            process->terminate();
            if (!process->waitForFinished(1000)) {
                process->kill();
            }
        }
        process->deleteLater();
    }

    m_readBuffer.clear();
    m_socketPath.clear();
    m_pendingSubtitles.clear();
    m_preferredSubtitleLanguage.clear();
    m_connectAttempts = 0;
    m_lastPosition = 0.0;
    m_lastDuration = 0.0;
    m_paused = false;
    m_finishEmitted = false;
    m_playingEmitted = false;
    m_subtitleSelected = false;
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
        m_socket->write("{\"command\":[\"observe_property\",3,\"pause\"]}\n");
        m_socket->write("{\"command\":[\"observe_property\",4,\"eof-reached\"]}\n");
        m_socket->write("{\"command\":[\"observe_property\",5,\"track-list\"]}\n");

        // Add external subtitles now, off the first-frame path. mpv receives
        // the configured language, then track-list observation below selects
        // the matching external track once mpv exposes it.
        for (int i = 0; i < m_pendingSubtitles.size(); ++i) {
            const QString flag = (i == 0) ? QStringLiteral("select") : QStringLiteral("auto");
            const QString title = m_preferredSubtitleLanguage.isEmpty()
                ? QStringLiteral("External subtitle")
                : QStringLiteral("External %1").arg(m_preferredSubtitleLanguage);
            sendCommand(QJsonArray{QStringLiteral("sub-add"), m_pendingSubtitles.at(i), flag, title, m_preferredSubtitleLanguage});
        }
        m_pendingSubtitles.clear();
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
        if (object.value(QStringLiteral("event")).toString() == QStringLiteral("end-file")) {
            finishPlayback();
        }
        if (object.value(QStringLiteral("event")).toString() == QStringLiteral("property-change")) {
            const int id = object.value(QStringLiteral("id")).toInt();
            const QJsonValue data = object.value(QStringLiteral("data"));
            if (data.isDouble()) {
                if (id == 1) {
                    m_lastPosition = data.toDouble();
                    // First numeric time-pos means mpv has opened the stream and
                    // is decoding -- the real "now playing" moment, after the
                    // network open/buffering delay.
                    if (!m_playingEmitted) {
                        m_playingEmitted = true;
                        emit playbackPlaying();
                    }
                } else if (id == 2) {
                    m_lastDuration = data.toDouble();
                }
                emitProgressIfDue();
            } else if (id == 3 && data.isBool()) {
                const bool paused = data.toBool();
                if (m_paused != paused) {
                    m_paused = paused;
                    emit pauseChanged(paused);
                }
            } else if (id == 4 && data.isBool() && data.toBool()) {
                finishPlayback();
            } else if (id == 5 && data.isArray()) {
                selectPreferredSubtitle(data.toArray());
            }
        }

        newline = m_readBuffer.indexOf('\n');
    }
}

void ExternalMpvPlayer::selectPreferredSubtitle(const QJsonArray &tracks)
{
    if (m_subtitleSelected || m_preferredSubtitleLanguage.isEmpty()
        || m_preferredSubtitleLanguage.compare(QStringLiteral("off"), Qt::CaseInsensitive) == 0) {
        return;
    }

    int fallbackExternalSub = -1;
    for (const QJsonValue &value : tracks) {
        const QJsonObject track = value.toObject();
        if (track.value(QStringLiteral("type")).toString() != QStringLiteral("sub")) {
            continue;
        }
        const int id = track.value(QStringLiteral("id")).toInt(-1);
        if (id < 0) {
            continue;
        }
        const bool external = track.value(QStringLiteral("external")).toBool();
        if (!external) {
            continue;
        }
        if (fallbackExternalSub < 0) {
            fallbackExternalSub = id;
        }
        if (track.value(QStringLiteral("lang")).toString().compare(m_preferredSubtitleLanguage, Qt::CaseInsensitive) == 0) {
            sendCommand(QJsonArray{QStringLiteral("set_property"), QStringLiteral("sid"), id});
            m_subtitleSelected = true;
            return;
        }
    }

    if (fallbackExternalSub >= 0) {
        sendCommand(QJsonArray{QStringLiteral("set_property"), QStringLiteral("sid"), fallbackExternalSub});
        m_subtitleSelected = true;
    }
}

bool ExternalMpvPlayer::sendCommand(const QJsonArray &command)
{
    if (!m_socket || m_socket->state() != QLocalSocket::ConnectedState) {
        return false;
    }

    QJsonObject object;
    object.insert(QStringLiteral("command"), command);
    m_socket->write(QJsonDocument(object).toJson(QJsonDocument::Compact));
    m_socket->write("\n");
    m_socket->flush();
    return true;
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
