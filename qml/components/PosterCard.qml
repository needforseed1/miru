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

    z: hover.hovered ? 1 : 0

    // ---- Poster artwork (rounded + drop shadow) ---------------------------
    Item {
        id: posterFrame
        width: parent.width
        height: root.posterHeight

        // Zoom only the artwork on hover. The title/year below keep their
        // layout position, so they never shift or get clipped by the rail.
        transformOrigin: Item.Center
        scale: hover.hovered ? 1.05 : 1.0
        Behavior on scale { NumberAnimation { duration: Theme.durMed; easing.type: Easing.OutCubic } }

        // The placeholder, artwork and legibility gradient render into one
        // layer that is rounded by a single mask and given one drop shadow,
        // so every corner matches instead of stacking separately-rounded
        // layers that mismatch at the edges.
        Item {
            id: posterContent
            anchors.fill: parent
            layer.enabled: true
            layer.smooth: true
            layer.effect: MultiEffect {
                maskEnabled: true
                maskSource: posterMask
                shadowEnabled: true
                shadowColor: "#000000"
                shadowBlur: hover.hovered ? 1.0 : 0.6
                shadowVerticalOffset: hover.hovered ? 12 : 6
                shadowOpacity: 0.6
                Behavior on shadowBlur { NumberAnimation { duration: Theme.durMed } }
            }

            Rectangle {
                id: placeholder
                anchors.fill: parent
                gradient: Gradient {
                    GradientStop { position: 0.0; color: Theme.surfaceAlt }
                    GradientStop { position: 1.0; color: Theme.surface }
                }
                Text {
                    anchors.centerIn: parent
                    text: (root.item.name || "?").charAt(0).toUpperCase()
                    color: Theme.textMute
                    font.pixelSize: 48
                    font.weight: Font.Bold
                }
            }

            Image {
                id: poster
                anchors.fill: parent
                source: root.item.poster || ""
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                cache: true
            }

        }

        // rounded shape that masks posterContent
        Rectangle {
            id: posterMask
            anchors.fill: parent
            radius: Theme.rLg
            visible: false
            layer.enabled: true
        }

        // hover ring: crisp accent outline above the artwork layer
        Rectangle {
            anchors.fill: parent
            radius: Theme.rLg
            color: "transparent"
            border.width: 2
            border.color: Theme.alpha(Theme.accentBright, hover.hovered ? 0.85 : 0.0)
            Behavior on border.color { ColorAnimation { duration: Theme.durFast } }
        }

        // rating badge (crisp, above the masked artwork)
        Rectangle {
            id: ratingPill
            visible: appController.showPosterRatings && !!root.item.imdbRating
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: Theme.s8
            radius: Theme.rPill
            height: 22
            width: ratingRow.width + Theme.s12 + 2
            color: "#d9090a11"
            border.width: 1
            border.color: Qt.rgba(1, 1, 1, 0.1)
            Row {
                id: ratingRow
                anchors.centerIn: parent
                spacing: 3
                Text { text: "★"; color: Theme.gold; font.pixelSize: Theme.fTiny; anchors.verticalCenter: parent.verticalCenter }
                Text {
                    text: root.item.imdbRating || ""
                    color: Theme.text
                    font.pixelSize: Theme.fTiny
                    font.weight: Font.DemiBold
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }

    }

    // ---- Title -------------------------------------------------------------
    Column {
        anchors.top: posterFrame.bottom
        anchors.topMargin: Theme.s8 + 2
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 2

        Text {
            width: parent.width
            text: root.item.name || "Untitled"
            color: hover.hovered ? Theme.accentBright : Theme.text
            font.pixelSize: Theme.fSmall
            font.weight: Font.DemiBold
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
