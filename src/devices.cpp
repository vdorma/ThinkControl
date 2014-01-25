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



#include <h/devices.h>

/* Block of sensor places in /proc/acpi/ibm/thermal */

#define CPU 1
#define GPU 4
#define MCH 9
#define ICH 10
#define APS 2
#define PWR 11
#define PCM 3
#define MFB 5
#define MSB 7
#define BFB 6
#define BSB 8

/* Block of hardware critical temperature */

#define CPU_CT 100
#define GPU_CT 100
#define MCH_CT 100      // as of http://www.intel.com/assets/pdf/designguide/308643.pdf
#define ICH_CT 100
#define APS_CT 60
#define PWR_CT 9999
#define PCM_CT 9999
#define MB_CT 60        // http://www.nfpa.org/assets/files/pdf/research/rflithiumionbatterieshazard.pdf
#define BB_CT 60        // http://industrial.panasonic.com/www-data/pdf/ACI4000/ACI4000PE5.pdf



#define CORES_THERM_PATH "/sys/class/thermal/thermal_zone"
#define THINKPAD_ACPI_THERMAL_PATH "/proc/acpi/ibm/thermal"
#define THINKPAD_ACPI_FAN_PATH "/proc/acpi/ibm/fan"

#define CPU_PATH "/sys/devices/system/cpu/cpu"
#define CPU_AVAIL_CORES_PATH "/sys/devices/system/cpu/present"
#define CPU_AVAIL_FREQ_PATH "/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies"
#define CPU_CUR_FREQ_PATH "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq"
#define CPU_CUR_GOVNR_PATH "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"

#define GPU_METHOD_PATH "/sys/class/drm/card0/device/power_method"
#define GPU_PROFILE_PATH "/sys/class/drm/card0/device/power_profile"

#define THINKPAD_WAN_PATH "/proc/acpi/ibm/wan"
#define THINKPAD_UWB_PATH "/proc/acpi/ibm/uwb"
#define THINKPAD_BT_PATH "/proc/acpi/ibm/bluetooth"


#include <X11/Xatom.h>
#include <X11/extensions/XInput.h>

void setXDevProp(unsigned long dev, char *propty, int val, int val2 = -1, int val3 = -1, int val4 = -1);
void setXDevProp32(unsigned long dev, char *propty, int fmt, int val, int val2 = -1);
void setXdevPropFloat(unsigned long dev, char *propty, float val, float val2 = -1);
long int findDeviceId(char *devname);



Timer::Timer()
{
    connect(&t, SIGNAL(timeout()), this, SLOT(timeIsOut()));
    t.start(1000);
}

void Timer::setTimerInterval(int i)
{
    t.setInterval(i);
}

void Timer::timeIsOut()
{
    emit timeout();
}

Sensor::Sensor(QStringList *vPtr, int p, int c)
{
    valuesPtr = vPtr;
    positionNum = p;
    critTemp = c;
}

QString Sensor::getTemp()
{
    QString s = valuesPtr->value(positionNum-1);

    if (s == "-128")
        return "none";
    else
        return s;
}

int Sensor::getCritTemp()
{
    return critTemp;
}

SensorsArray::SensorsArray()
{
    thermalSrc.setFileName(THINKPAD_ACPI_THERMAL_PATH);
    thermalSrc.open(QIODevice::ReadOnly);
    stream.setDevice(&thermalSrc);
    values = stream.readLine().split(' ');

    cpu = new Cpu();
    gpu = new Gpu(&values, GPU, GPU_CT);
    mch = new Sensor(&values, MCH, MCH_CT);
    ich = new Sensor(&values, ICH, ICH_CT);
    aps = new Sensor(&values, APS, APS_CT);
    pwr = new Sensor(&values, PWR, PWR_CT);
    pcmcia = new Sensor(&values, PCM, PCM_CT);
    mainBatFirst = new Sensor(&values, MFB, MB_CT);
    mainBatSecond = new Sensor(&values, MSB, MB_CT);
    bayBatFirst = new Sensor(&values, BFB, BB_CT);
    bayBatSecond = new Sensor(&values, BSB, BB_CT);
}

