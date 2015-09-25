//System includes
#include <iostream>

//Library includes
#include <QtConcurrentRun>

//Local includes
#include "PlotsWidget.h"
#include "ui_PlotsWidget.h"

using namespace std;

cPlotsWidget::cPlotsWidget(QWidget *parent) :
    QWidget(parent),
    m_pUI(new Ui::cPlotsWidget),
    m_pPowerPlotWidget(new cQwtLinePlotWidget),
    m_pStokesPlotWidget(new cQwtLinePlotWidget)
{
    m_pUI->setupUi(this);

    m_pUI->horizontalLayout_powers->insertWidget(0, m_pPowerPlotWidget);
    m_pUI->horizontalLayout_stokes->insertWidget(0, m_pStokesPlotWidget);
}

cPlotsWidget::~cPlotsWidget()
{
    delete m_pUI;
}

void cPlotsWidget::slotConnect(QString qstrPeer, unsigned short usPeerPort)
{
    //Create and start the TCP receiver
    m_pTCPReceiver.reset(new cTCPReceiver(qstrPeer.toStdString(), usPeerPort));
    m_pTCPReceiver->startReceiving();
    sigConnected();
    sigConnected(true);

    //Start the thread which retrieves data, formats and sends to the plotter widgetts
    setIsRunning(true);
    m_oGetDataFuture = QtConcurrent::run(this, &cPlotsWidget::getDataThreadFunction);
}

void cPlotsWidget::slotDisconnect()
{
    //Stop the thread sending data to the plotter.
    setIsRunning(false);

    //Block until this is done
    cout << "cPlotsWidget::slotDisconnect(): Waiting for getDataThread to stop..." << endl;
    m_oGetDataFuture.waitForFinished();



    //Stop the TCP receiver
    cout << "cPlotsWidget::slotDisconnect(): Closing TCPReceiver..." << endl;

    m_pTCPReceiver.reset();

    cout << "cPlotsWidget::slotDisconnect(): TCPReceiver destroyed." << endl;

    sigDisconnected();
    sigConnected(false);
}

void cPlotsWidget::slotPausePlots(bool bPause)
{
    m_pPowerPlotWidget->slotPause(bPause);
    m_pStokesPlotWidget->slotPause(bPause);
}

void cPlotsWidget::slotPausePlots()
{
    slotPausePlots(true);
}

void cPlotsWidget::slotResumePlots()
{
    slotPausePlots(false);
}

bool cPlotsWidget::isRunning()
{
    QReadLocker oReadLock(&m_oIsRunningMutex);

    return m_bIsRunning;
}

void cPlotsWidget::setIsRunning(bool bIsRunning)
{
    QWriteLocker oWriteLock(&m_oIsRunningMutex);

    m_bIsRunning = bIsRunning;
}

