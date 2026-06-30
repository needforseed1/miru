#include "ApplicationController.h"

#include <algorithm>
#include <QDateTime>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QClipboard>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QUrl>
#include <QVariant>

namespace {
QString stringValue(const QVariantMap &map, const QString &key)
{
    return map.value(key).toString();
}

QStringList stringListValue(const QVariant &value)
{
    QStringList out = value.toStringList();
    if (!out.isEmpty()) {
        return out;
    }

    for (const QVariant &entry : value.toList()) {
        const QString item = entry.toString();
        if (!item.isEmpty()) {
            out.append(item);
        }
    }
    return out;
}

bool shouldForwardPlaybackHeader(const QString &key, const QString &value)
{
    return !key.trimmed().isEmpty()
        && !value.trimmed().isEmpty()
        && key.compare(QStringLiteral("Range"), Qt::CaseInsensitive) != 0;
}

QString normalizedSearchText(QString text)
{
    text = text.toCaseFolded();
    text.replace(QRegularExpression(QStringLiteral("[^\\p{L}\\p{N}]+")), QStringLiteral(" "));
    return text.simplified();
}

QString withoutLeadingArticle(const QString &text)
{
    for (const QString &article : {QStringLiteral("the "), QStringLiteral("a "), QStringLiteral("an ")}) {
        if (text.startsWith(article)) {
            return text.mid(article.size());
        }
    }
    return text;
}

int searchRank(const QVariantMap &item, const QString &query)
{
    const QString q = normalizedSearchText(query);
    const QString name = normalizedSearchText(item.value(QStringLiteral("name")).toString());
    const QString noArticle = withoutLeadingArticle(name);
    if (q.isEmpty() || name.isEmpty()) {
        return 100;
    }
    // Exact title (ignoring a leading article). Ordering within this tier is
    // left to popularity/rating so the canonical title wins over obscure films
    // that happen to share the exact name.
    if (name == q || noArticle == q) {
        return 0;
    }
    const QStringList nameTokens = name.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    const QStringList noArticleTokens = noArticle.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    // First whole word equals the query ("Dune: Part Two", "The Matrix
    // Reloaded"): a franchise/sequel entry, grouped with exact matches so
    // popularity can interleave them rather than burying the popular ones.
    if ((!nameTokens.isEmpty() && nameTokens.first() == q)
        || (!noArticleTokens.isEmpty() && noArticleTokens.first() == q)) {
        return 0;
    }
    // Bare character prefix of the first word ("Aliens" for "alien"): related,
    // but kept below exact/franchise so an unrelated word that merely starts
    // with the query ("Hereditary" for "her") cannot outrank the real title.
    if (name.startsWith(q) || noArticle.startsWith(q)) {
        return 15;
    }
    // The query is a prefix of some later word in the title.
    for (const QString &token : nameTokens) {
        if (token.startsWith(q)) {
            return 25;
        }
    }
    // The query appears as a raw substring (inside a word).
    if (name.contains(q)) {
        return 45;
    }
    // No textual match; the metadata addon matched this fuzzily.
    return 100;
}

int runtimeMinutes(const QVariantMap &item)
{
    static const QRegularExpression runtimePattern(QStringLiteral("\\b(\\d+)\\s*min\\b"),
                                                   QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = runtimePattern.match(item.value(QStringLiteral("runtime")).toString());
    return match.hasMatch() ? match.captured(1).toInt() : 0;
}

// Short films and clips/trailers (sub-45-minute runtimes) the addon sometimes
// returns under an exact title. They are demoted within their relevance tier so
// they cannot sit above a real feature that shares the name, without otherwise
// disturbing the addon's popularity ordering.
bool isShortResult(const QVariantMap &item)
{
    if (item.value(QStringLiteral("genres")).toStringList().contains(QStringLiteral("Short"),
                                                                     Qt::CaseInsensitive)) {
        return true;
    }
    const int runtime = runtimeMinutes(item);
    return runtime > 0 && runtime < 45;
}

QVariantMap healthItem(const QString &name, const QString &status,
                       const QString &detail, const QString &state)
{
    QVariantMap item;
    item.insert(QStringLiteral("name"), name);
    item.insert(QStringLiteral("status"), status);
    item.insert(QStringLiteral("detail"), detail);
    item.insert(QStringLiteral("state"), state);
    return item;
}

void sortSearchResults(QVariantList &items, const QString &query)
{
    // The metadata addon already ranks its search results (TMDB popularity for
    // AIOMetadata, Cinemeta's own ordering otherwise), and that is the order
    // Stremio itself shows. We only apply a relevance guard: a stable sort by
    // match tier keeps the addon's order within each tier while demoting the
    // fuzzy non-matches the addon mixes in (e.g. "Anatomy of a Fall" for a
    // "Dune" search). Within a tier, shorts/clips sink to the bottom but the
    // addon's order is otherwise preserved.
    std::stable_sort(items.begin(), items.end(), [&query](const QVariant &left, const QVariant &right) {
        const QVariantMap a = left.toMap();
        const QVariantMap b = right.toMap();
        const int rankA = searchRank(a, query);
        const int rankB = searchRank(b, query);
        if (rankA != rankB) {
            return rankA < rankB;
        }

        const bool shortA = isShortResult(a);
        const bool shortB = isShortResult(b);
        if (shortA != shortB) {
            return !shortA;
        }

        return false;
    });
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
    m_resumeMetadata.setBaseUrl(m_metadataUrl);
    m_subtitleLanguage = settings.value(QStringLiteral("subtitles/language"), QStringLiteral("eng")).toString();
    m_uiScale = settings.value(QStringLiteral("ui/scaleFactor"), 1.0).toDouble();
    m_showPosterRatings = settings.value(QStringLiteral("ui/showPosterRatings"), true).toBool();
    m_mpvHardwareDecoding = settings.value(QStringLiteral("mpv/hardwareDecoding"), true).toBool();
    m_mpvGpuNext = settings.value(QStringLiteral("mpv/gpuNext"), false).toBool();
    m_mpvHdrHint = settings.value(QStringLiteral("mpv/hdrHint"), false).toBool();
    m_mpvModernz = settings.value(QStringLiteral("mpv/modernz"), true).toBool();
    m_mpvFullscreen = settings.value(QStringLiteral("mpv/fullscreen"), true).toBool();
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

    connect(&m_cinemeta, &CinemetaClient::searchReady, this,
            [this](const QString &, const QString &query, const QVariantList &items) {
        if (query != m_searchQuery) {
            return;
        }
        for (const QVariant &item : withTitleRatings(items)) {
            m_searchResults.append(item);
        }
        sortSearchResults(m_searchResults, m_searchQuery);
        emit searchResultsChanged();
        if (m_searchRequestsPending > 0) {
            --m_searchRequestsPending;
        }
        if (m_searchRequestsPending == 0) {
            setLoading(false);
        }
    });

    connect(&m_cinemeta, &CinemetaClient::metaReady, this,
            [this](const QString &type, const QString &id, const QVariantMap &meta) {
        if (type != m_activeMetaType || id != m_activeMetaId) {
            return;
        }
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

    connect(&m_trakt, &TraktClient::changed, this, [this]() {
        emit traktChanged();
        emit sourceHealthChanged();
        emit continueWatchingChanged();
        emit nextUpChanged();
        hydrateTraktResumeMetadata();
    });
    connect(&m_trakt, &TraktClient::errorOccurred, this, [this](const QString &message) {
        setStatusMessage(message);
    });
    connect(&m_trakt, &TraktClient::playbackProgressRemoved, this, [this]() {
        setStatusMessage(QStringLiteral("Trakt playback progress removed"));
    });

    connect(&m_resumeMetadata, &CinemetaClient::metaReady, this,
            [this](const QString &, const QString &, const QVariantMap &meta) {
        m_trakt.applyMetadata(meta);
        m_watchHistory.applyMetadata(meta);
    });
    connect(&m_resumeMetadata, &CinemetaClient::errorOccurred, this, [](const QString &) {
        // Resume artwork hydration is best-effort; details/search errors still
        // surface through the main metadata client.
    });

    connect(&m_subtitles, &SubtitlesClient::subtitlesReady, this,
            [this](const QString &type, const QString &id, const QVariantList &subtitles) {
        if (type != m_activeSubtitleType || id != m_activeSubtitleId) {
            return;
        }
        m_currentSubtitles = subtitles;
    });

    connect(&m_player, &ExternalMpvPlayer::errorOccurred, this, [this](const QString &message) {
        m_playbackBuffering = false;
        abortStreamPrewarm();
        emit playbackStateChanged();
        setStatusMessage(message);
    });

    connect(&m_player, &ExternalMpvPlayer::playbackStarted, this, [this]() {
        m_playbackActive = true;
        m_playbackBuffering = true;
        emit playbackStateChanged();
        setStatusMessage(QStringLiteral("Opening stream in mpv…"));
    });

    connect(&m_player, &ExternalMpvPlayer::playbackPlaying, this, [this]() {
        m_playbackBuffering = false;
        abortStreamPrewarm(); // mpv has read the tail; stop draining it
        emit playbackStateChanged();
        setStatusMessage(QStringLiteral("Playing in mpv"));
    });

    connect(&m_player, &ExternalMpvPlayer::positionChanged, this, [this](double position, double duration) {
        m_playbackPosition = position;
        m_playbackDuration = duration;
        emit playbackPositionChanged();
        if (!m_currentPlaybackMedia.isEmpty()) {
            m_watchHistory.record(m_currentPlaybackMedia, position, duration);
        }
    });

    connect(&m_player, &ExternalMpvPlayer::pauseChanged, this, [this](bool paused) {
        if (m_playbackPaused == paused) {
            return;
        }
        m_playbackPaused = paused;
        emit playbackStateChanged();
    });

    connect(&m_player, &ExternalMpvPlayer::playbackFinished, this, [this](double position, double duration) {
        m_playbackActive = false;
        m_playbackBuffering = false;
        abortStreamPrewarm();
        m_playbackPosition = position;
        m_playbackDuration = duration;
        m_playbackPaused = false;
        emit playbackStateChanged();
        emit playbackPositionChanged();
        if (!m_currentPlaybackMedia.isEmpty()) {
            m_watchHistory.record(m_currentPlaybackMedia, position, duration);
            m_trakt.sendPlaybackProgress(m_currentPlaybackMedia, position, duration);
        }
    });

    connect(&m_watchHistory, &WatchHistory::changed, this, [this]() {
        emit continueWatchingChanged();
        hydrateTraktResumeMetadata();
    });

    connect(&m_imdbRatings, &ImdbRatings::statusChanged, this, [this](const QString &message) {
        setStatusMessage(message);
    });

    // When the IMDb ratings cache finishes (re)building, refresh the ratings
    // on the series currently shown.
    connect(&m_imdbRatings, &ImdbRatings::refreshFinished, this, [this](bool changed) {
        emit imdbRatingsUpdatedChanged();
        emit sourceHealthChanged();
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
            // Backfill poster ratings only; ordering does not depend on ratings.
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
    hydrateTraktResumeMetadata();
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
            media.insert(QStringLiteral("background"), m_selectedMeta.value(QStringLiteral("background")).toString());
            for (const QVariant &entry : m_selectedMeta.value(QStringLiteral("videos")).toList()) {
                const QVariantMap video = entry.toMap();
                if (video.value(QStringLiteral("season")).toInt() == season
                    && video.value(QStringLiteral("episode")).toInt() == episode) {
                    media.insert(QStringLiteral("thumbnail"), video.value(QStringLiteral("thumbnail")).toString());
                    media.insert(QStringLiteral("episodeTitle"), video.value(QStringLiteral("title")).toString());
                    break;
                }
            }
        }
    } else {
        media.insert(QStringLiteral("baseId"), id);
        if (m_selectedMeta.value(QStringLiteral("id")).toString() == id) {
            media.insert(QStringLiteral("name"), m_selectedMeta.value(QStringLiteral("name")).toString());
            media.insert(QStringLiteral("poster"), m_selectedMeta.value(QStringLiteral("poster")).toString());
            media.insert(QStringLiteral("background"), m_selectedMeta.value(QStringLiteral("background")).toString());
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
    if (stringValue(media, QStringLiteral("background")).isEmpty()) {
        media.insert(QStringLiteral("background"), m_selectedMeta.value(QStringLiteral("background")).toString());
    }
    if (media.value(QStringLiteral("type")).toString() == QStringLiteral("series")
        && stringValue(media, QStringLiteral("thumbnail")).isEmpty()) {
        const int season = media.value(QStringLiteral("season")).toInt();
        const int episode = media.value(QStringLiteral("episode")).toInt();
        for (const QVariant &entry : m_selectedMeta.value(QStringLiteral("videos")).toList()) {
            const QVariantMap video = entry.toMap();
            if (video.value(QStringLiteral("season")).toInt() == season
                && video.value(QStringLiteral("episode")).toInt() == episode) {
                media.insert(QStringLiteral("thumbnail"), video.value(QStringLiteral("thumbnail")).toString());
                media.insert(QStringLiteral("episodeTitle"), video.value(QStringLiteral("title")).toString());
                break;
            }
        }
    }
    media.insert(QStringLiteral("stream"), stream);

    return media;
}

void ApplicationController::hydrateTraktResumeMetadata()
{
    QVariantList items = m_watchHistory.inProgress();
    if (m_trakt.connected()) {
        items.append(m_trakt.playbackProgress());
        items.append(m_trakt.nextUp());
    }

    for (const QVariant &entry : items) {
        const QVariantMap item = entry.toMap();
        const QString type = item.value(QStringLiteral("type")).toString();
        const bool needsSeriesStill = type == QStringLiteral("series")
            && item.value(QStringLiteral("thumbnail")).toString().isEmpty()
            && item.value(QStringLiteral("episodeThumbnail")).toString().isEmpty();
        if (!item.value(QStringLiteral("poster")).toString().isEmpty()
            && !item.value(QStringLiteral("background")).toString().isEmpty()
            && !needsSeriesStill) {
            continue;
        }

        const QString id = type == QStringLiteral("series")
            ? item.value(QStringLiteral("baseId")).toString()
            : item.value(QStringLiteral("id")).toString();
        if (type.isEmpty() || id.isEmpty()) {
            continue;
        }

        const QString key = QStringLiteral("%1:%2").arg(type, id);
        if (m_resumeMetadataRequests.contains(key)) {
            continue;
        }
        m_resumeMetadataRequests.insert(key);
        m_resumeMetadata.fetchMeta(type, id);
    }
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
QVariantList ApplicationController::continueWatching() const
{
    return m_trakt.connected() ? m_trakt.playbackProgress() : m_watchHistory.inProgress();
}
QVariantList ApplicationController::nextUp() const
{
    return m_trakt.connected() ? m_trakt.nextUp() : QVariantList{};
}
QVariantMap ApplicationController::selectedMeta() const { return m_selectedMeta; }
QVariantList ApplicationController::streams() const { return m_streams; }
bool ApplicationController::streamsLoading() const { return m_streamsLoading; }
QVariantList ApplicationController::sourceHealth() const
{
    QVariantList items;

    if (m_aioStreamsUrl.trimmed().isEmpty()) {
        items.append(healthItem(QStringLiteral("AIOStreams"), QStringLiteral("Needs setup"),
                                QStringLiteral("Add a manifest URL to load releases."),
                                QStringLiteral("warning")));
    } else {
        const QUrl url(m_aioStreamsUrl);
        items.append(healthItem(QStringLiteral("AIOStreams"), QStringLiteral("Configured"),
                                url.host().isEmpty() ? QStringLiteral("Manifest URL saved.")
                                                     : QStringLiteral("Using %1").arg(url.host()),
                                QStringLiteral("good")));
    }

    if (m_metadataUrl.trimmed().isEmpty()) {
        items.append(healthItem(QStringLiteral("Metadata"), QStringLiteral("Cinemeta"),
                                QStringLiteral("Default catalogs, details, and artwork."),
                                QStringLiteral("good")));
    } else {
        const QUrl url(m_metadataUrl);
        items.append(healthItem(QStringLiteral("Metadata"), QStringLiteral("Custom addon"),
                                url.host().isEmpty() ? QStringLiteral("Manifest URL saved.")
                                                     : QStringLiteral("Using %1").arg(url.host()),
                                QStringLiteral("good")));
    }

    if (m_subtitleLanguage == QStringLiteral("off")) {
        items.append(healthItem(QStringLiteral("OpenSubtitles"), QStringLiteral("Off"),
                                QStringLiteral("Subtitle fetching is disabled."),
                                QStringLiteral("neutral")));
    } else {
        items.append(healthItem(QStringLiteral("OpenSubtitles"), m_subtitleLanguage.toUpper(),
                                QStringLiteral("Preferred subtitle language is active."),
                                QStringLiteral("good")));
    }

    const QDateTime imdbRefresh = QSettings().value(QStringLiteral("imdb/lastRefresh")).toDateTime();
    if (m_imdbRatings.ready()) {
        items.append(healthItem(QStringLiteral("IMDb ratings"), QStringLiteral("Ready"),
                                imdbRefresh.isValid()
                                    ? QStringLiteral("Updated %1").arg(imdbRefresh.toString(QStringLiteral("yyyy-MM-dd")))
                                    : QStringLiteral("Local cache is available."),
                                QStringLiteral("good")));
    } else {
        items.append(healthItem(QStringLiteral("IMDb ratings"), QStringLiteral("Needs download"),
                                QStringLiteral("Refresh to build the local ratings cache."),
                                QStringLiteral("warning")));
    }

    if (m_trakt.connected()) {
        const QString username = m_trakt.username();
        items.append(healthItem(QStringLiteral("Trakt"), QStringLiteral("Connected"),
                                username.isEmpty() ? QStringLiteral("Resume and Next Up sync enabled.")
                                                   : QStringLiteral("Signed in as %1.").arg(username),
                                QStringLiteral("good")));
    } else if (m_trakt.authPending()) {
        items.append(healthItem(QStringLiteral("Trakt"), QStringLiteral("Waiting"),
                                QStringLiteral("Finish device authorization in the browser."),
                                QStringLiteral("warning")));
    } else if (!m_trakt.clientId().isEmpty() && !m_trakt.clientSecret().isEmpty()) {
        items.append(healthItem(QStringLiteral("Trakt"), QStringLiteral("Ready to connect"),
                                QStringLiteral("Credentials are saved locally."),
                                QStringLiteral("neutral")));
    } else {
        items.append(healthItem(QStringLiteral("Trakt"), QStringLiteral("Optional"),
                                QStringLiteral("Connect to sync resume progress and Next Up."),
                                QStringLiteral("neutral")));
    }

    const QString mpv = ExternalMpvPlayer::resolvedExecutablePath();
    items.append(healthItem(QStringLiteral("mpv"), mpv.isEmpty() ? QStringLiteral("Missing") : QStringLiteral("Available"),
                            mpv.isEmpty() ? QStringLiteral("Install mpv or use a build that bundles it.")
                                          : QStringLiteral("Playback executable found."),
                            mpv.isEmpty() ? QStringLiteral("danger") : QStringLiteral("good")));

    return items;
}
QString ApplicationController::aioStreamsUrl() const { return m_aioStreamsUrl; }
QString ApplicationController::metadataUrl() const { return m_metadataUrl; }
QString ApplicationController::subtitleLanguage() const { return m_subtitleLanguage; }
double ApplicationController::uiScale() const { return m_uiScale; }
bool ApplicationController::showPosterRatings() const { return m_showPosterRatings; }
bool ApplicationController::mpvHardwareDecoding() const { return m_mpvHardwareDecoding; }
bool ApplicationController::mpvGpuNext() const { return m_mpvGpuNext; }
bool ApplicationController::mpvHdrHint() const { return m_mpvHdrHint; }
bool ApplicationController::mpvModernz() const { return m_mpvModernz; }
bool ApplicationController::mpvFullscreen() const { return m_mpvFullscreen; }
QString ApplicationController::mpvExtraArgs() const { return m_mpvExtraArgs; }
QString ApplicationController::traktClientId() const { return m_trakt.clientId(); }
QString ApplicationController::traktClientSecret() const { return m_trakt.clientSecret(); }
QString ApplicationController::traktStatus() const { return m_trakt.statusMessage(); }
QString ApplicationController::traktUserCode() const { return m_trakt.userCode(); }
QString ApplicationController::traktVerificationUrl() const { return m_trakt.verificationUrl(); }
QString ApplicationController::traktUsername() const { return m_trakt.username(); }
bool ApplicationController::traktConnected() const { return m_trakt.connected(); }
bool ApplicationController::traktAuthPending() const { return m_trakt.authPending(); }
bool ApplicationController::traktBusy() const { return m_trakt.busy(); }
bool ApplicationController::playbackActive() const { return m_playbackActive; }
bool ApplicationController::playbackBuffering() const { return m_playbackBuffering; }
bool ApplicationController::playbackPaused() const { return m_playbackPaused; }
QString ApplicationController::playbackTitle() const { return m_playbackTitle; }
double ApplicationController::playbackPosition() const { return m_playbackPosition; }
double ApplicationController::playbackDuration() const { return m_playbackDuration; }

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
    m_searchQuery = trimmed;
    m_searchRequestsPending = 2;
    emit searchResultsChanged();
    setLoading(true);
    setStatusMessage(QStringLiteral("Searching"));
    m_cinemeta.search(QStringLiteral("movie"), trimmed);
    m_cinemeta.search(QStringLiteral("series"), trimmed);
}

void ApplicationController::clearSearch()
{
    m_searchQuery.clear();
    m_searchRequestsPending = 0;
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
    m_activeMetaType = type;
    m_activeMetaId = id;
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
    m_activeSubtitleType.clear();
    m_activeSubtitleId.clear();
    if (m_subtitleLanguage != QStringLiteral("off")) {
        m_activeSubtitleType = type;
        m_activeSubtitleId = id;
        m_subtitles.fetchSubtitles(type, id);
    }
}

void ApplicationController::clearStreams()
{
    m_currentSubtitles.clear();
    m_activeSubtitleType.clear();
    m_activeSubtitleId.clear();
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

    QVariantMap playbackMedia = currentPlaybackMedia(stream);
    if (!subtitleUrls.isEmpty()) {
        playbackMedia.insert(QStringLiteral("subtitleUrls"), subtitleUrls);
    }
    double startSeconds = m_watchHistory.positionFor(playbackMedia);

    double startPercent = 0.0;
    if (m_pendingRemoteResumeType == m_streamMedia.value(QStringLiteral("type")).toString()
        && m_pendingRemoteResumeId == m_streamMedia.value(QStringLiteral("id")).toString()) {
        startPercent = m_pendingRemoteResumePercent;
        startSeconds = 0.0;
        m_pendingRemoteResumeType.clear();
        m_pendingRemoteResumeId.clear();
        m_pendingRemoteResumePercent = 0.0;
    }

    startPlayback(playbackMedia, url, title, headers, subtitleUrls, startSeconds, startPercent);
}

void ApplicationController::resumeContinueWatching(const QString &key)
{
    const QVariantMap entry = m_watchHistory.entry(key);
    const QVariantMap stream = entry.value(QStringLiteral("stream")).toMap();
    const QString url = stream.value(QStringLiteral("url")).toString();
    if (entry.isEmpty() || url.trimmed().isEmpty()) {
        setStatusMessage(QStringLiteral("Saved release is unavailable; choose a release from details"));
        return;
    }

    const QString title = stream.value(QStringLiteral("title")).toString();
    const QVariantMap headers = stream.value(QStringLiteral("headers")).toMap();
    const QStringList subtitleUrls = stringListValue(entry.value(QStringLiteral("subtitleUrls")));

    startPlayback(entry, url, title, headers, subtitleUrls, m_watchHistory.positionFor(entry), 0.0);
}

bool ApplicationController::startPlayback(const QVariantMap &playbackMedia,
                                          const QString &url, const QString &title, const QVariantMap &headers,
                                          const QStringList &subtitleUrls, double startSeconds, double startPercent)
{
    abortStreamPrewarm();
    const QStringList extraArgs = QProcess::splitCommand(m_mpvExtraArgs);
    setPlaybackState(false, title);
    m_playbackBuffering = true;
    emit playbackStateChanged();
    setStatusMessage(QStringLiteral("Preparing stream…"));

    QNetworkReply *reply = prewarmStreamTail(url, headers);
    if (!reply) {
        m_playbackBuffering = false;
        emit playbackStateChanged();
        return false;
    }

    auto launchPlayback = [this, reply, playbackMedia, url, title, headers, subtitleUrls, extraArgs, startSeconds, startPercent]() {
        if (m_streamPrewarmReply != reply) {
            return;
        }
        if (m_player.play(url, title, headers, subtitleUrls,
                          m_subtitleLanguage,
                          m_mpvHardwareDecoding, m_mpvGpuNext, m_mpvHdrHint,
                          m_mpvModernz, m_mpvFullscreen, extraArgs,
                          startSeconds, startPercent)) {
            m_currentPlaybackMedia = playbackMedia;
        } else {
            m_playbackBuffering = false;
            emit playbackStateChanged();
        }
    };

    connect(reply, &QNetworkReply::readyRead, this, [this, reply, launchPlayback]() {
        reply->readAll();
        if (m_streamPrewarmReply != reply || m_streamPrewarmLaunchStarted) {
            return;
        }
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status >= 400) {
            return;
        }
        m_streamPrewarmLaunchStarted = true;
        launchPlayback();
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, launchPlayback]() {
        reply->readAll();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const bool ok = reply->error() == QNetworkReply::NoError && (status == 0 || status < 400);
        if (m_streamPrewarmReply == reply && !m_streamPrewarmLaunchStarted) {
            m_streamPrewarmLaunchStarted = true;
            if (ok) {
                launchPlayback();
            } else {
                m_playbackBuffering = false;
                emit playbackStateChanged();
                setStatusMessage(QStringLiteral("Stream did not become ready: %1").arg(reply->errorString()));
            }
        }
        if (m_streamPrewarmReply == reply) {
            m_streamPrewarmReply = nullptr;
        }
        reply->deleteLater();
    });

    return true;
}

QNetworkReply *ApplicationController::prewarmStreamTail(const QString &url, const QVariantMap &headers)
{
    const QString trimmed = url.trimmed();
    if (trimmed.isEmpty()) {
        setStatusMessage(QStringLiteral("Cannot play an empty stream URL"));
        return nullptr;
    }

    QNetworkRequest request{QUrl(trimmed)};
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("AIOStreamsLinux/0.1"));
    // Last 16 MB: comfortably covers the MKV Cues/SeekHead the demuxer reads at
    // open, without pulling a meaningful fraction of a multi-GB file.
    request.setRawHeader("Range", "bytes=-16777216");
    for (auto it = headers.constBegin(); it != headers.constEnd(); ++it) {
        const QString value = it.value().toString();
        if (shouldForwardPlaybackHeader(it.key(), value)) {
            request.setRawHeader(it.key().trimmed().toUtf8(), value.trimmed().toUtf8());
        }
    }

    QNetworkReply *reply = m_streamPrewarm.get(request);
    m_streamPrewarmReply = reply;
    m_streamPrewarmLaunchStarted = false;
    return reply;
}

void ApplicationController::abortStreamPrewarm()
{
    if (m_streamPrewarmReply) {
        m_streamPrewarmReply->abort();
        m_streamPrewarmReply = nullptr;
    }
    m_streamPrewarmLaunchStarted = false;
}

void ApplicationController::setPlaybackState(bool active, const QString &title)
{
    m_playbackActive = active;
    m_playbackBuffering = false;
    m_playbackPaused = false;
    m_playbackPosition = 0.0;
    m_playbackDuration = 0.0;
    m_playbackTitle = title;
    emit playbackStateChanged();
    emit playbackPositionChanged();
}

void ApplicationController::stopPlayback()
{
    abortStreamPrewarm();
    if (m_playbackBuffering && !m_playbackActive) {
        m_playbackBuffering = false;
        emit playbackStateChanged();
    }
    m_player.stop();
}

void ApplicationController::setPlaybackPaused(bool paused)
{
    m_player.setPaused(paused);
}

void ApplicationController::seekPlayback(double seconds)
{
    m_player.seek(seconds);
}

void ApplicationController::seekPlaybackRelative(double seconds)
{
    m_player.seekRelative(seconds);
}

void ApplicationController::removeContinueWatching(const QString &key)
{
    if (m_trakt.connected()) {
        setStatusMessage(QStringLiteral("Removing Trakt playback progress"));
        m_trakt.removePlaybackProgress(key);
        return;
    }
    m_watchHistory.remove(key);
    setStatusMessage(QStringLiteral("Playback progress removed"));
}

void ApplicationController::setPendingRemoteResume(const QString &type, const QString &id, double progressPercent)
{
    m_pendingRemoteResumeType = type;
    m_pendingRemoteResumeId = id;
    m_pendingRemoteResumePercent = std::clamp(progressPercent, 0.0, 99.0);
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
    emit sourceHealthChanged();
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
    m_resumeMetadata.setBaseUrl(trimmed);
    m_resumeMetadataRequests.clear();
    QSettings settings;
    settings.setValue(QStringLiteral("addons/metadataUrl"), trimmed);
    emit metadataUrlChanged();
    emit sourceHealthChanged();
    hydrateTraktResumeMetadata();
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
    emit sourceHealthChanged();
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

void ApplicationController::setMpvModernz(bool enabled)
{
    if (m_mpvModernz == enabled) {
        return;
    }

    m_mpvModernz = enabled;
    QSettings().setValue(QStringLiteral("mpv/modernz"), enabled);
    emit mpvModernzChanged();
    setStatusMessage(enabled ? QStringLiteral("ModernZ controls enabled")
                             : QStringLiteral("ModernZ controls disabled"));
}

void ApplicationController::setMpvFullscreen(bool enabled)
{
    if (m_mpvFullscreen == enabled) {
        return;
    }

    m_mpvFullscreen = enabled;
    QSettings().setValue(QStringLiteral("mpv/fullscreen"), enabled);
    emit mpvFullscreenChanged();
    setStatusMessage(enabled ? QStringLiteral("External mpv starts fullscreen")
                             : QStringLiteral("External mpv starts windowed"));
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

void ApplicationController::setTraktClientId(const QString &clientId)
{
    m_trakt.setClientId(clientId);
}

void ApplicationController::setTraktClientSecret(const QString &clientSecret)
{
    m_trakt.setClientSecret(clientSecret);
}

void ApplicationController::connectTrakt()
{
    m_trakt.startDeviceAuthorization();
}

void ApplicationController::cancelTraktAuth()
{
    m_trakt.cancelDeviceAuthorization();
}

void ApplicationController::disconnectTrakt()
{
    m_trakt.disconnectTrakt();
    emit continueWatchingChanged();
}

void ApplicationController::openTraktVerificationUrl()
{
    const QUrl url(m_trakt.verificationUrl());
    if (url.isValid()) {
        QDesktopServices::openUrl(url);
    }
}

void ApplicationController::copyTraktUserCode()
{
    if (QClipboard *clipboard = QGuiApplication::clipboard()) {
        clipboard->setText(m_trakt.userCode());
        setStatusMessage(QStringLiteral("Trakt code copied"));
    }
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
