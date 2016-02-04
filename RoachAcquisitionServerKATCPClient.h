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
                                                                                     uint64_t u64DiskSpaceRemaining_B) = 0;
    };

    cRoachAcquisitionServerKATCPClient(const std::string &strServerAddress, uint16_t u16Port = 7147);
    cRoachAcquisitionServerKATCPClient();
    ~cRoachAcquisitionServerKATCPClient();

    //Client requests
    void                                                requestStartRecording(const std::string &strFilenamePrefix = std::string(""),
                                                                              int64_t i64StartTime_us = 0, int64_t i64Duration_us = 0);
    void                                                requestStopRecording();
    void                                                requestRecordingStatus();
    void                                                requestRecordingInfoUpdate();

private:
    //Implement from base class
    void                                                processKATCPMessage(const std::vector<std::string> &vstrMessageTokens);

    //Notifications sent to all callback handlers
    void                                                sendRecordingStarted();
    void                                                sendRecordingStopped();
    void                                                sendRecordingInfoUpdate(const std::string &strFilename,
                                                                            int64_t i64StartTime_us, int64_t i64EllapsedTime_us, int64_t
                                                                            i64StopTime_us, int64_t i64TimeLeft_us,
                                                                            uint64_t u64DiskSpaceRemaining_B);

};

#endif // ROACH_ACQUISITION_SERVER_KATCP_CLIENT_H
