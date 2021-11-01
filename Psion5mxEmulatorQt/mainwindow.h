/* Code modified version Copyright (c) 2021 Philippe Cavenel
 *
 * This Source Code Form is subject to the terms of the
 * GNU GENERAL PUBLIC LICENSE Version 2, June 1991.
 *
 * The Psion-specific code is copyright (c) 2019 Ash Wolf.
 * The ARM disassembly code is a modified version of the one used in mGBA by endrift.
 * WindEmu is available under the Mozilla Public License 2.0.
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QElapsedTimer>
#include "../Psion5mxEmulatorCore/emubase.h"
#include "pdascreenwindow.h"
#include <QThread>


class Engine : public QThread
{
    Q_OBJECT
};

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
	explicit MainWindow(EmuBase *emu, QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void execTimer_1();
    void execTimer_2();


private:
	QElapsedTimer elapsedTimer;
    Ui::MainWindow *ui;
	PDAScreenWindow pdaScreen;
	EmuBase *emu;
    QTimer *timer_1;
    QTimer *timer_2;

};

#endif // MAINWINDOW_H
