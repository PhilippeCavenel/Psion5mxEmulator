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
