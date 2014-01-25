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



#include "h/mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    dbus(QDBusConnection::systemBus())
{
    ui->setupUi(this);

    if (dbus.isConnected())
        dbus.connect("org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower",
                                "Resuming", this, SLOT(applyAfterSusped()));
    else
        qDebug() << "Cannot connect to system dbus interface";

    if (profiles.isSettingsExists())
        profiles.loadProfiles();
    else
        profiles.addInitialProfiles();
    currentProfile = profiles.getCurrentProfile();

    timer = new Timer();
    sensorsArray = new SensorsArray();
    gov = new Governor(sensorsArray, profiles.at(currentProfile));
    wlgov = new WLGovernor(sensorsArray, &profiles);
//    apsgov = new APSGovernor();
    batgov = new BatteryGovernor();
    ws = new WirelessSwitchers();
    tp = new TrackPoint();
    touchpad = new TouchPad();
    tpvol = new TPVolume();

    ui->trackpointEnabled->setChecked(tp->getState());
    ui->TouchPadWidget->setEnabled(touchpad->isPresent());
    ui->touchpadEnabled->setChecked(touchpad->getState());

    this->initTrayIcon();
    this->initProfiles();
    this->initMachineInfo();
    this->initCpuPolicies();
    this->initGpuStates();
    this->setWirelessStates();

    // Check for module presence. Disable Battery tab if false, otherwise
    // check for main bat and bay bat presence and disable ui if they wasn't found.

    if (batgov->isModulePresent()) {

        if (batgov->mainBat->isInstalled())
            this->initMainBatInfo();
        else
            this->mainBatInstalled(false);

        /* Main Battery */
        connect(batgov, SIGNAL(mainBatInstalled(bool)), this, SLOT(mainBatInstalled(bool)));
//        connect(batgov, SIGNAL(mainBatStateChanged(QString)), this, SLOT(mainBatUpdateState(QString)));
        connect(batgov, SIGNAL(mainBatValuesChanged()), this, SLOT(mainBatRefreshValues()));
        connect(ui->mainBatChargTreshsStartSpinBox, SIGNAL(valueChanged(int)), batgov, SLOT(mainBatSetStartChargeTreshold(int)));
        connect(ui->mainBatChargTreshsStopSpinBox, SIGNAL(valueChanged(int)), batgov, SLOT(mainBatSetStopChargeTreshold(int)));

    } else {
        ui->Battery->setDisabled(true);
    }


    realyClose = false;                                     // by default we go in tray
    this->programCtrlActivated(settings.isProgramControlled());       // set fan control
    ui->programCtrlBtn->setDown(settings.isProgramControlled());

    /* Timer */
    connect(timer, SIGNAL(timeout()), sensorsArray, SLOT(updateThermValues()));
    connect(timer, SIGNAL(timeout()), batgov, SLOT(updateBatteriesState()));

    /* Sensors Array */
    connect(sensorsArray, SIGNAL(thermalValuesUpdated()), gov, SLOT(refresh()));
    connect(sensorsArray, SIGNAL(thermalValuesUpdated()), wlgov, SLOT(updateLevels()));
    connect(sensorsArray, SIGNAL(thermalValuesUpdated()), this, SLOT(refreshValues()));

    /* Fan */
    connect(ui->programCtrlBtn, SIGNAL(toggled(bool)), this, SLOT(programCtrlActivated(bool)));
    connect(ui->presetRadBtn, SIGNAL(clicked()), this, SLOT(presetCtrlActivated()));
    connect(ui->manualRadBtn, SIGNAL(clicked()), this, SLOT(manualCtrlActivated()));
    connect(ui->speedLevelDial, SIGNAL(valueChanged(int)), this, SLOT(speedLvlDial(int)));
    connect(ui->fanOffBtn, SIGNAL(toggled(bool)), gov, SLOT(fanOff(bool)));
    connect(ui->fanFullSpeedBtn, SIGNAL(toggled(bool)), gov, SLOT(fanFullSpeed(bool)));
    connect(ui->fanPresetDialog, SIGNAL(clicked()), this, SLOT(fanPresetBtnPressed()));

    /* Warning Levels Governor */
    connect(wlgov, SIGNAL(changeProfileTo(int)), this, SLOT(profileChoosed(int)));

    /* CPU Frequency and GPU profiles */
    connect(ui->frequencyBox, SIGNAL(activated(int)), this, SLOT(cpuPolicyChoosed(int)));
//    connect(ui->methodComboBox, SIGNAL(activated(int)), this, SLOT(gpuMethodChoosed(int)));
    connect(ui->profileComboBox, SIGNAL(activated(int)), this, SLOT(gpuProfileChoosed(int)));

    /* APS */
//    connect(ui->apsStartButton, SIGNAL(clicked()), this, SLOT(apsStartBtnPressed()));
//    connect(ui->apsStopButton, SIGNAL(clicked()), this, SLOT(apsStopBtnPressed()));
//    connect(ui->apsSensitivitySpinBox, SIGNAL(valueChanged(int)), apsgov, SLOT(setSensitivity(int)));
//    connect(ui->apsAdaptiveChkBox, SIGNAL(toggled(bool)), apsgov, SLOT(setAdaptive(bool)));

    /* Profiles */
    connect(ui->profileChooser, SIGNAL(activated(int)), this, SLOT(profileChoosed(int)));
    connect(ui->profileAdd, SIGNAL(clicked()), this, SLOT(profileAddBtnPressed()));
    connect(&profileLnDialog, SIGNAL(endOfInput(QString)), this, SLOT(addProfileAction(QString)));
    connect(ui->profileRemove, SIGNAL(clicked()), this, SLOT(deleteProfileAction()));

    /* Wireless Switchers */
    connect(ui->wanButton, SIGNAL(toggled(bool)), this, SLOT(wanBtnSwitched(bool)));
    connect(ui->uwbButton, SIGNAL(toggled(bool)), this, SLOT(uwbBtnSwitched(bool)));
    connect(ui->btButton, SIGNAL(toggled(bool)), this, SLOT(btBtnSwitched(bool)));

    /* Settings */
    connect(ui->settingsDialog, SIGNAL(clicked()), this, SLOT(settingsDialogBtnPressed()));

    /* TrackPoint */
    connect(ui->trackpointDialog, SIGNAL(clicked()), this, SLOT(trackpointDialogBtnPressed()));
    connect(ui->trackpointEnabled, SIGNAL(clicked(bool)), this, SLOT(trackpointEnabledChecked()));

    /* TouchPad */
    connect(ui->touchpadDialog, SIGNAL(clicked()), this, SLOT(touchpadDialogBtnPressed()));
    connect(ui->touchpadEnabled, SIGNAL(clicked(bool)), this, SLOT(touchpadEnabledChecked()));
}

