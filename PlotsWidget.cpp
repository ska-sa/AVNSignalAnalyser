//System includes
#include <iostream>
#include <cmath>

//Library includes
#include <QtConcurrentRun>

#ifndef Q_MOC_RUN //Qt's MOC and Boost have some issues don't let MOC process boost headers
#include <boost/make_shared.hpp>
#endif

//Local includes
#include "PlotsWidget.h"
#include "ui_PlotsWidget.h"

#include "AVNDataTypes/SpectrometerDataStream/SpectrometerDefinitions.h"

using namespace std;

cPlotsWidget::cPlotsWidget(QWidget *pParent) :
    QWidget(pParent),
    m_pUI(new Ui::cPlotsWidget),
    m_pPowerPlotWidget(new cFramedQwtLinePlotWidget(this)),
    m_pStokesPhasePlotWidget(new cFramedQwtLinePlotWidget(this)),
    m_pBandPowerPlotWidget(new cBandPowerQwtLinePlot(this)),
    m_bPowerEnabled(true),
    m_bStokesPhaseEnabled(true),
    m_bBandPowerEnabled(true),
    m_u16PlotType(AVN::Spectrometer::UNDEFINED),
    m_u32AccumulationLength_nFrames(1)
{
    m_pUI->setupUi(this);

    updatePlotType(AVN::Spectrometer::UNDEFINED); //Launch in a safe state (Unknown plot type rejects sample data). This will change when a known stream type is identified.

    m_pUI->horizontalLayout_powers->insertWidget(0, m_pPowerPlotWidget);
    m_pUI->horizontalLayout_stokesPhase->insertWidget(0, m_pStokesPhasePlotWidget);
    m_pUI->horizontalLayout_bandPower->insertWidget(0, m_pBandPowerPlotWidget);

    //Use differnt colours for StokesPhase assumes 2 channels
    m_pStokesPhasePlotWidget->m_qveCurveColours.erase(m_pStokesPhasePlotWidget->m_qveCurveColours.begin(), m_pStokesPhasePlotWidget->m_qveCurveColours.begin() + 2);

    m_pBandPowerPlotWidget->setSpanLengthControlScalingFactor(1, QString("s")); //Band power span is in seconds:

    //Plot enabling/disabling
    //When the groupboxes for any of the 3 plots types are enabled or disabled catch and fire a signal off from this class
    QObject::connect(m_pUI->groupBox_powers, SIGNAL(clicked(bool)), this, SLOT(slotPowerPlotEnabled(bool)));
    QObject::connect(m_pUI->groupBox_stokesPhase, SIGNAL(clicked(bool)), this, SLOT(slotStokesPhasePlotEnabled(bool)));
    QObject::connect(m_pUI->groupBox_bandPower, SIGNAL(clicked(bool)), this, SLOT(slotBandPowerPlotEnabled(bool)));

    //Vertical lines for intergrated bandwidth
    QObject::connect(m_pBandPowerPlotWidget, SIGNAL(sigSelectedBandChanged(QVector<double>)), m_pPowerPlotWidget, SLOT(slotDrawVerticalLines(QVector<double>)));
    QObject::connect(m_pBandPowerPlotWidget, SIGNAL(sigSelectedBandChanged(QVector<double>)), m_pStokesPhasePlotWidget, SLOT(slotDrawVerticalLines(QVector<double>)));

    //SocketStreamer triggered disconnect
    QObject::connect(this, SIGNAL(sigDisconnect()), this, SLOT(slotDisconnect()), Qt::QueuedConnection); //Must be queued to prevent the socket reading thread try to join itself.

    //Connect Zoom of X axis together
    QObject::connect(m_pPowerPlotWidget, SIGNAL(sigXScaleDivChanged(double,double)), m_pStokesPhasePlotWidget, SLOT(slotUpdateXScaleDiv(double,double)) );
    QObject::connect(m_pStokesPhasePlotWidget, SIGNAL(sigXScaleDivChanged(double,double)), m_pPowerPlotWidget, SLOT(slotUpdateXScaleDiv(double,double)) );

    //Connect mouse position indicator of power and stoke/phase plots together.
    QObject::connect(m_pPowerPlotWidget, SIGNAL(sigSharedMousePositionChanged(QPointF,bool)), m_pStokesPhasePlotWidget, SLOT(slotUpdateSharedMouseHPosition(QPointF,bool)) );
    QObject::connect(m_pStokesPhasePlotWidget, SIGNAL(sigSharedMousePositionChanged(QPointF,bool)), m_pPowerPlotWidget, SLOT(slotUpdateSharedMouseHPosition(QPointF,bool)) );
}

