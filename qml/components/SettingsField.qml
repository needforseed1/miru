import QtQuick
import QtQuick.Controls.Basic
import StremioLinux

// A plain styled text field used across the settings cards (URLs, mpv args).
TextField {
    id: control
    color: Theme.text
    placeholderTextColor: Theme.textMute
    font.pixelSize: Theme.fBody
    selectionColor: Theme.alpha(Theme.accent, 0.55)
    selectByMouse: true
    leftPadding: Theme.s16
    rightPadding: Theme.s16
    topPadding: Theme.s12
    bottomPadding: Theme.s12
    background: Rectangle {
        radius: Theme.rMd
        color: control.activeFocus ? Qt.rgba(1, 1, 1, 0.06) : Qt.rgba(1, 1, 1, 0.04)
        border.width: 1
        border.color: control.activeFocus ? Theme.alpha(Theme.accent, 0.8)
                                          : (control.hovered ? Qt.rgba(1, 1, 1, 0.14)
                                                             : Qt.rgba(1, 1, 1, 0.08))
        Behavior on border.color { ColorAnimation { duration: Theme.durFast } }
        Behavior on color { ColorAnimation { duration: Theme.durFast } }
    }
}
