//System includes
#include <iostream>

//Library includes
#include <QtConcurrentRun>

//Local includes
#include "PlotsWidget.h"
#include "ui_PlotsWidget.h"

#include "AVNDataTypes/SpectrometerDataStream/SpetrometerDefinitions.h"

using namespace std;

cPlotsWidget::cPlotsWidget(QWidget *parent) :
    QWidget(parent),
    m_pUI(new Ui::cPlotsWidget),
    m_pPowerPlotWidget(new cFramedQwtLinePlotWidget(this)),
    m_pStokesPlotWidget(new cFramedQwtLinePlotWidget(this)),
    m_pBandPowerPlotWidget(new cBandPowerQwtLinePlot(this)),
    m_bPowerEnabled(true),
    m_bStokesEnabled(true),
    m_bBandPowerEnabled(true)
{
    m_pUI->setupUi(this);

    m_pUI->horizontalLayout_powers->insertWidget(0, m_pPowerPlotWidget);
    m_pUI->horizontalLayout_stokes->insertWidget(0, m_pStokesPlotWidget);
    m_pUI->horizontalLayout_bandPower->insertWidget(0, m_pBandPowerPlotWidget);

    //Use differnt colours for Stokes assumes 2 channels
    m_pStokesPlotWidget->m_qveCurveColours.erase(m_pStokesPlotWidget->m_qveCurveColours.begin(), m_pStokesPlotWidget->m_qveCurveColours.begin() + 2);

    m_pBandPowerPlotWidget->setSpanLengthControlScalingFactor(1, QString("s"));

    //Plot enabling/disabling
    QObject::connect(m_pUI->groupBox_powers, SIGNAL(clicked(bool)), this, SLOT(slotPowerWidgetEnabled(bool)));
    QObject::connect(m_pUI->groupBox_stokes, SIGNAL(clicked(bool)), this, SLOT(slotStokesWidgetEnabled(bool)));
    QObject::connect(m_pUI->groupBox_bandPower, SIGNAL(clicked(bool)), this, SLOT(slotBandPowerWidgetEnabled(bool)));

    //Vertical lines for intergrated bandwidth
    QObject::connect(m_pBandPowerPlotWidget, SIGNAL(sigSelectedBandChanged(QVector<double>)), m_pPowerPlotWidget, SLOT(slotDrawVerticalLines(QVector<double>)));
    QObject::connect(m_pBandPowerPlotWidget, SIGNAL(sigSelectedBandChanged(QVector<double>)), m_pStokesPlotWidget, SLOT(slotDrawVerticalLines(QVector<double>)));
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
    m_pBandPowerPlotWidget->slotPause(bPause);
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
    QReadLocker oReadLock(&m_oMutex);

    return m_bIsRunning;
}

