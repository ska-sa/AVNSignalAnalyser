#ifndef ROACH_ACQUISITION_CONTROL_WIDGET_H
#define ROACH_ACQUISITION_CONTROL_WIDGET_H

//System includes

//Libary includes
#include <QGroupBox>
#include <QTimer>

//Local includes

namespace Ui {
class cRoachAcquistionControlWidget;
}

class cRoachAcquistionControlWidget : public QGroupBox
{
    Q_OBJECT

public:
    explicit cRoachAcquistionControlWidget(QWidget *pParent = 0);
    ~cRoachAcquistionControlWidget();

private:
    Ui::cRoachAcquistionControlWidget   *m_pUI;

    QTimer                              m_oSecondTimer;

    bool                                m_bIsRecording;

    void                                connectSignalToSlots();

private slots:
    void                                slotSecondTimerTrigger();
    void                                slotStartStopRecordingClicked();
};

#endif // ROACH_ACQUISITION_CONTROL_WIDGET_H
