//System includes
#include <cmath>
#include <iostream>

//Library includes
#include <QPen>
#include <QThread>
#include <qwt_scale_engine.h>
#include <QDebug>
#include <qwt_picker.h>

//Local includes
#include "QwtLinePlotWidget.h"
#include "ui_QwtLinePlotWidget.h"
#include "AVNUtilLibs/Timestamp/Timestamp.h"

using namespace std;

cQwtLinePlotWidget::cQwtLinePlotWidget(QWidget *pParent) :
    QWidget(pParent),
    m_pUI(new Ui::cQwtLinePlotWidget),
    m_u32NextHistoryInputIndex(0),
    m_dXBegin(0.0),
    m_dXEnd(1.0),
    m_bXSpanChanged(true),
    m_bIsGridEnabled(true),
    m_bIsPaused(false),
    m_bIsAutoscaleEnabled(false),
    m_u32Averaging(1),
    m_bTimestampInTitleEnabled(true),
    m_bDoLogConversion(false),
    m_bDoPowerLogConversion(false),
    m_bRejectData(true)
{
    m_pUI->setupUi(this);

    slotSetAverage(1);

    //Black background canvas and grid lines by default
    //The background colour is not currently changable. A mutator can be added as necessary
    m_pUI->qwtPlot->setCanvasBackground(QBrush(QColor(Qt::black)));
    enablePlotGrid(true);

    //Set up other plot controls
    m_pPlotPicker = new cQwtLinePlotPicker(QwtPlot::xBottom, QwtPlot::yLeft, QwtPicker::RectRubberBand, QwtPicker::AlwaysOn, m_pUI->qwtPlot->canvas());
    m_pPlotPicker->setTrackerPen(QPen(Qt::white));

    m_pPlotZoomer = new QwtPlotZoomer(m_pUI->qwtPlot->canvas());
    m_pPlotZoomer->setMousePattern(QwtEventPattern::MouseSelect2, Qt::RightButton, Qt::ControlModifier);
    m_pPlotZoomer->setMousePattern(QwtEventPattern::MouseSelect3, Qt::RightButton);
    m_pPlotZoomer->setRubberBandPen(QPen(Qt::white));
    m_pPlotZoomer->setTrackerMode(QwtPicker::AlwaysOff); //Use our own picker.

    m_pPlotPanner = new QwtPlotPanner(m_pUI->qwtPlot->canvas());
    m_pPlotPanner->setAxisEnabled(QwtPlot::yRight, false);
    m_pPlotPanner->setMouseButton(Qt::MidButton);

    m_pPlotMagnifier = new QwtPlotMagnifier(m_pUI->qwtPlot->canvas());
    m_pPlotMagnifier->setAxisEnabled(QwtPlot::yRight, false);
    m_pPlotMagnifier->setMouseButton(Qt::NoButton);
    m_pPlotMagnifier->setWheelFactor(1.1);

    m_pUI->qwtPlot->setAutoReplot(true);

    //Create colours for the plot curves
    m_qveCurveColours.push_back(Qt::red);
    m_qveCurveColours.push_back(Qt::green);
    m_qveCurveColours.push_back(Qt::blue);
    m_qveCurveColours.push_back(Qt::yellow);
    m_qveCurveColours.push_back(Qt::darkRed);
    m_qveCurveColours.push_back(Qt::darkGreen);
    m_qveCurveColours.push_back(Qt::darkBlue);
    m_qveCurveColours.push_back(Qt::darkYellow);

    connectSignalsToSlots();
}

cQwtLinePlotWidget::~cQwtLinePlotWidget()
{
    for(unsigned int uiChannelNo = 0; uiChannelNo < (unsigned)m_qvpPlotCurves.size(); uiChannelNo++)
    {
        delete m_qvpPlotCurves[uiChannelNo];
    }

    delete m_pUI;
}

