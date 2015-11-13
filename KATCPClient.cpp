
//System includes
#include <iostream>
#include <stdlib.h>

//Library includes
#ifndef Q_MOC_RUN //Qt's MOC and Boost have some issues don't let MOC process boost headers
#include <boost/make_shared.hpp>
#endif

//Local includes
#include "KATCPClient.h"

using namespace std;

cKATCPClient::cKATCPClient(const std::string &strServerAddress, uint16_t u16Port) :
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

void cKATCPClient::connect(const std::string &strServerAddress, uint16_t u16Port)
{
    cout << "cKATCPClient::connect() Starting KATCP server" << endl;

    //Store config parameters in members
    m_strServerAddress      = strServerAddress;
    m_u16Port               = u16Port;

    //Launch KATCP client in a new thread
    m_pSocketThread.reset(new boost::thread(&cKATCPClient::threadFunction, this));
}

void cKATCPClient::disconnect()
{
    cout << "cKATCPClient::disconnect() Disconnecting KATCP client..." << endl;

    {
        boost::unique_lock<boost::shared_mutex> oLock(m_oFlagMutex);

        m_bDisconnectFlag = true;
    }

    m_pSocketThread->join();
    m_pSocketThread.reset();

    cout << "cKATCPClient::disconnect() KATCP disconnected." << endl;
}

void cKATCPClient::threadFunction()
{
    //Connect the socket
    m_pSocket.reset(new cInterruptibleBlockingTCPSocket());
    bool bFullMessage = false;
    string strKATCPMessage;

    while(!disconnectRequested())
    {
        if(m_pSocket->openAndConnect(m_strServerAddress, m_u16Port, 500))
            break;

        cout << "Reached timeout attempting to connect to server " << m_strServerAddress << ":" << m_u16Port << ". Retrying..." << endl;
    }

    while(!disconnectRequested())
    {
        strKATCPMessage.clear();

        do
        {
            bFullMessage = m_pSocket->readUntil( strKATCPMessage, string("\r\n"), 2000 );
        }
        while(!bFullMessage);

        processKATCPMessage(strKATCPMessage);
    }
}

void cKATCPClient::recordingStarted()
{
    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
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
}

void cKATCPClient::recordingInfoUpdate(int64_t i64StartTime_us, int64_t i64StopTime_us, int64_t i64EllapsedTime_us, int64_t i64TimeLeft_us)
{
    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size(); ui++)
    {
        m_vpCallbackHandlers[ui]->recordingInfoUpdate_callback(i64StartTime_us, i64StopTime_us, i64EllapsedTime_us, i64TimeLeft_us);
    }
}

void cKATCPClient::registerCallbackHandler(boost::shared_ptr<cCallbackInterface> pNewHandler)
{
    boost::unique_lock<boost::shared_mutex> oLock(m_oCallbackHandlersMutex);

    m_vpCallbackHandlers.push_back(pNewHandler);

    cout << "cKATCPClient::registerCallbackHandler(): Successfully registered callback handler: " << pNewHandler.get() << endl;
}

void cKATCPClient::deregisterCallbackHandler(boost::shared_ptr<cCallbackInterface> pHandler)
{
    boost::unique_lock<boost::shared_mutex> oLock(m_oCallbackHandlersMutex);
    bool bSuccess = false;

    //Search for matching pointer values and erase
    for(uint32_t ui = 0; ui < m_vpCallbackHandlers.size();)
    {
        if(m_vpCallbackHandlers[ui].get() == pHandler.get())
        {
            m_vpCallbackHandlers.erase(m_vpCallbackHandlers.begin() + ui);

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

bool cKATCPClient::disconnectRequested()
{
    boost::shared_lock<boost::shared_mutex> oLock(m_oFlagMutex);

    return m_bDisconnectFlag;
}

void cKATCPClient::processKATCPMessage(const std::string &strMessage)
{
    cout << "Got KATCP message" << strMessage << endl;

    try
    {
        if(!strMessage.compare(0, 16, "recordingStopped"))
        {
            recordingStopped();
            return;
        }
    }
    catch(std::out_of_range &oError)
    {
    }


    try
    {

        if(!strMessage.compare(0, 16, "recordingStarted"))
        {
            recordingStarted();
            return;
        }
    }
    catch(std::out_of_range &oError)
    {
    }

    try
    {
        if(!strMessage.compare(0, 16, "recordingInfoUpdate"))
        {
            //int64_t i64StartTime_us     = strtoll(arg_string_katcp(pKATCPDispatch, 1), NULL, 10);
            //int64_t i64StopTime_us      = strtoll(arg_string_katcp(pKATCPDispatch, 2), NULL, 10);
            //int64_t i64EllapsedTime_us  = strtoll(arg_string_katcp(pKATCPDispatch, 3), NULL, 10);
            //int64_t i64TimeLeft_us      = strtoll(arg_string_katcp(pKATCPDispatch, 4), NULL, 10);

            return;
        }
    }
    catch(std::out_of_range &oError)
    {
    }
}
