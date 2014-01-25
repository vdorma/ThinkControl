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



#include "h/governors.h"

Governor::Governor(SensorsArray *sa, Profile *p)
{
    snsArray = sa;
    plPtr = p;

    mode = false;
    cpuTdTmp = 0;
    gpuTdTmp = 0;
    mchTdTmp = 0;
}

void Governor::profileChanged(Profile *p)
{
    plPtr = p;
}

void Governor::setMode(bool st)
{
    mode = st;
}

void Governor::fanOff(bool s)
{
    if (s == true) {
        prevLevel = getLevel().toInt();     // fix-it: avoid situations when getLevel return text
        this->setFanOff();
    } else
        this->setLevel(prevLevel);
}

void Governor::fanFullSpeed(bool s)
{
    if (s == true) {
        prevLevel = getLevel().toInt();
        this->setFullSpeed();
    } else
        this->setLevel(prevLevel);
}

void Governor::refresh()
{
    if (mode == true)
        this->adjustFanSpeed();
}

/*
 * If temperature is bigger than cpuMin(mid,max) then next if statement will be executed.
 * In this next statement treshold may or may not be set.
 * Algorithm: if temperature equal to cpuMin then treshold must be set, if temp get
 * lower to cpuMin-treshold (ex. 60-3 = 57) then treshold is set to zero and temp in
 * range 57-59 will not trigger cpuMid level execution.
 *
 * min = 75     mid = 80    max = 85
 * lvl - setLevel
 * td - treshold value. In this example is 3.
 *
 *              td=3     75        td=3(lvl(0))  80       td=3(lvl(1))  85      td=3(lvl(5))
 * --------------|-------|---------------|-------|--------------|-------|------------------
 *         lvl(0)                  lvl(1)                 lvl(5)                 lvl(7)
 */

void Governor::adjustFanSpeed()
{
    int cpuTmp = snsArray->cpu->getTemp();
    int gpuTmp = snsArray->gpu->getTemp().toInt();
    int mchTmp = snsArray->mch->getTemp().toInt();

    if (cpuTmp < plPtr->getCpuMin() - cpuTdTmp && gpuTmp < plPtr->getGpuMin() - gpuTdTmp &&
            mchTmp < plPtr->getMchMin() - mchTdTmp) {                                               // Temp < Min
//        if (!previousLevel == 0)
            this->setLevel(0);
    }
    else if (cpuTmp < plPtr->getCpuMid() - cpuTdTmp && gpuTmp < plPtr->getGpuMid() - gpuTdTmp &&
             mchTmp < plPtr->getMchMid() - mchTdTmp) {                                              // Min < Temp < Mid
//        if (!previousLevel == 1)
            this->setLevel(1);

        if (cpuTmp == plPtr->getCpuMin())
            cpuTdTmp = plPtr->getTreshold();
        else if (cpuTmp <= plPtr->getCpuMin() - plPtr->getTreshold())
            cpuTdTmp = 0;

        if (gpuTmp == plPtr->getGpuMin())
            gpuTdTmp = plPtr->getTreshold();
        else if (gpuTmp <= plPtr->getGpuMin() - plPtr->getTreshold())
            gpuTdTmp = 0;

        if (mchTmp == plPtr->getMchMin())
            mchTdTmp = plPtr->getTreshold();
        else if (mchTmp <= plPtr->getMchMin() - plPtr->getTreshold())
            mchTdTmp = 0;
    }
    else if (cpuTmp < plPtr->getCpuMax() - cpuTdTmp && gpuTmp < plPtr->getGpuMax() - gpuTdTmp &&
             mchTmp < plPtr->getMchMax() - mchTdTmp) {                                              // Mid < Temp < Max
//        if (!previousLevel == 5)
            this->setLevel(5);

        if (cpuTmp == plPtr->getCpuMid())
            cpuTdTmp = plPtr->getTreshold();
        else if (cpuTmp <= plPtr->getCpuMid() - plPtr->getTreshold())
            cpuTdTmp = 0;

        if (gpuTmp == plPtr->getGpuMid())
            gpuTdTmp = plPtr->getTreshold();
        else if (gpuTmp <= plPtr->getGpuMid() - plPtr->getTreshold())
            gpuTdTmp = 0;

        if (mchTmp == plPtr->getMchMid())
            mchTdTmp = plPtr->getTreshold();
        else if (mchTmp <= plPtr->getMchMid() - plPtr->getTreshold())
            mchTdTmp = 0;
    }
    else {                                                                              // Temp > Max
//        if (!previousLevel == 7)
            this->setLevel(7);

        if (cpuTmp == plPtr->getCpuMax())
            cpuTdTmp = plPtr->getTreshold();
        else if (cpuTmp <= plPtr->getCpuMax() - plPtr->getTreshold())
            cpuTdTmp = 0;

        if (gpuTmp == plPtr->getGpuMax())
            gpuTdTmp = plPtr->getTreshold();
        else if (gpuTmp <= plPtr->getGpuMax() - plPtr->getTreshold())
            gpuTdTmp = 0;

        if (mchTmp == plPtr->getMchMax())
            mchTdTmp = plPtr->getTreshold();
        else if (mchTmp <= plPtr->getMchMax() - plPtr->getTreshold())
            mchTdTmp = 0;
    }
}

