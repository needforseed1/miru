import QtQuick
import StremioLinux

// A small selectable pill used for the language / zoom choices.
Rectangle {
    id: chip

    property string label: ""
    property bool current: false
    signal clicked()

    width: chipLabel.implicitWidth + Theme.s24
    height: 34
    radius: Theme.rMd
    color: current ? Theme.accentSoft : (chipHover.hovered ? Theme.surfaceHover : Theme.surfaceAlt)
    border.width: 1
    border.color: current ? Theme.accent : Theme.line

    Text {
        id: chipLabel
        anchors.centerIn: parent
        text: chip.label
        color: chip.current ? Theme.text : Theme.textDim
        font.pixelSize: Theme.fSmall
        font.bold: chip.current
    }

    HoverHandler { id: chipHover; cursorShape: Qt.PointingHandCursor }
    TapHandler { onTapped: chip.clicked() }
}
