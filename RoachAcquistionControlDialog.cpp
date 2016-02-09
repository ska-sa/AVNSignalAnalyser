//System includes
#include <climits>

//Libary includes
#ifndef Q_MOC_RUN //Qt's MOC and Boost have some issues don't let MOC process boost headers
#include <boost/make_shared.hpp>
#endif

//Local includes
#include "RoachAcquistionControlDialog.h"
#include "ui_RoachAcquistionControlDialog.h"
#include "AVNUtilLibs/Timestamp/Timestamp.h"
#include "AVNDataTypes/SpectrometerDataStream/cSpectrometerOverflowRegisters.h"

using namespace std;

cRoachAcquistionControlDialog::cRoachAcquistionControlDialog(const QString &qstrHostname, uint16_t u16Port, QWidget *pParent) :
    QDialog(pParent),
    m_pUI(new Ui::cRoachAcquistionControlDialog),
    m_eTimeSpec(Qt::LocalTime),
    m_bIsRecording(false),
    m_bStationControllerKATCPConnected(false),
    m_dFrequencyRFChan0_MHz(0.0),
    m_dFrequencyRFChan1_MHz(0.0),

    m_bRoachKATCPConnected(false),
    m_u32AccumulationLength_nFrames(0),
    m_u32CoarseChannelSelect(0),
    m_dFrequencyFs_MHz(0.0),
    m_u32SizeOfCoarseFFT_nSamp(0),
    m_u32SizeOfFineFFT_nSamp(0),
    m_u32CoarseFFTShiftMask(0),
    m_dAttenuationADCChan0_dB(0.0),
    m_dAttenuationADCChan1_dB(0.0),
    m_bNoiseDiodeEnabled(false),
    m_bNoiseDiodeDutyCycleEnabled(false),
    m_u32NoiseDiodeDutyCycleOnDuration_nAccs(0),
    m_u32NoiseDiodeDutyCycleOffDuration_nAccs(0),
    m_bEth10GbEUp(false),
    m_u32PPSCount(0),
    m_u32PreviousPPSCount(0),
    m_u32ClockFrequency_Hz(0)

{
    m_pUI->setupUi(this);

    connectSignalToSlots();

    connect(qstrHostname, u16Port);
}

cRoachAcquistionControlDialog::cRoachAcquistionControlDialog(QWidget *pParent) :
    QDialog(pParent),
    m_pUI(new Ui::cRoachAcquistionControlDialog),
    m_eTimeSpec(Qt::LocalTime),
    m_bIsRecording(false)
{
    m_pUI->setupUi(this);

    connectSignalToSlots();
}

cRoachAcquistionControlDialog::~cRoachAcquistionControlDialog()
{
    delete m_pUI;
}

void cRoachAcquistionControlDialog::connect(const QString &qstrHostname, uint16_t u16Port)
{
    m_pKATCPClient = boost::make_shared<cRoachAcquisitionServerKATCPClient>();
    m_pKATCPClient->registerCallbackHandler(this);

    m_pKATCPClient->connect(qstrHostname.toStdString(), u16Port);

    m_oSecondTimer.start(1000);
    m_oTwoSecondTimer.start(2000);
}

void cRoachAcquistionControlDialog::connectSignalToSlots()
{
    QObject::connect( &m_oSecondTimer, SIGNAL(timeout()), this, SLOT(slotSecondTimerTrigger()) );
    QObject::connect( &m_oTwoSecondTimer, SIGNAL(timeout()), this, SLOT(slotTwoSecondTimerTrigger()) );
    QObject::connect( m_pUI->pushButton_startStopRecording, SIGNAL(pressed()), this, SLOT(slotStartStopRecordingClicked()) );
    QObject::connect( m_pUI->comboBox_timeZone, SIGNAL(currentIndexChanged(QString)), this, SLOT(slotTimeZoneChanged(QString)) );

    //Select last editted option
    QObject::connect( m_pUI->timeEdit_recordAt, SIGNAL(editingFinished()), m_pUI->radioButton_recordAt, SLOT(click()) );
    QObject::connect( m_pUI->doubleSpinBox_recordIn, SIGNAL(editingFinished()), m_pUI->radioButton_recordIn, SLOT(click()) );
    QObject::connect( m_pUI->doubleSpinBox_recordFor, SIGNAL(editingFinished()), m_pUI->radioButton_recordFor, SLOT(click()) );


    //Call backs that alter the GUI decoupled by Queued connections to be executed by the GUI thread
    qRegisterMetaType<int64_t>("int64_t");
    qRegisterMetaType<uint64_t>("uint64_t");
    QObject::connect( this, SIGNAL(sigRecordingInfoUpdate(QString,int64_t,int64_t,int64_t,int64_t,uint64_t, uint64_t)),
                      this, SLOT(slotRecordingInfoUpdate(QString,int64_t,int64_t,int64_t,int64_t,uint64_t, uint64_t)), Qt::QueuedConnection);
    QObject::connect( this, SIGNAL(sigRecordingStarted()), this, SLOT(slotRecordingStarted()), Qt::QueuedConnection);
    QObject::connect( this, SIGNAL(sigRecordingStoppped()), this, SLOT(slotRecordingStoppped()), Qt::QueuedConnection);

    QObject::connect( this, SIGNAL(sigUpdateRoachGUIParameters()), this, SLOT(slotUpdateRoachGUIParameters()), Qt::QueuedConnection);

}