void cPlotsWidget::setIsRunning(bool bIsRunning)
{
    QWriteLocker oWriteLock(&m_oMutex);

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
    int64_t i64LastUsedTimestamp_us = 0;

    int64_t i64Timestamp_us = 0;

    bool bSkipFrame = false;
    bool bResync = false;

    QVector<char> qvcPacket;
    QVector<QVector<float> > qvvfPlotData;
    qvvfPlotData.resize(4);

    QVector<uint32_t> qvu32PowerLRChanList;
    qvu32PowerLRChanList.push_back(0);
    qvu32PowerLRChanList.push_back(1);

    QVector<uint32_t> qvu32StokesQUChanList;
    qvu32StokesQUChanList.push_back(2);
    qvu32StokesQUChanList.push_back(3);

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
            if( __builtin_bswap32( *(uint32_t*)qvcPacket.constData() ) != AVN::Spectrometer::SYNC_WORD)
            {
                cout << "cPlotsWidget::getDataThreadFunction(): Warning got wrong magic no: "
                     << std::hex << __builtin_bswap32( *(uint32_t*)qvcPacket.constData() ) << ". Expected " << AVN::Spectrometer::SYNC_WORD << std::dec << endl;

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
        u32NSamplesPerPacket = (i32NextPacketSize_B - AVN::Spectrometer::HEADER_SIZE_B) / sizeof(int32_t);
        u32NSamplesPerFrame = u32NSamplesPerPacket * u8NPacketsPerFrame;

        //Resize plot vectors as necessary
        if((uint32_t)qvvfPlotData[0].size() != u32NSamplesPerFrame / 4)
        {
            for(uint32_t ui = 0; ui <  qvvfPlotData.size(); ui++)
            {
                qvvfPlotData[ui].resize(u32NSamplesPerFrame / 4);
            }
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
            fpPowerLeft = &qvvfPlotData[0].front();
            fpPowerRight = &qvvfPlotData[1].front();
            fpStokesQ = &qvvfPlotData[2].front();
            fpStokesU = &qvvfPlotData[3].front();

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

                //Get timestamp on the first subframe
                if(!u8ExpectedPacketIndex)
                {
#ifdef _WIN32
                    i64Timestamp_us = _byteswap_uint64( *(int64_t*)(qvcPacket.constData() + 4) );
#else
                    i64Timestamp_us = __builtin_bswap64( *(int64_t*)(qvcPacket.constData() + 4) );
#endif
                    //cout << "Got packet with timestamp " << i64Timestamp_us << endl;

                    //Attempt to reach 30 frames per second.
                    bSkipFrame = (i64Timestamp_us - i64LastUsedTimestamp_us) < 33333;
                }

                if(bSkipFrame)
                    continue;

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

                if(u16PlotType != m_u16PlotType)
                {
                    cout << "Expected plot type " << m_u16PlotType << " , got " << u16PlotType << ". Resynchronising." << endl;
                    bResync = true;
                    break;
                }

                i32pData = (int32_t*)(qvcPacket.constData() + 16); //Go to offset of first sample (header is 16 bytes).

                //Deinterleave data (requires endianess change).
                for(uint32_t u32SampleNo = 0; u32SampleNo < u32NSamplesPerPacket / 4; u32SampleNo++)
                {
#ifdef _WIN32
                    *fpPowerLeft++ =    (int32_t)( _byteswap_long( *i32pData++ ) );
                    *fpPowerRight++ =   (int32_t)( _byteswap_long( *i32pData++ ) );
                    *fpStokesQ++ =      (int32_t)( _byteswap_long( *i32pData++ ) );
                    *fpStokesU++ =      (int32_t)( _byteswap_long( *i32pData++ ) );
#else
                    *fpPowerLeft++ =    (int32_t)( __builtin_bswap32( *i32pData++ ) );
                    *fpPowerRight++ =   (int32_t)( __builtin_bswap32( *i32pData++ ) );
                    *fpStokesQ++ =      (int32_t)( __builtin_bswap32( *i32pData++ ) );
                    *fpStokesU++ =      (int32_t)( __builtin_bswap32( *i32pData++ ) );
#endif
                }

            }

            if(!bResync && !bSkipFrame)
            {
                {
                    QReadLocker oLock(&m_oMutex);

                    if(m_bPowerEnabled)
                    {
                        m_pPowerPlotWidget->addData(qvvfPlotData, i64Timestamp_us, qvu32PowerLRChanList);
                    }

                    if(m_bStokesEnabled)
                    {
                        m_pStokesPlotWidget->addData(qvvfPlotData, i64Timestamp_us, qvu32StokesQUChanList);
                    }

                    if(m_bBandPowerEnabled)
                    {
                        m_pBandPowerPlotWidget->addData(qvvfPlotData, i64Timestamp_us);
                    }
                }

                i64LastUsedTimestamp_us = i64Timestamp_us;
            }
        }
    }
}

