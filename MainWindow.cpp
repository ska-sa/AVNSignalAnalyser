//System includes
#include <iostream>

//Library includes
#include <QThread>

//Local includes
#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "AboutDialog.h"

using namespace std;

cMainWindow::cMainWindow(QWidget *pParent) :
    QMainWindow(pParent),
    m_pUI(new Ui::cMainWindow),
    m_pNetworkGroupBox(new cNetworkConnectionWidget(true, true, this)), //No UDP currently
    m_pPlotsWidget(new cPlotsWidget(this))
{
    m_pUI->setupUi(this);
    qApp->installEventFilter( this );

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
    //NetworkGroupBox controls
    QObject::connect( m_pNetworkGroupBox, SIGNAL(sigConnectClicked(int, const QString&, unsigned short, const QString&, unsigned short)),
                      m_pPlotsWidget, SLOT(slotConnect(int, const QString&, unsigned short, const QString&, unsigned short)) );
    QObject::connect( m_pNetworkGroupBox, SIGNAL(sigDisconnectClicked()), m_pPlotsWidget, SLOT(slotDisconnect()) );
    QObject::connect( m_pPlotsWidget, SIGNAL(sigConnected(bool)), m_pNetworkGroupBox, SLOT(slotSetConnectedOrBound(bool)) );
    QObject::connect( m_pNetworkGroupBox, SIGNAL(sigConnectedOrBound(bool)), this, SLOT(slotSetConnectedOrBound(bool)) );
    QObject::connect( m_pNetworkGroupBox, SIGNAL(sigConnectKATCP(bool)), this, SLOT(slotKATCPEnabled(bool)) );

    //File menu actions
    QObject::connect( m_pUI->actionExit, SIGNAL(triggered()), this, SLOT(close()));

    //Server menu actions
    QObject::connect( m_pUI->actionConnect_Bind, SIGNAL(triggered()), m_pNetworkGroupBox, SLOT(slotConnectDisconnect()));
    QObject::connect( m_pUI->actionDisconnect_Unbind, SIGNAL(triggered()), m_pNetworkGroupBox, SLOT(slotConnectDisconnect()));

    //Help menu actions
    QObject::connect( m_pUI->actionAbout, SIGNAL ( triggered()), this, SLOT ( slotOpenAboutDialog() ) );

    //Plots menu actions
    //Menu notifying PlotsWidget
    QObject::connect( m_pUI->actionShowChannelPowers, SIGNAL ( triggered(bool) ), m_pPlotsWidget, SLOT ( slotEnablePowerPlot(bool) ) );
    QObject::connect( m_pUI->actionShowStokesPhase, SIGNAL ( triggered(bool) ), m_pPlotsWidget, SLOT ( slotEnableStokesPhasePlot(bool) ) );
    QObject::connect( m_pUI->actionShowBandPowers, SIGNAL ( triggered(bool) ), m_pPlotsWidget, SLOT ( slotEnableBandPowerPlot(bool) ) );
    //PlotsWidget notifying menu
    QObject::connect( m_pPlotsWidget, SIGNAL ( sigPowerPlotEnabled(bool) ), m_pUI->actionShowChannelPowers, SLOT ( setChecked(bool) ) );
    QObject::connect( m_pPlotsWidget, SIGNAL ( sigStokesPhasePlotEnabled(bool) ), m_pUI->actionShowStokesPhase, SLOT ( setChecked(bool) ) );
    QObject::connect( m_pPlotsWidget, SIGNAL ( sigBandPowerPlotEnabled(bool) ), m_pUI->actionShowBandPowers, SLOT ( setChecked(bool) ) );
}

void cMainWindow::slotSetConnectedOrBound(bool bIsConnectedOrBound)
{
    //Update menu entries
    m_pUI->actionConnect_Bind->setEnabled(!bIsConnectedOrBound);
    m_pUI->actionDisconnect_Unbind->setEnabled(bIsConnectedOrBound);
}

void cMainWindow::slotKATCPEnabled(bool bEnabled)
{
    if(bEnabled)
    {
        cout << "cMainWindow::slotKATCPEnabled() Starting KATCP client" << endl;

        m_pAquisitionDialog.reset( new cRoachAcquistionControlDialog(m_pPlotsWidget) );
        m_pAquisitionDialog->setModal(false); //Don't block the rest of the GUI and don't be always-on-top
        m_pAquisitionDialog->show();

        QObject::connect(m_pAquisitionDialog.data(), SIGNAL(sigKATCPSocketConnected(bool)), this, SLOT(slotSetKATCPConnected(bool)), Qt::QueuedConnection);
        QObject::connect( m_pUI->actionOpen_Roach_Aquisition_Control_Dialogue, SIGNAL(triggered()), m_pAquisitionDialog.data(), SLOT(show()) );

        m_pAquisitionDialog->connect(m_pNetworkGroupBox->getPeerAddress(), m_pNetworkGroupBox->getKATCPPort());

        cout << "cMainWindow::slotKATCPEnabled() KATCP client started..." << endl;
    }
    else
    {
        cout << "cMainWindow::slotKATCPEnabled() Stopping KATCP client" << endl;

        QObject::disconnect(m_pAquisitionDialog.data(), SIGNAL(sigKATCPSocketConnected(bool)), this, SLOT(slotSetKATCPConnected(bool)));
        QObject::disconnect( m_pUI->actionOpen_Roach_Aquisition_Control_Dialogue, SIGNAL(triggered()), m_pAquisitionDialog.data(), SLOT(show()) );
        m_pAquisitionDialog.reset();

        cout << "cMainWindow::slotKATCPEnabled() KATCP client terminated..." << endl;
    }
}

void cMainWindow::slotSetKATCPConnected(bool bIsKATCPConnected)
{
    //Update menu entry
    m_pUI->actionOpen_Roach_Aquisition_Control_Dialogue->setEnabled(bIsKATCPConnected);
    m_pAquisitionDialog->show();
}

void cMainWindow::slotOpenAboutDialog()
{
    cAboutDialog oAboutDialog(this);
    oAboutDialog.layout()->setSizeConstraint( QLayout::SetFixedSize );
    oAboutDialog.show();
    oAboutDialog.exec();
}

bool cMainWindow::eventFilter(QObject *pObj, QEvent *pEvent)
{
    //Intercept close event and close the KATCP dialog
//    if(pEvent->type() == QEvent::Close)
//    {
//        slotKATCPEnabled(false);
//    }

    //Otherwise process the event as normal
    return QMainWindow::eventFilter(pObj, pEvent);
}