void cQwtLinePlotWidget::connectSignalsToSlots()
{
    QObject::connect(m_pUI->pushButton_pauseResume, SIGNAL(clicked()), this, SLOT(slotPauseResume()));
    QObject::connect(m_pUI->checkBox_autoscale, SIGNAL(clicked(bool)), this, SLOT(slotEnableAutoscale(bool)));
    QObject::connect(m_pUI->spinBox_averaging, SIGNAL(valueChanged(int)), this, SLOT(slotSetAverage(int)));

    //Connections to update plot data as well as labels and scales are forced to be queued as the actual drawing of the widget needs to be done in the GUI thread
    //This allows an update request to come from an arbirary thread to get executed by the GUI thread
    qRegisterMetaType<QVector<double> >("QVector<double>");
    qRegisterMetaType<int64_t>("int64_t");
    QObject::connect(this, SIGNAL(sigUpdatePlotData(unsigned int,QVector<double>,QVector<double>,int64_t)),
                     this, SLOT(slotUpdatePlotData(unsigned int,QVector<double>,QVector<double>,int64_t)), Qt::QueuedConnection);

    QObject::connect(this, SIGNAL(sigUpdateScalesAndLabels()), this, SLOT(slotUpdateScalesAndLabels()), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(sigUpdateXScaleBase(int)), this, SLOT(slotUpdateXScaleBase(int)), Qt::QueuedConnection);
}


