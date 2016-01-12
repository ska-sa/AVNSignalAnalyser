#ifndef PLOTSWIDGET_H
#define PLOTSWIDGET_H

//System includes

//Library includes
#include <QWidget>
#include <QReadWriteLock>
#include <QFuture>

#ifndef Q_MOC_RUN //Qt's MOC and Boost have some issues don't let MOC process boost headers
#include <boost/shared_ptr.hpp>
#endif

//Local includes
#include "AVNAppLibs/SocketStreamers/TCPReceiver/TCPReceiver.h"
#include "AVNAppLibs/SocketStreamers/UDPReceiver/UDPReceiver.h"
#include "AVNDataTypes/SpectrometerDataStream/SpectrometerDefinitions.h"
#include "AVNDataTypes/SpectrometerDataStream/SpectrometerDataStreamInterpreter.h"
#include "AVNGUILibs/QwtPlotting/FramedQwtLinePlotWidget.h"
#include "AVNGUILibs/QwtPlotting/BandPowerQwtLinePlotWidget.h"
#include "NetworkConnectionWidget.h"

namespace Ui {
class cPlotsWidget;
}

class cPlotsWidget : public QWidget, public cTCPReceiver::cNotificationCallbackInterface
{
    Q_OBJECT

public:
    explicit cPlotsWidget(QWidget *pParent = 0);
    ~cPlotsWidget();

    bool                                                    isRunning();
    void                                                    setIsRunning(bool bIsRunning);

private:
    Ui::cPlotsWidget                                        *m_pUI;

    cFramedQwtLinePlotWidget                                *m_pPowerPlotWidget;
    cFramedQwtLinePlotWidget                                *m_pStokesPhasePlotWidget;
    cBandPowerQwtLinePlot                                   *m_pBandPowerPlotWidget;

    boost::shared_ptr<cSocketReceiverBase>                  m_pSocketReceiver;
    boost::shared_ptr<cSpectrometerDataStreamInterpreter>   m_pStreamInterpreter;

    bool                                                    m_bIsRunning;

    bool                                                    m_bPowerEnabled;
    bool                                                    m_bStokesPhaseEnabled;
    bool                                                    m_bBandPowerEnabled;

    QReadWriteLock                                          m_oMutex;

    void                                                    getDataThreadFunction();
    QFuture<void>                                           m_oGetDataFuture;

    void                                                    updatePlotType(uint16_t u16PlotType);
    uint16_t                                                m_u16PlotType;

    void                                                    socketConnected_callback();
    void                                                    socketDisconnected_callback();

public slots:
    void                                                    slotConnect(int iProtocol, const QString &qstrLocalInterface, unsigned short usLocalPort,
                                                                        const QString &qstrPeerAddress, unsigned short usPeerPort);
    void                                                    slotDisconnect();
    void                                                    slotPausePlots();
    void                                                    slotPausePlots(bool bPause);
    void                                                    slotResumePlots();

    //Slots for enabling / disabling plots from outside of this widget
    void                                                    slotEnablePowerPlot(bool bEnable);
    void                                                    slotEnableStokesPhasePlot(bool bEnable);
    void                                                    slotEnableBandPowerPlot(bool bEnable);

private slots:
    //Slots to catch enabling / disabling of groupboxes and in turn fire signals to be emitted to other classes
    void                                                    slotPowerPlotEnabled(bool bEnabled);
    void                                                    slotStokesPhasePlotEnabled(bool bEnabled);
    void                                                    slotBandPowerPlotEnabled(bool bEnabled);

signals:
    void                                                    sigConnected();
    void                                                    sigDisconnected();
    void                                                    sigConnected(bool bIsConnected);
    void                                                    sigDisconnect();

    //Signals emitted when plots are enabled on the actual plot widget
    void                                                    sigPowerPlotEnabled(bool bEnabled);
    void                                                    sigStokesPhasePlotEnabled(bool bEnabled);
    void                                                    sigBandPowerPlotEnabled(bool bEnabled);

};

#endif // PLOTSWIDGET_H
