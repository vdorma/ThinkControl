/*
    Copyright (C) 2012  vold@sdf.org

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/



#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include <QSettings>
#include <QCloseEvent>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include "governors.h"
#include "settings.h"
#include "dialogs.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    FanPresetDialog fanPstDialog;
    ProfileLineDialog profileLnDialog;
    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;
    QAction *actionExit;
    QPoint windowPosition;
    QDBusConnection dbus;

    Timer *timer;
    SensorsArray *sensorsArray;
    WirelessSwitchers *ws;
    TrackPoint *tp;
    TouchPad *touchpad;
    ProfileList profiles;
    Settings settings;
    MachineInfo mi;

    Governor *gov;
    WLGovernor *wlgov;
//    APSGovernor *apsgov;
    BatteryGovernor *batgov;
    TPVolume *tpvol;

    int currentProfile;
    int warnDiff;
    bool realyClose;                 // true - close program, false - minimize in tray

    void notifyAction(QString, QString, int, int);
    void initTrayIcon();
    void addInitialProfiles();
    void saveProfiles();
    void saveSettings();
    void loadProfiles();

    void initProfiles();
    void initMachineInfo();
    void initCpuPolicies();
    void initGpuStates();
//    void initGpuMethods();
    void initGpuProfiles();
//    void initAPS();
    void initMainBatInfo();
    void mainBatUpdateVolValues();

    void setWirelessStates();

    void closeEvent(QCloseEvent *);

private slots:
    void refreshValues();
    void programCtrlActivated(bool);
    void presetCtrlActivated();
    void manualCtrlActivated();
    void speedLvlDial(int);
    void fanPresetBtnPressed();
    void cpuPolicyChoosed(int);
//    void gpuMethodChoosed(int);
    void gpuProfileChoosed(int);
    void profileChoosed(int);
    void profileAddBtnPressed();
    void addProfileAction(QString);
    void deleteProfileAction();
    void setCpuPolicy(int n);
    void setGpuPolicy(int n);

//    void apsStartBtnPressed();
//    void apsStopBtnPressed();

    void wanBtnSwitched(bool);
    void uwbBtnSwitched(bool);
    void btBtnSwitched(bool);

    void settingsDialogBtnPressed();

    void trackpointEnabledChecked();
    void trackpointDialogBtnPressed();

    void touchpadEnabledChecked();
    void touchpadDialogBtnPressed();

    void mainBatInstalled(bool s);
    void mainBatWidgetsState(bool s);
    void mainBatRefreshValues();
//    void mainBatUpdateState(QString s);

    void trayIconActivated(QSystemTrayIcon::ActivationReason);
    void trayMenuAction(QAction *);

    void applyAfterSusped();

    QString getColor(WLevelsT lvl);
};

QString minToHrsAndMin(int m);

#endif // MAINWINDOW_H
