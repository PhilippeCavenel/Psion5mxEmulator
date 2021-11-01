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
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QSysInfo>
#include <QOperatingSystemVersion>
#include "../Psion5mxEmulatorCore/windermere.h"
#include <QElapsedTimer>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
	auto args = a.arguments();
    QString romFile;
    QString ramFile;


#ifdef Q_OS_WIN64
        romFile=QString(qApp->applicationDirPath()).append(QString("/../../../Psion5mxEmulator/Psion5mxEmulatorQt/pkg_src/assets/5mx.rom"));
        ramFile=QString(qApp->applicationDirPath()).append(QString("/../../../Psion5mxEmulator/Psion5mxEmulatorQt/pkg_src/assets/5mx.context"));
#else // ANDROID
        romFile= "assets:5mx.rom";
        ramFile= "assets:5mx.context";
#endif

	// what do we have?
    QFile fRomFile(romFile);
    fRomFile.open(QFile::ReadOnly);
    auto bufferRom = fRomFile.readAll();
    fRomFile.close();

    if (bufferRom.size() < 0x400000) {
        QMessageBox::critical(nullptr, romFile, "Invalid ROM file!");
        return 0;
	}

	EmuBase *emu = nullptr;
    uint8_t *romData = (uint8_t *)bufferRom.data();

	// parse this ROM to learn what hardware it's for
	int variantFile = *((uint32_t *)&romData[0x80 + 0x4C]) & 0xFFFFFFF;
    if (variantFile < (bufferRom.size() - 8)) {
		int variantImg = *((uint32_t *)&romData[variantFile + 4]) & 0xFFFFFFF;
        if (variantImg < (bufferRom.size() - 0x70)) {
			int variant = *((uint32_t *)&romData[variantImg + 0x60]);

			if (variant == 0x7060001) {
				// 5mx ROM
				emu = new Windermere::Emulator;
            } else {
                QMessageBox::critical(nullptr, romFile, "Unrecognized ROM file!");
				return 0;
			}
		}
	}

    emu->loadROM(romData, bufferRom.size());
    MainWindow w(emu);
    int ret = a.exec();


    // Read RAM => Need to know how to update it inside the application

    QFile fRamFile(ramFile);
    if(fRamFile.open(QFile::ReadOnly)) {

        // read RAM
        auto bufferRam = fRamFile.read(emu->getRAMsizeC0());
        if (bufferRam.size() == emu->getRAMsizeC0()) {
            uint8_t *ramData = (uint8_t *)bufferRam.data();
            emu->loadRAMC0(ramData, bufferRam.size());
        } else {
            QMessageBox::critical(nullptr, romFile, "Unrecognized context file (RAM)!");
            return 0;
        }
        bufferRam = fRamFile.read(emu->getRAMsizeC1());
        if (bufferRam.size() == emu->getRAMsizeC1()) {
            uint8_t *ramData = (uint8_t *)bufferRam.data();
            emu->loadRAMC1(ramData, bufferRam.size());
        } else {
            QMessageBox::critical(nullptr, romFile, "Unrecognized context file (RAM)!");
            return 0;
        }
        bufferRam = fRamFile.read(emu->getRAMsizeD0());
        if (bufferRam.size() == emu->getRAMsizeD0()) {
            uint8_t *ramData = (uint8_t *)bufferRam.data();
            emu->loadRAMD0(ramData, bufferRam.size());
        } else {
            QMessageBox::critical(nullptr, romFile, "Unrecognized context file (RAM)!");
            return 0;
        }
        bufferRam = fRamFile.read(emu->getRAMsizeD1());
        if (bufferRam.size() == emu->getRAMsizeD1()) {
            uint8_t *ramData = (uint8_t *)bufferRam.data();
            emu->loadRAMD1(ramData, bufferRam.size());
        } else {
            QMessageBox::critical(nullptr, romFile, "Unrecognized context file (RAM)!");
            return 0;
        }

        fRamFile.close();
        emu->getRamFromDisk=true;

    }

   // If we call after initialization with the saved RAM, it works !
    MainWindow w1(emu);
   ret = a.exec();

    // Save RAM
    if (emu)  {
        QFile f(ramFile);
        f.open(QFile::WriteOnly);

        // RAM
        f.write((const char *)emu->getRAMC0(),emu->getRAMsizeC0());
        f.write((const char *)emu->getRAMC1(),emu->getRAMsizeC1());
        f.write((const char *)emu->getRAMD0(),emu->getRAMsizeD0());
        f.write((const char *)emu->getRAMD1(),emu->getRAMsizeD1());

        f.close();
    }

return ret;
}


