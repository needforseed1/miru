#pragma once

#include <QObject>
#include <QByteArray>
#include <QElapsedTimer>
#include <QStringList>
#include <QVariantMap>

class QLocalSocket;

class ExternalMpvPlayer : public QObject
{
    Q_OBJECT

public:
    explicit ExternalMpvPlayer(QObject *parent = nullptr);

    Q_INVOKABLE void play(const QString &url, const QString &title,
                          const QVariantMap &headers = {},
                          const QStringList &subtitleUrls = {},
                          bool enableHwdec = true,
                          bool enableGpuNext = false,
                          bool enableHdrHint = false,
                          const QStringList &extraArgs = {},
                          double startSeconds = 0.0);

signals:
    void playbackStarted();
    void positionChanged(double position, double duration);
    void playbackFinished(double position, double duration);
    void errorOccurred(const QString &message);

private:
    void resetWatcher(bool emitFinished);
    void startWatcher(const QString &socketPath);
    void retryConnect();
    void handleReadyRead();
    void emitProgressIfDue(bool force = false);
    void finishPlayback();

    QLocalSocket *m_socket = nullptr;
    QByteArray m_readBuffer;
    QString m_socketPath;
    int m_connectAttempts = 0;
    int m_generation = 0;
    double m_lastPosition = 0.0;
    double m_lastDuration = 0.0;
    bool m_finishEmitted = false;
    QElapsedTimer m_progressTimer;
};
