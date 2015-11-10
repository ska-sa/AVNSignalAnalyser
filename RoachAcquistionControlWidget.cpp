#include "RoachAcquistionControlWidget.h"
#include "ui_RoachAcquistionControlWidget.h"

cRoachAcquistionControlWidget::cRoachAcquistionControlWidget(QWidget *pParent) :
    QGroupBox(pParent),
    m_pUI(new Ui::cRoachAcquistionControlWidget),
    m_bIsRecording(false)
{
    m_pUI->setupUi(this);

    connectSignalToSlots();

    m_oSecondTimer.start(1000);
}

cRoachAcquistionControlWidget::~cRoachAcquistionControlWidget()
{
    delete m_pUI;
}

void cRoachAcquistionControlWidget::connectSignalToSlots()
{
    QObject::connect( &m_oSecondTimer, SIGNAL(timeout()), this, SLOT(slotSecondTimerTrigger()) );
    QObject::connect( m_pUI->pushButton_startStopRecording, SIGNAL(pressed()), this, SLOT(slotStartStopRecordingClicked()) );
}

void cRoachAcquistionControlWidget::slotSecondTimerTrigger()
{
    //Update the start time in the GUI every second to the current time.
    m_pUI->timeEdit_recordAt->setTime(QTime::currentTime().addSecs(1)); //Now plus 1 second

    //Update the recording info
    if(m_bIsRecording)
    {
        m_pUI->label_recordingDuration->setText(QString());
        m_pUI->label_recordingStartTime->setText(QString());
        m_pUI->label_recordingStopTime->setText(QString());
        m_pUI->label_recordingTimeLeft->setText(QString());
    }
}

void cRoachAcquistionControlWidget::slotStartStopRecordingClicked()
{
    if(m_bIsRecording)
    {
        m_pUI->pushButton_startStopRecording->setText(QString("Stop recording"));
    }
    else
    {
        m_pUI->pushButton_startStopRecording->setText(QString("Start recording"));
    }
}
