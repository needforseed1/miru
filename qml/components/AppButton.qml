import QtQuick
import QtQuick.Controls.Basic
import StremioLinux

Button {
    id: control

    // variant: "primary" | "ghost" | "nav" | "danger"
    property string variant: "ghost"
    property bool active: false
    property real radius: Theme.rMd

    implicitHeight: 38
    leftPadding: Theme.s16
    rightPadding: Theme.s16
    font.pixelSize: Theme.fBody
    font.bold: variant === "primary"
    opacity: control.enabled ? 1.0 : 0.45

    contentItem: Text {
        text: control.text
        font: control.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        color: {
            if (control.variant === "primary")
                return "white"
            if (control.variant === "danger")
                return Theme.danger
            if (control.active)
                return Theme.text
            return control.hovered ? Theme.text : Theme.textDim
        }
    }

    background: Rectangle {
        radius: control.radius
        color: {
            if (control.variant === "primary")
                return control.down ? Qt.darker(Theme.accent, 1.15)
                                     : (control.hovered ? Theme.accentBright : Theme.accent)
            if (control.variant === "danger")
                return control.hovered ? Qt.rgba(Theme.danger.r, Theme.danger.g, Theme.danger.b, 0.15)
                                       : Theme.surfaceAlt
            if (control.variant === "nav")
                return control.active ? Theme.accentSoft
                                      : (control.hovered ? Theme.surfaceHover : "transparent")
            // ghost
            return control.hovered ? Theme.surfaceHover : Theme.surfaceAlt
        }
        border.width: control.variant === "primary" ? 0 : 1
        border.color: {
            if (control.active && control.variant === "nav")
                return Theme.accent
            if (control.variant === "danger")
                return control.hovered ? Theme.danger : Theme.line
            return control.hovered ? Theme.lineStrong : Theme.line
        }

        Behavior on color { ColorAnimation { duration: Theme.durFast } }
        Behavior on border.color { ColorAnimation { duration: Theme.durFast } }
    }
}