void SensorsArray::updateThermValues()
{
    stream.seek(0);
    values = stream.readLine().split(' ');

    emit thermalValuesUpdated();
}

Fan::Fan()
{
    fanSrc.setFileName(THINKPAD_ACPI_FAN_PATH);
    if (fanSrc.open(QIODevice::ReadWrite) == false) {
        qErrnoWarning("Cannot open fan for writing.");
        fanSrc.open(QIODevice::ReadOnly);
    }
    stream.setDevice(&fanSrc);
}

QString Fan::getLevel()
{
    stream.seek(0);
    for (int i = 0; i < 2; i++)
        stream.readLine();

    return stream.readLine().section(':', 1).trimmed();
}

int Fan::getSpeed()
{
    stream.seek(0);
    stream.readLine();

    return stream.readLine().section(':', 1).trimmed().toInt();
}

QString Fan::getStatus()
{
    stream.seek(0);

    return stream.readLine().section(':', 1).trimmed();
}

void Fan::setLevel(int l)           // To-do: add check for module fan parametr
{
    QString s = "level ";
    s = s+=QString::number(l);
    stream << s;
}

void Fan::setLevelAuto()
{
    stream << "level auto";
}

void Fan::setFullSpeed()
{
    stream << "level full-speed";
}

void Fan::setFanOff()
{
    setLevel(0);
}

Cpu::Cpu()
{
    critTemp = CPU_CT;
    availCores = this->getAvailCores();

    for (int i = 0; i < availCores; i++) {
        QString str;
        str.append(CORES_THERM_PATH).append(QString::number(i)).append("/temp");

        QFile *f = new QFile(str);
        f->open(QIODevice::ReadOnly);
        file.append(f);
    }
}

int Cpu::getAvailCores()
{
    QFile f(CPU_AVAIL_CORES_PATH);
    QTextStream ts(&f);
    QStringList strLst;

    f.open(QIODevice::ReadOnly);
    strLst = ts.readLine().split('-');

    return strLst.size();
}

QStringList Cpu::getAvailFreq()
{
    QFile f(CPU_AVAIL_FREQ_PATH);
    QTextStream ts(&f);
    QStringList sl;

    f.open(QIODevice::ReadOnly);
    sl = ts.readLine().split(' ', QString::SkipEmptyParts);

    return sl;
}

int Cpu::getTemp()
{
    int sum = 0;
    int average;

    for (int i = 0; i < availCores; i++) {
        QTextStream ts(file.at(i));
        sum += ts.readLine().toInt();
        ts.seek(0);
    }

    average = sum/availCores/1000;             // divide on 1000 for delete zeroes in the end

    return average;
}

int Cpu::getCritTemp()
{
    return critTemp;
}

int Cpu::getCurFreq()
{
    QFile f(CPU_CUR_FREQ_PATH);
    QTextStream ts(&f);

    f.open(QIODevice::ReadOnly);

    return ts.readLine().toInt();
}

QString Cpu::getCurGovernor()
{
    QFile f(CPU_CUR_GOVNR_PATH);
    QTextStream ts(&f);

    f.open(QIODevice::ReadOnly);

    return ts.readLine();
}

void Cpu::setGovernor(QString s)
{
    QString pStr;

    for (int i = 0; i < availCores; i++) {
        pStr.append(CPU_PATH).append(QString::number(i)).append("/cpufreq/scaling_governor");
        QFile *f = new QFile(pStr);
        QTextStream *ts = new QTextStream(f);

        if (f->open(QIODevice::WriteOnly) == false)
            qDebug() << "Cannot write to cpu scaling governor files.";
        else
            *ts << s;

        f->close();
        delete ts;
        delete f;
        pStr.clear();
    }
}

void Cpu::setFreq(int fq)
{
    QString pStr;

    if (getCurGovernor() != "userspace")
        setGovernor("userspace");

    for (int i = 0; i < availCores; i++) {
        pStr.append(CPU_PATH).append(QString::number(i)).append("/cpufreq/scaling_setspeed");
        QFile *f = new QFile(pStr);
        QTextStream *ts = new QTextStream(f);

        if (f->open(QIODevice::WriteOnly) == false)
            qDebug() << "Cannot write to cpu frequency set files";
        else
            *ts << fq;

        f->close();
        delete ts;
        delete f;
        pStr.clear();
    }
}

