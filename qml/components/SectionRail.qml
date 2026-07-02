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
    readonly property int fadeWidth: 64

    Layout.fillWidth: true
    spacing: Theme.s12

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.s12

        Text {
            text: rail.title
            color: Theme.text
            font.pixelSize: Theme.fH3
            font.weight: Font.Bold
            font.letterSpacing: -0.2
        }
        Text {
            visible: rail.subtitle.length > 0
            text: rail.subtitle
            color: Theme.textMute
            font.pixelSize: Theme.fSmall
            Layout.alignment: Qt.AlignBottom
            bottomPadding: 3
        }
        Item { Layout.fillWidth: true }
    }

    // empty / loading placeholder
    Rectangle {
        visible: !rail.loading && (!rail.model || rail.model.length === 0)
        Layout.fillWidth: true
        Layout.preferredHeight: rail.rowHeight
        radius: Theme.rLg
        color: Qt.rgba(1, 1, 1, 0.015)
        border.color: Qt.rgba(1, 1, 1, 0.06)
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
                    NumberAnimation { from: 0.35; to: 0.75; duration: 700; easing.type: Easing.InOutQuad }
                    NumberAnimation { from: 0.75; to: 0.35; duration: 700; easing.type: Easing.InOutQuad }
                }
            }
        }
    }

    // horizontal row with fade-out edges that signal more content to scroll
    Item {
        visible: !rail.loading && rail.model && rail.model.length > 0
        Layout.fillWidth: true
        Layout.preferredHeight: rail.rowHeight

        ListView {
            id: railList
            anchors.fill: parent
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

        // left fade + chevron — appears once scrolled away from the start
        Rectangle {
            anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
            width: rail.fadeWidth
            opacity: railList.atXBeginning ? 0 : 1
            Behavior on opacity { NumberAnimation { duration: 90; easing.type: Easing.OutQuad } }
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: Theme.bg }
                GradientStop { position: 1.0; color: "transparent" }
            }
            Text {
                anchors.left: parent.left
                anchors.leftMargin: Theme.s4
                anchors.verticalCenter: parent.verticalCenter
                text: "‹"
                color: Theme.textDim
                font.pixelSize: 30
                font.weight: Font.Bold
            }
        }

        // right fade + chevron — appears while more content remains to the right
        Rectangle {
            anchors { right: parent.right; top: parent.top; bottom: parent.bottom }
            width: rail.fadeWidth
            opacity: railList.atXEnd ? 0 : 1
            Behavior on opacity { NumberAnimation { duration: 90; easing.type: Easing.OutQuad } }
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 1.0; color: Theme.bg }
            }
            Text {
                anchors.right: parent.right
                anchors.rightMargin: Theme.s4
                anchors.verticalCenter: parent.verticalCenter
                text: "›"
                color: Theme.textDim
                font.pixelSize: 30
                font.weight: Font.Bold
            }
        }
    }
}
