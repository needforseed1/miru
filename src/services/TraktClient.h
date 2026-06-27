#pragma once

#include <QDateTime>
#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>

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
    bool connected() const;
    bool authPending() const;
    bool busy() const;

    void setClientId(const QString &clientId);
    void setClientSecret(const QString &clientSecret);
    void startDeviceAuthorization();
    void cancelDeviceAuthorization();
    void disconnectTrakt();

signals:
    void changed();
    void errorOccurred(const QString &message);

private:
    void load();
    void persistCredentials();
    void persistTokens();
    void clearTokens();
    void postJson(const QString &path, const QByteArray &body, const std::function<void(QNetworkReply *)> &handler);
    void handleDeviceCodeReply(QNetworkReply *reply);
    void pollDeviceToken();
    void handleDeviceTokenReply(QNetworkReply *reply);
    void scheduleDeviceTokenPoll();
    bool isTerminalDeviceAuthError(const QString &error) const;
    void fetchUserSettings();
    void handleUserSettingsReply(QNetworkReply *reply);
    void applyTokenResponse(const QByteArray &payload);
    void setStatus(const QString &message);
    void setBusy(bool busy);
    void finishPendingAuth();

    QNetworkAccessManager m_network;
    QTimer m_pollTimer;
    QString m_clientId;
    QString m_clientSecret;
    QString m_accessToken;
    QString m_refreshToken;
    qint64 m_tokenCreatedAt = 0;
    int m_tokenExpiresIn = 0;
    QString m_username;
    QString m_statusMessage;
    QString m_deviceCode;
    QString m_userCode;
    QString m_verificationUrl;
    QDateTime m_deviceCodeExpiresAt;
    int m_pollIntervalSeconds = 5;
    int m_pollFailures = 0;
    bool m_busy = false;
};
