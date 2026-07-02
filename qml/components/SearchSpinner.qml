import QtQuick
import StremioLinux

// Lightweight "searching" indicator: three dots sweeping through the brand
// gradient colors, with an optional label.
Row {
    id: root

    property string label: "Searching…"

    spacing: Theme.s12

    Row {
        anchors.verticalCenter: parent.verticalCenter
        spacing: 6
        Repeater {
            model: [Theme.accent, Theme.accentBright, Theme.accent2]
            delegate: Rectangle {
                id: dot
                required property int index
                required property var modelData
                width: 8
                height: 8
                radius: width / 2
                color: modelData
                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    PauseAnimation { duration: dot.index * 160 }
                    NumberAnimation { from: 0.25; to: 1.0; duration: 380; easing.type: Easing.InOutQuad }
                    NumberAnimation { from: 1.0; to: 0.25; duration: 380; easing.type: Easing.InOutQuad }
                    PauseAnimation { duration: (2 - dot.index) * 160 }
                }
            }
        }
    }

    Text {
        anchors.verticalCenter: parent.verticalCenter
        text: root.label
        color: Theme.textDim
        font.pixelSize: Theme.fBody
    }
}
