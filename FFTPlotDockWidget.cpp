//System includes

//Library includes
#include <QPen>

//Local includes
#include "FFTPlotDockWidget.h"
#include "ui_FFTPlotDockWidget.h"

cFFTPlotDockWidget::cFFTPlotDockWidget(QWidget *parent) :
    cPlotDockWidgetBase(parent),
    m_pUI(new Ui::cFFTPlotDockWidget)
{
    m_pUI->setupUi(this);

    setupPlot();
}

cFFTPlotDockWidget::~cFFTPlotDockWidget()
{
    delete m_pUI;
}

void cFFTPlotDockWidget::addData(QVector<float> qvfData)
{

}

void cFFTPlotDockWidget::setupPlot()
{
    //Titles
    //m_pUI->qwtPlot->setTitle(QwtText("Power Spectrum"));
    m_pUI->qwtPlot->setAxisTitle(QwtPlot::xBottom, QwtText("Frequency [MHz]"));
    m_pUI->qwtPlot->setAxisTitle(QwtPlot::yLeft, QwtText("Nomalised Power [dB]"));

    //Background canvas and grid lines
    m_pUI->qwtPlot->setCanvasBackground(QBrush(QColor(Qt::black)));

    QColor oGridlineColour(Qt::gray);
    oGridlineColour.setAlphaF(0.7);

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#if QWT_VERSION < 0x060100 //Account for Ubuntu's typically outdated package versions
    m_oPlotGrid.setMajPen(QPen(Qt::gray, 0.5, Qt::SolidLine ));
#else
     m_oPlotGrid.setMajorPen(QPen(Qt::gray, 0.5, Qt::SolidLine ));
#endif

    m_oPlotGrid.attach(m_pUI->qwtPlot);
}
