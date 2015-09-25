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
#include "QwtLinePlotWidget.h"
#include "AVNAppLibs/SocketStreamers/TCPReceiver/TCPReceiver.h"

namespace Ui {
class cPlotsWidget;
}

class cPlotsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit cPlotsWidget(QWidget *parent = 0);
    ~cPlotsWidget();

    bool                            isRunning();
    void                            setIsRunning(bool bIsRunning);

    static const uint32_t           HEADER_SIZE_B = 16;
    static const uint32_t           SYNC_WORD = 0x1a2b3c4d;

    enum plotType
    {
        WB_SPECTROMETER_CFFT = 0,
        WB_SPECTROMETER_LRQU = 1,
        NB_SPECTROMETER_CFFT = 2,
        NB_SPECTROMETER_LRQU = 3,
        TIME_DATA = 4,
        UNDEFINED = 0xffff
    };

private:
    Ui::cPlotsWidget                *m_pUI;

    cQwtLinePlotWidget              *m_pPowerPlotWidget;
    cQwtLinePlotWidget              *m_pStokesPlotWidget;

    boost::scoped_ptr<cTCPReceiver> m_pTCPReceiver;

    bool                            m_bIsRunning;
    QReadWriteLock                  m_oIsRunningMutex;

    void                            getDataThreadFunction();
    QFuture<void>                   m_oGetDataFuture;

    void                            updatePlotType(uint16_t u16PlotType);
    plotType                        m_ePlotType;

public slots:
    void                            slotConnect(QString qstrPeer, unsigned short usPeerPort);
    void                            slotDisconnect();
    void                            slotPausePlots();
    void                            slotPausePlots(bool bPause);
    void                            slotResumePlots();

signals:
    void                            sigConnected();
    void                            sigDisconnected();
    void                            sigConnected(bool bIsConnected);

};

#endif // PLOTSWIDGET_H
