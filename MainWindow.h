#ifndef MAINWINDOW_H
#define MAINWINDOW_H

//System includes

//Library includes
#include <QMainWindow>
#include <QVector>

//Local includes
#include "PlotDockWidgetBase.h"
#include "NetworkGroupBox.h"

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

    QVector<cPlotDockWidgetBase*>    m_qvPlotWidgets;

    void addPlot(cPlotDockWidgetBase::WidgetType eWidgetType);
};

#endif // MAINWINDOW_H