Radeon::Radeon()
{
    profilesList = (QStringList() << "auto" << "low" << "mid" << "high");
}

QStringList Radeon::getProfiles()
{
    return profilesList;
}

int Radeon::getCurProfile()
{
    QFile f(GPU_METHOD_PATH);
    QTextStream ts(&f);

    f.open(QIODevice::ReadOnly);

    return profilesList.indexOf(ts.readLine());
}

void Radeon::setProfile(int p)
{
    QFile f(GPU_PROFILE_PATH);
    QTextStream ts(&f);

    if (f.open(QIODevice::WriteOnly) == false)
        qDebug() << "Cannot write to gpu profile file";
    else {
        ts << profilesList.at(p);

        f.close();
    }
}
/*
QStringList Gpu::getMethodsLst()
{
    return methodsLst;
}
*/

Gpu::~Gpu()
{
    delete gpu;
}

QStringList Gpu::getProfiles()
{
    return gpu->getProfiles();
}

bool Gpu::isPresent() { return present; }
/*
QString Gpu::getCurMethod()
{
    QFile f(GPU_METHOD_PATH);
    QTextStream ts(&f);

    f.open(QIODevice::ReadOnly);

    return ts.readLine();
}
*/
/*
QString Gpu::getCurProfile()
{
    QFile f(GPU_PROFILE_PATH);
    QTextStream ts(&f);

    f.open(QIODevice::ReadOnly);

    return ts.readLine();
}
*/

int Gpu::getCurProfile()
{
    return gpu->getCurProfile();
}
/*
void Gpu::setMethod(QString s)
{
    QFile f(GPU_METHOD_PATH);
    QTextStream ts(&f);

    if (f.open(QIODevice::WriteOnly) == false)
        qDebug() << "Cannot write to gpu method file";
    else {
        ts << s;

        f.close();
    }
}
*/
/*
void Gpu::setProfile(QString s)
{
    QFile f(GPU_PROFILE_PATH);
    QTextStream ts(&f);

    if (f.open(QIODevice::WriteOnly) == false)
        qDebug() << "Cannot write to gpu profile file";
    else {
        ts << s;

        f.close();
    }
}
*/
void Gpu::setProfile(int p)
{
    gpu->setProfile(p);
}

/* Battery class implementation */

Battery::Battery(int batNum)
{
    batPath.append("/sys/devices/platform/smapi/BAT").append(QString::number(batNum)).append("/");
}

bool Battery::isInstalled()
{
    return (bool)getIntValueFromFile(QString(batPath).append("installed"));
}

bool Battery::isACConnected()
{
    return (bool)getIntValueFromFile("/sys/devices/platform/smapi/ac_connected");
}

QString Battery::getState()
{
    return getStringValueFromFile(QString(batPath).append("state"));
}

int Battery::getChargeLvl()
{
    return getIntValueFromFile(QString(batPath).append("remaining_percent"));
}

int Battery::getRemainingRunningTime()
{
    return getIntValueFromFile(QString(batPath).append("remaining_running_time"));
}

int Battery::getChargingTime()
{
    return getIntValueFromFile(QString(batPath).append("remaining_charging_time"));
}

int Battery::getCyclesCount()
{
    return getIntValueFromFile(QString(batPath).append("cycle_count"));
}

int Battery::getCurrentVoltage()
{
    return getIntValueFromFile(QString(batPath).append("voltage"));
}

int Battery::getRemainingCapacity()
{
    return getIntValueFromFile(QString(batPath).append("remaining_capacity"));
}

QString Battery::getFirstUseDate()
{
    return getStringValueFromFile(QString(batPath).append("first_use_date"));
}

QString Battery::getManufacturer()
{
    return getStringValueFromFile(QString(batPath).append("manufacturer"));
}

QString Battery::getModel()
{
    return getStringValueFromFile(QString(batPath).append("model"));
}

QString Battery::getType()
{
    return getStringValueFromFile(QString(batPath).append("chemistry"));
}

int Battery::getDesignCapacity()
{
    return getIntValueFromFile(QString(batPath).append("design_capacity"));
}

