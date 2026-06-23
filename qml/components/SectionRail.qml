import QtQuick
import QtQuick.Layouts
import StremioLinux

ColumnLayout {
    id: rail

    property string title: ""
    property string subtitle: ""
    property var model: []
    property bool loading: false
    signal openRequested(var item)

    readonly property int rowHeight: 316

    Layout.fillWidth: true
    spacing: Theme.s12

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.s12

        Rectangle { Layout.preferredWidth: 4; Layout.preferredHeight: 22; radius: 2; color: Theme.accent }

        Text {
            text: rail.title
            color: Theme.text
            font.pixelSize: Theme.fH3
            font.bold: true
        }
        Text {
            visible: rail.subtitle.length > 0
            text: rail.subtitle
            color: Theme.textMute
            font.pixelSize: Theme.fSmall
            Layout.alignment: Qt.AlignBottom
            bottomPadding: 4
        }
        Item { Layout.fillWidth: true }
    }

    // empty / loading placeholder
    Rectangle {
        visible: !rail.loading && (!rail.model || rail.model.length === 0)
        Layout.fillWidth: true
        Layout.preferredHeight: rail.rowHeight
        radius: Theme.rLg
        color: "transparent"
        border.color: Theme.line
        border.width: 1
        Text {
            anchors.centerIn: parent
            text: "Nothing here yet"
            color: Theme.textMute
            font.pixelSize: Theme.fBody
        }
    }

    // skeleton row while loading
    RowLayout {
        visible: rail.loading
        Layout.fillWidth: true
        spacing: Theme.s16
        Repeater {
            model: 6
            delegate: Rectangle {
                Layout.preferredWidth: 168
                Layout.preferredHeight: 168 * 1.5
                radius: Theme.rLg
                color: Theme.surface
                SequentialAnimation on opacity {
                    running: rail.loading
                    loops: Animation.Infinite
                    NumberAnimation { from: 0.4; to: 0.8; duration: 700; easing.type: Easing.InOutQuad }
                    NumberAnimation { from: 0.8; to: 0.4; duration: 700; easing.type: Easing.InOutQuad }
                }
            }
        }
    }

    ListView {
        visible: !rail.loading && rail.model && rail.model.length > 0
        Layout.fillWidth: true
        Layout.preferredHeight: rail.rowHeight
        orientation: ListView.Horizontal
        spacing: Theme.s16
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: rail.model
        delegate: PosterCard {
            item: modelData
            onClicked: clickedItem => rail.openRequested(clickedItem)
        }
    }
}