cPlotsWidget::~cPlotsWidget()
{
    setIsRunning(false);

    //Wait for data fetching thread to exit
    m_oGetDataFuture.waitForFinished();

    delete m_pUI;
}

void cPlotsWidget::slotConnect(int iDataProtocol, const QString &qstrLocalInterface, unsigned short usLocalPort, const QString &qstrPeerAddress, unsigned short usPeerPort)
{
    //Create and start the socket receiver depending on protocol
    switch(cNetworkConnectionWidget::dataProtocol(iDataProtocol))
    {
    case cNetworkConnectionWidget::TCP:
        m_pSocketReceiver = boost::make_shared<cTCPReceiver>(qstrPeerAddress.toStdString(), usPeerPort);

        //Register this class instance to get notification callbacks about socket connectivity.
        boost::static_pointer_cast<cTCPReceiver>(m_pSocketReceiver)->registerNoticationCallbackHandler(this);

        break;

    case cNetworkConnectionWidget::UDP:
        m_pSocketReceiver = boost::make_shared<cUDPReceiver>(qstrLocalInterface.toStdString(), usLocalPort, qstrPeerAddress.toStdString(), usPeerPort);
        break;

    default:
        //Return if protocol unknown. Should never happen
        return;
    }

    m_pStreamInterpreter = boost::make_shared<cSpectrometerDataStreamInterpreter>(m_pSocketReceiver);
    m_pStreamInterpreter->setUpdateRate(33);

    m_pSocketReceiver->startReceiving();

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

    //Stop the TCP receiver and interpreter

    cout << "cPlotsWidget::slotDisconnect(): Closing SpectrometerDataStreamInterpreter..." << endl;

    m_pStreamInterpreter.reset();

    cout << "cPlotsWidget::slotDisconnect(): SpectrometerDataStreamInterpreter destroyed..." << endl;

    cout << "cPlotsWidget::slotDisconnect(): Closing SocketReceiver..." << endl;

    m_pSocketReceiver.reset();

    cout << "cPlotsWidget::slotDisconnect(): SocketReceiver destroyed." << endl;

    sigDisconnected();
    sigConnected(false);
}

void cPlotsWidget::slotPausePlots(bool bPause)
{
    m_pPowerPlotWidget->slotPause(bPause);
    m_pStokesPhasePlotWidget->slotPause(bPause);
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
    {
        QWriteLocker oWriteLock(&m_oMutex);
        m_bIsRunning = bIsRunning;
    }

    if(m_pStreamInterpreter.get())
    {
        m_pStreamInterpreter->setIsRunning(bIsRunning);
    }
}

