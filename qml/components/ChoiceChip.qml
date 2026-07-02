import QtQuick
import StremioLinux

// A small selectable pill used for the language / zoom choices.
Rectangle {
    id: chip

    property string label: ""
    property bool current: false
    signal clicked()

    width: chipLabel.implicitWidth + Theme.s24 + 4
    height: 34
    radius: Theme.rPill
    color: current ? Theme.accentSoft
                   : (chipHover.hovered ? Qt.rgba(1, 1, 1, 0.09) : Qt.rgba(1, 1, 1, 0.045))
    border.width: 1
    border.color: current ? Theme.alpha(Theme.accent, 0.75)
                          : (chipHover.hovered ? Qt.rgba(1, 1, 1, 0.16) : Qt.rgba(1, 1, 1, 0.08))

    scale: chipTap.pressed ? 0.96 : 1.0
    Behavior on scale { NumberAnimation { duration: Theme.durFast; easing.type: Easing.OutQuad } }
    Behavior on color { ColorAnimation { duration: Theme.durFast } }
    Behavior on border.color { ColorAnimation { duration: Theme.durFast } }

    Text {
        id: chipLabel
        anchors.centerIn: parent
        text: chip.label
        color: chip.current ? Theme.text : (chipHover.hovered ? Theme.text : Theme.textDim)
        font.pixelSize: Theme.fSmall
        font.weight: chip.current ? Font.DemiBold : Font.Medium
        Behavior on color { ColorAnimation { duration: Theme.durFast } }
    }

    HoverHandler { id: chipHover; cursorShape: Qt.PointingHandCursor }
    TapHandler { id: chipTap; onTapped: chip.clicked() }
}