void cPlotsWidget::getDataThreadFunction()
{
    //Thread function fetching data from TCP Receiver and conditioning for plotting.

    //Variables and vectors used in loops:
    uint32_t u32PacketSize_B = 0;
    int32_t i32NextPacketSize_B = 0;
    uint32_t u32NSamplesPerPacket = 0;
    uint32_t u32NSamplesPerFrame = 0;
    uint8_t u8NPacketsPerFrame = 0;
    uint8_t u8CurrentNPacketsPerFrame = 0;
    uint8_t u8CurrentPacketIndex = 0;
    uint16_t u16PlotType = 0xffff;

    int64_t i64Timestamp_us = 0;

    bool bResync = false;

    QVector<char> qvcPacket;
    QVector<QVector<float> > qvvfPowerLR;
    QVector<QVector<float> > qvvfStokesQU;
    qvvfPowerLR.resize(2);
    qvvfStokesQU.resize(2);

    float *fpPowerLeft = NULL;
    float *fpPowerRight = NULL;
    float *fpStokesQ = NULL;
    float *fpStokesU = NULL;
    int32_t *i32pData = NULL;


    while(isRunning())
    {
        do
        {
            if(!isRunning())
                return;

            i32NextPacketSize_B = m_pTCPReceiver->getNextPacketSize_B(500);
        }
        while(i32NextPacketSize_B == -1);

        //Resize Arrays as required
        if(qvcPacket.size() != i32NextPacketSize_B)
        {
            qvcPacket.resize(i32NextPacketSize_B);
            u32PacketSize_B = i32NextPacketSize_B;
        }

        //Synchronise: Find the last packet of the frame
        cout << "Resynchronising to frame border." << endl;
        do
        {
            if(!isRunning())
                return;

            m_pTCPReceiver->getNextPacket(&qvcPacket.front(), 500);
            u8CurrentPacketIndex = *(uint8_t*)(qvcPacket.constData() + 12);
            u8CurrentNPacketsPerFrame = *(uint8_t*)(qvcPacket.constData() + 13);

#ifdef _WIN32
            if( _byteswap_long( *(uint32_t*)qvcPacket.constData() ) != SYNC_WORD)
            {
                cout << "cPlotsWidget::getDataThreadFunction(): Warning got wrong magic no: "
                     << std::hex << _byteswap_long( *(uint32_t*)qvcPacket.constData() ) << ". Expected " << SYNC_WORD << std::dec << endl;

                continue;
            }
#else
            if( __builtin_bswap32( *(uint32_t*)qvcPacket.constData() ) != SYNC_WORD)
            {
                cout << "cPlotsWidget::getDataThreadFunction(): Warning got wrong magic no: "
                     << std::hex << __builtin_bswap32( *(uint32_t*)qvcPacket.constData() ) << ". Expected " << SYNC_WORD << std::dec << endl;

                continue;
            }
#endif

            cout << "cPlotsWidget::getDataThreadFunction(): Synchronising: Got packet " << (uint32_t)u8CurrentPacketIndex << " of " << (uint32_t)u8CurrentNPacketsPerFrame << endl;
        }
        while(u8CurrentPacketIndex != u8CurrentNPacketsPerFrame - 1);

        bResync = false;

        cout << "cPlotsWidget::getDataThreadFunction(): Synchronisation successful. Now aquiring data for plotting..." << endl;

        //Set the number of packets per FFT frame, number of bins per packet and total number of bins per frame
        u8NPacketsPerFrame = u8CurrentNPacketsPerFrame;
        u32NSamplesPerPacket = (i32NextPacketSize_B - HEADER_SIZE_B) / sizeof(int32_t);
        u32NSamplesPerFrame = u32NSamplesPerPacket * u8NPacketsPerFrame;

        //Resize plot vectors as necessary
        if((uint32_t)qvvfPowerLR[0].size() != u32NSamplesPerFrame / 4)
        {
            qvvfPowerLR[0].resize(u32NSamplesPerFrame / 4);
            qvvfPowerLR[1].resize(u32NSamplesPerFrame / 4);
            qvvfStokesQU[0].resize(u32NSamplesPerFrame / 4);
            qvvfStokesQU[1].resize(u32NSamplesPerFrame / 4);
        }

        //Update scales etc. as necessary based on the plot type.
#ifdef _WIN32
        u16PlotType = 0b0000000000001111 & _byteswap_ushort ( *(int16_t*)(qvcPacket.constData() + 14) );
#else
        u16PlotType = 0b0000000000001111 & __builtin_bswap16( *(int16_t*)(qvcPacket.constData() + 14) );
#endif
        updatePlotType(u16PlotType);

        //Read the data
        while(!bResync)
        {
            //Some pointers for copying values
            fpPowerLeft = &qvvfPowerLR[0].front();
            fpPowerRight = &qvvfPowerLR[1].front();
            fpStokesQ = &qvvfStokesQU[0].front();
            fpStokesU = &qvvfStokesQU[1].front();

            for(uint8_t u8ExpectedPacketIndex = 0; u8ExpectedPacketIndex < u8NPacketsPerFrame; u8ExpectedPacketIndex++)
            {
                //Check for data consistency
                do
                {
                    if(!isRunning())
                        return;

                    i32NextPacketSize_B = m_pTCPReceiver->getNextPacketSize_B(500);
                }
                while(i32NextPacketSize_B == -1);

                if((unsigned)i32NextPacketSize_B != u32PacketSize_B)
                {
                    cout << "cPlotsWidget::getDataThreadFunction(): Got different packet size, resynchronising." << endl;
                    bResync = true;
                    break;
                }

                //Read the next packet
                while(!m_pTCPReceiver->getNextPacket(&qvcPacket.front(), 500))
                {
                    if(!isRunning())
                        return;
                }

                //Some more consistency checks
                u8CurrentPacketIndex = *(uint8_t*)(qvcPacket.constData() + 12);
                u8CurrentNPacketsPerFrame = *(uint8_t*)(qvcPacket.constData() + 13);
#ifdef _WIN32
                u16PlotType = 0b0000000000001111 & _byteswap_ushort ( *(int16_t*)(qvcPacket.constData() + 14) );
#else
                u16PlotType = 0b0000000000001111 & __builtin_bswap16( *(int16_t*)(qvcPacket.constData() + 14) );
#endif
                if(u8CurrentPacketIndex != u8ExpectedPacketIndex)
                {
                    cout << "Expected packet index " << (uint32_t)u8ExpectedPacketIndex << ", got " << (uint32_t)u8CurrentPacketIndex << ". Resynchronising." << endl;
                    bResync = true;
                    break;
                }

                if(u8NPacketsPerFrame != u8CurrentNPacketsPerFrame)
                {
                    cout << "Expected " << (uint32_t)u8NPacketsPerFrame << " packets per frame, got " << (uint32_t)u8CurrentNPacketsPerFrame << ". Resynchronising." << endl;
                    bResync = true;
                    break;
                }

                if((plotType)u16PlotType != m_ePlotType)
                {
                    cout << "Expected plot type " << m_ePlotType << " , got " << u16PlotType << ". Resynchronising." << endl;
                    bResync = true;
                    break;
                }

                i32pData = (int32_t*)(qvcPacket.constData() + 16); //Go to offset of first sample (header is 16 bytes).

                //Deinterleave data (requires endianess change).
                for(uint32_t u32SampleNo = 0; u32SampleNo < u32NSamplesPerPacket / 4; u32SampleNo++)
                {
#ifdef _WIN32
                    *fpPowerLeft++ =    _byteswap_long( *i32pData++ );
                    *fpPowerRight++ =   _byteswap_long( *i32pData++ );
                    *fpStokesQ++ =      _byteswap_long( *i32pData++ );
                    *fpStokesU++ =      _byteswap_long( *i32pData++ );
#else
                    *fpPowerLeft++ =    __builtin_bswap32( *i32pData++ );
                    *fpPowerRight++ =   __builtin_bswap32( *i32pData++ );
                    *fpStokesQ++ =      __builtin_bswap32( *i32pData++ );
                    *fpStokesU++ =      __builtin_bswap32( *i32pData++ );
#endif
                }

                //Get timestamp on the first subframe
                if(!u8ExpectedPacketIndex)
                {
#ifdef _WIN32
                    i64Timestamp_us = _byteswap_uint64( *(int64_t*)(qvcPacket.constData() + 4) );
#else
                    i64Timestamp_us = __builtin_bswap64( *(int64_t*)(qvcPacket.constData() + 4) );
#endif
                    //cout << "Got packet with timestamp " << i64Timestamp_us << endl;
                }
            }

            if(!bResync)
            {
                m_pPowerPlotWidget->addData(qvvfPowerLR, i64Timestamp_us);
                m_pStokesPlotWidget->addData(qvvfStokesQU, i64Timestamp_us);
            }
        }
    }
}