void cRoachAcquistionControlDialog::slotSecondTimerTrigger()
{
    //Update the start time in the GUI every second to the current time.
    QDateTime oTimeNow = QDateTime::currentDateTime().addSecs(2); //Now plus 2 seconds

    if(m_eTimeSpec == Qt::UTC)
    {
        oTimeNow = oTimeNow.toUTC();
    }

    //Update the start time to time now if it is older than time now
    //Do change if the user is busy editing the box
    if(!m_pUI->timeEdit_recordAt->hasFocus() && m_pUI->timeEdit_recordAt->dateTime() < oTimeNow)
    {
        m_pUI->timeEdit_recordAt->setDateTime(oTimeNow);
    }

    //Update the recording info
    QReadLocker oLock(&m_oMutex);
    if(m_bIsRecording)
    {
        m_pKATCPClient->requestRecordingInfoUpdate();
    }

}

void cRoachAcquistionControlDialog::slotTwoSecondTimerTrigger()
{
    //Check PPS progress:
    //Each 2 seconds check that the PPS has progressed by at least 1

    boost::unique_lock<boost::mutex> oLock(m_oParameterMutex);

    if(m_u32PPSCount - m_u32PreviousPPSCount)
    {
        m_pUI->label_ppsCount->setStyleSheet("QLabel { background-color : pallete(label)}");
        m_bPPSValid = true;
    }
    else
    {
        m_pUI->label_ppsCount->setStyleSheet("QLabel { background-color : red}");
        m_bPPSValid = false;
    }

    m_u32PreviousPPSCount = m_u32PPSCount;
}

void cRoachAcquistionControlDialog::slotStartStopRecordingClicked()
{
    if(m_bIsRecording)
    {
        m_pKATCPClient->requestStopRecording();
    }
    else
    {
        int64_t i64StartTime_us = 0;
        int64_t i64Duration_us = 0;

        if(m_pUI->radioButton_recordAt->isChecked())
        {
            i64StartTime_us = m_pUI->timeEdit_recordAt->dateTime().toMSecsSinceEpoch() * 1000;
        }
        else if(m_pUI->radioButton_recordIn->isChecked())
        {
            i64StartTime_us = QDateTime::currentMSecsSinceEpoch() * 1000;

            if(m_pUI->comboBox_recordInUnits->currentText() == QString("seconds"))
            {
                i64StartTime_us += m_pUI->doubleSpinBox_recordIn->value() * 1e6;
            }
            else if(m_pUI->comboBox_recordInUnits->currentText() == QString("minutes"))
            {
                i64StartTime_us += m_pUI->doubleSpinBox_recordIn->value() * 60e6;
            }
            else if(m_pUI->comboBox_recordInUnits->currentText() == QString("hours"))
            {
                i64StartTime_us += m_pUI->doubleSpinBox_recordIn->value() * 3600e6;
            }
        }


        if(m_pUI->radioButton_recordFor->isChecked())
        {
            if(m_pUI->comboBox_recordForUnits->currentText() == QString("seconds"))
            {
                i64Duration_us = m_pUI->doubleSpinBox_recordFor->value() * 1e6;
            }
            else if(m_pUI->comboBox_recordForUnits->currentText() == QString("minutes"))
            {
                i64Duration_us = m_pUI->doubleSpinBox_recordFor->value() * 60e6;
            }
            else if(m_pUI->comboBox_recordForUnits->currentText() == QString("hours"))
            {
                i64Duration_us = m_pUI->doubleSpinBox_recordFor->value() * 3600e6;
            }
        }

        m_pKATCPClient->requestStartRecording(m_pUI->lineEdit_filenamePrefix->text().toStdString(), i64StartTime_us, i64Duration_us);

        cout << "cRoachAcquistionControlDialog::slotStartStopRecordingClicked(): Requesting recording start time of " << AVN::stringFromTimestamp_full(i64StartTime_us) << endl;
    }
}

void cRoachAcquistionControlDialog::connected_callback(bool bConnected)
{
    sigKATCPSocketConnected(bConnected);
    cout << "cRoachAcquistionControlDialog::connected_callback() Got connected = " << bConnected << " callback" << endl;
}

