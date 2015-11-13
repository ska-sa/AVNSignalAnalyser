//System includes
#include <iostream>

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

cPlotsWidget::cPlotsWidget(QWidget *parent) :
    QWidget(parent),
    m_pUI(new Ui::cPlotsWidget),
    m_pThis(this),
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

    m_pBandPowerPlotWidget->setSpanLengthControlScalingFactor(1, QString("s")); //Band power span is in seconds:

    //Plot enabling/disabling
    QObject::connect(m_pUI->groupBox_powers, SIGNAL(clicked(bool)), this, SLOT(slotPowerWidgetEnabled(bool)));
    QObject::connect(m_pUI->groupBox_stokes, SIGNAL(clicked(bool)), this, SLOT(slotStokesWidgetEnabled(bool)));
    QObject::connect(m_pUI->groupBox_bandPower, SIGNAL(clicked(bool)), this, SLOT(slotBandPowerWidgetEnabled(bool)));

    //Vertical lines for intergrated bandwidth
    QObject::connect(m_pBandPowerPlotWidget, SIGNAL(sigSelectedBandChanged(QVector<double>)), m_pPowerPlotWidget, SLOT(slotDrawVerticalLines(QVector<double>)));
    QObject::connect(m_pBandPowerPlotWidget, SIGNAL(sigSelectedBandChanged(QVector<double>)), m_pStokesPlotWidget, SLOT(slotDrawVerticalLines(QVector<double>)));

    //SocketStreamer triggered disconnect
    QObject::connect(this, SIGNAL(sigDisconnect()), this, SLOT(slotDisconnect()), Qt::QueuedConnection); //Must be queued to prevent the socket reading thread try to join itself.
}

cPlotsWidget::~cPlotsWidget()
{
    setIsRunning(false);
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
        boost::static_pointer_cast<cTCPReceiver>(m_pSocketReceiver)->registerNoticationCallbackHandler(m_pThis);

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

    QVector<uint32_t> qvu32StokesQUChanList;
    qvu32StokesQUChanList.push_back(2);
    qvu32StokesQUChanList.push_back(3);

    float *fpPowerLeft = NULL;
    float *fpPowerRight = NULL;
    float *fpStokesQ = NULL;
    float *fpStokesU = NULL;

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
        fpPowerLeft = &qvvfPlotData[0].front();
        fpPowerRight = &qvvfPlotData[1].front();
        fpStokesQ = &qvvfPlotData[2].front();
        fpStokesU = &qvvfPlotData[3].front();

        //Update plotting widget to plot data correctly
        updatePlotType(m_pStreamInterpreter->getLastHeader().getDigitiserType());

        while(isRunning())
        {
            //Get the data from the next frame. Return false means an inconsistency was incountered
            if(!m_pStreamInterpreter->getNextFrame(fpPowerLeft, fpPowerRight, fpStokesQ, fpStokesU, qvvfPlotData[0].size()))
            {
                break; //Causes resync
            }

            QReadLocker oLock(&m_oMutex);

            if(m_bPowerEnabled)
            {
                m_pPowerPlotWidget->addData(qvvfPlotData, m_pStreamInterpreter->getLastHeader().getTimestamp_us(), qvu32PowerLRChanList);
            }

            if(m_bStokesEnabled)
            {
                m_pStokesPlotWidget->addData(qvvfPlotData, m_pStreamInterpreter->getLastHeader().getTimestamp_us(), qvu32StokesQUChanList);
            }

            if(m_bBandPowerEnabled)
            {
                m_pBandPowerPlotWidget->addData(qvvfPlotData, m_pStreamInterpreter->getLastHeader().getTimestamp_us());
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
        m_pBandPowerPlotWidget->setSelectableBand(0.0, 400.0, QString("MHz"));

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
        m_pBandPowerPlotWidget->setSelectableBand(0.0, 400.0, QString("MHz"));

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
        m_pBandPowerPlotWidget->setSelectableBand(-781.25, 781.25, QString("kHz"));

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
        m_pBandPowerPlotWidget->setSelectableBand(-781.25, 781.25, QString("kHz"));

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