MainWindow::~MainWindow()
{
    profiles.setCurrentProfile(currentProfile);
    profiles.saveProfiles();
    this->gov->setLevelAuto();

    delete tp;
    delete touchpad;
    delete ws;
    delete wlgov;
    delete gov;
    delete sensorsArray;
    delete timer;
    delete ui;
}

QString MainWindow::getColor(WLevelsT lvl)
{
    QString color("color: ");

    switch (lvl) {
    case NSEN:
        return color.append("black");
        break;
    case NORM:
        return color.append(settings.getNormColor());
        break;
    case WARN:
        return color.append(settings.getWarnColor());
        break;
    case CRIT:
        return color.append(settings.getCritColor());
        break;
    }
}

void MainWindow::refreshValues()
{
    /* Term */
    ui->cpuValueLabel->setNum(sensorsArray->cpu->getTemp());
    ui->cpuValueLabel->setStyleSheet(getColor(wlgov->cpuGetLevel()));
    ui->gpuValueLabel->setText(sensorsArray->gpu->getTemp());
    ui->gpuValueLabel->setStyleSheet(getColor(wlgov->gpuGetLevel()));
    ui->mchValueLabel->setText(sensorsArray->mch->getTemp());
    ui->mchValueLabel->setStyleSheet(getColor(wlgov->mchGetLevel()));
    ui->ichValueLabel->setText(sensorsArray->ich->getTemp());
    ui->ichValueLabel->setStyleSheet(getColor(wlgov->ichGetLevel()));
    ui->apsValueLabel->setText(sensorsArray->aps->getTemp());
    ui->apsValueLabel->setStyleSheet(getColor(wlgov->apsGetLevel()));
    ui->pwrValueLabel->setText(sensorsArray->pwr->getTemp());
    ui->pcmciaValueLabel->setText(sensorsArray->pcmcia->getTemp());
    ui->mainBatFstSenValLabel->setText(sensorsArray->mainBatFirst->getTemp());
    ui->mainBatFstSenValLabel->setStyleSheet(getColor(wlgov->mainBatFirstGetLevel()));
//    ui->mainBatSecSenValLabel->setText(sensorsArray->mainBatSecond->getTemp());
    ui->mainBatSecSenValLabel->setText(sensorsArray->mainBatSecond->getTemp());
    ui->mainBatSecSenValLabel->setStyleSheet(getColor(wlgov->mainBatSecondGetLevel()));
    ui->bayBatFstSenValLabel->setText(sensorsArray->bayBatFirst->getTemp());
    ui->bayBatFstSenValLabel->setStyleSheet(getColor(wlgov->bayBatFirstGetLevel()));
    ui->bayBatSecSenValLabel->setText(sensorsArray->bayBatSecond->getTemp());
    ui->bayBatSecSenValLabel->setStyleSheet(getColor(wlgov->bayBatSecondGetLevel()));

    /* Fan */
    ui->fanSpeedValueLabel->setNum(gov->getSpeed());
    ui->fanSpeedValueLabelOvw->setNum(gov->getSpeed());
    ui->fanLevelValueLabel->setText(gov->getLevel());
    ui->fanLevelValueLabelOvw->setText(gov->getLevel());
}

