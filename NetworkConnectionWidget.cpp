//System includes

//Library includes
#include <QDebug>

//Local includes
#include "NetworkConnectionWidget.h"
#include "ui_NetworkConnectionWidget.h"

cNetworkConnectionWidget::cNetworkConnectionWidget(bool bTCPAvailable, bool bUDPAvailable, bool bKATCPAvailable, QWidget *pParent) :
    QGroupBox(pParent),
    m_pUI(new Ui::cNetworkConnectionWidget),
    m_bIsConnectedOrBound(false),
    m_bTCPIsAvailable(bTCPAvailable),
    m_bUDPIsAvailable(bUDPAvailable),
    m_bKATCPAvailable(bKATCPAvailable)
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
    QObject::connect( m_pUI->comboBox_dataProtocol, SIGNAL(currentIndexChanged(int)), this, SLOT(slotSetDataProtocol(int)) );
    QObject::connect( m_pUI->pushButton_connectDisconnect, SIGNAL(clicked()), this, SLOT(slotConnectDisconnect()) );
}

void cNetworkConnectionWidget::slotSetDataProtocol(int iDataProtocol)
{
    m_eDataProtocol = dataProtocol(iDataProtocol);
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
        switch(m_eDataProtocol)
        {
        case TCP:
            sigConnectClicked(TCP, QString(""), 0, m_pUI->lineEdit_server->text(), (unsigned short)m_pUI->spinBox_serverPort->value());
            break;

        case UDP:
            sigConnectClicked(UDP, m_pUI->lineEdit_interface->text(), m_pUI->spinBox_localPort->value(), m_pUI->lineEdit_server->text(), (unsigned short)m_pUI->spinBox_serverPort->value());
            break;

        default:
            return; //Shouldn't ever be reached if the GUI is correct
        }
    }
}

void cNetworkConnectionWidget::slotSetConnectedOrBound(bool bIsConnectedOrBound)
{
    m_bIsConnectedOrBound = bIsConnectedOrBound;

    updateGUI();

    sigConnectedOrBound(m_bIsConnectedOrBound);
}

void cNetworkConnectionWidget::updateGUI()
{
    if(m_pUI->comboBox_dataProtocol->currentText() == QString("TCP"))
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

}

void cNetworkConnectionWidget::setTCPAvailable(bool bAvailable)
{
    m_bTCPIsAvailable = bAvailable;
    updateAvailableProtocols();
}

void cNetworkConnectionWidget::setUDPAvailable(bool bAvailable)
{
    m_bUDPIsAvailable = bAvailable;
    updateAvailableProtocols();
}

void cNetworkConnectionWidget::setKATCPAvailable(bool bAvailable)
{
    m_bUDPIsAvailable = bAvailable;
    updateAvailableProtocols();
}

void cNetworkConnectionWidget::updateAvailableProtocols()
{
    m_pUI->comboBox_dataProtocol->clear();

    if(m_bTCPIsAvailable)
    {
        m_pUI->comboBox_dataProtocol->addItem(QString("TCP"));
    }

    if(m_bUDPIsAvailable)
    {
        m_pUI->comboBox_dataProtocol->addItem(QString("UDP"));
    }


}