void cRoachAcquistionControlDialog::recordingStarted_callback()
{
    sigRecordingStarted();
    cout << "cRoachAcquistionControlDialog::recordingStarted_callback() Got recording started callback" << endl;
}

void cRoachAcquistionControlDialog::recordingStopped_callback()
{
    sigRecordingStoppped();
    cout << "cRoachAcquistionControlDialog::recordingStopped_callback() Got recording stopped callback" << endl;
}

void cRoachAcquistionControlDialog::recordingInfoUpdate_callback(const string &strFilename, int64_t i64StartTime_us, int64_t i64EllapsedTime_us,
                                                                 int64_t i64StopTime_us, int64_t i64TimeLeft_us,
                                                                 uint64_t u64CurrentFileSize_B, uint64_t u64DiskSpaceRemaining_B)
{
    //Send this info to private slot via queued connection to change GUI. Needs to be a queued connection for execution from the
    //main (GUI) thread. You can't alter the GUI from arbitary threads.

    sigRecordingInfoUpdate(QString(strFilename.c_str()), i64StartTime_us, i64EllapsedTime_us, i64StopTime_us, i64TimeLeft_us, u64CurrentFileSize_B, u64DiskSpaceRemaining_B);
}

void cRoachAcquistionControlDialog::stationControllerKATCPConnected_callback(bool bConnected)
{
    boost::unique_lock<boost::mutex> oLock(m_oParameterMutex);

    m_bStationControllerKATCPConnected = bConnected;

    sigUpdateRoachGUIParameters();
}

void cRoachAcquistionControlDialog::actualAntennaAz_callback(int64_t i64Timestamp_us, double dAzimuth_deg)
{
    //TODO: Add to plotting
}

void cRoachAcquistionControlDialog::actualAntennaEl_callback(int64_t i64Timestamp_us, double dElevation_deg)
{
    //TODO: Add to plotting
}

void cRoachAcquistionControlDialog::actualSourceOffsetAz_callback(int64_t i64Timestamp_us, double dAzimuthOffset_deg)
{
    //TODO: Add to plotting
}

void cRoachAcquistionControlDialog::actualSourceOffsetEl_callback(int64_t i64Timestamp_us, double dElevationOffset_deg)
{
    //TODO: Add to plotting
}

void cRoachAcquistionControlDialog::frequencyRFChan0_callback(int64_t i64Timestamp_us, double dFrequencyRFChan0_MHz)
{
    boost::unique_lock<boost::mutex> oLock(m_oParameterMutex);

    m_dFrequencyRFChan0_MHz = dFrequencyRFChan0_MHz;

    sigUpdateRoachGUIParameters();
}

void cRoachAcquistionControlDialog::frequencyRFChan1_callback(int64_t i64Timestamp_us, double dFrequencyRFChan1_MHz)
{
    boost::unique_lock<boost::mutex> oLock(m_oParameterMutex);

    m_dFrequencyRFChan1_MHz = dFrequencyRFChan1_MHz;

    sigUpdateRoachGUIParameters();
}

void cRoachAcquistionControlDialog::roachKATCPConnected_callback(bool bConnected)
{
    boost::unique_lock<boost::mutex> oLock(m_oParameterMutex);

    m_bRoachKATCPConnected = bConnected;

    sigUpdateRoachGUIParameters();
}

void cRoachAcquistionControlDialog::accumulationLength_callback(int64_t i64Timestamp_us, uint32_t u32NFrames)
{
    boost::unique_lock<boost::mutex> oLock(m_oParameterMutex);

    m_u32AccumulationLength_nFrames = u32NFrames;

    sigUpdateRoachGUIParameters();
}

void cRoachAcquistionControlDialog::coarseChannelSelect_callback(int64_t i64Timestamp_us, uint32_t u32ChannelNo)
{
    boost::unique_lock<boost::mutex> oLock(m_oParameterMutex);

    m_u32CoarseChannelSelect = u32ChannelNo;

    sigUpdateRoachGUIParameters();
}

void cRoachAcquistionControlDialog::frequencyFs_callback(double dFrequencyFs_MHz)
{
    boost::unique_lock<boost::mutex> oLock(m_oParameterMutex);

    m_dFrequencyFs_MHz = dFrequencyFs_MHz;

    sigUpdateRoachGUIParameters();
}

void cRoachAcquistionControlDialog::sizeOfCoarseFFT_callback(uint32_t u32SizeOfCoarseFFT_nSamp)
{
    boost::unique_lock<boost::mutex> oLock(m_oParameterMutex);

    m_u32SizeOfCoarseFFT_nSamp = u32SizeOfCoarseFFT_nSamp;

    sigUpdateRoachGUIParameters();
}

