#pragma once

#include <QMainWindow>
#include <QProcess>
#include <QTimer>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QStatusBar>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QLineEdit>
#include <QTabWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QCheckBox>
#include <QFrame>
#include <QCloseEvent>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Show the window if hidden (e.g. minimised to tray) and bring it to the front
    void bringToFront();

private slots:
    void refreshStatus();
    void refreshSchedulerList();
    void startScheduler();
    void stopScheduler();
    void switchScheduler();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void toggleServiceAutostart();
    void showAbout();
    void clearLog();

private:
    void setupUi();
    void setupMenuBar();
    void setupTray();
    void setupConnections();
    void runScxctl(const QStringList &args, const QString &description = QString());
    void appendLog(const QString &text, const QString &color = QString());
    void updateStatusIndicator(bool running, const QString &schedulerName = QString());

    // scx-tools detection and setup mode
    bool isScxctlInstalled();
    bool isCoprEnabled();
    void buildSetupTab();
    void applySetupMode();
    void applyNormalMode();
    void refreshToolStatus();

    // Core state
    QString     currentOutput;
    QString     pendingOperation;
    bool        isRunning = false;
    QString     currentScheduler;
    QString     currentMode;
    QStringList supportedSchedulers;
    QTimer     *autoRefreshTimer;

    // UI elements
    QWidget    *centralWidget;
    QTabWidget *tabWidget;

    // Status tab
    QLabel      *statusDot;
    QLabel      *statusLabel;
    QLabel      *schedulerNameLabel;
    QLabel      *modeLabel;
    QLabel      *serviceStatusLabel;
    QPushButton *refreshBtn;
    QPushButton *stopBtn;

    // Control tab
    QComboBox   *schedulerCombo;
    QComboBox   *modeCombo;
    QLineEdit   *customFlagsEdit;
    QPushButton *startBtn;
    QPushButton *switchBtn;
    QPushButton *listBtn;
    QCheckBox   *autostartCheck;

    // Log tab
    QTextEdit   *logView;
    QPushButton *clearLogBtn;

    // Tray
    QSystemTrayIcon *trayIcon;
    QMenu           *trayMenu;
    QAction         *trayStartAction;
    QAction         *trayStopAction;
    QAction         *trayStatusAction;
    QAction         *trayQuitAction;

    // Reference tables - stored for resize-on-tab-switch and resizeEvent
    QTableWidget *refTableWidget   = nullptr;
    QTableWidget *flagsTableWidget = nullptr;

    // Setup tab - shown when scxctl is not found on PATH
    QWidget *setupTab = nullptr;

    // Tools status labels on Status tab
    QLabel *coprStatusLabel    = nullptr;
    QLabel *scxToolsStatusLabel = nullptr;

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override {
        QMainWindow::resizeEvent(event);
        if (refTableWidget)   refTableWidget->resizeRowsToContents();
        if (flagsTableWidget) flagsTableWidget->resizeRowsToContents();
    }
};
