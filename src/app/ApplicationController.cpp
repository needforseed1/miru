#include "ApplicationController.h"

#include <QDateTime>
#include <QProcess>
#include <QSettings>
#include <QVariant>

namespace {
QString stringValue(const QVariantMap &map, const QString &key)
{
    return map.value(key).toString();
}
}

ApplicationController::ApplicationController(QObject *parent)
    : QObject(parent)
{
    QSettings settings;
    m_aioStreamsUrl = settings.value(QStringLiteral("addons/aioStreamsUrl")).toString();
    m_aioStreams.setBaseUrl(m_aioStreamsUrl);
    m_metadataUrl = settings.value(QStringLiteral("addons/metadataUrl")).toString();
    m_cinemeta.setBaseUrl(m_metadataUrl);
    m_subtitleLanguage = settings.value(QStringLiteral("subtitles/language"), QStringLiteral("eng")).toString();
    m_uiScale = settings.value(QStringLiteral("ui/scaleFactor"), 1.0).toDouble();
    m_showPosterRatings = settings.value(QStringLiteral("ui/showPosterRatings"), true).toBool();
    m_mpvHardwareDecoding = settings.value(QStringLiteral("mpv/hardwareDecoding"), true).toBool();
    m_mpvGpuNext = settings.value(QStringLiteral("mpv/gpuNext"), false).toBool();
    m_mpvHdrHint = settings.value(QStringLiteral("mpv/hdrHint"), false).toBool();
    m_mpvExtraArgs = settings.value(QStringLiteral("mpv/extraArgs")).toString();

    connect(&m_cinemeta, &CinemetaClient::catalogsDiscovered, this, [this](const QVariantList &sections) {
        m_homeSections.clear();
        for (const QVariant &entry : sections) {
            QVariantMap section = entry.toMap();
            section.insert(QStringLiteral("items"), QVariantList{});
            m_homeSections.append(section);
        }
        emit homeSectionsChanged();

        m_homeRequestsPending = sections.size();
        if (sections.isEmpty()) {
            setLoading(false);
            return;
        }
        for (const QVariant &entry : sections) {
            const QVariantMap section = entry.toMap();
            m_cinemeta.fetchCatalog(section.value(QStringLiteral("type")).toString(),
                                    section.value(QStringLiteral("id")).toString());
        }
    });

    connect(&m_cinemeta, &CinemetaClient::catalogReady, this,
            [this](const QString &type, const QString &catalogId, const QVariantList &items) {
        for (int i = 0; i < m_homeSections.size(); ++i) {
            QVariantMap section = m_homeSections.at(i).toMap();
            if (section.value(QStringLiteral("id")).toString() == catalogId
                && section.value(QStringLiteral("type")).toString() == type) {
                section.insert(QStringLiteral("items"), withTitleRatings(items));
                m_homeSections[i] = section;
                break;
            }
        }
        emit homeSectionsChanged();

        if (m_homeRequestsPending > 0) {
            --m_homeRequestsPending;
            if (m_homeRequestsPending == 0) {
                setLoading(false);
                setStatusMessage(QStringLiteral("Ready"));
            }
        }
    });

    connect(&m_cinemeta, &CinemetaClient::searchReady, this, [this](const QString &, const QVariantList &items) {
        for (const QVariant &item : withTitleRatings(items)) {
            m_searchResults.append(item);
        }
        emit searchResultsChanged();
        setLoading(false);
    });

    connect(&m_cinemeta, &CinemetaClient::metaReady, this, [this](const QVariantMap &meta) {
        m_selectedMeta = meta;
        enrichEpisodeRatings();
        emit selectedMetaChanged();
        setLoading(false);
    });

    connect(&m_cinemeta, &CinemetaClient::errorOccurred, this, [this](const QString &message) {
        setLoading(false);
        setStatusMessage(message);
    });

    connect(&m_aioStreams, &AIOStreamsClient::streamsReady, this, [this](const QVariantList &streams) {
        m_streams = streams;
        emit streamsChanged();
        setLoading(false);
        setStreamsLoading(false);
        setStatusMessage(streams.isEmpty() ? QStringLiteral("No playable HTTP streams found") : QStringLiteral("Streams loaded"));
    });

    connect(&m_aioStreams, &AIOStreamsClient::errorOccurred, this, [this](const QString &message) {
        setLoading(false);
        setStreamsLoading(false);
        setStatusMessage(message);
    });

    connect(&m_subtitles, &SubtitlesClient::subtitlesReady, this, [this](const QVariantList &subtitles) {
        m_currentSubtitles = subtitles;
    });

    connect(&m_player, &ExternalMpvPlayer::errorOccurred, this, [this](const QString &message) {
        setStatusMessage(message);
    });

    connect(&m_player, &ExternalMpvPlayer::playbackStarted, this, [this]() {
        setStatusMessage(QStringLiteral("Playback started in mpv"));
    });

    connect(&m_player, &ExternalMpvPlayer::positionChanged, this, [this](double position, double duration) {
        if (!m_currentPlaybackMedia.isEmpty()) {
            m_watchHistory.record(m_currentPlaybackMedia, position, duration);
        }
    });

    connect(&m_player, &ExternalMpvPlayer::playbackFinished, this, [this](double position, double duration) {
        if (!m_currentPlaybackMedia.isEmpty()) {
            m_watchHistory.record(m_currentPlaybackMedia, position, duration);
        }
    });

    connect(&m_watchHistory, &WatchHistory::changed, this, &ApplicationController::continueWatchingChanged);

    connect(&m_imdbRatings, &ImdbRatings::statusChanged, this, [this](const QString &message) {
        setStatusMessage(message);
    });

    // When the IMDb ratings cache finishes (re)building, refresh the ratings
    // on the series currently shown.
    connect(&m_imdbRatings, &ImdbRatings::refreshFinished, this, [this](bool changed) {
        emit imdbRatingsUpdatedChanged();
        if (!changed) {
            return;
        }
        // Backfill poster ratings now that the cache exists.
        for (int i = 0; i < m_homeSections.size(); ++i) {
            QVariantMap section = m_homeSections.at(i).toMap();
            section.insert(QStringLiteral("items"),
                           withTitleRatings(section.value(QStringLiteral("items")).toList()));
            m_homeSections[i] = section;
        }
        emit homeSectionsChanged();
        if (!m_searchResults.isEmpty()) {
            m_searchResults = withTitleRatings(m_searchResults);
            emit searchResultsChanged();
        }
        if (m_selectedMeta.contains(QStringLiteral("videos"))) {
            enrichEpisodeRatings();
            emit selectedMetaChanged();
        }
    });

    m_imdbRatings.init();
    m_imdbRatings.refreshIfStale();
}

