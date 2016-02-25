#ifndef MAINWINDOW_H
#define MAINWINDOW_H

//System includes

//Library includes
#include <QMainWindow>
#include <QVector>
#include <QScopedPointer>

//Local includes
#include "NetworkConnectionWidget.h"
#include "PlotsWidget.h"
#include "RoachAcquistionControlWidget.h"

namespace Ui {
class cMainWindow;
}

class cMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit cMainWindow(QWidget *pParent = 0);
    ~cMainWindow();

private:
    Ui::cMainWindow                                 *m_pUI;
    cNetworkConnectionWidget                        *m_pNetworkGroupBox;
    cPlotsWidget                                    *m_pPlotsWidget;
    QScopedPointer<cRoachAcquistionControlWidget>   m_pAquisitionWidget;

    void                                            connectSignalsToSlots();

    void                                            closeEvent(QCloseEvent *pEvent); //Overload to close RoachAcquisitionControlDialog if it is open before closing main window

private slots:
    void                                            slotSetConnectedOrBound(bool bIsConnectedOrBound);
    void                                            slotKATCPEnabled(bool bEnabled);
    void                                            slotSetKATCPConnected(bool bIsKATCPConnected);
    void                                            slotOpenAboutDialog();
};

#endif // MAINWINDOW_H