void cRoachAcquistionControlDialog::sizeOfFineFFT_callback(uint32_t u32SizeOfCoarseFFT_nSamp)
{
    boost::unique_lock<boost::mutex> oLock(m_oParameterMutex);

    m_u32SizeOfFineFFT_nSamp = u32SizeOfCoarseFFT_nSamp;

    sigUpdateRoachGUIParameters();
}

void cRoachAcquistionControlDialog::coarseFFTShiftMask_callback(int64_t i64Timestamp_us, uint32_t u32ShiftMask)
{
    boost::unique_lock<boost::mutex> oLock(m_oParameterMutex);

    m_u32CoarseFFTShiftMask = u32ShiftMask;

    sigUpdateRoachGUIParameters();
}

void cRoachAcquistionControlDialog::attenuationADCChan0_callback(int64_t i64Timestamp_us, double dADCAttenuationChan0_dB)
{
    boost::unique_lock<boost::mutex> oLock(m_oParameterMutex);

    m_dAttenuationADCChan0_dB = dADCAttenuationChan0_dB;

    sigUpdateRoachGUIParameters();
}

void cRoachAcquistionControlDialog::attenuationADCChan1_callback(int64_t i64Timestamp_us, double dADCAttenuationChan1_dB)
{
    boost::unique_lock<boost::mutex> oLock(m_oParameterMutex);

    m_dAttenuationADCChan1_dB = dADCAttenuationChan1_dB;

    sigUpdateRoachGUIParameters();
}
void cRoachAcquistionControlDialog::noiseDiodeEnabled_callback(int64_t i64Timestamp_us, bool bNoiseDiodeEnabled)
{
    boost::unique_lock<boost::mutex> oLock(m_oParameterMutex);

    m_bNoiseDiodeEnabled = bNoiseDiodeEnabled;

    sigUpdateRoachGUIParameters();
}

void cRoachAcquistionControlDialog::noiseDiodeDutyCycleEnabled_callback(int64_t i64Timestamp_us, bool bNoiseDiodeDutyCyleEnabled)
{
    boost::unique_lock<boost::mutex> oLock(m_oParameterMutex);

    m_bNoiseDiodeDutyCycleEnabled = bNoiseDiodeDutyCyleEnabled;

    sigUpdateRoachGUIParameters();
}

void cRoachAcquistionControlDialog::noiseDiodeDutyCycleOnDuration_callback(int64_t i64Timestamp_us, uint32_t u32NAccums)
{
    boost::unique_lock<boost::mutex> oLock(m_oParameterMutex);

    m_u32NoiseDiodeDutyCycleOnDuration_nAccs = u32NAccums;

    sigUpdateRoachGUIParameters();
}

void cRoachAcquistionControlDialog::noiseDiodeDutyCycleOffDuration_callback(int64_t i64Timestamp_us, uint32_t u32NAccums)
{
    boost::unique_lock<boost::mutex> oLock(m_oParameterMutex);

    m_u32NoiseDiodeDutyCycleOffDuration_nAccs = u32NAccums;

    sigUpdateRoachGUIParameters();
}

void cRoachAcquistionControlDialog::overflowsRegs_callback(int64_t i64Timestamp_us, uint32_t u32OverflowRegs)
{
    boost::unique_lock<boost::mutex> oLock(m_oParameterMutex);

    m_u32OverflowsRegs = u32OverflowRegs;

    sigUpdateRoachGUIParameters();
}

void cRoachAcquistionControlDialog::eth10GbEUp_callback(int64_t i64Timestamp_us, bool bEth10GbEUp)
{
    boost::unique_lock<boost::mutex> oLock(m_oParameterMutex);

    m_bEth10GbEUp = bEth10GbEUp;

    sigUpdateRoachGUIParameters();
}

void cRoachAcquistionControlDialog::ppsCount_callback(int64_t i64Timestamp_us, uint32_t u32PPSCount)
{
    boost::unique_lock<boost::mutex> oLock(m_oParameterMutex);

    m_u32PPSCount = u32PPSCount;

    sigUpdateRoachGUIParameters();
}

void cRoachAcquistionControlDialog::clockFrequency_callback(int64_t i64Timestamp_us, uint32_t u32ClockFrequency_Hz)
{
    boost::unique_lock<boost::mutex> oLock(m_oParameterMutex);

    m_u32ClockFrequency_Hz = u32ClockFrequency_Hz;

    sigUpdateRoachGUIParameters();
}

