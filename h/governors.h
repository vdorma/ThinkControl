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



#ifndef GOVERNORS_H
#define GOVERNORS_H

#include <QObject>
#include "devices.h"
#include "settings.h"

#include <QtDBus/QtDBus>

#ifndef WITHOUTOSD
#include <alsa/asoundlib.h>
#endif


/* Fan Governor classes definitions */

class Governor: public QObject,
        public Fan
{
    Q_OBJECT

public:
    Governor(SensorsArray *, Profile *);

    void profileChanged(Profile *);

public slots:
    void setMode(bool);
    void fanOff(bool);
    void fanFullSpeed(bool);

private:
    bool mode;              // true - preset, false - manual
    int cpuTdTmp;           // temp values for treshold states
    int gpuTdTmp;           // prevent for constant level switching
    int mchTdTmp;
    int prevLevel;

    SensorsArray *snsArray;
    Profile *plPtr;

    void adjustFanSpeed();

private slots:
    void refresh();

signals:
    void showCtrldBox(bool);
    void showModeBoxes(bool);
};

/* Warning Levels Governor classes definitions */

enum WLevelsT {NSEN, NORM, WARN, CRIT};
enum critLvlActionT {NONE, CPROF, SHUTD};
enum shutdownMethodT {SUSP, HBTE, HALT};

struct WLDevice {
    WLevelsT level;

    bool warnNotifyWasSent;
    bool critNotifyWasSent;
};

class Notifications {
public:
    Notifications();
    ~Notifications();

    bool getWarnNotifySend();
    bool getCritNotifySend();
    int getNotifyTreshold();

    void sendWarnNotify(QString);
    void sendCritNotify(QString);
    void sendProfileChanged(QString);
    void sendShutdownPerformed(QString);

    void sendVolumeLvl(int v, bool m);

private:
    QDBusInterface *notify;
    QSettings *settings;

    bool warnNotify;
    bool critNotify;
    bool volumeLvl;
    int notifyTreshold;

    void sendMsg(QString str, bool constant);
    void loadSettings();
    void saveSettings();
    void addInitialSettings();
    bool isSettingsExists();
};

class CritTempActions {
public:
    CritTempActions();
    ~CritTempActions();

    void performSuspend();
    void performHibernate();
    void performShutdow();

private:
    bool actionWasCompleted;
    QDBusInterface *dbus;
};

class WLGovernor: public QObject,
        public Notifications,
        public CritTempActions
{
    Q_OBJECT

public:
    WLGovernor(SensorsArray *, ProfileList *);
    ~WLGovernor();

    WLevelsT cpuGetLevel();
    WLevelsT gpuGetLevel();
    WLevelsT mchGetLevel();
    WLevelsT ichGetLevel();
    WLevelsT apsGetLevel();
    WLevelsT pwrGetLevel();
    WLevelsT pcmciaGetLevel();
    WLevelsT mainBatFirstGetLevel();
    WLevelsT mainBatSecondGetLevel();
    WLevelsT bayBatFirstGetLevel();
    WLevelsT bayBatSecondGetLevel();

    int getWarnLvlDiff();
    critLvlActionT getCritLvlAction();
    shutdownMethodT getShutdownMethod();
    int getFallbackProfile();

    void setWarnLvlDiff(int n);
    void setWarnNotifySend(bool s);
    void setCritNotifySend(bool s);
    void setNotifyTreshold(int n);
    void setCritLvlAction(critLvlActionT action);
    void setShutdownMethod(shutdownMethodT method);
    void setFallbackProfile(int n);

private:    
    SensorsArray *snsArray;
    Settings *sttgs;
    ProfileList *pfls;
    QSettings *settings;

    int warnLvlDiff;
    bool warnNotify;
    bool critNotify;
    int notifyTreshold;
    critLvlActionT critLvlAction;
    shutdownMethodT shutdownMethod;
    int fallbackProfile;

    WLDevice cpu;
    WLDevice gpu;
    WLDevice mch;
    WLDevice ich;
    WLDevice aps;
    WLDevice pwr;
    WLDevice pcmcia;
    WLDevice mainBatFirst;
    WLDevice mainBatSecond;
    WLDevice bayBatFirst;
    WLDevice bayBatSecond;

    WLevelsT checkLevel(QString, WLDevice *, QString, int);
    void checkForCritActions();

    void loadSettings();
    void saveSettings();
    void addInitialSettings();
    bool isSettingsExists();

public slots:
    void updateLevels();

signals:
    void changeProfileTo(int);
};

class BatteryGovernor : public QObject {

    Q_OBJECT

public:
    BatteryGovernor();

    Battery *mainBat;

    bool isModulePresent(void);

public slots:
    void updateBatteriesState();
    void mainBatSetStartChargeTreshold(int n);
    void mainBatSetStopChargeTreshold(int n);

signals:
    void mainBatValuesChanged();
    void mainBatInstalled(bool);
//    void mainBatStateChanged(QString);

private:
    bool presence;
    QString mainBatPrevState;   // avoid repeating

    QSettings *settings;
    void loadSettings();
    void saveSettings();
};

/*
#ifndef WITHOUTHDAPSD

class APSGovernor : public QObject {

    Q_OBJECT

public:
    APSGovernor();

    bool isAlredyStarted(void);

    int getSensitivity(void);
    bool getAdaptive(void);

public slots:
    void start(void);
    void stop(void);
    void pause(void);

    void setSensitivity(int n);
    void setAdaptive(bool s);

private slots:
    readFoundPid();

private:
    int sensitivity;
    bool adaptive;
    bool isRunned;
    int pid;
    QProcess *proc;
};

#endif
*/

#ifndef WITHOUTOSD

class OSD : public Notifications {
public:
    OSD();
    ~OSD();

    void sendMsg(int vl, bool mSt);

private:
    QDBusInterface *osdDbus;

    bool kosdPresent;
    bool isKosdPresent();
    void sendKosd(int n, bool m);
};

class TPVolume : public QObject
{
    Q_OBJECT

public:
    TPVolume();
    ~TPVolume();

    int getCurrentVolumeLvl();

private:
    QTimer *timer;

    snd_mixer_t *handle;
    snd_mixer_elem_t *elem;
    snd_mixer_elem_callback_t elemCallback;
    snd_mixer_selem_id_t *selemId;

    static int showVolumeLvl(snd_mixer_elem_t *e, unsigned int m);

private slots:
    void checkForEvent();

signals:
    void sendMessage(int);
};

//int showVolumeLvl(snd_mixer_elem_t *e, unsigned int m);

#endif


#endif // GOVERNORS_H
