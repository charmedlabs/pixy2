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

#include <stdexcept>
#include "debug.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QMetaType>
#include <QSettings>
#include <QUrl>
#include <QDesktopServices>
#include <QSysInfo>
#include "mainwindow.h"
#include "pixymon.h"
#include "videowidget.h"
#include "console.h"
#include "interpreter.h"
#include "chirpmon.h"
#include "dfu.h"
#include "flash.h"
#include "ui_mainwindow.h"
#include "configdialog.h"
#include "dataexport.h"
#include "sleeper.h"
#include "aboutdialog.h"
#include "parameters.h"
#include "paramfile.h"

extern ChirpProc c_grabFrame;

MainWindow::MainWindow(int argc, char *argv[], QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::MainWindow)
{
    QCoreApplication::setOrganizationName(PIXYMON_COMPANY);
    QCoreApplication::setApplicationName(PIXYMON_TITLE);

    qRegisterMetaType<Device>("Device");

    m_ui->setupUi(this);
    setWindowTitle(PIXYMON_TITLE);

    m_interpreter = NULL;
    m_flash = NULL;
    m_pixyConnected = false;
    m_pixyDFUConnected = false;
    m_configDialog = NULL;
    m_fwInstructions = NULL;
    m_fwMessage = NULL;
    m_versionIncompatibility = false;
    m_testCycle = false;
    m_waiting = WAIT_NONE;

    parseCommandline(argc, argv);

    m_settings = new QSettings(QSettings::NativeFormat, QSettings::UserScope, PIXYMON_COMPANY, PIXYMON_TITLE);
    m_console = new ConsoleWidget(this);
    m_video = new VideoWidget(this);
    connect(m_video, SIGNAL(mouseLoc(int,int)), this, SLOT(handleMouseLoc(int, int)));

    m_ui->imageLayout->addWidget(m_video);
    m_ui->imageLayout->addWidget(m_console);

    // hide console
    m_showConsole = m_testCycle;
    m_console->setVisible(m_testCycle);
    m_ui->actionConsole->setChecked(m_testCycle);

    m_ui->toolBar->addAction(m_ui->actionPlay_Pause);

    m_ui->actionDefault_program->setIcon(QIcon(":/icons/icons/home.png"));
    m_ui->toolBar->addAction(m_ui->actionDefault_program);

    m_ui->actionRaw_video->setIcon(QIcon(":/icons/icons/raw.png"));
    m_ui->toolBar->addAction(m_ui->actionRaw_video);

    m_ui->actionConfigure->setIcon(QIcon(":/icons/icons/config.png"));
    m_ui->toolBar->addAction(m_ui->actionConfigure);

    m_ui->menuProgram->setToolTipsVisible(true);

    m_statusLeft = new QLabel;
    m_statusRight = new QLabel;
    // give the status a little of a left margin
    m_ui->statusBar->setContentsMargins(6, 0, 0, 0);
    m_ui->statusBar->addWidget(m_statusLeft);
    m_ui->statusBar->addPermanentWidget(m_statusRight);
    updateButtons();

    m_parameters.add("Pixy start command", PT_STRING, "",
        "The command that is sent to Pixy upon initialization");

    // start looking for devices
    m_connect = new ConnectEvent(this);
    if (m_connect->getConnected()==NONE)
        error("No Pixy devices have been detected.\n");
}

MainWindow::~MainWindow()
{
    if (m_connect)
        delete m_connect;

    DBG("deleting mainWindow");
    // we don't delete any of the widgets because the parent deletes it's children upon deletion

    delete m_settings;
}

void MainWindow::parseCommandline(int argc, char *argv[])
{
    int i;

    // when updating to Qt 5, we can use QCommandLineParser
    for (i=1; i<argc; i++)
    {
        if (!strcmp("-firmware", argv[i]) && i+1<argc)
        {
            i++;
            m_argvFirmwareFile = argv[i];
            m_argvFirmwareFile.remove(QRegExp("[\"']"));
        }
        else if (!strcmp("-initscript", argv[i]) && i+1<argc)
        {
            i++;
            m_initScript = argv[i];
        }
        else if (!strcmp("-tc", argv[i]))
            m_testCycle = true;
        else if (!strcmp("-pf", argv[i]) && i+1<argc)
        {
            i++;
            m_pixyflash = argv[i];
            m_pixyflash.remove(QRegExp("[\"']"));
        }
    }
}

