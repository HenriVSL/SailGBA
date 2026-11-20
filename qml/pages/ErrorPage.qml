import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    property string message

    ViewPlaceholder {
        enabled: true
        text: qsTr("Could not start the game")
        hintText: message
    }
}