Notifications::Notifications()
{
    settings = new QSettings("thinkctl", "settings");
    notify = new QDBusInterface("org.freedesktop.Notifications", "/org/freedesktop/Notifications",
                                      "org.freedesktop.Notifications", QDBusConnection::sessionBus());

    if (isSettingsExists())
        loadSettings();
    else
        addInitialSettings();

    if (!notify->isValid())
        qDebug() << "Cannot connect to dbus session";
}

Notifications::~Notifications()
{
    delete notify;
    delete settings;
}

bool Notifications::getWarnNotifySend() { return warnNotify; }
bool Notifications::getCritNotifySend() { return critNotify; }
int Notifications::getNotifyTreshold() { return notifyTreshold; }

void Notifications::loadSettings()
{
    settings->beginGroup("Notifications");
    warnNotify = settings->value("send_warning_notifications").toBool();
    critNotify = settings->value("send_critical_notifications").toBool();
    volumeLvl = settings->value("send_volume_level").toBool();
    notifyTreshold = settings->value("notifications_treshold").toInt();
    settings->endGroup();
}

void Notifications::saveSettings()
{
    settings->beginGroup("Notifications");
    settings->setValue("send_warning_notifications", warnNotify);
    settings->setValue("send_critical_notifications", critNotify);
    settings->setValue("send_volume_level", volumeLvl);
    settings->setValue("notifications_treshold", notifyTreshold);
    settings->endGroup();
}

void Notifications::addInitialSettings()
{
    warnNotify = false;
    critNotify = true;
    volumeLvl = true;
    notifyTreshold = 5;
}

bool Notifications::isSettingsExists()
{
    QFile f(settings->fileName());

    return f.exists();
}

void Notifications::sendMsg(QString str, bool constant)
{
    QList<QVariant> args;

    args.append(QString("ThinkControl"));   // app_name
    constant ? args.append(uint(29)) : args.append(uint(0)); // replaces_id
    args.append("");                        // app_icon
    args.append("");                        // summary
    args.append(str);                       // body
    args.append(QStringList());             // actions
    args.append(QVariantMap());             // hints
    args.append(int(10000));                // timeout

    notify->callWithArgumentList(QDBus::AutoDetect, "Notify", args);
}

void Notifications::sendWarnNotify(QString dev)
{
    if (warnNotify) {
        QString msg;
        msg.append("Attention: ").append(dev).append(" warning level reached!");
        sendMsg(msg, false);
    }
}

void Notifications::sendCritNotify(QString dev)
{
    if (critNotify) {
        QString msg;
        msg.append("Warning: ").append(dev).append(" critical level reached.");
        sendMsg(msg, false);
    }
}