int Battery::getDesignVoltage()
{
    return getIntValueFromFile(QString(batPath).append("design_voltage"));
}

QString Battery::getManufactureDate()
{
    return getStringValueFromFile(QString(batPath).append("manufacture_date"));
}

int Battery::getStartChargingTreshold()
{
    return getIntValueFromFile(QString(batPath).append("start_charge_thresh"));
}

int Battery::getStopChargingTreshold()
{
    return getIntValueFromFile(QString(batPath).append("stop_charge_thresh"));
}

void Battery::setStartChargingTreshold(int n)
{
    setIntValueToFile(QString(batPath).append("start_charge_thresh"), n);
}

void Battery::setStopChargingTreshold(int n)
{
    setIntValueToFile(QString(batPath).append("stop_charge_thresh"), n);
}

WirelessDevice::WirelessDevice(QString s)
{
    fileName = s;
}

int WirelessDevice::getState()
{
    QFile f(fileName);
    QTextStream ts(&f);
    QString str;

    if (!f.exists())
        return -1;
    else {
        f.open(QIODevice::ReadOnly);

        str = ts.readLine().section(':', 1).trimmed();

        if (str == "enabled")
            return 1;
        else
            return 0;

        f.close();
    }
}

void WirelessDevice::turnOff()
{
    QFile f(fileName);
    QTextStream ts(&f);

    if (f.open(QIODevice::WriteOnly) == false)
        qDebug() << "Cannot write to wireless device file";
    else {
        ts << "disable";

        f.close();
    }
}

void WirelessDevice::turnOn()
{
    QFile f(fileName);
    QTextStream ts(&f);

    if (f.open(QIODevice::WriteOnly) == false)
        qDebug() << "Cannot write to wireless device file";
    else {
        ts << "enable";

        f.close();
    }
}

WirelessSwitchers::WirelessSwitchers()
{
    wan = new WirelessDevice(THINKPAD_WAN_PATH);
    uwb = new WirelessDevice(THINKPAD_UWB_PATH);
    bt = new WirelessDevice(THINKPAD_BT_PATH);
}

MachineInfo::MachineInfo()
{
    QFile f("/var/log/messages");
    QTextStream ts(&f);
    QString line;

    type = "unknown";                   // if we can't find info in syslog file
    model = "unknown";
    biosVer = 0;

    if (f.open(QIODevice::ReadOnly) == false)
        qDebug() << "Cannot open syslog file";
    else {
        while (!f.atEnd()) {
            line = ts.readLine();

            if (line.contains("ThinkPad")) {
                ts.readLine();                                                  // skip thinkpad_acpi version string
                line = ts.readLine();
                biosVer = line.mid(line.indexOf('(') + 1, 4).toFloat();         // get position, then get four digits of version
                                                                                // +1 for skiping ( char
                line = ts.readLine();
                QString subStr = line.right(7);
                type = subStr.left(4);
                model = subStr.right(3);

                break;
            }
        }

        f.close();
    }
}

QString MachineInfo::getType() { return type; }
QString MachineInfo::getModel() { return model; }
float MachineInfo::getBiosVersion() { return biosVer; }

TrackPoint::TrackPoint()
{
    devID = findDeviceId("TPPS/2 IBM TrackPoint");

    if (devID != -1) {
        devPresented = true;

        settings = new QSettings("thinkctl", "input");

        if (isSettingsExists(settings)) {
            settings->beginGroup("TrackPoint");
            setEnabled(devEnabled = settings->value("device_enabled").toInt());
            setPressToSelectEnabled(pressToSelectState = settings->value("set_press_to_select_enabled").toBool());
            setMiddleBtnEnabled(middleBtnState = settings->value("set_middle_button_emulation_enabled").toBool());
            setScrollingEnabled(scrollingState = settings->value("set_scrolling_enabled").toBool());
            setSpeed(speed = settings->value("speed").toInt());
            setSensitivity(sensitivity = settings->value("sensitivity").toInt());
            setInertia(inertia = settings->value("inertia").toInt());
            settings->endGroup();

            applySettings();
        } else {                                                // set initial values
            setEnabled(devEnabled = true);
            setPressToSelectEnabled(pressToSelectState = false);
            setMiddleBtnEnabled(middleBtnState = false);
            setScrollingEnabled(scrollingState = false);
            setSpeed(speed = 170);
            setSensitivity(sensitivity = 150);
            setInertia(inertia = 45);
        }

    } else {
        devPresented = false;
        qDebug() << "Cannot find trackpoint device";
    }
}

