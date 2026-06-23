import QtQuick
import StremioLinux

// Lightweight "searching" indicator: three accent dots pulsing in sequence
// with an optional label.
Row {
    id: root

    property string label: "Searching…"

    spacing: Theme.s12

    Row {
        anchors.verticalCenter: parent.verticalCenter
        spacing: 6
        Repeater {
            model: 3
            delegate: Rectangle {
                id: dot
                required property int index
                width: 9
                height: 9
                radius: width / 2
                color: Theme.accentBright
                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    PauseAnimation { duration: dot.index * 160 }
                    NumberAnimation { from: 0.3; to: 1.0; duration: 380; easing.type: Easing.InOutQuad }
                    NumberAnimation { from: 1.0; to: 0.3; duration: 380; easing.type: Easing.InOutQuad }
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
