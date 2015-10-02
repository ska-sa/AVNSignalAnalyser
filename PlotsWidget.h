#ifndef PLOTSWIDGET_H
#define PLOTSWIDGET_H

//System includes

//Library includes
#include <QWidget>
#include <QReadWriteLock>
#include <QFuture>

#ifndef Q_MOC_RUN //Qt's MOC and Boost have some issues don't let MOC process boost headers
#include <boost/scoped_ptr.hpp>
#endif

//Local includes
#include "AVNAppLibs/SocketStreamers/TCPReceiver/TCPReceiver.h"
#include "AVNDataTypes/SpectrometerDataStream/SpetrometerDefinitions.h"
#include "AVNGUILibs/QwtPlotting/FramedQwtLinePlotWidget.h"
#include "AVNGUILibs/QwtPlotting/BandPowerQwtLinePlotWidget.h"

namespace Ui {
class cPlotsWidget;
}

class cPlotsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit cPlotsWidget(QWidget *parent = 0);
    ~cPlotsWidget();

    bool                                isRunning();
    void                                setIsRunning(bool bIsRunning);

private:
    Ui::cPlotsWidget                    *m_pUI;

    cFramedQwtLinePlotWidget             *m_pPowerPlotWidget;
    cFramedQwtLinePlotWidget             *m_pStokesPlotWidget;
    cBandPowerQwtLinePlot                *m_pBandPowerPlotWidget;

    boost::scoped_ptr<cTCPReceiver>     m_pTCPReceiver;

    bool                                m_bIsRunning;

    bool                                m_bPowerEnabled;
    bool                                m_bStokesEnabled;
    bool                                m_bBandPowerEnabled;

    QReadWriteLock                      m_oMutex;

    void                                getDataThreadFunction();
    QFuture<void>                       m_oGetDataFuture;

    void                                updatePlotType(uint16_t u16PlotType);
    uint16_t                            m_u16PlotType;

public slots:
    void                                slotConnect(QString qstrPeer, unsigned short usPeerPort);
    void                                slotDisconnect();
    void                                slotPausePlots();
    void                                slotPausePlots(bool bPause);
    void                                slotResumePlots();
    void                                slotPowerWidgetEnabled(bool bEnabled);
    void                                slotStokesWidgetEnabled(bool bEnabled);
    void                                slotBandPowerWidgetEnabled(bool bEnabled);

signals:
    void                                sigConnected();
    void                                sigDisconnected();
    void                                sigConnected(bool bIsConnected);

};

#endif // PLOTSWIDGET_H