void cPlotsWidget::updatePlotType(uint16_t u16PlotType)
{
    //Only use lowest 4 bits
    u16PlotType = 0b0000000000001111 & u16PlotType;

    //If the plot is the same do nothing
    if(m_u16PlotType == u16PlotType)
        return;

    cout << "--- Plot type " << u16PlotType << endl;

    //Otherwise update
    switch(u16PlotType)
    {
    case AVN::Spectrometer::WB_SPECTROMETER_LRQU:
    {
        m_pPowerPlotWidget->setTitle(QString("WB Spectrometer - L and R Circular Polarisation"));
        m_pPowerPlotWidget->setYLabel(QString("Relative Power"));
        m_pPowerPlotWidget->setYUnit(QString("dB"));
        m_pPowerPlotWidget->setXLabel(QString("Frequency"));
        m_pPowerPlotWidget->setXUnit(QString("MHz"));
        QVector<QString> qvqstrCurveNames;
        qvqstrCurveNames.push_back(QString("LCP     "));
        qvqstrCurveNames.push_back(QString("RCP     "));
        m_pPowerPlotWidget->setCurveNames(qvqstrCurveNames);

        m_pStokesPlotWidget->setTitle(QString("WB Spectrometer - Stokes Q and U"));
        m_pStokesPlotWidget->setYLabel(QString("Power"));
        m_pStokesPlotWidget->setYUnit(QString(""));
        m_pStokesPlotWidget->setXLabel(QString("Frequency"));
        m_pStokesPlotWidget->setXUnit(QString("MHz"));
        qvqstrCurveNames.clear();
        qvqstrCurveNames.push_back(QString("Stokes Q"));
        qvqstrCurveNames.push_back(QString("Stokes U"));
        m_pStokesPlotWidget->setCurveNames(qvqstrCurveNames);

        m_pBandPowerPlotWidget->setTitle(QString("Band Power Density - L, R, Q, U"));
        m_pBandPowerPlotWidget->setYLabel(QString("Power Density"));
        m_pBandPowerPlotWidget->setYUnit(QString("dB/MHz"));
        m_pBandPowerPlotWidget->setXLabel(QString("Timestamp"));
        m_pBandPowerPlotWidget->setXUnit(QString(""));
        m_pBandPowerPlotWidget->setSpanLengthControlScalingFactor(1.0, QString("s"));
        qvqstrCurveNames.clear();
        qvqstrCurveNames.push_back(QString("LCP"));
        qvqstrCurveNames.push_back(QString("RCP"));
        qvqstrCurveNames.push_back(QString("Stokes Q"));
        qvqstrCurveNames.push_back(QString("Stokes U"));
        m_pBandPowerPlotWidget->setCurveNames(qvqstrCurveNames);

        m_pStokesPlotWidget->setXSpan(0.0, 400.0);
        m_pPowerPlotWidget->setXSpan(0.0, 400.0);
        m_pBandPowerPlotWidget->setSelectableBand(0.0, 400.0, 1024, QString("MHz")); //Todo, find a good way to determine the number of bins implicity


        m_pPowerPlotWidget->enableLogConversion(true);
        m_pBandPowerPlotWidget->enableLogConversion(true);

        m_pPowerPlotWidget->enableRejectData(false);
        m_pStokesPlotWidget->enableRejectData(false);
        m_pBandPowerPlotWidget->enableRejectData(false);

        cout << "cPlotsWidget::updatePlotType(): Setup plotting for Wideband Spectrometer LRQU" << endl;
        break;
    }

    case AVN::Spectrometer::WB_SPECTROMETER_CFFT:
    {
        m_pPowerPlotWidget->setTitle(QString("WB Spectrometer - L and R Circular Polarisation"));
        m_pPowerPlotWidget->setYLabel(QString("Relative Power"));
        m_pPowerPlotWidget->setYUnit(QString("dB"));
        m_pPowerPlotWidget->setXLabel(QString("Frequency"));
        m_pPowerPlotWidget->setXUnit(QString("MHz"));
        QVector<QString> qvqstrCurveNames;
        qvqstrCurveNames.push_back(QString("LCP"));
        qvqstrCurveNames.push_back(QString("RCP"));
        m_pPowerPlotWidget->setCurveNames(qvqstrCurveNames);

        m_pStokesPlotWidget->setTitle(QString("WB Spectrometer - Relative Phase"));
        m_pStokesPlotWidget->setYLabel(QString("Relative Phase"));
        m_pStokesPlotWidget->setYUnit(QString("rad"));
        m_pStokesPlotWidget->setXLabel(QString("Frequency"));
        m_pStokesPlotWidget->setXUnit(QString("MHz"));
        qvqstrCurveNames.clear();
        qvqstrCurveNames.push_back(QString("Relative Phase"));
        m_pStokesPlotWidget->setCurveNames(qvqstrCurveNames);

        m_pBandPowerPlotWidget->setTitle(QString("Band Power Density - L, R, Q, U"));
        m_pBandPowerPlotWidget->setYLabel(QString("Power Density"));
        m_pBandPowerPlotWidget->setYUnit(QString("dB/MHz"));
        m_pBandPowerPlotWidget->setXLabel(QString("Timestamp"));
        m_pBandPowerPlotWidget->setXUnit(QString(""));
        m_pBandPowerPlotWidget->setSpanLengthControlScalingFactor(1.0, QString("s"));
        qvqstrCurveNames.clear();
        qvqstrCurveNames.push_back(QString("LCP"));
        qvqstrCurveNames.push_back(QString("RCP"));
        qvqstrCurveNames.push_back(QString("Stokes Q"));
        qvqstrCurveNames.push_back(QString("Stokes U"));
        m_pBandPowerPlotWidget->setCurveNames(qvqstrCurveNames);

        m_pStokesPlotWidget->setXSpan(0.0, 400.0);
        m_pPowerPlotWidget->setXSpan(0.0, 400.0);
        m_pBandPowerPlotWidget->setSelectableBand(0.0, 400.0, 1024, QString("MHz")); //Todo, find a good way to determine the number of bins implicity

        m_pPowerPlotWidget->enableLogConversion(true);
        m_pBandPowerPlotWidget->enableLogConversion(true);

        m_pPowerPlotWidget->enableRejectData(false);
        m_pStokesPlotWidget->enableRejectData(false);
        m_pBandPowerPlotWidget->enableRejectData(false);
        cout << "cPlotsWidget::updatePlotType(): Setup plotting for Wideband Spectrometer complex FFT data" << endl;
        break;
    }

    case AVN::Spectrometer::NB_SPECTROMETER_LRQU:
    {
        m_pPowerPlotWidget->setTitle(QString("NB Spectrometer - L and R Circular Polarisation"));
        m_pPowerPlotWidget->setYLabel(QString("Relative Power"));
        m_pPowerPlotWidget->setYUnit(QString("dB"));
        m_pPowerPlotWidget->setXLabel(QString("Frequency"));
        m_pPowerPlotWidget->setXUnit(QString("kHz"));
        QVector<QString> qvqstrCurveNames;
        qvqstrCurveNames.push_back(QString("LCP"));
        qvqstrCurveNames.push_back(QString("RCP"));
        m_pPowerPlotWidget->setCurveNames(qvqstrCurveNames);

        m_pStokesPlotWidget->setTitle(QString("NB Spectrometer - Stokes Q and U"));
        m_pStokesPlotWidget->setYLabel(QString("Relative Power"));
        m_pStokesPlotWidget->setYUnit(QString(""));
        m_pStokesPlotWidget->setXLabel(QString("Frequency"));
        m_pStokesPlotWidget->setXUnit(QString("kHz"));
        qvqstrCurveNames.clear();
        qvqstrCurveNames.push_back(QString("Stokes Q"));
        qvqstrCurveNames.push_back(QString("Stokes U"));
        m_pStokesPlotWidget->setCurveNames(qvqstrCurveNames);

        m_pBandPowerPlotWidget->setTitle(QString("Band Power Density - L, R, Q, U"));
        m_pBandPowerPlotWidget->setYLabel(QString("Power Density"));
        m_pBandPowerPlotWidget->setYUnit(QString("dB/kHz"));
        m_pBandPowerPlotWidget->setXLabel(QString("Timestamp"));
        m_pBandPowerPlotWidget->setXUnit(QString(""));
        m_pBandPowerPlotWidget->setSpanLengthControlScalingFactor(1.0, QString("s"));
        qvqstrCurveNames.clear();
        qvqstrCurveNames.push_back(QString("LCP"));
        qvqstrCurveNames.push_back(QString("RCP"));
        qvqstrCurveNames.push_back(QString("Stokes Q"));
        qvqstrCurveNames.push_back(QString("Stokes U"));
        m_pBandPowerPlotWidget->setCurveNames(qvqstrCurveNames);

        m_pStokesPlotWidget->setXSpan(-781.25, 781.25);
        m_pPowerPlotWidget->setXSpan(-781.25, 781.25);
        m_pBandPowerPlotWidget->setSelectableBand(-781.25, 781.25, 4096, QString("kHz")); //Todo, find a good way to determine the number of bins implicity

        m_pPowerPlotWidget->enableLogConversion(true);
        m_pBandPowerPlotWidget->enableLogConversion(true);

        m_pPowerPlotWidget->enableRejectData(false);
        m_pStokesPlotWidget->enableRejectData(false);
        m_pBandPowerPlotWidget->enableRejectData(false);

        cout << "cPlotsWidget::updatePlotType(): Setup plotting for Narrowband Spectrometer LRQU" << endl;
        break;
    }

    case AVN::Spectrometer::NB_SPECTROMETER_CFFT:
    {
        m_pPowerPlotWidget->setTitle(QString("NB Spectrometer - L and R Circular Polarisation"));
        m_pPowerPlotWidget->setYLabel(QString("Relative Power"));
        m_pPowerPlotWidget->setYUnit(QString("dB"));
        m_pPowerPlotWidget->setXLabel(QString("Frequency"));
        m_pPowerPlotWidget->setXUnit(QString("kHz"));
        QVector<QString> qvqstrCurveNames;
        qvqstrCurveNames.push_back(QString("LCP"));
        qvqstrCurveNames.push_back(QString("RCP"));
        m_pPowerPlotWidget->setCurveNames(qvqstrCurveNames);

        m_pStokesPlotWidget->setTitle(QString("NB Spectrometer - Relative Phase"));
        m_pStokesPlotWidget->setYLabel(QString("Relative Phase"));
        m_pStokesPlotWidget->setYUnit(QString("rad"));
        m_pStokesPlotWidget->setXLabel(QString("Frequency"));
        m_pStokesPlotWidget->setXUnit(QString("kHz"));
        qvqstrCurveNames.clear();
        qvqstrCurveNames.push_back(QString("Relative Phase"));
        m_pStokesPlotWidget->setCurveNames(qvqstrCurveNames);

        m_pBandPowerPlotWidget->setTitle(QString("Band Power Density - L, R, Q, U"));
        m_pBandPowerPlotWidget->setYLabel(QString("Power Density"));
        m_pBandPowerPlotWidget->setYUnit(QString("dB/kHz"));
        m_pBandPowerPlotWidget->setXLabel(QString("Timestamp"));
        m_pBandPowerPlotWidget->setXUnit(QString(""));
        m_pBandPowerPlotWidget->setSpanLengthControlScalingFactor(1.0, QString("s"));
        qvqstrCurveNames.clear();
        qvqstrCurveNames.push_back(QString("LCP"));
        qvqstrCurveNames.push_back(QString("RCP"));
        qvqstrCurveNames.push_back(QString("Stokes Q"));
        qvqstrCurveNames.push_back(QString("Stokes U"));
        m_pBandPowerPlotWidget->setCurveNames(qvqstrCurveNames);

        m_pStokesPlotWidget->setXSpan(-781.25, 781.25);
        m_pPowerPlotWidget->setXSpan(-781.25, 781.25);
        m_pBandPowerPlotWidget->setSelectableBand(-781.25, 781.25, 4096, QString("kHz")); //Todo, find a good way to determine the number of bins implicity

        m_pPowerPlotWidget->enableLogConversion(true);
        m_pBandPowerPlotWidget->enableLogConversion(true);

        m_pPowerPlotWidget->enableRejectData(false);
        m_pStokesPlotWidget->enableRejectData(false);
        m_pBandPowerPlotWidget->enableRejectData(false);
        cout << "cPlotsWidget::updatePlotType(): Setup plotting for Narrowband Spectrometer complex FFT data" << endl;
        break;
    }


    default:
    {
        m_pPowerPlotWidget->setTitle(QString("Received unknown plot type: %1").arg(u16PlotType));
        m_pStokesPlotWidget->setTitle(QString("Received unknown plot type: %1").arg(u16PlotType));
        m_pBandPowerPlotWidget->setTitle(QString("Received unknown plot type: %1").arg(u16PlotType));

        QVector<QString> qvqstrCurveNames;
        m_pPowerPlotWidget->setCurveNames(qvqstrCurveNames);
        m_pStokesPlotWidget->setCurveNames(qvqstrCurveNames);
        m_pBandPowerPlotWidget->setCurveNames(qvqstrCurveNames);

        m_pPowerPlotWidget->enableRejectData(true);
        m_pStokesPlotWidget->enableRejectData(true);
        m_pBandPowerPlotWidget->enableRejectData(true);

        cout << "cPlotsWidget::updatePlotType(): Warning got unknown plot type " << u16PlotType << ". Will not plot data" << endl;
        break;
    }

    }

    //Store the current plot type
    m_u16PlotType = u16PlotType;
}

void cPlotsWidget::slotPowerWidgetEnabled(bool bEnabled)
{
    m_pPowerPlotWidget->setVisible(bEnabled);

    QWriteLocker oLock(&m_oMutex);
    m_bPowerEnabled = bEnabled;
}

void cPlotsWidget::slotStokesWidgetEnabled(bool bEnabled)
{
    m_pStokesPlotWidget->setVisible(bEnabled);

    QWriteLocker oLock(&m_oMutex);
    m_bStokesEnabled = bEnabled;
}

void cPlotsWidget::slotBandPowerWidgetEnabled(bool bEnabled)
{
    m_pBandPowerPlotWidget->setVisible(bEnabled);

    QWriteLocker oLock(&m_oMutex);
    m_bBandPowerEnabled = bEnabled;

    //Remove band lines from the power plot while the band power power is hidden
    m_pPowerPlotWidget->slotShowVerticalLines(bEnabled);
}
