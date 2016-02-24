//System includes

//Library includes

//Local includes
#include "RoachAcquisitionServerKATCPClient.h"

using namespace std;

cRoachAcquisitionServerKATCPClient::cRoachAcquisitionServerKATCPClient() :
    cKATCPClientBase()
{
}

cRoachAcquisitionServerKATCPClient::~cRoachAcquisitionServerKATCPClient()
{
}

void cRoachAcquisitionServerKATCPClient::onConnected()
{
    //Request the current recording status.
    requestRecordingStatus();

    //Subscribe to the necessary sensors
    subscribeToSensors();

    //Ask for an updated list of Firmware launchers
    requestRoachGatewareList();
}

void cRoachAcquisitionServerKATCPClient::processKATCPMessage(const vector<string> &vstrTokens)
{
    //    cout << "Got KATCP message: ";
    //    for(uint32_t ui = 0; ui < vstrTokens.size(); ui++ )
    //    {
    //        cout << vstrTokens[ui] << " ";
    //    }
    //    cout << endl;

    if( !vstrTokens[0].compare("#recordingStopped") )
    {
        sendRecordingStopped();
        return;
    }



    if(!vstrTokens[0].compare("#recordingStarted"))
    {
        cout <<"Sending recording started" << endl;
        sendRecordingStarted();
        return;
    }


    if(!vstrTokens[0].compare("#recordingInfo"))
    {
        if(vstrTokens.size() < 8)
            return;

        int64_t i64StartTime_us             = strtoll(vstrTokens[2].c_str(), NULL, 10);
        int64_t i64EllapsedTime_us          = strtoll(vstrTokens[3].c_str(), NULL, 10);
        int64_t i64StopTime_us              = strtoll(vstrTokens[4].c_str(), NULL, 10);
        int64_t i64TimeLeft_us              = strtoll(vstrTokens[5].c_str(), NULL, 10);
        uint64_t u64CurrentFileSize_B       = strtoull(vstrTokens[6].c_str(), NULL, 10);
        uint64_t u64DiskSpaceRemaining_B    = strtoull(vstrTokens[7].c_str(), NULL, 10);

        sendRecordingInfoUpdate(vstrTokens[1], i64StartTime_us, i64EllapsedTime_us, i64StopTime_us, i64TimeLeft_us, u64CurrentFileSize_B, u64DiskSpaceRemaining_B);

        return;
    }


    if(!vstrTokens[0].compare("#roachGatewareList"))
    {
        vector<string> vstrGatewareList(vstrTokens);
        vstrGatewareList.erase(vstrGatewareList.begin());

        sendRoachGatewareList(vstrGatewareList);

        return;
    }


    //Sensors

    if(!vstrTokens[0].compare("#sensor-status"))
    {
        if(vstrTokens.size() < 6)
            return;

        int64_t i64Timestamp_us = strtoll(vstrTokens[1].c_str(), NULL, 10);

        if(!vstrTokens[3].compare("stationControllerConnected"))
        {
            sendStationControllerKATCPConnected( (bool)(0x00000001 & strtol(vstrTokens[3].c_str(), NULL, 10)) );
            return;
        }

        if(!vstrTokens[3].compare("actualAntennaAz"))
        {
            sendActualAntennaAz(i64Timestamp_us, strtod(vstrTokens[5].c_str(), NULL) );
            return;
        }

        if(!vstrTokens[3].compare("actualAntennaEl"))
        {
            sendActualAntennaEl(i64Timestamp_us, strtod(vstrTokens[5].c_str(), NULL) );
            return;
        }

        if(!vstrTokens[3].compare("actualSourceOffsetAz"))
        {
            sendActualSourceOffsetAz(i64Timestamp_us, strtod(vstrTokens[5].c_str(), NULL) );
            return;
        }

        if(!vstrTokens[3].compare("actualSourceOffsetEl"))
        {
            sendActualSourceOffsetEl(i64Timestamp_us, strtod(vstrTokens[5].c_str(), NULL) );
            return;
        }

        if(!vstrTokens[3].compare("frequencyRFChan0"))
        {
            sendFrequencyRFChan0(i64Timestamp_us, strtod(vstrTokens[5].c_str(), NULL) );
            return;
        }

        if(!vstrTokens[3].compare("frequencyRFChan1"))
        {
            sendFrequencyRFChan1(i64Timestamp_us, strtod(vstrTokens[5].c_str(), NULL) );
            return;
        }

        if(!vstrTokens[3].compare("roachConnected"))
        {
            sendRoachKATCPConnected( (bool)(0x00000001 & strtol(vstrTokens[5].c_str(), NULL, 10)) );
            return;
        }

        if(!vstrTokens[3].compare("roachStokesEnabled"))
        {
            sendStokesEnabled( (bool)(0x00000001 & strtol(vstrTokens[5].c_str(), NULL, 10)) );
            return;
        }

        if(!vstrTokens[3].compare("roachAccumulationLength"))
        {
            sendAccumulationLength(i64Timestamp_us, strtol(vstrTokens[5].c_str(), NULL, 10));
            return;
        }

        if(!vstrTokens[3].compare("roachCoarseChannelSelect"))
        {
            sendCoarseChannelSelect(i64Timestamp_us, strtol(vstrTokens[5].c_str(), NULL, 10));
            return;
        }

        if(!vstrTokens[3].compare("roachFrequencyFs"))
        {
            sendFrequencyFs(strtod(vstrTokens[5].c_str(), NULL));
            return;
        }

        if(!vstrTokens[3].compare("roachSizeOfCoarseFFT"))
        {
            sendSizeOfCoarseFFT(strtol(vstrTokens[5].c_str(), NULL, 10));
            return;
        }

        if(!vstrTokens[3].compare("roachSizeOfFineFFT"))
        {
            sendSizeOfFineFFT(strtol(vstrTokens[5].c_str(), NULL, 10));
            return;
        }

        if(!vstrTokens[3].compare("roachCoarseFFTShiftMask"))
        {
            sendCoarseFFTShiftMask(i64Timestamp_us, strtol(vstrTokens[5].c_str(), NULL, 10));
            return;
        }

        if(!vstrTokens[3].compare("roachAttenuationADCChan0"))
        {
            sendAttenuationADCChan0(i64Timestamp_us, strtod(vstrTokens[5].c_str(), NULL));
            return;
        }

        if(!vstrTokens[3].compare("roachAttenuationADCChan1"))
        {
            sendAttenuationADCChan1(i64Timestamp_us, strtod(vstrTokens[5].c_str(), NULL));
            return;
        }

        if(!vstrTokens[3].compare("roachNoiseDiodeEnabled"))
        {
            sendNoiseDiodeEnabled(i64Timestamp_us, (bool)(0x00000001 & strtol(vstrTokens[5].c_str(), NULL, 10)));
            return;
        }

        if(!vstrTokens[3].compare("roachNoiseDiodeDutyCycleEnabled"))
        {
            sendNoiseDiodeDutyCycleEnabled(i64Timestamp_us, (bool)(0x00000001 & strtol(vstrTokens[5].c_str(), NULL, 10)));
            return;
        }

        if(!vstrTokens[3].compare("roachNoiseDiodeDutyCycleOnDuration"))
        {
            sendNoiseDiodeDutyCycleOnDuration(i64Timestamp_us, strtol(vstrTokens[5].c_str(), NULL, 10));
            return;
        }

        if(!vstrTokens[3].compare("roachNoiseDiodeDutyCycleOffDuration"))
        {
            sendNoiseDiodeDutyCycleOffDuration(i64Timestamp_us, strtol(vstrTokens[5].c_str(), NULL, 10));
            return;
        }

        if(!vstrTokens[3].compare("roachOverflowRegs"))
        {
            sendOverflowsRegs(i64Timestamp_us, strtol(vstrTokens[5].c_str(), NULL, 10));
            return;
        }

        if(!vstrTokens[3].compare("roachEth10GbEUp"))
        {
            sendEth10GbEUp(i64Timestamp_us, (bool)(0x00000001 & strtol(vstrTokens[5].c_str(), NULL, 10)));
            return;
        }

        if(!vstrTokens[3].compare("roachPPSCount"))
        {
            sendPPSCount(i64Timestamp_us, strtol(vstrTokens[5].c_str(), NULL, 10));
            return;
        }

        if(!vstrTokens[3].compare("roachClockFrequency"))
        {
            sendClockFrequency(i64Timestamp_us, strtol(vstrTokens[5].c_str(), NULL, 10));
            return;
        }
    }

    //    cout << "cRoachAcquisitionServerKATCPClient::processKATCPMessage(): Ignoring KATCP message: ";
    //    for(uint32_t ui = 0; ui < vstrTokens.size(); ui++ )
    //    {
    //        cout << vstrTokens[ui] << " ";
    //    }
    //    cout << endl;
}

