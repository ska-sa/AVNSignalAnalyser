#ifndef FFTPLOTDOCKWIDGET_H
#define FFTPLOTDOCKWIDGET_H

//System includes

//Library includes
#include <qwt_plot_grid.h>

//Local includes
#include "PlotDockWidgetBase.h"

namespace Ui {
class cFFTPlotDockWidget;
}

class cFFTPlotDockWidget : public cPlotDockWidgetBase
{
    Q_OBJECT

public:
    explicit cFFTPlotDockWidget(QWidget *parent = 0);
    ~cFFTPlotDockWidget();

    void addData(QVector<float> qvfData);

private:
    Ui::cFFTPlotDockWidget  *m_pUI;

    QwtPlotGrid             m_oPlotGrid;

    void setupPlot();
};

#endif // FFTPLOTDOCKWIDGET_H
