import QtQuick
import QtQuick.Controls.Basic
import StremioLinux

// A boolean setting rendered as a modern slide toggle.
CheckBox {
    id: control

    hoverEnabled: true
    implicitHeight: 36
    leftPadding: Theme.s12
    rightPadding: Theme.s12
    spacing: Theme.s12

    indicator: Rectangle {
        x: control.leftPadding
        y: Math.round((control.height - height) / 2)
        width: 36
        height: 20
        radius: height / 2
        color: control.checked ? Theme.accent
                               : (control.hovered ? Qt.rgba(1, 1, 1, 0.14) : Qt.rgba(1, 1, 1, 0.09))
        Behavior on color { ColorAnimation { duration: Theme.durFast } }

        Rectangle {
            id: knob
            width: 14
            height: 14
            radius: width / 2
            y: 3
            x: control.checked ? parent.width - width - 3 : 3
            color: "white"
            opacity: control.checked ? 1.0 : 0.75
            Behavior on x { NumberAnimation { duration: Theme.durFast; easing.type: Easing.OutQuad } }
            Behavior on opacity { NumberAnimation { duration: Theme.durFast } }
        }
    }

    contentItem: Text {
        text: control.text
        color: control.checked ? Theme.text : (control.hovered ? Theme.text : Theme.textDim)
        font.pixelSize: Theme.fSmall
        leftPadding: control.leftPadding + control.indicator.width + control.spacing
        rightPadding: control.rightPadding
        verticalAlignment: Text.AlignVCenter
        Behavior on color { ColorAnimation { duration: Theme.durFast } }
    }

    background: Rectangle {
        radius: Theme.rMd
        color: control.hovered ? Qt.rgba(1, 1, 1, 0.04) : "transparent"
        Behavior on color { ColorAnimation { duration: Theme.durFast } }
    }

    HoverHandler { cursorShape: Qt.PointingHandCursor }
}