void cRoachAcquisitionServerKATCPClient::sendStationControllerKATCPConnected(bool bConnected)
{
    boost::shared_lock<boost::shared_mutex> oLock;

    //Note the vector contains the base type callback handler pointer so cast to the derived version is this class
    //to call function added in the derived version of the callback handler interface class

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        cCallbackInterface *pHandler = dynamic_cast<cCallbackInterface*>(m_vpCallbackHandlers[ui]);
        pHandler->stationControllerKATCPConnected_callback(bConnected);
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        boost::shared_ptr<cCallbackInterface> pHandler = boost::dynamic_pointer_cast<cCallbackInterface>(m_vpCallbackHandlers_shared[ui]);
        pHandler->stationControllerKATCPConnected_callback(bConnected);
    }
}

void cRoachAcquisitionServerKATCPClient::sendRecordingStarted()
{
    boost::shared_lock<boost::shared_mutex> oLock;

    //Note the vector contains the base type callback handler pointer so cast to the derived version is this class
    //to call function added in the derived version of the callback handler interface class

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        cCallbackInterface *pHandler = dynamic_cast<cCallbackInterface*>(m_vpCallbackHandlers[ui]);
        pHandler->recordingStarted_callback();
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        boost::shared_ptr<cCallbackInterface> pHandler = boost::dynamic_pointer_cast<cCallbackInterface>(m_vpCallbackHandlers_shared[ui]);
        pHandler->recordingStarted_callback();
    }
}