void Notifications::sendProfileChanged(QString pName) {
    QString msg;
    msg.append("Profile changed to ").append(pName).append(".");
    sendMsg(msg, false);
}

void Notifications::sendShutdownPerformed(QString str)
{
    QString msg;
    msg.append("Performing ").append(str).append(" action.");
    sendMsg(msg, false);
}

void Notifications::sendVolumeLvl(int v, bool m)
{
    if (volumeLvl) {
        QString msg;
        msg.append("Volume: ");
        m ? msg.append("muted") : msg.append(QString::number(v));
        sendMsg(msg, true);
    }
}

WLGovernor::WLGovernor(SensorsArray *sa, ProfileList *pls)
{
    settings = new QSettings("thinkctl", "settings");

    if (isSettingsExists())
        loadSettings();
    else
        addInitialSettings();

    snsArray = sa;
    pfls = pls;

    updateLevels();
}

WLGovernor::~WLGovernor()
{
    saveSettings();
    delete settings;
}

void WLGovernor::loadSettings()
{
    settings->beginGroup("Warning_Levels");
    warnLvlDiff = settings->value("warning_level_difference").toInt();
    settings->endGroup();

    settings->beginGroup("Critical_Level_Actions");
    critLvlAction = (critLvlActionT)settings->value("critical_level_action").toInt();
    shutdownMethod = (shutdownMethodT)settings->value("shutdown_method").toInt();
    fallbackProfile = settings->value("fallback_profile").toInt();
    settings->endGroup();
}

void WLGovernor::saveSettings()
{
    settings->beginGroup("Warning_Levels");
    settings->setValue("warning_level_difference", warnLvlDiff);
    settings->endGroup();

    settings->beginGroup("Critical_Level_Actions");
    settings->setValue("critical_level_action", critLvlAction);
    settings->setValue("shutdown_method", shutdownMethod);
    settings->setValue("fallback_profile", fallbackProfile);
    settings->endGroup();
}

void WLGovernor::addInitialSettings()
{
    warnLvlDiff = 10;
    critLvlAction = NONE;
    shutdownMethod = SUSP;
    fallbackProfile = 0;
}

bool WLGovernor::isSettingsExists()
{
    QFile f(settings->fileName());

    return f.exists();
}

WLevelsT WLGovernor::checkLevel(QString devname, WLDevice *dev, QString t, int ct)
{
        int temp;

        if (t == "none")
            return NSEN;
        else
            temp = t.toInt();

        if (temp < ct-warnLvlDiff*2) {

            if (dev->warnNotifyWasSent == true && temp < ct-warnLvlDiff*2-notifyTreshold)
                dev->warnNotifyWasSent = false;

            return NORM;

        } else if (temp < ct-warnLvlDiff) {

            if (!dev->warnNotifyWasSent) {
                this->sendWarnNotify(devname);
                dev->warnNotifyWasSent = true;
            }

            if (dev->critNotifyWasSent == true && temp < ct-warnLvlDiff-notifyTreshold)
                dev->critNotifyWasSent = false;

            return WARN;

        } else {

            if (!dev->critNotifyWasSent) {
                this->sendCritNotify(devname);
                dev->critNotifyWasSent = true;
            }

            checkForCritActions();

            return CRIT;
        }
}

