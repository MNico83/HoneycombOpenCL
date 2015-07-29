#include "mainwindow.h"
#include "window.h"
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>


QHoneyCombMainWindow::QHoneyCombMainWindow()
{
    QMenuBar *menuBar = new QMenuBar;
    QMenu *menuWindow = menuBar->addMenu(tr("&Window"));
    
    if (!centralWidget())
      setCentralWidget(new HoneyCombWindow(this));
    setMenuBar(menuBar);

}

