#pragma once

#include <QHash>
#include <QNetworkAccessManager>
#include <QObject>
#include <QSet>
#include <QTimer>
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
    Q_PROPERTY(QVariantList sourceHealth READ sourceHealth NOTIFY sourceHealthChanged)
    Q_PROPERTY(QString aioStreamsUrl READ aioStreamsUrl WRITE setAioStreamsUrl NOTIFY aioStreamsUrlChanged)
    Q_PROPERTY(QString metadataUrl READ metadataUrl WRITE setMetadataUrl NOTIFY metadataUrlChanged)
    Q_PROPERTY(QString subtitleLanguage READ subtitleLanguage WRITE setSubtitleLanguage NOTIFY subtitleLanguageChanged)
    Q_PROPERTY(QString imdbRatingsUpdated READ imdbRatingsUpdated NOTIFY imdbRatingsUpdatedChanged)
    Q_PROPERTY(double uiScale READ uiScale WRITE setUiScale NOTIFY uiScaleChanged)
    Q_PROPERTY(bool showPosterRatings READ showPosterRatings WRITE setShowPosterRatings NOTIFY showPosterRatingsChanged)
    Q_PROPERTY(QString uiMainColor READ uiMainColor WRITE setUiMainColor NOTIFY themeColorsChanged)
    Q_PROPERTY(QString uiProgressStartColor READ uiProgressStartColor WRITE setUiProgressStartColor NOTIFY themeColorsChanged)
    Q_PROPERTY(QString uiProgressEndColor READ uiProgressEndColor WRITE setUiProgressEndColor NOTIFY themeColorsChanged)
    Q_PROPERTY(bool mpvHardwareDecoding READ mpvHardwareDecoding WRITE setMpvHardwareDecoding NOTIFY mpvHardwareDecodingChanged)
    Q_PROPERTY(bool mpvGpuNext READ mpvGpuNext WRITE setMpvGpuNext NOTIFY mpvGpuNextChanged)
    Q_PROPERTY(bool mpvHdrHint READ mpvHdrHint WRITE setMpvHdrHint NOTIFY mpvHdrHintChanged)
    Q_PROPERTY(bool mpvModernz READ mpvModernz WRITE setMpvModernz NOTIFY mpvModernzChanged)
    Q_PROPERTY(bool mpvFullscreen READ mpvFullscreen WRITE setMpvFullscreen NOTIFY mpvFullscreenChanged)
    Q_PROPERTY(QString mpvExtraArgs READ mpvExtraArgs WRITE setMpvExtraArgs NOTIFY mpvExtraArgsChanged)
    Q_PROPERTY(QString traktClientId READ traktClientId WRITE setTraktClientId NOTIFY traktChanged)
    Q_PROPERTY(QString traktClientSecret READ traktClientSecret WRITE setTraktClientSecret NOTIFY traktChanged)
    Q_PROPERTY(QString traktStatus READ traktStatus NOTIFY traktChanged)
    Q_PROPERTY(QString traktUserCode READ traktUserCode NOTIFY traktChanged)
    Q_PROPERTY(QString traktVerificationUrl READ traktVerificationUrl NOTIFY traktChanged)
    Q_PROPERTY(QString traktUsername READ traktUsername NOTIFY traktChanged)
    Q_PROPERTY(bool traktConnected READ traktConnected NOTIFY traktChanged)
    Q_PROPERTY(bool traktAuthPending READ traktAuthPending NOTIFY traktChanged)
    Q_PROPERTY(bool traktBusy READ traktBusy NOTIFY traktChanged)
    Q_PROPERTY(bool autoAdvanceEnabled READ autoAdvanceEnabled WRITE setAutoAdvanceEnabled NOTIFY autoAdvanceEnabledChanged)
    Q_PROPERTY(bool autoAdvanceCloseMpv READ autoAdvanceCloseMpv WRITE setAutoAdvanceCloseMpv NOTIFY autoAdvanceCloseMpvChanged)
    Q_PROPERTY(bool autoAdvanceActive READ autoAdvanceActive NOTIFY autoAdvanceChanged)
    Q_PROPERTY(QVariantMap autoAdvanceMedia READ autoAdvanceMedia NOTIFY autoAdvanceChanged)
    Q_PROPERTY(int autoAdvanceSecondsLeft READ autoAdvanceSecondsLeft NOTIFY autoAdvanceChanged)
    Q_PROPERTY(QString autoAdvanceReleaseTitle READ autoAdvanceReleaseTitle NOTIFY autoAdvanceChanged)
    Q_PROPERTY(bool playbackActive READ playbackActive NOTIFY playbackStateChanged)
    Q_PROPERTY(bool playbackBuffering READ playbackBuffering NOTIFY playbackStateChanged)
    Q_PROPERTY(bool playbackPaused READ playbackPaused NOTIFY playbackStateChanged)
    Q_PROPERTY(QString playbackTitle READ playbackTitle NOTIFY playbackStateChanged)
    Q_PROPERTY(QVariantMap playbackMedia READ playbackMedia NOTIFY playbackStateChanged)
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
    QVariantList sourceHealth() const;
    QString aioStreamsUrl() const;
    QString metadataUrl() const;
    QString subtitleLanguage() const;
    QString imdbRatingsUpdated() const;
    double uiScale() const;
    bool showPosterRatings() const;
    QString uiMainColor() const;
    QString uiProgressStartColor() const;
    QString uiProgressEndColor() const;
    bool mpvHardwareDecoding() const;
    bool mpvGpuNext() const;
    bool mpvHdrHint() const;
    bool mpvModernz() const;
    bool mpvFullscreen() const;
    QString mpvExtraArgs() const;
    QString traktClientId() const;
    QString traktClientSecret() const;
    QString traktStatus() const;
    QString traktUserCode() const;
    QString traktVerificationUrl() const;
    QString traktUsername() const;
    bool traktConnected() const;
    bool traktAuthPending() const;
    bool traktBusy() const;
    bool autoAdvanceEnabled() const;
    bool autoAdvanceCloseMpv() const;
    bool autoAdvanceActive() const;
    QVariantMap autoAdvanceMedia() const;
    int autoAdvanceSecondsLeft() const;
    QString autoAdvanceReleaseTitle() const;
    bool playbackActive() const;
    bool playbackBuffering() const;
    bool playbackPaused() const;
    QString playbackTitle() const;
    QVariantMap playbackMedia() const;
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
    Q_INVOKABLE void resumeContinueWatching(const QString &key);
    Q_INVOKABLE void removeContinueWatching(const QString &key);
    Q_INVOKABLE void setPendingRemoteResume(const QString &type, const QString &id, double progressPercent);
    Q_INVOKABLE void stopPlayback();
    Q_INVOKABLE void cancelAutoAdvance();
    Q_INVOKABLE void playAutoAdvanceNow();
    void setAutoAdvanceEnabled(bool enabled);
    void setAutoAdvanceCloseMpv(bool enabled);
    Q_INVOKABLE void setPlaybackPaused(bool paused);
    Q_INVOKABLE void seekPlayback(double seconds);
    Q_INVOKABLE void seekPlaybackRelative(double seconds);
    Q_INVOKABLE void refreshImdbRatings();
    Q_INVOKABLE void setAioStreamsUrl(const QString &url);
    void setMetadataUrl(const QString &url);
    void setSubtitleLanguage(const QString &language);
    void setUiScale(double scale);
    void setShowPosterRatings(bool enabled);
    void setUiMainColor(const QString &color);
    void setUiProgressStartColor(const QString &color);
    void setUiProgressEndColor(const QString &color);
    Q_INVOKABLE void setThemeColors(const QString &mainColor, const QString &progressStartColor, const QString &progressEndColor);
    Q_INVOKABLE void resetThemeColors();
    void setMpvHardwareDecoding(bool enabled);
    void setMpvGpuNext(bool enabled);
    void setMpvHdrHint(bool enabled);
    void setMpvModernz(bool enabled);
    void setMpvFullscreen(bool enabled);
    void setMpvExtraArgs(const QString &args);
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
    void sourceHealthChanged();
    void aioStreamsUrlChanged();
    void metadataUrlChanged();
    void subtitleLanguageChanged();
    void imdbRatingsUpdatedChanged();
    void uiScaleChanged();
    void showPosterRatingsChanged();
    void themeColorsChanged();
    void mpvHardwareDecodingChanged();
    void mpvGpuNextChanged();
    void mpvHdrHintChanged();
    void mpvModernzChanged();
    void mpvFullscreenChanged();
    void mpvExtraArgsChanged();
    void traktChanged();
    void autoAdvanceEnabledChanged();
    void autoAdvanceCloseMpvChanged();
    void autoAdvanceChanged();
    void playbackStateChanged();
    void playbackPositionChanged();
    void loadingChanged();
    void statusMessageChanged();