void cRoachAcquistionControlDialog::slotUpdateRoachGUIParameters()
{
    //This design is not greatly efficient as it calls this function which writes all values for the update of any of them.
    //For now it appears good enough.

    //FYI connections to this slot is queued so it is save to lock the same mutex here as in the calling function.
    boost::unique_lock<boost::mutex> oLock(m_oParameterMutex);

    m_pUI->label_accumulationLength_nFrames->setText(QString("%1 frames").arg(m_u32AccumulationLength_nFrames));

    double dSingleAccumulationLength_ms = 1 / (m_dFrequencyFs_MHz * 1e6) * m_u32SizeOfCoarseFFT_nSamp;
    if(m_u32SizeOfFineFFT_nSamp)
    {
        dSingleAccumulationLength_ms *= m_u32SizeOfFineFFT_nSamp;
    }
    dSingleAccumulationLength_ms *= 1000;

    m_pUI->label_accumulationLength_ms->setText(QString("%1 ms").arg(m_u32AccumulationLength_nFrames * dSingleAccumulationLength_ms));

    if(m_u32SizeOfFineFFT_nSamp)
    {
        m_pUI->label_coarseChannelSelect_binNo->setText(QString("%1").arg(m_u32CoarseChannelSelect));

        double dCourseChannelSelectBaseband_MHz = (double)m_u32CoarseChannelSelect / (m_u32SizeOfCoarseFFT_nSamp / 2) * (m_dFrequencyFs_MHz / 2);
        m_pUI->label_coarseChannelSelect_baseband->setText(QString("%1 MHz").arg(dCourseChannelSelectBaseband_MHz));

        m_pUI->label_coarseChannelSelect_finalIF->setText(QString("%1 MHz").arg(m_dFrequencyFs_MHz - dCourseChannelSelectBaseband_MHz)); //Assume 2nd Nyquist zone

        //TODO: Not sure if this is right way around relative to RF.
        //Presumably, the first mixing stage is low side injection so no spectral flipping.
        //The second mixing stage is high side which causes a flip.
        //The sampling is second Nqyuist zone which also causes a flip and so the spectrum
        //The correct way around. Hence simply add the coarse channel frequnecy offset.
        if(m_dFrequencyRFChan0_MHz)
        {
            m_pUI->label_coarseChannelSelect_RF0->setText(QString("%1").arg(m_dFrequencyRFChan0_MHz + dCourseChannelSelectBaseband_MHz));
        }
        else
        {
            m_pUI->label_coarseChannelSelect_RF0->setText(QString("RF frequency unavailable."));
        }

        if(m_dFrequencyRFChan1_MHz)
        {
            m_pUI->label_coarseChannelSelect_RF1->setText(QString("%1").arg(m_dFrequencyRFChan1_MHz + dCourseChannelSelectBaseband_MHz));
        }
        else
        {
            m_pUI->label_coarseChannelSelect_RF1->setText(QString("RF frequency unavailable."));
        }

        m_pUI->spinBox_coarseChannelSelect_binNo->setEnabled(true);
        m_pUI->spinBox_coarseChannelFrequencyBaseband_MHz->setEnabled(true);
        m_pUI->spinBox_coarseChannelFrequencyIF_MHz->setEnabled(true);
        m_pUI->spinBox_coarseChannelFrequencyRF0_MHz->setEnabled(true);
        m_pUI->spinBox_coarseChannelFrequencyRF1_MHz->setEnabled(true);

        m_pUI->pushButton_sendCoarseChannelSelect_binNo->setEnabled(true);
        m_pUI->pushButton_sendCoarseChannelSelect_baseband->setEnabled(true);
        m_pUI->pushButton_sendCoarseChannelSelect_finalIF->setEnabled(true);
        m_pUI->pushButton_sendCoarseChannelSelect_RF0->setEnabled(true);
        m_pUI->pushButton_sendCoarseChannelSelect_RF1->setEnabled(true);
    }
    else
    {
        m_pUI->label_coarseChannelSelect_binNo->setText(QString("N/A"));
        m_pUI->label_coarseChannelSelect_baseband->setText(QString("N/A"));
        m_pUI->label_coarseChannelSelect_finalIF->setText(QString("N/A"));
        m_pUI->label_coarseChannelSelect_RF0->setText(QString("N/A"));
        m_pUI->label_coarseChannelSelect_RF1->setText(QString("N/A"));

        m_pUI->spinBox_coarseChannelSelect_binNo->setEnabled(false);
        m_pUI->spinBox_coarseChannelFrequencyBaseband_MHz->setEnabled(false);
        m_pUI->spinBox_coarseChannelFrequencyIF_MHz->setEnabled(false);
        m_pUI->spinBox_coarseChannelFrequencyRF0_MHz->setEnabled(false);
        m_pUI->spinBox_coarseChannelFrequencyRF1_MHz->setEnabled(false);

        m_pUI->pushButton_sendCoarseChannelSelect_binNo->setEnabled(false);
        m_pUI->pushButton_sendCoarseChannelSelect_baseband->setEnabled(false);
        m_pUI->pushButton_sendCoarseChannelSelect_finalIF->setEnabled(false);
        m_pUI->pushButton_sendCoarseChannelSelect_RF0->setEnabled(false);
        m_pUI->pushButton_sendCoarseChannelSelect_RF1->setEnabled(false);
    }

    m_pUI->label_coarseFFTShiftMask->setText(QString("%1").arg(m_u32CoarseFFTShiftMask));

    m_pUI->label_ADC0Attenuation->setText(QString("%1 dB").arg(m_dAttenuationADCChan0_dB));

    m_pUI->label_ADC1Attenuation->setText(QString("%1 dB").arg(m_dAttenuationADCChan1_dB));

    if(m_bNoiseDiodeEnabled)
    {
        m_pUI->label_noideDiodeEnabled->setText(QString("True"));
    }
    else
    {
        m_pUI->label_noideDiodeEnabled->setText(QString("False"));
    }

    m_pUI->spinBox_noiseDiodeDutyCycleOnDuration_ms->setEnabled(m_bNoiseDiodeEnabled && m_bNoiseDiodeDutyCycleEnabled);
    m_pUI->spinBox_noiseDiodeDutyCycleOffDuration_accums->setEnabled(m_bNoiseDiodeEnabled && m_bNoiseDiodeDutyCycleEnabled);
    m_pUI->spinBox_noiseDiodeDutyCycleOffDuration_ms->setEnabled(m_bNoiseDiodeEnabled && m_bNoiseDiodeDutyCycleEnabled);

    m_pUI->pushButton_toggleNoiseDiodeDutyCycleEnabled->setEnabled(m_bNoiseDiodeEnabled);
    m_pUI->pushButton_sendNoiseDiodeDutyCycleOnDuration_accums->setEnabled(m_bNoiseDiodeEnabled && m_bNoiseDiodeDutyCycleEnabled);
    m_pUI->pushButton_sendNoiseDiodeDutyCycleOnDuration_ms->setEnabled(m_bNoiseDiodeEnabled && m_bNoiseDiodeDutyCycleEnabled);
    m_pUI->pushButton_sendNoiseDiodeDutyCycleOffDuration_accums->setEnabled(m_bNoiseDiodeEnabled && m_bNoiseDiodeDutyCycleEnabled);
    m_pUI->pushButton_sendNoiseDiodeDutyCycleOffDuration_ms->setEnabled(m_bNoiseDiodeEnabled && m_bNoiseDiodeDutyCycleEnabled);

    if(m_bNoiseDiodeDutyCycleEnabled)
    {
        m_pUI->label_noiseDiodeDutyCycleEnabled->setText(QString("True"));
    }
    else
    {
        m_pUI->label_noiseDiodeDutyCycleEnabled->setText(QString("False"));
    }

    m_pUI->label_noiseDiodeDutyCycleOnDuration_accums->setText(QString("%1").arg(m_u32NoiseDiodeDutyCycleOnDuration_nAccs));

    m_pUI->label_noiseDiodeDutyCycleOnDuration_ms->setText(QString("%1 ms").arg(m_u32NoiseDiodeDutyCycleOnDuration_nAccs * dSingleAccumulationLength_ms));

    m_pUI->label_noiseDiodeDutyCycleOffDuration_accums->setText(QString("%1").arg(m_u32NoiseDiodeDutyCycleOffDuration_nAccs));

    m_pUI->label_noiseDiodeDutyCycleOffDuration_ms->setText(QString("%1 ms").arg(m_u32NoiseDiodeDutyCycleOffDuration_nAccs * dSingleAccumulationLength_ms));

    cSpectrometerOverflowRegisters oOverflowRegisters(m_u32OverflowsRegs);

    if(oOverflowRegisters.m_bADC0OverRange)
    {
        m_pUI->label_adc0OverRange->setText(QString("True"));
    }
    else
    {
        m_pUI->label_adc0OverRange->setText(QString("False"));
    }

    if(oOverflowRegisters.m_bADC1OverRange)
    {
        m_pUI->label_adc1OverRange->setText("True");
    }
    else
    {
        m_pUI->label_adc1OverRange->setText("False");
    }

    if(oOverflowRegisters.m_bCoarseFFT0Overflow)
    {
        m_pUI->label_coarseFFT0OverRange->setText("True");
    }
    else
    {
        m_pUI->label_coarseFFT0OverRange->setText("False");
    }

    if(oOverflowRegisters.m_bCoarseFFT1Overflow)
    {
        m_pUI->label_coarseFFT1OverRange->setText("True");
    }
    else
    {
        m_pUI->label_coarseFFT1OverRange->setText("False");
    }

    if(oOverflowRegisters.m_bPacketiserOverflow)
    {
        m_pUI->label_packetiserOverflow->setText("True");
    }
    else
    {
        m_pUI->label_packetiserOverflow->setText("False");
    }

    if(oOverflowRegisters.m_bQueueFull)
    {
        m_pUI->label_queueFull->setText("True");
    }
    else
    {
        m_pUI->label_queueFull->setText("False");
    }

    if(oOverflowRegisters.m_bQueueFull)
    {
        m_pUI->label_queueFull->setText("True");
    }
    else
    {
        m_pUI->label_queueFull->setText("False");
    }

    if(oOverflowRegisters.m_b10GbETxOverflow)
    {
        m_pUI->label_10GbETxOverflow->setText("True");
    }
    else
    {
        m_pUI->label_10GbETxOverflow->setText("False");
    }

    if(m_bEth10GbEUp)
    {
        m_pUI->label_10GbELinkUp->setText(QString("True"));
    }
    else
    {
        m_pUI->label_10GbELinkUp->setText(QString("False"));
    }

    m_pUI->label_ppsCount->setText(QString("%1").arg(m_u32PPSCount));

    m_pUI->label_fpgaClock->setText(QString("%1 Hz").arg(m_u32ClockFrequency_Hz));

    //Flag an parameters which are not valid
    checkParametersValid();
}

