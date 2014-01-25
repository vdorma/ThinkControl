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



#include "h/dialogs.h"
#include "ui_fanpreset.h"
#include "ui_profileline.h"
#include "ui_settings.h"
#include "ui_trackpoint.h"
#include "ui_touchpad.h"

FanPresetDialog::FanPresetDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FanPresetDialog)
{
    ui->setupUi(this);

    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(setValues()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(close()));
}

FanPresetDialog::~FanPresetDialog()
{
    delete ui;
}

void FanPresetDialog::setValues()
{
    pf->setCpuMin(ui->cpuMinSpinBox->value());
    pf->setCpuMid(ui->cpuMidSpinBox->value());
    pf->setCpuMax(ui->cpuMaxSpinBox->value());

    pf->setGpuMin(ui->gpuMinSpinBox->value());
    pf->setGpuMid(ui->gpuMidSpinBox->value());
    pf->setGpuMax(ui->gpuMaxSpinBox->value());

    pf->setMchMin(ui->mchMinSpinBox->value());
    pf->setMchMid(ui->mchMidSpinBox->value());
    pf->setMchMax(ui->mchMaxSpinBox->value());

    pf->setTreshold(ui->tresholdSpinBox->value());

    this->close();
}

void FanPresetDialog::showDialog(Profile *p)
{
    pf = p;

    ui->cpuMinSpinBox->setValue(p->getCpuMin());
    ui->cpuMidSpinBox->setValue(p->getCpuMid());
    ui->cpuMaxSpinBox->setValue(p->getCpuMax());

    ui->gpuMinSpinBox->setValue(p->getGpuMin());
    ui->gpuMidSpinBox->setValue(p->getGpuMid());
    ui->gpuMaxSpinBox->setValue(p->getGpuMax());

    ui->mchMinSpinBox->setValue(p->getMchMin());
    ui->mchMidSpinBox->setValue(p->getMchMid());
    ui->mchMaxSpinBox->setValue(p->getMchMax());

    ui->tresholdSpinBox->setValue(p->getTreshold());

    this->show();
}

ProfileLineDialog::ProfileLineDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProfileLineDialog)
{
    ui->setupUi(this);

    connect(this, SIGNAL(accepted()), this, SLOT(saveText()));
}

ProfileLineDialog::~ProfileLineDialog()
{
    delete ui;
}

void ProfileLineDialog::saveText()
{
    str = ui->lineEdit->text();
    ui->lineEdit->clear();
    emit endOfInput(str);
}

QString ProfileLineDialog::getName()
{
    return str;
}