void cRoachAcquisitionServerKATCPClient::sendRecordingStopped()
{
    boost::shared_lock<boost::shared_mutex> oLock;

    //Note the vector contains the base type callback handler pointer so cast to the derived version is this class
    //to call function added in the derived version of the callback handler interface class

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        cCallbackInterface *pHandler = dynamic_cast<cCallbackInterface*>(m_vpCallbackHandlers[ui]);
        pHandler->recordingStopped_callback();
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        boost::shared_ptr<cCallbackInterface> pHandler = boost::dynamic_pointer_cast<cCallbackInterface>(m_vpCallbackHandlers_shared[ui]);
        pHandler->recordingStopped_callback();
    }
}

void cRoachAcquisitionServerKATCPClient::sendRecordingInfoUpdate(const string &strFilename,
                                                                 int64_t i64StartTime_us, int64_t i64EllapsedTime_us,
                                                                 int64_t i64StopTime_us, int64_t i64TimeLeft_us,
                                                                 uint64_t u64CurrentFileSize_B, uint64_t u64DiskSpaceRemaining_B)
{
    boost::shared_lock<boost::shared_mutex> oLock;

    //Note the vector contains the base type callback handler pointer so cast to the derived version is this class
    //to call function added in the derived version of the callback handler interface class

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        cCallbackInterface *pHandler = dynamic_cast<cCallbackInterface*>(m_vpCallbackHandlers[ui]);
        pHandler->recordingInfoUpdate_callback(strFilename, i64StartTime_us, i64EllapsedTime_us, i64StopTime_us, i64TimeLeft_us, u64CurrentFileSize_B, u64DiskSpaceRemaining_B);
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        boost::shared_ptr<cCallbackInterface> pHandler = boost::dynamic_pointer_cast<cCallbackInterface>(m_vpCallbackHandlers_shared[ui]);
        pHandler->recordingInfoUpdate_callback(strFilename, i64StartTime_us, i64EllapsedTime_us, i64StopTime_us, i64TimeLeft_us, u64CurrentFileSize_B, u64DiskSpaceRemaining_B);
    }
}

void cRoachAcquisitionServerKATCPClient::sendActualAntennaAz(int64_t i64Timestamp_us,double dAzimuth_deg)
{
    boost::shared_lock<boost::shared_mutex> oLock;

    //Note the vector contains the base type callback handler pointer so cast to the derived version is this class
    //to call function added in the derived version of the callback handler interface class

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        cCallbackInterface *pHandler = dynamic_cast<cCallbackInterface*>(m_vpCallbackHandlers[ui]);
        pHandler->actualAntennaAz_callback(i64Timestamp_us, dAzimuth_deg);
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        boost::shared_ptr<cCallbackInterface> pHandler = boost::dynamic_pointer_cast<cCallbackInterface>(m_vpCallbackHandlers_shared[ui]);
        pHandler->actualAntennaAz_callback(i64Timestamp_us, dAzimuth_deg);
    }
}

void cRoachAcquisitionServerKATCPClient::sendActualAntennaEl(int64_t i64Timestamp_us,double dElevation_deg)
{
    boost::shared_lock<boost::shared_mutex> oLock;

    //Note the vector contains the base type callback handler pointer so cast to the derived version is this class
    //to call function added in the derived version of the callback handler interface class

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        cCallbackInterface *pHandler = dynamic_cast<cCallbackInterface*>(m_vpCallbackHandlers[ui]);
        pHandler->actualAntennaEl_callback(i64Timestamp_us, dElevation_deg);
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        boost::shared_ptr<cCallbackInterface> pHandler = boost::dynamic_pointer_cast<cCallbackInterface>(m_vpCallbackHandlers_shared[ui]);
        pHandler->actualAntennaEl_callback(i64Timestamp_us, dElevation_deg);
    }
}

void cRoachAcquisitionServerKATCPClient::sendActualSourceOffsetAz(int64_t i64Timestamp_us, double dAzimuthOffset_deg)
{
    boost::shared_lock<boost::shared_mutex> oLock;

    //Note the vector contains the base type callback handler pointer so cast to the derived version is this class
    //to call function added in the derived version of the callback handler interface class

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        cCallbackInterface *pHandler = dynamic_cast<cCallbackInterface*>(m_vpCallbackHandlers[ui]);
        pHandler->actualSourceOffsetAz_callback(i64Timestamp_us, dAzimuthOffset_deg);
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        boost::shared_ptr<cCallbackInterface> pHandler = boost::dynamic_pointer_cast<cCallbackInterface>(m_vpCallbackHandlers_shared[ui]);
        pHandler->actualSourceOffsetAz_callback(i64Timestamp_us, dAzimuthOffset_deg);
    }
}

void cRoachAcquisitionServerKATCPClient::sendActualSourceOffsetEl(int64_t i64Timestamp_us, double dElevationOffset_deg)
{
    boost::shared_lock<boost::shared_mutex> oLock;

    //Note the vector contains the base type callback handler pointer so cast to the derived version is this class
    //to call function added in the derived version of the callback handler interface class

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        cCallbackInterface *pHandler = dynamic_cast<cCallbackInterface*>(m_vpCallbackHandlers[ui]);
        pHandler->actualSourceOffsetEl_callback(i64Timestamp_us, dElevationOffset_deg);
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        boost::shared_ptr<cCallbackInterface> pHandler = boost::dynamic_pointer_cast<cCallbackInterface>(m_vpCallbackHandlers_shared[ui]);
        pHandler->actualSourceOffsetEl_callback(i64Timestamp_us, dElevationOffset_deg);
    }
}

