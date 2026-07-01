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
    property string pendingRemoveResumeKey: ""
    property string pendingRemoveResumeTitle: ""

    readonly property bool isSeries: detailsItem && detailsItem.type === "series"
    readonly property var preparingMedia: appController.playbackMedia || ({})
    readonly property string preparingArtwork: preparingMedia.background || preparingMedia.thumbnail
                                                || preparingMedia.episodeThumbnail || preparingMedia.poster || ""
    readonly property string preparingLogo: preparingMedia.logo || ""
    readonly property string preparingTitle: preparingMedia.name || appController.playbackTitle || "Preparing stream"
    readonly property string preparingSubtitle: {
        if (preparingMedia.type === "series" && preparingMedia.season > 0 && preparingMedia.episode > 0) {
            const episode = "S" + preparingMedia.season + " · E" + preparingMedia.episode
            return preparingMedia.episodeTitle ? episode + " · " + preparingMedia.episodeTitle : episode
        }
        return ""
    }

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
        const streamId = item.type === "series" ? resumeEpisodeId : baseId
        appController.setPendingRemoteResume(item.source === "trakt" ? item.type : "",
                                             item.source === "trakt" ? streamId : "",
                                             item.source === "trakt" ? (item.progressPercent || 0) : 0)
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

    function sourceHealthState(name) {
        const health = appController.sourceHealth || []
        for (let i = 0; i < health.length; ++i) {
            if (health[i].name === name)
                return health[i].state || "neutral"
        }
        return "neutral"
    }

    function playStreamIndex(index) {
        appController.playStream(index)
    }

    function resumeItem(item) {
        if (!item.stream || !item.stream.url) {
            openDetails(item)
            return
        }

        appController.resumeContinueWatching(item.key)
    }

    function resumeRemoveTitle(item) {
        const name = item.name || "this item"
        if (item.type === "series" && item.season > 0 && item.episode > 0) {
            const episode = "S" + item.season + " E" + item.episode
            return item.episodeTitle ? name + " - " + episode + " - " + item.episodeTitle
                                     : name + " - " + episode
        }
        return name
    }

    function confirmRemoveResume(item) {
        if (!item || !item.key)
            return
        pendingRemoveResumeKey = item.key
        pendingRemoveResumeTitle = resumeRemoveTitle(item)
        removeResumeDialog.open()
    }

    // For a series, auto-select the first episode once its metadata arrives,
    // so releases for S01E01 load by default instead of showing stale ones.

    Dialog {
        id: removeResumeDialog
        modal: true
        focus: true
        width: Math.min(420, Math.max(280, root.width - Theme.s32 * 2))
        x: Math.round((root.width - width) / 2)
        y: Math.round((root.height - height) / 2)
        padding: Theme.s24
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            radius: Theme.rLg
            color: Theme.surface
            border.color: Theme.lineStrong
            border.width: 1
        }

        contentItem: ColumnLayout {
            spacing: Theme.s16
            implicitWidth: removeResumeDialog.width - Theme.s24 * 2

            Text {
                text: "Remove resume progress?"
                color: Theme.text
                font.pixelSize: Theme.fH3
                font.bold: true
                Layout.fillWidth: true
            }

            Text {
                text: root.pendingRemoveResumeTitle
                color: Theme.text
                font.pixelSize: Theme.fBody
                font.bold: true
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            Text {
                text: appController.traktConnected
                      ? "This removes the paused playback progress from Trakt. It will not mark the item watched."
                      : "This removes the saved local playback progress. It will not mark the item watched."
                color: Theme.textDim
                font.pixelSize: Theme.fBody
                lineHeight: 1.18
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: Theme.s8
                spacing: Theme.s12

                Item { Layout.fillWidth: true }

                AppButton {
                    text: "Cancel"
                    onClicked: removeResumeDialog.close()
                }

                AppButton {
                    text: "Remove"
                    variant: "danger"
                    onClicked: {
                        appController.removeContinueWatching(root.pendingRemoveResumeKey)
                        removeResumeDialog.close()
                    }
                }
            }
        }
    }

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
            visible: appController.loading || appController.playbackBuffering
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
                    running: appController.loading || appController.playbackBuffering
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
                        Layout.preferredHeight: 170
                        orientation: ListView.Horizontal
                        spacing: Theme.s16
                        clip: true
                        boundsBehavior: Flickable.StopAtBounds
                        model: appController.continueWatching
                        delegate: ResumeCard {
                            item: modelData
                            landscape: true
                            removable: true
                            onClicked: clickedItem => root.resumeItem(clickedItem)
                            onRemoveRequested: key => root.confirmRemoveResume(modelData)
                        }
                    }
                }

                // next up
                ColumnLayout {
                    id: nextUpRail
                    visible: appController.searchResults.length === 0 && appController.nextUp.length > 0
                    Layout.fillWidth: true
                    Layout.leftMargin: Theme.s32
                    Layout.rightMargin: Theme.s32
                    spacing: Theme.s12

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.s12
                        Rectangle { Layout.preferredWidth: 4; Layout.preferredHeight: 22; radius: 2; color: Theme.accentBright }
                        Text {
                            text: "Next Up"
                            color: Theme.text
                            font.pixelSize: Theme.fH3
                            font.bold: true
                        }
                        Text {
                            text: appController.nextUp.length + " shows"
                            color: Theme.textMute
                            font.pixelSize: Theme.fSmall
                            Layout.alignment: Qt.AlignBottom
                            bottomPadding: 4
                        }
                        Item { Layout.fillWidth: true }
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 170
                        orientation: ListView.Horizontal
                        spacing: Theme.s16
                        clip: true
                        boundsBehavior: Flickable.StopAtBounds
                        model: appController.nextUp
                        delegate: ResumeCard {
                            item: modelData
                            landscape: true
                            removable: false
                            onClicked: clickedItem => root.resumeItem(clickedItem)
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

                // hero details
                Item {
                    Layout.fillWidth: true
                    Layout.topMargin: Theme.s24
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

                    AppButton {
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.margins: Theme.s16
                        text: "‹  Back"
                        onClicked: root.page = 0
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
                    description: "Enter your AIOStreams manifest URL. Stored locally and used to find playable HTTP streams."
                    statusState: root.sourceHealthState("AIOStreams")

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
                    description: "Leave blank to use Cinemeta. Add an AIOMetadata or other Stremio-compatible manifest URL for details, artwork, episode stills, and episode ratings."
                    statusState: root.sourceHealthState("Metadata")

                    AddonUrlField {
                        value: appController.metadataUrl
                        placeholder: "https://your-aiometadata.example/manifest.json (blank = Cinemeta)"
                        statusOn: "Custom metadata addon configured"
                        statusOff: "Using Cinemeta (default)"
                        onSaveRequested: text => appController.metadataUrl = text
                        onRemoveRequested: appController.metadataUrl = ""
                    }
                }

                SettingsCard {
                    title: "Subtitles"
                    description: "Load subtitles from the OpenSubtitles Stremio addon. Choose the preferred language for mpv."
                    statusState: root.sourceHealthState("OpenSubtitles")

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
                    title: "Trakt"
                    description: "Sync resume progress and Next Up with Trakt. Enter a Trakt API app client ID and secret, then connect your account."
                    statusState: root.sourceHealthState("Trakt")

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: Theme.s8
                        spacing: Theme.s8

                        SettingsField {
                            id: traktClientIdField
                            Layout.fillWidth: true
                            text: appController.traktClientId
                            placeholderText: "Trakt API client ID"
                            onAccepted: appController.traktClientId = text
                            onTextEdited: appController.traktClientId = text
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.s8

                        SettingsField {
                            id: traktClientSecretField
                            Layout.fillWidth: true
                            text: appController.traktClientSecret
                            placeholderText: "Trakt API client secret"
                            echoMode: TextInput.Password
                            onAccepted: appController.traktClientSecret = text
                            onTextEdited: appController.traktClientSecret = text
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: appController.traktStatus || (appController.traktConnected ? "Trakt connected" : "Trakt not connected")
                        color: appController.traktConnected ? Theme.text : Theme.textDim
                        font.pixelSize: Theme.fSmall
                        wrapMode: Text.WordWrap
                    }

                    Rectangle {
                        visible: appController.traktAuthPending
                        Layout.fillWidth: true
                        Layout.preferredHeight: authDetails.implicitHeight + Theme.s16 * 2
                        radius: Theme.rMd
                        color: Theme.surfaceAlt
                        border.width: 1
                        border.color: Theme.line

                        ColumnLayout {
                            id: authDetails
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: Theme.s16
                            spacing: Theme.s8

                            Text {
                                Layout.fillWidth: true
                                text: "Open " + appController.traktVerificationUrl + " and enter this code:"
                                color: Theme.textDim
                                font.pixelSize: Theme.fSmall
                                wrapMode: Text.WordWrap
                            }
                            Text {
                                text: appController.traktUserCode
                                color: Theme.text
                                font.pixelSize: Theme.fH3
                                font.bold: true
                                font.letterSpacing: 2
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.s12

                                AppButton {
                                    text: "Open verification page"
                                    enabled: appController.traktVerificationUrl.length > 0
                                    onClicked: appController.openTraktVerificationUrl()
                                }
                                AppButton {
                                    text: "Copy code"
                                    enabled: appController.traktUserCode.length > 0
                                    onClicked: appController.copyTraktUserCode()
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.s12

                        AppButton {
                            text: appController.traktConnected ? "Reconnect" : "Connect Trakt"
                            variant: "primary"
                            enabled: !appController.traktBusy
                                     && appController.traktClientId.length > 0
                                     && appController.traktClientSecret.length > 0
                            onClicked: {
                                appController.traktClientId = traktClientIdField.text
                                appController.traktClientSecret = traktClientSecretField.text
                                appController.connectTrakt()
                            }
                        }
                        AppButton {
                            text: "Cancel connection"
                            visible: appController.traktAuthPending
                            onClicked: appController.cancelTraktAuth()
                        }
                        AppButton {
                            text: "Disconnect"
                            variant: "danger"
                            enabled: appController.traktConnected
                            onClicked: appController.disconnectTrakt()
                        }
                    }
                }

                SettingsCard {
                    title: "IMDb ratings"
                    description: "Downloads IMDb's public ratings dataset and caches scores locally for posters and episodes."
                    statusState: root.sourceHealthState("IMDb ratings")

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
                        text: "Show poster scores; episode ratings stay enabled"
                        checked: appController.showPosterRatings
                        onToggled: appController.showPosterRatings = checked
                    }
                }

                SettingsCard {
                    title: "Display"
                    description: "Adjust interface zoom on top of desktop scaling. Restart the app to apply changes."

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
                    description: "Launch streams in external mpv. Extra mpv arguments are appended last so they can override defaults."
                    statusState: root.sourceHealthState("mpv")

                    SettingsCheck {
                        text: "Use ModernZ mpv control overlay"
                        checked: appController.mpvModernz
                        onToggled: appController.mpvModernz = checked
                    }
                    SettingsCheck {
                        text: "Start external mpv fullscreen"
                        checked: appController.mpvFullscreen
                        onToggled: appController.mpvFullscreen = checked
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
                        text: "Use Vulkan and pass HDR color-space hints (--gpu-api=vulkan --target-colorspace-hint=yes)"
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
                            placeholderText: "Extra mpv arguments, e.g. --profile=high-quality --deband=yes"
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

    }

    // ===== Playback preparation ===========================================
    Item {
        id: preparingOverlay
        anchors.fill: parent
        z: 40
        visible: appController.playbackBuffering
        opacity: visible ? 1 : 0

        Behavior on opacity {
            NumberAnimation { duration: Theme.durMed }
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            hoverEnabled: true
        }

        Rectangle {
            anchors.fill: parent
            color: Theme.bg
        }

        Image {
            anchors.fill: parent
            source: root.preparingArtwork
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            opacity: 0.42
            visible: source != ""
        }

        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#ee090a10" }
                GradientStop { position: 0.46; color: "#aa090a10" }
                GradientStop { position: 1.0; color: "#f6090a10" }
            }
        }

        ColumnLayout {
            anchors.centerIn: parent
            width: Math.min(parent.width - Theme.s40 * 2, 560)
            spacing: Theme.s20

            Image {
                id: preparingLogoImage
                Layout.alignment: Qt.AlignHCenter
                Layout.maximumWidth: Math.min(420, preparingOverlay.width - Theme.s40 * 2)
                Layout.maximumHeight: 160
                source: root.preparingLogo
                fillMode: Image.PreserveAspectFit
                asynchronous: true
                visible: source != ""

                SequentialAnimation on opacity {
                    running: preparingOverlay.visible && preparingLogoImage.visible
                    loops: Animation.Infinite
                    NumberAnimation { from: 0.78; to: 1.0; duration: 900; easing.type: Easing.InOutSine }
                    NumberAnimation { from: 1.0; to: 0.78; duration: 900; easing.type: Easing.InOutSine }
                }
            }

            Text {
                Layout.fillWidth: true
                visible: root.preparingLogo === ""
                text: root.preparingTitle
                color: Theme.text
                font.pixelSize: Theme.fH1
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                visible: root.preparingSubtitle !== ""
                text: root.preparingSubtitle
                color: Theme.textDim
                font.pixelSize: Theme.fTitle
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                maximumLineCount: 1
                elide: Text.ElideRight
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: Theme.s12

                BusyIndicator {
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    running: preparingOverlay.visible
                }

                Text {
                    text: "Preparing stream"
                    color: Theme.textDim
                    font.pixelSize: Theme.fBody
                    font.bold: true
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
