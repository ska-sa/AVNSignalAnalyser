//System includes
#include <iostream>

//Library includes
#include <QThread>

//Local includes
#include "MainWindow.h"
#include "ui_MainWindow.h"

using namespace std;

cMainWindow::cMainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_pUI(new Ui::cMainWindow),
    m_pNetworkGroupBox(new cNetworkConnectionWidget(true, true, this)), //No UDP currently
    m_pPlotsWidget(new cPlotsWidget(this))
{
    m_pUI->setupUi(this);
    m_pUI->horizontalLayout_MainWindowTop->insertWidget(3, m_pNetworkGroupBox);
    m_pUI->verticalLayout_mainWindow->insertWidget(1, m_pPlotsWidget);

    connectSignalsToSlots();

    cout << "cMainWindow::cMainWindow() Thread is: " << QThread::currentThread() << endl;
}

cMainWindow::~cMainWindow()
{
    delete m_pUI;
}

void cMainWindow::connectSignalsToSlots()
{
    QObject::connect( m_pNetworkGroupBox, SIGNAL(sigConnectClicked(int, const QString&, unsigned short, const QString&, unsigned short)),
                      m_pPlotsWidget, SLOT(slotConnect(int, const QString&, unsigned short, const QString&, unsigned short)) );
    QObject::connect( m_pNetworkGroupBox, SIGNAL(sigDisconnectClicked()), m_pPlotsWidget, SLOT(slotDisconnect()) );
    QObject::connect( m_pNetworkGroupBox, SIGNAL(sigPausePlots(bool)), m_pPlotsWidget, SLOT(slotPausePlots(bool)) );
    QObject::connect( m_pPlotsWidget, SIGNAL(sigConnected(bool)), m_pNetworkGroupBox, SLOT(slotSetConnectedOrBound(bool)) );
    QObject::connect( m_pUI->actionExit, SIGNAL(triggered()), this, SLOT(close()));
}