void cPlotsWidget::updatePlotType(uint16_t u16PlotType)
{
    //Only use lowest 4 bits
    u16PlotType = 0b0000000000001111 & u16PlotType;

    //If the plot is the same do nothing
    if(m_ePlotType == (plotType)u16PlotType)
        return;

    cout << "--- Plot type " << u16PlotType << endl;

    //Otherwise update
    switch(u16PlotType)
    {
    case WB_SPECTROMETER_LRQU:
        m_pPowerPlotWidget->setTitle(QString("WB Spectrometer - Left and Right Power"));
        m_pPowerPlotWidget->setYLabel(QString("Relative Power"));
        m_pPowerPlotWidget->setYUnit(QString("dB"));
        m_pPowerPlotWidget->setXLabel(QString("Frequency"));
        m_pPowerPlotWidget->setXUnit(QString("MHz"));

        m_pStokesPlotWidget->setTitle(QString("WB Spectrometer - Stokes Q and U"));
        m_pStokesPlotWidget->setYLabel(QString("Power"));
        m_pStokesPlotWidget->setYUnit(QString(""));
        m_pStokesPlotWidget->setXLabel(QString("Frequency"));
        m_pStokesPlotWidget->setXUnit(QString("MHz"));

        m_pStokesPlotWidget->setXSpan(0.0, 400.0);
        m_pPowerPlotWidget->setXSpan(0.0, 400.0);


        m_pPowerPlotWidget->enableLogConversion(true);

        m_pPowerPlotWidget->enableRejectData(false);
        m_pStokesPlotWidget->enableRejectData(false);

        cout << "cPlotsWidget::updatePlotType(): Setup plotting for Wideband Spectrometer LRQU" << endl;
        break;

    case WB_SPECTROMETER_CFFT:
        m_pPowerPlotWidget->setTitle(QString("WB Spectrometer - Left and Right Power"));
        m_pPowerPlotWidget->setYLabel(QString("Relative Power"));
        m_pPowerPlotWidget->setYUnit(QString("dB"));
        m_pPowerPlotWidget->setXLabel(QString("Frequency"));
        m_pPowerPlotWidget->setXUnit(QString("MHz"));

        m_pStokesPlotWidget->setTitle(QString("WB Spectrometer - Relative Phase"));
        m_pStokesPlotWidget->setYLabel(QString("Relative Phase"));
        m_pStokesPlotWidget->setYUnit(QString("rad"));
        m_pStokesPlotWidget->setXLabel(QString("Frequency"));
        m_pStokesPlotWidget->setXUnit(QString("MHz"));

        m_pStokesPlotWidget->setXSpan(0.0, 400.0);
        m_pPowerPlotWidget->setXSpan(0.0, 400.0);

        m_pPowerPlotWidget->enableLogConversion(true);

        m_pPowerPlotWidget->enableRejectData(false);
        m_pStokesPlotWidget->enableRejectData(false);

        cout << "cPlotsWidget::updatePlotType(): Setup plotting for Wideband Spectrometer complex FFT data" << endl;
        break;

    case NB_SPECTROMETER_LRQU:
        m_pPowerPlotWidget->setTitle(QString("NB Spectrometer - Left and Right Power"));
        m_pPowerPlotWidget->setYLabel(QString("Relative Power"));
        m_pPowerPlotWidget->setYUnit(QString("dB"));
        m_pPowerPlotWidget->setXLabel(QString("Frequency"));
        m_pPowerPlotWidget->setXUnit(QString("Hz"));

        m_pStokesPlotWidget->setTitle(QString("NB Spectrometer - Stokes Q and U"));
        m_pStokesPlotWidget->setYLabel(QString("Power"));
        m_pStokesPlotWidget->setYUnit(QString(""));
        m_pStokesPlotWidget->setXLabel(QString("Frequency"));
        m_pStokesPlotWidget->setXUnit(QString("Hz"));

        m_pStokesPlotWidget->setXSpan(-781.25, 781.25);
        m_pPowerPlotWidget->setXSpan(-781.25, 781.25);

        m_pPowerPlotWidget->enableLogConversion(true);

        m_pPowerPlotWidget->enableRejectData(false);
        m_pStokesPlotWidget->enableRejectData(false);

        cout << "cPlotsWidget::updatePlotType(): Setup plotting for Narrowband Spectrometer LRQU" << endl;
        break;

    case NB_SPECTROMETER_CFFT:
        m_pPowerPlotWidget->setTitle(QString("NB Spectrometer - Left and Right Power"));
        m_pPowerPlotWidget->setYLabel(QString("Relative Power"));
        m_pPowerPlotWidget->setYUnit(QString("dB"));
        m_pPowerPlotWidget->setXLabel(QString("Frequency"));
        m_pPowerPlotWidget->setXUnit(QString("Hz"));

        m_pStokesPlotWidget->setTitle(QString("NB Spectrometer - Relative Phase"));
        m_pStokesPlotWidget->setYLabel(QString("Relative Phase"));
        m_pStokesPlotWidget->setYUnit(QString("rad"));
        m_pStokesPlotWidget->setXLabel(QString("Frequency"));
        m_pStokesPlotWidget->setXUnit(QString("Hz"));

        m_pStokesPlotWidget->setXSpan(-781.25,781.25);
        m_pPowerPlotWidget->setXSpan(-781.25, 781.25);

        m_pPowerPlotWidget->enableLogConversion(true);

        m_pPowerPlotWidget->enableRejectData(false);
        m_pStokesPlotWidget->enableRejectData(false);

        cout << "cPlotsWidget::updatePlotType(): Setup plotting for Narrowband Spectrometer complex FFT data" << endl;
        break;

    case TIME_DATA:
        //TODO: Reject until implemented
        m_pPowerPlotWidget->enableRejectData(true);
        m_pStokesPlotWidget->enableRejectData(true);

        cout << "cPlotsWidget::updatePlotType(): Setup plotting for Time data" << endl;
        break;

    default:
        m_pPowerPlotWidget->setTitle(QString("Received unknown plot type: %1").arg(u16PlotType));
        m_pStokesPlotWidget->setTitle(QString("Received unknown plot type: %1").arg(u16PlotType));

        m_pPowerPlotWidget->enableRejectData(true);
        m_pStokesPlotWidget->enableRejectData(true);

        cout << "cPlotsWidget::updatePlotType(): Warning got unknown plot type " << u16PlotType << ". Will not plot data" << endl;

        break;
    }

    //Store the current plot type
    m_ePlotType = (plotType)u16PlotType;
}
