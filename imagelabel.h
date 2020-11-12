#ifndef IMAGELABEL_H
#define IMAGELABEL_H

#include <QLabel>
#include <QMouseEvent>

class ImageLabel : public QLabel
{
	Q_OBJECT
public:
	ImageLabel(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
	~ImageLabel();
protected:
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
signals:
	void mouseClicked(int x, int y);
};

#endif // IMAGELABEL_H