void MainWindow::updateButtons()
{
    uint runstate = 0;

    if (m_interpreter && (runstate=m_interpreter->programRunning()))
    {
        m_ui->actionPlay_Pause->setIcon(QIcon(":/icons/icons/stop.png"));
        m_ui->actionPlay_Pause->setToolTip("Stop program");
    }
    else
    {
        m_ui->actionPlay_Pause->setIcon(QIcon(":/icons/icons/play.png"));
        m_ui->actionPlay_Pause->setToolTip("Run program");
    }

    if (m_pixyDFUConnected && m_pixyConnected) // we're in programming mode
    {
        m_ui->actionPlay_Pause->setEnabled(false);
        m_ui->actionDefault_program->setEnabled(false);
        m_ui->actionRaw_video->setEnabled(false);
        m_ui->actionConfigure->setEnabled(false);
        m_ui->actionLoad_Pixy_parameters->setEnabled(false);
        m_ui->actionSave_Pixy_parameters->setEnabled(false);
        m_ui->actionRestore_default_parameters->setEnabled(false);
        m_ui->actionSave_Image->setEnabled(false);
        setEnabledActionsViews(false);
        setEnabledProgs(false);
    }
    else if (runstate==2) // we're in forced runstate
    {
        m_ui->actionPlay_Pause->setEnabled(false);
        m_ui->actionDefault_program->setEnabled(false);
        m_ui->actionRaw_video->setEnabled(false);
        m_ui->actionConfigure->setEnabled(true);
        m_ui->actionLoad_Pixy_parameters->setEnabled(true);
        m_ui->actionSave_Pixy_parameters->setEnabled(true);
        m_ui->actionRestore_default_parameters->setEnabled(true);
        m_ui->actionSave_Image->setEnabled(true);
        setEnabledActionsViews(false);
        setEnabledProgs(false);
    }
    else if (runstate) // runstate==1
    {
        m_ui->actionPlay_Pause->setEnabled(true);
        m_ui->actionDefault_program->setEnabled(true);
        m_ui->actionRaw_video->setEnabled(true);
        m_ui->actionConfigure->setEnabled(true);
        m_ui->actionLoad_Pixy_parameters->setEnabled(true);
        m_ui->actionSave_Pixy_parameters->setEnabled(true);
        m_ui->actionRestore_default_parameters->setEnabled(true);
        m_ui->actionSave_Image->setEnabled(true);
        setEnabledActionsViews(true);
        setEnabledProgs(true);
    }
    else if (m_pixyConnected) // runstate==0
    {
        m_ui->actionPlay_Pause->setEnabled(true);
        m_ui->actionDefault_program->setEnabled(true);
        m_ui->actionRaw_video->setEnabled(true);
        m_ui->actionConfigure->setEnabled(true);
        m_ui->actionLoad_Pixy_parameters->setEnabled(true);
        m_ui->actionSave_Pixy_parameters->setEnabled(true);
        m_ui->actionRestore_default_parameters->setEnabled(true);
        m_ui->actionSave_Image->setEnabled(true);
        setEnabledActionsViews(true);
        setEnabledProgs(true);
    }
    else // nothing connected
    {
        m_ui->actionPlay_Pause->setEnabled(false);
        m_ui->actionDefault_program->setEnabled(false);
        m_ui->actionRaw_video->setEnabled(false);
        m_ui->actionConfigure->setEnabled(false);
        m_ui->actionLoad_Pixy_parameters->setEnabled(false);
        m_ui->actionSave_Pixy_parameters->setEnabled(false);
        m_ui->actionRestore_default_parameters->setEnabled(false);
        m_ui->actionSave_Image->setEnabled(false);
        setEnabledActionsViews(false);
        setEnabledProgs(false);
    }

    if (m_configDialog)
        m_ui->actionConfigure->setEnabled(false);
}

