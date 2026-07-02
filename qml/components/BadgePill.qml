import QtQuick
import StremioLinux

// Bordered badge in the "ColorfulAndConcise" style: dark/transparent fill,
// a colored outline, and a short label. Colors come from the badge JSON and
// are preserved verbatim — only the shell is styled here.
Rectangle {
    id: root

    property string label: ""
    property color bgColor: "#000000"
    property color outlineColor: "#888888"
    property color textColor: "#FFFFFF"

    implicitWidth: text.implicitWidth + Theme.s12 + 2
    implicitHeight: 21
    radius: 6
    color: bgColor
    border.width: 1
    border.color: outlineColor

    Text {
        id: text
        anchors.centerIn: parent
        text: root.label
        color: root.textColor
        font.pixelSize: Theme.fTiny
        font.weight: Font.DemiBold
        font.letterSpacing: 0.4
    }
}
