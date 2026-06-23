#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantMap>

class ExternalMpvPlayer : public QObject
{
    Q_OBJECT

public:
    explicit ExternalMpvPlayer(QObject *parent = nullptr);

    Q_INVOKABLE void play(const QString &url, const QString &title,
                          const QVariantMap &headers = {},
                          const QStringList &subtitleUrls = {},
                          const QStringList &extraArgs = {});

signals:
    void playbackStarted();
    void errorOccurred(const QString &message);
};
