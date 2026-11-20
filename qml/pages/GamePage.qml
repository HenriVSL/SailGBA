import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.KeepAlive 1.2
import SailGBA 1.0
import "../components"

Page {
    id: page
    objectName: "gamePage"

    allowedOrientations: settings.orientation === Settings.OrientationPortrait ? Orientation.Portrait
                       : settings.orientation === Settings.OrientationLandscape ? Orientation.LandscapeMask
                       : Orientation.All
    // Swiping back by accident would end the game; leave through the menu.
    backNavigation: errorMessage.length > 0

    property string errorMessage
    showNavigationIndicator: false

    property bool menuOpen: false
    readonly property bool shouldRun: status === PageStatus.Active && !menuOpen && Qt.application.active

    onShouldRunChanged: {
        if (emulator.running)
            emulator.paused = !shouldRun
        if (!shouldRun)
            controls.releaseAll()
    }

    function openMenu() {
        menuOpen = true
        emulator.paused = true
        pageStack.push(Qt.resolvedUrl("GameMenuPage.qml"), { gamePage: page })
    }

    onStatusChanged: {
        if (status === PageStatus.Active)
            menuOpen = false
    }

    // Keep the display on while playing; touch controls alone don't reset the blanking timer.
    DisplayBlanking {
        preventBlanking: emulator.running && !emulator.paused && page.status === PageStatus.Active
    }

    Connections {
        target: emulator
        // Shown in place: the failure usually arrives while this page is
        // still being pushed, when the page stack refuses to replace it.
        onFailed: page.errorMessage = message
    }

    // Also covers the strip Silica leaves beside the camera cutout in landscape.
    Rectangle {
        x: -Screen.topCutout.height
        y: -Screen.topCutout.height
        width: parent.width + 2 * Screen.topCutout.height
        height: parent.height + 2 * Screen.topCutout.height
        color: "black"
    }

    GameScreen {
        id: screen
        source: emulator
        smoothScaling: settings.smoothScaling
        integerScaling: settings.integerScaling
        x: 0
        y: page.isPortrait ? Screen.topCutout.height : 0
        width: parent.width
        height: page.isPortrait ? Math.round(width * emulator.frameHeight / emulator.frameWidth) : parent.height
    }

    TouchControls {
        id: controls
        anchors.fill: parent
        landscape: page.isLandscape
        topY: screen.y + screen.height
        onMenuRequested: page.openMenu()
    }

    Label {
        visible: settings.showFps && emulator.running
        anchors {
            right: parent.right
            rightMargin: Theme.paddingMedium
            top: screen.top
            topMargin: Theme.paddingSmall
        }
        text: emulator.fps.toFixed(1)
        font.pixelSize: Theme.fontSizeExtraSmall
        color: "white"
        style: Text.Outline
        styleColor: "black"
    }

    Label {
        visible: emulator.fastForward
        anchors {
            left: parent.left
            leftMargin: Theme.paddingMedium
            top: screen.top
            topMargin: Theme.paddingSmall
        }
        text: "▶▶"
        font.pixelSize: Theme.fontSizeSmall
        color: "white"
        style: Text.Outline
        styleColor: "black"
    }

    Rectangle {
        anchors.fill: parent
        visible: page.errorMessage.length > 0
        color: Theme.overlayBackgroundColor

        MouseArea { anchors.fill: parent } // keep touches away from the gamepad

        Column {
            anchors.centerIn: parent
            width: parent.width - 2 * Theme.horizontalPageMargin
            spacing: Theme.paddingLarge

            InfoLabel {
                text: qsTr("Could not start the game")
            }
            Label {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                color: Theme.secondaryHighlightColor
                text: page.errorMessage
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Back to library")
                onClicked: pageStack.pop()
            }
        }
    }

    // Hardware keyboard: arrows, Z/X = B/A, A/S = L/R, Enter = Start, Backspace/Shift = Select.
    focus: true
    function keyMask(key) {
        switch (key) {
        case Qt.Key_Up: return Emulator.KeyUp
        case Qt.Key_Down: return Emulator.KeyDown
        case Qt.Key_Left: return Emulator.KeyLeft
        case Qt.Key_Right: return Emulator.KeyRight
        case Qt.Key_X: return Emulator.KeyA
        case Qt.Key_Z: return Emulator.KeyB
        case Qt.Key_A: return Emulator.KeyL
        case Qt.Key_S: return Emulator.KeyR
        case Qt.Key_Return:
        case Qt.Key_Enter: return Emulator.KeyStart
        case Qt.Key_Backspace:
        case Qt.Key_Shift: return Emulator.KeySelect
        }
        return 0
    }
    Keys.onPressed: {
        if (event.key === Qt.Key_Escape) {
            openMenu()
            event.accepted = true
            return
        }
        var m = keyMask(event.key)
        if (m) {
            controls.keyboardKeys |= m
            event.accepted = true
        }
    }
    Keys.onReleased: {
        if (event.isAutoRepeat)
            return
        var m = keyMask(event.key)
        if (m) {
            controls.keyboardKeys &= ~m
            event.accepted = true
        }
    }
}
