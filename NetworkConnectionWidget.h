#ifndef NETWORK_CONNECTION_WIDGET_H
#define NETWORK_CONNECTION_WIDGET_H

//System includes
#ifdef _WIN32
#include <stdint.h>
#else
#include <inttypes.h>
#endif

//Library includes
#include <QGroupBox>

//Local includes


namespace Ui {
class cNetworkConnectionWidget;
}

class cNetworkConnectionWidget : public QGroupBox
{
    Q_OBJECT

public:
    enum dataProtocol
    {
        TCP=0,
        UDP
    };

    explicit cNetworkConnectionWidget(bool bTCPAvailable = true, bool bUDPAvailable = true, bool bKATCPAvailable = true, QWidget *pParent = 0);
    ~cNetworkConnectionWidget();

    void                            setTCPAvailable(bool bAvailable);
    void                            setUDPAvailable(bool bAvailable);
    void                            setKATCPAvailable(bool bAvailable);

    QString                         getPeerAddress() const;
    uint16_t                        getDataPort() const;
    uint16_t                        getKATCPPort() const;

private:
    Ui::cNetworkConnectionWidget    *m_pUI;

    dataProtocol                    m_eDataProtocol;
    bool                            m_bIsConnectedOrBound;
    bool                            m_bTCPIsAvailable;
    bool                            m_bUDPIsAvailable;
    bool                            m_bKATCPAvailable;

    void                            connectSignalsToSlots();
    void                            updateAvailableProtocols();
    void                            updateGUI();

private slots:
    void                            slotSetDataProtocol(int iDataProtocol);
    void                            slotKATCPEnabled(bool bEnabled);

public slots:
    void                            slotSetConnectedOrBound(bool bIsConnectedOrBound);
    void                            slotConnectDisconnect();

signals:
    void                            sigConnectClicked(int iDataProtocol, const QString &qstrLocalInterface, unsigned short usLocalPort, const QString &qstrPeerAddress, unsigned short usPort);
    void                            sigDisconnectClicked();
    void                            sigConnectedOrBound(bool);
    void                            sigConnectKATCP(bool bConnect);

};

#endif // NETWORK_CONNECTION_WIDGET_H
