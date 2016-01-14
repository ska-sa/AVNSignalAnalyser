#ifndef KATCP_CLIENT_H
#define KATCP_CLIENT_H

//System includes
#include <vector>
#include <queue>

//Library includes
#ifndef Q_MOC_RUN //Qt's MOC and Boost have some issues don't let MOC process boost headers
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#endif

//Local includes
#include "AVNUtilLibs/Sockets/InterruptibleBlockingSockets/InterruptibleBlockingTCPSocket.h"

//This KATCP client is implemented manually using a client TCP socket and not the KATCP library.
//This makes portability a bit better especially with Windows.

class cKATCPClient
{
public:
    class cCallbackInterface
    {
    public:
        virtual void                                    connected_callback(bool bConnected) = 0;
        virtual void                                    recordingStarted_callback() = 0;
        virtual void                                    recordingStopped_callback() = 0;
        virtual void                                    recordingInfoUpdate_callback(const std::string &strFilename,
                                                                                     int64_t i64StartTime_us, int64_t i64EllapsedTime_us, int64_t i64StopTime_us, int64_t i64TimeLeft_us) = 0;
    };

    cKATCPClient(const std::string &strServerAddress, uint16_t u16Port = 7147);
    cKATCPClient();
    ~cKATCPClient();

    void                                                connect(const std::string &strServerAddress, uint16_t u16Port);
    void                                                disconnect();

    //Client requests
    void                                                requestStartRecording(const std::string &strFilenamePrefix = std::string(""),
                                                                              int64_t i64StartTime_us = 0, int64_t i64Duration_us = 0);
    void                                                requestStopRecording();
    void                                                requestRecordingStatus();
    void                                                requestRecordingInfoUpdate();

    void                                                sendKATCPMessage(const std::string &strMessage); //Send a custom KATCP message to the connected peer


    //Callback handler registration
    void                                                registerCallbackHandler(cCallbackInterface *pNewHandler);
    void                                                registerCallbackHandler(boost::shared_ptr<cCallbackInterface> pNewHandler);
    void                                                deregisterCallbackHandler(cCallbackInterface *pHandler);
    void                                                deregisterCallbackHandler(boost::shared_ptr<cCallbackInterface> pHandler);

    std::vector<std::string>                            tokeniseString(const std::string &strInputString, const std::string &strSeperators);

protected:
    void                                                threadReadFunction();
    void                                                threadWriteFunction();
    void                                                processKATCPMessage(const std::string &strMessage);

    //Server informs
    void                                                recordingStarted();
    void                                                recordingStopped();
    void                                                recordingInfoUpdate(const std::string &strFilename,
                                                                            int64_t i64StartTime_us, int64_t i64EllapsedTime_us, int64_t i64StopTime_us, int64_t i64TimeLeft_us);

    //Threads
    boost::scoped_ptr<boost::thread>                    m_pSocketReadThread;
    boost::scoped_ptr<boost::thread>                    m_pSocketWriteThread;

    //Sockets
    boost::scoped_ptr<cInterruptibleBlockingTCPSocket>  m_pSocket;

    //Members description operation state
    std::string                                         m_strServerAddress;
    uint16_t                                            m_u16Port;

    //Other variables
    bool                                                m_bDisconnectFlag;
    boost::shared_mutex                                 m_oFlagMutex;
    bool                                                disconnectRequested();

    std::queue<std::string>                             m_qstrWriteQueue;
    boost::condition_variable                           m_oConditionWriteQueueNoLongerEmpty;
    boost::mutex                                        m_oWriteQueueMutex;

    //Callback handlers
    std::vector<cCallbackInterface*>                    m_vpCallbackHandlers;
    std::vector<boost::shared_ptr<cCallbackInterface> > m_vpCallbackHandlers_shared;
    boost::shared_mutex                                 m_oCallbackHandlersMutex;
};

#endif // KATCP_CLIENT_H
