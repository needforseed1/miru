#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QSet>
#include <QVariantList>
#include <QVariantMap>

#include "../player/ExternalMpvPlayer.h"
#include "../services/AIOStreamsClient.h"
#include "../services/CinemetaClient.h"
#include "../services/ImdbRatings.h"
#include "../services/SubtitlesClient.h"
#include "../services/TraktClient.h"
#include "WatchHistory.h"

class QNetworkReply;

class ApplicationController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList homeSections READ homeSections NOTIFY homeSectionsChanged)
    Q_PROPERTY(QVariantMap featured READ featured NOTIFY homeSectionsChanged)
    Q_PROPERTY(QVariantList continueWatching READ continueWatching NOTIFY continueWatchingChanged)
    Q_PROPERTY(QVariantList nextUp READ nextUp NOTIFY nextUpChanged)
    Q_PROPERTY(QVariantList searchResults READ searchResults NOTIFY searchResultsChanged)
    Q_PROPERTY(QVariantMap selectedMeta READ selectedMeta NOTIFY selectedMetaChanged)
    Q_PROPERTY(QVariantList streams READ streams NOTIFY streamsChanged)
    Q_PROPERTY(bool streamsLoading READ streamsLoading NOTIFY streamsLoadingChanged)
    Q_PROPERTY(QString aioStreamsUrl READ aioStreamsUrl WRITE setAioStreamsUrl NOTIFY aioStreamsUrlChanged)
    Q_PROPERTY(QString metadataUrl READ metadataUrl WRITE setMetadataUrl NOTIFY metadataUrlChanged)
    Q_PROPERTY(QString subtitleLanguage READ subtitleLanguage WRITE setSubtitleLanguage NOTIFY subtitleLanguageChanged)
    Q_PROPERTY(QString imdbRatingsUpdated READ imdbRatingsUpdated NOTIFY imdbRatingsUpdatedChanged)
    Q_PROPERTY(double uiScale READ uiScale WRITE setUiScale NOTIFY uiScaleChanged)
    Q_PROPERTY(bool showPosterRatings READ showPosterRatings WRITE setShowPosterRatings NOTIFY showPosterRatingsChanged)
    Q_PROPERTY(bool mpvHardwareDecoding READ mpvHardwareDecoding WRITE setMpvHardwareDecoding NOTIFY mpvHardwareDecodingChanged)
    Q_PROPERTY(bool mpvGpuNext READ mpvGpuNext WRITE setMpvGpuNext NOTIFY mpvGpuNextChanged)
    Q_PROPERTY(bool mpvHdrHint READ mpvHdrHint WRITE setMpvHdrHint NOTIFY mpvHdrHintChanged)
    Q_PROPERTY(bool mpvUosc READ mpvUosc WRITE setMpvUosc NOTIFY mpvUoscChanged)
    Q_PROPERTY(bool mpvFullscreen READ mpvFullscreen WRITE setMpvFullscreen NOTIFY mpvFullscreenChanged)
    Q_PROPERTY(QString mpvExtraArgs READ mpvExtraArgs WRITE setMpvExtraArgs NOTIFY mpvExtraArgsChanged)
    Q_PROPERTY(QString playerMode READ playerMode WRITE setPlayerMode NOTIFY playerModeChanged)
    Q_PROPERTY(QString traktClientId READ traktClientId WRITE setTraktClientId NOTIFY traktChanged)
    Q_PROPERTY(QString traktClientSecret READ traktClientSecret WRITE setTraktClientSecret NOTIFY traktChanged)
    Q_PROPERTY(QString traktStatus READ traktStatus NOTIFY traktChanged)
    Q_PROPERTY(QString traktUserCode READ traktUserCode NOTIFY traktChanged)
    Q_PROPERTY(QString traktVerificationUrl READ traktVerificationUrl NOTIFY traktChanged)
    Q_PROPERTY(QString traktUsername READ traktUsername NOTIFY traktChanged)
    Q_PROPERTY(bool traktConnected READ traktConnected NOTIFY traktChanged)
    Q_PROPERTY(bool traktAuthPending READ traktAuthPending NOTIFY traktChanged)
    Q_PROPERTY(bool traktBusy READ traktBusy NOTIFY traktChanged)
    Q_PROPERTY(bool playbackActive READ playbackActive NOTIFY playbackStateChanged)
    Q_PROPERTY(bool playbackBuffering READ playbackBuffering NOTIFY playbackStateChanged)
    Q_PROPERTY(bool playbackEmbedded READ playbackEmbedded NOTIFY playbackStateChanged)
    Q_PROPERTY(bool playbackPaused READ playbackPaused NOTIFY playbackStateChanged)
    Q_PROPERTY(QString playbackTitle READ playbackTitle NOTIFY playbackStateChanged)
    Q_PROPERTY(double playbackPosition READ playbackPosition NOTIFY playbackPositionChanged)
    Q_PROPERTY(double playbackDuration READ playbackDuration NOTIFY playbackPositionChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit ApplicationController(QObject *parent = nullptr);

    QVariantList homeSections() const;
    QVariantMap featured() const;
    QVariantList continueWatching() const;
    QVariantList nextUp() const;
    QVariantList searchResults() const;
    QVariantMap selectedMeta() const;
    QVariantList streams() const;
    bool streamsLoading() const;
    QString aioStreamsUrl() const;
    QString metadataUrl() const;
    QString subtitleLanguage() const;
    QString imdbRatingsUpdated() const;
    double uiScale() const;
    bool showPosterRatings() const;
    bool mpvHardwareDecoding() const;
    bool mpvGpuNext() const;
    bool mpvHdrHint() const;
    bool mpvUosc() const;
    bool mpvFullscreen() const;
    QString mpvExtraArgs() const;
    QString playerMode() const;
    QString traktClientId() const;
    QString traktClientSecret() const;
    QString traktStatus() const;
    QString traktUserCode() const;
    QString traktVerificationUrl() const;
    QString traktUsername() const;
    bool traktConnected() const;
    bool traktAuthPending() const;
    bool traktBusy() const;
    bool playbackActive() const;
    bool playbackBuffering() const;
    bool playbackEmbedded() const;
    bool playbackPaused() const;
    QString playbackTitle() const;
    double playbackPosition() const;
    double playbackDuration() const;
    bool loading() const;
    QString statusMessage() const;

    Q_INVOKABLE void loadHome();
    Q_INVOKABLE void search(const QString &query);
    Q_INVOKABLE void clearSearch();
    Q_INVOKABLE void loadMeta(const QString &type, const QString &id);
    Q_INVOKABLE void loadStreams(const QString &type, const QString &id);
    Q_INVOKABLE void clearStreams();
    Q_INVOKABLE void playStream(int index);
    Q_INVOKABLE bool playStreamEmbedded(int index, qulonglong windowId);
    Q_INVOKABLE void resumeContinueWatching(const QString &key);
    Q_INVOKABLE bool resumeContinueWatchingEmbedded(const QString &key, qulonglong windowId);
    Q_INVOKABLE void removeContinueWatching(const QString &key);
    Q_INVOKABLE void setPendingRemoteResume(const QString &type, const QString &id, double progressPercent);
    Q_INVOKABLE void stopPlayback();
    Q_INVOKABLE void setPlaybackPaused(bool paused);
    Q_INVOKABLE void seekPlayback(double seconds);
    Q_INVOKABLE void seekPlaybackRelative(double seconds);
    Q_INVOKABLE void refreshImdbRatings();
    Q_INVOKABLE void setAioStreamsUrl(const QString &url);
    void setMetadataUrl(const QString &url);
    void setSubtitleLanguage(const QString &language);
    void setUiScale(double scale);
    void setShowPosterRatings(bool enabled);
    void setMpvHardwareDecoding(bool enabled);
    void setMpvGpuNext(bool enabled);
    void setMpvHdrHint(bool enabled);
    void setMpvUosc(bool enabled);
    void setMpvFullscreen(bool enabled);
    void setMpvExtraArgs(const QString &args);
    void setPlayerMode(const QString &mode);
    void setTraktClientId(const QString &clientId);
    void setTraktClientSecret(const QString &clientSecret);
    Q_INVOKABLE void connectTrakt();
    Q_INVOKABLE void cancelTraktAuth();
    Q_INVOKABLE void disconnectTrakt();
    Q_INVOKABLE void openTraktVerificationUrl();
    Q_INVOKABLE void copyTraktUserCode();

signals:
    void homeSectionsChanged();
    void continueWatchingChanged();
    void nextUpChanged();
    void searchResultsChanged();
    void selectedMetaChanged();
    void streamsChanged();
    void streamsLoadingChanged();
    void aioStreamsUrlChanged();
    void metadataUrlChanged();
    void subtitleLanguageChanged();
    void imdbRatingsUpdatedChanged();
    void uiScaleChanged();
    void showPosterRatingsChanged();
    void mpvHardwareDecodingChanged();
    void mpvGpuNextChanged();
    void mpvHdrHintChanged();
    void mpvUoscChanged();
    void mpvFullscreenChanged();
    void mpvExtraArgsChanged();
    void playerModeChanged();
    void traktChanged();
    void playbackStateChanged();
    void playbackPositionChanged();
    void loadingChanged();
    void statusMessageChanged();

private:
    bool playStreamWithWindow(int index, qulonglong windowId);
    bool resumeContinueWatchingWithWindow(const QString &key, qulonglong windowId);
    bool startPlayback(const QString &url, const QString &title, const QVariantMap &headers,
                       const QStringList &subtitleUrls, double startSeconds, double startPercent,
                       qulonglong windowId);
    void setPlaybackState(bool active, bool embedded, const QString &title = QString());
    // Warm the on-demand backend's tail range (MKV Cues live there) in parallel
    // with mpv's open, so its seek-to-EOF index read is served from a warm
    // backend cache instead of a cold ~9s Usenet fetch.
    void prewarmStreamTail(const QString &url, const QVariantMap &headers);
    void abortStreamPrewarm();
    void setLoading(bool loading);
    void setStreamsLoading(bool loading);
    void setStatusMessage(const QString &message);
    void enrichEpisodeRatings();
    QVariantList withTitleRatings(const QVariantList &items) const;
    QVariantMap mediaForStreamRequest(const QString &type, const QString &id) const;
    QVariantMap currentPlaybackMedia(const QVariantMap &stream) const;
    void hydrateTraktResumeMetadata();

    CinemetaClient m_cinemeta;
    CinemetaClient m_resumeMetadata;
    AIOStreamsClient m_aioStreams;
    SubtitlesClient m_subtitles;
    ImdbRatings m_imdbRatings;
    TraktClient m_trakt;
    WatchHistory m_watchHistory;
    ExternalMpvPlayer m_player;
    QNetworkAccessManager m_streamPrewarm;
    QNetworkReply *m_streamPrewarmReply = nullptr;

    QVariantList m_homeSections; // [{ id, type, title, items }]
    QVariantList m_searchResults;
    QString m_searchQuery;
    QVariantMap m_selectedMeta;
    QVariantList m_streams;
    QVariantList m_currentSubtitles;
    QVariantMap m_streamMedia;
    QVariantMap m_currentPlaybackMedia;
    QSet<QString> m_resumeMetadataRequests;
    QString m_pendingRemoteResumeType;
    QString m_pendingRemoteResumeId;
    double m_pendingRemoteResumePercent = 0.0;
    QString m_aioStreamsUrl;
    QString m_metadataUrl;
    QString m_subtitleLanguage;
    double m_uiScale = 1.0;
    bool m_showPosterRatings = true;
    bool m_mpvHardwareDecoding = true;
    bool m_mpvGpuNext = false;
    bool m_mpvHdrHint = false;
    bool m_mpvUosc = true;
    bool m_mpvFullscreen = true;
    QString m_mpvExtraArgs;
    QString m_playerMode = QStringLiteral("external");
    bool m_playbackActive = false;
    bool m_playbackBuffering = false;
    bool m_playbackEmbedded = false;
    bool m_playbackPaused = false;
    QString m_playbackTitle;
    double m_playbackPosition = 0.0;
    double m_playbackDuration = 0.0;
    QString m_statusMessage;
    bool m_loading = false;
    bool m_streamsLoading = false;
    int m_homeRequestsPending = 0;
    int m_searchRequestsPending = 0;
};
