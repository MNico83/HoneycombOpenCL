#ifndef HONEYCOMB_WINDOW_H
#define HONEYCOMB_WINDOW_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QSlider;
class QPushButton;
QT_END_NAMESPACE

class QHoneycombWidget;
class QHoneyCombMainWindow;

class HoneyCombWindow : public QWidget
{
    Q_OBJECT

public:
    HoneyCombWindow(QHoneyCombMainWindow *mw);

protected:
    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;

signals:
    void Recaller();

private slots:
    void Randomize();
    void StartStop();



private:
    
    QHoneycombWidget *glWidget;
    QPushButton * m_RandomBtn, *m_IterBtn;
    QHoneyCombMainWindow *mainWindow;

    bool m_IsOn;
};

#endif //HONEYCOMB_WINDOW_H
