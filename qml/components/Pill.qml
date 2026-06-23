import QtQuick
import StremioLinux

Rectangle {
    id: root

    property string text: ""
    property color accentColor: Theme.line
    property bool filled: false

    implicitWidth: label.implicitWidth + Theme.s16
    implicitHeight: 24
    radius: Theme.rSm
    color: filled ? accentColor : Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.16)
    border.width: filled ? 0 : 1
    border.color: Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.45)

    Text {
        id: label
        anchors.centerIn: parent
        text: root.text
        font.pixelSize: Theme.fTiny
        font.bold: true
        color: root.filled ? "white" : Qt.lighter(root.accentColor, 1.4)
    }
}