TrackPoint::~TrackPoint()
{
    settings->beginGroup("TrackPoint");
    settings->setValue("device_enabled", devEnabled);
    settings->setValue("set_press_to_select_enabled", pressToSelectState);
    settings->setValue("set_middle_button_emulation_enabled", middleBtnState);
    settings->setValue("set_scrolling_enabled", scrollingState);
    settings->setValue("speed", speed);
    settings->setValue("sensitivity", sensitivity);
    settings->setValue("inertia", inertia);
    settings->endGroup();

    delete settings;
}

int TrackPoint::getSpeedValue()
{
    QFile f("/sys/devices/platform/i8042/serio1/serio2/speed");
    if (!f.exists())
        f.setFileName("/sys/devices/platform/i8042/serio1/speed");

    QTextStream ts(&f);
    f.open(QIODevice::ReadOnly);

    return ts.readLine().toInt();
}

int TrackPoint::getSensitivityValue()
{
    QFile f("/sys/devices/platform/i8042/serio1/serio2/sensitivity");
    if (!f.exists())
        f.setFileName("/sys/devices/platform/i8042/serio1/sensitivity");

    QTextStream ts(&f);
    f.open(QIODevice::ReadOnly);

    return ts.readLine().toInt();
}

bool TrackPoint::getState()
{
    return devEnabled == 1 ? true : false;
}

bool TrackPoint::getScrollingState()
{
    return scrollingState;
}

bool TrackPoint::getPressToSelectState()
{
    return pressToSelectState;
}

bool TrackPoint::getMiddleBtnState()
{
    return middleBtnState == 1 ? true : false;
}

int TrackPoint::getCurSpeed()
{
    return speed;
}

int TrackPoint::getCurSensitivity()
{
    return sensitivity;
}

int TrackPoint::getCurInertia()
{
    return inertia;
}

void TrackPoint::setEnabled(bool st)
{    
    if (st)
        devEnabled = 1;
    else
        devEnabled = 0;

    setXDevProp(devID, "Device Enabled", devEnabled);
}

void TrackPoint::setPressToSelectEnabled(bool st)
{
    QFile f("/sys/devices/platform/i8042/serio1/serio2/press_to_select");
    if (!f.exists())
        f.setFileName("/sys/devices/platform/i8042/serio1/press_to_select");

    pressToSelectState = st;

    if (f.open(QIODevice::WriteOnly)) {
        if (st)
            f.write("1");
        else
            f.write("0");
    } else {
        qDebug() << "Cannot open trackpoint press_to_select file for writing";
    }

    f.close();
}

void TrackPoint::setScrollingEnabled(bool st)
{
    if (st) {
        setXDevProp(devID, "Evdev Wheel Emulation", 1);
        setXDevProp(devID, "Evdev Wheel Emulation Button", 2);
        setXDevProp(devID, "Evdev Wheel Emulation Axes", 6, 7, 4, 5);
        scrollingState = true;
    } else {
        setXDevProp(devID, "Evdev Wheel Emulation", 0);
        scrollingState = false;
    }
}

void TrackPoint::setMiddleBtnEnabled(bool st)
{
    if (st) {
        setXDevProp(devID, "Evdev Middle Button Emulation", 1);
        middleBtnState = true;
    } else {
        setXDevProp(devID, "Evdev Middle Button Emulation", 0);
        middleBtnState = false;
    }
}

void TrackPoint::setSpeed(int n)
{
    QFile f("/sys/devices/platform/i8042/serio1/serio2/speed");
    if (!f.exists())
        f.setFileName("/sys/devices/platform/i8042/serio1/speed");

    QTextStream ts(&f);

    speed = n;

    if (f.open(QIODevice::WriteOnly))
        ts << n;
    else
        qDebug() << "Cannot open trackpoint speed file for writing";

    f.close();
}

