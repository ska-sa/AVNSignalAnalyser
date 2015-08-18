//System includes

//Library includes
#include <QPen>

//Local includes
#include "IntegratedBandwidthTimeDockWidget.h"
#include "ui_IntegratedBandwidthTimeDockWidget.h"

cIntegratedBandwidthTimeDockWidget::cIntegratedBandwidthTimeDockWidget(QWidget *parent) :
    cPlotDockWidgetBase(parent),
    m_pUI(new Ui::cIntegratedBandwidthTimeDockWidget)
{
    m_pUI->setupUi(this);

    setupPlot();
}

cIntegratedBandwidthTimeDockWidget::~cIntegratedBandwidthTimeDockWidget()
{
    delete m_pUI;
}

void cIntegratedBandwidthTimeDockWidget::addData(QVector<float> qvfData)
{

}

void cIntegratedBandwidthTimeDockWidget::setupPlot()
{
    //Titles
    //m_pUI->qwtPlot->setTitle(QwtText("Band Power"));
    m_pUI->qwtPlot->setAxisTitle(QwtPlot::xBottom, QwtText("Frequency [MHz]"));
    m_pUI->qwtPlot->setAxisTitle(QwtPlot::yLeft, QwtText("Nomalised Power [dB]"));

    //Background canvas and grid lines
    m_pUI->qwtPlot->setCanvasBackground(QBrush(QColor(Qt::black)));

    QColor oGridlineColour(Qt::gray);
    oGridlineColour.setAlphaF(0.7);
#if QWT_VERSION < 0x060100 //Account for Ubuntu's typically outdated package versions
    m_oPlotGrid.setMajPen(QPen(Qt::gray, 0.5, Qt::SolidLine ));
#else
     m_oPlotGrid.setMajorPen(QPen(Qt::gray, 0.5, Qt::SolidLine ));
#endif
    m_oPlotGrid.attach(m_pUI->qwtPlot);
}

