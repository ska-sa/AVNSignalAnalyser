#ifndef NETWORK_CONNECTION_WIDGET_H
#define NETWORK_CONNECTION_WIDGET_H

#include <QGroupBox>

namespace Ui {
class cNetworkConnectionWidget;
}

class cNetworkConnectionWidget : public QGroupBox
{
    Q_OBJECT

public:
    enum Protocol
    {
        TCP=0,
        UDP
    };

    explicit cNetworkConnectionWidget(bool bTCPEnabled = true, bool bUDPEnabled = true, QWidget *parent = 0);
    ~cNetworkConnectionWidget();

    void                            setTCPEnabled(bool bEnabled);
    void                            setUDPEnabled(bool bEnabled);

private:
    Ui::cNetworkConnectionWidget    *m_pUI;

    Protocol                        m_eNetworkProtocol;
    bool                            m_bIsConnectedOrBound;
    bool                            m_bIsPaused;
    bool                            m_bTCPIsEnabled;
    bool                            m_bUDPIsEnabled;

    void                            connectSignalsToSlots();
    void                            updateAvailableProtocols();
    void                            updateGUI();

private slots:
    void                            slotSetNetworkProtocol(int iNetworkProtocol);
    void                            slotConnectDisconnect();
    void                            slotPauseResumePlots();

public slots:
    void                            slotSetConnectedOrBound(bool bIsConnectedOrBound);

signals:
    void                            sigConnectClicked(int iProtocol, const QString &qstrLocalInterface, unsigned short usLocalPort, const QString &qstrPeerAddress, unsigned short usPort);
    void                            sigDisconnectClicked();
    void                            sigPausePlots(bool bPause);
};

#endif // NETWORK_CONNECTION_WIDGET_H
