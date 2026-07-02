import QtQuick
import StremioLinux

// Rounded-full metadata tag tinted from a single accent color.
Rectangle {
    id: root

    property string text: ""
    property color accentColor: Theme.line
    property bool filled: false

    implicitWidth: label.implicitWidth + Theme.s16 + 4
    implicitHeight: 24
    radius: Theme.rPill
    color: filled ? accentColor : Theme.alpha(accentColor, 0.13)
    border.width: filled ? 0 : 1
    border.color: Theme.alpha(accentColor, 0.38)

    Text {
        id: label
        anchors.centerIn: parent
        text: root.text
        font.pixelSize: Theme.fTiny
        font.weight: Font.DemiBold
        font.letterSpacing: 0.3
        color: root.filled ? "white" : Qt.lighter(root.accentColor, 1.45)
    }
}