void cPlotsWidget::getDataThreadFunction()
{
    cout << "Entered cPlotsWidget::getDataThreadFunction()" << endl;

    //Thread function fetching data from TCP Receiver and conditioning for plotting.

    //Variables and vectors used for plot data:
    QVector<QVector<float> > qvvfPlotData;
    qvvfPlotData.resize(4);

    QVector<uint32_t> qvu32PowerLRChanList;
    qvu32PowerLRChanList.push_back(0);
    qvu32PowerLRChanList.push_back(1);

    QVector<uint32_t> qvu32StokesPhaseQUChanList;
    qvu32StokesPhaseQUChanList.push_back(2);
    qvu32StokesPhaseQUChanList.push_back(3);

    QVector<uint32_t> qvu32RelativePhaseChanList;
    qvu32RelativePhaseChanList.push_back(2);

    float *fpChan0 = NULL;
    float *fpChan1 = NULL;
    float *fpChan2 = NULL;
    float *fpChan3 = NULL;

    while(isRunning())
    {
        while(!m_pStreamInterpreter->synchronise())
        {
            if(!isRunning())
                return;
        }

        //After synchronisation stream parameter are available so...

        //Resize plot vectors as necessary
        if((uint32_t)qvvfPlotData[0].size() != m_pStreamInterpreter->getNValuesPerChannelPerFrame())
        {
            for(uint32_t ui = 0; ui <  (uint32_t)qvvfPlotData.size(); ui++)
            {
                qvvfPlotData[ui].resize(m_pStreamInterpreter->getNValuesPerChannelPerFrame());
            }
        }

        //Some pointers for copying data values
        fpChan0 = &qvvfPlotData[0].front();
        fpChan1 = &qvvfPlotData[1].front();
        fpChan2 = &qvvfPlotData[2].front();
        fpChan3 = &qvvfPlotData[3].front();

        //Update plotting widget to plot data correctly
        updatePlotType(m_pStreamInterpreter->getLastHeader().getDigitiserType());

        while(isRunning())
        {
            //Some pointers for copying data values
            fpChan0 = &qvvfPlotData[0].front();
            fpChan1 = &qvvfPlotData[1].front();
            fpChan2 = &qvvfPlotData[2].front();
            fpChan3 = &qvvfPlotData[3].front();


            //Get the data from the next frame. Return false means an inconsistency was incountered
            if(!m_pStreamInterpreter->getNextFrame(fpChan0, fpChan1, fpChan2, fpChan3, qvvfPlotData[0].size()))
            {
                break; //Causes resync
            }

            //Calculate  phase for LRPP mode
            switch(m_pStreamInterpreter->getLastHeader().getDigitiserType())
            {
            case AVN::Spectrometer::WB_SPECTROMETER_LRPP:
            case AVN::Spectrometer::NB_SPECTROMETER_LRPP:
            {
                QReadLocker oLock(&m_oMutex);

                for(uint32_t ui = 0; ui < (uint32_t)qvvfPlotData[0].size(); ui++)
                {
                    //Phase calculations: Store relative phase between ADC0 and ADC1 in Chan2 in progressive frequency bins
                    //Phase for ADC0 and ADC1 in channels 2 and 3 respectively

                    //Phase is calculated in FPGA as a Fixed 18_15 value in radians (18 bits. 15 lower 15 of which are fractional).
                    //The decimal point is ignored and the values are accumulated to a 32 bit signed integer
                    //for interpretation by a x86 platform.
                    //So we need to rescale back to radians:

                    *fpChan2 = ( *fpChan2 / 32768) - ( *fpChan3  / 32768);
                    *fpChan2 /= m_u32AccumulationLength_nFrames; //Divide out accumulation length

                    fpChan2++;
                    fpChan3++;
                }

                if(m_bPowerEnabled)
                {
                    m_pPowerPlotWidget->addData(qvvfPlotData, m_pStreamInterpreter->getLastHeader().getTimestamp_us(), qvu32PowerLRChanList);
                }

                if(m_bStokesPhaseEnabled)
                {
                    m_pStokesPhasePlotWidget->addData(qvvfPlotData, m_pStreamInterpreter->getLastHeader().getTimestamp_us(), qvu32RelativePhaseChanList);
                }

                if(m_bBandPowerEnabled)
                {
                    m_pBandPowerPlotWidget->addData(qvvfPlotData, m_pStreamInterpreter->getLastHeader().getTimestamp_us(), qvu32PowerLRChanList);
                }

                break;
            }

            case AVN::Spectrometer::WB_SPECTROMETER_LRQU:
            case AVN::Spectrometer::NB_SPECTROMETER_LRQU:
            {
                QReadLocker oLock(&m_oMutex);

                if(m_bPowerEnabled)
                {
                    m_pPowerPlotWidget->addData(qvvfPlotData, m_pStreamInterpreter->getLastHeader().getTimestamp_us(), qvu32PowerLRChanList);
                }

                if(m_bStokesPhaseEnabled)
                {
                    m_pStokesPhasePlotWidget->addData(qvvfPlotData, m_pStreamInterpreter->getLastHeader().getTimestamp_us(), qvu32StokesPhaseQUChanList);
                }

                if(m_bBandPowerEnabled)
                {
                    m_pBandPowerPlotWidget->addData(qvvfPlotData, m_pStreamInterpreter->getLastHeader().getTimestamp_us());
                }

                break;
            }

            default:
            {
            }

            }
        }
    }
}

