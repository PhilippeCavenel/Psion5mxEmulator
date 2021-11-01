/* Code modified version Copyright (c) 2021 Philippe Cavenel
 *
 * This Source Code Form is subject to the terms of the
 * GNU GENERAL PUBLIC LICENSE Version 2, June 1991.
 *
 * The Psion-specific code is copyright (c) 2019 Ash Wolf.
 * The ARM disassembly code is a modified version of the one used in mGBA by endrift.
 * WindEmu is available under the Mozilla Public License 2.0.
*/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QTimer>
#include <stdio.h>

MainWindow::MainWindow(EmuBase *emu, QWidget *parent) :
	pdaScreen(emu),
    emu(emu)
{

	elapsedTimer.start();
	emu->setLogger([&](const char *str) {
	});

    // Timer for main CPU loop
    timer_1 = new QTimer(this);
    timer_1->setInterval(1000/64);
    connect(timer_1, SIGNAL(timeout()), SLOT(execTimer_1()));

    // Timer for screen
    timer_2 = new QTimer(this);
    timer_2->setInterval(250); // each 0.25 s
    connect(timer_2, SIGNAL(timeout()), SLOT(execTimer_2()));

    pdaScreen.show();
    timer_1->start(); // To run main emulator loop
    timer_2->start(); // To refresh QT screen


    // Simulate serial link
    emu->OpenSerialinterface();
}


MainWindow::~MainWindow()
{
    // Leave application
    delete ui;
}

void MainWindow::execTimer_1()
{

    if (emu)  {
        emu->executeUntil(emu->currentCycles() + (emu->getClockSpeed() / 64));
    }

}
void MainWindow::execTimer_2()
{
    if (emu) {
        pdaScreen.updateScreen();
    }
}




