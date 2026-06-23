import QtQuick
import QtQuick.Layouts
import StremioLinux

ColumnLayout {
    id: root

    property var videos: []
    property int selectedSeason: 1
    property string selectedEpisodeId: ""
    signal episodeSelected(var video)

    spacing: Theme.s16

    // Switch season and immediately select its first episode, so changing
    // season starts a new release search without an extra click.
    function selectSeason(season) {
        selectedSeason = season
        var first = null
        for (var i = 0; i < videos.length; ++i) {
            var v = videos[i]
            if (v.season === season && (first === null || v.episode < first.episode))
                first = v
        }
        if (first)
            episodeSelected(first)
    }

    // distinct, sorted seasons (skip specials / season 0)
    readonly property var seasons: {
        var set = {}
        for (var i = 0; i < videos.length; ++i) {
            var s = videos[i].season
            if (s > 0) set[s] = true
        }
        var list = Object.keys(set).map(function (k) { return parseInt(k) })
        list.sort(function (a, b) { return a - b })
        return list
    }

    readonly property var episodes: {
        var out = []
        for (var i = 0; i < videos.length; ++i)
            if (videos[i].season === selectedSeason) out.push(videos[i])
        out.sort(function (a, b) { return a.episode - b.episode })
        return out
    }

    // season selector
    Flow {
        Layout.fillWidth: true
        spacing: Theme.s8
        visible: root.seasons.length > 1
        Repeater {
            model: root.seasons
            delegate: Rectangle {
                required property var modelData
                width: seasonLabel.implicitWidth + Theme.s24
                height: 34
                radius: Theme.rMd
                readonly property bool current: modelData === root.selectedSeason
                color: current ? Theme.accentSoft : (seasonHover.hovered ? Theme.surfaceHover : Theme.surfaceAlt)
                border.width: 1
                border.color: current ? Theme.accent : Theme.line
                Text {
                    id: seasonLabel
                    anchors.centerIn: parent
                    text: "Season " + parent.modelData
                    color: parent.current ? Theme.text : Theme.textDim
                    font.pixelSize: Theme.fSmall
                    font.bold: parent.current
                }
                HoverHandler { id: seasonHover; cursorShape: Qt.PointingHandCursor }
                TapHandler { onTapped: root.selectSeason(parent.modelData) }
            }
        }
    }

    // episode list
    ColumnLayout {
        Layout.fillWidth: true
        spacing: Theme.s8
        Repeater {
            model: root.episodes
            delegate: Rectangle {
                id: epDelegate
                required property var modelData
                Layout.fillWidth: true
                implicitHeight: Math.max(94, info.implicitHeight + Theme.s24)
                radius: Theme.rMd
                readonly property bool current: modelData.id === root.selectedEpisodeId
                color: current ? Theme.accentSoft : (epHover.hovered ? Theme.surfaceHover : Theme.surface)
                border.width: 1
                border.color: current ? Theme.accent : Theme.line

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.s12
                    spacing: Theme.s12

                    // thumbnail (16:9) with episode-number fallback
                    Rectangle {
                        Layout.preferredWidth: 124
                        Layout.preferredHeight: 70
                        Layout.alignment: Qt.AlignVCenter
                        radius: Theme.rSm
                        clip: true
                        color: Theme.surfaceAlt
                        border.color: Theme.line

                        Text {
                            anchors.centerIn: parent
                            text: "E" + epDelegate.modelData.episode
                            color: Theme.textMute
                            font.pixelSize: Theme.fTitle
                            font.bold: true
                        }
                        Image {
                            anchors.fill: parent
                            source: epDelegate.modelData.thumbnail || ""
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            visible: status === Image.Ready
                        }
                        // play overlay
                        Rectangle {
                            anchors.fill: parent
                            color: "#66000000"
                            visible: epHover.hovered || epDelegate.current
                            Text {
                                anchors.centerIn: parent
                                text: "▶"
                                color: "white"
                                font.pixelSize: 20
                            }
                        }
                    }

                    ColumnLayout {
                        id: info
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 3

                        Text {
                            Layout.fillWidth: true
                            text: epDelegate.modelData.episode + ". " + (epDelegate.modelData.title || ("Episode " + epDelegate.modelData.episode))
                            color: Theme.text
                            font.pixelSize: Theme.fBody
                            font.bold: true
                            elide: Text.ElideRight
                            maximumLineCount: 1
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.s8
                            Text {
                                visible: !!epDelegate.modelData.rating
                                text: "★ " + (epDelegate.modelData.rating || "")
                                color: Theme.gold
                                font.pixelSize: Theme.fTiny
                                font.bold: true
                            }
                            Text {
                                visible: !!epDelegate.modelData.released
                                text: epDelegate.modelData.released ? epDelegate.modelData.released.split("T")[0] : ""
                                color: Theme.accentBright
                                font.pixelSize: Theme.fTiny
                            }
                            Item { Layout.fillWidth: true }
                        }
                        Text {
                            Layout.fillWidth: true
                            visible: !!epDelegate.modelData.overview
                            text: epDelegate.modelData.overview || ""
                            color: Theme.textDim
                            font.pixelSize: Theme.fSmall
                            wrapMode: Text.WordWrap
                            maximumLineCount: 2
                            elide: Text.ElideRight
                        }
                    }
                }

                HoverHandler { id: epHover; cursorShape: Qt.PointingHandCursor }
                TapHandler { onTapped: root.episodeSelected(epDelegate.modelData) }
            }
        }
    }
}