void MainWindow::programCtrlActivated(bool s)
{
    if (s == true) {
        ui->controlledBox->setEnabled(true);
        if (ui->presetRadBtn->isChecked())
            this->presetCtrlActivated();
        else if (ui->manualRadBtn->isChecked())
            this->manualCtrlActivated();
    } else {
        ui->controlledBox->setEnabled(false);
        ui->speedLevelWidget->setEnabled(false);
        ui->speedLevelDialerLabel->setEnabled(false);
        ui->speedLevelDial->setEnabled(false);
        ui->swithersWidget->setEnabled(false);
        gov->setMode(false);
        gov->setLevelAuto();
    }

    settings.setProgramControlled(s);
}

void MainWindow::presetCtrlActivated()
{
    gov->setMode(true);

    ui->speedLevelWidget->setEnabled(false);
    ui->speedLevelDialerLabel->setEnabled(false);
    ui->speedLevelDial->setEnabled(false);
    ui->swithersWidget->setEnabled(false);
}

void MainWindow::manualCtrlActivated()
{
    gov->setMode(false);

    ui->speedLevelWidget->setEnabled(true);
    ui->speedLevelDialerLabel->setEnabled(true);
    ui->speedLevelDial->setEnabled(true);
    ui->swithersWidget->setEnabled(true);
}

void MainWindow::fanPresetBtnPressed()
{
    fanPstDialog.showDialog(profiles.at(currentProfile));
}

void MainWindow::profileAddBtnPressed()
{
    profileLnDialog.show();
}

void MainWindow::speedLvlDial(int n)
{
    if (ui->fanOffBtn->isDown())
        ui->fanOffBtn->setDown(false);

    gov->setLevel(n);
}

