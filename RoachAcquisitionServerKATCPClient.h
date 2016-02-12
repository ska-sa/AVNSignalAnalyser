#ifndef ROACH_ACQUISITION_SERVER_KATCP_CLIENT_H
#define ROACH_ACQUISITION_SERVER_KATCP_CLIENT_H

//System includes

//Library includes

//Local includes
#include "AVNAppLibs/KATCP/KATCPClientBase.h"

//This KATCP client is implemented manually using a client TCP socket and not the KATCP library.
//This makes portability a bit better especially with Windows.

class cRoachAcquisitionServerKATCPClient : public cKATCPClientBase
{
public:
    class cCallbackInterface : public cKATCPClientBase::cCallbackInterface
    {
    public:
        virtual void                                    recordingStarted_callback() = 0;
        virtual void                                    recordingStopped_callback() = 0;
        virtual void                                    recordingInfoUpdate_callback(const std::string &strFilename,
                                                                                     int64_t i64StartTime_us, int64_t i64EllapsedTime_us,
                                                                                     int64_t i64StopTime_us, int64_t i64TimeLeft_us,
                                                                                     uint64_t u64CurrentFileSize_B, uint64_t u64DiskSpaceRemaining_B) = 0;

        virtual void                                    stationControllerKATCPConnected_callback(bool bConnected) = 0;
        virtual void                                    actualAntennaAz_callback(int64_t i64Timestamp_us, double dAzimuth_deg) = 0;
        virtual void                                    actualAntennaEl_callback(int64_t i64Timestamp_us, double dElevation_deg) = 0;
        virtual void                                    actualSourceOffsetAz_callback(int64_t i64Timestamp_us, double dAzimuthOffset_deg) = 0;
        virtual void                                    actualSourceOffsetEl_callback(int64_t i64Timestamp_us, double dElevationOffset_deg) = 0;

        virtual void                                    frequencyRFChan0_callback(int64_t i64Timestamp_us, double dFrequencyRFChan0_MHz) = 0;
        virtual void                                    frequencyRFChan1_callback(int64_t i64Timestamp_us, double dFrequencyRFChan1_MHz) = 0;

        virtual void                                    roachGatewareList_callback(const std::vector<std::string> &vstrGatewareList) = 0;
        virtual void                                    roachKATCPConnected_callback(bool bConnected) = 0;
        virtual void                                    stokesEnabled_callback(bool bEnabled) = 0;
        virtual void                                    accumulationLength_callback(int64_t i64Timestamp_us, uint32_t u32NFrames) = 0;
        virtual void                                    coarseChannelSelect_callback(int64_t i64Timestamp_us, uint32_t u32ChannelNo) = 0;
        virtual void                                    frequencyFs_callback(double dFrequencyFs_MHz) = 0;
        virtual void                                    sizeOfCoarseFFT_callback(uint32_t u32SizeOfCoarseFFT_nSamp) = 0;
        virtual void                                    sizeOfFineFFT_callback(uint32_t u32FineFFTSize_nSamp) = 0;
        virtual void                                    coarseFFTShiftMask_callback(int64_t i64Timestamp_us, uint32_t u32ShiftMask) = 0;
        virtual void                                    attenuationADCChan0_callback(int64_t i64Timestamp_us, double dADCAttenuationChan0_dB) = 0;
        virtual void                                    attenuationADCChan1_callback(int64_t i64Timestamp_us, double dADCAttenuationChan1_dB) = 0;
        virtual void                                    noiseDiodeEnabled_callback(int64_t i64Timestamp_us, bool bNoideDiodeEnabled) = 0;
        virtual void                                    noiseDiodeDutyCycleEnabled_callback(int64_t i64Timestamp_us, bool bNoiseDiodeDutyCyleEnabled) = 0;
        virtual void                                    noiseDiodeDutyCycleOnDuration_callback(int64_t i64Timestamp_us, uint32_t u32NAccums) = 0;
        virtual void                                    noiseDiodeDutyCycleOffDuration_callback(int64_t i64Timestamp_us, uint32_t u32NAccums) = 0;
        virtual void                                    overflowsRegs_callback(int64_t i64Timestamp_us, uint32_t u32OverflowRegs) = 0;
        virtual void                                    eth10GbEUp_callback(int64_t i64Timestamp_us, bool bEth10GbEUp) = 0;
        virtual void                                    ppsCount_callback(int64_t i64Timestamp_us, uint32_t u32PPSCount) = 0;
        virtual void                                    clockFrequency_callback(int64_t i64Timestamp_us, uint32_t u32ClockFrequency_Hz) = 0;

    };

    cRoachAcquisitionServerKATCPClient();
    ~cRoachAcquisitionServerKATCPClient();

