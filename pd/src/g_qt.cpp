#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include "m_pd.h"
#include "g_canvas.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtGui/QtGui>
#include "g_qt.h"

int tv_ms_diff (timeval &a, timeval &b) {
  return (a.tv_sec-b.tv_sec)*1000 + (a.tv_usec-b.tv_usec)/1000;
}

static void infinite_loop () {
	timeval t0,t1;
	gettimeofday(&t0,0);
	for (;;) {
		gettimeofday(&t1,0);
		fprintf(stderr, "Qt thread running... (%d s)\n", tv_ms_diff(t1,t0));
		sleep(2);
	}
}

void *qt_thread_main (void *) {
	int argc=0; char **argv=0;
	QApplication app(argc,argv);
	app.setApplicationName("PureData L2ork for Qt");
	setlocale(LC_NUMERIC,"C"); //HACK because QApplication constructor sets LC_NUMERIC while pd assumes a C locale.
	MainWindow mainWin;
	mainWin.show();
	app.exec();
	fprintf(stderr,"qt_thread_main EXITING\n");
	return 0;
}

#define MENUITEM(MENU,ACT,TEXT,FUNC) \
	ACT = new QAction(tr(TEXT), this); \
	connect(ACT, SIGNAL(triggered()), this, SLOT(FUNC)); \
	MENU->addAction(ACT);

MainWindow::MainWindow () {
	QLabel *l = new QLabel("Hello Data Flowers");
	l->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
	setCentralWidget(l);

	fileMenu = menuBar()->addMenu(tr("&File"));
	MENUITEM(fileMenu,quitAct,"Quit",close());

	editMenu = menuBar()->addMenu(tr("&Edit"));
	MENUITEM(editMenu,editModeAct,"&Edit mode",editMode());

	menuBar()->addSeparator();
	helpMenu = menuBar()->addMenu(tr("&Help"));
	MENUITEM(helpMenu,aboutAct,"&About",about());
}

void MainWindow::about () {
	QMessageBox::about(this, tr("About PureData L2ork"),
		tr("<b>PureData</b> is an app whose purpose is to turn bits into potentially different bits."));
}

void MainWindow::editMode () {
	QMessageBox::about(this, tr("Edit Mode"),
		tr("To toggle edit mode, please implement this feature and recompile."));
}

bool MainWindow::askQuit () {
	return QMessageBox::Ok == QMessageBox::warning(this, tr("Quit"),
		tr("This will close Qt but leave Tk windows open. Proceed ?"), QMessageBox::Ok | QMessageBox::Cancel);
}

//void MainWindow::quit () {if (askQuit()) ...}

void MainWindow::closeEvent(QCloseEvent *event) {
	if (askQuit()) event->accept(); else event->ignore();
}
