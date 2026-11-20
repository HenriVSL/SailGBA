import QtQuick 2.0
import Sailfish.Silica 1.0

// Visual only: TouchControls does the hit testing for every button at once,
// so fingers can slide between buttons and press two at a time.
Item {
    id: button

    property string text
    property int key
    property bool pressed
    property bool round: true
    // How far around the visible shape a touch still counts, in pixels.
    property real hitMargin: Theme.paddingMedium

    Rectangle {
        anchors.fill: parent
        radius: button.round ? width / 2 : height / 2
        color: button.pressed ? Theme.rgba(Theme.highlightBackgroundColor, 0.75)
                              : Theme.rgba(Theme.primaryColor, 0.16)
        border.width: Math.max(2, Theme.paddingSmall / 2)
        border.color: button.pressed ? Theme.highlightColor : Theme.rgba(Theme.primaryColor, 0.55)
        scale: button.pressed ? 0.94 : 1.0
        Behavior on scale { NumberAnimation { duration: 60 } }
    }

    Label {
        anchors.centerIn: parent
        text: button.text
        color: button.pressed ? Theme.highlightColor : Theme.primaryColor
        font.pixelSize: button.round ? button.height * 0.38 : button.height * 0.42
        font.bold: true
    }

    function hit(x, y) {
        var m = hitMargin
        if (round) {
            var cx = button.x + width / 2, cy = button.y + height / 2
            var r = width / 2 + m
            return (x - cx) * (x - cx) + (y - cy) * (y - cy) <= r * r
        }
        return x >= button.x - m && x <= button.x + width + m && y >= button.y - m && y <= button.y + height + m
    }
}
