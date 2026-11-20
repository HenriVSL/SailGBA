import QtQuick 2.0
import Sailfish.Silica 1.0
import "pages"

ApplicationWindow {
    id: app

    initialPage: Component { LibraryPage { } }
    cover: Qt.resolvedUrl("cover/CoverPage.qml")
    allowedOrientations: defaultAllowedOrientations
    _defaultPageOrientations: Orientation.All

    function play(path, resume) {
        if (pageStack.currentPage.objectName === "gamePage")
            pageStack.pop(pageStack.find(function(p) { return p.objectName === "libraryPage" }), PageStackAction.Immediate)
        emulator.paused = false
        emulator.fastForward = false
        emulator.start(path, resume === undefined ? true : resume)
        pageStack.push(Qt.resolvedUrl("pages/GamePage.qml"))
    }

    Component.onCompleted: {
        if (launchRom.length > 0)
            play(launchRom)
    }
}
