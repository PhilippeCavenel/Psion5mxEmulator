/* Code modified version Copyright (c) 2021 Philippe Cavenel
 *
 * This Source Code Form is subject to the terms of the
 * GNU GENERAL PUBLIC LICENSE Version 2, June 1991.
 *
 * The Psion-specific code is copyright (c) 2019 Ash Wolf.
 * The ARM disassembly code is a modified version of the one used in mGBA by endrift.
 * WindEmu is available under the Mozilla Public License 2.0.
*/

#include "pdascreenwindow.h"
#include <QKeyEvent>
#include <QStandardPaths>
#include <QSysInfo>
#include <QPainter>
#include <QOperatingSystemVersion>
#include <QtCore/qdebug.h>
#include <QApplication>
#include <QtCore>
#include <QImageReader>
#include <QColor>



#ifndef Q_OS_WIN64

void setFullScreenMode()
{
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([]() {
       QJniObject activity = QNativeInterface::QAndroidApplication::context();
       // Hide system ui elements or go full screen
       activity.callObjectMethod("getWindow", "()Landroid/view/Window;")
               .callObjectMethod("getDecorView", "()Landroid/view/View;")
               .callMethod<void>("setSystemUiVisibility", "(I)V", 0x00000001+0x00000002+0x00001000);
       // SYSTEM_UI_FLAG_LOW_PROFILE + SYSTEM_UI_FLAG_HIDE_NAVIGATION + SYSTEM_UI_FLAG_IMMERSIVE_STICKY
       // Check value at https://developer.android.com/reference/android/view/View#setSystemUiVisibility(int)
    }
);}
#endif


PDAScreenWindow::PDAScreenWindow(EmuBase *emu, QWidget *parent) :
	QWidget(parent),
	emu(emu),
	lcd(new QLabel(this))
{    

    // Get Picture size
#ifdef Q_OS_WIN64
        m_pictureFile=QString(qApp->applicationDirPath()).append(QString("/../../../Psion5mxEmulator/Psion5mxEmulatorQt/pkg_src/assets/SERIES5MX.BMP"));
#else // Android
        m_pictureFile= QString("assets:SERIES5MX.BMP");

#endif

      //  int m_height_picture;
      //  int m_width_picture;
     //   int m_height_screen;
      //  int m_width_screen;


     QImageReader reader(m_pictureFile);
     QSize sizeOfImage = reader.size();
     m_width_picture = sizeOfImage.width();
     m_height_picture = sizeOfImage.height();


#ifdef Q_OS_WIN64
    const char *who = emu->getDeviceName();
    setWindowTitle(who);
    m_width_screen=m_width_picture,
    m_height_screen=m_height_picture;
    setFixedSize(m_width_screen,m_height_screen);
#else
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect  screenGeometry = screen->geometry();
    m_width_screen=screenGeometry.width();
    m_height_screen=screenGeometry.height();

#endif
    m_xOrig = (m_width_screen-m_width_picture)/2;
    m_yOrig = (m_height_screen-m_height_picture)/2;
    lcd->setGeometry(0, 0, m_width_screen,m_height_screen);
}

void PDAScreenWindow::updateScreen() {
    uint8_t *lines[1024];
    QImage img(emu->getLCDWidth(), emu->getLCDHeight(), QImage::Format_RGB32);
    for (int y = 0; y < img.height(); y++)
        lines[y] = img.scanLine(y);
    emu->readLCDIntoBuffer(lines, true);

 /*  QImage img(emu->getLCDWidth(), emu->getLCDHeight(), QImage::Format_RGB32);
      emu->readLCDColorIntoBuffer(lines, true);*/

    QString pictureFile;

#ifdef Q_OS_WIN64
#else
        setFullScreenMode();
#endif
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect  screenGeometry = screen->geometry();
    QPixmap pixmap(m_width_screen, m_height_screen);
    QPainter painter=QPainter(&pixmap);
    int pos_X = m_xOrig+emu->getLCDOffsetX()-2;
    int pox_Y = m_yOrig+emu->getLCDOffsetY();
    int width = emu->getLCDWidth();
    int height = emu->getLCDHeight();
    painter.fillRect(0,0,m_width_screen,m_height_screen,Qt::black);
    painter.drawPixmap(m_xOrig, m_yOrig, m_width_picture, m_height_picture, QPixmap(m_pictureFile));
    painter.drawPixmap(pos_X,pox_Y,width,height, QPixmap::fromImage(std::move(img)));

    painter.end();
    lcd->setPixmap(pixmap);
}

static EpocKey resolveKey(int key, int vk) {
    // Placeholder, doesn't work for all keys
    switch (key) {
        case Qt::Key_Apostrophe: return EStdKeySingleQuote;
        case Qt::Key_Backspace: return EStdKeyBackspace;
        case Qt::Key_Escape: return EStdKeyEscape;
        case Qt::Key_Enter: return EStdKeyEnter;
        case Qt::Key_Return: return EStdKeyEnter;
        case Qt::Key_Alt: return EStdKeyMenu;
        case Qt::Key_Tab: return EStdKeyTab;
        case Qt::Key_Control: return EStdKeyLeftCtrl;
        case Qt::Key_Down: return EStdKeyDownArrow;
        case Qt::Key_Period: return EStdKeyFullStop;
        case Qt::Key_Meta: return EStdKeyLeftFunc;
        case Qt::Key_Shift: return EStdKeyLeftShift;
        case Qt::Key_Right: return EStdKeyRightArrow;
        case Qt::Key_Left: return EStdKeyLeftArrow;
        case Qt::Key_Comma: return EStdKeyComma;
        case Qt::Key_Up: return EStdKeyUpArrow;
        case Qt::Key_Space: return EStdKeySpace;
    }

    if (key >= '0' && key <= '9') return (EpocKey)key;
    if (key >= 'A' && key <= 'Z') return (EpocKey)key;
    return EStdKeyNull;
}


void PDAScreenWindow::keyPressEvent(QKeyEvent *event)
{
    //emu->log("KeyPress: QtKey=%d nativeVirtualKey=%x nativeModifiers=%x", event->key(), event->nativeVirtualKey(), event->nativeModifiers());
    EpocKey k = resolveKey(event->key(), event->nativeVirtualKey());
    if (k != EStdKeyNull)
        emu->setKeyboardKey(k, true);

   //updateScreen();

}

void PDAScreenWindow::keyReleaseEvent(QKeyEvent *event)
{
    EpocKey k = resolveKey(event->key(), event->nativeVirtualKey());
    if (k != EStdKeyNull)
        emu->setKeyboardKey(k, false);
}

void PDAScreenWindow::mousePressEvent(QMouseEvent *event)
{
#ifdef Q_OS_WIN64
    emu->updateTouchInput(event->x(), event->y(), true);
#else
    emu->updateTouchInput(event->x()-m_xOrig, event->y()-m_yOrig, true);
#endif

    //updateScreen();

}

void PDAScreenWindow::mouseReleaseEvent(QMouseEvent *event)
{
#ifdef Q_OS_WIN64
    emu->updateTouchInput(event->x(), event->y(), false);
#else
    emu->updateTouchInput(event->x()-m_xOrig, event->y()-m_yOrig, false);
#endif

    //updateScreen();

}

void PDAScreenWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
#ifdef Q_OS_WIN64
        emu->updateTouchInput(event->x(), event->y(), true);
#else
        emu->updateTouchInput(event->x()-m_xOrig, event->y()-m_yOrig, true);
#endif
    //updateScreen();
    }

}