void MainWindow::close()
{
    QCoreApplication::exit();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // delete interpreter
    if (m_configDialog)
    {
        m_configDialog->close();
        m_configDialog = NULL;
    }
    if (m_interpreter)
    {
        m_waiting = WAIT_EXITTING;
        event->ignore(); // ignore event
        DBG("closing interpreter");
        m_interpreter->close();
    }
}

void MainWindow::handleText(QString text, uint flags)
{
    // only popup important messages, and only if console isn't visible
    if (!m_console->isVisible() && (flags&TEXT_FLAG_PRIORITY_HIGH))
        QMessageBox::information(NULL, PIXYMON_TITLE, text);
}

void MainWindow::handleRunState(int state, QString stat)
{
    status(stat);
    updateButtons();
    if (state==-1 && m_interpreter)
    {
        if (m_configDialog)
        {
            m_configDialog->close();
            m_configDialog = NULL;
        }
        DBG("closing interpreter");
        m_interpreter->close();
    }
}


void MainWindow::connectPixyDFU(bool state)
{
    if (state) // connect
    {
        // show console and print messages for firmware upload
        m_console->clear();
        m_console->print("Pixy firmware programming state detected.\n");
        status("Firmware programming state");
        if (!m_testCycle)
            showConsole(true);
        // close the instuctions dialog if it's open
        if (m_fwInstructions)
            m_fwInstructions->done(0);
        try
        {
            if (!m_pixyDFUConnected)
            {
                Dfu *dfu;
                dfu = new Dfu();
                QString path = QFileInfo(QCoreApplication::applicationFilePath()).absolutePath();
                if (m_pixyflash=="")
                    m_pixyflash = "pixyflash.bin.hdr";
                QString file = QFileInfo(path, m_pixyflash).absoluteFilePath();
                dfu->download(file);
                if (!m_testCycle)
                    m_pixyDFUConnected = true;
                delete dfu;
                Sleeper::msleep(1000);
                m_connect = new ConnectEvent(this);
            }
        }
        catch (std::runtime_error &exception)
        {
            m_pixyDFUConnected = false;
            error(QString(exception.what())+"\n");
        }
    }
    else // disconnect
    {
        error("Pixy DFU has stopped working.\n");
        // if we're disconnected, start the connect thread
        m_connect = new ConnectEvent(this);
        m_pixyDFUConnected = false;
    }

    updateButtons();
}

