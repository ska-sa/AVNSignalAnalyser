
//System includes
#include <iostream>
#include <stdlib.h>
#include <sstream>

//Library includes
#ifndef Q_MOC_RUN //Qt's MOC and Boost have some issues don't let MOC process boost headers
#include <boost/make_shared.hpp>
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/thread.hpp>
#endif

//Local includes
#include "KATCPClient.h"

using namespace std;

cKATCPClient::cKATCPClient(const string &strServerAddress, uint16_t u16Port) :
    m_bDisconnectFlag(false)
{
    connect(strServerAddress, u16Port);
}

cKATCPClient::cKATCPClient() :
    m_bDisconnectFlag(false)
{
}

cKATCPClient::~cKATCPClient()
{
    disconnect();
}

void cKATCPClient::connect(const string &strServerAddress, uint16_t u16Port)
{
    cout << "cKATCPClient::connect() Starting KATCP server" << endl;

    //Store config parameters in members
    m_strServerAddress      = strServerAddress;
    m_u16Port               = u16Port;

    //Launch KATCP client in a new thread
    m_pSocketReadThread.reset(new boost::thread(&cKATCPClient::threadReadFunction, this));
    m_pSocketWriteThread.reset(new boost::thread(&cKATCPClient::threadWriteFunction, this));
}

void cKATCPClient::disconnect()
{
    cout << "cKATCPClient::disconnect() Disconnecting KATCP client..." << endl;

    {
        boost::unique_lock<boost::shared_mutex> oLock(m_oFlagMutex);

        m_bDisconnectFlag = true;
        m_pSocket->cancelCurrrentOperations();

        m_oConditionWriteQueueNoLongerEmpty.notify_all();
    }

    m_pSocketReadThread->join();
    m_pSocketReadThread.reset();

    m_pSocketWriteThread->join();
    m_pSocketWriteThread.reset();

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        m_vpCallbackHandlers[ui]->connected_callback(false);
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        m_vpCallbackHandlers[ui]->connected_callback(false);
    }

    cout << "cKATCPClient::disconnect() KATCP disconnected." << endl;
}

bool cKATCPClient::disconnectRequested()
{
    //Thread safe function to check the disconnect flag
    boost::shared_lock<boost::shared_mutex> oLock(m_oFlagMutex);

    return m_bDisconnectFlag;
}

void cKATCPClient::threadReadFunction()
{
    //Connect the socket
    m_pSocket.reset(new cInterruptibleBlockingTCPSocket());
    bool bFullMessage = false;
    string strKATCPMessage;

    while(!disconnectRequested())
    {
        if(m_pSocket->openAndConnect(m_strServerAddress, m_u16Port, 500))
            break;

        cout << "cKATCPClient::threadFunction() Reached timeout attempting to connect to server " << m_strServerAddress << ":" << m_u16Port << ". Retrying in 0.5 seconds..." << endl;
        boost::this_thread::sleep(boost::posix_time::milliseconds(500));
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        m_vpCallbackHandlers[ui]->connected_callback(true);
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        m_vpCallbackHandlers[ui]->connected_callback(true);
    }

    cout << "cKATCPClient::threadFunction() successfully connected KATCP server " << m_strServerAddress << ":" << m_u16Port << "." << endl;

    requestRecordingStatus();

    while(!disconnectRequested())
    {
        strKATCPMessage.clear();

        do
        {
            bFullMessage = m_pSocket->readUntil( strKATCPMessage, string("\n"), 500);

            if(disconnectRequested())
                return;

            //readUntil function will append to the message string if each iteration if the stop character is not reached.
        }
        while(!bFullMessage);

        processKATCPMessage(strKATCPMessage);
    }
}

void cKATCPClient::threadWriteFunction()
{
    string strMessageToSend;

    while(!disconnectRequested())
    {
        {
            boost::unique_lock<boost::mutex> oLock(m_oWriteQueueMutex);

            //If the queue is empty wait for data
            if(!m_qstrWriteQueue.size())
            {
                if(!m_oConditionWriteQueueNoLongerEmpty.timed_wait(oLock, boost::posix_time::milliseconds(500)) )
                {
                    //Timeout after 500 ms then check again (Loop restarts)
                    continue;
                }
            }

            //Check size again
            if(!m_qstrWriteQueue.size())
                continue;

            //Make a of copy of the string and pop it from the queue
            strMessageToSend = m_qstrWriteQueue.back();
            m_qstrWriteQueue.pop();
        }

        //Write the data to the socket
        //reattempt to send on timeout or failure
        while(!m_pSocket->write(strMessageToSend, 500));
        {
            //Check for shutdown in between attempts
            if(disconnectRequested())
                return;
        }
    }
}

void cKATCPClient::recordingStarted()
{
    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        m_vpCallbackHandlers[ui]->recordingStarted_callback();
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        m_vpCallbackHandlers[ui]->recordingStarted_callback();
    }
}

void cKATCPClient::recordingStopped()
{
    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        m_vpCallbackHandlers[ui]->recordingStopped_callback();
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        m_vpCallbackHandlers[ui]->recordingStopped_callback();
    }
}

void cKATCPClient::recordingInfoUpdate(const string &strFilename, int64_t i64StartTime_us, int64_t i64EllapsedTime_us, int64_t i64StopTime_us, int64_t i64TimeLeft_us)
{
    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        m_vpCallbackHandlers[ui]->recordingInfoUpdate_callback(strFilename, i64StartTime_us, i64EllapsedTime_us, i64StopTime_us, i64TimeLeft_us);
    }

    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size(); ui++)
    {
        m_vpCallbackHandlers[ui]->recordingInfoUpdate_callback(strFilename, i64StartTime_us, i64EllapsedTime_us, i64StopTime_us, i64TimeLeft_us);
    }
}

