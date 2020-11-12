#include "imageviewer.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <iomanip>

#include <QtWidgets>
#include <QPixmap>
#include <QPainter>
#include <QPlainTextEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

ImageViewer::ImageViewer(QWidget *parent)
   : QMainWindow(parent), mainWidget(new QWidget), mainLayout(new QHBoxLayout), sideLayout(new QVBoxLayout), imageLabel(new ImageLabel),
	 hintLabel(new QLabel), textArea(new QPlainTextEdit), scrollArea(new QScrollArea),
	 resetButton(new QPushButton("Reset")), skipButton(new QPushButton("Skip")), exportButton(new QPushButton("Export")),
	 undoButton(new QPushButton("Undo")), nextButton(new QPushButton("Next")), scaleFactor(1)
{
	setFocusPolicy(Qt::StrongFocus);

	imageLabel->setBackgroundRole(QPalette::Base);
	imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	imageLabel->setScaledContents(true);
	connect(imageLabel, SIGNAL(mouseClicked(int, int)), this, SLOT(imageClicked(int, int)));

	scrollArea->setBackgroundRole(QPalette::Dark);
	scrollArea->setWidget(imageLabel);
	scrollArea->setVisible(false);

	textArea->setFixedWidth(120);

	hintLabel->setTextFormat(Qt::RichText);
	hint_str.push_back(std::string("camera"));
	hint_str.push_back(std::string("left_shoulder"));
	hint_str.push_back(std::string("right_shoulder"));
	hint_str.push_back(std::string("left_elbow"));
	hint_str.push_back(std::string("right_elbow"));
	hint_str.push_back(std::string("left_wrist"));
	hint_str.push_back(std::string("right_wrist"));
	hint_str.push_back(std::string("left_hip"));
	hint_str.push_back(std::string("right_hip"));
	hint_str.push_back(std::string("left_knee"));
	hint_str.push_back(std::string("right_knee"));
	hint_str.push_back(std::string("left_ankle"));
	hint_str.push_back(std::string("right_ankle"));
	updateHint();

	connect(resetButton, SIGNAL(clicked()), this, SLOT(resetData()));
	connect(skipButton, SIGNAL(clicked()), this, SLOT(skipKeypoint()));
	connect(exportButton, SIGNAL(clicked()), this, SLOT(exportData()));
	connect(undoButton, SIGNAL(clicked()), this, SLOT(undoData()));
	connect(nextButton, SIGNAL(clicked()), this, SLOT(nextFileOpen()));

	sideLayout->addWidget(resetButton);
	sideLayout->addWidget(skipButton);
	sideLayout->addWidget(exportButton);
	sideLayout->addWidget(undoButton);
	sideLayout->addWidget(nextButton);
	sideLayout->addWidget(hintLabel);
	sideLayout->addWidget(textArea);

	mainLayout->addWidget(scrollArea);
	mainLayout->addLayout(sideLayout);

	mainWidget->setLayout(mainLayout);
	setCentralWidget(mainWidget);

	createActions();

	resize(QGuiApplication::primaryScreen()->availableSize() * 3 / 5);
}

bool ImageViewer::loadFile(const QString &fileName)
{
	currentFileName = fileName;
	QImageReader reader(fileName);
	reader.setAutoTransform(true);
	const QImage newImage = reader.read();
	if (newImage.isNull()) {
		QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
								 tr("Cannot load %1: %2")
								 .arg(QDir::toNativeSeparators(fileName), reader.errorString()));
		return false;
	}

	setImage(newImage);

	setWindowFilePath(fileName);

	const QString message = tr("Opened \"%1\", %2x%3, Depth: %4")
		.arg(QDir::toNativeSeparators(fileName)).arg(image.width()).arg(image.height()).arg(image.depth());
	statusBar()->showMessage(message);
	return true;
}

void ImageViewer::setImage(const QImage &newImage)
{
	image = newImage;
	imagePixmap = QPixmap::fromImage(image);
	imageLabel->setPixmap(imagePixmap);

	scaleFactor = 1.0;

	scrollArea->setVisible(true);
	fitToWindowAct->setEnabled(true);
	updateActions();

	if (!fitToWindowAct->isChecked())
		imageLabel->adjustSize();

	keypoints.clear();
	textArea->setPlainText(QString(""));
}

bool ImageViewer::saveFile(const QString &fileName)
{
	QImageWriter writer(fileName);

	if (!writer.write(image)) {
		QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
								 tr("Cannot write %1: %2")
								 .arg(QDir::toNativeSeparators(fileName)), writer.errorString());
		return false;
	}
	const QString message = tr("Wrote \"%1\"").arg(QDir::toNativeSeparators(fileName));
	statusBar()->showMessage(message);
	return true;
}