void cPlotsWidget::updatePlotType(uint16_t u16PlotType)
{
    //Only use lowest 4 bits
    u16PlotType = 0x000F & u16PlotType;

    //If the plot is the same do nothing
    if(m_u16PlotType == u16PlotType)
        return;

    //Otherwise update
    switch(u16PlotType)
    {
    case AVN::Spectrometer::WB_SPECTROMETER_LRQU:
    {
        m_pUI->groupBox_stokesPhase->setTitle(QString("Stokes Q / U"));

        m_pPowerPlotWidget->setTitle(QString("WB Spectrometer - Relative Power - LCP / RCP"));
        m_pPowerPlotWidget->setYLabel(QString("Relative Power"));
        m_pPowerPlotWidget->setYUnit(QString("dB"));
        m_pPowerPlotWidget->setXLabel(QString("Frequency"));
        m_pPowerPlotWidget->setXUnit(QString("MHz"));
        QVector<QString> qvqstrCurveNames;
        qvqstrCurveNames.push_back(QString("LCP     "));
        qvqstrCurveNames.push_back(QString("RCP     "));
        m_pPowerPlotWidget->setCurveNames(qvqstrCurveNames);

        m_pStokesPhasePlotWidget->setTitle(QString("WB Spectrometer - Stokes Q / U"));
        m_pStokesPhasePlotWidget->setYLabel(QString("Power"));
        m_pStokesPhasePlotWidget->setYUnit(QString(""));
        m_pStokesPhasePlotWidget->setXLabel(QString("Frequency"));
        m_pStokesPhasePlotWidget->setXUnit(QString("MHz"));
        qvqstrCurveNames.clear();
        qvqstrCurveNames.push_back(QString("Stokes Q"));
        qvqstrCurveNames.push_back(QString("Stokes U"));
        m_pStokesPhasePlotWidget->setCurveNames(qvqstrCurveNames);

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

        m_pStokesPhasePlotWidget->setXSpan(0.0, 400.0);
        m_pPowerPlotWidget->setXSpan(0.0, 400.0);
        m_pBandPowerPlotWidget->setSelectableBand(0.0, 400.0, QString("MHz"));

        m_pPowerPlotWidget->enableLogConversion(true);
        m_pBandPowerPlotWidget->enableLogConversion(true);

        m_pPowerPlotWidget->enableRejectData(false);
        m_pStokesPhasePlotWidget->enableRejectData(false);
        m_pBandPowerPlotWidget->enableRejectData(false);

        cout << "cPlotsWidget::updatePlotType(): Setup plotting for Wideband Spectrometer LRQU" << endl;
        break;
    }

    case AVN::Spectrometer::WB_SPECTROMETER_LRPP:
    {
        m_pUI->groupBox_stokesPhase->setTitle(QString("Relative Phase"));

        m_pPowerPlotWidget->setTitle(QString("WB Spectrometer - Relative Power - LCP / RCP"));
        m_pPowerPlotWidget->setYLabel(QString("Relative Power"));
        m_pPowerPlotWidget->setYUnit(QString("dB"));
        m_pPowerPlotWidget->setXLabel(QString("Frequency"));
        m_pPowerPlotWidget->setXUnit(QString("MHz"));
        QVector<QString> qvqstrCurveNames;
        qvqstrCurveNames.push_back(QString("LCP"));
        qvqstrCurveNames.push_back(QString("RCP"));
        m_pPowerPlotWidget->setCurveNames(qvqstrCurveNames);

        m_pStokesPhasePlotWidget->setTitle(QString("WB Spectrometer - Relative Phase - LCP / RCP"));
        m_pStokesPhasePlotWidget->setYLabel(QString("Relative Phase"));
        m_pStokesPhasePlotWidget->setYUnit(QString("rad"));
        m_pStokesPhasePlotWidget->setXLabel(QString("Frequency"));
        m_pStokesPhasePlotWidget->setXUnit(QString("MHz"));
        qvqstrCurveNames.clear();
        qvqstrCurveNames.push_back(QString("Relative Phase"));
        m_pStokesPhasePlotWidget->setCurveNames(qvqstrCurveNames);

        m_pBandPowerPlotWidget->setTitle(QString("Band Power Density - LCP, RCP"));
        m_pBandPowerPlotWidget->setYLabel(QString("Power Density"));
        m_pBandPowerPlotWidget->setYUnit(QString("dB/MHz"));
        m_pBandPowerPlotWidget->setXLabel(QString("Timestamp"));
        m_pBandPowerPlotWidget->setXUnit(QString(""));
        m_pBandPowerPlotWidget->setSpanLengthControlScalingFactor(1.0, QString("s"));
        qvqstrCurveNames.clear();
        qvqstrCurveNames.push_back(QString("LCP"));
        qvqstrCurveNames.push_back(QString("RCP"));
        m_pBandPowerPlotWidget->setCurveNames(qvqstrCurveNames);

        m_pStokesPhasePlotWidget->setXSpan(0.0, 400.0);
        m_pPowerPlotWidget->setXSpan(0.0, 400.0);
        m_pBandPowerPlotWidget->setSelectableBand(0.0, 400.0, QString("MHz"));

        m_pPowerPlotWidget->enableLogConversion(true);
        m_pBandPowerPlotWidget->enableLogConversion(true);

        m_pPowerPlotWidget->enableRejectData(false);
        m_pStokesPhasePlotWidget->enableRejectData(false);
        m_pBandPowerPlotWidget->enableRejectData(false);

        cout << "cPlotsWidget::updatePlotType(): Setup plotting for Wideband Spectrometer complex FFT data" << endl;

        break;
    }

    case AVN::Spectrometer::NB_SPECTROMETER_LRQU:
    {
        m_pUI->groupBox_stokesPhase->setTitle(QString("Stokes Q / U"));

        m_pPowerPlotWidget->setTitle(QString("NB Spectrometer - Relative Power - LCP / RCP"));
        m_pPowerPlotWidget->setYLabel(QString("Relative Power"));
        m_pPowerPlotWidget->setYUnit(QString("dB"));
        m_pPowerPlotWidget->setXLabel(QString("Frequency"));
        m_pPowerPlotWidget->setXUnit(QString("kHz"));
        QVector<QString> qvqstrCurveNames;
        qvqstrCurveNames.push_back(QString("LCP"));
        qvqstrCurveNames.push_back(QString("RCP"));
        m_pPowerPlotWidget->setCurveNames(qvqstrCurveNames);

        m_pStokesPhasePlotWidget->setTitle(QString("NB Spectrometer - Stokes Q / U"));
        m_pStokesPhasePlotWidget->setYLabel(QString("Relative Power"));
        m_pStokesPhasePlotWidget->setYUnit(QString(""));
        m_pStokesPhasePlotWidget->setXLabel(QString("Frequency"));
        m_pStokesPhasePlotWidget->setXUnit(QString("kHz"));
        qvqstrCurveNames.clear();
        qvqstrCurveNames.push_back(QString("Stokes Q"));
        qvqstrCurveNames.push_back(QString("Stokes U"));
        m_pStokesPhasePlotWidget->setCurveNames(qvqstrCurveNames);

        m_pBandPowerPlotWidget->setTitle(QString("Band Power Density - LCP, RCP, Q, U"));
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

        m_pStokesPhasePlotWidget->setXSpan(-781.25, 781.25);
        m_pPowerPlotWidget->setXSpan(-781.25, 781.25);
        m_pBandPowerPlotWidget->setSelectableBand(-781.25, 781.25, QString("kHz"));

        m_pPowerPlotWidget->enableLogConversion(true);
        m_pBandPowerPlotWidget->enableLogConversion(true);

        m_pPowerPlotWidget->enableRejectData(false);
        m_pStokesPhasePlotWidget->enableRejectData(false);
        m_pBandPowerPlotWidget->enableRejectData(false);

        cout << "cPlotsWidget::updatePlotType(): Setup plotting for Narrowband Spectrometer LRQU" << endl;
        break;
    }

    case AVN::Spectrometer::NB_SPECTROMETER_LRPP:
    {
        m_pUI->groupBox_stokesPhase->setTitle(QString("Relative Phase"));

        m_pPowerPlotWidget->setTitle(QString("NB Spectrometer - Relative Power - LCP / RCP"));
        m_pPowerPlotWidget->setYLabel(QString("Relative Power"));
        m_pPowerPlotWidget->setYUnit(QString("dB"));
        m_pPowerPlotWidget->setXLabel(QString("Frequency"));
        m_pPowerPlotWidget->setXUnit(QString("kHz"));
        QVector<QString> qvqstrCurveNames;
        qvqstrCurveNames.push_back(QString("LCP"));
        qvqstrCurveNames.push_back(QString("RCP"));
        m_pPowerPlotWidget->setCurveNames(qvqstrCurveNames);

        m_pStokesPhasePlotWidget->setTitle(QString("NB Spectrometer - Relative Phase"));
        m_pStokesPhasePlotWidget->setYLabel(QString("Relative Phase"));
        m_pStokesPhasePlotWidget->setYUnit(QString("rad"));
        m_pStokesPhasePlotWidget->setXLabel(QString("Frequency"));
        m_pStokesPhasePlotWidget->setXUnit(QString("kHz"));
        qvqstrCurveNames.clear();
        qvqstrCurveNames.push_back(QString("Relative Phase"));
        m_pStokesPhasePlotWidget->setCurveNames(qvqstrCurveNames);

        m_pBandPowerPlotWidget->setTitle(QString("Band Power Density - LCP, RCP"));
        m_pBandPowerPlotWidget->setYLabel(QString("Power Density"));
        m_pBandPowerPlotWidget->setYUnit(QString("dB/kHz"));
        m_pBandPowerPlotWidget->setXLabel(QString("Timestamp"));
        m_pBandPowerPlotWidget->setXUnit(QString(""));
        m_pBandPowerPlotWidget->setSpanLengthControlScalingFactor(1.0, QString("s"));
        qvqstrCurveNames.clear();
        qvqstrCurveNames.push_back(QString("LCP"));
        qvqstrCurveNames.push_back(QString("RCP"));
        m_pBandPowerPlotWidget->setCurveNames(qvqstrCurveNames);

        m_pStokesPhasePlotWidget->setXSpan(-781.25, 781.25);
        m_pPowerPlotWidget->setXSpan(-781.25, 781.25);
        m_pBandPowerPlotWidget->setSelectableBand(-781.25, 781.25, QString("kHz"));

        m_pPowerPlotWidget->enableLogConversion(true);
        m_pBandPowerPlotWidget->enableLogConversion(true);

        m_pPowerPlotWidget->enableRejectData(false);
        m_pStokesPhasePlotWidget->enableRejectData(false);
        m_pBandPowerPlotWidget->enableRejectData(false);
        cout << "cPlotsWidget::updatePlotType(): Setup plotting for Narrowband Spectrometer complex FFT data" << endl;
        break;
    }


    default:
    {
        m_pPowerPlotWidget->setTitle(QString("Received unknown plot type: %1").arg(u16PlotType));
        m_pStokesPhasePlotWidget->setTitle(QString("Received unknown plot type: %1").arg(u16PlotType));
        m_pBandPowerPlotWidget->setTitle(QString("Received unknown plot type: %1").arg(u16PlotType));

        QVector<QString> qvqstrCurveNames;
        m_pPowerPlotWidget->setCurveNames(qvqstrCurveNames);
        m_pStokesPhasePlotWidget->setCurveNames(qvqstrCurveNames);
        m_pBandPowerPlotWidget->setCurveNames(qvqstrCurveNames);

        m_pPowerPlotWidget->enableRejectData(true);
        m_pStokesPhasePlotWidget->enableRejectData(true);
        m_pBandPowerPlotWidget->enableRejectData(true);

        cout << "cPlotsWidget::updatePlotType(): Warning got unknown plot type " << u16PlotType << ". Will not plot data" << endl;
        break;
    }

    }

    //Store the current plot type
    m_u16PlotType = u16PlotType;

    //Strobe auto scale:
    m_pPowerPlotWidget->strobeAutoscale(100);
    m_pStokesPhasePlotWidget->strobeAutoscale(100);

    m_pBandPowerPlotWidget->resetHistory();
    m_pBandPowerPlotWidget->strobeAutoscale(5000);
}

