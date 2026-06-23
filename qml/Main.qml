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
    title: "AIOStreams Linux"
    color: Theme.bg

    property int page: 0
    property var detailsItem: ({})
    property string baseId: ""
    property string selectedEpisodeId: ""

    readonly property bool isSeries: detailsItem && detailsItem.type === "series"
    readonly property var featured: (appController.featured && appController.featured.name) ? appController.featured : null

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
        baseId = item.id
        selectedEpisodeId = ""
        appController.clearStreams()   // drop any stale releases from the last view
        page = 1
        appController.loadMeta(item.type, item.id)
        if (item.type !== "series")
            appController.loadStreams(item.type, item.id)
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

    // For a series, auto-select the first episode once its metadata arrives,
    // so releases for S01E01 load by default instead of showing stale ones.
    Connections {
        target: appController
        function onSelectedMetaChanged() {
            if (!root.isSeries || root.selectedEpisodeId !== "")
                return
            const meta = appController.selectedMeta
            if (!meta || meta.id !== root.baseId)
                return
            const videos = meta.videos || []
            let first = null
            for (let i = 0; i < videos.length; ++i) {
                const v = videos[i]
                if (v.season > 0 && (first === null
                        || v.season < first.season
                        || (v.season === first.season && v.episode < first.episode)))
                    first = v
            }
            if (first)
                root.playEpisode(first)
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
                    text: "AIOStreams"
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
                active: root.page === 0 || root.page === 1
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
                onClicked: root.page = 2
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

                // hero
                Item {
                    Layout.fillWidth: true
                    Layout.topMargin: Theme.s24
                    Layout.leftMargin: Theme.s32
                    Layout.rightMargin: Theme.s32
                    implicitHeight: 320

                    Rectangle {
                        anchors.fill: parent
                        radius: Theme.rXl
                        clip: true
                        color: Theme.surface

                        Image {
                            anchors.fill: parent
                            source: root.featured ? (root.featured.background || root.featured.poster || "") : ""
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            opacity: 0.55
                        }
                        Rectangle {
                            anchors.fill: parent
                            gradient: Gradient {
                                orientation: Gradient.Horizontal
                                GradientStop { position: 0.0; color: "#ee0c0e16" }
                                GradientStop { position: 0.6; color: "#660c0e16" }
                                GradientStop { position: 1.0; color: "transparent" }
                            }
                        }

                        ColumnLayout {
                            anchors.left: parent.left
                            anchors.bottom: parent.bottom
                            anchors.top: parent.top
                            anchors.margins: Theme.s40
                            width: Math.min(640, parent.width * 0.6)
                            spacing: Theme.s12

                            Item { Layout.fillHeight: true }

                            Pill {
                                text: root.featured ? "FEATURED" : "WELCOME"
                                accentColor: Theme.accent
                            }
                            Text {
                                Layout.fillWidth: true
                                text: root.featured ? root.featured.name : "Cinematic Linux streaming"
                                color: Theme.text
                                font.pixelSize: Theme.fH1
                                font.bold: true
                                wrapMode: Text.WordWrap
                                maximumLineCount: 2
                                elide: Text.ElideRight
                            }
                            Text {
                                Layout.fillWidth: true
                                text: root.featured && root.featured.description
                                      ? root.featured.description
                                      : "Browse Cinemeta catalogs, resolve AIOStreams HTTP sources, and play with mpv."
                                color: Theme.textDim
                                font.pixelSize: Theme.fBody
                                wrapMode: Text.WordWrap
                                maximumLineCount: 3
                                elide: Text.ElideRight
                            }
                            AppButton {
                                visible: !!root.featured
                                text: "View details  ›"
                                variant: "primary"
                                Layout.topMargin: Theme.s8
                                onClicked: root.openDetails(root.featured)
                            }
                            Item { Layout.fillHeight: true }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: Theme.s32
                    Layout.rightMargin: Theme.s32
                    Layout.bottomMargin: Theme.s32
                    spacing: Theme.s32

                    // search results (while searching)
                    SectionRail {
                        visible: appController.searchResults.length > 0
                        title: "Search Results"
                        subtitle: appController.searchResults.length + " matches"
                        model: appController.searchResults
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
                                        visible: !!appController.selectedMeta.imdbRating
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
                        Layout.fillWidth: true
                        Layout.horizontalStretchFactor: 9
                        Layout.alignment: Qt.AlignTop
                        spacing: Theme.s12

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

                        Repeater {
                            model: appController.streams
                            delegate: StreamCard {
                                required property var modelData
                                required property int index
                                Layout.fillWidth: true
                                stream: modelData
                                onPlayRequested: appController.playStream(index)
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

                // AIOStreams card
                Rectangle {
                    Layout.fillWidth: true
                    Layout.maximumWidth: 760
                    Layout.leftMargin: Theme.s32
                    radius: Theme.rLg
                    color: Theme.surface
                    border.color: Theme.line
                    implicitHeight: aioCol.implicitHeight + Theme.s40

                    ColumnLayout {
                        id: aioCol
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.margins: Theme.s24
                        spacing: Theme.s12

                        Text {
                            text: "AIOStreams addon"
                            color: Theme.text
                            font.pixelSize: Theme.fTitle
                            font.bold: true
                        }
                        Text {
                            Layout.fillWidth: true
                            text: "Paste your AIOStreams addon base URL or its manifest.json URL. Stored locally. Remove and re-add after changing your AIOStreams backend, since reconfiguring it gives a new addon URL."
                            color: Theme.textDim
                            font.pixelSize: Theme.fSmall
                            wrapMode: Text.WordWrap
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: Theme.s8
                            spacing: Theme.s12

                            TextField {
                                id: aioUrlField
                                Layout.fillWidth: true
                                text: appController.aioStreamsUrl
                                placeholderText: "https://your-aiostreams-addon.example/manifest.json"
                                color: Theme.text
                                placeholderTextColor: Theme.textMute
                                font.pixelSize: Theme.fBody
                                selectionColor: Theme.accent
                                selectByMouse: true
                                leftPadding: Theme.s16
                                rightPadding: Theme.s16
                                topPadding: Theme.s12
                                bottomPadding: Theme.s12
                                background: Rectangle {
                                    radius: Theme.rMd
                                    color: Theme.surfaceAlt
                                    border.width: 1
                                    border.color: aioUrlField.activeFocus ? Theme.accent : Theme.line
                                    Behavior on border.color { ColorAnimation { duration: Theme.durFast } }
                                }
                            }

                            AppButton {
                                text: appController.aioStreamsUrl.length > 0 ? "Update" : "Save"
                                variant: "primary"
                                onClicked: appController.setAioStreamsUrl(aioUrlField.text)
                            }

                            AppButton {
                                text: "Remove"
                                variant: "danger"
                                enabled: appController.aioStreamsUrl.length > 0
                                onClicked: {
                                    appController.setAioStreamsUrl("")
                                    aioUrlField.clear()
                                }
                            }
                        }

                        // connection status
                        RowLayout {
                            spacing: Theme.s8
                            Rectangle {
                                Layout.preferredWidth: 8; Layout.preferredHeight: 8; radius: 4
                                Layout.alignment: Qt.AlignVCenter
                                color: appController.aioStreamsUrl.length > 0 ? Theme.success : Theme.textMute
                            }
                            Text {
                                text: appController.aioStreamsUrl.length > 0 ? "Addon configured" : "No addon configured"
                                color: Theme.textDim
                                font.pixelSize: Theme.fSmall
                            }
                        }
                    }
                }

                // Metadata addon card
                Rectangle {
                    Layout.fillWidth: true
                    Layout.maximumWidth: 760
                    Layout.leftMargin: Theme.s32
                    radius: Theme.rLg
                    color: Theme.surface
                    border.color: Theme.line
                    implicitHeight: metaCol.implicitHeight + Theme.s40

                    ColumnLayout {
                        id: metaCol
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.margins: Theme.s24
                        spacing: Theme.s12

                        Text {
                            text: "Metadata addon"
                            color: Theme.text
                            font.pixelSize: Theme.fTitle
                            font.bold: true
                        }
                        Text {
                            Layout.fillWidth: true
                            text: "Source for detailed metadata: per-episode IMDb/TMDB ratings, episode stills and overviews. Defaults to Cinemeta, which lacks most episode ratings. Point this at a self-hosted AIOMetadata (Stremio addon URL or manifest.json) for full TMDB episode data. Catalogs and search still use Cinemeta."
                            color: Theme.textDim
                            font.pixelSize: Theme.fSmall
                            wrapMode: Text.WordWrap
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: Theme.s8
                            spacing: Theme.s12

                            TextField {
                                id: metaUrlField
                                Layout.fillWidth: true
                                text: appController.metadataUrl
                                placeholderText: "https://your-aiometadata.example/manifest.json (blank = Cinemeta)"
                                color: Theme.text
                                placeholderTextColor: Theme.textMute
                                font.pixelSize: Theme.fBody
                                selectionColor: Theme.accent
                                selectByMouse: true
                                leftPadding: Theme.s16
                                rightPadding: Theme.s16
                                topPadding: Theme.s12
                                bottomPadding: Theme.s12
                                background: Rectangle {
                                    radius: Theme.rMd
                                    color: Theme.surfaceAlt
                                    border.width: 1
                                    border.color: metaUrlField.activeFocus ? Theme.accent : Theme.line
                                    Behavior on border.color { ColorAnimation { duration: Theme.durFast } }
                                }
                            }

                            AppButton {
                                text: appController.metadataUrl.length > 0 ? "Update" : "Save"
                                variant: "primary"
                                onClicked: appController.metadataUrl = metaUrlField.text
                            }

                            AppButton {
                                text: "Remove"
                                variant: "danger"
                                enabled: appController.metadataUrl.length > 0
                                onClicked: {
                                    appController.metadataUrl = ""
                                    metaUrlField.clear()
                                }
                            }
                        }

                        RowLayout {
                            spacing: Theme.s8
                            Rectangle {
                                Layout.preferredWidth: 8; Layout.preferredHeight: 8; radius: 4
                                Layout.alignment: Qt.AlignVCenter
                                color: appController.metadataUrl.length > 0 ? Theme.success : Theme.textMute
                            }
                            Text {
                                text: appController.metadataUrl.length > 0 ? "AIOMetadata configured" : "Using Cinemeta (default)"
                                color: Theme.textDim
                                font.pixelSize: Theme.fSmall
                            }
                        }
                    }
                }

                // Subtitles card
                Rectangle {
                    Layout.fillWidth: true
                    Layout.maximumWidth: 760
                    Layout.leftMargin: Theme.s32
                    radius: Theme.rLg
                    color: Theme.surface
                    border.color: Theme.line
                    implicitHeight: subsCol.implicitHeight + Theme.s40

                    ColumnLayout {
                        id: subsCol
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.margins: Theme.s24
                        spacing: Theme.s12

                        Text {
                            text: "Subtitles"
                            color: Theme.text
                            font.pixelSize: Theme.fTitle
                            font.bold: true
                        }
                        Text {
                            Layout.fillWidth: true
                            text: "Automatic subtitles via the OpenSubtitles addon. The preferred language is fetched for each title and loaded into mpv on playback."
                            color: Theme.textDim
                            font.pixelSize: Theme.fSmall
                            wrapMode: Text.WordWrap
                        }

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
                                delegate: Rectangle {
                                    id: langChip
                                    required property var modelData
                                    width: langLabel.implicitWidth + Theme.s24
                                    height: 34
                                    radius: Theme.rMd
                                    readonly property bool current: modelData.code === appController.subtitleLanguage
                                    color: current ? Theme.accentSoft : (langHover.hovered ? Theme.surfaceHover : Theme.surfaceAlt)
                                    border.width: 1
                                    border.color: current ? Theme.accent : Theme.line
                                    Text {
                                        id: langLabel
                                        anchors.centerIn: parent
                                        text: langChip.modelData.label
                                        color: langChip.current ? Theme.text : Theme.textDim
                                        font.pixelSize: Theme.fSmall
                                        font.bold: langChip.current
                                    }
                                    HoverHandler { id: langHover; cursorShape: Qt.PointingHandCursor }
                                    TapHandler { onTapped: appController.subtitleLanguage = langChip.modelData.code }
                                }
                            }
                        }
                    }
                }

                // IMDb ratings card
                Rectangle {
                    Layout.fillWidth: true
                    Layout.maximumWidth: 760
                    Layout.leftMargin: Theme.s32
                    radius: Theme.rLg
                    color: Theme.surface
                    border.color: Theme.line
                    implicitHeight: imdbCol.implicitHeight + Theme.s40

                    ColumnLayout {
                        id: imdbCol
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.margins: Theme.s24
                        spacing: Theme.s12

                        Text {
                            text: "IMDb ratings"
                            color: Theme.text
                            font.pixelSize: Theme.fTitle
                            font.bold: true
                        }
                        Text {
                            Layout.fillWidth: true
                            text: "Real IMDb ratings for posters and episodes, built from IMDb's public dataset and cached locally. Refreshes automatically about once a week."
                            color: Theme.textDim
                            font.pixelSize: Theme.fSmall
                            wrapMode: Text.WordWrap
                        }

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
                    }
                }

                // Display / zoom card
                Rectangle {
                    Layout.fillWidth: true
                    Layout.maximumWidth: 760
                    Layout.leftMargin: Theme.s32
                    radius: Theme.rLg
                    color: Theme.surface
                    border.color: Theme.line
                    implicitHeight: zoomCol.implicitHeight + Theme.s40

                    ColumnLayout {
                        id: zoomCol
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.margins: Theme.s24
                        spacing: Theme.s12

                        Text {
                            text: "Display"
                            color: Theme.text
                            font.pixelSize: Theme.fTitle
                            font.bold: true
                        }
                        Text {
                            Layout.fillWidth: true
                            text: "Interface zoom, applied on top of your desktop's scaling. Takes effect after restarting the app."
                            color: Theme.textDim
                            font.pixelSize: Theme.fSmall
                            wrapMode: Text.WordWrap
                        }

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
                                delegate: Rectangle {
                                    id: zoomChip
                                    required property var modelData
                                    width: zoomLabel.implicitWidth + Theme.s24
                                    height: 34
                                    radius: Theme.rMd
                                    readonly property bool current: Math.abs(modelData.scale - appController.uiScale) < 0.001
                                    color: current ? Theme.accentSoft : (zoomHover.hovered ? Theme.surfaceHover : Theme.surfaceAlt)
                                    border.width: 1
                                    border.color: current ? Theme.accent : Theme.line
                                    Text {
                                        id: zoomLabel
                                        anchors.centerIn: parent
                                        text: zoomChip.modelData.label
                                        color: zoomChip.current ? Theme.text : Theme.textDim
                                        font.pixelSize: Theme.fSmall
                                        font.bold: zoomChip.current
                                    }
                                    HoverHandler { id: zoomHover; cursorShape: Qt.PointingHandCursor }
                                    TapHandler { onTapped: appController.uiScale = zoomChip.modelData.scale }
                                }
                            }
                        }
                    }
                }

                // playback note
                Rectangle {
                    Layout.fillWidth: true
                    Layout.maximumWidth: 760
                    Layout.leftMargin: Theme.s32
                    radius: Theme.rLg
                    color: Theme.surface
                    border.color: Theme.line
                    implicitHeight: playbackCol.implicitHeight + Theme.s40

                    ColumnLayout {
                        id: playbackCol
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.margins: Theme.s24
                        spacing: Theme.s8

                        Text {
                            text: "Playback"
                            color: Theme.text
                            font.pixelSize: Theme.fTitle
                            font.bold: true
                        }
                        Text {
                            Layout.fillWidth: true
                            text: "External mpv is used for playback. Torrent and magnet streams are ignored; only direct HTTP/HTTPS sources are playable."
                            color: Theme.textDim
                            font.pixelSize: Theme.fSmall
                            wrapMode: Text.WordWrap
                        }
                    }
                }

                // bottom padding so the last card clears the status bar
                Item { Layout.preferredHeight: Theme.s32 }
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
                color: appController.loading ? Theme.gold : Theme.success
            }
            Text {
                text: appController.statusMessage || "Ready"
                color: Theme.textDim
                font.pixelSize: Theme.fSmall
            }
        }
    }
}