void MainWindow::initProfiles()
{
    for (int i = 0; i < profiles.size(); i++)
        ui->profileChooser->addItem(profiles.at(i)->getName());

    ui->profileChooser->setCurrentIndex(currentProfile);
}

void MainWindow::initCpuPolicies()
{
    ui->frequencyBox->addItem("ondemand");
    ui->frequencyBox->addItems(sensorsArray->cpu->getAvailFreq());
    ui->frequencyBox->setCurrentIndex(profiles.at(currentProfile)->getCpuPolicy());
    this->setCpuPolicy(currentProfile);
}

/* Check if driver present, fill gpu profiles combobox and set current profile settings */
void MainWindow::initGpuStates()
{
    if (sensorsArray->gpu->isPresent()) {
        this->initGpuProfiles();
        this->setGpuPolicy(currentProfile);
    } else
        ui->gpuBox->setEnabled(false);
}
/*
void MainWindow::initGpuMethods()
{
    ui->methodComboBox->addItems(sensorsArray->gpu->getMethodsLst());
    ui->methodComboBox->setCurrentIndex(profiles.at(currentProfile)->getGpuMethod());
}
*/
void MainWindow::initGpuProfiles()
{
    ui->profileComboBox->addItems(sensorsArray->gpu->getProfiles());
}
/*
void MainWindow::initAPS()
{
    if (apsgov->isAlredyStarted())
        ui->apsStartButton->setText("❙❙");
    else
        ui->apsStartButton->setText("▶");

    ui->apsSensitivitySpinBox->setValue(apsgov->getSensitivity());
    ui->apsAdaptiveChkBox->setChecked(apsgov->getAdaptive());
}

void MainWindow::apsStartBtnPressed()
{
    if (apsgov->isAlredyStarted()) {
        apsgov->pause();

    } else {
        apsgov->start();
        ui->apsStartButton->setText("❙❙");
    }
}

void MainWindow::apsStopBtnPressed()
{
    apsgov->stop();
    ui->apsStartButton->setText("▶");
}
*/
void MainWindow::initMainBatInfo()
{
//    ui->mainBatStateValLbl->setText(batgov->mainBat->getState());

    this->mainBatRefreshValues();

    ui->mainBatFirstUseDatValLbl->setText(batgov->mainBat->getFirstUseDate());

    ui->mainBatManufValLbl->setText(batgov->mainBat->getManufacturer());
    ui->mainBatModelValLbl->setText(batgov->mainBat->getModel());
    ui->mainBatTypeValLbl->setText(batgov->mainBat->getType());
    ui->mainBatDesVoltValLbl->setText(QString::number((float)batgov->mainBat->getDesignVoltage()/1000).append("V"));
    ui->mainBatDesCapValLbl->setText(QString::number((float)batgov->mainBat->getDesignCapacity()/1000).append("Wh"));
    ui->mainBatManufDateValLbl->setText(batgov->mainBat->getManufactureDate());

    ui->mainBatChargTreshsStartSpinBox->setValue(batgov->mainBat->getStartChargingTreshold());
    ui->mainBatChargTreshsStopSpinBox->setValue(batgov->mainBat->getStopChargingTreshold());
}

