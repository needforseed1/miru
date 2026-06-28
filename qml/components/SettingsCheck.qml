import QtQuick
import QtQuick.Controls.Basic
import StremioLinux

// A checkbox with the settings label styling.
CheckBox {
    id: control

    hoverEnabled: true
    implicitHeight: 34
    leftPadding: Theme.s12
    rightPadding: Theme.s12
    spacing: Theme.s8

    indicator: Rectangle {
        x: control.leftPadding
        y: Math.round((control.height - height) / 2)
        width: 18
        height: 18
        radius: 6
        color: control.checked ? Theme.accentSoft : Theme.surfaceAlt
        border.width: 1
        border.color: control.checked ? Theme.accent : (control.hovered ? Theme.lineStrong : Theme.line)

        Text {
            anchors.centerIn: parent
            text: "✓"
            visible: control.checked
            color: Theme.text
            font.pixelSize: 13
            font.bold: true
        }

        Behavior on color { ColorAnimation { duration: Theme.durFast } }
        Behavior on border.color { ColorAnimation { duration: Theme.durFast } }
    }

    contentItem: Text {
        text: control.text
        color: control.checked ? Theme.text : (control.hovered ? Theme.text : Theme.textDim)
        font.pixelSize: Theme.fSmall
        leftPadding: control.leftPadding + control.indicator.width + control.spacing
        rightPadding: control.rightPadding
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        radius: Theme.rMd
        color: control.hovered ? Theme.surfaceHover : "transparent"

        Behavior on color { ColorAnimation { duration: Theme.durFast } }
    }
}
