#include "h/sensors.h"

/* Block of sensors position in /proc/acpi/ibm/thermal */
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
#define CPU_CT 99
#define GPU_CT 99
#define MCH_CT 99       // as of http://www.intel.com/assets/pdf/designguide/308643.pdf
#define ICH_CT 99
#define APS_CT 9999
#define PWR_CT 9999
#define PCM_CT 9999
#define MB_CT 9999
#define BB_CT 9999



ThermalSource::ThermalSource()
{
    tSource = new QFile ("/proc/acpi/ibm/thermal");
    tSource->open(QIODevice::ReadOnly);
    stream = new QTextStream(tSource);
    values = new QStringList();
    *values = stream->readLine().split(' ');

//    QTimer *timer = new QTimer(this);
//    connect(timer, SIGNAL(timeout()), this, SLOT(refresh()));
//    timer->start(1000);
}

ThermalSource::~ThermalSource()
{
    delete stream;
    tSource->close();
    delete tSource;
    delete values;
}

void ThermalSource::refresh()
{
    stream->seek(0);
    *values = stream->readLine().split(' ');
    emit valuesUpdated();
}

Sensor::Sensor(QStringList *slPtr, int pN, int cT)
{
    vPtr = slPtr;
    posNumber = pN;
    critTemp = cT;
}

int Sensor::getCritTemp()
{
    return critTemp;
}

void Sensor::setCritTemp(int t)
{
    critTemp = t;
}

QString Sensor::getTemp()
{
    QString s = vPtr->value(posNumber-1);
    if (posNumber != 1)                 // skip cpu in thermal source
        if (s == "-128")
            return "none";
        else
            return s;
    else
        return s.right(2);              // read cpu value in thermal source
}

SensorsGrid::SensorsGrid(QStringList *s)
{
    cpu = new Sensor(s, CPU, CPU_CT);
    gpu = new Sensor(s, GPU, GPU_CT);
    mch = new Sensor(s, MCH, MCH_CT);
    ich = new Sensor(s, ICH, ICH_CT);
    aps = new Sensor(s, APS, APS_CT);
    pwr = new Sensor(s, PWR, PWR_CT);
    pcmcia = new Sensor(s, PCM, PCM_CT);
    mainBatFirst = new Sensor(s, MFB, MB_CT);
    mainBatSecond = new Sensor(s, MSB, MB_CT);
    bayBatFirst = new Sensor(s, BFB, BB_CT);
    bayBatSecond = new Sensor(s, BSB, BB_CT);
}

SensorsGrid::~SensorsGrid()
{
    delete cpu;
    delete gpu;
    delete mch;
    delete ich;
    delete aps;
    delete pwr;
    delete pcmcia;
    delete mainBatFirst;
    delete mainBatSecond;
    delete bayBatFirst;
    delete bayBatSecond;
}

void SensorsGrid::refresh()
{
    emit valuesChanged(this);
}
