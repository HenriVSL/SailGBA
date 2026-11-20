import QtQuick 2.0
import Sailfish.Silica 1.0
import SailGBA 1.0

CoverBackground {
    id: cover

    readonly property bool playing: emulator.running

    // The game keeps its last frame on the cover while the app is in the background.
    Rectangle {
        anchors.fill: parent
        color: "black"
        visible: cover.playing
    }

    GameScreen {
        id: screen
        visible: cover.playing
        anchors {
            left: parent.left
            right: parent.right
            verticalCenter: parent.verticalCenter
            verticalCenterOffset: -Theme.paddingLarge * 2
        }
        height: width * emulator.frameHeight / emulator.frameWidth
        source: cover.playing ? emulator : null
        smoothScaling: true
    }

    Label {
        visible: cover.playing
        anchors {
            top: screen.bottom
            topMargin: Theme.paddingMedium
            left: parent.left
            right: parent.right
            margins: Theme.paddingMedium
        }
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        maximumLineCount: 2
        truncationMode: TruncationMode.Fade
        font.pixelSize: Theme.fontSizeSmall
        color: Theme.primaryColor
        text: emulator.gameTitle
    }

    Column {
        visible: !cover.playing
        anchors.centerIn: parent
        width: parent.width - 2 * Theme.paddingLarge
        spacing: Theme.paddingLarge

        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            source: "/usr/share/icons/hicolor/172x172/apps/harbour-sailgba.png"
            width: Theme.iconSizeExtraLarge
            height: width
            sourceSize.width: width
            sourceSize.height: height
        }
        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: "SailGBA"
            font.pixelSize: Theme.fontSizeLarge
        }
    }

    CoverActionList {
        enabled: cover.playing
        CoverAction {
            iconSource: emulator.paused ? "image://theme/icon-cover-play" : "image://theme/icon-cover-pause"
            onTriggered: {
                if (emulator.paused) {
                    app.activate()
                } else {
                    emulator.paused = true
                }
            }
        }
    }
}
