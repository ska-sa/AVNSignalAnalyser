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

using namespace std;

cRoachAcquistionControlDialog::cRoachAcquistionControlDialog(const QString &qstrHostname, uint16_t u16Port, QWidget *pParent) :
    QDialog(pParent),
    m_pUI(new Ui::cRoachAcquistionControlDialog),
    m_eTimeSpec(Qt::LocalTime),
    m_bIsRecording(false)
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
    m_pKATCPClient = boost::make_shared<cRoachAcquisitionServerKATCPClient>(qstrHostname.toStdString(), u16Port);
    m_pKATCPClient->registerCallbackHandler(this);

    m_oSecondTimer.start(1000);
}

void cRoachAcquistionControlDialog::connectSignalToSlots()
{
    QObject::connect( &m_oSecondTimer, SIGNAL(timeout()), this, SLOT(slotSecondTimerTrigger()) );
    QObject::connect( m_pUI->pushButton_startStopRecording, SIGNAL(pressed()), this, SLOT(slotStartStopRecordingClicked()) );
    QObject::connect( m_pUI->comboBox_timeZone, SIGNAL(currentIndexChanged(QString)), this, SLOT(slotTimeZoneChanged(QString)) );

    //Select last editted option
    QObject::connect( m_pUI->timeEdit_recordAt, SIGNAL(editingFinished()), m_pUI->radioButton_recordAt, SLOT(click()) );
    QObject::connect( m_pUI->doubleSpinBox_recordIn, SIGNAL(editingFinished()), m_pUI->radioButton_recordIn, SLOT(click()) );
    QObject::connect( m_pUI->doubleSpinBox_recordFor, SIGNAL(editingFinished()), m_pUI->radioButton_recordFor, SLOT(click()) );


    //Call backs that alter the GUI decoupled by Queued connections to be executed by the GUI thread
    qRegisterMetaType<int64_t>("int64_t");
    qRegisterMetaType<uint64_t>("uint64_t");
    QObject::connect( this, SIGNAL(sigRecordingInfoUpdate(QString,int64_t,int64_t,int64_t,int64_t,uint64_t)),
                      this, SLOT(slotRecordingInfoUpdate(QString,int64_t,int64_t,int64_t,int64_t,uint64_t)), Qt::QueuedConnection);
    QObject::connect( this, SIGNAL(sigRecordingStarted()), this, SLOT(slotRecordingStarted()), Qt::QueuedConnection);
    QObject::connect( this, SIGNAL(sigRecordingStoppped()), this, SLOT(slotRecordingStoppped()), Qt::QueuedConnection);

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
                                                                 int64_t i64StopTime_us, int64_t i64TimeLeft_us, uint64_t u64DiskSpaceRemaining_B)
{
    //Send this info to private slot via queued connection to change GUI. Needs to be a queued connection for execution from the
    //main (GUI) thread. You can't alter the GUI from arbitary threads.

    sigRecordingInfoUpdate(QString(strFilename.c_str()), i64StartTime_us, i64EllapsedTime_us, i64StopTime_us, i64TimeLeft_us, u64DiskSpaceRemaining_B);
}

void cRoachAcquistionControlDialog::slotRecordingInfoUpdate(const QString &qstrFilename, int64_t i64StartTime_us, int64_t i64EllapsedTime_us,
                                                            int64_t i64StopTime_us, int64_t i64TimeLeft_us, uint64_t u64DiskSpaceRemaining_B)
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
    }

    //Always update disk space
    m_pUI->label_diskSpaceRemaining->setText(QString("%1 GB").arg((double)u64DiskSpaceRemaining_B / 1e9, 0, 'f'));
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
    //Intercept close events and hide instead
    if(pEvent->type() == QEvent::Close)
    {
        pEvent->ignore();
        this->hide();
        return true;
    }

//    if(pEvent->type() == QEvent::KeyPress)
//    {
//        if(pEvent->KeyPress == QKeyEvent)
//        pEvent->ignore();
//        this->hide();
//        return true;
//    }

    //Otherwise process the event as normal
    return cRoachAcquistionControlDialog::eventFilter(pObj, pEvent);
}
