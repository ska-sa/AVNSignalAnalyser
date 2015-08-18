#ifndef INTEGRATEDBANDWIDTHTIMEDOCKWIDGET_H
#define INTEGRATEDBANDWIDTHTIMEDOCKWIDGET_H

//System includes

//Library includes
#include <qwt_plot_grid.h>

//Local includes
#include "PlotDockWidgetBase.h"

namespace Ui {
class cIntegratedBandwidthTimeDockWidget;
}

class cIntegratedBandwidthTimeDockWidget : public cPlotDockWidgetBase
{
    Q_OBJECT

public:
    explicit cIntegratedBandwidthTimeDockWidget(QWidget *parent = 0);
    ~cIntegratedBandwidthTimeDockWidget();

    void addData(QVector<float> qvfData);

private:
    Ui::cIntegratedBandwidthTimeDockWidget  *m_pUI;

    QwtPlotGrid                             m_oPlotGrid;

    void setupPlot();
};

#endif // INTEGRATEDBANDWIDTHTIMEDOCKWIDGET_H
