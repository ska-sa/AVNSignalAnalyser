//System includes

//Library includes
#include <QDebug>

//Local includes
#include "NetworkConnectionWidget.h"
#include "ui_NetworkConnectionWidget.h"

cNetworkConnectionWidget::cNetworkConnectionWidget(bool bTCPEnabled, bool bUDPEnabled, QWidget *parent) :
    QGroupBox(parent),
    m_pUI(new Ui::cNetworkConnectionWidget),
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

cNetworkConnectionWidget::~cNetworkConnectionWidget()
{
    delete m_pUI;
}

void cNetworkConnectionWidget::connectSignalsToSlots()
{
    QObject::connect( m_pUI->comboBox_networkProtocol, SIGNAL(currentIndexChanged(int)), this, SLOT(slotSetNetworkProtocol(int)) );
    QObject::connect( m_pUI->pushButton_connectDisconnect, SIGNAL(clicked()), this, SLOT(slotConnectDisconnect()) );
    QObject::connect( m_pUI->pushButton_pauseResume, SIGNAL(clicked()), this, SLOT(slotPauseResumePlots()) );
}

void cNetworkConnectionWidget::slotSetNetworkProtocol(int iNetworkProtocol)
{
    m_eNetworkProtocol = Protocol(iNetworkProtocol);
    updateGUI();
}

void cNetworkConnectionWidget::slotConnectDisconnect()
{
    if(m_bIsConnectedOrBound)
    {
        sigDisconnectClicked();
    }
    else
    {
        switch(m_eNetworkProtocol)
        {
        case TCP:
            sigConnectClicked(TCP, QString(""), 0, m_pUI->lineEdit_server->text(), (unsigned short)m_pUI->spinBox_port->value());
            break;

        case UDP:
            sigConnectClicked(UDP, m_pUI->lineEdit_interface->text(), m_pUI->spinBox_localPort->value(), m_pUI->lineEdit_server->text(), (unsigned short)m_pUI->spinBox_port->value());
            break;

        default:
            return; //Shouldn't ever be reached if the GUI is correct
        }
    }
}

void cNetworkConnectionWidget::slotPauseResumePlots()
{
    m_bIsPaused = !m_bIsPaused;

    updateGUI();

    sigPausePlots(m_bIsPaused);
}

void cNetworkConnectionWidget::slotSetConnectedOrBound(bool bIsConnectedOrBound)
{
    m_bIsConnectedOrBound = bIsConnectedOrBound;

    updateGUI();
}

void cNetworkConnectionWidget::updateGUI()
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
        m_pUI->spinBox_localPort->setVisible(false);
        m_pUI->label_localColon->setVisible(false);
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
        m_pUI->spinBox_localPort->setVisible(true);
        m_pUI->label_localColon->setVisible(true);
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

void cNetworkConnectionWidget::setTCPEnabled(bool bEnabled)
{
    m_bTCPIsEnabled = bEnabled;
    updateAvailableProtocols();
}

void cNetworkConnectionWidget::setUDPEnabled(bool bEnabled)
{
    m_bUDPIsEnabled = bEnabled;
    updateAvailableProtocols();
}

void cNetworkConnectionWidget::updateAvailableProtocols()
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