void cRoachAcquisitionServerKATCPClient::sendFrequencyRFChan0(int64_t i64Timestamp_us, double dFreqencyRFChan0_MHz)
{
    boost::shared_lock<boost::shared_mutex> oLock;

    //Note the vector contains the base type callback handler pointer so cast to the derived version is this class
    //to call function added in the derived version of the callback handler interface class

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        cCallbackInterface *pHandler = dynamic_cast<cCallbackInterface*>(m_vpCallbackHandlers[ui]);
        pHandler->frequencyRFChan0_callback(i64Timestamp_us, dFreqencyRFChan0_MHz);
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        boost::shared_ptr<cCallbackInterface> pHandler = boost::dynamic_pointer_cast<cCallbackInterface>(m_vpCallbackHandlers_shared[ui]);
        pHandler->frequencyRFChan0_callback(i64Timestamp_us, dFreqencyRFChan0_MHz);
    }
}

void cRoachAcquisitionServerKATCPClient::sendFrequencyRFChan1(int64_t i64Timestamp_us, double dFreqencyRFChan1_MHz)
{
    boost::shared_lock<boost::shared_mutex> oLock;

    //Note the vector contains the base type callback handler pointer so cast to the derived version is this class
    //to call function added in the derived version of the callback handler interface class

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        cCallbackInterface *pHandler = dynamic_cast<cCallbackInterface*>(m_vpCallbackHandlers[ui]);
        pHandler->frequencyRFChan1_callback(i64Timestamp_us, dFreqencyRFChan1_MHz);
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        boost::shared_ptr<cCallbackInterface> pHandler = boost::dynamic_pointer_cast<cCallbackInterface>(m_vpCallbackHandlers_shared[ui]);
        pHandler->frequencyRFChan1_callback(i64Timestamp_us, dFreqencyRFChan1_MHz);
    }
}

void cRoachAcquisitionServerKATCPClient::sendRoachGatewareList(const vector<string> &vstrGatewareList)
{
    boost::shared_lock<boost::shared_mutex> oLock;

    //Note the vector contains the base type callback handler pointer so cast to the derived version is this class
    //to call function added in the derived version of the callback handler interface class

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        cCallbackInterface *pHandler = dynamic_cast<cCallbackInterface*>(m_vpCallbackHandlers[ui]);
        pHandler->roachGatewareList_callback(vstrGatewareList);
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        boost::shared_ptr<cCallbackInterface> pHandler = boost::dynamic_pointer_cast<cCallbackInterface>(m_vpCallbackHandlers_shared[ui]);
        pHandler->roachGatewareList_callback(vstrGatewareList);
    }
}

void cRoachAcquisitionServerKATCPClient::sendRoachKATCPConnected(bool bConnected)
{
    boost::shared_lock<boost::shared_mutex> oLock;

    //Note the vector contains the base type callback handler pointer so cast to the derived version is this class
    //to call function added in the derived version of the callback handler interface class

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        cCallbackInterface *pHandler = dynamic_cast<cCallbackInterface*>(m_vpCallbackHandlers[ui]);
        pHandler->roachKATCPConnected_callback(bConnected);
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        boost::shared_ptr<cCallbackInterface> pHandler = boost::dynamic_pointer_cast<cCallbackInterface>(m_vpCallbackHandlers_shared[ui]);
        pHandler->roachKATCPConnected_callback(bConnected);
    }
}

void cRoachAcquisitionServerKATCPClient::sendStokesEnabled(bool bConnected)
{
    boost::shared_lock<boost::shared_mutex> oLock;

    //Note the vector contains the base type callback handler pointer so cast to the derived version is this class
    //to call function added in the derived version of the callback handler interface class

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        cCallbackInterface *pHandler = dynamic_cast<cCallbackInterface*>(m_vpCallbackHandlers[ui]);
        pHandler->stokesEnabled_callback(bConnected);
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        boost::shared_ptr<cCallbackInterface> pHandler = boost::dynamic_pointer_cast<cCallbackInterface>(m_vpCallbackHandlers_shared[ui]);
        pHandler->stokesEnabled_callback(bConnected);
    }
}

void cRoachAcquisitionServerKATCPClient::sendAccumulationLength(int64_t i64Timestamp_us, uint32_t u32NFrames)
{
    boost::shared_lock<boost::shared_mutex> oLock;

    //Note the vector contains the base type callback handler pointer so cast to the derived version is this class
    //to call function added in the derived version of the callback handler interface class

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        cCallbackInterface *pHandler = dynamic_cast<cCallbackInterface*>(m_vpCallbackHandlers[ui]);
        pHandler->accumulationLength_callback(i64Timestamp_us, u32NFrames);
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        boost::shared_ptr<cCallbackInterface> pHandler = boost::dynamic_pointer_cast<cCallbackInterface>(m_vpCallbackHandlers_shared[ui]);
        pHandler->accumulationLength_callback(i64Timestamp_us, u32NFrames);
    }
}