void TrackPoint::setSensitivity(int n)
{
    QFile f("/sys/devices/platform/i8042/serio1/serio2/sensitivity");
    if (!f.exists())
        f.setFileName("/sys/devices/platform/i8042/serio1/sensitivity");

    QTextStream ts(&f);

    sensitivity = n;

    if (f.open(QIODevice::WriteOnly))
        ts << n;
    else
        qDebug() << "Cannot open trackpoint sensitivity file for writing";

    f.close();
}

void TrackPoint::setInertia(int n)
{
    setXDevProp32(devID, "Evdev Wheel Emulation Inertia", 16, n);
    inertia = n;
}

void TrackPoint::applySettings()
{
    setEnabled(devEnabled);
    setScrollingEnabled(scrollingState);
}

TouchPad::TouchPad()
{
    devID = findDeviceId("SynPS/2 Synaptics TouchPad");

    if (devID != -1) {
        devPresented = true;

        settings = new QSettings("thinkctl", "input");

        if (isSettingsExists(settings)) {
            settings->beginGroup("TouchPad");
            setEnabled(devEnabled = settings->value("device_enabled").toInt());
            setTwoFingerVertScrolling(twoFingerVertScrolling = settings->value("two_finger_vertical_scrolling_enabled").toBool());
            setEdgeVertScrolling(edgeVertScrolling = settings->value("edge_vertical_scrolling_enabled").toBool());
            setVertScrollingSpeed(vertScrollingSpeed = settings->value("vertical_scrolling_speed").toInt());
            setTwoFingerHorizScrolling(twoFingerHorizScrolling = settings->value("two_finger_horizontal_scrolling_enabled").toBool());
            setEdgeHorizScrolling(edgeHorizScrolling = settings->value("edge_horizontal_scrolling_enabled").toBool());
            setHorizScrollingSpeed(horizScrollingSpeed = settings->value("horizontal_scrolling_speed").toInt());
            setEdgeCoasting(edgeCoasting = settings->value("edge_coasting_enabled").toBool());
            setCoastingAccel(coastingAccel = settings->value("coasting_acceleration").toInt());
            setCoastingDecel(coastingDecel = settings->value("coasting_deceleration").toInt());
            settings->endGroup();
        } else {
            setEnabled(devEnabled = true);
            setTwoFingerVertScrolling(twoFingerVertScrolling = false);
            setTwoFingerHorizScrolling(twoFingerHorizScrolling = false);
            setEdgeVertScrolling(edgeVertScrolling = false);
            setEdgeHorizScrolling(edgeHorizScrolling = false);
            setVertScrollingSpeed(vertScrollingSpeed = 1);
            setHorizScrollingSpeed(horizScrollingSpeed = 1);
            setEdgeCoasting(edgeCoasting = false);
            setCoastingAccel(coastingAccel = 1);
            setCoastingDecel(coastingDecel = 1);
        }

    } else {
        devPresented = false;
        qDebug() << "Cannot find touchpad device";
    }
}

TouchPad::~TouchPad()
{
    settings->beginGroup("TouchPad");
    settings->setValue("device_enabled", devEnabled);
    settings->setValue("two_finger_vertical_scrolling_enabled", twoFingerVertScrolling);
    settings->setValue("edge_vertical_scrolling_enabled", edgeVertScrolling);
    settings->setValue("vertical_scrolling_speed", vertScrollingSpeed);
    settings->setValue("two_finger_horizontal_scrolling_enabled", twoFingerHorizScrolling);
    settings->setValue("edge_horizontal_scrolling_enabled", edgeHorizScrolling);
    settings->setValue("horizontal_scrolling_speed", horizScrollingSpeed);
    settings->setValue("edge_coasting_enabled", edgeCoasting);
    settings->setValue("coasting_acceleration", coastingAccel);
    settings->setValue("coasting_deceleration", coastingDecel);
    settings->endGroup();

    delete settings;
}

bool TouchPad::isPresent()
{
    return devPresented;
}

bool TouchPad::getState()
{
    return devEnabled == 1 ? true : false;
}

bool TouchPad::getTwoFingetVertScrolling()
{
    return twoFingerVertScrolling == 1 ? true : false;
}

