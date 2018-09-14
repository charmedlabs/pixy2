//
// begin license header
//
// This file is part of Pixy CMUcam5 or "Pixy" for short
//
// All Pixy source code is provided under the terms of the
// GNU General Public License v2 (http://www.gnu.org/licenses/gpl-2.0.html).
// Those wishing to use Pixy source code, software and/or
// technologies under different licensing terms should contact us at
// cmucam@cs.cmu.edu. Such licensing terms are available for
// all portions of the Pixy codebase presented here.
//
// end license header
//

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <vector>
#include "monparameterdb.h"

#define PIXY_PARAMFILE_TAG      "Pixy_parameters"

class QLabel;

namespace Ui {
    class MainWindow;
}

class ChirpMon;
class VideoWidget;
class ConsoleWidget;
class Interpreter;
class Flash;
class ConnectEvent;
class ConfigDialog;
class QSettings;
class QMessageBox;

enum Device {NONE, PIXY, PIXY_DFU};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(int argc, char *argv[], QWidget *parent = 0);
    ~MainWindow();

    friend class VideoWidget;
    friend class ConsoleWidget;

private slots:
    void error(QString message);
    void status(QString message);
    void handleText(QString text, uint flags);
    void handleRunState(int state, QString stat);
    void handleConnected(Device device, bool state);
    void handleActions();
    void handleViews();
    void handleProgs();
    void handleActionScriptlet(QString action, QStringList scriptlet, bool reset);
    void handleView(QString view, uint index, bool current, bool reset);
    void handleProg(QString name, QString desc, uint index, bool reset);
    void handleLoadParams();
    void handleParamChange();
    void handleConfigDialogFinished();
    void handleMouseLoc(int x, int y);
    void interpreterFinished();
    void handleVersion(ushort major, ushort minor, ushort build, QString type, ushort hwMajor, ushort hwMinor, ushort hwBuild);
    void on_actionAbout_triggered();
    void on_actionPlay_Pause_triggered();
    void on_actionDefault_program_triggered();
    void on_actionConfigure_triggered();
    void on_actionHelp_triggered();
    void on_actionSave_Image_triggered();
    void on_actionSave_Pixy_parameters_triggered();
    void on_actionLoad_Pixy_parameters_triggered();
    void on_actionRestore_default_parameters_triggered();
    void on_actionExit_triggered();
    void on_actionRaw_video_triggered();
    void on_actionConsole_triggered();

protected:
     void closeEvent(QCloseEvent *event);

private:
    void connectPixy(bool state);
    void connectPixyDFU(bool state);
    void updateButtons();
    void addAction(const QString &label, const QStringList &command);
    void addView(const QString &view, uint index, bool current);
    void addProg(const QString &name, const QString &desc, uint index);
    void setEnabledActionsViews(bool enable);
    void setEnabledProgs(bool enable);
    void close();
    void parseCommandline(int argc, char *argv[]);
    void program(const QString &file);
    void handleFirmware(ushort major, ushort minor, ushort build, const QString &type, ushort hwMajor, ushort hwMinor, ushort hwBuild);
    void showConsole(bool show);

    bool m_pixyConnected;
    bool m_pixyDFUConnected;
    enum {WAIT_NONE, WAIT_EXITTING, WAIT_SAVING_PARAMS, WAIT_LOADING_PARAMS} m_waiting;
    VideoWidget *m_video;
    ConsoleWidget *m_console;
    Interpreter *m_interpreter;
    ConnectEvent *m_connect;
    Flash *m_flash;
    ConfigDialog *m_configDialog;
    std::vector<QAction *> m_actions;
    std::vector<QAction *> m_views;
    std::vector<QAction *> m_progs;
    Ui::MainWindow *m_ui;

    QString m_firmwareFile;
    QString m_argvFirmwareFile;
    QString m_initScript;
    QString m_pixyflash;
    bool m_versionIncompatibility;
    QSettings *m_settings;
    MonParameterDB m_parameters;
    QMessageBox *m_fwInstructions;
    QMessageBox *m_fwMessage;
    bool m_testCycle;
    bool m_showConsole;
    QLabel *m_statusLeft;
    QLabel *m_statusRight;
};

#endif // MAINWINDOW_H