void cQwtLinePlotWidget::addData(const QVector<QVector<float> > &qvvfData, int64_t i64Timestamp_us)
{
    //cout << "cQwtLinePlotWidget::addData(): Thread is: " << QThread::currentThread() << endl;

    //Safety flag which can be set by other threads if data is known not to be interpretable
    if(m_bRejectData)
        return;

    //Check that our history has the right number of channels
    if(m_qvvvfAverageHistory.size() != qvvfData.size())
    {
        m_qvvvfAverageHistory.resize(qvvfData.size());
        m_qvvdYDataToPlot.resize(qvvfData.size());
    }

    //Update history length
    m_oMutex.lockForRead(); //Ensure averaging doesn't change during this section

    for(uint32_t u32ChannelNo = 0; u32ChannelNo < (unsigned)m_qvvvfAverageHistory.size(); u32ChannelNo++)
    {
        m_qvvvfAverageHistory[u32ChannelNo].resize(m_u32Averaging);

        //Also ensure that all new history frames are the same size as the input data
        for(uint32_t u32HistoryEntry = 0; u32HistoryEntry < (unsigned)m_qvvvfAverageHistory[u32ChannelNo].size(); u32HistoryEntry++)
        {
            m_qvvvfAverageHistory[u32ChannelNo][u32HistoryEntry].resize(qvvfData[u32ChannelNo].size());
        }

        //Update number of samples in the array sent to the plotting widget too
        m_qvvdYDataToPlot[u32ChannelNo].resize(qvvfData[u32ChannelNo].size());
    }

    m_oMutex.unlock();


    //Update X data
    m_oMutex.lockForRead(); //Ensure span doesn't change during this section

    if(m_qvdXDataToPlot.size() != qvvfData[0].size() || m_bXSpanChanged)
    {
        m_qvdXDataToPlot.resize(qvvfData[0].size());

        double dInterval = (m_dXEnd - m_dXBegin) / (double)(m_qvdXDataToPlot.size() - 1);

        for(uint32_t u32XTick = 0; u32XTick < (unsigned)m_qvdXDataToPlot.size(); u32XTick++)
        {
            m_qvdXDataToPlot[u32XTick] = m_dXBegin + u32XTick * dInterval;
        }

        //Check if number of points is base 2:
        double dExponent = log2(m_qvdXDataToPlot.size());
        double dIntegerPart; //Unused

        if(modf(dExponent, &dIntegerPart) == 0.0)
        {
            sigUpdateXScaleBase(2);
            //Plot grid lines along base 2 numbers
        }
        else
        {
            sigUpdateXScaleBase(10);
            //Otherwise plot grid lines along base 10 numbers
        }

        m_bXSpanChanged = false;
    }

    m_oMutex.unlock();

    //Wrap history circular buffer as necessary
    if(m_u32NextHistoryInputIndex >= (unsigned)m_qvvvfAverageHistory[0].size())
    {
        m_u32NextHistoryInputIndex = 0;
    }

    //Copy data into history
    for(uint32_t u32ChannelNo = 0; u32ChannelNo < (unsigned)m_qvvvfAverageHistory.size(); u32ChannelNo++)
    {
        std::copy(qvvfData[u32ChannelNo].begin(), qvvfData[u32ChannelNo].end(), m_qvvvfAverageHistory[u32ChannelNo][m_u32NextHistoryInputIndex].begin());
    }

    //Increment index for next data input
    m_u32NextHistoryInputIndex++;

    //Calculate Y data average to plot
    for(uint32_t u32ChannelNo = 0; u32ChannelNo < (unsigned)m_qvvdYDataToPlot.size(); u32ChannelNo++)
    {
        for(uint32_t u32SampleNo = 0; u32SampleNo < (unsigned)m_qvvdYDataToPlot[u32ChannelNo].size(); u32SampleNo++)
        {
            m_qvvdYDataToPlot[u32ChannelNo][u32SampleNo] = 0.0;

            //Sum
            for(uint32_t u32HistoryEntry = 0; u32HistoryEntry < (unsigned)m_qvvvfAverageHistory[u32ChannelNo].size(); u32HistoryEntry++)
            {
                m_qvvdYDataToPlot[u32ChannelNo][u32SampleNo] += m_qvvvfAverageHistory[u32ChannelNo][u32HistoryEntry][u32SampleNo];
            }

            //Divide
            m_qvvdYDataToPlot[u32ChannelNo][u32SampleNo] /= m_qvvvfAverageHistory[u32ChannelNo].size();

            //cout << "m_qvvdYDataToPlot[" << u32ChannelNo << "][" << u32SampleNo << "] = " << m_qvvdYDataToPlot[u32ChannelNo][u32SampleNo] << endl;
        }

        m_oMutex.lockForRead(); //Ensure the 2 bool flags don't change during these operations

        if(m_bDoLogConversion)
        {
            for(uint32_t u32SampleNo = 0; u32SampleNo <  (unsigned)m_qvvdYDataToPlot[u32ChannelNo].size(); u32SampleNo++)
            {
                m_qvvdYDataToPlot[u32ChannelNo][u32SampleNo] = 10 * log10(m_qvvdYDataToPlot[u32ChannelNo][u32SampleNo] + 0.001);
            }
        }

        if(m_bDoPowerLogConversion)
        {
            for(uint32_t u32SampleNo = 0; u32SampleNo <  (unsigned)m_qvvdYDataToPlot[u32ChannelNo].size(); u32SampleNo++)
            {
                m_qvvdYDataToPlot[u32ChannelNo][u32SampleNo] = 20 * log10(m_qvvdYDataToPlot[u32ChannelNo][u32SampleNo]  + 0.001);
            }
        }

        m_oMutex.unlock();
    }

    m_oMutex.lockForRead(); //Lock for pause flag

    if(!m_bIsPaused)
    {
        //Make sure that there is a curve for each channel
        if(m_qvpPlotCurves.size() != m_qvvdYDataToPlot.size())
        {
            //If not, delete existing curves
            for(uint32_t u32ChannelNo = 0; u32ChannelNo < (unsigned)m_qvpPlotCurves.size(); u32ChannelNo++)
            {
                delete m_qvpPlotCurves[u32ChannelNo];
            }
            m_qvpPlotCurves.clear();

            //Create new curves
            for(unsigned int u32ChannelNo = 0; u32ChannelNo < (unsigned)m_qvvdYDataToPlot.size(); u32ChannelNo++)
            {
                m_qvpPlotCurves.push_back(new QwtPlotCurve(QString("Channel %1").arg(u32ChannelNo)));
                m_qvpPlotCurves[u32ChannelNo]->attach(m_pUI->qwtPlot);
                m_qvpPlotCurves[u32ChannelNo]->setPen(m_qveCurveColours[u32ChannelNo], 1.0, Qt::SolidLine);
            }
        }

        for(uint32_t u32ChannelNo = 0; u32ChannelNo < (unsigned)m_qvpPlotCurves.size(); u32ChannelNo++)
        {
            sigUpdatePlotData(u32ChannelNo, m_qvdXDataToPlot, m_qvvdYDataToPlot[u32ChannelNo], i64Timestamp_us);
        }
    }

    m_oMutex.unlock();
}