QVariantList ApplicationController::withTitleRatings(const QVariantList &items) const
{
    if (!m_imdbRatings.ready()) {
        return items;
    }
    QVariantList out;
    out.reserve(items.size());
    for (const QVariant &entry : items) {
        QVariantMap item = entry.toMap();
        if (item.value(QStringLiteral("imdbRating")).toString().isEmpty()) {
            const QString rating = m_imdbRatings.titleRating(item.value(QStringLiteral("id")).toString());
            if (!rating.isEmpty()) {
                item.insert(QStringLiteral("imdbRating"), rating);
            }
        }
        out.append(item);
    }
    return out;
}

QVariantMap ApplicationController::mediaForStreamRequest(const QString &type, const QString &id) const
{
    QVariantMap media;
    media.insert(QStringLiteral("type"), type);
    media.insert(QStringLiteral("id"), id);

    if (type == QStringLiteral("series")) {
        const QStringList parts = id.split(QLatin1Char(':'));
        const QString baseId = parts.value(0);
        const int season = parts.value(1).toInt();
        const int episode = parts.value(2).toInt();
        media.insert(QStringLiteral("baseId"), baseId);
        media.insert(QStringLiteral("season"), season);
        media.insert(QStringLiteral("episode"), episode);
        if (m_selectedMeta.value(QStringLiteral("id")).toString() == baseId) {
            media.insert(QStringLiteral("name"), m_selectedMeta.value(QStringLiteral("name")).toString());
            media.insert(QStringLiteral("poster"), m_selectedMeta.value(QStringLiteral("poster")).toString());
        }
    } else {
        media.insert(QStringLiteral("baseId"), id);
        if (m_selectedMeta.value(QStringLiteral("id")).toString() == id) {
            media.insert(QStringLiteral("name"), m_selectedMeta.value(QStringLiteral("name")).toString());
            media.insert(QStringLiteral("poster"), m_selectedMeta.value(QStringLiteral("poster")).toString());
        }
    }

    return media;
}

