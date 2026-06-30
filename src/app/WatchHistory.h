#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class QTimer;

class WatchHistory : public QObject
{
    Q_OBJECT

public:
    explicit WatchHistory(QObject *parent = nullptr);
    ~WatchHistory() override;

    QVariantList inProgress() const;
    QVariantMap entry(const QString &key) const;
    double positionFor(const QVariantMap &media) const;

    void record(const QVariantMap &media, double position, double duration);
    void applyMetadata(const QVariantMap &meta);
    Q_INVOKABLE void remove(const QString &key);

signals:
    void changed();

private:
    static QString keyForMedia(const QVariantMap &media);
    static bool isInProgress(const QVariantMap &entry);

    QString storePath() const;
    void load();
    void scheduleSave();
    void saveNow();

    QVariantList m_entries;
    QTimer *m_saveTimer = nullptr;
};