bool TouchPad::getTwoFingerHorizScrolling()
{
    return twoFingerHorizScrolling == 1 ? true : false;
}

int TouchPad::getVertScrollingSpeed()
{
    return vertScrollingSpeed;
}

int TouchPad::getHorizScrollingSpeed()
{
    return horizScrollingSpeed;
}

bool TouchPad::getEdgeVertScrollingState()
{
    return edgeVertScrolling;
}

bool TouchPad::getEdgeHorizScrollingState()
{
    return edgeHorizScrolling;
}

bool TouchPad::getEdgeCoastingState()
{
    return edgeCoasting;
}

int TouchPad::getCoastingAccel()
{
    return (int)coastingAccel;
}

int TouchPad::getCoastingDecel()
{
    return (int)coastingDecel;
}

void TouchPad::setEnabled(bool st)
{
    if (st)
        devEnabled = 1;
    else
        devEnabled = 0;

    setXDevProp(devID, "Device Enabled", devEnabled);
}

void TouchPad::setTwoFingerVertScrolling(bool st)
{
    if (st)
        twoFingerVertScrolling = 1;
    else
        twoFingerVertScrolling = 0;

    setXDevProp(devID, "Synaptics Two-Finger Scrolling", twoFingerVertScrolling, twoFingerHorizScrolling);
}

void TouchPad::setTwoFingerHorizScrolling(bool st)
{
    if (st)
        twoFingerHorizScrolling = 1;
    else
        twoFingerHorizScrolling = 0;

    setXDevProp(devID, "Synaptics Two-Finger Scrolling", twoFingerVertScrolling, twoFingerHorizScrolling);
}

void TouchPad::setVertScrollingSpeed(int n)
{
    vertScrollingSpeed = n;
    setXDevProp32(devID, "Synaptics Scrolling Distance", 32, vertScrollingSpeed, horizScrollingSpeed);
}

void TouchPad::setHorizScrollingSpeed(int n)
{
    horizScrollingSpeed = n;
    setXDevProp32(devID, "Synaptics Scrolling Distance", 32, vertScrollingSpeed, horizScrollingSpeed);
}

void TouchPad::setEdgeVertScrolling(bool st)
{
    if (st)
        edgeVertScrolling = 1;
    else
        edgeVertScrolling = 0;

    setXDevProp(devID, "Synaptics Edge Scrolling", edgeVertScrolling, edgeHorizScrolling, edgeCoasting);
}

void TouchPad::setEdgeHorizScrolling(bool st)
{
    if (st)
        edgeHorizScrolling = 1;
    else
        edgeHorizScrolling = 0;

    setXDevProp(devID, "Synaptics Edge Scrolling", edgeVertScrolling, edgeHorizScrolling, edgeCoasting);
}

void TouchPad::setEdgeCoasting(bool st)
{
    if (st)
        edgeCoasting = 1;
    else
        edgeCoasting = 0;

    setXDevProp(devID, "Synaptics Edge Scrolling", edgeVertScrolling, edgeHorizScrolling, edgeCoasting);
}

void TouchPad::setCoastingAccel(int n)
{
    coastingAccel = (float)n;
    setXdevPropFloat(devID, "Synaptics Coasting Speed", coastingDecel, coastingAccel);
}

void TouchPad::setCoastingDecel(int n)
{
    coastingDecel = (float)n;
    setXdevPropFloat(devID, "Synaptics Coasting Speed", coastingDecel, coastingAccel);
}

void TouchPad::applySettings()
{
    setXDevProp(devID, "Device Enabled", devEnabled);
    setXDevProp(devID, "Synaptics Two-Finger Scrolling", twoFingerVertScrolling, twoFingerHorizScrolling);
    setXDevProp(devID, "Synaptics Scrolling Distance", vertScrollingSpeed, horizScrollingSpeed);
}