QVariantMap ApplicationController::currentPlaybackMedia(const QVariantMap &stream) const
{
    QVariantMap media = m_streamMedia;
    if (media.isEmpty()) {
        return media;
    }

    if (stringValue(media, QStringLiteral("name")).isEmpty()) {
        const QString metaName = m_selectedMeta.value(QStringLiteral("name")).toString();
        media.insert(QStringLiteral("name"), metaName.isEmpty()
                         ? stream.value(QStringLiteral("title")).toString()
                         : metaName);
    }
    if (stringValue(media, QStringLiteral("poster")).isEmpty()) {
        media.insert(QStringLiteral("poster"), m_selectedMeta.value(QStringLiteral("poster")).toString());
    }

    return media;
}

void ApplicationController::enrichEpisodeRatings()
{
    if (!m_imdbRatings.ready()) {
        return;
    }

    const QString parent = m_selectedMeta.value(QStringLiteral("id")).toString();
    if (!parent.startsWith(QStringLiteral("tt"))) {
        return;
    }

    QVariantList videos = m_selectedMeta.value(QStringLiteral("videos")).toList();
    if (videos.isEmpty()) {
        return;
    }

    bool changed = false;
    for (QVariant &entry : videos) {
        QVariantMap video = entry.toMap();
        const QString rating = m_imdbRatings.ratingFor(parent,
                                                       video.value(QStringLiteral("season")).toInt(),
                                                       video.value(QStringLiteral("episode")).toInt());
        if (!rating.isEmpty()) {
            video.insert(QStringLiteral("rating"), rating);
            entry = video;
            changed = true;
        }
    }

    if (changed) {
        m_selectedMeta.insert(QStringLiteral("videos"), videos);
    }
}

QVariantList ApplicationController::homeSections() const { return m_homeSections; }

QVariantMap ApplicationController::featured() const
{
    // Prefer the first item of the first movie section for the hero.
    for (const QVariant &entry : m_homeSections) {
        const QVariantMap section = entry.toMap();
        if (section.value(QStringLiteral("type")).toString() == QStringLiteral("movie")) {
            const QVariantList items = section.value(QStringLiteral("items")).toList();
            if (!items.isEmpty()) {
                return items.first().toMap();
            }
        }
    }
    for (const QVariant &entry : m_homeSections) {
        const QVariantList items = entry.toMap().value(QStringLiteral("items")).toList();
        if (!items.isEmpty()) {
            return items.first().toMap();
        }
    }
    return {};
}

QVariantList ApplicationController::searchResults() const { return m_searchResults; }
QVariantList ApplicationController::continueWatching() const { return m_watchHistory.inProgress(); }
QVariantMap ApplicationController::selectedMeta() const { return m_selectedMeta; }
QVariantList ApplicationController::streams() const { return m_streams; }
bool ApplicationController::streamsLoading() const { return m_streamsLoading; }
QString ApplicationController::aioStreamsUrl() const { return m_aioStreamsUrl; }
QString ApplicationController::metadataUrl() const { return m_metadataUrl; }
QString ApplicationController::subtitleLanguage() const { return m_subtitleLanguage; }
double ApplicationController::uiScale() const { return m_uiScale; }
bool ApplicationController::showPosterRatings() const { return m_showPosterRatings; }
bool ApplicationController::mpvHardwareDecoding() const { return m_mpvHardwareDecoding; }
bool ApplicationController::mpvGpuNext() const { return m_mpvGpuNext; }
bool ApplicationController::mpvHdrHint() const { return m_mpvHdrHint; }
QString ApplicationController::mpvExtraArgs() const { return m_mpvExtraArgs; }

QString ApplicationController::imdbRatingsUpdated() const
{
    const QDateTime last = QSettings().value(QStringLiteral("imdb/lastRefresh")).toDateTime();
    if (!last.isValid()) {
        return QStringLiteral("Not downloaded yet");
    }
    return QStringLiteral("Updated %1").arg(last.toString(QStringLiteral("yyyy-MM-dd hh:mm")));
}
bool ApplicationController::loading() const { return m_loading; }
QString ApplicationController::statusMessage() const { return m_statusMessage; }

void ApplicationController::loadHome()
{
    setLoading(true);
    setStatusMessage(QStringLiteral("Loading catalogs"));
    m_cinemeta.discoverCatalogs();
}