static void initializeImageFileDialog(QFileDialog &dialog, QFileDialog::AcceptMode acceptMode)
{
	static bool firstDialog = true;

	if (firstDialog) {
		firstDialog = false;
		const QStringList picturesLocations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
		dialog.setDirectory(picturesLocations.isEmpty() ? QDir::currentPath() : picturesLocations.last());
	}
	return; // for all file types

	QStringList mimeTypeFilters;
	const QByteArrayList supportedMimeTypes = acceptMode == QFileDialog::AcceptOpen
		? QImageReader::supportedMimeTypes() : QImageWriter::supportedMimeTypes();
	for (const QByteArray &mimeTypeName : supportedMimeTypes)
		mimeTypeFilters.append(mimeTypeName);
	mimeTypeFilters.sort();
	dialog.setMimeTypeFilters(mimeTypeFilters);
	dialog.selectMimeTypeFilter("image/jpeg");
	if (acceptMode == QFileDialog::AcceptSave)
		dialog.setDefaultSuffix("jpg");
}

void ImageViewer::open()
{
	QFileDialog dialog(this, tr("Open File"));
	initializeImageFileDialog(dialog, QFileDialog::AcceptOpen);

	while (dialog.exec() == QDialog::Accepted && !loadFile(dialog.selectedFiles().first())) {}
}

void ImageViewer::saveAs()
{
	QFileDialog dialog(this, tr("Save File As"));
	initializeImageFileDialog(dialog, QFileDialog::AcceptSave);

	while (dialog.exec() == QDialog::Accepted && !saveFile(dialog.selectedFiles().first())) {}
}

void ImageViewer::copy()
{
#ifndef QT_NO_CLIPBOARD
	QGuiApplication::clipboard()->setImage(image);
#endif // !QT_NO_CLIPBOARD
}

#ifndef QT_NO_CLIPBOARD
static QImage clipboardImage()
{
	if (const QMimeData *mimeData = QGuiApplication::clipboard()->mimeData()) {
		if (mimeData->hasImage()) {
			const QImage image = qvariant_cast<QImage>(mimeData->imageData());
			if (!image.isNull())
				return image;
		}
	}
	return QImage();
}
#endif // !QT_NO_CLIPBOARD

void ImageViewer::paste()
{
#ifndef QT_NO_CLIPBOARD
	const QImage newImage = clipboardImage();
	if (newImage.isNull()) {
		statusBar()->showMessage(tr("No image in clipboard"));
	} else {
		setImage(newImage);
		setWindowFilePath(QString());
		const QString message = tr("Obtained image from clipboard, %1x%2, Depth: %3")
			.arg(newImage.width()).arg(newImage.height()).arg(newImage.depth());
		statusBar()->showMessage(message);
	}
#endif // !QT_NO_CLIPBOARD
}

void ImageViewer::zoomIn()
{
	scaleImage(1.25);
}

void ImageViewer::zoomOut()
{
	scaleImage(0.8);
}

void ImageViewer::normalSize()
{
	imageLabel->adjustSize();
	scaleFactor = 1.0;
}

void ImageViewer::fitToWindow()
{
	bool fitToWindow = fitToWindowAct->isChecked();
	scrollArea->setWidgetResizable(fitToWindow);
	if (!fitToWindow)
		normalSize();
	updateActions();
}

void ImageViewer::about()
{
	QMessageBox::about(this, tr("About Image Viewer"),
			tr("Annotation tool for 2D humanoid pose."));
}

