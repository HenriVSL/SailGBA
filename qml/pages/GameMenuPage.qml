import QtQuick 2.0
import Sailfish.Silica 1.0
import SailGBA 1.0

Page {
    id: page

    property Item gamePage
    property int statesRevision: 0

    Connections {
        target: emulator
        onStatesChanged: page.statesRevision++
        onStateSaved: {
            if (ok) {
                Notices.show(qsTr("Saved to slot %1").arg(slot), Notice.Short)
            } else {
                Notices.show(qsTr("Saving failed"), Notice.Short)
            }
        }
        onStateLoaded: {
            if (ok) {
                pageStack.pop()
            } else {
                Notices.show(qsTr("Loading failed"), Notice.Short)
            }
        }
    }

    function quitGame() {
        emulator.stop()
        romLibrary.refresh()
        pageStack.pop(pageStack.find(function(p) { return p.objectName === "libraryPage" }))
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        PullDownMenu {
            MenuItem {
                text: qsTr("Settings")
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
            }
            MenuItem {
                text: qsTr("Restart game")
                onClicked: {
                    emulator.reset()
                    pageStack.pop()
                }
            }
            MenuItem {
                text: qsTr("Quit game")
                onClicked: page.quitGame()
            }
        }

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: emulator.gameTitle
                description: qsTr("Paused")
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Resume")
                preferredWidth: Theme.buttonWidthLarge
                onClicked: pageStack.pop()
            }

            Item { width: 1; height: Theme.paddingLarge }

            TextSwitch {
                text: qsTr("Fast forward")
                description: settings.fastForwardSpeed === 0 ? qsTr("As fast as possible")
                                                              : qsTr("%1× speed").arg(settings.fastForwardSpeed)
                checked: emulator.fastForward
                onClicked: emulator.fastForward = checked
            }

            SectionHeader { text: qsTr("Save states") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryHighlightColor
                text: qsTr("Tap a slot to load it, or an empty slot to save. Press and hold for more.")
            }

            Item { width: 1; height: Theme.paddingMedium }

            Grid {
                id: grid
                x: Theme.horizontalPageMargin
                columns: page.isLandscape ? 3 : 2
                readonly property real cellWidth: (page.width - 2 * Theme.horizontalPageMargin - (columns - 1) * spacing) / columns
                spacing: Theme.paddingMedium
                width: page.width - 2 * x

                Repeater {
                    model: 6
                    delegate: ListItem {
                        id: slotItem
                        readonly property int slot: index + 1
                        readonly property bool exists: page.statesRevision >= 0 && emulator.stateExists(slot)
                        width: grid.cellWidth
                        contentHeight: thumb.height + info.height + Theme.paddingMedium * 2

                        onClicked: {
                            if (exists)
                                emulator.loadState(slot)
                            else
                                emulator.saveState(slot)
                        }

                        menu: ContextMenu {
                            MenuItem {
                                text: qsTr("Save here")
                                onClicked: emulator.saveState(slotItem.slot)
                            }
                            MenuItem {
                                text: qsTr("Load")
                                visible: slotItem.exists
                                onClicked: emulator.loadState(slotItem.slot)
                            }
                            MenuItem {
                                text: qsTr("Delete")
                                visible: slotItem.exists
                                onClicked: slotItem.remorseDelete(function() { emulator.deleteState(slotItem.slot) })
                            }
                        }

                        Rectangle {
                            id: thumb
                            y: Theme.paddingMedium
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: parent.width - 2 * Theme.paddingSmall
                            height: width * 2 / 3
                            color: Theme.rgba(Theme.primaryColor, 0.1)
                            border.color: Theme.rgba(Theme.primaryColor, 0.3)

                            Image {
                                anchors.fill: parent
                                anchors.margins: 1
                                fillMode: Image.PreserveAspectFit
                                smooth: false
                                source: page.statesRevision >= 0 && slotItem.exists ? emulator.stateThumbnail(slotItem.slot) : ""
                                cache: false
                            }
                            Icon {
                                anchors.centerIn: parent
                                visible: !slotItem.exists
                                source: "image://theme/icon-m-add"
                            }
                        }
                        Column {
                            id: info
                            anchors {
                                top: thumb.bottom
                                topMargin: Theme.paddingSmall
                                left: thumb.left
                                right: thumb.right
                            }
                            Label {
                                text: qsTr("Slot %1").arg(slotItem.slot)
                                font.pixelSize: Theme.fontSizeSmall
                                color: slotItem.highlighted ? Theme.highlightColor : Theme.primaryColor
                            }
                            Label {
                                width: parent.width
                                truncationMode: TruncationMode.Fade
                                font.pixelSize: Theme.fontSizeExtraSmall
                                color: Theme.secondaryColor
                                text: slotItem.exists
                                      ? Format.formatDate(new Date(emulator.stateTime(slotItem.slot) + page.statesRevision * 0), Formatter.DurationElapsed)
                                      : qsTr("Empty")
                            }
                        }
                    }
                }
            }

            Item { width: 1; height: Theme.paddingLarge }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Quit game")
                preferredWidth: Theme.buttonWidthMedium
                onClicked: page.quitGame()
            }
        }
        VerticalScrollDecorator { }
    }
}
