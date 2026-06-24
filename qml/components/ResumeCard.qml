import QtQuick
import QtQuick.Controls.Basic
import StremioLinux

Item {
    id: root

    property var item: ({})
    signal clicked(var item)
    signal removeRequested(string key)

    width: 190
    height: 326

    readonly property real posterHeight: width * 1.42
    readonly property real progress: item.duration > 0 ? Math.max(0, Math.min(1, item.position / item.duration)) : 0
    readonly property int minutesLeft: item.duration > item.position ? Math.max(1, Math.round((item.duration - item.position) / 60)) : 0
    readonly property string episodeText: item.type === "series" ? "S" + item.season + " / E" + item.episode + " / " : ""

    scale: hover.hovered ? 1.03 : 1.0
    Behavior on scale { NumberAnimation { duration: Theme.durFast; easing.type: Easing.OutQuad } }

    Rectangle {
        id: posterFrame
        width: parent.width
        height: root.posterHeight
        radius: Theme.rLg
        color: Theme.surface
        border.color: hover.hovered ? Theme.accent : Theme.line
        border.width: 1
        clip: true

        Image {
            anchors.fill: parent
            source: root.item.poster || ""
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
        }

        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#22000000" }
                GradientStop { position: 0.68; color: "transparent" }
                GradientStop { position: 1.0; color: "#cc000000" }
            }
        }

        Text {
            anchors.centerIn: parent
            visible: !root.item.poster
            text: (root.item.name || "?").charAt(0).toUpperCase()
            color: Theme.textMute
            font.pixelSize: 48
            font.bold: true
        }

        Button {
            id: removeButton
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: Theme.s8
            width: 30
            height: 30
            text: "x"
            visible: !!root.item.key
            onClicked: root.removeRequested(root.item.key)

            contentItem: Text {
                text: removeButton.text
                color: Theme.text
                font.pixelSize: Theme.fTitle
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
            height: 5
            color: "#99000000"
            Rectangle {
                width: parent.width * root.progress
                height: parent.height
                color: Theme.accentBright
            }
        }
    }

    Column {
        anchors.top: posterFrame.bottom
        anchors.topMargin: Theme.s8
        anchors.left: parent.left
        anchors.right: parent.right
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
            text: root.episodeText + root.minutesLeft + " min left"
            color: Theme.textMute
            font.pixelSize: Theme.fSmall
            maximumLineCount: 1
            elide: Text.ElideRight
        }
    }

    HoverHandler { id: hover }
    TapHandler { onTapped: root.clicked(root.item) }
    HoverHandler { cursorShape: Qt.PointingHandCursor }
}
