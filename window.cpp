#include "QHoneycombWidget.h"
#include "window.h"
#include "mainwindow.h"

#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QPushButton>
#include <QDesktopWidget>
#include <QApplication>
#include <QMessageBox>

HoneyCombWindow::HoneyCombWindow(QHoneyCombMainWindow *mw)
: mainWindow(mw)
{
  m_IsOn = false;
  glWidget = new QHoneycombWidget;

  m_RandomBtn = new QPushButton(tr("Randomize"), this);
  m_IterBtn = new QPushButton(tr("Iterate"), this);

  QVBoxLayout *mainLayout = new QVBoxLayout;
  QHBoxLayout *container = new QHBoxLayout;

  container->addWidget(glWidget);

  QWidget *w = new QWidget;
  w->setLayout(container);
  mainLayout->addWidget(w);

  connect(m_RandomBtn, SIGNAL(clicked()), this, SLOT(Randomize()));
  connect(m_IterBtn, SIGNAL(clicked()), this, SLOT(StartStop()));
  
  mainLayout->addWidget(m_RandomBtn);
  mainLayout->addWidget(m_IterBtn);

  setLayout(mainLayout);

  setWindowTitle(tr("HoneyComb GL"));
}



void HoneyCombWindow::Randomize()
{
  glWidget->Randomize();
}
void HoneyCombWindow::StartStop()
{
  glWidget->StartStop();

}





void HoneyCombWindow::keyPressEvent(QKeyEvent *e)
{
  //if (e->key() == Qt::Key_Control)
  //{
  //  Randomize();

  //}
  //else if (e->key() == Qt::Key_Plus)
  //{
  //  glWidget->Iteration();
  //}
  //else if (e->key() == Qt::Key_Alt)
  //{
  //  m_IsOn = !m_IsOn;
  //}
  //else if (e->key() == Qt::Key_Escape)
  //  close();
  
  QWidget::keyPressEvent(e);
}