void ApplicationController::search(const QString &query)
{
    const QString trimmed = query.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }

    m_searchResults.clear();
    emit searchResultsChanged();
    setLoading(true);
    setStatusMessage(QStringLiteral("Searching"));
    m_cinemeta.search(QStringLiteral("movie"), trimmed);
    m_cinemeta.search(QStringLiteral("series"), trimmed);
}

void ApplicationController::clearSearch()
{
    if (m_searchResults.isEmpty()) {
        return;
    }
    m_searchResults.clear();
    emit searchResultsChanged();
    setStatusMessage(QStringLiteral("Ready"));
}

void ApplicationController::loadMeta(const QString &type, const QString &id)
{
    if (type.isEmpty() || id.isEmpty()) {
        return;
    }

    setLoading(true);
    setStatusMessage(QStringLiteral("Loading details"));
    m_cinemeta.fetchMeta(type, id);
}

void ApplicationController::loadStreams(const QString &type, const QString &id)
{
    m_streamMedia.clear();
    if (m_aioStreamsUrl.trimmed().isEmpty()) {
        m_streams.clear();
        emit streamsChanged();
        setStreamsLoading(false);
        setStatusMessage(QStringLiteral("Add your AIOStreams addon URL in Settings first"));
        return;
    }

    setLoading(true);
    setStreamsLoading(true);
    setStatusMessage(QStringLiteral("Loading streams"));
    m_streamMedia = mediaForStreamRequest(type, id);

    // Drop the previous episode's releases immediately so the search
    // animation shows again instead of stale results lingering.
    if (!m_streams.isEmpty()) {
        m_streams.clear();
        emit streamsChanged();
    }

    m_aioStreams.fetchStreams(type, id);

    // Prefetch subtitles for this title/episode so they are ready by the
    // time a release is played (added to mpv as external --sub-file).
    m_currentSubtitles.clear();
    if (m_subtitleLanguage != QStringLiteral("off")) {
        m_subtitles.fetchSubtitles(type, id);
    }
}

void ApplicationController::clearStreams()
{
    m_currentSubtitles.clear();
    m_streamMedia.clear();
    setStreamsLoading(false);
    if (m_streams.isEmpty()) {
        return;
    }
    m_streams.clear();
    emit streamsChanged();
}

void ApplicationController::playStream(int index)
{
    if (index < 0 || index >= m_streams.size()) {
        return;
    }

    const QVariantMap stream = m_streams.at(index).toMap();
    const QString url = stream.value(QStringLiteral("url")).toString();
    const QString title = stream.value(QStringLiteral("title")).toString();
    const QVariantMap headers = stream.value(QStringLiteral("headers")).toMap();
    m_currentPlaybackMedia = currentPlaybackMedia(stream);
    const double startSeconds = m_watchHistory.positionFor(m_currentPlaybackMedia);

    // Pick subtitles in the preferred language (a couple, mpv selects the
    // first). Keep the count small so it does not delay playback start.
    QStringList subtitleUrls;
    if (m_subtitleLanguage != QStringLiteral("off")) {
        for (const QVariant &entry : m_currentSubtitles) {
            const QVariantMap sub = entry.toMap();
            if (sub.value(QStringLiteral("lang")).toString() == m_subtitleLanguage) {
                subtitleUrls.append(sub.value(QStringLiteral("url")).toString());
                if (subtitleUrls.size() >= 2) {
                    break;
                }
            }
        }
    }

    const QStringList extraArgs = QProcess::splitCommand(m_mpvExtraArgs);
    m_player.play(url, title, headers, subtitleUrls,
                  m_mpvHardwareDecoding, m_mpvGpuNext, m_mpvHdrHint, extraArgs, startSeconds);
}

void ApplicationController::removeContinueWatching(const QString &key)
{
    m_watchHistory.remove(key);
}

void ApplicationController::setAioStreamsUrl(const QString &url)
{
    const QString trimmed = url.trimmed();
    if (m_aioStreamsUrl == trimmed) {
        return;
    }

    m_aioStreamsUrl = trimmed;
    m_aioStreams.setBaseUrl(trimmed);
    QSettings settings;
    settings.setValue(QStringLiteral("addons/aioStreamsUrl"), trimmed);
    emit aioStreamsUrlChanged();
    setStatusMessage(trimmed.isEmpty() ? QStringLiteral("AIOStreams URL cleared") : QStringLiteral("AIOStreams URL configured"));
}

