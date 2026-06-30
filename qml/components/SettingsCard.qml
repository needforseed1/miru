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
    property string statusState: ""
    default property alias content: body.data

    Layout.fillWidth: true
    Layout.maximumWidth: 760
    Layout.leftMargin: Theme.s32
    Layout.preferredHeight: body.implicitHeight + Theme.s24 * 2

    radius: Theme.rLg
    color: Theme.surface
    border.width: 1
    border.color: Theme.line

    readonly property color statusColor: {
        switch (card.statusState) {
        case "good": return Theme.success
        case "warning": return Theme.gold
        case "danger": return Theme.danger
        case "neutral": return Theme.textMute
        default: return "transparent"
        }
    }

    ColumnLayout {
        id: body
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: Theme.s24
        spacing: Theme.s12

        RowLayout {
            visible: card.title.length > 0
            Layout.fillWidth: true
            spacing: Theme.s12

            Text {
                Layout.fillWidth: true
                text: card.title
                color: Theme.text
                font.pixelSize: Theme.fTitle
                font.bold: true
                elide: Text.ElideRight
            }

            Rectangle {
                visible: card.statusState.length > 0
                Layout.preferredWidth: 10
                Layout.preferredHeight: 10
                Layout.alignment: Qt.AlignTop
                radius: 5
                color: card.statusColor
                border.width: 1
                border.color: Qt.rgba(card.statusColor.r, card.statusColor.g, card.statusColor.b, 0.65)
            }
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
