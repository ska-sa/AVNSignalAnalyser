//System includes

//Library includes

//Local includes
#include "PlotDockWidgetBase.h"
#include "ui_PlotDockWidgetBase.h"

cPlotDockWidgetBase::cPlotDockWidgetBase(QWidget *parent) :
    QDockWidget(parent),
    m_pUI(new Ui::cPlotDockWidgetBase)
{
    m_pUI->setupUi(this);

    setupPlot();
}

cPlotDockWidgetBase::~cPlotDockWidgetBase()
{
    delete m_pUI;
}

void cPlotDockWidgetBase::pausePlot()
{
}

void cPlotDockWidgetBase::addData(QVector<float> qvfData)
{
}

void cPlotDockWidgetBase::setupPlot()
{
}

void cPlotDockWidgetBase::slotMoveUp()
{
}

void cPlotDockWidgetBase::slotMoveDown()
{

}
