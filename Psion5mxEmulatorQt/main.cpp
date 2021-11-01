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

#ifdef Q_OS_WIN64
        romFile=QString(qApp->applicationDirPath()).append(QString("/../../../Psion5mxEmulator/Psion5mxEmulatorQt/pkg_src/assets/5mx.rom"));
#else // ANDROID
        romFile= "assets:5mx.rom";
#endif

	// what do we have?
    QFile f(romFile);
	f.open(QFile::ReadOnly);
	auto buffer = f.readAll();
	f.close();

	if (buffer.size() < 0x400000) {
        QMessageBox::critical(nullptr, romFile, "Invalid ROM file!");
        return 0;
	}

	EmuBase *emu = nullptr;
	uint8_t *romData = (uint8_t *)buffer.data();

	// parse this ROM to learn what hardware it's for
	int variantFile = *((uint32_t *)&romData[0x80 + 0x4C]) & 0xFFFFFFF;
	if (variantFile < (buffer.size() - 8)) {
		int variantImg = *((uint32_t *)&romData[variantFile + 4]) & 0xFFFFFFF;
		if (variantImg < (buffer.size() - 0x70)) {
			int variant = *((uint32_t *)&romData[variantImg + 0x60]);

			if (variant == 0x7060001) {
				// 5mx ROM
				emu = new Windermere::Emulator;
            } else {
                QMessageBox::critical(nullptr, romFile, "Unrecognised ROM file!");
				return 0;
			}
		}
	}

	emu->loadROM(romData, buffer.size());

    MainWindow w(emu);


return a.exec();
}


