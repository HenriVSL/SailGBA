import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0
import SailGBA 1.0

Page {
    id: page

    Component {
        id: biosPicker
        FilePickerPage {
            title: qsTr("Select GBA BIOS")
            nameFilters: ["*.bin", "*.rom", "*.bios"]
            onSelectedContentPropertiesChanged: {
                var error = settings.importBios(selectedContentProperties.filePath)
                if (error.length > 0)
                    Notices.show(error, Notice.Long)
                else
                    Notices.show(qsTr("BIOS installed"), Notice.Short)
            }
        }
    }

    Component {
        id: folderPicker
        FolderPickerPage {
            dialogTitle: qsTr("Add game folder")
            onSelectedPathChanged: settings.addRomFolder(selectedPath)
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width

            PageHeader { title: qsTr("Settings") }

            SectionHeader { text: qsTr("Sound") }

            TextSwitch {
                text: qsTr("Sound")
                checked: settings.audioEnabled
                onClicked: settings.audioEnabled = checked
            }

            Slider {
                width: parent.width
                enabled: settings.audioEnabled
                label: qsTr("Volume")
                minimumValue: 0
                maximumValue: 100
                stepSize: 5
                value: settings.volume
                valueText: value + " %"
                onReleased: settings.volume = value
            }

            SectionHeader { text: qsTr("Touch controls") }

            TextSwitch {
                text: qsTr("Vibration")
                description: qsTr("Short buzz on button presses, and rumble in games that support it")
                checked: settings.hapticsEnabled
                onClicked: settings.hapticsEnabled = checked
            }

            Slider {
                width: parent.width
                label: qsTr("Button opacity")
                minimumValue: 0.1
                maximumValue: 1.0
                stepSize: 0.05
                value: settings.controlsOpacity
                valueText: Math.round(value * 100) + " %"
                onReleased: settings.controlsOpacity = value
            }

            Slider {
                width: parent.width
                label: qsTr("Button size")
                minimumValue: 0.6
                maximumValue: 1.5
                stepSize: 0.05
                value: settings.controlsScale
                valueText: Math.round(value * 100) + " %"
                onReleased: settings.controlsScale = value
            }

            SectionHeader { text: qsTr("Display") }

            ComboBox {
                label: qsTr("Orientation")
                currentIndex: settings.orientation
                menu: ContextMenu {
                    MenuItem { text: qsTr("Automatic") }
                    MenuItem { text: qsTr("Portrait") }
                    MenuItem { text: qsTr("Landscape") }
                }
                onCurrentIndexChanged: settings.orientation = currentIndex
            }

            TextSwitch {
                text: qsTr("Smooth scaling")
                description: qsTr("Soften pixels instead of keeping them sharp")
                checked: settings.smoothScaling
                onClicked: settings.smoothScaling = checked
            }

            TextSwitch {
                text: qsTr("Integer scaling")
                description: qsTr("Scale only by whole numbers so every pixel has the same size")
                checked: settings.integerScaling
                onClicked: settings.integerScaling = checked
            }

            TextSwitch {
                text: qsTr("Show frame rate")
                checked: settings.showFps
                onClicked: settings.showFps = checked
            }

            SectionHeader { text: qsTr("Emulation") }

            TextSwitch {
                text: qsTr("Resume where you left off")
                description: qsTr("Save the game state automatically when quitting and restore it next time")
                checked: settings.autoSaveState
                onClicked: settings.autoSaveState = checked
            }

            ComboBox {
                label: qsTr("Fast forward speed")
                currentIndex: [2, 3, 4, 8, 0].indexOf(settings.fastForwardSpeed)
                menu: ContextMenu {
                    MenuItem { text: qsTr("2×") }
                    MenuItem { text: qsTr("3×") }
                    MenuItem { text: qsTr("4×") }
                    MenuItem { text: qsTr("8×") }
                    MenuItem { text: qsTr("Unlimited") }
                }
                onCurrentIndexChanged: settings.fastForwardSpeed = [2, 3, 4, 8, 0][currentIndex]
            }

            SectionHeader { text: qsTr("BIOS") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryHighlightColor
                text: qsTr("Games run without a BIOS file using mGBA's built-in replacement. A BIOS dumped from your own Game Boy Advance improves compatibility and shows the original start-up logo. Changes apply when a game is next started.")
            }

            ValueButton {
                label: qsTr("GBA BIOS")
                value: settings.hasBios ? qsTr("Installed") : qsTr("Not installed")
                onClicked: pageStack.push(biosPicker)
            }

            TextSwitch {
                text: qsTr("Use BIOS")
                enabled: settings.hasBios
                checked: settings.useBios
                onClicked: settings.useBios = checked
            }

            TextSwitch {
                text: qsTr("Skip start-up logo")
                enabled: settings.hasBios && settings.useBios
                checked: settings.skipBiosIntro
                onClicked: settings.skipBiosIntro = checked
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: settings.hasBios
                text: qsTr("Remove BIOS")
                onClicked: Remorse.popupAction(page, qsTr("Removing BIOS"), function() { settings.removeBios() })
            }

            SectionHeader { text: qsTr("Game folders") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryHighlightColor
                text: qsTr("These folders and memory cards are searched for games, including subfolders.")
            }

            Repeater {
                model: settings.romFolders
                delegate: ListItem {
                    id: folderItem
                    contentHeight: Theme.itemSizeSmall
                    menu: ContextMenu {
                        MenuItem {
                            text: qsTr("Remove")
                            onClicked: folderItem.remorseDelete(function() { settings.removeRomFolder(modelData) })
                        }
                    }
                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * x
                        truncationMode: TruncationMode.Fade
                        text: modelData.replace(StandardPaths.home, "~")
                        color: folderItem.highlighted ? Theme.highlightColor : Theme.primaryColor
                    }
                }
            }

            BackgroundItem {
                onClicked: pageStack.push(folderPicker)
                Icon {
                    id: addIcon
                    x: Theme.horizontalPageMargin - Theme.paddingSmall
                    anchors.verticalCenter: parent.verticalCenter
                    source: "image://theme/icon-m-add"
                }
                Label {
                    anchors {
                        left: addIcon.right
                        leftMargin: Theme.paddingSmall
                        verticalCenter: parent.verticalCenter
                    }
                    text: qsTr("Add folder")
                    color: parent.highlighted ? Theme.highlightColor : Theme.primaryColor
                }
            }
        }
        VerticalScrollDecorator { }
    }
}
