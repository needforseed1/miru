import QtQuick
import QtQuick.Controls.Basic
import StremioLinux

// Rounded-full search pill with a magnifier glyph and inline clear button.
TextField {
    id: control

    property bool showClear: control.text.length > 0
    signal cleared()

    color: Theme.text
    placeholderTextColor: Theme.textMute
    font.pixelSize: Theme.fBody
    selectionColor: Theme.alpha(Theme.accent, 0.55)
    selectByMouse: true
    leftPadding: Theme.s40
    rightPadding: showClear ? Theme.s40 : Theme.s16
    topPadding: Theme.s8 + 1
    bottomPadding: Theme.s8 + 1

    background: Rectangle {
        radius: height / 2
        color: control.activeFocus ? Qt.rgba(1, 1, 1, 0.07) : Qt.rgba(1, 1, 1, 0.045)
        border.width: 1
        border.color: control.activeFocus ? Theme.alpha(Theme.accent, 0.8)
                                          : (control.hovered ? Qt.rgba(1, 1, 1, 0.16)
                                                             : Qt.rgba(1, 1, 1, 0.08))
        Behavior on border.color { ColorAnimation { duration: Theme.durFast } }
        Behavior on color { ColorAnimation { duration: Theme.durFast } }

        // soft focus halo
        Rectangle {
            anchors.fill: parent
            anchors.margins: -3
            radius: height / 2
            color: "transparent"
            border.width: 3
            border.color: Theme.alpha(Theme.accent, 0.16)
            visible: control.activeFocus
        }

        // magnifier glyph
        Text {
            anchors.verticalCenter: parent.verticalCenter
            x: Theme.s16
            text: "⌕"
            font.pixelSize: 18
            color: control.activeFocus ? Theme.accentBright : Theme.textMute
            Behavior on color { ColorAnimation { duration: Theme.durFast } }
        }
    }

    // clear button
    Text {
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: Theme.s16
        text: "×"
        font.pixelSize: 18
        color: clearArea.containsMouse ? Theme.text : Theme.textMute
        visible: control.showClear

        MouseArea {
            id: clearArea
            anchors.fill: parent
            anchors.margins: -6
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                control.clear()
                control.cleared()
            }
        }
    }
}
