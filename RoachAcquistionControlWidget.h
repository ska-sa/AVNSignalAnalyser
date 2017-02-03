#ifndef ROACH_ACQUISITION_CONTROL_WIDGET_H
#define ROACH_ACQUISITION_CONTROL_WIDGET_H

//System includes

//Libary includes
#include <QDialog>
#include <QTimer>
#include <QReadWriteLock>

#ifndef Q_MOC_RUN //Qt's MOC and Boost have some issues don't let MOC process boost headers
#include <boost/shared_ptr.hpp>
#endif

//Local includes
#include "RoachAcquisitionServerKATCPClient.h"
#include "PlotsWidget.h"

namespace Ui {
class cRoachAcquistionControlWidget;
}

class cRoachAcquistionControlWidget : public QDialog, public cRoachAcquisitionServerKATCPClient::cCallbackInterface
{
    Q_OBJECT

public:
    explicit cRoachAcquistionControlWidget(cPlotsWidget *pPlotsWidget, QWidget *pParent = 0);
    ~cRoachAcquistionControlWidget();

    bool connect(const QString &qstrHostname, uint16_t u16Port);

private:
    Ui::cRoachAcquistionControlWidget                       *m_pUI;
    cPlotsWidget                                            *m_pPlotsWidget;

    QTimer                                                  m_oSecondTimer;
    QTimer                                                  m_oTwoSecondTimer;
    QTimer                                                  m_o200msTimer;

    Qt::TimeSpec                                            m_eTimeSpec;

    bool                                                    m_bIsRecording;

    boost::shared_ptr<cRoachAcquisitionServerKATCPClient>   m_pKATCPClient;

    QReadWriteLock                                          m_oMutex;

    //Station controller and Roach parameters
    bool                                                    m_bStationControllerKATCPConnected;
    double                                                  m_dFrequencyRFChan0_MHz;
    double                                                  m_dFrequencyRFChan1_MHz;

    std::vector<std::string>                                m_vstrRoachGatewareList;
    bool                                                    m_bRoachKATCPConnected;
    bool                                                    m_bStokesEnabled;
    uint32_t                                                m_u32AccumulationLength_nFrames;
    double                                                  m_dSingleAccumulationLength_ms;
    uint32_t                                                m_u32CoarseChannelSelect;
    double                                                  m_dCoarseChannelSelectBaseband_MHz;
    double                                                  m_dFrequencyFs_Hz;
    uint32_t                                                m_u32SizeOfCoarseFFT_nSamp;
    uint32_t                                                m_u32SizeOfFineFFT_nSamp;
    uint32_t                                                m_u32CoarseFFTShiftMask;
    double                                                  m_dAttenuationADCChan0_dB;
    double                                                  m_dAttenuationADCChan1_dB;
    bool                                                    m_bNoiseDiodeEnabled;
    bool                                                    m_bNoiseDiodeDutyCycleEnabled;
    uint32_t                                                m_u32NoiseDiodeDutyCycleOnDuration_nAccs;
    uint32_t                                                m_u32NoiseDiodeDutyCycleOffDuration_nAccs;
    uint32_t                                                m_u32OverflowsRegs;
    bool                                                    m_bEth10GbEUp;
    uint32_t                                                m_u32PPSCount;
    uint32_t                                                m_u32PreviousPPSCount;
    bool                                                    m_bPPSValid;
    uint32_t                                                m_u32ClockFrequency_Hz;

    boost::mutex                                            m_oParameterMutex;

    void                                                    connectSignalToSlots();

    //Implemented callbacks from cRoachAcquisitionServerKATCPClient::cCallbackInterface
    void                                                    connected_callback(bool bConnected, const std::string &strHostAddress, uint16_t u16Port, const std::string &strDescription);
    void                                                    recordingStarted_callback();
    void                                                    recordingStopped_callback();
    void                                                    recordingInfoUpdate_callback(const std::string &strFilename,
                                                                                         int64_t i64StartTime_us, int64_t i64EllapsedTime_us,
                                                                                         int64_t i64StopTime_us, int64_t i64TimeLeft_us,
                                                                                         uint64_t u64CurrentFileSize_B, uint64_t u64DiskSpaceRemaining_B);

    void                                                    stationControllerKATCPConnected_callback(bool bConnected);
    void                                                    actualAntennaAz_callback(int64_t i64Timestamp_us, double dAzimuth_deg);
    void                                                    actualAntennaEl_callback(int64_t i64Timestamp_us, double dElevation_deg);
    void                                                    actualSourceOffsetAz_callback(int64_t i64Timestamp_us, double dAzimuthOffset_deg);
    void                                                    actualSourceOffsetEl_callback(int64_t i64Timestamp_us, double dElevationOffset_deg);

    void                                                    frequencyRFChan0_callback(int64_t i64Timestamp_us, double dFrequencyRFChan0_MHz);
    void                                                    frequencyRFChan1_callback(int64_t i64Timestamp_us, double dFrequencyRFChan1_MHz);

