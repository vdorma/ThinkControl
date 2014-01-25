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



#ifndef DEVICES_H
#define DEVICES_H

#include <QFile>
#include <QList>
#include <QTextStream>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QObject>
#include <QDebug>
#include <QSettings>
#include <QFileSystemWatcher>


class Timer : public QObject {

    Q_OBJECT

public:
    Timer();

    void setTimerInterval(int i);

signals:
    void timeout();

private slots:
    void timeIsOut();

private:
    QTimer t;
};

/* Sensor class represents each thinkpad sensor */

class Sensor {
public:
    Sensor(QStringList *, int, int);
    QString getTemp();
    int getCritTemp();

private:
    int critTemp;               // critical temperature of device
    int positionNum;            // calculated position in thermal file
    QStringList *valuesPtr;
};


/*
 * Cpu class represent interface for acquaring core temperatures
 * and frequency controlling.
 */

class Cpu {
public:
    Cpu();
    int getAvailCores();
    QStringList getAvailFreq();
    int getTemp();
    int getCritTemp();
    int getCurFreq();
    QString getCurGovernor();

    void setFreq(int);
    void setGovernor(QString);

private:
    int availCores;
    int critTemp;
    QList<QFile*> file;
    QList<QTextStream*> txtStr;
};

/* Gpu interface */
class AbstractGpu {
public:
    virtual QStringList getProfiles() = 0;
    virtual int getCurProfile() = 0;
    virtual void setProfile(int p) = 0;
};

/* Radeon driver class */

class Radeon : public AbstractGpu {
public:
    Radeon();

    QStringList getProfiles();
    int getCurProfile();
    void setProfile(int p);

private:
    QStringList profilesList;
};

/* Nouveau driver class */

class Nvidia : public AbstractGpu {
public:
    Nvidia();

    QStringList getProfiles();
    int getCurProfile();
    void setProfile(int p);

private:
    QStringList profilesList;
};

/*
 * Gpu class represent interface for acquaring gpu temperature
 * and changing driver profiles
 */

class Gpu : public Sensor {
public:
    Gpu(QStringList *sLst, int pNum, int cTmp) : Sensor(sLst, pNum, cTmp) {

        QFile f("/proc/modules");                       // check for module presence
        QTextStream ts(&f);
        f.open(QIODevice::ReadOnly);

        if (ts.readAll().contains("radeon")) {
            gpu = new Radeon();
            present = true;
        } else if (ts.readAll().contains("nouveau")) {
//            gpu = new Nvidia();
            present = true;
        } else
            present = false;

        f.close();
    }

    ~Gpu();

//    QString getCurMethod();
//    QString getCurProfile();
    int getCurProfile();
//    QStringList getMethodsLst();
    QStringList getProfiles();
    bool isPresent();
//    void setMethod(QString);
//    void setProfile(QString);
    void setProfile(int p);

private:
    bool present;

    AbstractGpu *gpu;
};

class Battery : public QObject {
    Q_OBJECT

public:
    Battery(int batNum);

    bool isInstalled(void);
    bool isACConnected(void);

    QString getState(void);
    int getChargeLvl(void);
    int getRemainingRunningTime(void);
    int getChargingTime(void);
    int getCyclesCount(void);
    int getCurrentVoltage(void);
    int getRemainingCapacity(void);
    QString getFirstUseDate(void);

    QString getManufacturer(void);
    QString getModel(void);
    QString getType(void);
    int getDesignCapacity(void);
    int getDesignVoltage(void);
    QString getManufactureDate(void);

    int getStartChargingTreshold(void);
    int getStopChargingTreshold(void);
    void setStartChargingTreshold(int n);
    void setStopChargingTreshold(int n);

private:
    bool powerConnected;
    int startChargeTreshold;
    int stopChargeTreshold;
    QString batPath;

    QSettings *settings;
};

/*
 * SensorsArray represents all thinkpad sensors and interface
 * for updating their thermal values.
 */

class SensorsArray : public QObject
{
    Q_OBJECT

public:
    SensorsArray();

