#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <QtWidgets/QApplication>
#include "g_qt.h"

int tv_ms_diff (timeval &a, timeval &b) {
	return (a.tv_sec-b.tv_sec)*1000 + (a.tv_usec-b.tv_usec)/1000;
}

void qt_thread_main () {
	int argc=0; char **argv=0;
    //QApplication app(argc,argv);

	timeval t0,t1;
	gettimeofday(&t0,0);
	for (;;) {
		gettimeofday(&t1,0);
		fprintf(stderr, "Qt thread running... (%d s)\n", tv_ms_diff(t1,t0));
		sleep(2);
	}
}
