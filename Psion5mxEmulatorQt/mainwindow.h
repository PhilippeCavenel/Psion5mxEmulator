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
public:
// TODO
private:
    void run();
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
    void execTimer_3();

private:
	QElapsedTimer elapsedTimer;
    Ui::MainWindow *ui;
	PDAScreenWindow pdaScreen;
	EmuBase *emu;
    QTimer *timer_1;
    QTimer *timer_2;
    QTimer *timer_3;
};

#endif // MAINWINDOW_H
