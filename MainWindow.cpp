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
    m_pUI->horizontalLayout_MainWindowTop->insertSpacerItem(1, new QSpacerItem(20, 20, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
    m_pUI->horizontalLayout_MainWindowTop->insertWidget(2, m_pNetworkGroupBox);
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
    QObject::connect( m_pPlotsWidget, SIGNAL(sigConnected(bool)), m_pNetworkGroupBox, SLOT(slotSetConnectedOrBound(bool)) );
    QObject::connect( m_pUI->actionExit, SIGNAL(triggered()), this, SLOT(close()));
    QObject::connect( m_pUI->actionConnect_Bind, SIGNAL(triggered()), m_pNetworkGroupBox, SLOT(slotConnectDisconnect()));
    QObject::connect( m_pUI->actionDisconnect_Unbind, SIGNAL(triggered()), m_pNetworkGroupBox, SLOT(slotConnectDisconnect()));
    QObject::connect( m_pNetworkGroupBox, SIGNAL(sigConnectedOrBound(bool)), this, SLOT(slotSetConnectedOrBound(bool)) );
}

void cMainWindow::slotSetConnectedOrBound(bool bIsConnectedOrBound)
{
    //Update menu entries
    m_pUI->actionConnect_Bind->setEnabled(!bIsConnectedOrBound);
    m_pUI->actionDisconnect_Unbind->setEnabled(bIsConnectedOrBound);
}

void cMainWindow::slotSetKATCPConnected(bool bIsKATCPConnected)
{
    //Update menu entry
    m_pUI->actionOpen_Roach_Aquisition_Control_Dialogue->setEnabled(bIsKATCPConnected);
}