void cRoachAcquistionControlDialog::checkParametersValid()
{
    cSpectrometerOverflowRegisters oOverflowRegisters(m_u32OverflowsRegs);

    if(!oOverflowRegisters.m_bADC0OverRange)
    {
        m_pUI->label_adc0OverRange->setStyleSheet("QLabel { background-color : pallete(label)}");
    }
    else
    {
        m_pUI->label_adc0OverRange->setStyleSheet("QLabel { background-color : red}");
    }

    if(!oOverflowRegisters.m_bADC1OverRange)
    {
        m_pUI->label_adc1OverRange->setStyleSheet("QLabel { background-color : pallete(label)}");
    }
    else
    {
        m_pUI->label_adc1OverRange->setStyleSheet("QLabel { background-color : red}");
    }

    if(!oOverflowRegisters.m_bCoarseFFT0Overflow)
    {
        m_pUI->label_coarseFFT0OverRange->setStyleSheet("QLabel { background-color : pallete(label)}");
    }
    else
    {
        m_pUI->label_coarseFFT0OverRange->setStyleSheet("QLabel { background-color : red}");
    }

    if(!oOverflowRegisters.m_bCoarseFFT1Overflow)
    {
        m_pUI->label_coarseFFT1OverRange->setStyleSheet("QLabel { background-color : pallete(label)}");
    }
    else
    {
        m_pUI->label_coarseFFT1OverRange->setStyleSheet("QLabel { background-color : red}");
    }

    if(!oOverflowRegisters.m_bPacketiserOverflow)
    {
        m_pUI->label_packetiserOverflow->setStyleSheet("QLabel { background-color : pallete(label)}");
    }
    else
    {
        m_pUI->label_packetiserOverflow->setStyleSheet("QLabel { background-color : red}");
    }

    if(!oOverflowRegisters.m_bQueueFull)
    {
        m_pUI->label_queueFull->setStyleSheet("QLabel { background-color : pallete(label)}");
    }
    else
    {
        m_pUI->label_queueFull->setStyleSheet("QLabel { background-color : red}");
    }

    if(!oOverflowRegisters.m_b10GbETxOverflow)
    {
        m_pUI->label_10GbETxOverflow->setStyleSheet("QLabel { background-color : pallete(label)}");
    }
    else
    {
        m_pUI->label_10GbETxOverflow->setStyleSheet("QLabel { background-color : red}");
    }

    if(m_bEth10GbEUp)
    {
        m_pUI->label_10GbELinkUp->setStyleSheet("QLabel { background-color : pallete(label)}");
    }
    else
    {
        m_pUI->label_10GbELinkUp->setStyleSheet("QLabel { background-color : red}");
    }

    if(m_u32ClockFrequency_Hz == 200000000)
    {
        m_pUI->label_fpgaClock->setStyleSheet("QLabel { background-color : pallete(label)}");
    }
    else
    {
        m_pUI->label_fpgaClock->setStyleSheet("QLabel { background-color : red}");
    }

    //Check all variables and flag "Registers" label in GUI with red background if any fail
    bool bGlobalError = oOverflowRegisters.m_bADC0OverRange
            || oOverflowRegisters.m_bADC1OverRange
            || oOverflowRegisters.m_bCoarseFFT0Overflow
            || oOverflowRegisters.m_bCoarseFFT1Overflow
            || oOverflowRegisters.m_bPacketiserOverflow
            || oOverflowRegisters.m_bQueueFull
            || oOverflowRegisters.m_b10GbETxOverflow
            || !m_bEth10GbEUp
            || m_u32ClockFrequency_Hz != 200000000
            || !m_bPPSValid;

    if(bGlobalError)
    {
        m_pUI->label_registers->setStyleSheet("QLabel { background-color : red}");
    }
    else
    {
        m_pUI->label_registers->setStyleSheet("QLabel { background-color : pallete(label)}");
    }

}


