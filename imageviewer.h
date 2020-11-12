#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <vector>
#include <string>
#include <utility>

#include <QMainWindow>
#include <QImage>

#include "imagelabel.h"

QT_BEGIN_NAMESPACE
class QAction;
class QLabel;
class QMenu;
class QScrollArea;
class QScrollBar;
class QHBoxLayout;
class QVBoxLayout;
class QPlainTextEdit;
class QPushButton;
QT_END_NAMESPACE

class ImageViewer : public QMainWindow
{
	Q_OBJECT

public:
	ImageViewer(QWidget *parent = nullptr);
	bool loadFile(const QString &);

protected:
	void keyPressEvent(QKeyEvent *event) override;

private slots:
	void open();
	void saveAs();
	void copy();
	void paste();
	void zoomIn();
	void zoomOut();
	void normalSize();
	void fitToWindow();
	void about();
	void imageClicked(int, int);
	void resetData();
	void skipKeypoint();
	void exportData();
	void undoData();
	void nextFileOpen();

private:
	void createActions();
	void createMenus();
	void updateActions();
	bool saveFile(const QString &fileName);
	void setImage(const QImage &newImage);
	void scaleImage(double factor);
	void adjustScrollBar(QScrollBar *scrollBar, double factor);
	void updateVisualize();
	void drawWireFrame(QPainter &painter, std::pair<int, int> kp1, std::pair<int, int> kp2);
	void updateHint();

	QString currentFileName;
	QWidget *mainWidget;
	QHBoxLayout *mainLayout;
	QVBoxLayout *sideLayout;
	QImage image;
	ImageLabel *imageLabel;
	QPixmap imagePixmap;
	QLabel *hintLabel;
	QPlainTextEdit *textArea;
	QScrollArea *scrollArea;
	QPushButton *resetButton;
	QPushButton *skipButton;
	QPushButton *exportButton;
	QPushButton *undoButton;
	QPushButton *nextButton;
	double scaleFactor;
	std::vector<std::pair<int, int>> keypoints;
	std::vector<std::string> hint_str;

	QAction *saveAsAct;
	QAction *copyAct;
	QAction *zoomInAct;
	QAction *zoomOutAct;
	QAction *normalSizeAct;
	QAction *fitToWindowAct;
};

#endif

