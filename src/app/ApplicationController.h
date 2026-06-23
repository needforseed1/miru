#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

#include "../player/ExternalMpvPlayer.h"
#include "../services/AIOStreamsClient.h"
#include "../services/CinemetaClient.h"
#include "../services/ImdbRatings.h"
#include "../services/SubtitlesClient.h"

class ApplicationController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList homeSections READ homeSections NOTIFY homeSectionsChanged)
    Q_PROPERTY(QVariantMap featured READ featured NOTIFY homeSectionsChanged)
    Q_PROPERTY(QVariantList searchResults READ searchResults NOTIFY searchResultsChanged)
    Q_PROPERTY(QVariantMap selectedMeta READ selectedMeta NOTIFY selectedMetaChanged)
    Q_PROPERTY(QVariantList streams READ streams NOTIFY streamsChanged)
    Q_PROPERTY(bool streamsLoading READ streamsLoading NOTIFY streamsLoadingChanged)
    Q_PROPERTY(QString aioStreamsUrl READ aioStreamsUrl WRITE setAioStreamsUrl NOTIFY aioStreamsUrlChanged)
    Q_PROPERTY(QString metadataUrl READ metadataUrl WRITE setMetadataUrl NOTIFY metadataUrlChanged)
    Q_PROPERTY(QString subtitleLanguage READ subtitleLanguage WRITE setSubtitleLanguage NOTIFY subtitleLanguageChanged)
    Q_PROPERTY(QString imdbRatingsUpdated READ imdbRatingsUpdated NOTIFY imdbRatingsUpdatedChanged)
    Q_PROPERTY(double uiScale READ uiScale WRITE setUiScale NOTIFY uiScaleChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit ApplicationController(QObject *parent = nullptr);

    QVariantList homeSections() const;
    QVariantMap featured() const;
    QVariantList searchResults() const;
    QVariantMap selectedMeta() const;
    QVariantList streams() const;
    bool streamsLoading() const;
    QString aioStreamsUrl() const;
    QString metadataUrl() const;
    QString subtitleLanguage() const;
    QString imdbRatingsUpdated() const;
    double uiScale() const;
    bool loading() const;
    QString statusMessage() const;

    Q_INVOKABLE void loadHome();
    Q_INVOKABLE void search(const QString &query);
    Q_INVOKABLE void clearSearch();
    Q_INVOKABLE void loadMeta(const QString &type, const QString &id);
    Q_INVOKABLE void loadStreams(const QString &type, const QString &id);
    Q_INVOKABLE void clearStreams();
    Q_INVOKABLE void playStream(int index);
    Q_INVOKABLE void refreshImdbRatings();
    Q_INVOKABLE void setAioStreamsUrl(const QString &url);
    void setMetadataUrl(const QString &url);
    void setSubtitleLanguage(const QString &language);
    void setUiScale(double scale);

signals:
    void homeSectionsChanged();
    void searchResultsChanged();
    void selectedMetaChanged();
    void streamsChanged();
    void streamsLoadingChanged();
    void aioStreamsUrlChanged();
    void metadataUrlChanged();
    void subtitleLanguageChanged();
    void imdbRatingsUpdatedChanged();
    void uiScaleChanged();
    void loadingChanged();
    void statusMessageChanged();

private:
    void setLoading(bool loading);
    void setStreamsLoading(bool loading);
    void setStatusMessage(const QString &message);
    void enrichEpisodeRatings();
    QVariantList withTitleRatings(const QVariantList &items) const;

    CinemetaClient m_cinemeta;
    AIOStreamsClient m_aioStreams;
    SubtitlesClient m_subtitles;
    ImdbRatings m_imdbRatings;
    ExternalMpvPlayer m_player;

    QVariantList m_homeSections; // [{ id, type, title, items }]
    QVariantList m_searchResults;
    QVariantMap m_selectedMeta;
    QVariantList m_streams;
    QVariantList m_currentSubtitles;
    QString m_aioStreamsUrl;
    QString m_metadataUrl;
    QString m_subtitleLanguage;
    double m_uiScale = 1.0;
    QString m_statusMessage;
    bool m_loading = false;
    bool m_streamsLoading = false;
    int m_homeRequestsPending = 0;
};
