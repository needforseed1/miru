import QtQuick
import QtQuick.Layouts
import StremioLinux

// A settings card: standard shell with an optional title + description and a
// content slot. Auto-sizes to its content, so callers don't repeat the
// Rectangle/anchor/implicitHeight boilerplate.
Rectangle {
    id: card

    property string title: ""
    property string description: ""
    default property alias content: body.data

    Layout.fillWidth: true
    Layout.maximumWidth: 760
    Layout.leftMargin: Theme.s32
    Layout.preferredHeight: body.implicitHeight + Theme.s24 * 2

    radius: Theme.rLg
    color: Theme.surface
    border.width: 1
    border.color: Theme.line

    ColumnLayout {
        id: body
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: Theme.s24
        spacing: Theme.s12

        Text {
            visible: card.title.length > 0
            text: card.title
            color: Theme.text
            font.pixelSize: Theme.fTitle
            font.bold: true
        }
        Text {
            visible: card.description.length > 0
            Layout.fillWidth: true
            text: card.description
            color: Theme.textDim
            font.pixelSize: Theme.fSmall
            wrapMode: Text.WordWrap
        }
    }
}