void cPlotsWidget::slotPowerPlotEnabled(bool bEnabled)
{
    m_pPowerPlotWidget->setVisible(bEnabled);

    QWriteLocker oLock(&m_oMutex);
    m_bPowerEnabled = bEnabled;

    sigPowerPlotEnabled(bEnabled);
}

void cPlotsWidget::slotStokesPhasePlotEnabled(bool bEnabled)
{
    m_pStokesPhasePlotWidget->setVisible(bEnabled);

    QWriteLocker oLock(&m_oMutex);
    m_bStokesPhaseEnabled = bEnabled;

    sigStokesPhasePlotEnabled(bEnabled);
}

void cPlotsWidget::slotBandPowerPlotEnabled(bool bEnabled)
{
    m_pBandPowerPlotWidget->setVisible(bEnabled);

    QWriteLocker oLock(&m_oMutex);
    m_bBandPowerEnabled = bEnabled;

    //Remove band lines from the power plot while the band power power is hidden
    m_pPowerPlotWidget->slotShowVerticalLines(bEnabled);

    sigBandPowerPlotEnabled(bEnabled);
}

void cPlotsWidget::socketConnected_callback()
{
    sigConnected();
    sigConnected(true);
}

void cPlotsWidget::socketDisconnected_callback()
{
    setIsRunning(false);

    sigDisconnect(); //Queued connection to slotDisconnect in this class.
    //This will be called by the socket reading thread. so it needs to be decoupled with a queued connection
    //The slotDisconnect asked the socket reading thread to join so this queued connection prevents the socket
    //reading thread from joining itself.
}

void cPlotsWidget::accumulationLength_callback(int64_t i64Timestamp_us, uint32_t u32NFrames)
{
    //Update accumulation length

    QWriteLocker oLock(&m_oMutex);

    m_u32AccumulationLength_nFrames = u32NFrames;
}

void cPlotsWidget::slotEnablePowerPlot(bool bEnable)
{
    blockSignals(true); //Don't notify change to other classes and the request came from outside this class

    m_pUI->groupBox_powers->setChecked(bEnable);
    slotPowerPlotEnabled(bEnable);

    blockSignals(false);
}

void cPlotsWidget::slotEnableStokesPhasePlot(bool bEnable)
{
   blockSignals(true);

    m_pUI->groupBox_stokesPhase->setChecked(bEnable);
    slotStokesPhasePlotEnabled(bEnable);

    blockSignals(false);
}

void cPlotsWidget::slotEnableBandPowerPlot(bool bEnable)
{
    blockSignals(true);

    m_pUI->groupBox_bandPower->setChecked(bEnable);
    slotBandPowerPlotEnabled(bEnable);

    blockSignals(false);
}
