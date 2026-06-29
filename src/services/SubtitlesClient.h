#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QVariantList>

// Fetches subtitles from the OpenSubtitles v3 Stremio addon, the same
// source Stremio/Nuvio use. Returns a list of {url, lang, id} maps.
class SubtitlesClient : public QObject
{
    Q_OBJECT

public:
    explicit SubtitlesClient(QObject *parent = nullptr);

    void fetchSubtitles(const QString &type, const QString &id);

signals:
    void subtitlesReady(const QString &type, const QString &id, const QVariantList &subtitles);
    void errorOccurred(const QString &message);

private:
    QNetworkAccessManager m_network;
    QString m_baseUrl = QStringLiteral("https://opensubtitles-v3.strem.io");
};
