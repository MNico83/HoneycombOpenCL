#include <QApplication>
#include <QDesktopWidget>
#include <QSurfaceFormat>

#include <iostream>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);

  QHoneyCombMainWindow mainWindow;
  mainWindow.resize(mainWindow.sizeHint());
  
  mainWindow.show();

  return app.exec();
}
