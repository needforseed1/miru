import QtQuick
import StremioLinux

// Bordered badge in the "ColorfulAndConcise" style: dark/transparent fill,
// a colored outline, and a short label.
Rectangle {
    id: root

    property string label: ""
    property color bgColor: "#000000"
    property color outlineColor: "#888888"
    property color textColor: "#FFFFFF"

    implicitWidth: text.implicitWidth + Theme.s12
    implicitHeight: 22
    radius: Theme.rSm
    color: bgColor
    border.width: 1
    border.color: outlineColor

    Text {
        id: text
        anchors.centerIn: parent
        text: root.label
        color: root.textColor
        font.pixelSize: Theme.fTiny
        font.bold: true
    }
}