void MainWindow::connectPixy(bool state)
{
    if (state) // connect
    {
        try
        {
            if (m_pixyDFUConnected) // we're in programming mode
            {
                m_flash = new Flash();
                if (m_argvFirmwareFile!="")
                    program(m_argvFirmwareFile);
                else if (m_firmwareFile!="")
                {
                    program(m_firmwareFile);
                    m_firmwareFile = "";
                }
                else
                {
                    QString dir;
                    QFileDialog fd(this);
                    dir = m_settings->value("fw_dialog").toString();
                    fd.setWindowTitle("Select a Firmware File");
                    fd.setDirectory(QDir(dir));
                    fd.setNameFilter("Firmware (*.hex)");
                    if (fd.exec())
                    {
                        QStringList slist = fd.selectedFiles();
                        if (slist.size()==1 && m_flash)
                        {
                            program(slist.at(0));
                        }
                    }
                    dir = fd.directory().absolutePath();
                    m_settings->setValue("fw_dialog", QVariant(dir));
                }
            }
            else if (m_interpreter==NULL)
            {
                m_console->clear();
                m_console->print("Pixy detected.\n");
                m_interpreter = new Interpreter(m_console, m_video, &m_parameters, m_initScript);

                connect(m_interpreter, SIGNAL(error(QString)), this, SLOT(error(QString)));
                connect(m_interpreter, SIGNAL(textOut(QString,uint)), this, SLOT(handleText(QString,uint)));
                connect(m_interpreter, SIGNAL(runState(int,QString)), this, SLOT(handleRunState(int,QString)));
                connect(m_interpreter, SIGNAL(finished()), this, SLOT(interpreterFinished())); // thread will send finished event when it exits
                connect(m_interpreter, SIGNAL(connected(Device,bool)), this, SLOT(handleConnected(Device,bool)));
                connect(m_interpreter, SIGNAL(actionScriptlet(QString,QStringList,bool)), this, SLOT(handleActionScriptlet(QString,QStringList,bool)));
                connect(m_interpreter, SIGNAL(view(QString,uint,bool,bool)), this, SLOT(handleView(QString,uint,bool,bool)));
                connect(m_interpreter, SIGNAL(prog(QString,QString,uint,bool)), this, SLOT(handleProg(QString,QString,uint,bool)));
                connect(m_interpreter, SIGNAL(paramLoaded()), this, SLOT(handleLoadParams()));
                connect(m_interpreter, SIGNAL(paramChange()), this, SLOT(handleParamChange()));
                connect(m_interpreter, SIGNAL(version(ushort,ushort,ushort,QString,ushort,ushort,ushort)), this, SLOT(handleVersion(ushort,ushort,ushort,QString,ushort,ushort,ushort)));
                m_interpreter->start();
            }
            m_pixyConnected = true;
        }
        catch (std::runtime_error &exception)
        {
            error(QString(exception.what())+"\n");
            m_flash = NULL;
            m_pixyDFUConnected = false;
            m_pixyConnected = false;
        }
    }
    else // disconnect
    {
        error("Pixy has been unplugged or has stopped working.\n");
        if (m_interpreter)
        {
            DBG("closing interpreter");
            m_interpreter->close();
        }
    }

    updateButtons();
}

void MainWindow::setEnabledActionsViews(bool enable)
{
    uint i;

    for (i=0; i<m_actions.size(); i++)
        m_actions[i]->setEnabled(enable);
}

void MainWindow::setEnabledProgs(bool enable)
{
    uint i;

    for (i=0; i<m_progs.size(); i++)
        m_progs[i]->setEnabled(enable);
}

void MainWindow::addAction(const QString &label, const QStringList &command)
{
    QAction *action = new QAction(this);
    action->setText(label);
    action->setProperty("command", QVariant(command));
    m_ui->menuAction->addAction(action);
    m_actions.push_back(action);
    connect(action, SIGNAL(triggered()), this, SLOT(handleActions()));
}

void  MainWindow::addView(const QString &view, uint index, bool current)
{
    QAction *v = new QAction(this);
    v->setText(view);
    v->setProperty("index", QVariant(index));
    v->setCheckable(current);
    v->setChecked(current);
    m_ui->menuView->addAction(v);
    m_views.push_back(v);
    connect(v, SIGNAL(triggered()), this, SLOT(handleViews()));
}

void MainWindow::addProg(const QString &name, const QString &desc, uint index)
{
    QAction *prog = new QAction(this);
    if (index==0)
        prog->setIcon(QIcon(":/icons/icons/red.png"));
    else if (index==1)
        prog->setIcon(QIcon(":/icons/icons/orange.png"));
    else if (index==2)
        prog->setIcon(QIcon(":/icons/icons/yellow.png"));
    else if (index==3)
        prog->setIcon(QIcon(":/icons/icons/green.png"));
    else if (index==4)
        prog->setIcon(QIcon(":/icons/icons/lt_blue.png"));
    else if (index==5)
        prog->setIcon(QIcon(":/icons/icons/blue.png"));
    else if (index==6)
        prog->setIcon(QIcon(":/icons/icons/purple.png"));
    prog->setText(name);
    prog->setToolTip(desc);
    prog->setProperty("index", QVariant(index));
    m_ui->menuProgram->addAction(prog);
    m_progs.push_back(prog);
    connect(prog, SIGNAL(triggered()), this, SLOT(handleProgs()));
}

void MainWindow::error(QString message)
{
    m_console->error(message);
    status(message);
}