SettingsDialog::SettingsDialog(Settings &stgs, ProfileList &pfls, WLGovernor *wlgovnr, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    settings = &stgs;
    profileList = &pfls;
    wlgovernor = wlgovnr;

    ui->normalLineEdit->insert(settings->getNormColor());
    ui->warningLineEdit->insert(settings->getWarnColor());
    ui->criticalLineEdit->insert(settings->getCritColor());

    ui->warningLvlCheckBox->setChecked(wlgovernor->getWarnNotifySend());
    ui->critLvlCheckBox->setChecked(wlgovernor->getCritNotifySend());

    if (wlgovernor->getCritLvlAction() == CPROF) {
        ui->changeProfileRadioButton->setChecked(true);
        ui->shutdownComboBox->setDisabled(true);
    } else if (wlgovernor->getCritLvlAction() == SHUTD) {
        ui->perfShutdwnRadioButton->setChecked(true);
        ui->changeProfileComboBox->setDisabled(true);
    } else {
        ui->noActionRadioButton->setChecked(true);
        ui->shutdownComboBox->setDisabled(true);
        ui->changeProfileComboBox->setDisabled(true);
    }

    ui->shutdownComboBox->setCurrentIndex((int)wlgovernor->getShutdownMethod());

    for (int i = 0; i < profileList->size(); i++)
        ui->changeProfileComboBox->addItem(profileList->at(i)->getName());
    ui->changeProfileComboBox->setCurrentIndex(wlgovernor->getFallbackProfile());

    connect(this, SIGNAL(accepted()), this, SLOT(setValues()));

    this->show();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::setValues()
{
    settings->setNormColor(ui->normalLineEdit->text());
    settings->setWarnColor(ui->warningLineEdit->text());
    settings->setCritColor(ui->criticalLineEdit->text());

    wlgovernor->setWarnNotifySend(ui->warningLvlCheckBox->isChecked());
    wlgovernor->setCritNotifySend(ui->critLvlCheckBox->isChecked());

    if (ui->changeProfileRadioButton->isChecked())
        wlgovernor->setCritLvlAction(CPROF);
    else if (ui->perfShutdwnRadioButton->isChecked())
        wlgovernor->setCritLvlAction(SHUTD);
    else
        wlgovernor->setCritLvlAction(NONE);

    wlgovernor->setFallbackProfile(ui->changeProfileComboBox->currentIndex());
    wlgovernor->setShutdownMethod((shutdownMethodT)ui->shutdownComboBox->currentIndex());
}

TrackPointDialog::TrackPointDialog(TrackPoint *tp, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TrackPointDialog)
{
    ui->setupUi(this);

    tpDev = tp;

    ui->pressToSelect->setChecked(tpDev->getPressToSelectState());
    ui->middleBtnEmu->setChecked(tpDev->getMiddleBtnState());
    ui->scrolling->setChecked(tpDev->getScrollingState());
    ui->speed->setValue(tpDev->getCurSpeed());
    ui->sensitivity->setValue(tpDev->getCurSensitivity());
    ui->inertiaSpinBox->setValue(tpDev->getCurInertia());

    connect(this, SIGNAL(accepted()), this, SLOT(setValues()));

    this->show();
}

TrackPointDialog::~TrackPointDialog()
{
    delete ui;
}

void TrackPointDialog::setValues()
{
    tpDev->setPressToSelectEnabled(ui->pressToSelect->isChecked());
    tpDev->setMiddleBtnEnabled(ui->middleBtnEmu->isChecked());
    tpDev->setScrollingEnabled(ui->scrolling->isChecked());
    tpDev->setSpeed(ui->speed->value());
    tpDev->setSensitivity(ui->sensitivity->value());
    tpDev->setInertia(ui->inertiaSpinBox->value());
}

TouchPadDialog::TouchPadDialog(TouchPad *tp, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TouchPadDialog)
{
    ui->setupUi(this);

    tpDev = tp;

    ui->twoFingerVertScrollingChkBox->setChecked(tpDev->getTwoFingetVertScrolling());
    ui->edgeVertScrollingChkBox->setChecked(tpDev->getEdgeVertScrollingState());
    ui->vertScrollingSlider->setValue(tpDev->getVertScrollingSpeed());
    ui->twoFingerHorizScrollingChkBox->setChecked(tpDev->getTwoFingerHorizScrolling());
    ui->edgeHorizScrollingChkBox->setChecked(tpDev->getEdgeHorizScrollingState());
    ui->horizScrollingSlider->setValue(tpDev->getHorizScrollingSpeed());

    ui->edgeCoastingChkBox->setChecked(tpDev->getEdgeCoastingState());
    ui->coastingAccelSlider->setValue(tpDev->getCoastingAccel());
    ui->coastingDecelSlider->setValue(tpDev->getCoastingDecel());

    connect(this, SIGNAL(accepted()), this, SLOT(setValues()));

    this->show();
}

TouchPadDialog::~TouchPadDialog()
{
    delete ui;
}

void TouchPadDialog::setValues()
{
    tpDev->setTwoFingerVertScrolling(ui->twoFingerVertScrollingChkBox->isChecked());
    tpDev->setEdgeVertScrolling(ui->edgeVertScrollingChkBox->isChecked());
    tpDev->setVertScrollingSpeed(ui->vertScrollingSlider->value());
    tpDev->setTwoFingerHorizScrolling(ui->twoFingerHorizScrollingChkBox->isChecked());
    tpDev->setEdgeHorizScrolling(ui->edgeHorizScrollingChkBox->isChecked());
    tpDev->setHorizScrollingSpeed(ui->horizScrollingSlider->value());

    tpDev->setEdgeCoasting(ui->edgeCoastingChkBox->isChecked());
    tpDev->setCoastingAccel(ui->coastingAccelSlider->value());
    tpDev->setCoastingDecel(ui->coastingDecelSlider->value());
}
