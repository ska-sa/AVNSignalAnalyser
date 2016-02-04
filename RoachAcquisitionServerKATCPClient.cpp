//System includes

//Library includes

//Local includes
#include "RoachAcquisitionServerKATCPClient.h"

using namespace std;

cRoachAcquisitionServerKATCPClient::cRoachAcquisitionServerKATCPClient(const string &strServerAddress, uint16_t u16Port) :
    cKATCPClientBase(strServerAddress, u16Port)
{
}

cRoachAcquisitionServerKATCPClient::cRoachAcquisitionServerKATCPClient() :
    cKATCPClientBase()
{
}

cRoachAcquisitionServerKATCPClient::~cRoachAcquisitionServerKATCPClient()
{
}

void cRoachAcquisitionServerKATCPClient::processKATCPMessage(const vector<string> &vstrTokens)
{
    try
    {
        if( !vstrTokens[0].compare(0, 17, "#recordingStopped") )
        {
            sendRecordingStopped();
            return;
        }
    }
    catch(out_of_range &oError)
    {
    }


    try
    {
        if(!vstrTokens[0].compare(0, 17, "#recordingStarted"))
        {
            sendRecordingStarted();
            return;
        }
    }
    catch(out_of_range &oError)
    {
    }

    try
    {
        if(!vstrTokens[0].compare(0, 14, "#recordingInfo"))
        {
            if(vstrTokens.size() < 7)
                return;

            int64_t i64StartTime_us             = strtoll(vstrTokens[2].c_str(), NULL, 10);
            int64_t i64EllapsedTime_us          = strtoll(vstrTokens[3].c_str(), NULL, 10);
            int64_t i64StopTime_us              = strtoll(vstrTokens[4].c_str(), NULL, 10);
            int64_t i64TimeLeft_us              = strtoll(vstrTokens[5].c_str(), NULL, 10);
            uint64_t u64DiskSpaceRemaining_B    = strtoull(vstrTokens[6].c_str(), NULL, 10);

            sendRecordingInfoUpdate(vstrTokens[1], i64StartTime_us, i64EllapsedTime_us, i64StopTime_us, i64TimeLeft_us, u64DiskSpaceRemaining_B);

            return;
        }
    }
    catch(out_of_range &oError)
    {
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

void cRoachAcquisitionServerKATCPClient::sendRecordingInfoUpdate(const string &strFilename, int64_t i64StartTime_us, int64_t i64EllapsedTime_us,
                                       int64_t i64StopTime_us, int64_t i64TimeLeft_us, uint64_t u64DiskSpaceRemaining_B)
{
    boost::shared_lock<boost::shared_mutex> oLock;

    //Note the vector contains the base type callback handler pointer so cast to the derived version is this class
    //to call function added in the derived version of the callback handler interface class

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        cCallbackInterface *pHandler = dynamic_cast<cCallbackInterface*>(m_vpCallbackHandlers[ui]);
        pHandler->recordingInfoUpdate_callback(strFilename, i64StartTime_us, i64EllapsedTime_us, i64StopTime_us, i64TimeLeft_us, u64DiskSpaceRemaining_B);
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        boost::shared_ptr<cCallbackInterface> pHandler = boost::dynamic_pointer_cast<cCallbackInterface>(m_vpCallbackHandlers_shared[ui]);
        pHandler->recordingInfoUpdate_callback(strFilename, i64StartTime_us, i64EllapsedTime_us, i64StopTime_us, i64TimeLeft_us, u64DiskSpaceRemaining_B);
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
