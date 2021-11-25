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
    timerMainLoop = new QTimer(this);
    timerMainLoop->setInterval(1000/64);
    connect(timerMainLoop, SIGNAL(timeout()), SLOT(execTimerMainLoop()));

    pdaScreen.show();
    timerMainLoop->start(); // To run main emulator loop

    // Timer for audio codec
    timerCodec = new QTimer(this);
    timerCodec->setInterval(10); //10ms
    connect(timerCodec, SIGNAL(timeout()), SLOT(execTimerCodec()));

    pdaScreen.show();
    timerMainLoop->start(); // To run main emulator loop
    timerCodec->start(); // To run main emulator loop

    // Simulate serial link
    emu->OpenSerialinterface();
}


MainWindow::~MainWindow()
{
    // Leave application
    delete ui;
}

void MainWindow::execTimerMainLoop()
{

    if (emu)  {
        emu->executeUntil(emu->currentCycles() + (emu->getClockSpeed() / 64));
        if (emu->m_screenRefresh) {
            pdaScreen.updateScreen();
            emu->m_screenRefresh=false;
        }      
    }
}
void MainWindow::execTimerCodec()
{

    if (emu)  {
        emu->playSound();
    }
}





