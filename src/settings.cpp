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



#include "h/settings.h"
#include <QDebug>


QString Profile::getName() { return name; }
int Profile::getCpuMin() { return cpuMin; }
int Profile::getCpuMid() { return cpuMid; }
int Profile::getCpuMax() { return cpuMax; }
int Profile::getGpuMin() { return gpuMin; }
int Profile::getGpuMid() { return gpuMid; }
int Profile::getGpuMax() { return gpuMax; }
int Profile::getMchMin() { return mchMin; }
int Profile::getMchMid() { return mchMid; }
int Profile::getMchMax() { return mchMax; }
int Profile::getTreshold() { return treshold; }
int Profile::getGpuMethod() { return gpuMethod; }
int Profile::getGpuProfile() { return gpuProfile; }
int Profile::getCpuPolicy() { return cpuPolicy; }

void Profile::setName(QString s) { name = s; }
void Profile::setCpuMin(int n) { cpuMin = n; }
void Profile::setCpuMid(int n) { cpuMid = n; }
void Profile::setCpuMax(int n) { cpuMax = n; }
void Profile::setGpuMin(int n) { gpuMin = n; }
void Profile::setGpuMid(int n) { gpuMid = n; }
void Profile::setGpuMax(int n) { gpuMax = n; }
void Profile::setMchMin(int n) { mchMin = n; }
void Profile::setMchMid(int n) { mchMid = n; }
void Profile::setMchMax(int n) { mchMax = n; }
void Profile::setTreshold(int n) { treshold = n; }
void Profile::setGpuMethod(int n) { gpuMethod = n; }
void Profile::setGpuProfile(int n) { gpuProfile = n; }
void Profile::setCpuPolicy(int n) { cpuPolicy = n; }

ProfileList::ProfileList()
{
    settings = new QSettings("thinkctl", "profiles");
}

ProfileList::~ProfileList()
{
    delete settings;
}

int ProfileList::getCurrentProfile() { return currentProfile; }
void ProfileList::setCurrentProfile(int n) { currentProfile = n; }

bool ProfileList::isSettingsExists()
{
    QFile f(settings->fileName());

    return f.exists();
}

void ProfileList::addProfile(QString name)
{
    Profile *profile = new Profile;

    profile->setName(name);

    profile->setCpuMin(50);
    profile->setCpuMid(60);
    profile->setCpuMax(70);

    profile->setGpuMin(60);
    profile->setGpuMid(70);
    profile->setGpuMax(80);

    profile->setMchMin(50);
    profile->setMchMid(60);
    profile->setMchMax(70);

    profile->setTreshold(3);
    profile->setCpuPolicy(0);
    profile->setGpuMethod(1);
    profile->setGpuProfile(3);

    this->append(profile);
}

void ProfileList::deleteProfile(int num)
{
    delete this->at(num);
    this->removeAt(num);
}

void ProfileList::addInitialProfiles()
{
    currentProfile = 0;

    Profile *performance = new Profile;

    performance->setName("Performance");

    performance->setCpuMin(50);
    performance->setCpuMid(60);
    performance->setCpuMax(70);
    performance->setGpuMin(60);
    performance->setGpuMid(70);
    performance->setGpuMax(80);
    performance->setMchMin(50);
    performance->setMchMid(60);
    performance->setMchMax(70);
    performance->setTreshold(3);

    performance->setCpuPolicy(0);
    performance->setGpuMethod(1);
    performance->setGpuProfile(3);


    Profile *silent = new Profile;

    silent->setName("Silent");

    silent->setCpuMin(65);
    silent->setCpuMid(75);
    silent->setCpuMax(85);
    silent->setGpuMin(65);
    silent->setGpuMid(75);
    silent->setGpuMax(85);
    silent->setMchMin(65);
    silent->setMchMid(75);
    silent->setMchMax(85);
    silent->setTreshold(3);

    silent->setCpuPolicy(3);
    silent->setGpuMethod(1);
    silent->setGpuProfile(2);

    this->append(performance);
    this->append(silent);
}

