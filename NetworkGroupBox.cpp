//System includes

//Library includes
#include <QDebug>

//Local includes
#include "NetworkGroupBox.h"
#include "ui_NetworkGroupBox.h"

cNetworkGroupBox::cNetworkGroupBox(QWidget *parent) :
    QGroupBox(parent),
    m_pUI(new Ui::cNetworkGroupBox),
    m_bIsConnectedOrBound(false),
    m_bIsPaused(false)
{
    m_pUI->setupUi(this);

    //m_pUI->comboBox_networkProtocol->se

    connectSignalsToSlots();

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
}

void cNetworkGroupBox::slotPauseResumePlots()
{
    m_bIsPaused = !m_bIsPaused;

    updateGUI();
}

void cNetworkGroupBox::slotSetConnectedOrBound(bool bIsConnectedOrBound)
{
    m_bIsConnectedOrBound = bIsConnectedOrBound;

    updateGUI();
}

void cNetworkGroupBox::updateGUI()
{
    if(m_pUI->comboBox_networkProtocol->currentIndex() == TCP)
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