void WLGovernor::updateLevels()
{
    cpu.level = checkLevel("cpu", &cpu, QString::number(snsArray->cpu->getTemp()), snsArray->cpu->getCritTemp());
    gpu.level = checkLevel("gpu", &gpu, snsArray->gpu->getTemp(), snsArray->gpu->getCritTemp());
    mch.level = checkLevel("mch", &mch, snsArray->mch->getTemp(), snsArray->mch->getCritTemp());
    ich.level = checkLevel("ich", &ich, snsArray->ich->getTemp(), snsArray->ich->getCritTemp());
    aps.level = checkLevel("aps", &aps, snsArray->aps->getTemp(), snsArray->aps->getCritTemp());
    pwr.level = checkLevel("pwr", &pwr, snsArray->pwr->getTemp(), snsArray->pwr->getCritTemp());
    pcmcia.level = checkLevel("pcmcia", &pcmcia, snsArray->pcmcia->getTemp(), snsArray->pcmcia->getCritTemp());
    mainBatFirst.level = checkLevel("main battery", &mainBatFirst, snsArray->mainBatFirst->getTemp(), snsArray->mainBatFirst->getCritTemp());
    mainBatSecond.level = checkLevel("main battery", &mainBatSecond, snsArray->mainBatSecond->getTemp(), snsArray->mainBatSecond->getCritTemp());
//    mainBatSecond.level = checkLevel("main battery", &mainBatSecond, snsArray->mainBatSe->getTemp(), snsArray->mainBat->getCritTemp());
    bayBatFirst.level = checkLevel("bay battery", &bayBatFirst, snsArray->bayBatFirst->getTemp(), snsArray->bayBatFirst->getCritTemp());
    bayBatSecond.level = checkLevel("bay battery", &bayBatSecond, snsArray->bayBatSecond->getTemp(), snsArray->bayBatSecond->getCritTemp());
}

void WLGovernor::checkForCritActions()
{
    if (critLvlAction == CPROF) {
        int cp = pfls->getCurrentProfile();

        if (cp != fallbackProfile) {
            sendProfileChanged(pfls->at(fallbackProfile)->getName());
            emit changeProfileTo(fallbackProfile);
        }
    } else if (critLvlAction == SHUTD) {
        if (shutdownMethod == SUSP) {
            sendShutdownPerformed("suspend");
            performSuspend();
        } else if (shutdownMethod == HBTE) {
            sendShutdownPerformed("hibernate");
            performHibernate();
        }
    }
}


WLevelsT WLGovernor::cpuGetLevel() { return cpu.level; }
WLevelsT WLGovernor::gpuGetLevel() { return gpu.level; }
WLevelsT WLGovernor::mchGetLevel() { return mch.level; }
WLevelsT WLGovernor::ichGetLevel() { return ich.level; }
WLevelsT WLGovernor::apsGetLevel() { return aps.level; }
WLevelsT WLGovernor::pwrGetLevel() { return pwr.level; }
WLevelsT WLGovernor::pcmciaGetLevel() { return pcmcia.level; }
WLevelsT WLGovernor::mainBatFirstGetLevel() { return mainBatFirst.level; }
WLevelsT WLGovernor::mainBatSecondGetLevel() { return mainBatSecond.level; }
WLevelsT WLGovernor::bayBatFirstGetLevel() { return bayBatFirst.level; }
WLevelsT WLGovernor::bayBatSecondGetLevel() { return bayBatSecond.level; }

int WLGovernor::getWarnLvlDiff() { return warnLvlDiff; }
critLvlActionT WLGovernor::getCritLvlAction() { return critLvlAction; }
shutdownMethodT WLGovernor::getShutdownMethod() { return shutdownMethod; }
int WLGovernor::getFallbackProfile() { return fallbackProfile; }

void WLGovernor::setWarnLvlDiff(int n) { warnLvlDiff = n; }
void WLGovernor::setWarnNotifySend(bool s) { warnNotify = s; }
void WLGovernor::setCritNotifySend(bool s) { critNotify = s; }
void WLGovernor::setNotifyTreshold(int n) { notifyTreshold = n; }
void WLGovernor::setCritLvlAction(critLvlActionT action) { critLvlAction = action; }
void WLGovernor::setShutdownMethod(shutdownMethodT method) { shutdownMethod = method; }
void WLGovernor::setFallbackProfile(int n) { fallbackProfile = n; }

CritTempActions::CritTempActions()
{
    dbus = new QDBusInterface("org.freedesktop.PowerManagement", "/org/freedesktop/PowerManagement",
                              "org.freedesktop.PowerManagement", QDBusConnection::sessionBus());

    if (!dbus->isValid())
        qDebug() << "Cannot connect to dbus session";
}