void ImageViewer::createActions()
{
	QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

	QAction *openAct = fileMenu->addAction(tr("&Open..."), this, &ImageViewer::open);
	openAct->setShortcut(QKeySequence::Open);

	saveAsAct = fileMenu->addAction(tr("&Save As..."), this, &ImageViewer::saveAs);
	saveAsAct->setEnabled(false);

	fileMenu->addSeparator();

	QAction *exitAct = fileMenu->addAction(tr("E&xit"), this, &QWidget::close);
	exitAct->setShortcut(tr("Ctrl+Q"));

	QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));

	copyAct = editMenu->addAction(tr("&Copy"), this, &ImageViewer::copy);
	copyAct->setShortcut(QKeySequence::Copy);
	copyAct->setEnabled(false);

	QAction *pasteAct = editMenu->addAction(tr("&Paste"), this, &ImageViewer::paste);
	pasteAct->setShortcut(QKeySequence::Paste);

	QMenu *viewMenu = menuBar()->addMenu(tr("&View"));

	zoomInAct = viewMenu->addAction(tr("Zoom &In (25%)"), this, &ImageViewer::zoomIn);
	zoomInAct->setShortcut(QKeySequence::ZoomIn);
	zoomInAct->setEnabled(false);

	zoomOutAct = viewMenu->addAction(tr("Zoom &Out (25%)"), this, &ImageViewer::zoomOut);
	zoomOutAct->setShortcut(QKeySequence::ZoomOut);
	zoomOutAct->setEnabled(false);

	normalSizeAct = viewMenu->addAction(tr("&Normal Size"), this, &ImageViewer::normalSize);
	normalSizeAct->setShortcut(tr("Ctrl+S"));
	normalSizeAct->setEnabled(false);

	viewMenu->addSeparator();

	fitToWindowAct = viewMenu->addAction(tr("&Fit to Window"), this, &ImageViewer::fitToWindow);
	fitToWindowAct->setEnabled(false);
	fitToWindowAct->setCheckable(true);
	fitToWindowAct->setShortcut(tr("Ctrl+F"));

	QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));

	helpMenu->addAction(tr("&About"), this, &ImageViewer::about);
}

void ImageViewer::updateActions()
{
	saveAsAct->setEnabled(!image.isNull());
	copyAct->setEnabled(!image.isNull());
	zoomInAct->setEnabled(!fitToWindowAct->isChecked());
	zoomOutAct->setEnabled(!fitToWindowAct->isChecked());
	normalSizeAct->setEnabled(!fitToWindowAct->isChecked());
}

void ImageViewer::scaleImage(double factor)
{
	Q_ASSERT(imageLabel->pixmap());
	//Q_ASSERT(! (imageLabel->pixmap(Qt::ReturnByValue).isNull()));
	scaleFactor *= factor;
	imageLabel->resize(scaleFactor * imageLabel->pixmap()->size());
	//imageLabel->resize(scaleFactor * imageLabel->pixmap(Qt::ReturnByValue).size());

	adjustScrollBar(scrollArea->horizontalScrollBar(), factor);
	adjustScrollBar(scrollArea->verticalScrollBar(), factor);

	zoomInAct->setEnabled(scaleFactor < 3.0);
	zoomOutAct->setEnabled(scaleFactor > 0.333);
}

void ImageViewer::adjustScrollBar(QScrollBar *scrollBar, double factor)
{
	scrollBar->setValue(int(factor * scrollBar->value() + ((factor - 1) * scrollBar->pageStep() / 2)));
}

void ImageViewer::imageClicked(int x, int y)
{
	keypoints.push_back(std::pair<int, int>(x, y));
	updateVisualize();
}

void ImageViewer::drawWireFrame(QPainter &painter, std::pair<int, int> kp1, std::pair<int, int> kp2)
{
	if((kp1.first == -1 && kp1.second == -1) || (kp2.first == -1 && kp2.first == -1))
		return;
	painter.drawLine(kp1.first, kp1.second, kp2.first, kp2.second);
}

void ImageViewer::updateVisualize(void)
{
	updateHint();

	QPixmap tmp_map = imagePixmap;
	QPainter painter(&tmp_map);
	QPen pen;
	pen.setWidth(10);
	pen.setColor(QColor(255, 0, 0));
	pen.setCapStyle(Qt::RoundCap);
	painter.setPen(pen);

	for(size_t i = 0; i < keypoints.size(); i++) {
		const int x = keypoints[i].first;
		const int y = keypoints[i].second;
		if(x == -1 && y == -1)
			continue;
		painter.drawPoint(x, y);
	}

	pen.setWidth(3);
	pen.setColor(QColor(128, 0, 0));
	painter.setPen(pen);

	const size_t len = keypoints.size();
	if(len >= 3) {
		drawWireFrame(painter, keypoints[1], keypoints[2]);
	}
	if(len >= 4) {
		drawWireFrame(painter, keypoints[1], keypoints[3]);
	}
	if(len >= 5) {
		drawWireFrame(painter, keypoints[2], keypoints[4]);
	}
	if(len >= 6) {
		drawWireFrame(painter, keypoints[3], keypoints[5]);
	}
	if(len >= 7) {
		drawWireFrame(painter, keypoints[4], keypoints[6]);
	}
	if(len >= 8) {
		drawWireFrame(painter, keypoints[1], keypoints[7]);
	}
	if(len >= 9) {
		drawWireFrame(painter, keypoints[2], keypoints[8]);
		drawWireFrame(painter, keypoints[7], keypoints[8]);
	}
	if(len >= 10) {
		drawWireFrame(painter, keypoints[7], keypoints[9]);
	}
	if(len >= 11) {
		drawWireFrame(painter, keypoints[8], keypoints[10]);
	}
	if(len >= 12) {
		drawWireFrame(painter, keypoints[9], keypoints[11]);
	}
	if(len >= 13) {
		drawWireFrame(painter, keypoints[10], keypoints[12]);
	}

	imageLabel->setPixmap(tmp_map);

	if(len == 13) {
		QString str;
		str = QString::number(keypoints[0].first) + QString(", ") + QString::number(keypoints[0].second) + QString(", 2");
		for(size_t i = 0; i < 4; i++) {
			str = str + QString("\n0, 0, 0");
		}
		for(size_t i = 1; i < keypoints.size(); i++) {
			if(keypoints[i].first == -1 && keypoints[i].second == -1) {
				str = str + QString("\n0, 0, 0");
			} else {
				str = str + QString("\n") + QString::number(keypoints[i].first) + QString(", ") + QString::number(keypoints[i].second) + QString(", 2");
			}
		}
		textArea->setPlainText(str);
	}
}

