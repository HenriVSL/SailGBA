import QtQuick 2.0
import Sailfish.Silica 1.0
import SailGBA 1.0

// An eight-way directional pad. Directions are decided by the angle from the
// centre, so rolling a thumb around the pad moves smoothly through diagonals.
Item {
    id: pad

    property int keys: 0 // Emulator.KeyUp | ... currently held

    readonly property real arm: width / 3

    function hitKeys(x, y) {
        var cx = pad.x + width / 2, cy = pad.y + height / 2
        var dx = x - cx, dy = y - cy
        var dist = Math.sqrt(dx * dx + dy * dy)
        var r = width / 2
        if (dist > r * 1.3 || dist < r * 0.15)
            return 0
        var angle = Math.atan2(dy, dx) * 180 / Math.PI // 0 = right, 90 = down
        var mask = 0
        if (angle > -67.5 && angle < 67.5) mask |= Emulator.KeyRight
        if (angle > 22.5 && angle < 157.5) mask |= Emulator.KeyDown
        if (angle > 112.5 || angle < -112.5) mask |= Emulator.KeyLeft
        if (angle > -157.5 && angle < -22.5) mask |= Emulator.KeyUp
        return mask
    }

    function within(x, y) {
        var cx = pad.x + width / 2, cy = pad.y + height / 2
        var r = width / 2 * 1.3
        return (x - cx) * (x - cx) + (y - cy) * (y - cy) <= r * r
    }

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: Theme.rgba(Theme.primaryColor, 0.06)
        border.width: Math.max(1, Theme.paddingSmall / 3)
        border.color: Theme.rgba(Theme.primaryColor, 0.2)
    }

    Repeater {
        // up, right, down, left
        model: [
            { key: Emulator.KeyUp,    x: 1, y: 0, rot: 0 },
            { key: Emulator.KeyRight, x: 2, y: 1, rot: 90 },
            { key: Emulator.KeyDown,  x: 1, y: 2, rot: 180 },
            { key: Emulator.KeyLeft,  x: 0, y: 1, rot: 270 }
        ]
        delegate: Rectangle {
            readonly property bool down: (pad.keys & modelData.key) !== 0
            x: modelData.x * pad.arm
            y: modelData.y * pad.arm
            width: pad.arm
            height: pad.arm
            radius: Theme.paddingSmall
            color: down ? Theme.rgba(Theme.highlightBackgroundColor, 0.75) : Theme.rgba(Theme.primaryColor, 0.16)
            border.width: Math.max(2, Theme.paddingSmall / 2)
            border.color: down ? Theme.highlightColor : Theme.rgba(Theme.primaryColor, 0.55)

            // Arrow pointing outwards
            Canvas {
                anchors.centerIn: parent
                width: parent.width * 0.4
                height: width
                rotation: modelData.rot
                property color fill: parent.down ? Theme.highlightColor : Theme.primaryColor
                onFillChanged: requestPaint()
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.reset()
                    ctx.fillStyle = fill
                    ctx.beginPath()
                    ctx.moveTo(width / 2, height * 0.15)
                    ctx.lineTo(width * 0.9, height * 0.8)
                    ctx.lineTo(width * 0.1, height * 0.8)
                    ctx.closePath()
                    ctx.fill()
                }
            }
        }
    }

    Rectangle {
        x: pad.arm
        y: pad.arm
        width: pad.arm
        height: pad.arm
        color: Theme.rgba(Theme.primaryColor, 0.16)
    }
}
