//System includes
#include <iostream>

//Library includes

//Local includes
#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "FFTPlotDockWidget.h"
#include "IntegratedBandwidthTimeDockWidget.h"
#include "WaterfallPlotDockWidget.h"

using namespace std;

cMainWindow::cMainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_pUI(new Ui::cMainWindow),
    m_pNetworkGroupBox(new cNetworkGroupBox)
{
    setDockNestingEnabled(true); //Required because we have potentially lots of docking widgets
    m_pUI->setupUi(this);
    m_pUI->horizontalLayout_MainWindowTop->insertWidget(3, m_pNetworkGroupBox);

    //Add initial widgets
    addPlot(cPlotDockWidgetBase::WATER_FALL);
    addPlot(cPlotDockWidgetBase::FFT);
}

cMainWindow::~cMainWindow()
{
    delete m_pUI;

    //Delete all plot widgets
    for(int i = 0; i < m_qvPlotWidgets.size(); i++)
    {
        delete m_qvPlotWidgets[i];
    }
}

void cMainWindow::addPlot(cPlotDockWidgetBase::WidgetType eWidgetType)
{
    cPlotDockWidgetBase* pNewWidget = NULL;

    switch(eWidgetType)
    {
    case cPlotDockWidgetBase::FFT:
        pNewWidget = new cFFTPlotDockWidget;
        break;

    case cPlotDockWidgetBase::WATER_FALL:
        pNewWidget = new cWaterfallPlotDockWidget;
        break;

    case cPlotDockWidgetBase::INTEGRATED_BANDWIDTH:
        pNewWidget = new cIntegratedBandwidthTimeDockWidget;
        break;

    default:
        cout << "Warning unknown widget type requested. Returning." << endl;
        return;
    }

    m_qvPlotWidgets.push_back(pNewWidget);

    m_pUI->verticalLayout_mainWindow->insertWidget(m_pUI->verticalLayout_mainWindow->count() - 1, pNewWidget);

}

