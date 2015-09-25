#ifndef NETWORKGROUPBOX_H
#define NETWORKGROUPBOX_H

#include <QGroupBox>

namespace Ui {
class cNetworkGroupBox;
}

class cNetworkGroupBox : public QGroupBox
{
    Q_OBJECT

public:
    enum NetworkProtocol
    {
        TCP=0,
        UDP
    };

    explicit cNetworkGroupBox(bool bTCPEnabled = true, bool bUDPEnabled = true, QWidget *parent = 0);
    ~cNetworkGroupBox();

    void                    setTCPEnabled(bool bEnabled);
    void                    setUDPEnabled(bool bEnabled);

private:
    Ui::cNetworkGroupBox    *m_pUI;

    NetworkProtocol         m_eNetworkProtocol;
    bool                    m_bIsConnectedOrBound;
    bool                    m_bIsPaused;
    bool                    m_bTCPIsEnabled;
    bool                    m_bUDPIsEnabled;

    void                    connectSignalsToSlots();
    void                    updateAvailableProtocols();
    void                    updateGUI();

private slots:
    void                    slotSetNetworkProtocol(int iNetworkProtocol);
    void                    slotConnectDisconnect();
    void                    slotPauseResumePlots();

public slots:
    void                    slotSetConnectedOrBound(bool bIsConnectedOrBound);

signals:
    void                    sigConnectClicked(QString qstrPeerAddress, unsigned short usPort);
    void                    sigDisconnectClicked();
    void                    sigPausePlots(bool bPause);
};

#endif // NETWORKGROUPBOX_H
