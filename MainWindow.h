#ifndef MAINWINDOW_H
#define MAINWINDOW_H

//System includes

//Library includes
#include <QMainWindow>
#include <QVector>

//Local includes
#include "NetworkGroupBox.h"
#include "PlotsWidget.h"

namespace Ui {
class cMainWindow;
}

class cMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit cMainWindow(QWidget *parent = 0);
    ~cMainWindow();

private:
    Ui::cMainWindow                 *m_pUI;
    cNetworkGroupBox                *m_pNetworkGroupBox;
    cPlotsWidget                    *m_pPlotsWidget;

    void                            connectSignalsToSlots();
};

#endif // MAINWINDOW_H
