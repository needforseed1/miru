import QtQuick
import QtQuick.Controls.Basic
import StremioLinux

Button {
    id: control

    // variant: "primary" | "ghost" | "nav" | "danger"
    property string variant: "ghost"
    property bool active: false
    property real radius: variant === "nav" ? Theme.rPill : Theme.rMd

    implicitHeight: 38
    leftPadding: Theme.s16 + 2
    rightPadding: Theme.s16 + 2
    font.pixelSize: Theme.fBody
    font.weight: variant === "primary" || active ? Font.DemiBold : Font.Medium
    opacity: control.enabled ? 1.0 : 0.4

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
        Behavior on color { ColorAnimation { duration: Theme.durFast } }
    }

    background: Rectangle {
        radius: control.radius

        // Primary gets a faint top-lit vertical gradient; other variants are
        // translucent "glass" fills over whatever sits behind them.
        gradient: control.variant === "primary" ? primaryFill : null

        readonly property Gradient primaryFill: Gradient {
            GradientStop {
                position: 0.0
                color: control.down ? Qt.darker(Theme.accent, 1.2)
                                    : (control.hovered ? Qt.lighter(Theme.accent, 1.18)
                                                       : Qt.lighter(Theme.accent, 1.08))
            }
            GradientStop {
                position: 1.0
                color: control.down ? Qt.darker(Theme.accent, 1.35)
                                    : (control.hovered ? Theme.accent
                                                       : Qt.darker(Theme.accent, 1.1))
            }
        }

        color: {
            if (control.variant === "primary")
                return "transparent"   // painted by the gradient
            if (control.variant === "danger")
                return control.hovered ? Theme.alpha(Theme.danger, 0.14)
                                       : Theme.alpha(Theme.danger, 0.05)
            if (control.variant === "nav")
                return control.active ? Qt.rgba(1, 1, 1, 0.09)
                                      : (control.hovered ? Qt.rgba(1, 1, 1, 0.05) : "transparent")
            // ghost
            return control.hovered ? Qt.rgba(1, 1, 1, 0.1) : Qt.rgba(1, 1, 1, 0.05)
        }
        border.width: control.variant === "primary" || control.variant === "nav" ? 0 : 1
        border.color: {
            if (control.variant === "danger")
                return control.hovered ? Theme.alpha(Theme.danger, 0.6)
                                       : Theme.alpha(Theme.danger, 0.3)
            return control.hovered ? Qt.rgba(1, 1, 1, 0.16) : Qt.rgba(1, 1, 1, 0.08)
        }

        Behavior on color { ColorAnimation { duration: Theme.durFast } }
        Behavior on border.color { ColorAnimation { duration: Theme.durFast } }
    }

    HoverHandler { cursorShape: Qt.PointingHandCursor }
}
