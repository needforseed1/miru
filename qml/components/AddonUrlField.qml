import QtQuick
import QtQuick.Layouts
import StremioLinux

// URL entry row for an addon: text field + Save/Update + Remove, with a
// connection-status dot underneath. Used by the AIOStreams and Metadata cards.
ColumnLayout {
    id: ctrl

    property string value: ""
    property string placeholder: ""
    property string statusOn: ""
    property string statusOff: ""
    signal saveRequested(string text)
    signal removeRequested()

    Layout.fillWidth: true
    Layout.topMargin: Theme.s8
    spacing: Theme.s12

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.s12

        SettingsField {
            id: field
            Layout.fillWidth: true
            text: ctrl.value
            placeholderText: ctrl.placeholder
            onAccepted: ctrl.saveRequested(text)
        }

        AppButton {
            text: ctrl.value.length > 0 ? "Update" : "Save"
            variant: "primary"
            onClicked: ctrl.saveRequested(field.text)
        }

        AppButton {
            text: "Remove"
            variant: "danger"
            enabled: ctrl.value.length > 0
            onClicked: {
                ctrl.removeRequested()
                field.clear()
            }
        }
    }

    RowLayout {
        spacing: Theme.s8
        Rectangle {
            Layout.preferredWidth: 8; Layout.preferredHeight: 8; radius: 4
            Layout.alignment: Qt.AlignVCenter
            color: ctrl.value.length > 0 ? Theme.success : Theme.textMute
        }
        Text {
            text: ctrl.value.length > 0 ? ctrl.statusOn : ctrl.statusOff
            color: Theme.textDim
            font.pixelSize: Theme.fSmall
        }
    }
}