void ApplicationController::setMetadataUrl(const QString &url)
{
    const QString trimmed = url.trimmed();
    if (m_metadataUrl == trimmed) {
        return;
    }

    m_metadataUrl = trimmed;
    m_cinemeta.setBaseUrl(trimmed);
    QSettings settings;
    settings.setValue(QStringLiteral("addons/metadataUrl"), trimmed);
    emit metadataUrlChanged();
    setStatusMessage(trimmed.isEmpty() ? QStringLiteral("Using Cinemeta for metadata")
                                       : QStringLiteral("Metadata addon configured"));
    loadHome();
}

void ApplicationController::refreshImdbRatings()
{
    m_imdbRatings.refreshNow();
}

void ApplicationController::setUiScale(double scale)
{
    if (scale <= 0.0 || qFuzzyCompare(m_uiScale, scale)) {
        return;
    }
    m_uiScale = scale;
    QSettings().setValue(QStringLiteral("ui/scaleFactor"), scale);
    emit uiScaleChanged();
    setStatusMessage(QStringLiteral("Zoom set to %1% — restart to apply")
                         .arg(qRound(scale * 100)));
}

void ApplicationController::setSubtitleLanguage(const QString &language)
{
    if (m_subtitleLanguage == language) {
        return;
    }

    m_subtitleLanguage = language;
    QSettings settings;
    settings.setValue(QStringLiteral("subtitles/language"), language);
    emit subtitleLanguageChanged();
    setStatusMessage(language == QStringLiteral("off")
                         ? QStringLiteral("Subtitles disabled")
                         : QStringLiteral("Subtitle language set to %1").arg(language));
}

void ApplicationController::setShowPosterRatings(bool enabled)
{
    if (m_showPosterRatings == enabled) {
        return;
    }

    m_showPosterRatings = enabled;
    QSettings().setValue(QStringLiteral("ui/showPosterRatings"), enabled);
    emit showPosterRatingsChanged();
}

void ApplicationController::setMpvHardwareDecoding(bool enabled)
{
    if (m_mpvHardwareDecoding == enabled) {
        return;
    }

    m_mpvHardwareDecoding = enabled;
    QSettings().setValue(QStringLiteral("mpv/hardwareDecoding"), enabled);
    emit mpvHardwareDecodingChanged();
    setStatusMessage(enabled ? QStringLiteral("mpv hardware decoding enabled")
                             : QStringLiteral("mpv hardware decoding disabled"));
}

void ApplicationController::setMpvGpuNext(bool enabled)
{
    if (m_mpvGpuNext == enabled) {
        return;
    }

    m_mpvGpuNext = enabled;
    QSettings().setValue(QStringLiteral("mpv/gpuNext"), enabled);
    emit mpvGpuNextChanged();
    setStatusMessage(enabled ? QStringLiteral("mpv gpu-next enabled")
                             : QStringLiteral("mpv gpu-next disabled"));
}

void ApplicationController::setMpvHdrHint(bool enabled)
{
    if (m_mpvHdrHint == enabled) {
        return;
    }

    m_mpvHdrHint = enabled;
    QSettings().setValue(QStringLiteral("mpv/hdrHint"), enabled);
    emit mpvHdrHintChanged();
    setStatusMessage(enabled ? QStringLiteral("mpv HDR/Vulkan hint enabled")
                             : QStringLiteral("mpv HDR/Vulkan hint disabled"));
}

void ApplicationController::setMpvExtraArgs(const QString &args)
{
    const QString trimmed = args.trimmed();
    if (m_mpvExtraArgs == trimmed) {
        return;
    }

    m_mpvExtraArgs = trimmed;
    QSettings().setValue(QStringLiteral("mpv/extraArgs"), trimmed);
    emit mpvExtraArgsChanged();
    setStatusMessage(trimmed.isEmpty() ? QStringLiteral("Custom mpv args cleared")
                                       : QStringLiteral("Custom mpv args saved"));
}

void ApplicationController::setLoading(bool loading)
{
    if (m_loading == loading) {
        return;
    }

    m_loading = loading;
    emit loadingChanged();
}

void ApplicationController::setStreamsLoading(bool loading)
{
    if (m_streamsLoading == loading) {
        return;
    }

    m_streamsLoading = loading;
    emit streamsLoadingChanged();
}

void ApplicationController::setStatusMessage(const QString &message)
{
    if (m_statusMessage == message) {
        return;
    }

    m_statusMessage = message;
    emit statusMessageChanged();
}
