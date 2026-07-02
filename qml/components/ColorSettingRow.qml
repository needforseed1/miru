import QtQuick
import QtQuick.Layouts
import StremioLinux

RowLayout {
    id: root

    property string label: ""
    property string value: "#000000"
    signal colorAccepted(string color)

    readonly property string draft: colorField.text.trim()
    readonly property string normalizedDraft: draft.startsWith("#") ? draft : "#" + draft
    readonly property bool validDraft: /^#[0-9a-fA-F]{6}$/.test(normalizedDraft)

    spacing: Theme.s12

    Text {
        Layout.preferredWidth: 132
        text: root.label
        color: Theme.text
        font.pixelSize: Theme.fSmall
        font.weight: Font.DemiBold
        verticalAlignment: Text.AlignVCenter
    }

    Rectangle {
        Layout.preferredWidth: 34
        Layout.preferredHeight: 34
        radius: Theme.rMd
        color: root.validDraft ? root.normalizedDraft : root.value
        border.width: 1
        border.color: Qt.rgba(1, 1, 1, 0.16)
    }

    SettingsField {
        id: colorField
        Layout.preferredWidth: 126
        text: root.value
        placeholderText: "#8460ff"
        maximumLength: 7
        onAccepted: root.applyDraft()
        onEditingFinished: root.applyDraft()
    }

    Text {
        Layout.fillWidth: true
        text: root.validDraft ? "" : "Use #RRGGBB"
        color: Theme.danger
        font.pixelSize: Theme.fSmall
        verticalAlignment: Text.AlignVCenter
    }

    function applyDraft() {
        if (validDraft && normalizedDraft.toLowerCase() !== root.value.toLowerCase())
            root.colorAccepted(normalizedDraft)
    }
}
