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



#ifndef DIALOGS_H
#define DIALOGS_H

#include <QDialog>
#include "settings.h"
#include "devices.h"
#include "governors.h"

namespace Ui {
    class FanPresetDialog;
    class ProfileLineDialog;
    class SettingsDialog;
    class TrackPointDialog;
    class TouchPadDialog;
}

class FanPresetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FanPresetDialog(QWidget *parent = 0);
    ~FanPresetDialog();

    void showDialog(Profile *);

private slots:
    void setValues();

private:
    Ui::FanPresetDialog *ui;
    Profile *pf;
};

class ProfileLineDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProfileLineDialog(QWidget *parent = 0);
    ~ProfileLineDialog();

    QString getName();

private slots:
    void saveText();

private:
    Ui::ProfileLineDialog *ui;
    QString str;

signals:
    void endOfInput(QString);
};

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(Settings &stgs, ProfileList &pfls, WLGovernor *wlgovnr, QWidget *parent = 0);
    ~SettingsDialog();

    void showDialog(Settings &stgs, ProfileList &pfls, WLGovernor *wlgovnr);

private slots:
    void setValues();

private:
    Ui::SettingsDialog *ui;
    Settings *settings;
    ProfileList *profileList;
    WLGovernor *wlgovernor;
};

class TrackPointDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TrackPointDialog(TrackPoint *tp, QWidget *parent = 0);
    ~TrackPointDialog();

    void showDialog(TrackPoint *tpd);

private slots:
    void setValues();

private:
    Ui::TrackPointDialog *ui;
    TrackPoint *tpDev;
};

class TouchPadDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TouchPadDialog(TouchPad *tp, QWidget *parent = 0);
    ~TouchPadDialog();

private slots:
    void setValues();

private:
    Ui::TouchPadDialog *ui;
    TouchPad *tpDev;
};

#endif // DIALOGS_H