    //Client requests
    void                                                requestStartRecording(const std::string &strFilenamePrefix = std::string(""),
                                                                              int64_t i64StartTime_us = 0, int64_t i64Duration_us = 0);
    void                                                requestStopRecording();
    void                                                requestRecordingStatus();
    void                                                requestRecordingInfoUpdate();

    void                                                requestRoachGatewareList();
    void                                                requestRoachProgram(const std::string strScriptPath);
    void                                                requestRoachSetStokesEnabled(bool bEnabled);
    void                                                requestRoachSetAccumulationLength(uint32_t u32Length_nFrames);
    void                                                requestRoachSetCoarseChannelSelect(uint32_t u32ChannelNo);
    void                                                requestRoachSetCoarseFFTShiftMask(uint32_t u32FFTShiftMask);
    void                                                requestRoachSetADC0Attenuation(uint32_t u32ADC0Attenuation);
    void                                                requestRoachSetADC1Attenuation(uint32_t u32ADC1Attenuation);
    void                                                requestRoachSetNoiseDiodeEnabled(bool bEnabled);
    void                                                requestRoachSetNoiseDiodeDutyCycleEnabled(bool bEnabled);
    void                                                requestRoachSetNoiseDiodeDutyCycleOnDuration(uint32_t u32NAccums);
    void                                                requestRoachSetNoiseDiodeDutyCycleOffDuration(uint32_t u32NAccums);

    void                                                subscribeToSensors();

private:
    //Implement from base class
    void                                                processKATCPMessage(const std::vector<std::string> &vstrMessageTokens);
    void                                                onConnected();    

    //Notifications sent to all callback handlers
    void                                                sendRecordingStarted();
    void                                                sendRecordingStopped();
    void                                                sendRecordingInfoUpdate(const std::string &strFilename,
                                                                                int64_t i64StartTime_us, int64_t i64EllapsedTime_us,
                                                                                int64_t i64StopTime_us, int64_t i64TimeLeft_us,
                                                                                uint64_t u64CurrentFileSize_B, uint64_t u64DiskSpaceRemaining_B);

    void                                                sendStationControllerKATCPConnected(bool bConnected);
    void                                                sendActualAntennaAz(int64_t i64Timestamp_us,double dAzimuth_deg);
    void                                                sendActualAntennaEl(int64_t i64Timestamp_us,double dElevation_deg);
    void                                                sendActualSourceOffsetAz(int64_t i64Timestamp_us, double dAzimuthOffset_deg);
    void                                                sendActualSourceOffsetEl(int64_t i64Timestamp_us, double dElevationOffset_deg);

    void                                                sendFrequencyRFChan0(int64_t i64Timestamp_us, double dFreqencyRF_MHz);
    void                                                sendFrequencyRFChan1(int64_t i64Timestamp_us, double dFreqencyRF_MHz);

    void                                                sendRoachGatewareList(const std::vector<std::string> &vstrGatewareList);
    void                                                sendRoachKATCPConnected(bool bConnected);
    void                                                sendStokesEnabled(bool bEnabled);
    void                                                sendAccumulationLength(int64_t i64Timestamp_us, uint32_t u32NFrames);
    void                                                sendCoarseChannelSelect(int64_t i64Timestamp_us, uint32_t u32ChannelNo);
    void                                                sendFrequencyFs(double dFrequencyFs_MHz);
    void                                                sendSizeOfCoarseFFT(uint32_t u32SizeOfCoarseFFT_nSamp);
    void                                                sendSizeOfFineFFT(uint32_t u32FineFFTSize_nSamp);
    void                                                sendCoarseFFTShiftMask(int64_t i64Timestamp_us, uint32_t u32ShiftMask);
    void                                                sendAttenuationADCChan0(int64_t i64Timestamp_us, double dADCAttenuationChan0_dB);
    void                                                sendAttenuationADCChan1(int64_t i64Timestamp_us, double dADCAttenuationChan1_dB);
    void                                                sendNoiseDiodeEnabled(int64_t i64Timestamp_us, bool bNoideDiodeEnabled);
    void                                                sendNoiseDiodeDutyCycleEnabled(int64_t i64Timestamp_us, bool bNoiseDiodeDutyCyleEnabled);
    void                                                sendNoiseDiodeDutyCycleOnDuration(int64_t i64Timestamp_us, uint32_t u32NAccums);
    void                                                sendNoiseDiodeDutyCycleOffDuration(int64_t i64Timestamp_us, uint32_t u32NAccums);
    void                                                sendOverflowsRegs(int64_t i64Timestamp_us, uint32_t u32OverflowRegs);
    void                                                sendEth10GbEUp(int64_t i64Timestamp_us, bool bEth10GbEUp);
    void                                                sendPPSCount(int64_t i64Timestamp_us, uint32_t u32PPSCount);
    void                                                sendClockFrequency(int64_t i64Timestamp_us, uint32_t u32ClockFrequency_Hz);


};

#endif // ROACH_ACQUISITION_SERVER_KATCP_CLIENT_H