void cQwtLinePlotWidget::setXLabel(const QString &qstrXLabel)
{
    m_qstrXLabel = qstrXLabel;

    sigUpdateScalesAndLabels();
}

void cQwtLinePlotWidget::setXUnit(const QString &qstrXUnit)
{
    m_qstrXUnit = qstrXUnit;

    sigUpdateScalesAndLabels();
}

void cQwtLinePlotWidget::setYLabel(const QString &qstrYLabel)
{
    m_qstrYLabel = qstrYLabel;

    sigUpdateScalesAndLabels();
}

void cQwtLinePlotWidget::setYUnit(const QString &qstrYUnit)
{
    m_qstrYUnit = qstrYUnit;

    sigUpdateScalesAndLabels();
}

void cQwtLinePlotWidget::setTitle(const QString &qstrTitle)
{
    m_qstrTitle = qstrTitle;

    sigUpdateScalesAndLabels();
}

void cQwtLinePlotWidget::enablePlotGrid(bool bEnable)
{
    m_bIsGridEnabled = bEnable;
    if(m_bIsGridEnabled)
    {
        QColor oGridlineColour(Qt::gray);
        oGridlineColour.setAlphaF(0.7);

#if QWT_VERSION < 0x060100 //Account for Ubuntu's typically outdated package versions
        m_oPlotGrid.setMajPen(QPen(Qt::gray, 0.5, Qt::SolidLine ));
#else
        m_oPlotGrid.setMajorPen(QPen(Qt::gray, 0.5, Qt::SolidLine ));
#endif

        m_oPlotGrid.attach(m_pUI->qwtPlot);
    }
    else
    {
        m_oPlotGrid.detach();
    }
}

void cQwtLinePlotWidget::enableAutoscaleControl(bool bEnable)
{
    m_pUI->checkBox_autoscale->setVisible(bEnable);
}

void cQwtLinePlotWidget::enableAveragingControl(bool bEnable)
{
    m_pUI->spinBox_averaging->setVisible(bEnable);
    m_pUI->label_averaging->setVisible(bEnable);
}

void cQwtLinePlotWidget::enablePauseControl(bool bEnable)
{

    m_pUI->pushButton_pauseResume->setVisible(bEnable);
}

void cQwtLinePlotWidget::slotSetAverage(int iAveraging)
{
    QWriteLocker oWriteLock(&m_oMutex);

    m_u32Averaging = iAveraging;
}

void cQwtLinePlotWidget::slotPauseResume()
{
    slotPause(!m_bIsPaused);
}

void cQwtLinePlotWidget::slotPause(bool bPause)
{
    QWriteLocker oWriteLock(&m_oMutex);

    m_bIsPaused = bPause;

    if(m_bIsPaused)
        m_pUI->pushButton_pauseResume->setText(QString("Resume"));
    else
        m_pUI->pushButton_pauseResume->setText(QString("Pause"));
}

void cQwtLinePlotWidget::slotEnableAutoscale(bool bEnable)
{
    //If autoscale is being disabled set the zoom base the current Y zoom
    //This allows the user to strobe the autoscale to get a useful zoom base
    if(m_bIsAutoscaleEnabled && !bEnable)
    {
        QRectF oZoomBase = m_pPlotZoomer->zoomBase();
        QwtInterval oAutoscaledYRange = m_pUI->qwtPlot->axisInterval(QwtPlot::yLeft);

        oZoomBase.setTop(oAutoscaledYRange.maxValue());
        oZoomBase.setBottom(oAutoscaledYRange.maxValue());

        m_pPlotZoomer->setZoomBase(oZoomBase);
    }

    m_bIsAutoscaleEnabled = bEnable;

    m_pUI->qwtPlot->setAxisAutoScale(QwtPlot::yLeft, m_bIsAutoscaleEnabled);

    cout << "cQwtLinePlotWidget::slotEnableAutoscale() Autoscale for plot \"" << m_qstrTitle.toStdString() << "\" set to " << m_bIsAutoscaleEnabled << endl;
}

