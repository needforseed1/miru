import QtQuick
import QtQuick.Controls.Basic
import StremioLinux

Item {
    id: root

    property var item: ({})
    property bool landscape: false
    property bool removable: !!root.item.key
    signal clicked(var item)
    signal removeRequested(string key)

    width: landscape ? 300 : 190
    height: artworkHeight

    readonly property real artworkHeight: landscape ? width * 9 / 16 : width * 1.42
    readonly property real progress: item.duration > 0 ? Math.max(0, Math.min(1, item.position / item.duration))
                                                         : Math.max(0, Math.min(1, (item.progressPercent || 0) / 100))
    readonly property int minutesLeft: item.duration > item.position ? Math.max(1, Math.round((item.duration - item.position) / 60)) : 0
    readonly property string episodeText: item.type === "series" ? "S" + item.season + " E" + item.episode + " · " : ""
    readonly property string episodeTitleText: item.type === "series" && item.episodeTitle ? item.episodeTitle + " · " : ""
    readonly property string progressText: item.nextUp ? "Next up"
                                                       : item.duration > 0 ? root.minutesLeft + " min left"
                                                                         : Math.round(root.progress * 100) + "% watched"
    readonly property string imageSource: landscape
                                         ? (item.thumbnail || item.episodeThumbnail || item.background || item.poster || "")
                                         : (item.poster || item.thumbnail || item.episodeThumbnail || item.background || "")
    readonly property bool removeVisible: root.removable && (hover.hovered || removeButton.hovered || removeButton.activeFocus)

    Rectangle {
        id: posterFrame
        width: parent.width
        height: root.artworkHeight
        transformOrigin: Item.Center
        scale: hover.hovered ? 1.03 : 1.0
        radius: Theme.rLg
        color: Theme.surface
        border.color: hover.hovered ? Theme.accent : Theme.line
        border.width: 1
        clip: true
        Behavior on scale { NumberAnimation { duration: Theme.durFast; easing.type: Easing.OutQuad } }

        Image {
            anchors.fill: parent
            source: root.imageSource
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
        }

        Text {
            anchors.centerIn: parent
            visible: !root.imageSource
            text: (root.item.name || "?").charAt(0).toUpperCase()
            color: Theme.textMute
            font.pixelSize: 48
            font.bold: true
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: Math.min(parent.height, 92)
            gradient: Gradient {
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 0.55; color: "#bb05070d" }
                GradientStop { position: 1.0; color: "#ee05070d" }
            }
        }

        Button {
            id: removeButton
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: Theme.s8
            width: 30
            height: 30
            text: "X"
            visible: root.removable
            enabled: root.removeVisible
            opacity: root.removeVisible ? 1.0 : 0.0
            onClicked: root.removeRequested(root.item.key)
            Behavior on opacity { NumberAnimation { duration: Theme.durFast } }

            contentItem: Text {
                text: removeButton.text
                color: Theme.text
                font.pixelSize: Theme.fSmall
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                radius: width / 2
                color: removeButton.hovered ? Theme.danger : "#bb0f111b"
                border.color: removeButton.hovered ? Theme.danger : Theme.line
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 2
            color: "transparent"
            Rectangle {
                width: parent.width * root.progress
                height: parent.height
                color: Theme.accentBright
            }
        }

        Column {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: Theme.s12
            anchors.bottomMargin: Theme.s8
            spacing: 3

            Text {
                width: parent.width
                text: root.item.name || "Untitled"
                color: hover.hovered ? Theme.accentBright : Theme.text
                font.pixelSize: Theme.fBody
                font.bold: true
                maximumLineCount: 1
                elide: Text.ElideRight
            }

            Text {
                width: parent.width
                text: root.episodeText + root.episodeTitleText + root.progressText
                color: Theme.textDim
                font.pixelSize: Theme.fSmall
                maximumLineCount: 1
                elide: Text.ElideRight
            }
        }
    }

    HoverHandler { id: hover }
    TapHandler { onTapped: root.clicked(root.item) }
    HoverHandler { cursorShape: Qt.PointingHandCursor }
}