void MainWindow::status(QString message)
{
    message.remove('\n');
    m_statusLeft->setText(message);
}

void MainWindow::handleActions()
{
    QAction *action = (QAction *)sender();
    if (m_interpreter)
    {
        QVariant var = action->property("command");
        QStringList list = var.toStringList();
        m_interpreter->execute(list);
    }
}

void MainWindow::handleViews()
{
    uint i;
    QAction *view = (QAction *)sender();
    if (m_interpreter)
    {
        QVariant var = view->property("index");
        uint index = var.toUInt();
        m_interpreter->setView(index);
        for (i=0; i<m_views.size(); i++)
        {
            m_views[i]->setCheckable(index==i);
            m_views[i]->setChecked(index==i);
        }
    }
}

void MainWindow::handleProgs()
{
    QAction *prog = (QAction *)sender();
    if (m_interpreter)
    {
        QVariant var = prog->property("index");
        uint index = var.toUInt();
        m_interpreter->execute("runProg "+QString::number(index));
    }
}


void MainWindow::handleActionScriptlet(QString action, QStringList scriptlet, bool reset)
{
    uint i;

    if (reset)
    {
        for (i=0; i<m_actions.size(); i++)
            m_ui->menuAction->removeAction(m_actions[i]);
        m_actions.clear();
    }
    if (action!="")
        addAction(action, scriptlet);
}

void MainWindow::handleView(QString view, uint index, bool current, bool reset)
{
    uint i;

    if (reset)
    {
        for (i=0; i<m_views.size(); i++)
            m_ui->menuView->removeAction(m_views[i]);
        m_views.clear();
    }

    addView(view, index, current);
}

void MainWindow::handleProg(QString name, QString desc, uint index, bool reset)
{
    uint i;

    if (reset)
    {
        for (i=0; i<m_progs.size(); i++)
            m_ui->menuProgram->removeAction(m_progs[i]);
        m_progs.clear();
    }

    addProg(name, desc, index);
}


// this is called from ConnectEvent and from Interpreter
void MainWindow::handleConnected(Device device, bool state)
{
    if (m_configDialog)
    {
        m_configDialog->close();
        m_configDialog = NULL;
    }

    // reset versionIncompatibility because we've unplugged, so we'll try again
    if (device==NONE && !state && m_versionIncompatibility)
        m_versionIncompatibility = false;

    // if we're incompatible, don't create/connect to device... wait for next event
    if (m_versionIncompatibility)
        return;

    if (m_connect && state)
    {
        delete m_connect;
        m_connect = NULL;
    }

    if (device==PIXY)
        connectPixy(state);
    else if (device==PIXY_DFU)
        connectPixyDFU(state);

}

void MainWindow::handleConfigDialogFinished()
{
    if (m_configDialog)
    {
        m_configDialog = NULL;
        updateButtons();
    }
}

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog *about;
    QString contents;

    contents.sprintf("<b>%s v2 version %d.%d.%d</b><br>", PIXYMON_TITLE, VER_MAJOR, VER_MINOR, VER_BUILD);

    if (m_interpreter)
    {
        QString fwver, fwtype;
        uint16_t *version;
        version = m_interpreter->getVersion();
        fwtype = m_interpreter->getVersionType();
        contents += fwver.sprintf("<b>Pixy2 firmware version %d.%d.%d ", version[0], version[1], version[2]) + fwtype + " build</b> (queried)<br>";
        contents += fwver.sprintf("<b>Pixy2 hardware version %d.%d.%d</b> (queried)<br>", version[3], version[4], version[5]);
      }

    contents +=
            "<br>PixyMon v2 is not compatible with the original "
            "Pixy hardware. "
            "The latest versions of PixyMon and Pixy firmware can be downloaded "
            "<a href=\"http://pixycam.com/pixylatest2\">here</a>.<br><br>"
            "CMUCam5 Pixy and PixyMon are open hardware and open source software "
            "maintained by Charmed Labs and Carnegie Mellon University. "
            "All software and firmware is provided under the GNU "
            "General Public License. Complete source code is available at "
            "<a href=\"http://pixycam.com/pixysource2\">github.com</a>.<br><br>"
            "Please send problems, suggestions, ideas, inquiries and feedback, positive or negative "
            "(although we do love to hear the positive stuff!) "
            "to <a href=\"mailto:support@pixycam.com\">support@pixycam.com</a>. "
            "Additional information can be found on our <a href=\"http://pixycam.com/pixy2\">website</a> "
            "and our <a href=\"http://pixycam.com/pixywiki2\">documentation wiki</a>.<br><br>"
            "Thanks for supporting our little guy!";

    about = new AboutDialog(contents);
    about->setAttribute(Qt::WA_DeleteOnClose);
    about->show();
}


