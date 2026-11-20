import QtQuick 2.0
import Sailfish.Silica 1.0
import QtFeedback 5.0
import SailGBA 1.0

// On-screen gamepad. A single MultiPointTouchArea tracks every finger and
// tests it against all buttons, so presses can slide from button to button,
// roll across the D-pad, or cover A and B at once.
Item {
    id: controls

    property bool landscape: false
    // Portrait: y where the area below the game picture begins.
    property real topY: 0
    property int keys: 0
    property int keyboardKeys: 0

    signal menuRequested()

    // Portrait has a whole half screen for the controls, so they get bigger.
    readonly property real unit: (landscape ? height / 720 : width / 600) * settings.controlsScale
    readonly property real margin: landscape ? Theme.paddingMedium : Theme.paddingLarge

    onKeyboardKeysChanged: emulator.setKeys(keys | keyboardKeys)

    ThemeEffect {
        id: pressEffect
        effect: ThemeEffect.PressWeak
    }

    HapticsEffect {
        id: rumbleEffect
        intensity: 0.8
        duration: 120
    }

    Timer {
        // Re-trigger the short vibration while the game holds rumble on.
        interval: 100
        repeat: true
        running: emulator.rumble && settings.hapticsEnabled && emulator.running && !emulator.paused
        triggeredOnStart: true
        onTriggered: rumbleEffect.start()
    }

    function update() {
        var mask = 0
        var points = touchArea.touchPoints
        for (var i = 0; i < points.length; ++i) {
            var p = points[i]
            if (!p.pressed)
                continue
            mask |= dpad.hitKeys(p.x, p.y)
            for (var j = 0; j < buttons.length; ++j) {
                if (buttons[j].hit(p.x, p.y))
                    mask |= buttons[j].key
            }
        }
        if (mask === keys)
            return
        if ((mask & ~keys) && settings.hapticsEnabled)
            pressEffect.play()
        keys = mask
        emulator.setKeys(keys | keyboardKeys)
    }

    function releaseAll() {
        keys = 0
        emulator.setKeys(keyboardKeys)
    }

    readonly property var buttons: [aButton, bButton, lButton, rButton, startButton, selectButton]

    MultiPointTouchArea {
        id: touchArea
        anchors.fill: parent
        mouseEnabled: true
        minimumTouchPoints: 1
        maximumTouchPoints: 10
        touchPoints: [
            TouchPoint { }, TouchPoint { }, TouchPoint { }, TouchPoint { }, TouchPoint { },
            TouchPoint { }, TouchPoint { }, TouchPoint { }, TouchPoint { }, TouchPoint { }
        ]
        onPressed: controls.update()
        onUpdated: controls.update()
        onReleased: controls.update()
        onCanceled: controls.releaseAll()
    }

    Item {
        id: layer
        anchors.fill: parent
        opacity: settings.controlsOpacity

        // Vertical centre of the D-pad / face button row.
        readonly property real padCenterY: controls.landscape
                                           ? controls.height * 0.6
                                           : controls.topY + (controls.height - controls.topY) * 0.47

        GameButton {
            id: lButton
            text: "L"
            key: Emulator.KeyL
            round: false
            width: 180 * controls.unit
            height: 76 * controls.unit
            x: controls.margin
            y: controls.landscape ? controls.margin : controls.topY + controls.margin
            pressed: (controls.keys & key) !== 0
        }

        GameButton {
            id: rButton
            text: "R"
            key: Emulator.KeyR
            round: false
            width: lButton.width
            height: lButton.height
            x: controls.width - width - controls.margin
            y: lButton.y
            pressed: (controls.keys & key) !== 0
        }

        DPad {
            id: dpad
            width: 270 * controls.unit
            height: width
            x: controls.landscape ? controls.margin : controls.margin * 1.5
            y: layer.padCenterY - height / 2
            keys: controls.keys & (Emulator.KeyUp | Emulator.KeyDown | Emulator.KeyLeft | Emulator.KeyRight)
        }

        // A and B sit on a diagonal like on the handheld; the gap between
        // them is small enough that one thumb can press both.
        GameButton {
            id: aButton
            text: "A"
            key: Emulator.KeyA
            width: 118 * controls.unit
            height: width
            hitMargin: width * 0.22
            x: controls.width - width - controls.margin * (controls.landscape ? 1 : 1.5)
            y: layer.padCenterY - height * 0.85
            pressed: (controls.keys & key) !== 0
        }

        GameButton {
            id: bButton
            text: "B"
            key: Emulator.KeyB
            width: aButton.width
            height: width
            hitMargin: aButton.hitMargin
            // Centres 1.25 widths apart: the enlarged hit circles overlap
            // in the middle, where a thumb presses both.
            x: aButton.x - width * 1.0
            y: aButton.y + height * 0.75
            pressed: (controls.keys & key) !== 0
        }

        GameButton {
            id: selectButton
            text: "SELECT"
            key: Emulator.KeySelect
            round: false
            width: 150 * controls.unit
            height: 54 * controls.unit
            x: controls.landscape ? dpad.x + (dpad.width - width) / 2
                                  : controls.width / 2 - width - controls.margin / 2
            y: controls.height - height - controls.margin * (controls.landscape ? 1 : 2.5)
            pressed: (controls.keys & key) !== 0
        }

        GameButton {
            id: startButton
            text: "START"
            key: Emulator.KeyStart
            round: false
            width: selectButton.width
            height: selectButton.height
            x: controls.landscape ? aButton.x + aButton.width - width
                                  : controls.width / 2 + controls.margin / 2
            y: selectButton.y
            pressed: (controls.keys & key) !== 0
        }
    }

    // Menu and fast-forward sit above the touch area, so they take their own
    // touches and never press a game button by accident.
    Row {
        id: toolRow
        spacing: controls.landscape ? 0 : Theme.paddingLarge
        opacity: Math.max(0.6, settings.controlsOpacity)
        x: controls.landscape ? controls.margin : (controls.width - width) / 2
        y: controls.landscape ? lButton.y + lButton.height + controls.margin
                              : lButton.y + (lButton.height - height) / 2

        IconButton {
            icon.source: "image://theme/icon-m-menu"
            onClicked: controls.menuRequested()
        }
        IconButton {
            icon.source: "image://theme/icon-m-forward"
            highlighted: emulator.fastForward || down
            onClicked: emulator.fastForward = !emulator.fastForward
        }
    }
}
