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

    explicit cNetworkGroupBox(QWidget *parent = 0);
    ~cNetworkGroupBox();

private:
    Ui::cNetworkGroupBox    *m_pUI;

    NetworkProtocol         m_eNetworkProtocol;
    bool                    m_bIsConnectedOrBound;
    bool                    m_bIsPaused;

    void                    connectSignalsToSlots();
    void                    updateGUI();

private slots:
    void                    slotSetNetworkProtocol(int iNetworkProtocol);
    void                    slotConnectDisconnect();
    void                    slotPauseResumePlots();

public slots:
    void                    slotSetConnectedOrBound(bool bIsConnectedOrBound);
};

#endif // NETWORKGROUPBOX_H
