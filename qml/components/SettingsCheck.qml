import QtQuick
import QtQuick.Controls.Basic
import StremioLinux

// A checkbox with the settings label styling.
CheckBox {
    id: control
    contentItem: Text {
        text: control.text
        color: Theme.textDim
        font.pixelSize: Theme.fSmall
        leftPadding: control.indicator.width + Theme.s8
        verticalAlignment: Text.AlignVCenter
    }
}