void MainWindow::on_actionPlay_Pause_triggered()
{
    if (m_interpreter)
        m_interpreter->runOrStopProgram();
}

void MainWindow::on_actionDefault_program_triggered()
{
    if (m_interpreter)
        m_interpreter->execute("runProgDefault");
}



void MainWindow::program(const QString &file)
{
    try
    {
        m_console->print("Programming... (" + file + ")\n");
        QApplication::processEvents(); // render message before we continue
        m_flash->program(file);
        m_console->print("done!\nPlease wait a few seconds while Pixy resets...\n");
    }
    catch (std::runtime_error &exception)
    {
        error(QString(exception.what())+"\n");
    }

    // clean up...
    delete m_flash;
    m_flash = NULL;
    m_pixyDFUConnected = false;
    m_pixyConnected = false;

    // start looking for devices again (wait 4 seconds before we start looking though)
    m_connect = new ConnectEvent(this, 4000);

}

void MainWindow::showConsole(bool show)
{
    if (show==m_showConsole)
        return;

    if (show)
    {
        resize(width(), height()+m_console->height()+m_ui->imageLayout->spacing());
        m_console->show();
        m_ui->actionConsole->setChecked(true);
    }
    else
    {
        int h = height();
        m_console->hide();
        // resize -- requires a little coercing to propagate everything
        resize(width(), h-m_console->height()-m_ui->imageLayout->spacing());
        QCoreApplication::processEvents();
        resize(width(), h-m_console->height()-m_ui->imageLayout->spacing());
        m_ui->actionConsole->setChecked(false);
    }
    m_showConsole = show;
}

void MainWindow::on_actionConfigure_triggered()
{
    if (m_interpreter)
    {
        m_configDialog = new ConfigDialog(this, m_interpreter);
        connect(m_configDialog, SIGNAL(finished(int)), this, SLOT(handleConfigDialogFinished()));
        m_configDialog->setAttribute(Qt::WA_DeleteOnClose);
        updateButtons();
    }
}

void MainWindow::on_actionRaw_video_triggered()
{
    if (m_interpreter)
        m_interpreter->execute("runProgName video");
}

void MainWindow::on_actionConsole_triggered()
{
    showConsole(m_ui->actionConsole->isChecked());
}


void MainWindow::interpreterFinished()
{
    DBG("interpreter finished");
    m_interpreter->deleteLater();
    m_interpreter = NULL;
    if (m_waiting==WAIT_EXITTING) // if we're exitting, shut down
    {
        close();
        return;
    }

    m_video->clear();
    m_console->acceptInput(false);
    // if we're disconnected, start the connect thread
    m_connect = new ConnectEvent(this);
    m_pixyConnected = false;
    updateButtons();
}

