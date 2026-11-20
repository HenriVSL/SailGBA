import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader { title: qsTr("About") }

            Image {
                anchors.horizontalCenter: parent.horizontalCenter
                source: "/usr/share/icons/hicolor/172x172/apps/harbour-sailgba.png"
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "SailGBA " + appVersion
                font.pixelSize: Theme.fontSizeLarge
                color: Theme.highlightColor
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Game Boy Advance, Game Boy and Game Boy Color emulator for Sailfish OS.")
            }

            SectionHeader { text: qsTr("Emulator core") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                textFormat: Text.StyledText
                linkColor: Theme.highlightColor
                onLinkActivated: Qt.openUrlExternally(link)
                text: qsTr("Emulation is done by mGBA %1 by Vicki Pfau and contributors, licensed under the Mozilla Public License 2.0. <a href=\"https://mgba.io\">mgba.io</a>").arg(mgbaVersion)
            }

            SectionHeader { text: qsTr("License") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                textFormat: Text.StyledText
                linkColor: Theme.highlightColor
                onLinkActivated: Qt.openUrlExternally(link)
                text: qsTr("SailGBA is free software under the Mozilla Public License 2.0. Source code: <a href=\"https://github.com/HenriVSL/SailGBA\">github.com/HenriVSL/SailGBA</a>")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                text: qsTr("No games or BIOS files are included. Play only games you own or homebrew that is free to share.")
            }
        }
        VerticalScrollDecorator { }
    }
}