long int findDeviceId(char *devname)
{
    Display *dp;
    XID dev;
    XDeviceInfo *devs;
    int n_dev;

    dp = XOpenDisplay(NULL);
    devs = XListInputDevices(dp, &n_dev);               // get pointer to Input Devices structures

    for (int i = 0; i < n_dev; i++) {                   // find device id by name
        if (strcmp(devs[i].name, devname) == 0) {
            dev = devs[i].id;
            XCloseDisplay(dp);
            return (long int)dev;                       // change XID unsigned long to long int bc of
        }                                               // return value and bug in include order
    }

    XCloseDisplay(dp);
    return -1;                                          // device not found
}

void setXDevProp(unsigned long dev, char *propty, int val, int val2, int val3, int val4)
{
    Display *dp;
    XDevice *xdev;
    Atom prop;
    unsigned char data[4];
//    data[0] = (unsigned char)val;
    int numOfVal;

    if (val4 != -1) {
        data[0] = (unsigned char)val;
        data[1] = (unsigned char)val2;
        data[2] = (unsigned char)val3;
        data[3] = (unsigned char)val4;
        numOfVal = 4;
    } else if (val3 != -1) {
        data[0] = (unsigned char)val;
        data[1] = (unsigned char)val2;
        data[2] = (unsigned char)val3;
        numOfVal = 3;
    } else if (val2 != -1) {
        data[0] = (unsigned char)val;
        data[1] = (unsigned char)val2;
        numOfVal = 2;
    } else {
        data[0] = (unsigned char)val;
        numOfVal = 1;
    }

    dp = XOpenDisplay(NULL);
    xdev = XOpenDevice(dp, dev);
    prop = XInternAtom(dp, propty, false);

    XChangeDeviceProperty(dp, xdev, prop, XA_INTEGER, 8, PropModeReplace, data, numOfVal);
    XCloseDevice(dp, xdev);
    XCloseDisplay(dp);
}

void setXDevProp32(unsigned long dev, char *propty, int fmt, int val, int val2)
{
    Display *dp;
    XDevice *xdev;
    Atom prop;

    int data[2];
    int numOfVal;

    if (val2 != -1) {
        data[1] = val2;
        data[0] = val;
        numOfVal = 2;
    } else {
        data[0] = val;
        numOfVal = 1;
    }

    dp = XOpenDisplay(NULL);
    xdev = XOpenDevice(dp, dev);
    prop = XInternAtom(dp, propty, false);

    XChangeDeviceProperty(dp, xdev, prop, XA_INTEGER, fmt, PropModeReplace, (unsigned char*)data, numOfVal);
    XCloseDevice(dp, xdev);
    XCloseDisplay(dp);
}

void setXdevPropFloat(unsigned long dev, char *propty, float val, float val2)
{
    Display *dp;
    XDevice *xdev;
    Atom prop;
    Atom tpe;

    float data[2];
    int numOfVal;

    if (val2 != -1){
        data[1] = val2;
        data[0] = val;
        numOfVal = 2;
    } else {
        data[0] = val;
        numOfVal = 1;
    }

    dp = XOpenDisplay(NULL);
    xdev = XOpenDevice(dp, dev);
    prop = XInternAtom(dp, propty, false);
    tpe = XInternAtom(dp, "FLOAT", false);

    XChangeDeviceProperty(dp, xdev, prop, tpe, 32, PropModeReplace, (unsigned char*)data, numOfVal);
    XCloseDevice(dp, xdev);
    XCloseDisplay(dp);
}

bool isSettingsExists(const QSettings *s)
{
    QFile f(s->fileName());

    if (f.exists())
        return true;
    else
        return false;
}

int getIntValueFromFile(QString path)
{
    int out;
    QFile f(path);
    QTextStream ts(&f);

    if (f.open(QIODevice::ReadOnly))
        out = ts.readLine().toInt();
    else
        qDebug() << "Cannot open" << path;

    f.close();

    return out;
}

void setIntValueToFile(QString path, int val)
{
    QFile f(path);
    QTextStream ts(&f);

    if (f.open(QIODevice::WriteOnly))
        ts << val;
    else
        qDebug() << "Cannot write to" << path;

    f.close();
}

QString getStringValueFromFile(QString path)
{
    QString out;
    QFile f(path);
    QTextStream ts(&f);

    if (f.exists() && f.open(QIODevice::ReadOnly))
        out = ts.readLine();
    else
        qDebug() << "Cannot open" << path;

    return out;
}