private:
    bool startPlayback(const QVariantMap &playbackMedia,
                       const QString &url, const QString &title, const QVariantMap &headers,
                       const QStringList &subtitleUrls, double startSeconds, double startPercent);
    void setPlaybackState(bool active, const QString &title = QString());
    // Warm the on-demand backend's tail range (MKV Cues live there) in parallel
    // with mpv's open, so its seek-to-EOF index read is served from a warm
    // backend cache instead of a cold ~9s Usenet fetch.
    void prewarmStreamTail(const QString &url, const QVariantMap &headers);
    void abortStreamPrewarm();
    void abortStreamProbe();
    void setLoading(bool loading);
    void setStreamsLoading(bool loading);
    void setStatusMessage(const QString &message);
    void enrichEpisodeRatings();
    QVariantList withTitleRatings(const QVariantList &items) const;
    QVariantMap mediaForStreamRequest(const QString &type, const QString &id) const;
    QVariantMap currentPlaybackMedia(const QVariantMap &stream) const;
    void hydrateTraktResumeMetadata();
    void rebuildLocalNextUp();
    QVariantMap seriesMeta(const QString &baseId) const;
    QStringList preferredSubtitleUrls() const;
    bool maybeStartAutoAdvance(const QVariantMap &media, double position, double duration);
    bool startAutoAdvanceForMeta(const QString &baseId, const QVariantMap &meta, int season, int episode);
    void launchAutoAdvance();

    CinemetaClient m_cinemeta;
    CinemetaClient m_resumeMetadata;
    AIOStreamsClient m_aioStreams;
    AIOStreamsClient m_autoAdvanceStreams; // separate client so auto-advance never clobbers the streams pane
    SubtitlesClient m_subtitles;
    ImdbRatings m_imdbRatings;
    TraktClient m_trakt;
    WatchHistory m_watchHistory;
    ExternalMpvPlayer m_player;
    QNetworkAccessManager m_streamPrewarm;
    QNetworkReply *m_streamPrewarmReply = nullptr;
    QNetworkAccessManager m_streamProbe;
    QNetworkReply *m_streamProbeReply = nullptr;
    int m_playbackStartGeneration = 0;

    QVariantList m_homeSections; // [{ id, type, title, items }]
    QVariantList m_searchResults;
    QString m_searchQuery;
    QVariantMap m_selectedMeta;
    QVariantList m_streams;
    QVariantList m_currentSubtitles;
    QVariantMap m_streamMedia;
    QVariantMap m_currentPlaybackMedia;
    QString m_activeMetaType;
    QString m_activeMetaId;
    QString m_activeSubtitleType;
    QString m_activeSubtitleId;
    QSet<QString> m_resumeMetadataRequests;
    QHash<QString, QVariantMap> m_seriesMetaCache; // baseId -> meta, for Next Up / auto-advance
    QVariantList m_localNextUp;
    QTimer m_autoAdvanceTimer; // 1 s countdown tick
    QVariantMap m_autoAdvanceMedia;
    QVariantMap m_autoAdvanceStream;
    QVariantMap m_autoAdvancePrevStream;
    QString m_autoAdvanceAwaitMetaId;
    int m_autoAdvanceFromSeason = 0;
    int m_autoAdvanceFromEpisode = 0;
    bool m_autoAdvanceEnabled = true;
    bool m_autoAdvanceCloseMpv = true;
    bool m_autoAdvanceActive = false;
    bool m_autoAdvanceStreamReady = false;
    bool m_autoAdvancePlayWhenReady = false;
    int m_autoAdvanceSecondsLeft = 0;
    QString m_pendingRemoteResumeType;
    QString m_pendingRemoteResumeId;
    double m_pendingRemoteResumePercent = 0.0;
    QString m_aioStreamsUrl;
    QString m_metadataUrl;
    QString m_subtitleLanguage;
    double m_uiScale = 1.0;
    bool m_showPosterRatings = true;
    QString m_uiMainColor;
    QString m_uiProgressStartColor;
    QString m_uiProgressEndColor;
    bool m_mpvHardwareDecoding = true;
    bool m_mpvGpuNext = false;
    bool m_mpvHdrHint = false;
    bool m_mpvModernz = true;
    bool m_mpvFullscreen = true;
    QString m_mpvExtraArgs;
    bool m_playbackActive = false;
    bool m_playbackBuffering = false;
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