void cQwtLinePlotWidget::enableTimestampInTitle(bool bEnable)
{
    m_bTimestampInTitleEnabled = bEnable;

    if(!m_bTimestampInTitleEnabled)
        m_pUI->qwtPlot->setTitle(m_qstrTitle);
}

void cQwtLinePlotWidget::enableLogConversion(bool bEnable)
{
    QWriteLocker oWriteLock(&m_oMutex);

    m_bDoLogConversion = bEnable;

    if(m_bDoLogConversion)
        m_bDoPowerLogConversion = false;
}

void cQwtLinePlotWidget::enablePowerLogConversion(bool bEnable)
{
    QWriteLocker oWriteLock(&m_oMutex);

    m_bDoPowerLogConversion = bEnable;

    if(m_bDoPowerLogConversion)
        m_bDoLogConversion = false;
}

void cQwtLinePlotWidget::enableRejectData(bool bEnable)
{
    QWriteLocker oWriteLock(&m_oMutex);

    m_bRejectData = bEnable;
}

void cQwtLinePlotWidget::setXSpan(double dXBegin, double dXEnd)
{
    QWriteLocker oWriteLock(&m_oMutex);

    m_dXBegin = dXBegin;
    m_dXEnd = dXEnd;

    //Flag for ploting code to update
    m_bXSpanChanged = true;
}

void cQwtLinePlotWidget::slotUpdatePlotData(unsigned int uiCurveNo, QVector<double> qvdXData, QVector<double> qvdYData, int64_t i64Timestamp_us)
{
    //This function sends data to the actually plot widget in the GUI thread. This is necessary as draw the curve (i.e. updating the GUI) must be done in the GUI thread.
    //Connections to this slot should be queued if from signals not orginating from the GUI thread.

    //cout << "cQwtLinePlotWidget::slotUpdatePlotData() Thread is: " << QThread::currentThread() << endl;

    if(uiCurveNo >= (unsigned int)m_qvpPlotCurves.size())
    {
        cout << "cQwtLinePlotWidget::slotUpdatePlotData(): Warning: Requested plotting for curve index "
             << uiCurveNo << " which is out of range [0, " << m_qvpPlotCurves.size() - 1 << "]. Ignoring." << endl;

        return;
    }

    m_qvpPlotCurves[uiCurveNo]->setSamples(qvdXData, qvdYData);

    //Update horizontal scale
    QRectF oRect = m_pPlotZoomer->zoomBase();
    if(oRect.left() != qvdXData.first() || oRect.right() != qvdXData.last() )
    {
        oRect.setLeft(qvdXData.first() );
        oRect.setRight(qvdXData.last() );

        m_pPlotZoomer->setZoomBase(oRect);
        m_pPlotZoomer->zoom(oRect);
    }

    //Update timestamp in Title if needed
    if(m_bTimestampInTitleEnabled)
        m_pUI->qwtPlot->setTitle( QString("%1 - %2").arg(m_qstrTitle).arg(AVN::stringFromTimestamp(i64Timestamp_us).c_str()) );
}

void cQwtLinePlotWidget::slotUpdateScalesAndLabels()
{
    if(m_qstrXUnit.length())
        m_pUI->qwtPlot->setAxisTitle(QwtPlot::xBottom, QString("%1 [%2]").arg(m_qstrXLabel).arg(m_qstrXUnit));
    else
        m_pUI->qwtPlot->setAxisTitle(QwtPlot::xBottom, QString("%1").arg(m_qstrXLabel));

    if(m_qstrYUnit.length())
        m_pUI->qwtPlot->setAxisTitle(QwtPlot::yLeft, QString("%1 [%2]").arg(m_qstrYLabel).arg(m_qstrYUnit));
    else
        m_pUI->qwtPlot->setAxisTitle(QwtPlot::yLeft, QString("%1").arg(m_qstrYLabel));

    m_pPlotPicker->setXUnit(m_qstrXUnit);
    m_pPlotPicker->setYUnit(m_qstrYUnit);

    m_pUI->qwtPlot->setTitle(m_qstrTitle);
}

void cQwtLinePlotWidget::slotUpdateXScaleBase(int iBase)
{
    m_pUI->qwtPlot->axisScaleEngine(QwtPlot::xBottom)->setBase(iBase);
}
