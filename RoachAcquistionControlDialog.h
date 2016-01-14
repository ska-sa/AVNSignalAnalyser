#ifndef ROACH_ACQUISITION_CONTROL_DIALOG_H
#define ROACH_ACQUISITION_CONTROL_DIALOG_H

//System includes

//Libary includes
#include <QDialog>
#include <QTimer>
#include <QReadWriteLock>

#ifndef Q_MOC_RUN //Qt's MOC and Boost have some issues don't let MOC process boost headers
#include <boost/shared_ptr.hpp>
#endif

//Local includes
#include "KATCPClient.h"

namespace Ui {
class cRoachAcquistionControlDialog;
}

class cRoachAcquistionControlDialog : public QDialog, public cKATCPClient::cCallbackInterface
{
    Q_OBJECT

public:
    explicit cRoachAcquistionControlDialog(const QString &qstrHostname, uint16_t u16Port, QWidget *pParent = 0);
    explicit cRoachAcquistionControlDialog(QWidget *pParent = 0);
    ~cRoachAcquistionControlDialog();

    void connect(const QString &qstrHostname, uint16_t u16Port);

private:
    Ui::cRoachAcquistionControlDialog   *m_pUI;

    QTimer                              m_oSecondTimer;

    Qt::TimeSpec                        m_eTimeSpec;

    bool                                m_bIsRecording;

    boost::shared_ptr<cKATCPClient>     m_pKATCPClient;

    QReadWriteLock                      m_oMutex;

    void                                connectSignalToSlots();

    //Implemented callbacks from cKATCPClient::cCallbackInterface
    void                                connected_callback(bool bConnected);
    void                                recordingStarted_callback();
    void                                recordingStopped_callback();
    void                                recordingInfoUpdate_callback(const std::string &strFilename,
                                                                     int64_t i64StartTime_us, int64_t i64EllapsedTime_us, int64_t i64StopTime_us, int64_t i64TimeLeft_us);

    bool                                eventFilter(QObject *pObj, QEvent *pEvent); //Overload to hide instead of close

private slots:
    void                                slotSecondTimerTrigger();
    void                                slotStartStopRecordingClicked();
    void                                slotRecordingInfoUpdate(const QString &qstrFilename,
                                                                int64_t i64StartTime_us, int64_t i64EllapsedTime_us, int64_t i64StopTime_us, int64_t i64TimeLeft_us);
    void                                slotRecordingStarted();
    void                                slotRecordingStoppped();
    void                                slotTimeZoneChanged(QString qstrTimeZone);

signals:
    void                                sigKATCPSocketConnected(bool bConnected);
    void                                sigRecordingInfoUpdate(const QString &qstrFilename,
                                                                int64_t i64StartTime_us, int64_t i64EllapsedTime_us, int64_t i64StopTime_us, int64_t i64TimeLeft_us);
    void                                sigRecordingStarted();
    void                                sigRecordingStoppped();

};

#endif // ROACH_ACQUISITION_CONTROL_DIALOG_H