void MainWindow::mainBatRefreshValues()
{
    ui->mainBatStateValLbl->setText(batgov->mainBat->getState());
    ui->mainBatChargedValLbl->setText(QString::number(batgov->mainBat->getChargeLvl()).append('%'));

    if (batgov->mainBat->isACConnected()) {
        ui->mainBatVolatileLbl->setText("Charging time:");
        if (batgov->mainBat->getChargingTime() == 0)
            ui->mainBatVolatileValLbl->setText("charged");
        else
            ui->mainBatVolatileValLbl->setText(minToHrsAndMin(batgov->mainBat->getChargingTime()));
    } else {
        ui->mainBatVolatileLbl->setText("Running time:");
        ui->mainBatVolatileValLbl->setText(minToHrsAndMin(batgov->mainBat->getRemainingRunningTime()));
    }

    ui->mainBatCyclCountValLbl->setNum(batgov->mainBat->getCyclesCount());
    ui->mainBatCurVoltValLbl->setText(QString::number((float)batgov->mainBat->getCurrentVoltage()/1000).append("V"));
    ui->mainBatRemCapValLbl->setText(QString::number((float)batgov->mainBat->getRemainingCapacity()/1000).append("Wh"));
}
/*
void MainWindow::mainBatUpdateState(QString s)
{
    ui->mainBatStateValLbl->setText(s);
}
*/
void MainWindow::mainBatInstalled(bool s)
{
    if (s) {
        this->mainBatWidgetsState(true);
        this->initMainBatInfo();

    } else {
        ui->mainBatStateValLbl->setText("Not installed");
        this->mainBatWidgetsState(false);
    }
}

void MainWindow::mainBatWidgetsState(bool s)
{
    ui->mainBatChargedWidget->setEnabled(s);
    ui->mainBatCyclesWidget->setEnabled(s);
    ui->mainBatInfoWidget->setEnabled(s);
    ui->mainBatChargingTresholdsGB->setEnabled(s);
    ui->mainBatCalibrationBtn->setEnabled(s);
}

void MainWindow::cpuPolicyChoosed(int p)
{
    QStringList alFq = sensorsArray->cpu->getAvailFreq();

    if (p == 0)
        sensorsArray->cpu->setGovernor("ondemand");
    else
        sensorsArray->cpu->setFreq(alFq.at(p-1).toInt());        // -1 for shift index

    profiles.at(currentProfile)->setCpuPolicy(p);
}
/*
void MainWindow::gpuMethodChoosed(int m)
{
    QStringList mLst = sensorsArray->gpu->getMethodsLst();
    sensorsArray->gpu->setMethod(mLst.at(m));
    profiles.at(currentProfile)->setGpuMethod(m);
}
*/
void MainWindow::gpuProfileChoosed(int p)
{
    sensorsArray->gpu->setProfile(p);
    profiles.at(currentProfile)->setGpuProfile(p);
}

void MainWindow::profileChoosed(int p)
{
    currentProfile = p;
    profiles.setCurrentProfile(p);

    if (ui->profileChooser->currentIndex() != p)
        ui->profileChooser->setCurrentIndex(p);

    gov->profileChanged(profiles.at(p));

    this->setCpuPolicy(p);
    if (ui->gpuBox->isEnabled())
        this->setGpuPolicy(p);
}

void MainWindow::setCpuPolicy(int n)
{
    ui->frequencyBox->setCurrentIndex(profiles.at(n)->getCpuPolicy());
    if (profiles.at(n)->getCpuPolicy() == 0)
        sensorsArray->cpu->setGovernor("ondemand");
    else
        sensorsArray->cpu->setFreq(sensorsArray->cpu->getAvailFreq().at(profiles.at(n)->getCpuPolicy()-1).toInt());
                                                                                                        // first get the policy num
                                                                                                        // from profile then get the
                                                                                                        // frequency from avail freq list
                                                                                                        // -1 to remove "ondemand"
}

void MainWindow::setGpuPolicy(int n)
{
    ui->profileComboBox->setCurrentIndex(profiles.at(n)->getGpuProfile());
    sensorsArray->gpu->setProfile(profiles.at(n)->getGpuProfile());
}

void MainWindow::setWirelessStates()
{
    int wan, uwb, bt;

    wan = ws->wan->getState();
    uwb = ws->uwb->getState();
    bt = ws->bt->getState();

    if (wan == -1) {
        ui->wanLabel->setEnabled(false);
        ui->wanButton->setEnabled(false);
    } else if (wan == 1)
        ui->wanButton->setDown(true);

    if (uwb == -1) {
        ui->uwbLabel->setEnabled(false);
        ui->uwbButton->setEnabled(false);
    } else if (uwb == 1)
        ui->uwbButton->setDown(true);

    if (bt == -1) {
        ui->btLabel->setEnabled(false);
        ui->btButton->setEnabled(false);
    } else if (bt == 1)
        ui->btButton->setDown(true);
}

