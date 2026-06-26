import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import StremioLinux
import "components"

ApplicationWindow {
    id: root
    width: 1320
    height: 820
    minimumWidth: 960
    minimumHeight: 640
    visible: true
    title: "Miru"
    color: Theme.bg

    property int page: 0
    property var detailsItem: ({})
    property string baseId: ""
    property string selectedEpisodeId: ""
    property int previousPage: 0
    property string pendingPlaybackKind: ""
    property int pendingStreamIndex: -1
    property string pendingResumeKey: ""

    readonly property bool isSeries: detailsItem && detailsItem.type === "series"

    // "S1 · E1" label for the selected episode, derived from "ttID:season:episode"
    readonly property string episodeLabel: {
        if (!isSeries || selectedEpisodeId === "")
            return ""
        const parts = selectedEpisodeId.split(":")
        return parts.length >= 3 ? "S" + parts[1] + " · E" + parts[2] : ""
    }

    Component.onCompleted: appController.loadHome()

    function openDetails(item) {
        detailsItem = item
        baseId = item.type === "series" && item.baseId ? item.baseId : item.id
        const resumeEpisodeId = item.type === "series" && item.baseId && item.season > 0 && item.episode > 0
                              ? baseId + ":" + item.season + ":" + item.episode
                              : ""
        selectedEpisodeId = resumeEpisodeId
        appController.clearStreams()   // drop any stale releases from the last view
        page = 1
        appController.loadMeta(item.type, baseId)
        if (item.type !== "series")
            appController.loadStreams(item.type, baseId)
        else if (resumeEpisodeId !== "")
            appController.loadStreams("series", resumeEpisodeId)
    }

    function playEpisode(video) {
        selectedEpisodeId = video.id
        appController.loadStreams("series", root.baseId + ":" + video.season + ":" + video.episode)
    }

    // Search from anywhere and surface the results on the home page.
    function runSearch(query) {
        if (query.trim().length === 0)
            return
        appController.search(query)
        page = 0
    }

    function searchResultsByType(type) {
        const out = []
        const results = appController.searchResults || []
        for (let i = 0; i < results.length; ++i) {
            if (results[i].type === type)
                out.push(results[i])
        }
        return out
    }

    function playStreamIndex(index) {
        if (appController.playerMode === "embedded") {
            previousPage = page
            pendingPlaybackKind = "stream"
            pendingStreamIndex = index
            pendingResumeKey = ""
            page = 3
            Qt.callLater(startPendingPlayback)
        } else {
            appController.playStream(index)
        }
    }

    function resumeItem(item) {
        if (!item.stream || !item.stream.url) {
            openDetails(item)
            return
        }

        if (appController.playerMode === "embedded") {
            previousPage = page
            pendingPlaybackKind = "resume"
            pendingStreamIndex = -1
            pendingResumeKey = item.key
            page = 3
            Qt.callLater(startPendingPlayback)
        } else {
            appController.resumeContinueWatching(item.key)
        }
    }

    function startPendingPlayback() {
        if (page !== 3 || pendingPlaybackKind === "")
            return
        if (!playerSurface || playerSurface.windowId === 0) {
            Qt.callLater(startPendingPlayback)
            return
        }

        let embeddedStarted = false
        if (pendingPlaybackKind === "stream")
            embeddedStarted = appController.playStreamEmbedded(pendingStreamIndex, playerSurface.windowId)
        else if (pendingPlaybackKind === "resume")
            embeddedStarted = appController.resumeContinueWatchingEmbedded(pendingResumeKey, playerSurface.windowId)

        pendingPlaybackKind = ""
        pendingStreamIndex = -1
        pendingResumeKey = ""
        if (!embeddedStarted)
            page = previousPage
    }

    function closePlayer() {
        appController.stopPlayback()
        pendingPlaybackKind = ""
        page = previousPage
    }

    function formatTime(seconds) {
        if (!seconds || seconds < 0)
            return "0:00"
        const total = Math.floor(seconds)
        const h = Math.floor(total / 3600)
        const m = Math.floor((total % 3600) / 60)
        const s = total % 60
        const ss = s < 10 ? "0" + s : "" + s
        if (h > 0)
            return h + ":" + (m < 10 ? "0" + m : "" + m) + ":" + ss
        return m + ":" + ss
    }

    // For a series, auto-select the first episode once its metadata arrives,
    // so releases for S01E01 load by default instead of showing stale ones.

    // ===== Background =======================================================
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#12151f" }
            GradientStop { position: 0.55; color: Theme.bg }
            GradientStop { position: 1.0; color: "#05060a" }
        }
    }

    // ===== Top bar ==========================================================
    header: Rectangle {
        implicitHeight: 64
        color: Theme.bgElevated

        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.line }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.s24
            anchors.rightMargin: Theme.s24
            spacing: Theme.s16

            // brand
            Row {
                spacing: Theme.s8
                Rectangle {
                    width: 12; height: 12; radius: 6
                    anchors.verticalCenter: parent.verticalCenter
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: Theme.accent }
                        GradientStop { position: 1.0; color: Theme.accentBright }
                    }
                }
                Text {
                    text: "Miru"
                    color: Theme.text
                    font.pixelSize: Theme.fH3
                    font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            // divider
            Rectangle {
                Layout.preferredWidth: 1
                Layout.preferredHeight: 26
                Layout.leftMargin: Theme.s8
                Layout.rightMargin: Theme.s8
                Layout.alignment: Qt.AlignVCenter
                color: Theme.line
            }

            // primary navigation
            AppButton {
                text: "Home"
                variant: "nav"
                active: root.page === 0
                onClicked: {
                    if (root.page === 3)
                        appController.stopPlayback()
                    root.pendingPlaybackKind = ""
                    searchField.clear()
                    appController.clearSearch()
                    root.page = 0
                }
            }
            AppButton {
                text: "Settings"
                variant: "nav"
                active: root.page === 2
                onClicked: {
                    if (root.page === 3)
                        appController.stopPlayback()
                    root.pendingPlaybackKind = ""
                    root.page = 2
                }
            }

            Item { Layout.fillWidth: true }

            // search (Enter to search; jumps to results from anywhere)
            SearchField {
                id: searchField
                Layout.preferredWidth: 340
                Layout.maximumWidth: 440
                Layout.alignment: Qt.AlignVCenter
                placeholderText: "Search movies and shows"
                onAccepted: root.runSearch(text)
                onCleared: appController.clearSearch()
            }
        }

        // indeterminate loading bar
        Rectangle {
            anchors.bottom: parent.bottom
            height: 2
            width: parent.width
            color: "transparent"
            visible: appController.loading
            clip: true
            Rectangle {
                id: loadingBar
                width: parent.width * 0.3
                height: parent.height
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: "transparent" }
                    GradientStop { position: 0.5; color: Theme.accentBright }
                    GradientStop { position: 1.0; color: "transparent" }
                }
                XAnimator on x {
                    running: appController.loading
                    loops: Animation.Infinite
                    from: -loadingBar.width
                    to: root.width
                    duration: 1100
                }
            }
        }
    }

    // ===== Pages ============================================================
    StackLayout {
        anchors.fill: parent
        currentIndex: root.page

        // ---- Home ----------------------------------------------------------
        ScrollView {
            contentWidth: availableWidth
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                width: parent.width
                spacing: Theme.s32

                // continue watching
                ColumnLayout {
                    id: continueWatchingRail
                    visible: appController.searchResults.length === 0 && appController.continueWatching.length > 0
                    Layout.fillWidth: true
                    Layout.topMargin: Theme.s24
                    Layout.leftMargin: Theme.s32
                    Layout.rightMargin: Theme.s32
                    spacing: Theme.s12

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.s12
                        Rectangle { Layout.preferredWidth: 4; Layout.preferredHeight: 22; radius: 2; color: Theme.accent }
                        Text {
                            text: "Continue Watching"
                            color: Theme.text
                            font.pixelSize: Theme.fH3
                            font.bold: true
                        }
                        Text {
                            text: appController.continueWatching.length + " in progress"
                            color: Theme.textMute
                            font.pixelSize: Theme.fSmall
                            Layout.alignment: Qt.AlignBottom
                            bottomPadding: 4
                        }
                        Item { Layout.fillWidth: true }
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 326
                        orientation: ListView.Horizontal
                        spacing: Theme.s16
                        clip: true
                        boundsBehavior: Flickable.StopAtBounds
                        model: appController.continueWatching
                        delegate: ResumeCard {
                            item: modelData
                            onClicked: clickedItem => root.resumeItem(clickedItem)
                            onRemoveRequested: key => appController.removeContinueWatching(key)
                        }
                    }
                }

                ColumnLayout {
                    id: searchAndHomeColumn

                    Layout.fillWidth: true
                    Layout.leftMargin: Theme.s32
                    Layout.rightMargin: Theme.s32
                    Layout.bottomMargin: Theme.s32
                    spacing: Theme.s32

                    readonly property var movieSearchResults: root.searchResultsByType("movie")
                    readonly property var seriesSearchResults: root.searchResultsByType("series")

                    // search results (while searching): one scrollable row per type
                    SectionRail {
                        visible: searchAndHomeColumn.movieSearchResults.length > 0
                        title: "Movies"
                        subtitle: searchAndHomeColumn.movieSearchResults.length + " matches"
                        model: searchAndHomeColumn.movieSearchResults
                        onOpenRequested: item => root.openDetails(item)
                    }

                    SectionRail {
                        visible: searchAndHomeColumn.seriesSearchResults.length > 0
                        title: "TV Shows"
                        subtitle: searchAndHomeColumn.seriesSearchResults.length + " matches"
                        model: searchAndHomeColumn.seriesSearchResults
                        onOpenRequested: item => root.openDetails(item)
                    }

                    // category rails discovered from the metadata addon.
                    // Index-stable so loading one catalog doesn't rebuild all rails.
                    Repeater {
                        model: appController.searchResults.length > 0 ? 0 : appController.homeSections.length
                        delegate: SectionRail {
                            required property int index
                            readonly property var section: appController.homeSections[index] || ({})
                            Layout.fillWidth: true
                            title: section.title !== undefined ? section.title : ""
                            model: section.items !== undefined ? section.items : []
                            loading: appController.loading && (!section.items || section.items.length === 0)
                            onOpenRequested: item => root.openDetails(item)
                        }
                    }
                }
            }
        }

        // ---- Details -------------------------------------------------------
        ScrollView {
            contentWidth: availableWidth
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                width: parent.width
                spacing: Theme.s24

                RowLayout {
                    Layout.topMargin: Theme.s24
                    Layout.leftMargin: Theme.s32
                    AppButton {
                        text: "‹  Back"
                        onClicked: root.page = 0
                    }
                }

                // hero details
                Item {
                    Layout.fillWidth: true
                    Layout.leftMargin: Theme.s32
                    Layout.rightMargin: Theme.s32
                    implicitHeight: 380

                    Rectangle {
                        anchors.fill: parent
                        radius: Theme.rXl
                        clip: true
                        color: Theme.surface

                        Image {
                            anchors.fill: parent
                            source: appController.selectedMeta.background
                                    || root.detailsItem.background || root.detailsItem.poster || ""
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            opacity: 0.4
                        }
                        Rectangle {
                            anchors.fill: parent
                            gradient: Gradient {
                                GradientStop { position: 0.0; color: "#330c0e16" }
                                GradientStop { position: 1.0; color: "#f00c0e16" }
                            }
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: Theme.s32
                            spacing: Theme.s24

                            // poster
                            Image {
                                Layout.preferredWidth: 170
                                Layout.preferredHeight: 255
                                Layout.alignment: Qt.AlignBottom
                                source: appController.selectedMeta.poster || root.detailsItem.poster || ""
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                                visible: source != ""
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                spacing: Theme.s12

                                Item { Layout.fillHeight: true }

                                Text {
                                    Layout.fillWidth: true
                                    text: appController.selectedMeta.name || root.detailsItem.name || "Details"
                                    color: Theme.text
                                    font.pixelSize: Theme.fH2
                                    font.bold: true
                                    wrapMode: Text.WordWrap
                                    maximumLineCount: 2
                                    elide: Text.ElideRight
                                }

                                // meta chips
                                Flow {
                                    Layout.fillWidth: true
                                    spacing: Theme.s8
                                    Pill {
                                        visible: appController.showPosterRatings
                                                 && !!appController.selectedMeta.imdbRating
                                        text: "★ " + (appController.selectedMeta.imdbRating || "")
                                        accentColor: Theme.gold
                                    }
                                    Pill {
                                        visible: !!appController.selectedMeta.releaseInfo
                                        text: appController.selectedMeta.releaseInfo || ""
                                        accentColor: Theme.lineStrong
                                    }
                                    Pill {
                                        visible: !!appController.selectedMeta.runtime
                                        text: appController.selectedMeta.runtime || ""
                                        accentColor: Theme.lineStrong
                                    }
                                    Pill {
                                        text: root.isSeries ? "SERIES" : "MOVIE"
                                        accentColor: Theme.accent
                                    }
                                }

                                // genres
                                Flow {
                                    Layout.fillWidth: true
                                    spacing: Theme.s8
                                    Repeater {
                                        model: appController.selectedMeta.genres || []
                                        delegate: Pill {
                                            required property var modelData
                                            text: modelData
                                            accentColor: Theme.accentBright
                                        }
                                    }
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: appController.selectedMeta.description || root.detailsItem.description || ""
                                    color: Theme.textDim
                                    font.pixelSize: Theme.fBody
                                    wrapMode: Text.WordWrap
                                    maximumLineCount: 4
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                }

                // episodes (left) + releases (right)
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: Theme.s32
                    Layout.rightMargin: Theme.s32
                    Layout.bottomMargin: Theme.s32
                    Layout.alignment: Qt.AlignTop
                    spacing: Theme.s24

                    // ---- Episodes (series only) ----
                    ColumnLayout {
                        visible: root.isSeries && (appController.selectedMeta.videos || []).length > 0
                        Layout.fillWidth: true
                        Layout.horizontalStretchFactor: 11
                        Layout.alignment: Qt.AlignTop
                        spacing: Theme.s16

                        RowLayout {
                            spacing: Theme.s12
                            Rectangle { Layout.preferredWidth: 4; Layout.preferredHeight: 22; radius: 2; color: Theme.accent }
                            Text {
                                text: "Episodes"
                                color: Theme.text
                                font.pixelSize: Theme.fH3
                                font.bold: true
                            }
                        }

                        EpisodeList {
                            Layout.fillWidth: true
                            videos: appController.selectedMeta.videos || []
                            selectedEpisodeId: root.selectedEpisodeId
                            onEpisodeSelected: video => root.playEpisode(video)
                        }
                    }

                    // ---- Releases for the selected episode / movie ----
                    ColumnLayout {
                        id: releasesColumn
                        Layout.fillWidth: true
                        Layout.horizontalStretchFactor: 9
                        Layout.alignment: Qt.AlignTop
                        spacing: Theme.s12

                        // Client-side filter over the loaded releases. Each
                        // entry keeps its original index so playStream() still
                        // targets the right stream in the unfiltered list.
                        property string releaseFilter: ""
                        readonly property var filteredStreams: {
                            var all = appController.streams
                            var q = releaseFilter.trim().toLowerCase()
                            var out = []
                            for (var i = 0; i < all.length; ++i) {
                                var s = all[i]
                                if (q.length === 0
                                    || ((s.title || "") + " " + (s.filename || "") + " "
                                        + (s.name || "") + " " + (s.description || ""))
                                       .toLowerCase().indexOf(q) !== -1)
                                    out.push({ stream: s, index: i })
                            }
                            return out
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.s12
                            Rectangle { Layout.preferredWidth: 4; Layout.preferredHeight: 22; radius: 2; color: Theme.accent }
                            Text {
                                text: "Releases"
                                color: Theme.text
                                font.pixelSize: Theme.fH3
                                font.bold: true
                            }
                            Pill {
                                visible: root.episodeLabel.length > 0
                                text: root.episodeLabel
                                accentColor: Theme.accent
                            }
                            Pill {
                                visible: appController.streams.length > 0
                                text: appController.streams.length + " sources"
                                accentColor: Theme.success
                            }
                            Item { Layout.fillWidth: true }
                        }

                        Text {
                            visible: appController.streams.length > 0
                            text: "Click a release to play in mpv"
                            color: Theme.textMute
                            font.pixelSize: Theme.fSmall
                        }

                        // filter the loaded releases by name / quality / group
                        SearchField {
                            id: releaseFilterField
                            visible: appController.streams.length > 0
                            Layout.fillWidth: true
                            placeholderText: "Filter releases… (e.g. 2160p, FLUX, LOCAL, REMUX)"
                            onTextChanged: releasesColumn.releaseFilter = text
                            onCleared: releasesColumn.releaseFilter = ""
                        }

                        // searching / empty hint
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 110
                            radius: Theme.rLg
                            color: "transparent"
                            border.color: Theme.line
                            border.width: 1
                            visible: appController.streams.length === 0

                            SearchSpinner {
                                anchors.centerIn: parent
                                visible: appController.streamsLoading
                                label: "Searching for releases…"
                            }

                            Text {
                                anchors.centerIn: parent
                                width: parent.width - Theme.s32
                                horizontalAlignment: Text.AlignHCenter
                                visible: !appController.streamsLoading
                                text: root.isSeries && root.selectedEpisodeId === ""
                                      ? "Select an episode to load releases"
                                      : "No releases found"
                                color: Theme.textMute
                                font.pixelSize: Theme.fBody
                                wrapMode: Text.WordWrap
                            }
                        }

                        // no-match hint when the filter excludes everything
                        Text {
                            visible: appController.streams.length > 0
                                     && releasesColumn.filteredStreams.length === 0
                            text: "No releases match the filter"
                            color: Theme.textMute
                            font.pixelSize: Theme.fBody
                            Layout.topMargin: Theme.s8
                        }

                        Repeater {
                            model: releasesColumn.filteredStreams
                            delegate: StreamCard {
                                required property var modelData
                                Layout.fillWidth: true
                                stream: modelData.stream
                                onPlayRequested: root.playStreamIndex(modelData.index)
                            }
                        }
                    }
                }
            }
        }

        // ---- Settings ------------------------------------------------------
        ScrollView {
            contentWidth: availableWidth
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                width: parent.width
                spacing: Theme.s24

                Text {
                    text: "Settings"
                    color: Theme.text
                    font.pixelSize: Theme.fH2
                    font.bold: true
                    Layout.topMargin: Theme.s32
                    Layout.leftMargin: Theme.s32
                }

                SettingsCard {
                    title: "AIOStreams addon"
                    description: "Paste your AIOStreams addon base URL or its manifest.json URL. Stored locally. Remove and re-add after changing your AIOStreams backend, since reconfiguring it gives a new addon URL."

                    AddonUrlField {
                        value: appController.aioStreamsUrl
                        placeholder: "https://your-aiostreams-addon.example/manifest.json"
                        statusOn: "Addon configured"
                        statusOff: "No addon configured"
                        onSaveRequested: text => appController.setAioStreamsUrl(text)
                        onRemoveRequested: appController.setAioStreamsUrl("")
                    }
                }

                SettingsCard {
                    title: "Metadata addon"
                    description: "Source for detailed metadata: per-episode IMDb/TMDB ratings, episode stills and overviews. Defaults to Cinemeta, which lacks most episode ratings. Point this at a self-hosted AIOMetadata (Stremio addon URL or manifest.json) for full TMDB episode data. Catalogs and search still use Cinemeta."

                    AddonUrlField {
                        value: appController.metadataUrl
                        placeholder: "https://your-aiometadata.example/manifest.json (blank = Cinemeta)"
                        statusOn: "AIOMetadata configured"
                        statusOff: "Using Cinemeta (default)"
                        onSaveRequested: text => appController.metadataUrl = text
                        onRemoveRequested: appController.metadataUrl = ""
                    }
                }

                SettingsCard {
                    title: "Subtitles"
                    description: "Automatic subtitles via the OpenSubtitles addon. The preferred language is fetched for each title and loaded into mpv on playback."

                    Flow {
                        Layout.fillWidth: true
                        Layout.topMargin: Theme.s8
                        spacing: Theme.s8
                        Repeater {
                            model: [
                                { code: "off", label: "Off" },
                                { code: "eng", label: "English" },
                                { code: "spa", label: "Spanish" },
                                { code: "fre", label: "French" },
                                { code: "ger", label: "German" },
                                { code: "ita", label: "Italian" },
                                { code: "por", label: "Portuguese" },
                                { code: "pob", label: "Portuguese (BR)" },
                                { code: "dut", label: "Dutch" },
                                { code: "pol", label: "Polish" },
                                { code: "rus", label: "Russian" },
                                { code: "ara", label: "Arabic" }
                            ]
                            delegate: ChoiceChip {
                                required property var modelData
                                label: modelData.label
                                current: modelData.code === appController.subtitleLanguage
                                onClicked: appController.subtitleLanguage = modelData.code
                            }
                        }
                    }
                }

                SettingsCard {
                    title: "IMDb ratings"
                    description: "Real IMDb ratings for posters and episodes, built from IMDb's public dataset and cached locally. Refreshes automatically about once a week."

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: Theme.s8
                        spacing: Theme.s12

                        RowLayout {
                            spacing: Theme.s8
                            Rectangle {
                                Layout.preferredWidth: 8; Layout.preferredHeight: 8; radius: 4
                                Layout.alignment: Qt.AlignVCenter
                                color: Theme.gold
                            }
                            Text {
                                text: appController.imdbRatingsUpdated
                                color: Theme.textDim
                                font.pixelSize: Theme.fSmall
                            }
                        }
                        Item { Layout.fillWidth: true }
                        AppButton {
                            text: "Refresh now"
                            onClicked: appController.refreshImdbRatings()
                        }
                    }

                    SettingsCheck {
                        text: "Show IMDb scores on posters (keeps episode ratings)"
                        checked: appController.showPosterRatings
                        onToggled: appController.showPosterRatings = checked
                    }
                }

                SettingsCard {
                    title: "Display"
                    description: "Interface zoom, applied on top of your desktop's scaling. Takes effect after restarting the app."

                    Flow {
                        Layout.fillWidth: true
                        Layout.topMargin: Theme.s8
                        spacing: Theme.s8
                        Repeater {
                            model: [
                                { scale: 0.9, label: "90%" },
                                { scale: 1.0, label: "100%" },
                                { scale: 1.1, label: "110%" },
                                { scale: 1.25, label: "125%" },
                                { scale: 1.5, label: "150%" }
                            ]
                            delegate: ChoiceChip {
                                required property var modelData
                                label: modelData.label
                                current: Math.abs(modelData.scale - appController.uiScale) < 0.001
                                onClicked: appController.uiScale = modelData.scale
                            }
                        }
                    }
                }

                SettingsCard {
                    title: "Playback"
                    description: "Choose embedded playback inside the app or the detached external mpv fallback. The app always passes stream URL, auth headers, subtitles and safe network options; rendering/HDR choices are configurable below. Custom args are appended last so they can override the defaults."

                    Flow {
                        Layout.fillWidth: true
                        Layout.topMargin: Theme.s8
                        spacing: Theme.s8
                        ChoiceChip {
                            label: "External"
                            current: appController.playerMode === "external"
                            onClicked: appController.playerMode = "external"
                        }
                        ChoiceChip {
                            label: "Embedded"
                            current: appController.playerMode === "embedded"
                            onClicked: appController.playerMode = "embedded"
                        }
                    }

                    SettingsCheck {
                        text: "Hardware decoding (--hwdec=auto-safe)"
                        checked: appController.mpvHardwareDecoding
                        onToggled: appController.mpvHardwareDecoding = checked
                    }
                    SettingsCheck {
                        text: "Use gpu-next renderer (--vo=gpu-next)"
                        checked: appController.mpvGpuNext
                        onToggled: appController.mpvGpuNext = checked
                    }
                    SettingsCheck {
                        text: "HDR/Vulkan hint (--gpu-api=vulkan --target-colorspace-hint=yes)"
                        checked: appController.mpvHdrHint
                        onToggled: appController.mpvHdrHint = checked
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: Theme.s8
                        spacing: Theme.s12

                        SettingsField {
                            id: mpvArgsField
                            Layout.fillWidth: true
                            text: appController.mpvExtraArgs
                            placeholderText: "Custom mpv args, e.g. --profile=high-quality --deband=yes"
                            onAccepted: appController.mpvExtraArgs = text
                        }

                        AppButton {
                            text: "Save"
                            variant: "primary"
                            onClicked: appController.mpvExtraArgs = mpvArgsField.text
                        }

                        AppButton {
                            text: "Clear"
                            enabled: appController.mpvExtraArgs.length > 0
                            onClicked: {
                                appController.mpvExtraArgs = ""
                                mpvArgsField.clear()
                            }
                        }
                    }
                }

                // bottom padding so the last card clears the status bar
                Item { Layout.preferredHeight: Theme.s32 }
            }
        }

        // ---- Player --------------------------------------------------------
        Item {
            Rectangle {
                anchors.fill: parent
                color: "#020308"
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.s24
                spacing: Theme.s16

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.s12

                    AppButton {
                        text: "‹  Back"
                        onClicked: root.closePlayer()
                    }
                    Text {
                        Layout.fillWidth: true
                        text: appController.playbackTitle || "Embedded player"
                        color: Theme.text
                        font.pixelSize: Theme.fH3
                        font.bold: true
                        elide: Text.ElideRight
                    }
                    Pill {
                        text: "Embedded mpv"
                        accentColor: Theme.accent
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 360
                    color: "black"
                    border.color: Theme.line
                    border.width: 1
                    clip: true

                    MpvVideoSurface {
                        id: playerSurface
                        anchors.fill: parent
                        anchors.margins: 1
                        onWindowIdChanged: root.startPendingPlayback()
                    }

                    SearchSpinner {
                        anchors.centerIn: parent
                        visible: appController.playbackBuffering
                                 || (!appController.playbackActive && root.pendingPlaybackKind !== "")
                        label: "Opening stream…"
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 92
                    radius: Theme.rLg
                    color: Theme.bgElevated
                    border.color: Theme.line
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Theme.s12
                        spacing: Theme.s8

                        Slider {
                            id: seekSlider
                            Layout.fillWidth: true
                            from: 0
                            to: Math.max(1, appController.playbackDuration)
                            value: 0
                            enabled: appController.playbackDuration > 0
                            onMoved: appController.seekPlayback(value)

                            Binding on value {
                                when: !seekSlider.pressed
                                value: appController.playbackPosition
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.s8

                            AppButton {
                                text: appController.playbackPaused ? "Play" : "Pause"
                                enabled: appController.playbackActive
                                onClicked: appController.setPlaybackPaused(!appController.playbackPaused)
                            }
                            AppButton {
                                text: "-10s"
                                enabled: appController.playbackActive
                                onClicked: appController.seekPlaybackRelative(-10)
                            }
                            AppButton {
                                text: "+10s"
                                enabled: appController.playbackActive
                                onClicked: appController.seekPlaybackRelative(10)
                            }
                            AppButton {
                                text: "Stop"
                                variant: "danger"
                                enabled: appController.playbackActive
                                onClicked: root.closePlayer()
                            }

                            Item { Layout.fillWidth: true }

                            Text {
                                text: root.formatTime(appController.playbackPosition)
                                      + " / " + root.formatTime(appController.playbackDuration)
                                color: Theme.textDim
                                font.pixelSize: Theme.fSmall
                            }
                        }
                    }
                }
            }
        }
    }

    // ===== Status bar =======================================================
    footer: Rectangle {
        implicitHeight: 34
        color: Theme.bgElevated
        Rectangle { width: parent.width; height: 1; color: Theme.line }
        RowLayout {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: Theme.s24
            spacing: Theme.s8
            Rectangle {
                Layout.preferredWidth: 8; Layout.preferredHeight: 8; radius: 4
                Layout.alignment: Qt.AlignVCenter
                color: appController.loading || appController.playbackBuffering ? Theme.gold : Theme.success
            }
            Text {
                text: appController.statusMessage || "Ready"
                color: Theme.textDim
                font.pixelSize: Theme.fSmall
            }
        }
    }
}
