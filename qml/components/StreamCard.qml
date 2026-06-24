import QtQuick
import QtQuick.Layouts
import StremioLinux

Rectangle {
    id: root

    property var stream: ({})
    signal playRequested()

    implicitHeight: layout.implicitHeight + Theme.s32
    radius: Theme.rLg
    color: hover.hovered ? Theme.surfaceHover : Theme.surface
    border.width: 1
    border.color: hover.hovered ? Theme.accent : Theme.line

    Behavior on color { ColorAnimation { duration: Theme.durFast } }
    Behavior on border.color { ColorAnimation { duration: Theme.durFast } }

    RowLayout {
        id: layout
        anchors.fill: parent
        anchors.margins: Theme.s16
        spacing: Theme.s16

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.s8

            // readable release filename
            Text {
                Layout.fillWidth: true
                text: root.stream.title || root.stream.filename || "Unnamed release"
                color: Theme.text
                font.pixelSize: Theme.fBody
                font.bold: true
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                maximumLineCount: 2
                elide: Text.ElideRight
            }

            // the addon's own formatted description, preserved verbatim
            Text {
                Layout.fillWidth: true
                visible: text.length > 0
                text: root.stream.description || ""
                textFormat: Text.PlainText
                color: Theme.textDim
                font.family: Theme.monoFont
                font.pixelSize: Theme.fSmall
                lineHeight: 1.2
                wrapMode: Text.Wrap
            }

            // badges pinned to the bottom of the card
            Flow {
                Layout.fillWidth: true
                Layout.topMargin: Theme.s4
                spacing: Theme.s8

                Repeater {
                    model: root.stream.badges || []
                    delegate: BadgePill {
                        required property var modelData
                        label: modelData.text
                        bgColor: modelData.bg
                        outlineColor: modelData.border
                        textColor: modelData.fg
                    }
                }

                BadgePill {
                    visible: !!root.stream.size
                    label: root.stream.size || ""
                    bgColor: "transparent"
                    outlineColor: Theme.lineStrong
                    textColor: Theme.textDim
                }

                // LOCAL first, then SEASON PACK
                BadgePill {
                    visible: !!root.stream.local
                    label: "LOCAL"
                    bgColor: Theme.success
                    outlineColor: Theme.success
                    textColor: Theme.bg
                }
                BadgePill {
                    visible: !!root.stream.seasonPack
                    label: "SEASON PACK"
                    bgColor: "transparent"
                    outlineColor: Theme.gold
                    textColor: Theme.gold
                }
            }
        }

        // play affordance
        Rectangle {
            Layout.preferredWidth: 48
            Layout.preferredHeight: 48
            Layout.alignment: Qt.AlignVCenter
            radius: width / 2
            color: hover.hovered ? Theme.accent : Theme.surfaceAlt
            border.width: 1
            border.color: hover.hovered ? Theme.accent : Theme.lineStrong
            Behavior on color { ColorAnimation { duration: Theme.durFast } }
            Text {
                anchors.centerIn: parent
                anchors.horizontalCenterOffset: 2
                text: "▶"
                color: hover.hovered ? "white" : Theme.textDim
                font.pixelSize: 17
            }
        }
    }

    HoverHandler { id: hover; cursorShape: Qt.PointingHandCursor }
    TapHandler { onTapped: root.playRequested() }
}
