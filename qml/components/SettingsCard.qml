import QtQuick
import QtQuick.Layouts
import StremioLinux

// A settings card: glass shell with an optional title + description and a
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

    radius: Theme.rXl
    color: Qt.rgba(1, 1, 1, 0.032)
    border.width: 1
    border.color: Qt.rgba(1, 1, 1, 0.07)

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
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }

            // status dot with a soft halo ring
            Rectangle {
                visible: card.statusState.length > 0
                Layout.preferredWidth: 18
                Layout.preferredHeight: 18
                Layout.alignment: Qt.AlignTop
                radius: 9
                color: Theme.alpha(card.statusColor, 0.15)

                Rectangle {
                    anchors.centerIn: parent
                    width: 8
                    height: 8
                    radius: 4
                    color: card.statusColor
                }
            }
        }

        Text {
            visible: card.description.length > 0
            Layout.fillWidth: true
            text: card.description
            color: Theme.textDim
            font.pixelSize: Theme.fSmall
            lineHeight: 1.25
            wrapMode: Text.WordWrap
        }
    }
}
