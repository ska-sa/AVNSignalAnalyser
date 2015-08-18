#ifndef WATERFALLPLOTDOCKWIDGET_H
#define WATERFALLPLOTDOCKWIDGET_H

//System includes

//Library includes

//Local includes
#include "PlotDockWidgetBase.h"

namespace Ui {
class cWaterfallPlotDockWidget;
}

class cWaterfallPlotDockWidget : public cPlotDockWidgetBase
{
    Q_OBJECT

public:
    explicit cWaterfallPlotDockWidget(QWidget *parent = 0);
    ~cWaterfallPlotDockWidget();

    void addData(QVector<float> qvfData);

private:
    Ui::cWaterfallPlotDockWidget *m_pUI;

    void setupPlot();
};

#endif // WATERFALLPLOTDOCKWIDGET_H