void cRoachAcquisitionServerKATCPClient::sendCoarseChannelSelect(int64_t i64Timestamp_us, uint32_t u32ChannelNo)
{
    boost::shared_lock<boost::shared_mutex> oLock;

    //Note the vector contains the base type callback handler pointer so cast to the derived version is this class
    //to call function added in the derived version of the callback handler interface class

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        cCallbackInterface *pHandler = dynamic_cast<cCallbackInterface*>(m_vpCallbackHandlers[ui]);
        pHandler->coarseChannelSelect_callback(i64Timestamp_us, u32ChannelNo);
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        boost::shared_ptr<cCallbackInterface> pHandler = boost::dynamic_pointer_cast<cCallbackInterface>(m_vpCallbackHandlers_shared[ui]);
        pHandler->coarseChannelSelect_callback(i64Timestamp_us, u32ChannelNo);
    }
}

void cRoachAcquisitionServerKATCPClient::sendFrequencyFs(double dFrequencyFs_MHz)
{
    boost::shared_lock<boost::shared_mutex> oLock;

    //Note the vector contains the base type callback handler pointer so cast to the derived version is this class
    //to call function added in the derived version of the callback handler interface class

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        cCallbackInterface *pHandler = dynamic_cast<cCallbackInterface*>(m_vpCallbackHandlers[ui]);
        pHandler->frequencyFs_callback(dFrequencyFs_MHz);
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        boost::shared_ptr<cCallbackInterface> pHandler = boost::dynamic_pointer_cast<cCallbackInterface>(m_vpCallbackHandlers_shared[ui]);
        pHandler->frequencyFs_callback(dFrequencyFs_MHz);
    }
}

void cRoachAcquisitionServerKATCPClient::sendSizeOfCoarseFFT(uint32_t u32SizeOfCoarseFFT_nSamp)
{
    boost::shared_lock<boost::shared_mutex> oLock;

    //Note the vector contains the base type callback handler pointer so cast to the derived version is this class
    //to call function added in the derived version of the callback handler interface class

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        cCallbackInterface *pHandler = dynamic_cast<cCallbackInterface*>(m_vpCallbackHandlers[ui]);
        pHandler->sizeOfCoarseFFT_callback(u32SizeOfCoarseFFT_nSamp);
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        boost::shared_ptr<cCallbackInterface> pHandler = boost::dynamic_pointer_cast<cCallbackInterface>(m_vpCallbackHandlers_shared[ui]);
        pHandler->sizeOfCoarseFFT_callback(u32SizeOfCoarseFFT_nSamp);
    }
}

void cRoachAcquisitionServerKATCPClient::sendSizeOfFineFFT(uint32_t u32FineFFTSize_nSamp)
{
    boost::shared_lock<boost::shared_mutex> oLock;

    //Note the vector contains the base type callback handler pointer so cast to the derived version is this class
    //to call function added in the derived version of the callback handler interface class

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        cCallbackInterface *pHandler = dynamic_cast<cCallbackInterface*>(m_vpCallbackHandlers[ui]);
        pHandler->sizeOfFineFFT_callback(u32FineFFTSize_nSamp);
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        boost::shared_ptr<cCallbackInterface> pHandler = boost::dynamic_pointer_cast<cCallbackInterface>(m_vpCallbackHandlers_shared[ui]);
        pHandler->sizeOfFineFFT_callback(u32FineFFTSize_nSamp);
    }
}

void cRoachAcquisitionServerKATCPClient::sendCoarseFFTShiftMask(int64_t i64Timestamp_us, uint32_t u32ShiftMask)
{
    boost::shared_lock<boost::shared_mutex> oLock;

    //Note the vector contains the base type callback handler pointer so cast to the derived version is this class
    //to call function added in the derived version of the callback handler interface class

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        cCallbackInterface *pHandler = dynamic_cast<cCallbackInterface*>(m_vpCallbackHandlers[ui]);
        pHandler->coarseFFTShiftMask_callback(i64Timestamp_us, u32ShiftMask);
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        boost::shared_ptr<cCallbackInterface> pHandler = boost::dynamic_pointer_cast<cCallbackInterface>(m_vpCallbackHandlers_shared[ui]);
        pHandler->coarseFFTShiftMask_callback(i64Timestamp_us, u32ShiftMask);
    }
}

void cRoachAcquisitionServerKATCPClient::sendAttenuationADCChan0(int64_t i64Timestamp_us, double dADCAttenuationChan0_dB)
{
    boost::shared_lock<boost::shared_mutex> oLock;

    //Note the vector contains the base type callback handler pointer so cast to the derived version is this class
    //to call function added in the derived version of the callback handler interface class

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        cCallbackInterface *pHandler = dynamic_cast<cCallbackInterface*>(m_vpCallbackHandlers[ui]);
        pHandler->attenuationADCChan0_callback(i64Timestamp_us, dADCAttenuationChan0_dB);
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        boost::shared_ptr<cCallbackInterface> pHandler = boost::dynamic_pointer_cast<cCallbackInterface>(m_vpCallbackHandlers_shared[ui]);
        pHandler->attenuationADCChan0_callback(i64Timestamp_us, dADCAttenuationChan0_dB);
    }
}

