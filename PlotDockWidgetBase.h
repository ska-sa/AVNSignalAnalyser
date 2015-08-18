#ifndef PLOTDOCKWIDGETBASE_H
#define PLOTDOCKWIDGETBASE_H

//System includes

//Library includes
#include <QDockWidget>
#include <QVector>

//Local includes

namespace Ui {
class cPlotDockWidgetBase;
}

class cPlotDockWidgetBase : public QDockWidget
{
    Q_OBJECT

public:
    enum WidgetType
    {
        FFT = 0,
        WATER_FALL,
        INTEGRATED_BANDWIDTH
    };

    explicit cPlotDockWidgetBase(QWidget *parent = 0);
    virtual ~cPlotDockWidgetBase();

    virtual void addData(QVector<float> qvfData);

    virtual void pausePlot();

protected:
    Ui::cPlotDockWidgetBase *m_pUI;

    virtual void setupPlot();

private slots:
    void slotMoveUp();
    void slotMoveDown();

signals:

};

#endif // PLOTDOCKWIDGETBASE_H