void cRoachAcquistionControlDialog::slotRecordingInfoUpdate(const QString &qstrFilename, int64_t i64StartTime_us, int64_t i64EllapsedTime_us,
                                                            int64_t i64StopTime_us, int64_t i64TimeLeft_us,
                                                            uint64_t u64CurrentFileSize_B, uint64_t u64DiskSpaceRemaining_B)
{
    //Update info about the recording progress in the GUI

    if(i64EllapsedTime_us < 0)
    {
        m_pUI->label_status->setText(QString("Recording queued: Starting in %1").arg(AVN::stringFromTimeDuration(-i64EllapsedTime_us).c_str()));
        setWindowTitle(QString("Roach Aquisition Control - Recording queued..."));

        m_pUI->label_recordingStartTime->setText(QString(""));
        m_pUI->label_recordingDuration->setText(QString(""));
        m_pUI->label_recordingStopTime->setText(QString(""));
        m_pUI->label_recordingTimeLeft->setText(QString(""));
        m_pUI->label_currentFileSize->setText(QString(""));
    }
    else
    {
        m_pUI->label_status->setText(QString("Recording: %1").arg(qstrFilename));
        setWindowTitle(QString("Roach Aquisition Control - Recording"));

        m_pUI->label_recordingStartTime->setText(QString(AVN::stringFromTimestamp_full(i64StartTime_us).c_str()));
        m_pUI->label_recordingDuration->setText(QString(AVN::stringFromTimeDuration(i64EllapsedTime_us).c_str()));
        if(i64StopTime_us == LLONG_MAX)
        {
            m_pUI->label_recordingStopTime->setText(QString("User controlled."));
            m_pUI->label_recordingTimeLeft->setText(QString("N/A"));
        }
        else
        {
            m_pUI->label_recordingStopTime->setText(QString(AVN::stringFromTimestamp_full(i64StopTime_us).c_str()));
            m_pUI->label_recordingTimeLeft->setText(QString(AVN::stringFromTimeDuration(i64TimeLeft_us).c_str()));
        }

        m_pUI->label_currentFileSize->setText(QString("%1 MB").arg((double)u64CurrentFileSize_B / 1e6, 0, 'f', 3));
    }

    //Always update disk space
    m_pUI->label_diskSpaceRemaining->setText(QString("%1 GB").arg((double)u64DiskSpaceRemaining_B / 1e9, 0, 'f', 3));
}