CritTempActions::~CritTempActions()
{
    delete dbus;
}

void CritTempActions::performSuspend()
{
    if (!actionWasCompleted)
        dbus->call("Suspend");
}

void CritTempActions::performHibernate()
{
    if (!actionWasCompleted)
        dbus->call("Hibernate");
}

BatteryGovernor::BatteryGovernor()
{
    settings = new QSettings("thinkctl", "batteries");

    QFile f("/proc/modules");
    QTextStream ts(&f);
    f.open(QIODevice::ReadOnly);

    if (ts.readAll().contains("tp_smapi"))
        presence = true;
    else
        presence = false;

    if (presence == true) {
        mainBat = new Battery(0);

        if (isSettingsExists(settings))
            loadSettings();
    }
}

bool BatteryGovernor::isModulePresent()
{
    return presence;
}

void BatteryGovernor::loadSettings()
{
    settings->beginGroup("Main_Battery");
    mainBat->setStartChargingTreshold(settings->value("start_charging_treshold").toInt());
    mainBat->setStopChargingTreshold(settings->value("stop_charging_treshold").toInt());
    settings->endGroup();
}

void BatteryGovernor::saveSettings()
{
    settings->beginGroup("Main_Battery");
    settings->setValue("start_charging_treshold", mainBat->getStartChargingTreshold());
    settings->setValue("stop_charging_threshold", mainBat->getStopChargingTreshold());
    settings->endGroup();
}

void BatteryGovernor::updateBatteriesState()
{
    QString mainBatState = mainBat->getState();

    if (mainBatState == "none")
        if (mainBatPrevState != mainBatState)
            emit mainBatInstalled(false);

    else {
        if (mainBatPrevState == "none")
            emit mainBatInstalled(true);

        emit mainBatValuesChanged();
    }

/*
    switch (batStates.indexOf(mainBatState)) {

    case 0:     // none

        if (mainBatPrevState != mainBatState)
            emit mainBatInstalled(false);

        break;

    case 1:     // idle

        if (mainBatPrevState == "none")
            emit mainBatInstalled(true);

        if (mainBatPrevState != mainBatState)
            emit mainBatStateChanged(mainBatState);

        break;

    case 2:     // charging

        if (mainBatPrevState == "none")
            emit mainBatInstalled(true);

        if (mainBatPrevState != mainBatState)
            emit mainBatStateChanged(mainBatState);

        emit mainBatValuesChanged();
        break;

    case 3:     // discharging

        if (mainBatPrevState == "none")
            emit mainBatInstalled(true);

        if (mainBatPrevState != mainBatState)
            emit mainBatStateChanged(mainBatState);

        emit mainBatValuesChanged();
        break;
    }

    mainBatPrevState = mainBatState;
*/
}

void BatteryGovernor::mainBatSetStartChargeTreshold(int n)
{
    mainBat->setStartChargingTreshold(n);
}

void BatteryGovernor::mainBatSetStopChargeTreshold(int n)
{
    mainBat->setStopChargingTreshold(n);
}
/*
APSGovernor::APSGovernor()
{
    QFile f("/var/run/hdapsd.pid");

    if (f.exists()) {
        QTextStream ts(&f);
        pid = ts.readAll().toInt();
        isRunned = true;

    } else {
        pid = -1;
        isRunned = false;
    }


    proc = new QProcess();
    pid = -1;               // if hdapsd not runned

    connect(proc, SIGNAL(readyReadStandardOutput()), this, SLOT(readFoundPid()));
    proc->start("pidof", QStringList() << "hdapsd");
    proc->waitForFinished();

}

void APSGovernor::readFoundPid()
{
    pid = proc->readAllStandardOutput().toInt();

    if (pid == -1)
        isRunned = false;
    else
        isRunned = true;
}

bool APSGovernor::isAlredyStarted()
{
    return isRunned;
}

void APSGovernor::start()
{
    proc = new QProcess();
    QStringList args;

    args << "-s" << sensitivity;

    if (adaptive)
        args << "-a";

    proc->start("hdapsd", args);
}

void APSGovernor::stop()
{
    proc->close();
}
*/
OSD::OSD()
{
    kosdPresent = isKosdPresent();
}

