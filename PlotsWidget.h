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
#include "RoachAcquisitionServerKATCPClient.h"

namespace Ui {
class cPlotsWidget;
}

class cPlotsWidget : public QWidget, public cTCPReceiver::cNotificationCallbackInterface, public cRoachAcquisitionServerKATCPClient::cCallbackInterface
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

    uint32_t                                                m_u32AccumulationLength_nFrames;

    void                                                    socketConnected_callback();
    void                                                    socketDisconnected_callback();

    //Callbacks from cRoachAcquisitionServerKATCPClient
    void                                                    connected_callback(bool bConnected, const std::string &strHostAddress, uint16_t u16Port, const std::string &strDescription){}

    void                                                    recordingStarted_callback(){}
    void                                                    recordingStopped_callback(){}
    void                                                    recordingInfoUpdate_callback(const std::string &strFilename,
                                                                                 int64_t i64StartTime_us, int64_t i64EllapsedTime_us,
                                                                                 int64_t i64StopTime_us, int64_t i64TimeLeft_us,
                                                                                 uint64_t u64CurrentFileSize_B, uint64_t u64DiskSpaceRemaining_B){}

    void                                                    stationControllerKATCPConnected_callback(bool bConnected){}
    void                                                    actualAntennaAz_callback(int64_t i64Timestamp_us, double dAzimuth_deg){}
    void                                                    actualAntennaEl_callback(int64_t i64Timestamp_us, double dElevation_deg){}
    void                                                    actualSourceOffsetAz_callback(int64_t i64Timestamp_us, double dAzimuthOffset_deg){}
    void                                                    actualSourceOffsetEl_callback(int64_t i64Timestamp_us, double dElevationOffset_deg){}

    void                                                    frequencyRFChan0_callback(int64_t i64Timestamp_us, double dFrequencyRFChan0_MHz){}
    void                                                    frequencyRFChan1_callback(int64_t i64Timestamp_us, double dFrequencyRFChan1_MHz){}

    void                                                    roachGatewareList_callback(const std::vector<std::string> &vstrGatewareList){}
    void                                                    roachKATCPConnected_callback(bool bConnected){}
    void                                                    stokesEnabled_callback(bool bEnabled){}
    void                                                    accumulationLength_callback(int64_t i64Timestamp_us, uint32_t u32NFrames);
    void                                                    coarseChannelSelect_callback(int64_t i64Timestamp_us, uint32_t u32ChannelNo){}
    void                                                    frequencyFs_callback(double dFrequencyFs_MHz){}
    void                                                    sizeOfCoarseFFT_callback(uint32_t u32SizeOfCoarseFFT_nSamp){}
    void                                                    sizeOfFineFFT_callback(uint32_t u32FineFFTSize_nSamp){}
    void                                                    coarseFFTShiftMask_callback(int64_t i64Timestamp_us, uint32_t u32ShiftMask){}
    void                                                    attenuationADCChan0_callback(int64_t i64Timestamp_us, double dADCAttenuationChan0_dB){}
    void                                                    attenuationADCChan1_callback(int64_t i64Timestamp_us, double dADCAttenuationChan1_dB){}
    void                                                    noiseDiodeEnabled_callback(int64_t i64Timestamp_us, bool bNoideDiodeEnabled){}
    void                                                    noiseDiodeDutyCycleEnabled_callback(int64_t i64Timestamp_us, bool bNoiseDiodeDutyCyleEnabled){}
    void                                                    noiseDiodeDutyCycleOnDuration_callback(int64_t i64Timestamp_us, uint32_t u32NAccums){}
    void                                                    noiseDiodeDutyCycleOffDuration_callback(int64_t i64Timestamp_us, uint32_t u32NAccums){}
    void                                                    overflowsRegs_callback(int64_t i64Timestamp_us, uint32_t u32OverflowRegs){}
    void                                                    eth10GbEUp_callback(int64_t i64Timestamp_us, bool bEth10GbEUp){}
    void                                                    ppsCount_callback(int64_t i64Timestamp_us, uint32_t u32PPSCount){}
    void                                                    clockFrequency_callback(int64_t i64Timestamp_us, uint32_t u32ClockFrequency_Hz){}


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