void cRoachAcquisitionServerKATCPClient::sendAttenuationADCChan1(int64_t i64Timestamp_us, double dADCAttenuationChan1_dB)
{
    boost::shared_lock<boost::shared_mutex> oLock;

    //Note the vector contains the base type callback handler pointer so cast to the derived version is this class
    //to call function added in the derived version of the callback handler interface class

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        cCallbackInterface *pHandler = dynamic_cast<cCallbackInterface*>(m_vpCallbackHandlers[ui]);
        pHandler->attenuationADCChan1_callback(i64Timestamp_us, dADCAttenuationChan1_dB);
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        boost::shared_ptr<cCallbackInterface> pHandler = boost::dynamic_pointer_cast<cCallbackInterface>(m_vpCallbackHandlers_shared[ui]);
        pHandler->attenuationADCChan1_callback(i64Timestamp_us, dADCAttenuationChan1_dB);
    }
}

void cRoachAcquisitionServerKATCPClient::sendNoiseDiodeEnabled(int64_t i64Timestamp_us, bool bNoideDiodeEnabled)
{
    boost::shared_lock<boost::shared_mutex> oLock;

    //Note the vector contains the base type callback handler pointer so cast to the derived version is this class
    //to call function added in the derived version of the callback handler interface class

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        cCallbackInterface *pHandler = dynamic_cast<cCallbackInterface*>(m_vpCallbackHandlers[ui]);
        pHandler->noiseDiodeEnabled_callback(i64Timestamp_us, bNoideDiodeEnabled);
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        boost::shared_ptr<cCallbackInterface> pHandler = boost::dynamic_pointer_cast<cCallbackInterface>(m_vpCallbackHandlers_shared[ui]);
        pHandler->noiseDiodeEnabled_callback(i64Timestamp_us, bNoideDiodeEnabled);
    }
}

void cRoachAcquisitionServerKATCPClient::sendNoiseDiodeDutyCycleEnabled(int64_t i64Timestamp_us, bool bNoiseDiodeDutyCyleEnabled)
{
    boost::shared_lock<boost::shared_mutex> oLock;

    //Note the vector contains the base type callback handler pointer so cast to the derived version is this class
    //to call function added in the derived version of the callback handler interface class

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        cCallbackInterface *pHandler = dynamic_cast<cCallbackInterface*>(m_vpCallbackHandlers[ui]);
        pHandler->noiseDiodeDutyCycleEnabled_callback(i64Timestamp_us, bNoiseDiodeDutyCyleEnabled);
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        boost::shared_ptr<cCallbackInterface> pHandler = boost::dynamic_pointer_cast<cCallbackInterface>(m_vpCallbackHandlers_shared[ui]);
        pHandler->noiseDiodeDutyCycleEnabled_callback(i64Timestamp_us, bNoiseDiodeDutyCyleEnabled);
    }
}

void cRoachAcquisitionServerKATCPClient::sendNoiseDiodeDutyCycleOnDuration(int64_t i64Timestamp_us, uint32_t u32NAccums)
{
    boost::shared_lock<boost::shared_mutex> oLock;

    //Note the vector contains the base type callback handler pointer so cast to the derived version is this class
    //to call function added in the derived version of the callback handler interface class

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        cCallbackInterface *pHandler = dynamic_cast<cCallbackInterface*>(m_vpCallbackHandlers[ui]);
        pHandler->noiseDiodeDutyCycleOnDuration_callback(i64Timestamp_us, u32NAccums);
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        boost::shared_ptr<cCallbackInterface> pHandler = boost::dynamic_pointer_cast<cCallbackInterface>(m_vpCallbackHandlers_shared[ui]);
        pHandler->noiseDiodeDutyCycleOnDuration_callback(i64Timestamp_us, u32NAccums);
    }
}

void cRoachAcquisitionServerKATCPClient::sendNoiseDiodeDutyCycleOffDuration(int64_t i64Timestamp_us, uint32_t u32NAccums)
{
    boost::shared_lock<boost::shared_mutex> oLock;

    //Note the vector contains the base type callback handler pointer so cast to the derived version is this class
    //to call function added in the derived version of the callback handler interface class

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        cCallbackInterface *pHandler = dynamic_cast<cCallbackInterface*>(m_vpCallbackHandlers[ui]);
        pHandler->noiseDiodeDutyCycleOffDuration_callback(i64Timestamp_us, u32NAccums);
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        boost::shared_ptr<cCallbackInterface> pHandler = boost::dynamic_pointer_cast<cCallbackInterface>(m_vpCallbackHandlers_shared[ui]);
        pHandler->noiseDiodeDutyCycleOffDuration_callback(i64Timestamp_us, u32NAccums);
    }
}

void cRoachAcquisitionServerKATCPClient::sendOverflowsRegs(int64_t i64Timestamp_us, uint32_t u32OverflowRegs)
{
    boost::shared_lock<boost::shared_mutex> oLock;

    //Note the vector contains the base type callback handler pointer so cast to the derived version is this class
    //to call function added in the derived version of the callback handler interface class

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        cCallbackInterface *pHandler = dynamic_cast<cCallbackInterface*>(m_vpCallbackHandlers[ui]);
        pHandler->overflowsRegs_callback(i64Timestamp_us, u32OverflowRegs);
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        boost::shared_ptr<cCallbackInterface> pHandler = boost::dynamic_pointer_cast<cCallbackInterface>(m_vpCallbackHandlers_shared[ui]);
        pHandler->overflowsRegs_callback(i64Timestamp_us, u32OverflowRegs);
    }
}

