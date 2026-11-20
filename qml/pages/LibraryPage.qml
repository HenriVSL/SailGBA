import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0

Page {
    id: page
    objectName: "libraryPage"

    // A game chosen in the file picker starts once the picker has closed.
    property string pendingRom

    onStatusChanged: {
        if (status !== PageStatus.Active)
            return
        if (pendingRom.length > 0) {
            var rom = pendingRom
            pendingRom = ""
            app.play(rom)
        } else if (!emulator.running) {
            romLibrary.refresh()
        }
    }

    Component {
        id: filePicker
        FilePickerPage {
            title: qsTr("Open game")
            nameFilters: ["*.gba", "*.gb", "*.gbc", "*.agb", "*.sgb"]
            onSelectedContentPropertiesChanged: page.pendingRom = selectedContentProperties.filePath
        }
    }

    SilicaListView {
        id: list
        anchors.fill: parent
        model: romLibrary

        PullDownMenu {
            busy: romLibrary.scanning
            MenuItem {
                text: qsTr("About")
                onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
            }
            MenuItem {
                text: qsTr("Settings")
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
            }
            MenuItem {
                text: qsTr("Rescan folders")
                onClicked: romLibrary.rescan()
            }
            MenuItem {
                text: qsTr("Open file")
                onClicked: pageStack.push(filePicker)
            }
        }

        header: Column {
            width: list.width
            PageHeader {
                title: "SailGBA"
                description: emulator.running ? qsTr("Playing: %1").arg(emulator.gameTitle) : ""
            }
            BackgroundItem {
                visible: emulator.running
                height: Theme.itemSizeMedium
                onClicked: pageStack.push(Qt.resolvedUrl("GamePage.qml"))
                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    x: Theme.horizontalPageMargin
                    text: qsTr("Return to game")
                    color: parent.highlighted ? Theme.highlightColor : Theme.primaryColor
                }
            }
            SearchField {
                width: parent.width
                visible: romLibrary.count > 6 || text.length > 0
                placeholderText: qsTr("Search")
                onTextChanged: romLibrary.filter = text
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
            }
        }

        delegate: ListItem {
            id: item
            contentHeight: Theme.itemSizeExtraLarge
            onClicked: app.play(model.path)

            menu: ContextMenu {
                MenuItem {
                    text: qsTr("Start from the beginning")
                    visible: model.hasResume
                    onClicked: app.play(model.path, false)
                }
                MenuItem {
                    text: qsTr("Remove from list")
                    onClicked: romLibrary.forget(model.path)
                }
            }

            Rectangle {
                id: art
                x: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                height: parent.height - 2 * Theme.paddingMedium
                width: height * 3 / 2
                color: Theme.rgba(Theme.primaryColor, 0.08)
                radius: Theme.paddingSmall

                Image {
                    id: thumbImage
                    anchors.fill: parent
                    fillMode: Image.PreserveAspectFit
                    smooth: false
                    asynchronous: true
                    source: model.thumbnail
                }
                Label {
                    anchors.centerIn: parent
                    visible: thumbImage.status !== Image.Ready
                    text: model.platform
                    font.pixelSize: Theme.fontSizeLarge
                    font.bold: true
                    color: Theme.rgba(Theme.primaryColor, 0.35)
                }
            }

            Column {
                anchors {
                    left: art.right
                    leftMargin: Theme.paddingLarge
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                Label {
                    width: parent.width
                    text: model.name
                    truncationMode: TruncationMode.Fade
                    color: item.highlighted ? Theme.highlightColor : Theme.primaryColor
                }
                Label {
                    width: parent.width
                    truncationMode: TruncationMode.Fade
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    text: model.headerTitle.length > 0 ? model.platform + " · " + model.headerTitle : model.platform
                }
                Label {
                    width: parent.width
                    truncationMode: TruncationMode.Fade
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    text: model.lastPlayed > 0
                          ? (model.hasResume ? qsTr("Resume · played %1") : qsTr("Played %1"))
                                .arg(Format.formatDate(new Date(model.lastPlayed), Formatter.DurationElapsed))
                          : qsTr("Not played yet")
                }
            }
        }

        ViewPlaceholder {
            enabled: romLibrary.count === 0 && !romLibrary.scanning
            text: romLibrary.filter.length > 0 ? qsTr("No matching games") : qsTr("No games found")
            hintText: romLibrary.filter.length > 0 ? ""
                      : qsTr("Copy .gba, .gbc or .gb files to Documents or Downloads, or pull down to open a file.")
        }

        BusyIndicator {
            anchors.centerIn: parent
            size: BusyIndicatorSize.Large
            running: romLibrary.scanning && romLibrary.count === 0
        }

        VerticalScrollDecorator { }
    }
}
