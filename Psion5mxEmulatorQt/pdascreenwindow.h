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