OSD::~OSD()
{
    delete osdDbus;
}

void OSD::sendMsg(int vl, bool mSt)
{
    if (kosdPresent)
        sendKosd(vl, mSt);
    else
        sendVolumeLvl(vl, mSt);
}

/* Check for KOSD and stay on bus if exist */

bool OSD::isKosdPresent()
{
    osdDbus = new QDBusInterface("org.kde.kded", "/modules/kosd", "org.kde.kosd", QDBusConnection::sessionBus());

    if (osdDbus->isValid())
        return true;
    else
        return false;
}

void OSD::sendKosd(int n, bool m)
{
    QList<QVariant> args;

    args.append(n);     // volume level
    args.append(m);     // mute state

    osdDbus->callWithArgumentList(QDBus::AutoDetect, "showVolume", args);
}

TPVolume::TPVolume()
{
    const char *mixName = "Console";
//    const char *cardName = "hw:29";
    const char *cardName = "hw:4";
    const int mixIndex = 0;

    snd_mixer_selem_id_alloca(&selemId);
    snd_mixer_selem_id_set_index(selemId, mixIndex);
    snd_mixer_selem_id_set_name(selemId, mixName);
    elemCallback = showVolumeLvl;

    if (snd_mixer_open(&handle, 0) < 0)
        qDebug() << "Cannot open alsa mixer handle";

    if (snd_mixer_attach(handle, cardName) < 0) {
        qDebug() << "Cannot attach alsa mixer handle to device";
        snd_mixer_close(handle);
    }

    if (snd_mixer_selem_register(handle, NULL, NULL) < 0) {
        qDebug() << "Cannot register alsa selem class";
        snd_mixer_close(handle);
    }

    if (snd_mixer_load(handle) < 0) {
        qDebug() << "Cannot load alsa mixer elem";
        snd_mixer_close(handle);
    }

    elem = snd_mixer_find_selem(handle, selemId);
    if (elem == NULL) {
        qDebug() << "Cannot find alsa selem";
        snd_mixer_close(handle);
    }

    snd_mixer_elem_set_callback(elem, elemCallback);

    timer = new QTimer;
    connect(timer, SIGNAL(timeout()), this, SLOT(checkForEvent()));
    timer->start(100);
}

TPVolume::~TPVolume()
{
    delete timer;
    snd_mixer_close(handle);
}

void TPVolume::checkForEvent()
{
    snd_mixer_handle_events(handle);
}

int TPVolume::showVolumeLvl(snd_mixer_elem_t *e, unsigned int m)
{
    OSD osd;
    long volumeLvl = -1;
    long minVl, maxVl;
    int muteState;

    snd_mixer_selem_get_playback_volume_range(e, &minVl, &maxVl);

    /* get mute switcher state */
    if (snd_mixer_selem_has_playback_switch(e))
        if (snd_mixer_selem_get_playback_switch(e, (snd_mixer_selem_channel_id_t)0, &muteState)) {
            qDebug() << "Cannot get mute state";
            return -1;
        }
/*
    if (!muteState) {
        osd.sendMsg(0);     // send zero if muted
        return 0;
    }
*/
    if (snd_mixer_selem_get_playback_volume(e, (snd_mixer_selem_channel_id_t)0, &volumeLvl) < 0) {
        qDebug() << "Canot get volume level";
        return -1;
    }

    /* make the value bound to 100 */
    volumeLvl -= minVl;
    maxVl -= minVl;
    minVl = 0;
    volumeLvl = 100 * volumeLvl / maxVl; // make the value bound from 0 to 100

    osd.sendMsg(volumeLvl, !muteState);     // invert muteState to make it false when not muted
    return 0;
}
