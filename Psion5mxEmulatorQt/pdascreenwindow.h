/* Code modified version Copyright (c) 2021 Philippe Cavenel
 *
 * This Source Code Form is subject to the terms of the
 * GNU GENERAL PUBLIC LICENSE Version 2, June 1991.
 *
 * The Psion-specific code is copyright (c) 2019 Ash Wolf.
 * The ARM disassembly code is a modified version of the one used in mGBA by endrift.
 * WindEmu is available under the Mozilla Public License 2.0.
*/

#ifndef PDASCREENWINDOW_H
#define PDASCREENWINDOW_H

#include <QWidget>
#include <QLabel>
#include "../Psion5mxEmulatorCore/emubase.h"

class PDAScreenWindow : public QWidget
{
	Q_OBJECT
private:
	EmuBase *emu;
	QLabel *lcd;
    int m_height_picture;
    int m_width_picture;
    int m_height_screen;
    int m_width_screen;
    int m_xOrig;
    int m_yOrig;
    QString m_pictureFile;

public:
	explicit PDAScreenWindow(EmuBase *emu, QWidget *parent = nullptr);

public slots:
	void updateScreen();

protected:
	void keyPressEvent(QKeyEvent *event) override;
	void keyReleaseEvent(QKeyEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;

};

#endif // PDASCREENWINDOW_H
