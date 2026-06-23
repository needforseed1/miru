import QtQuick
import QtQuick.Controls.Basic
import StremioLinux

TextField {
    id: control

    property bool showClear: control.text.length > 0
    signal cleared()

    color: Theme.text
    placeholderTextColor: Theme.textMute
    font.pixelSize: Theme.fBody
    selectionColor: Theme.accent
    selectByMouse: true
    leftPadding: Theme.s40
    rightPadding: showClear ? Theme.s40 : Theme.s16
    topPadding: Theme.s8
    bottomPadding: Theme.s8

    background: Rectangle {
        radius: Theme.rMd
        color: Theme.surfaceAlt
        border.width: 1
        border.color: control.activeFocus ? Theme.accent : Theme.line
        Behavior on border.color { ColorAnimation { duration: Theme.durFast } }

        // magnifier glyph
        Text {
            anchors.verticalCenter: parent.verticalCenter
            x: Theme.s16
            text: "⌕"
            font.pixelSize: 18
            color: control.activeFocus ? Theme.accent : Theme.textMute
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