void ProfileList::loadProfiles()
{
    currentProfile = settings->value("selected_profile").toInt();

    settings->beginGroup("Profiles");
    int size = settings->beginReadArray("profile");
    for (int i = 0; i < size; i++) {
        settings->setArrayIndex(i);
        Profile *p = new Profile;
        p->setName(settings->value("name").toString());

        p->setCpuMin(settings->value("cpu_min").toInt());
        p->setCpuMid(settings->value("cpu_mid").toInt());
        p->setCpuMax(settings->value("cpu_max").toInt());
        p->setGpuMin(settings->value("gpu_min").toInt());
        p->setGpuMid(settings->value("gpu_mid").toInt());
        p->setGpuMax(settings->value("gpu_max").toInt());
        p->setMchMin(settings->value("mch_min").toInt());
        p->setMchMid(settings->value("mch_mid").toInt());
        p->setMchMax(settings->value("mch_max").toInt());
        p->setTreshold(settings->value("treshold").toInt());

        p->setCpuPolicy(settings->value("cpu_policy").toInt());
        p->setGpuMethod(settings->value("gpu_method").toInt());
        p->setGpuProfile(settings->value("gpu_profile").toInt());

        this->append(p);
    }

    settings->endArray();
    settings->endGroup();
}

void ProfileList::saveProfiles()
{
    settings->setValue("selected_profile", currentProfile);

    settings->beginGroup("Profiles");
    settings->beginWriteArray("profile");
    for (int i = 0; i < this->size(); i++) {
        settings->setArrayIndex(i);

        settings->setValue("name", this->at(i)->getName());

        settings->setValue("cpu_min", this->at(i)->getCpuMin());
        settings->setValue("cpu_mid", this->at(i)->getCpuMid());
        settings->setValue("cpu_max", this->at(i)->getCpuMax());
        settings->setValue("gpu_min", this->at(i)->getGpuMin());
        settings->setValue("gpu_mid", this->at(i)->getGpuMid());
        settings->setValue("gpu_max", this->at(i)->getGpuMax());
        settings->setValue("mch_min", this->at(i)->getMchMin());
        settings->setValue("mch_mid", this->at(i)->getMchMid());
        settings->setValue("mch_max", this->at(i)->getMchMax());
        settings->setValue("treshold", this->at(i)->getTreshold());

        settings->setValue("cpu_policy", this->at(i)->getCpuPolicy());
        settings->setValue("gpu_method", this->at(i)->getGpuMethod());
        settings->setValue("gpu_profile", this->at(i)->getGpuProfile());
    }

    settings->endArray();
    settings->endGroup();
}

Settings::Settings()
{
    settings = new QSettings("thinkctl", "settings");

    if (isSettingsExists())
        loadSettings();
    else
        addInitialSettings();
}

Settings::~Settings()
{
    saveSettings();
    delete settings;
}

void Settings::addInitialSettings()
{
    programCtrl = false;
    normColor = "green";
    warnColor = "#0000FF";
    critColor = "red";
}

void Settings::loadSettings()
{
    programCtrl = settings->value("program_controlled").toBool();
    normColor = settings->value("thermal_normal_color").toString();
    warnColor = settings->value("thermal_warning_color").toString();
    critColor = settings->value("thermal_critical_color").toString();
}

void Settings::saveSettings()
{
    settings->setValue("program_controlled", programCtrl);
    settings->setValue("thermal_normal_color", normColor);
    settings->setValue("thermal_warning_color", warnColor);
    settings->setValue("thermal_critical_color", critColor);
}

bool Settings::isSettingsExists()
{
    QFile f(settings->fileName());

    return f.exists();
}

bool Settings::isProgramControlled() { return programCtrl; }
bool Settings::isWirelessPersistant() { return wirelessDevPersist; }
QString Settings::getNormColor() { return normColor; }
QString Settings::getWarnColor() { return warnColor; }
QString Settings::getCritColor() { return critColor; }

void Settings::setProgramControlled(bool st) { programCtrl = st; }
void Settings::setWirelessPersistant(bool st) { wirelessDevPersist = st; }
void Settings::setNormColor(QString col) { normColor = col; }
void Settings::setWarnColor(QString col) { warnColor = col; }
void Settings::setCritColor(QString col) { critColor = col; }
