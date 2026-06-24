import QtQuick
import QtQuick.Controls.Basic
import StremioLinux

// A plain styled text field used across the settings cards (URLs, mpv args).
TextField {
    id: control
    color: Theme.text
    placeholderTextColor: Theme.textMute
    font.pixelSize: Theme.fBody
    selectionColor: Theme.accent
    selectByMouse: true
    leftPadding: Theme.s16
    rightPadding: Theme.s16
    topPadding: Theme.s12
    bottomPadding: Theme.s12
    background: Rectangle {
        radius: Theme.rMd
        color: Theme.surfaceAlt
        border.width: 1
        border.color: control.activeFocus ? Theme.accent : Theme.line
        Behavior on border.color { ColorAnimation { duration: Theme.durFast } }
    }
}
