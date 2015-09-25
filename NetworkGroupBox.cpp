//System includes

//Library includes
#include <QDebug>

//Local includes
#include "NetworkGroupBox.h"
#include "ui_NetworkGroupBox.h"

cNetworkGroupBox::cNetworkGroupBox(bool bTCPEnabled, bool bUDPEnabled, QWidget *parent) :
    QGroupBox(parent),
    m_pUI(new Ui::cNetworkGroupBox),
    m_bIsConnectedOrBound(false),
    m_bIsPaused(false),
    m_bTCPIsEnabled(bTCPEnabled),
    m_bUDPIsEnabled(bUDPEnabled)
{
    m_pUI->setupUi(this);

    //m_pUI->comboBox_networkProtocol->se

    connectSignalsToSlots();

    updateAvailableProtocols();
    updateGUI();
}

cNetworkGroupBox::~cNetworkGroupBox()
{
    delete m_pUI;
}

void cNetworkGroupBox::connectSignalsToSlots()
{
    QObject::connect( m_pUI->comboBox_networkProtocol, SIGNAL(currentIndexChanged(int)), this, SLOT(slotSetNetworkProtocol(int)) );
    QObject::connect( m_pUI->pushButton_connectDisconnect, SIGNAL(clicked()), this, SLOT(slotConnectDisconnect()) );
    QObject::connect( m_pUI->pushButton_pauseResume, SIGNAL(clicked()), this, SLOT(slotPauseResumePlots()) );
}

void cNetworkGroupBox::slotSetNetworkProtocol(int iNetworkProtocol)
{
    m_eNetworkProtocol = NetworkProtocol(iNetworkProtocol);
    updateGUI();
}

void cNetworkGroupBox::slotConnectDisconnect()
{
    if(m_bIsConnectedOrBound)
    {
        sigDisconnectClicked();
    }
    else
    {
        sigConnectClicked(m_pUI->lineEdit_server->text(), (unsigned short)m_pUI->spinBox_port->value());
    }
}

void cNetworkGroupBox::slotPauseResumePlots()
{
    m_bIsPaused = !m_bIsPaused;

    updateGUI();

    sigPausePlots(m_bIsPaused);
}

void cNetworkGroupBox::slotSetConnectedOrBound(bool bIsConnectedOrBound)
{
    m_bIsConnectedOrBound = bIsConnectedOrBound;

    updateGUI();
}

void cNetworkGroupBox::updateGUI()
{
    if(m_pUI->comboBox_networkProtocol->currentText() == QString("TCP"))
    {
        if(m_bIsConnectedOrBound)
        {
            m_pUI->pushButton_connectDisconnect->setText(QString("Disconnect"));
        }
        else
        {
            m_pUI->pushButton_connectDisconnect->setText(QString("Connect"));
        }

        m_pUI->label_interface->setVisible(false);
        m_pUI->lineEdit_interface->setVisible(false);
    }
    else
    {
        if(m_bIsConnectedOrBound)
        {
            m_pUI->pushButton_connectDisconnect->setText(QString("Unbind"));
        }
        else
        {
            m_pUI->pushButton_connectDisconnect->setText(QString("Bind"));
        }

        m_pUI->label_interface->setVisible(true);
        m_pUI->lineEdit_interface->setVisible(true);
    }

    if(m_bIsPaused)
    {
        m_pUI->pushButton_pauseResume->setText("Resume Plots");
    }
    else
    {
        m_pUI->pushButton_pauseResume->setText("Pause Plots");
    }
}

void cNetworkGroupBox::setTCPEnabled(bool bEnabled)
{
    m_bTCPIsEnabled = bEnabled;
    updateAvailableProtocols();
}

void cNetworkGroupBox::setUDPEnabled(bool bEnabled)
{
    m_bUDPIsEnabled = bEnabled;
    updateAvailableProtocols();
}

void cNetworkGroupBox::updateAvailableProtocols()
{
    m_pUI->comboBox_networkProtocol->clear();

    if(m_bTCPIsEnabled)
    {
        m_pUI->comboBox_networkProtocol->addItem(QString("TCP"));
    }

    if(m_bUDPIsEnabled)
    {
        m_pUI->comboBox_networkProtocol->addItem(QString("UDP"));
    }
}