void cRoachAcquisitionServerKATCPClient::sendEth10GbEUp(int64_t i64Timestamp_us, bool bEth10GbEUp)
{
    boost::shared_lock<boost::shared_mutex> oLock;

    //Note the vector contains the base type callback handler pointer so cast to the derived version is this class
    //to call function added in the derived version of the callback handler interface class

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        cCallbackInterface *pHandler = dynamic_cast<cCallbackInterface*>(m_vpCallbackHandlers[ui]);
        pHandler->eth10GbEUp_callback(i64Timestamp_us, bEth10GbEUp);
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        boost::shared_ptr<cCallbackInterface> pHandler = boost::dynamic_pointer_cast<cCallbackInterface>(m_vpCallbackHandlers_shared[ui]);
        pHandler->eth10GbEUp_callback(i64Timestamp_us, bEth10GbEUp);
    }
}

void cRoachAcquisitionServerKATCPClient::sendPPSCount(int64_t i64Timestamp_us, uint32_t u32PPSCount)
{
    boost::shared_lock<boost::shared_mutex> oLock;

    //Note the vector contains the base type callback handler pointer so cast to the derived version is this class
    //to call function added in the derived version of the callback handler interface class

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        cCallbackInterface *pHandler = dynamic_cast<cCallbackInterface*>(m_vpCallbackHandlers[ui]);
        pHandler->ppsCount_callback(i64Timestamp_us, u32PPSCount);
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        boost::shared_ptr<cCallbackInterface> pHandler = boost::dynamic_pointer_cast<cCallbackInterface>(m_vpCallbackHandlers_shared[ui]);
        pHandler->ppsCount_callback(i64Timestamp_us, u32PPSCount);
    }
}

void cRoachAcquisitionServerKATCPClient::sendClockFrequency(int64_t i64Timestamp_us, uint32_t u32ClockFrequency_Hz)
{
    boost::shared_lock<boost::shared_mutex> oLock;

    //Note the vector contains the base type callback handler pointer so cast to the derived version is this class
    //to call function added in the derived version of the callback handler interface class

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        cCallbackInterface *pHandler = dynamic_cast<cCallbackInterface*>(m_vpCallbackHandlers[ui]);
        pHandler->clockFrequency_callback(i64Timestamp_us, u32ClockFrequency_Hz);
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        boost::shared_ptr<cCallbackInterface> pHandler = boost::dynamic_pointer_cast<cCallbackInterface>(m_vpCallbackHandlers_shared[ui]);
        pHandler->clockFrequency_callback(i64Timestamp_us, u32ClockFrequency_Hz);
    }
}

void cRoachAcquisitionServerKATCPClient::requestStartRecording(const string &strFilenamePrefix, int64_t i64StartTime_us, int64_t i64Duration_us)
{
    stringstream oSS;
    oSS << string("?startRecording");

    oSS << string(" ");
    if(strFilenamePrefix.length())
        oSS << strFilenamePrefix;
    else
        oSS << string("\\@"); //Empty string escape sequence for KATCP. Note actually \@ on the telnet line.

    oSS << string(" ");
    oSS << i64StartTime_us;

    oSS << string(" ");
    oSS << i64Duration_us;

    oSS << string("\n");

    sendKATCPMessage(oSS.str());

    cout << "cRoachAcquisitionServerKATCPClient::requestStartRecording() Send start recording request : " << oSS.str().c_str() << endl;
}

void cRoachAcquisitionServerKATCPClient::requestStopRecording()
{
    sendKATCPMessage(string("?stopRecording\n"));
}

void cRoachAcquisitionServerKATCPClient::requestRecordingStatus()
{
    sendKATCPMessage(string("?getRecordingStatus\n"));
}

void cRoachAcquisitionServerKATCPClient::requestRecordingInfoUpdate()
{
    sendKATCPMessage(string("?getRecordingInfo\n"));
}

void cRoachAcquisitionServerKATCPClient::requestRoachGatewareList()
{
    sendKATCPMessage("?getRoachGatewareList\n");
}

void cRoachAcquisitionServerKATCPClient::requestRoachProgram(const std::string strScriptPath)
{
    stringstream oSS;
    oSS << "?programRoach ";
    oSS << strScriptPath;
    oSS << "\n";

    sendKATCPMessage(oSS.str());
}

void cRoachAcquisitionServerKATCPClient::requestRoachSetStokesEnabled(bool bEnabled)
{
    stringstream oSS;
    oSS << "?setRoachStokesEnabled ";
    if(bEnabled)
    {
        oSS << "1";
    }
    else
    {
        oSS << "0";
    }
    oSS << "\n";

    sendKATCPMessage(oSS.str());
}

void cRoachAcquisitionServerKATCPClient::requestRoachSetAccumulationLength(uint32_t u32Length_nFrames)
{
    stringstream oSS;
    oSS << "?setRoachAccumulationLength ";
    oSS << u32Length_nFrames;
    oSS << "\n";

    sendKATCPMessage(oSS.str());
}

void cRoachAcquisitionServerKATCPClient::requestRoachSetCoarseChannelSelect(uint32_t u32ChannelNo)
{
    stringstream oSS;
    oSS << "?setRoachCoarseChannelSelect ";
    oSS << u32ChannelNo;
    oSS << "\n";

    sendKATCPMessage(oSS.str());
}