void MainWindow::wanBtnSwitched(bool s)
{
    if (s == true)
        ws->wan->turnOn();
    else
        ws->wan->turnOff();
}

void MainWindow::uwbBtnSwitched(bool s)
{
    if (s == true)
        ws->uwb->turnOn();
    else
        ws->uwb->turnOff();
}

void MainWindow::btBtnSwitched(bool s)
{
    if (s == true)
        ws->bt->turnOn();
    else
        ws->bt->turnOff();
}

void MainWindow::addProfileAction(QString s)
{
    profiles.addProfile(s);
    currentProfile = profiles.size() - 1;
    ui->profileChooser->addItem(profiles.at(currentProfile)->getName());
    ui->profileChooser->setCurrentIndex(currentProfile);
    this->profileChoosed(currentProfile);                   // fallback to previous profile
}

void MainWindow::deleteProfileAction()
{
    profiles.deleteProfile(currentProfile);
    ui->profileChooser->removeItem(currentProfile);
    currentProfile -= 1;
    this->profileChoosed(currentProfile);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (realyClose)
        event->accept();
    else {
        windowPosition = this->pos();
        this->hide();
        event->ignore();
    }
}

void MainWindow::initTrayIcon()
{
    trayIcon = new QSystemTrayIcon(QIcon("/usr/share/thinkctl/icons/trayicon.svg"));  // fix-me: change path
    trayIconMenu = new QMenu();
    actionExit = new QAction(trayIconMenu);

    actionExit->setText("Exit");
    trayIconMenu->addAction(actionExit);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->show();

    connect(trayIconMenu, SIGNAL(triggered(QAction*)), this, SLOT(trayMenuAction(QAction*)));
    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
}

void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch(reason) {
    case QSystemTrayIcon::Trigger:
        if (this->isVisible()) {
            windowPosition = this->pos();
            this->hide();
        } else {
            this->move(windowPosition);
            this->show();
        }

        break;

    default:
        ;
    }
}

void MainWindow::trayMenuAction(QAction *action)
{
    if (action == actionExit) {
        realyClose = true;
        this->close();
    }
}

void MainWindow::initMachineInfo()
{
    ui->typeValueLabel->setText(mi.getType());
    ui->modelValueLabel->setText(mi.getModel());
    ui->biosValueLabel->setNum(mi.getBiosVersion());
}

void MainWindow::settingsDialogBtnPressed()
{
    SettingsDialog *sd = new SettingsDialog(settings, profiles, wlgov);
    sd->setAttribute(Qt::WA_DeleteOnClose, true);
}

void MainWindow::trackpointEnabledChecked()
{
    tp->setEnabled(ui->trackpointEnabled->isChecked());
}

void MainWindow::trackpointDialogBtnPressed()
{
    TrackPointDialog *tpd = new TrackPointDialog(tp);
    tpd->setAttribute(Qt::WA_DeleteOnClose);
}

void MainWindow::touchpadEnabledChecked()
{
    touchpad->setEnabled(ui->touchpadEnabled->isChecked());
}

void MainWindow::touchpadDialogBtnPressed()
{
    TouchPadDialog *tpd = new TouchPadDialog(touchpad);
    tpd->setAttribute(Qt::WA_DeleteOnClose);
}

void MainWindow::applyAfterSusped()
{
    touchpad->applySettings();
    this->mainBatRefreshValues();
}

QString minToHrsAndMin(int m)
{
    QString str;
    int hrs, min;
    hrs = m/60;
    min = m%60;
    str = QString::number(hrs).append("h").append(" ").append(QString::number(min)).append("m");
    return str;
}


