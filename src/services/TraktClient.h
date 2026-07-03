#pragma once

#include <QDateTime>
#include <QHash>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

#include <functional>

class QNetworkReply;

class TraktClient : public QObject
{
    Q_OBJECT

public:
    explicit TraktClient(QObject *parent = nullptr);

    QString clientId() const;
    QString clientSecret() const;
    QString username() const;
    QString statusMessage() const;
    QString userCode() const;
    QString verificationUrl() const;
    QVariantList playbackProgress() const;
    QVariantList nextUp() const;
    bool connected() const;
    bool authPending() const;
    bool busy() const;

    void setClientId(const QString &clientId);
    void setClientSecret(const QString &clientSecret);
    void startDeviceAuthorization();
    void cancelDeviceAuthorization();
    void disconnectTrakt();
    void fetchPlaybackProgress();
    void sendPlaybackProgress(const QVariantMap &media, double position, double duration);
    void removePlaybackProgress(const QString &key);
    void applyMetadata(const QVariantMap &meta);

signals:
    void changed();
    void errorOccurred(const QString &message);
    void playbackProgressRemoved();

private:
    void load();
    void persistCredentials();
    void persistTokens();
    void clearTokens();
    void postJson(const QString &path, const QByteArray &body, const std::function<void(QNetworkReply *)> &handler);
    void getAuthorized(const QString &path, const std::function<void(QNetworkReply *)> &handler);
    void postAuthorized(const QString &path, const QByteArray &body, const std::function<void(QNetworkReply *)> &handler);
    void deleteAuthorized(const QString &path, const std::function<void(QNetworkReply *)> &handler);
    bool tokenExpiresSoon() const;
    void refreshAccessToken(const std::function<void(bool)> &handler);
    QJsonObject scrobbleBody(const QVariantMap &media, double progressPercent) const;
    void handleDeviceCodeReply(QNetworkReply *reply);
    void pollDeviceToken();
    void handleDeviceTokenReply(QNetworkReply *reply);
    void scheduleDeviceTokenPoll();
    bool isTerminalDeviceAuthError(const QString &error) const;
    void fetchUserSettings();
    void handleUserSettingsReply(QNetworkReply *reply);
    void pollTraktActivities();
    void handleActivitiesReply(QNetworkReply *reply);
    void handlePlaybackProgressReply(int generation, const QString &kind, QNetworkReply *reply);
    void handleWatchedShowsReply(int generation, QNetworkReply *reply);
    void handleShowProgressReply(int generation, const QVariantMap &show, QNetworkReply *reply);
    void publishPausedPlaybackIfReady(int generation);
    void publishNextUpIfReady(int generation);
    void applyCachedMetadata(QVariantList &items) const;
    void applyTokenResponse(const QByteArray &payload);
    void setStatus(const QString &message);
    void setBusy(bool busy);
    void finishPendingAuth();

    QNetworkAccessManager m_network;
    QTimer m_pollTimer;
    QTimer m_activityTimer;
    QString m_clientId;
    QString m_clientSecret;
    QString m_accessToken;
    QString m_refreshToken;
    qint64 m_tokenCreatedAt = 0;
    int m_tokenExpiresIn = 0;
    QString m_username;
    QString m_statusMessage;
    QString m_activityFingerprint;
    QHash<QString, QVariantMap> m_metadataCache;
    QVariantList m_playbackProgress;
    QVariantList m_nextUp;
    QVariantList m_pendingPlaybackMovies;
    QVariantList m_pendingPlaybackEpisodes;
    QVariantList m_pendingNextUpShows;
    bool m_playbackMoviesPending = false;
    bool m_playbackEpisodesPending = false;
    bool m_watchedShowsPending = false;
    int m_showProgressPending = 0;
    int m_refreshGeneration = 0;
    QString m_deviceCode;
    QString m_userCode;
    QString m_verificationUrl;
    QDateTime m_deviceCodeExpiresAt;
    int m_pollIntervalSeconds = 5;
    int m_pollFailures = 0;
    bool m_busy = false;
    bool m_refreshInFlight = false;
    QList<std::function<void(bool)>> m_refreshHandlers;
};