void cKATCPClient::requestStartRecording(const string &strFilenamePrefix, int64_t i64StartTime_us, int64_t i64Duration_us)
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

    cout << "cKATCPClient::requestStartRecording() Send start recording request : " << oSS.str().c_str() << endl;
}

void cKATCPClient::requestStopRecording()
{
    sendKATCPMessage(string("?stopRecording\n"));
}

void cKATCPClient::requestRecordingStatus()
{
    sendKATCPMessage(string("?getRecordingStatus\n"));
}

void cKATCPClient::requestRecordingInfoUpdate()
{
    sendKATCPMessage(string("?getRecordingInfo\n"));
}

void cKATCPClient::sendKATCPMessage(const std::string &strMessage)
{
    boost::unique_lock<boost::mutex> oLock(m_oWriteQueueMutex);

    m_qstrWriteQueue.push(strMessage);

    //If the queue has gone from being empty to not-empty notify the writing thread.
    if(m_qstrWriteQueue.size() == 1)
    {
        m_oConditionWriteQueueNoLongerEmpty.notify_one();
    }
}

void cKATCPClient::processKATCPMessage(const string &strMessage)
{
    //cout << "Got KATCP message: " << strMessage << endl;

    //Tokenise string and store tokens in a vector
    vector<string> vstrTokens = tokeniseString(strMessage, string(" "));

    if(!vstrTokens.size())
        return;

    try
    {
        if( !vstrTokens[0].compare(0, 17, "#recordingStopped") )
        {
            recordingStopped();
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
            recordingStarted();
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
            if(vstrTokens.size() < 6)
                return;

            int64_t i64StartTime_us     = strtoll(vstrTokens[2].c_str(), NULL, 10);
            int64_t i64EllapsedTime_us  = strtoll(vstrTokens[3].c_str(), NULL, 10);
            int64_t i64StopTime_us      = strtoll(vstrTokens[4].c_str(), NULL, 10);
            int64_t i64TimeLeft_us      = strtoll(vstrTokens[5].c_str(), NULL, 10);

            recordingInfoUpdate(vstrTokens[1], i64StartTime_us, i64EllapsedTime_us, i64StopTime_us, i64TimeLeft_us);

            return;
        }
    }
    catch(out_of_range &oError)
    {
    }
}

void cKATCPClient::registerCallbackHandler(cCallbackInterface *pNewHandler)
{
    boost::unique_lock<boost::shared_mutex> oLock(m_oCallbackHandlersMutex);

    m_vpCallbackHandlers.push_back(pNewHandler);

    cout << "cKATCPClient::::registerCallbackHandler(): Successfully registered callback handler: " << pNewHandler << endl;
}

void cKATCPClient::registerCallbackHandler(boost::shared_ptr<cCallbackInterface> pNewHandler)
{
    boost::unique_lock<boost::shared_mutex> oLock(m_oCallbackHandlersMutex);

    m_vpCallbackHandlers_shared.push_back(pNewHandler);

    cout << "cKATCPClient::::registerCallbackHandler(): Successfully registered callback handler: " << pNewHandler.get() << endl;
}

void cKATCPClient::deregisterCallbackHandler(cCallbackInterface *pHandler)
{
    boost::unique_lock<boost::shared_mutex> oLock(m_oCallbackHandlersMutex);
    bool bSuccess = false;

    //Search for matching pointer values and erase
    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size();)
    {
        if(m_vpCallbackHandlers[ui] == pHandler)
        {
            m_vpCallbackHandlers.erase(m_vpCallbackHandlers.begin() + ui);

            cout << "cKATCPClient::deregisterCallbackHandler(): Deregistered callback handler: " << pHandler << endl;
            bSuccess = true;
        }
        else
        {
            ui++;
        }
    }

    if(!bSuccess)
    {
        cout << "cKATCPClient::::deregisterCallbackHandler(): Warning: Deregistering callback handler: " << pHandler << " failed. Object instance not found." << endl;
    }
}

void cKATCPClient::deregisterCallbackHandler(boost::shared_ptr<cCallbackInterface> pHandler)
{
    boost::unique_lock<boost::shared_mutex> oLock(m_oCallbackHandlersMutex);
    bool bSuccess = false;

    //Search for matching pointer values and erase
    for(uint32_t ui = 0; ui < m_vpCallbackHandlers_shared.size();)
    {
        if(m_vpCallbackHandlers_shared[ui].get() == pHandler.get())
        {
            m_vpCallbackHandlers_shared.erase(m_vpCallbackHandlers_shared.begin() + ui);

            cout << "cKATCPClient::deregisterCallbackHandler(): Deregistered callback handler: " << pHandler.get() << endl;
            bSuccess = true;
        }
        else
        {
            ui++;
        }
    }

    if(!bSuccess)
    {
        cout << "cKATCPClient::deregisterCallbackHandler(): Warning: Deregistering callback handler: " << pHandler.get() << " failed. Object instance not found." << endl;
    }
}

std::vector<std::string> cKATCPClient::tokeniseString(const std::string &strInputString, const std::string &strSeparators)
{
    //This funciton is not complete efficient due to extra memory copies of filling the std::vector
    //It will also be copied again on return.
    //It does simply the calling code and should be adequate in the context of most KATCP control clients.

    boost::char_separator<char> oSeparators(strSeparators.c_str());
    boost::tokenizer< boost::char_separator<char> > oTokens(strInputString, oSeparators);

    vector<string> vstrTokens;

    for(boost::tokenizer< boost::char_separator<char> >::iterator it = oTokens.begin(); it != oTokens.end(); ++it)
    {
        vstrTokens.push_back(*it);
    }

    return vstrTokens;
}
