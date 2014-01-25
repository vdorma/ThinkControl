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



#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>
#include <QSettings>
#include <QFile>

class Profile {
public:
    QString getName();
    int getCpuMin();
    int getCpuMid();
    int getCpuMax();
    int getGpuMin();
    int getGpuMid();
    int getGpuMax();
    int getMchMin();
    int getMchMid();
    int getMchMax();
    int getTreshold();
    int getGpuMethod();
    int getGpuProfile();
    int getCpuPolicy();

    void setName(QString);
    void setCpuMin(int);
    void setCpuMid(int);
    void setCpuMax(int);
    void setGpuMin(int);
    void setGpuMid(int);
    void setGpuMax(int);
    void setMchMin(int);
    void setMchMid(int);
    void setMchMax(int);
    void setTreshold(int);

    void setGpuMethod(int);
    void setGpuProfile(int);
    void setCpuPolicy(int);

private:
    QString name;
    int cpuMin, cpuMid, cpuMax;
    int gpuMin, gpuMid, gpuMax;
    int mchMin, mchMid, mchMax;
    int treshold;
    int gpuMethod, gpuProfile;
    int cpuPolicy;
};

class ProfileList : public QList<Profile*> {
public:
    ProfileList();
    ~ProfileList();

    void loadProfiles();
    void saveProfiles();
    void addInitialProfiles();

    int getCurrentProfile();
    void setCurrentProfile(int);

    void addProfile(QString);
    void deleteProfile(int);

    bool isSettingsExists();
private:
    int currentProfile;
    QSettings *settings;
};

class Settings {
public:
    Settings();
    ~Settings();

    void addInitialSettings();
    void saveSettings();
    void loadSettings();
    bool isSettingsExists();

    bool isProgramControlled();
    bool isWirelessPersistant();

    QString getNormColor();
    QString getWarnColor();
    QString getCritColor();

    void setProgramControlled(bool st);
    void setWirelessPersistant(bool st);

    void setNormColor(QString col);
    void setWarnColor(QString col);
    void setCritColor(QString col);

private:
    QSettings *settings;

    bool programCtrl;
    bool wirelessDevPersist;
    QString normColor;
    QString warnColor;
    QString critColor;
};

#endif // SETTINGS_H