void cRoachAcquisitionServerKATCPClient::requestRoachSetCoarseFFTShiftMask(uint32_t u32FFTShiftMask)
{
    stringstream oSS;
    oSS << "?setRoachCoarseFFTMask ";
    oSS << u32FFTShiftMask;
    oSS << "\n";

    sendKATCPMessage(oSS.str());
}

void cRoachAcquisitionServerKATCPClient::requestRoachSetADC0Attenuation(uint32_t u32ADC0Attenuation)
{
    stringstream oSS;
    oSS << "?setRoachADC0Attenuation ";
    oSS << u32ADC0Attenuation;
    oSS << "\n";

    sendKATCPMessage(oSS.str());
}

void cRoachAcquisitionServerKATCPClient::requestRoachSetADC1Attenuation(uint32_t u32ADC1Attenuation)
{
    stringstream oSS;
    oSS << "?setRoachADC1Attenuation ";
    oSS << u32ADC1Attenuation;
    oSS << "\n";

    sendKATCPMessage(oSS.str());
}

void cRoachAcquisitionServerKATCPClient::requestRoachSetNoiseDiodeEnabled(bool bEnabled)
{
    stringstream oSS;
    oSS << "?setRoachNoiseDiodeEnabled ";
    if(bEnabled)
    {
        oSS << "1";
    }
    else
    {
        oSS << "0";
    }
    oSS << "\n";

    sendKATCPMessage(oSS.str());
}

void cRoachAcquisitionServerKATCPClient::requestRoachSetNoiseDiodeDutyCycleEnabled(bool bEnabled)
{
    stringstream oSS;
    oSS << "?setRoachNoiseDiodeDutyCycleEnabled ";
    if(bEnabled)
    {
        oSS << "1";
    }
    else
    {
        oSS << "0";
    }
    oSS << "\n";

    sendKATCPMessage(oSS.str());
}

void cRoachAcquisitionServerKATCPClient::requestRoachSetNoiseDiodeDutyCycleOnDuration(uint32_t u32NAccums)
{
    stringstream oSS;
    oSS << "?setRoachNoiseDiodeDutyCycleOnDuration ";
    oSS << u32NAccums;
    oSS << "\n";

    sendKATCPMessage(oSS.str());
}

void cRoachAcquisitionServerKATCPClient::requestRoachSetNoiseDiodeDutyCycleOffDuration(uint32_t u32NAccums)
{
    stringstream oSS;
    oSS << "?setRoachNoiseDiodeDutyCycleOffDuration ";
    oSS << u32NAccums;
    oSS << "\n";

    sendKATCPMessage(oSS.str());
}

void cRoachAcquisitionServerKATCPClient::subscribeToSensors()
{
    cout << "cRoachAcquisitionServerKATCPClient::subscribeToSensors(): Subscribing to sensors on KATCP server." << endl;

    sendKATCPMessage(string("?sensor-sampling stationControllerConnected period 1000\n"));
    sendKATCPMessage(string("?sensor-sampling actualAntennaAz period 1000\n"));
    sendKATCPMessage(string("?sensor-sampling actualAntennaEl period 1000\n"));
    sendKATCPMessage(string("?sensor-sampling actualSourceOffsetAz period 1000\n"));
    sendKATCPMessage(string("?sensor-sampling actualSourceOffsetEl period 1000\n"));
    sendKATCPMessage(string("?sensor-sampling frequencyRFChan0 period 1000\n"));
    sendKATCPMessage(string("?sensor-sampling frequencyRFChan1 period 1000\n"));
    sendKATCPMessage(string("?sensor-sampling roachConnected period 1000\n"));
    sendKATCPMessage(string("?sensor-sampling roachStokesEnabled period 1000\n"));
    sendKATCPMessage(string("?sensor-sampling roachAccumulationLength period 1000\n"));
    sendKATCPMessage(string("?sensor-sampling roachCoarseChannelSelect period 1000\n"));
    sendKATCPMessage(string("?sensor-sampling roachFrequencyFs period 1000\n"));
    sendKATCPMessage(string("?sensor-sampling roachSizeOfCoarseFFT period 1000\n"));
    sendKATCPMessage(string("?sensor-sampling roachSizeOfFineFFT period 1000\n"));
    sendKATCPMessage(string("?sensor-sampling roachCoarseFFTShiftMask period 1000\n"));
    sendKATCPMessage(string("?sensor-sampling roachAttenuationADCChan0 period 1000\n"));
    sendKATCPMessage(string("?sensor-sampling roachAttenuationADCChan1 period 1000\n"));
    sendKATCPMessage(string("?sensor-sampling roachNoiseDiodeEnabled period 1000\n"));
    sendKATCPMessage(string("?sensor-sampling roachNoiseDiodeDutyCycleEnabled period 1000\n"));
    sendKATCPMessage(string("?sensor-sampling roachNoiseDiodeDutyCycleOnDuration period 1000\n"));
    sendKATCPMessage(string("?sensor-sampling roachNoiseDiodeDutyCycleOffDuration period 1000\n"));
    sendKATCPMessage(string("?sensor-sampling roachOverflowRegs period 1000\n"));
    sendKATCPMessage(string("?sensor-sampling roachEth10GbEUp period 1000\n"));
    sendKATCPMessage(string("?sensor-sampling roachPPSCount period 1000\n"));
    sendKATCPMessage(string("?sensor-sampling roachClockFrequency period 1000\n"));
}
