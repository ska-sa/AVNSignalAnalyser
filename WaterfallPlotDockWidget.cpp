//System includes

//Library includes

//Local includes
#include "WaterfallPlotDockWidget.h"
#include "ui_WaterfallPlotDockWidget.h"

cWaterfallPlotDockWidget::cWaterfallPlotDockWidget(QWidget *parent) :
    cPlotDockWidgetBase(parent),
    m_pUI(new Ui::cWaterfallPlotDockWidget)
{
    m_pUI->setupUi(this);

    setupPlot();
}

cWaterfallPlotDockWidget::~cWaterfallPlotDockWidget()
{
    delete m_pUI;
}

void cWaterfallPlotDockWidget::addData(QVector<float> qvfData)
{

}

void cWaterfallPlotDockWidget::setupPlot()
{
    //Titles
    //m_pUI->qwtPlot->setTitle(QwtText("Band Power History"));
    m_pUI->qwtPlot->setAxisTitle(QwtPlot::xBottom, QwtText("Frequency [MHz]"));
    m_pUI->qwtPlot->setAxisTitle(QwtPlot::yLeft, QwtText("Elapsed Time [s]"));
    m_pUI->qwtPlot->setAxisTitle(QwtPlot::yRight, QwtText("Normalised Power [dB]"));

    //Background canvas and grid lines
    m_pUI->qwtPlot->setCanvasBackground(QBrush(QColor(Qt::black)));
}

