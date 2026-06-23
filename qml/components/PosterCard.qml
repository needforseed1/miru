import QtQuick
import QtQuick.Effects
import StremioLinux

Item {
    id: root

    property var item: ({})
    signal clicked(var item)

    width: 168
    height: posterHeight + 52   // poster + room for title and year

    readonly property real posterHeight: width * 1.5   // 2:3 poster

    scale: hover.hovered ? 1.04 : 1.0
    z: hover.hovered ? 1 : 0
    Behavior on scale { NumberAnimation { duration: Theme.durFast; easing.type: Easing.OutQuad } }

    // ---- Poster artwork (rounded + drop shadow) ---------------------------
    Item {
        id: posterFrame
        width: parent.width
        height: root.posterHeight

        Rectangle {
            id: placeholder
            anchors.fill: parent
            radius: Theme.rLg
            gradient: Gradient {
                GradientStop { position: 0.0; color: Theme.surfaceAlt }
                GradientStop { position: 1.0; color: Theme.surface }
            }
            border.color: Theme.line
            Text {
                anchors.centerIn: parent
                text: (root.item.name || "?").charAt(0).toUpperCase()
                color: Theme.textMute
                font.pixelSize: 48
                font.bold: true
            }
        }

        Image {
            id: poster
            anchors.fill: parent
            source: root.item.poster || ""
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            cache: true
            visible: false
        }

        MultiEffect {
            anchors.fill: poster
            source: poster
            visible: poster.status === Image.Ready
            maskEnabled: true
            maskSource: posterMask
            shadowEnabled: true
            shadowColor: "#000000"
            shadowBlur: hover.hovered ? 1.0 : 0.6
            shadowVerticalOffset: hover.hovered ? 10 : 5
            shadowOpacity: 0.55
            Behavior on shadowBlur { NumberAnimation { duration: Theme.durMed } }
        }

        Item {
            id: posterMask
            anchors.fill: poster
            layer.enabled: true
            visible: false
            Rectangle {
                anchors.fill: parent
                radius: Theme.rLg
            }
        }

        // legibility gradient behind the rating badge
        Rectangle {
            anchors.fill: parent
            radius: Theme.rLg
            visible: ratingPill.visible
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#88000000" }
                GradientStop { position: 0.3; color: "transparent" }
            }
        }

        // rating badge
        Rectangle {
            id: ratingPill
            visible: !!root.item.imdbRating
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: Theme.s8
            radius: Theme.rSm
            height: 22
            width: ratingRow.width + Theme.s12
            color: "#cc11151f"
            Row {
                id: ratingRow
                anchors.centerIn: parent
                spacing: 3
                Text { text: "★"; color: Theme.gold; font.pixelSize: Theme.fTiny; anchors.verticalCenter: parent.verticalCenter }
                Text {
                    text: root.item.imdbRating || ""
                    color: Theme.text
                    font.pixelSize: Theme.fTiny
                    font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }

        // hover ring
        Rectangle {
            anchors.fill: parent
            radius: Theme.rLg
            color: "transparent"
            border.width: 2
            border.color: Theme.accent
            opacity: hover.hovered ? 1.0 : 0.0
            Behavior on opacity { NumberAnimation { duration: Theme.durFast } }
        }
    }

    // ---- Title -------------------------------------------------------------
    Column {
        anchors.top: posterFrame.bottom
        anchors.topMargin: Theme.s8
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 2

        Text {
            width: parent.width
            text: root.item.name || "Untitled"
            color: hover.hovered ? Theme.accentBright : Theme.text
            font.pixelSize: Theme.fSmall
            font.bold: true
            elide: Text.ElideRight
            maximumLineCount: 1
            Behavior on color { ColorAnimation { duration: Theme.durFast } }
        }
        Text {
            width: parent.width
            visible: !!root.item.releaseInfo
            text: root.item.releaseInfo || ""
            color: Theme.textMute
            font.pixelSize: Theme.fTiny
            elide: Text.ElideRight
        }
    }

    HoverHandler { id: hover }
    TapHandler { onTapped: root.clicked(root.item) }
    Item {
        anchors.fill: parent
        // pointing-hand cursor for the whole card
        HoverHandler { cursorShape: Qt.PointingHandCursor }
    }
}
