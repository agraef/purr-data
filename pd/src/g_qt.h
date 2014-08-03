// this file is for MOC/Qt stuff.
// Do not include in C source.
// the C interface to g_qt.c will be in g_canvas.h.

#include <QtWidgets/QMainWindow>

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	MainWindow();
	bool askQuit();
	void closeEvent(QCloseEvent *event);

	// list of menus
	QMenu *fileMenu;
	QMenu *editMenu;
	QMenu *helpMenu;

	// list of actions connecting menus/buttons to slots
	QAction *quitAct;
	QAction *editModeAct;
	QAction *aboutAct;

public slots:
	// list of functions called by menus or buttons
	//void quit ();
	void editMode ();
	void about ();
};