    Cpu *cpu;
    Gpu *gpu;
    Sensor *mch;
    Sensor *ich;
    Sensor *aps;
    Sensor *pwr;
    Sensor *pcmcia;
    Sensor *mainBatFirst;
    Sensor *mainBatSecond;
    Sensor *bayBatFirst;
    Sensor *bayBatSecond;

private:
    QFile thermalSrc;
    QTextStream stream;
    QStringList values;

public slots:
    void updateThermValues();

signals:
    void thermalValuesUpdated();
};

/* Fan represents interface for controlling fan speed */

class Fan {
public:
    Fan();

    QString getLevel();
    QString getStatus();
    int getSpeed();
    void setLevel(int);
    void setLevelAuto();
    void setFullSpeed();
    void setFanOff();

private:
    QFile fanSrc;
    QTextStream stream;
};

/* Interface for controlling wireless device */

class WirelessDevice {
public:
    WirelessDevice(QString);
    int getState();
    void turnOff();
    void turnOn();

private:
    QString fileName;
};

/* Wireless devices that can be found in thinkpad */

class WirelessSwitchers {
public:
    WirelessSwitchers();

    WirelessDevice *wan;
    WirelessDevice *uwb;
    WirelessDevice *bt;
};

/*
 * Get information about machine, Type-model value
 * and BIOS version.
 */

class MachineInfo {
public:
    MachineInfo();

    QString getType();
    QString getModel();
    float getBiosVersion();

private:
    QString type, model;
    float biosVer;
};

/* IBM TrackPoint configuration */

class TrackPoint {
public:
    TrackPoint();
    ~TrackPoint();

    bool getState();
    bool getScrollingState();
    bool getPressToSelectState();
    bool getMiddleBtnState();
    int getCurSpeed();
    int getCurSensitivity();
    int getCurInertia();

    void setEnabled(bool st);
    void setScrollingEnabled(bool st);
    void setPressToSelectEnabled(bool st);
    void setMiddleBtnEnabled(bool st);
    void setSpeed(int n);
    void setSensitivity(int n);
    void setInertia(int n);

    void applySettings();               // Apply settings after resume from suspend

private:
    long int devID;                     // Bug in including order. So we can't use XID type
    bool devPresented;
    int devEnabled;
    bool scrollingState;
    bool pressToSelectState;
    bool middleBtnState;

    int speed;
    int sensitivity;
    int inertia;
    QSettings *settings;

    int getSpeedValue();
    int getSensitivityValue();
};

/* TouchPad configuration */

class TouchPad {
public:
    TouchPad();
    ~TouchPad();

    bool isPresent();

    bool getState();
    bool getTwoFingetVertScrolling();
    bool getTwoFingerHorizScrolling();
    int getVertScrollingSpeed();
    int getHorizScrollingSpeed();
    bool getEdgeVertScrollingState();
    bool getEdgeHorizScrollingState();
    bool getEdgeCoastingState();
    int getCoastingAccel();
    int getCoastingDecel();

    void setEnabled(bool st);
    void setTwoFingerVertScrolling(bool st);
    void setTwoFingerHorizScrolling(bool st);
    void setVertScrollingSpeed(int n);
    void setHorizScrollingSpeed(int n);
    void setEdgeVertScrolling(bool st);
    void setEdgeHorizScrolling(bool st);
    void setEdgeCoasting(bool st);
    void setCoastingAccel(int n);
    void setCoastingDecel(int n);

    void applySettings();

private:
    long int devID;
    bool devPresented;
    int devEnabled;

    int twoFingerVertScrolling;
    int twoFingerHorizScrolling;
    int vertScrollingSpeed;
    int horizScrollingSpeed;
    int edgeVertScrolling;
    int edgeHorizScrolling;
    int edgeCoasting;

    float coastingAccel;
    float coastingDecel;


    QSettings *settings;
};

bool isSettingsExists(const QSettings *s);

int getIntValueFromFile(QString path);
void setIntValueToFile(QString path, int val);
QString getStringValueFromFile(QString path);

#endif // DEVICES_H
