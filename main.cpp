#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QIcon>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>
#include <QStandardPaths>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("SCX Scheduler Manager");
    app.setApplicationVersion("1.1.1");
    app.setOrganizationName("lgl-scheduler-manager");

    QCommandLineParser parser;
    parser.setApplicationDescription("Manage sched-ext BPF schedulers via scxctl");
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption startMinimizedOption(
        "start-minimized", "Start with the main window minimized to the system tray.");
    parser.addOption(startMinimizedOption);
    parser.process(app);

    app.setWindowIcon(QIcon(":/packaging/hicolor/256x256/apps/lgl-scheduler-manager.png"));
    app.setQuitOnLastWindowClosed(false);

    // Single instance: the lock file decides who is primary. Both files live in the
    // per-user runtime dir, so different users can each run their own copy.
    const QString base = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation)
                         + "/lgl-scheduler-manager";
    const QString socketName = base + ".sock";

    QLockFile lock(base + ".lock");
    lock.setStaleLockTime(0);  // only a dead owner process makes the lock stale, never its age
    if (!lock.tryLock(0)) {
        // Already running (possibly hidden in the tray) — connecting is the "show yourself" signal.
        QLocalSocket client;
        client.connectToServer(socketName);
        client.waitForConnected(1000);
        return 0;
    }

    MainWindow w;

    QLocalServer server;
    QLocalServer::removeServer(socketName);  // clear a socket left behind by a crashed run
    if (server.listen(socketName)) {
        QObject::connect(&server, &QLocalServer::newConnection, &w, [&server, &w]() {
            while (QLocalSocket *client = server.nextPendingConnection())
                client->deleteLater();
            w.bringToFront();
        });
    } else {
        qWarning("Could not listen on %s: %s", qPrintable(socketName),
                 qPrintable(server.errorString()));
    }

    if (parser.isSet(startMinimizedOption))
        w.showMinimized();
    else
        w.show();
    return app.exec();
}
