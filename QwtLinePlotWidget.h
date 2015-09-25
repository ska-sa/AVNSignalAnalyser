#ifndef QWT_LINE_PLOT_WIDGET_H
#define QWT_LINE_PLOT_WIDGET_H

//System includes
#ifdef _WIN32
#include <stdint.h>

#ifndef int64_t
typedef __int64 int64_t;
#endif

#ifndef uint64_t
typedef unsigned __int64 uint64_t;
#endif

#else
#include <inttypes.h>
#endif

//Library includes
#include <QWidget>
#include <QString>
#include <QVector>
#include <QReadWriteLock>
#include <qwt_plot_grid.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_magnifier.h>

//Local includes
#include "QwtLinePlotPicker.h"

namespace Ui {
class cQwtLinePlotWidget;
}

class cQwtLinePlotWidget : public QWidget
{
    Q_OBJECT

public:
    explicit cQwtLinePlotWidget(QWidget *pParent = 0);
    ~cQwtLinePlotWidget();

    void                                addData(const QVector<QVector<float> > &qvvfData, int64_t i64Timestamp_us = 0);

    void                                setXLabel(const QString &qstrXLabel);
    void                                setXUnit(const QString &qstrXUnit);
    void                                setYLabel(const QString &qstrYLabel);
    void                                setYUnit(const QString &qstrYUnit);
    void                                setTitle(const QString &qstrTitle);

    void                                enablePlotGrid(bool bEnable);

    void                                enableAutoscaleControl(bool bEnable);
    void                                enableAveragingControl(bool bEnable);
    void                                enablePauseControl(bool bEnable);

    void                                enableLogConversion(bool bEnable);
    void                                enablePowerLogConversion(bool bEnable);

    void                                enableRejectData(bool bEnable);

    void                                enableTimestampInTitle(bool bEnable);

    void                                setXSpan(double dXBegin, double dXEnd);

    QVector<Qt::GlobalColor>            m_qveCurveColours;

protected:
    Ui::cQwtLinePlotWidget              *m_pUI;

    QVector<QwtPlotCurve*>              m_qvpPlotCurves;

    //Qwt Plot extensions
    QwtPlotGrid                         m_oPlotGrid;
    QwtPlotZoomer*                      m_pPlotZoomer;
    QwtPlotPanner*                      m_pPlotPanner;
    QwtPlotMagnifier*                   m_pPlotMagnifier;
    cQwtLinePlotPicker*                 m_pPlotPicker;

    //Data stuctures
    QVector<QVector<QVector<float> >  > m_qvvvfAverageHistory;
    uint32_t                            m_u32NextHistoryInputIndex;
    QVector<QVector<double> >           m_qvvdYDataToPlot;
    QVector<double>                     m_qvdXDataToPlot;

    //Span of X scale
    double                              m_dXBegin;
    double                              m_dXEnd;
    bool                                m_bXSpanChanged;

    //Plot settings
    QString                             m_qstrTitle;
    QString                             m_qstrXLabel;
    QString                             m_qstrXUnit;
    QString                             m_qstrYLabel;
    QString                             m_qstrYUnit;

    bool                                m_bIsGridEnabled;

    //Controls
    bool                                m_bIsPaused;
    bool                                m_bIsAutoscaleEnabled;
    uint32_t                            m_u32Averaging;

    bool                                m_bTimestampInTitleEnabled;

    bool                                m_bDoLogConversion;
    bool                                m_bDoPowerLogConversion;

    bool                                m_bRejectData;

    QReadWriteLock                      m_oMutex;

    void                                connectSignalsToSlots();

public slots:
    void                                slotSetAverage(int iAveraging);
    void                                slotPauseResume();
    void                                slotPause(bool bPause);
    void                                slotEnableAutoscale(bool bEnable);

private slots:
    void                                slotUpdatePlotData(unsigned int uiCurveNo, QVector<double> qvdXData, QVector<double> qvdYData, int64_t i64Timestamp_us);
    void                                slotUpdateScalesAndLabels();
    void                                slotUpdateXScaleBase(int iBase);

signals:
    void                                sigUpdatePlotData(unsigned int uiCurveNo, QVector<double> qvdXData, QVector<double> qvdYData, int64_t i64Timestamp_us);
    void                                sigUpdateScalesAndLabels();
    void                                sigUpdateXScaleBase(int iBase);

};

#endif // QWT_LINE_PLOT_WIDGET_H
