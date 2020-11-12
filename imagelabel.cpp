#include <iostream>
#include <QPainter>

#include "imagelabel.h"

ImageLabel::ImageLabel(QWidget *parent, Qt::WindowFlags f) : QLabel(parent, f)
{
}

ImageLabel::~ImageLabel()
{
}

void ImageLabel::mousePressEvent(QMouseEvent *event)
{
	emit mouseClicked(event->x(), event->y());
}

void ImageLabel::mouseReleaseEvent([[maybe_unused]] QMouseEvent *event)
{
}

