import QtQuick
import QtQuick.Layouts
import StremioLinux

Rectangle {
    id: root

    property var stream: ({})
    signal playRequested()

    implicitHeight: layout.implicitHeight + Theme.s32
    radius: Theme.rLg
    color: hover.hovered ? Qt.rgba(1, 1, 1, 0.065) : Qt.rgba(1, 1, 1, 0.032)
    border.width: 1
    border.color: hover.hovered ? Theme.alpha(Theme.accent, 0.65) : Qt.rgba(1, 1, 1, 0.07)

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
                font.weight: Font.DemiBold
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

                // LOCAL leads the badge row
                BadgePill {
                    visible: !!root.stream.local
                    label: "LOCAL"
                    bgColor: "transparent"
                    outlineColor: Theme.success
                    textColor: Theme.success
                }

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
            Layout.preferredWidth: 46
            Layout.preferredHeight: 46
            Layout.alignment: Qt.AlignVCenter
            radius: width / 2
            color: hover.hovered ? Theme.accent : Qt.rgba(1, 1, 1, 0.06)
            border.width: 1
            border.color: hover.hovered ? Qt.lighter(Theme.accent, 1.25) : Qt.rgba(1, 1, 1, 0.14)
            scale: hover.hovered ? 1.06 : 1.0
            Behavior on color { ColorAnimation { duration: Theme.durFast } }
            Behavior on border.color { ColorAnimation { duration: Theme.durFast } }
            Behavior on scale { NumberAnimation { duration: Theme.durFast; easing.type: Easing.OutQuad } }
            Text {
                anchors.centerIn: parent
                anchors.horizontalCenterOffset: 2
                text: "▶"
                color: hover.hovered ? "white" : Theme.textDim
                font.pixelSize: 16
            }
        }
    }

    HoverHandler { id: hover; cursorShape: Qt.PointingHandCursor }
    TapHandler { onTapped: root.playRequested() }
}