void MainWindow::handleFirmware(ushort major, ushort minor, ushort build, const QString &type, ushort hwMajor, ushort hwMinor, ushort hwBuild)
{
    // check executable directory for firmware
    int i;
    QStringList filters;

    filters << "*.hex";
    QString path = QFileInfo(QCoreApplication::applicationFilePath()).absolutePath();
    QDir dir(path);
    QStringList files = dir.entryList(filters);
    QString fwFilename, fwType, maxVerStr;
    ushort fwVersion[3], hwVersion[] = {hwMajor, hwMinor, hwBuild};
    qlonglong ver, currVer;

    // find the maximum version in the executable directory
    currVer = ((qlonglong)major<<32) | ((qlonglong)minor<<16) | build;
    for (i=0; i<files.size(); i++)
    {

        try
        {
            Flash::checkHardwareVersion(hwVersion, QFileInfo(path, files[i]).absoluteFilePath(), fwVersion, &fwType);
            ver = ((qlonglong)fwVersion[0]<<32) | ((qlonglong)fwVersion[1]<<16) | fwVersion[2];
            if (ver>currVer && fwType.compare(type, Qt::CaseInsensitive)==0)
            {
                fwFilename = files[i];
                maxVerStr = QString::number(fwVersion[0]) + "." + QString::number(fwVersion[1]) + "." + QString::number(fwVersion[2]) + "-" + fwType;
            }
        }
        catch (std::runtime_error &exception)
        {
            // do nothing
        }
    }

#if 0
    bool xp = false;
#ifdef __WINDOWS__
    xp = QSysInfo::WindowsVersion<=QSysInfo::WV_2003;
#endif
#endif
    // if this dialog is already open, close
    if (m_fwInstructions)
        m_fwInstructions->done(0);
    if (m_fwMessage==NULL)
    {
        if (fwFilename!="") // newer version!
        {
            QString str =
                    "There is a more recent firmware version (" + maxVerStr + ") available.<br>"
                    "Your Pixy is running firmware version " + QString::number(major) + "." + QString::number(minor) + "." + QString::number(build) + "-" + type + ".<br><br>"
                    "Would you like to upload this firmware into your Pixy? <i>Psst, click yes!</i>";
            m_fwMessage = new QMessageBox(QMessageBox::Question, "New firmware available!", str, QMessageBox::Yes|QMessageBox::No);
            m_fwMessage->setDefaultButton(QMessageBox::Yes);
            m_fwMessage->exec();
            if (m_fwMessage->standardButton(m_fwMessage->clickedButton())==QMessageBox::Yes)
            {
                m_firmwareFile = QFileInfo(path, fwFilename).absoluteFilePath();
                m_fwInstructions = new QMessageBox(QMessageBox::Information, "Get your Pixy ready!",
                                                   "Please follow these steps to get your Pixy ready to accept new firmware:<br><br>"
                                                   "1. Unplug your Pixy from USB (do this now).<br>"
                                                   "2. Press and hold down the button on top of Pixy.<br>"
                                                   "3. Plug the USB cable back in while continuing to hold down the button.<br>"
                                                   "4. Release the button after the USB cable is plugged back in.<br>"
                                                   "5. Wait a bit. This window will go away automatically when all is ready<br>"
                                                   "to proceed, but the drivers sometimes take a few minutes to install.");
                //m_fwInstructions->setStandardButtons(QMessageBox::NoButton);
                m_fwInstructions->setModal(false);
                m_fwInstructions->exec();
                m_fwInstructions = NULL;
            }
        }
        else if (m_versionIncompatibility) // otherwise tell them where to find the latest version
        {
            m_fwMessage = new QMessageBox(QMessageBox::Information, "Firmware or hardware incompatibility",
                                          "This Pixy's firmware or hardware is incompatible with this version of PixyMon.<br>"
                                          "The latest version of PixyMon and Pixy firmware can be downloaded "
                                          "<a href=\"http://pixycam.com/pixylatest2\">here</a>.<br><br>"
                                          "(For instructions on how to upload firmware onto your Pixy, go "
                                          "<a href=\"http://pixycam.com/pixyfirmwarehowto2\">here</a>.)");
            m_fwMessage->exec();
        }
        m_fwMessage = NULL;
    }
}

void MainWindow::handleVersion(ushort major, ushort minor, ushort build, QString type, ushort hwMajor, ushort hwMinor, ushort hwBuild)
{
    QString str;

    if (major!=VER_MAJOR || minor>VER_MINOR)
    {
        // Interpreter will automatically exit if there's a version incompatibility, so no need to close interpreter
        m_versionIncompatibility = true;
    }
    handleFirmware(major, minor, build, type, hwMajor, hwMinor, hwBuild);
    //str = str.sprintf("Pixy firmware version %d.%d.%d.\n", major, minor, build);
    //m_console->print(str);
}