void ImageViewer::resetData(void)
{
	textArea->setPlainText(QString(""));
	keypoints.clear();
	updateVisualize();
}

void ImageViewer::skipKeypoint(void)
{
	keypoints.push_back(std::pair<int, int>(-1, -1));
	updateVisualize();
}

void ImageViewer::exportData(void)
{
	QString tmp_name = currentFileName;
	tmp_name.replace("jpeg", "txt");
	tmp_name.replace("jpg", "txt");
	tmp_name.replace("png", "txt");
	std::string filename = tmp_name.toStdString();

	std::ofstream ofs(filename, std::ios::binary);
	if(!ofs) {
		statusBar()->showMessage(QString("export failed"));
		return;
	}

	std::string str = textArea->toPlainText().toStdString();
	ofs << str;
	statusBar()->showMessage(QString("Saved to ") + tmp_name);
}

void ImageViewer::undoData(void)
{
	if(!keypoints.empty())
		keypoints.pop_back();
	updateVisualize();
}

void ImageViewer::nextFileOpen(void)
{
	if(currentFileName.isEmpty())
		return;
	std::string filepath = currentFileName.toStdString();
	// get filename from file path.
	const size_t slash_pos = filepath.find_last_of('/');
	std::string filename;
	std::string filepath_noname;
	if(slash_pos != std::string::npos) {
		filename = std::string(filepath.c_str() + slash_pos + 1);
		filepath_noname = std::string(filepath.c_str(), filepath.c_str() + slash_pos);
	} else
		filename = filepath;
	// extract numbers from string.
	std::regex re("[0-9]+");
	std::smatch m;
	const bool match = std::regex_search(filename, m, re);
	if(match) {
		bool zero_padding = false;
		int zero_padding_width = 0;
		if(m.str()[0] == '0') {
			zero_padding = true;
			zero_padding_width = m.str().size();
		}
		int number = std::stoi(m.str());
		number++;
		std::ostringstream oss;
		if(zero_padding) {
			oss << filepath_noname << "/" << m.prefix() << std::setfill('0') << std::setw(zero_padding_width) << number << m.suffix();
		} else {
			oss << filepath_noname << "/" << m.prefix() << number << m.suffix();
		}
		QString nextfilename = QString::fromStdString(oss.str());
		loadFile(nextfilename);
	} else
		return;
}

void ImageViewer::updateHint(void)
{
	const size_t len = keypoints.size();

	std::ostringstream ss;

	for(size_t i = 0; i < len && i < hint_str.size(); i++) {
		ss << "<font color=\"gray\">" << hint_str[i] << "</font><br>";
	}
	for(size_t i = len; i < hint_str.size(); i++) {
		ss << "<font color=\"black\">" << hint_str[i] << "</font><br>";
	}

	hintLabel->setText(QString(ss.str().c_str()));
}

void ImageViewer::keyPressEvent(QKeyEvent *event)
{
	const char key = static_cast<char>(event->key());

	if(key == 'R') {
		// reset
		resetData();
	} else if(key == 'S') {
		// skip a current keypoint
		skipKeypoint();
	} else if(key == 'E') {
		// export data
		exportData();
	} else if(key == 'W') {
		// next image
		nextFileOpen();
	} else if(key == 'Z') {
		// undo
		undoData();
	} else {
	}
}

