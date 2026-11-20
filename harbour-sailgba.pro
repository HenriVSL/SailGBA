# SailGBA: a native Sailfish OS front end for the mGBA emulator core.
#
# The TARGET name must match: qml/harbour-sailgba.qml, harbour-sailgba.desktop,
# icons/*/harbour-sailgba.png and rpm/harbour-sailgba.spec.
#
# mGBA is not in the Sailfish OS repositories, so its unmodified upstream
# sources are vendored under 3rdparty/mgba and compiled straight into the app.

TARGET = harbour-sailgba
VERSION = 0.1.0
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

CONFIG += sailfishapp c++14
# Vendored C sources share base names with each other (core.c, audio.c, ...);
# keep each object next to its source path instead of one flat directory.
CONFIG += object_parallel_to_source
QT += multimedia concurrent

SOURCES += $$files(src/*.cpp, true)
HEADERS += $$files(src/*.h, true)
INCLUDEPATH += src

include(3rdparty/mgba.pri)

DISTFILES += qml/harbour-sailgba.qml \
    qml/cover/*.qml \
    qml/pages/*.qml \
    qml/components/*.qml \
    rpm/harbour-sailgba.changes \
    rpm/harbour-sailgba.spec \
    harbour-sailgba.desktop

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

CONFIG += sailfishapp_i18n
TRANSLATIONS += translations/harbour-sailgba-fi.ts