void cRoachAcquistionControlDialog::slotRecordingStarted()
{
    {
        QWriteLocker oLock(&m_oMutex);
        m_bIsRecording = true;
    }

    m_pUI->pushButton_startStopRecording->setText(QString("Stop recording"));
}

void cRoachAcquistionControlDialog::slotRecordingStoppped()
{
    {
        QWriteLocker oLock(&m_oMutex);
        m_bIsRecording = false;
    }

    m_pUI->pushButton_startStopRecording->setText(QString("Start recording"));

    m_pUI->label_status->setText(QString("Not recording."));
    setWindowTitle(QString("Roach Aquisition Control"));

    m_pUI->label_recordingStartTime->setText(QString(""));
    m_pUI->label_recordingDuration->setText(QString(""));
    m_pUI->label_recordingStopTime->setText(QString(""));
    m_pUI->label_recordingTimeLeft->setText(QString(""));
}

void cRoachAcquistionControlDialog::slotTimeZoneChanged(QString qstrTimeZone)
{
    if(qstrTimeZone == QString("UTC"))
    {
        m_eTimeSpec = Qt::UTC;
        cout << "Set start time display to UTC" << endl;
    }

    if(qstrTimeZone == QString("Local"))
    {
        m_eTimeSpec = Qt::LocalTime;
        cout << "Set start time display to local time" << endl;
    }

    m_pUI->timeEdit_recordAt->setTimeSpec(m_eTimeSpec);
}

bool cRoachAcquistionControlDialog::eventFilter(QObject *pObj, QEvent *pEvent)
{
    //Intercept close event and hide the dialogue instead
    if(pEvent->type() == QEvent::Close)
    {
        pEvent->ignore();
        this->hide();
        return true;
    }

    //Otherwise process the event as normal
    return cRoachAcquistionControlDialog::eventFilter(pObj, pEvent);
}