    void                                                    roachGatewareList_callback(const std::vector<std::string> &vstrGatewareList);
    void                                                    roachKATCPConnected_callback(bool bConnected);
    void                                                    stokesEnabled_callback(bool bEnabled);
    void                                                    accumulationLength_callback(int64_t i64Timestamp_us, uint32_t u32NFrames);
    void                                                    coarseChannelSelect_callback(int64_t i64Timestamp_us, uint32_t u32ChannelNo);
    void                                                    frequencyFs_callback(double dFrequencyFs_Hz);
    void                                                    sizeOfCoarseFFT_callback(uint32_t u32SizeOfCoarseFFT_nSamp);
    void                                                    sizeOfFineFFT_callback(uint32_t u32SizeOfFineFFT_nSamp);
    void                                                    coarseFFTShiftMask_callback(int64_t i64Timestamp_us, uint32_t u32ShiftMask);
    void                                                    attenuationADCChan0_callback(int64_t i64Timestamp_us, double dADCAttenuationChan0_dB);
    void                                                    attenuationADCChan1_callback(int64_t i64Timestamp_us, double dADCAttenuationChan1_dB);
    void                                                    noiseDiodeEnabled_callback(int64_t i64Timestamp_us, bool bNoiseDiodeEnabled);
    void                                                    noiseDiodeDutyCycleEnabled_callback(int64_t i64Timestamp_us, bool bNoiseDiodeDutyCyleEnabled);
    void                                                    noiseDiodeDutyCycleOnDuration_callback(int64_t i64Timestamp_us, uint32_t u32NAccums);
    void                                                    noiseDiodeDutyCycleOffDuration_callback(int64_t i64Timestamp_us, uint32_t u32NAccums);
    void                                                    overflowsRegs_callback(int64_t i64Timestamp_us, uint32_t u32OverflowRegs);
    void                                                    eth10GbEUp_callback(int64_t i64Timestamp_us, bool bEth10GbEUp);
    void                                                    ppsCount_callback(int64_t i64Timestamp_us, uint32_t u32PPSCount);
    void                                                    clockFrequency_callback(int64_t i64Timestamp_us, uint32_t u32ClockFrequency_Hz);

    void                                                    closeEvent(QCloseEvent *pEvent); //Overload to hide instead of close on clicking close

    void                                                    checkParametersValid();

private slots:
    void                                                    slotSecondTimerTrigger();
    void                                                    slot200msTimerTriger();
    void                                                    slotTwoSecondTimerTrigger();
    void                                                    slotStartStopRecordingClicked();
    void                                                    slotRecordingInfoUpdate(const QString &qstrFilename,
                                                                                    int64_t i64StartTime_us, int64_t i64EllapsedTime_us,
                                                                                    int64_t i64StopTime_us, int64_t i64TimeLeft_us,
                                                                                    uint64_t u64CurrentFileSize_B, uint64_t u64DiskSpaceRemaining_B);
    void                                                    slotRecordingStarted();
    void                                                    slotRecordingStoppped();
    void                                                    slotTimeZoneChanged(QString qstrTimeZone);

    //Update display Roach parameters in the GUI (These are needed for queued connections
    void                                                    slotUpdateRoachGUIParameters();
    void                                                    slotUpdateRoachGatewareList();

    //Slots for push buttons for Roach control
    void                                                    slotRefreshGatewareList();
    void                                                    slotProgram();
    void                                                    slotToggleStokes();
    void                                                    slotSendAccumulationLength_nFrames();
    void                                                    slotSendAccumulationLength_ms();
    void                                                    slotSendCoarseChannelSelect_binNo();
    void                                                    slotSendCoarseChannelSelect_baseband();
    void                                                    slotSendCoarseChannelSelect_finalIF();
    void                                                    slotSendCoarseChannelSelect_RF0();
    void                                                    slotSendCoarseChannelSelect_RF1();
    void                                                    slotSendCoarseFFTShiftMask();
    void                                                    slotSendADC0Attenuation();
    void                                                    slotSendADC1Attenuation();
    void                                                    slotToggleNoiseDiodeEnabled();
    void                                                    slotToggleNoiseDiodeDutyCycleEnabled();
    void                                                    slotSendNoiseDiodeDutyCycleOnDuration_nAccums();
    void                                                    slotSendNoiseDiodeDutyCycleOnDuration_ms();
    void                                                    slotSendNoiseDiodeDutyCycleOffDuration_nAccums();
    void                                                    slotSendNoiseDiodeDutyCycleOffDuration_ms();



signals:
    void                                                    sigRecordingInfoUpdate(const QString &qstrFilename,
                                                                                   int64_t i64StartTime_us, int64_t i64EllapsedTime_us,
                                                                                   int64_t i64StopTime_us, int64_t i64TimeLeft_us,
                                                                                   uint64_t u64CurrentFileSize_B, uint64_t u64DiskSpaceRemaining_B);
    void                                                    sigRecordingStarted();
    void                                                    sigRecordingStoppped();

    void                                                    sigUpdateRoachGatewareList();

};

#endif // ROACH_ACQUISITION_CONTROL_WIDGET_H