void MainWindow::handleMouseLoc(int x, int y)
{
    QString text;
    if (x<0) // invalid coordinates
        m_statusRight->setText(""); // clear text
    else
        m_statusRight->setText(text.sprintf("%d, %d", x, y));
}


void MainWindow::on_actionExit_triggered()
{
    if (m_configDialog)
    {
        m_configDialog->close();
        m_configDialog = NULL;
    }
    if (m_interpreter)
    {
        m_waiting = WAIT_EXITTING;
        DBG("closing interpreter");
        m_interpreter->close();
    }
    else
        close();
}

void MainWindow::on_actionHelp_triggered()
{
    QString fwtype;
    if (m_interpreter)
        fwtype = m_interpreter->getVersionType();

    if (fwtype.contains("LEGO", Qt::CaseInsensitive))
        QDesktopServices::openUrl(QUrl("http://pixycam.com/pixymonhelp_lego2"));
    else
        QDesktopServices::openUrl(QUrl("http://pixycam.com/pixymonhelp2"));
}

void MainWindow::handleLoadParams()
{
    if (m_waiting==WAIT_SAVING_PARAMS)
    {
        m_waiting = WAIT_NONE;
        QString dir;
        QFileDialog fd(this);
        fd.setWindowTitle("Please provide a filename for the parameter file");
        dir = m_parameters.value("Document folder").toString();
        fd.setDirectory(QDir(dir));
        fd.setNameFilter("Parameter file (*.prm)");
        fd.setAcceptMode(QFileDialog::AcceptSave);
        fd.setDefaultSuffix("prm");
        if (fd.exec())
        {
            QStringList flist = fd.selectedFiles();
            if (flist.size()==1)
            {
                ParamFile pf;
                if (pf.open(flist[0], false)>=0)
                {
                    pf.write(PIXY_PARAMFILE_TAG, &m_interpreter->m_pixyParameters);
                    pf.close();
                }
            }
        }
    }
}

void MainWindow::handleParamChange()
{
    if (m_interpreter)
        m_interpreter->loadParams(true);
}

void MainWindow::on_actionSave_Image_triggered()
{
    QString filename;

    if (m_interpreter)
    {
        filename = uniqueFilename(m_parameters.value("Document folder").toString(), "image", "png");
        m_interpreter->saveImage(filename);
    }
}

void MainWindow::on_actionSave_Pixy_parameters_triggered()
{
    if (m_interpreter)
    {
        m_interpreter->loadParams(false);
        m_waiting = WAIT_SAVING_PARAMS;
    }
}


void MainWindow::on_actionLoad_Pixy_parameters_triggered()
{
    if (m_interpreter)
    {
        int res;
        QString dir;
        QFileDialog fd(this);
        fd.setWindowTitle("Please choose a parameter file");
        dir = m_parameters.value("Document folder").toString();
        fd.setDirectory(QDir(dir));
        fd.setNameFilter("Parameter file (*.prm)");
        if (fd.exec())
        {
            QStringList flist = fd.selectedFiles();
            if (flist.size()==1)
            {
                ParamFile pf;
                pf.open(flist[0], true);
                res = pf.read(PIXY_PARAMFILE_TAG, &m_interpreter->m_pixyParameters, true);
                pf.close();

                if (res>=0)
                {
                    m_interpreter->saveParams(); // save parameters back to pixy
                    m_interpreter->execute("close");
                    m_console->print("Parameters have been successfully loaded!  Resetting...\n");
                }
            }
        }
    }
}

void MainWindow::on_actionRestore_default_parameters_triggered()
{
    if (m_interpreter)
    {
        QStringList commands;

        commands << "prm_restore";
        commands << "close";

        m_interpreter->execute(commands);
    }
}

