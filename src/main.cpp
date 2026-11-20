#include <QGuiApplication>
#include <QQmlContext>
#include <QQuickView>
#include <QScopedPointer>
#include <QSocketNotifier>
#include <QtQml>

#include <csignal>
#include <sys/socket.h>
#include <unistd.h>

#include <sailfishapp.h>

#include "emulator.h"
#include "gamescreen.h"
#include "romlibrary.h"
#include "settings.h"

namespace {
int signalFds[2];

void onTerminate(int)
{
    char c = 1;
    ssize_t ignored = ::write(signalFds[0], &c, 1);
    (void)ignored;
}

// SIGTERM (system shutdown, being killed from a terminal) quits through the
// event loop, so the battery save and resume state still get written.
void quitOnTerminate(QCoreApplication *app)
{
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, signalFds) != 0)
        return;
    QSocketNotifier *notifier = new QSocketNotifier(signalFds[1], QSocketNotifier::Read, app);
    QObject::connect(notifier, &QSocketNotifier::activated, app, &QCoreApplication::quit);
    struct sigaction action = {};
    action.sa_handler = onTerminate;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    sigaction(SIGTERM, &action, nullptr);
    sigaction(SIGINT, &action, nullptr);
    sigaction(SIGHUP, &action, nullptr);
}
} // namespace

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    // Must match [X-Sailjail] in the .desktop file: together they decide the data path.
    app->setOrganizationName(QStringLiteral("io.github.henrivsl"));
    app->setApplicationName(QStringLiteral("harbour-sailgba"));
    app->setApplicationVersion(QStringLiteral(APP_VERSION));
    quitOnTerminate(app.data());

    qmlRegisterType<GameScreen>("SailGBA", 1, 0, "GameScreen");
    qmlRegisterUncreatableType<Emulator>("SailGBA", 1, 0, "Emulator", QStringLiteral("Use the emulator context property"));
    qmlRegisterUncreatableType<Settings>("SailGBA", 1, 0, "Settings", QStringLiteral("Use the settings context property"));

    Settings settings;
    Emulator emulator(&settings);
    RomLibrary library(&settings);

    QScopedPointer<QQuickView> view(SailfishApp::createView());
    QQmlContext *ctx = view->rootContext();
    ctx->setContextProperty(QStringLiteral("settings"), &settings);
    ctx->setContextProperty(QStringLiteral("emulator"), &emulator);
    ctx->setContextProperty(QStringLiteral("romLibrary"), &library);
    ctx->setContextProperty(QStringLiteral("appVersion"), app->applicationVersion());
    ctx->setContextProperty(QStringLiteral("mgbaVersion"), QStringLiteral("0.10.5"));

    // A ROM path on the command line starts that game straight away.
    const QStringList args = app->arguments();
    ctx->setContextProperty(QStringLiteral("launchRom"),
                            args.size() > 1 && RomLibrary::isRom(args.at(1)) ? args.at(1) : QString());

    view->setSource(SailfishApp::pathToMainQml());
    view->show();

    const int result = app->exec();
    // Stop the core first so the battery save and the resume state are written.
    emulator.stop();
    return result;
}
