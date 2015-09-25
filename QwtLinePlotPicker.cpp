//System includes

//Library includes

//Local includes
#include "QwtLinePlotPicker.h"

cQwtLinePlotPicker::cQwtLinePlotPicker(int iXAxis, int iYAxis, RubberBand oRubberBand, DisplayMode oTrackerMode, QWidget *pCanvas) :
    QwtPlotPicker(iXAxis, iYAxis, oRubberBand, oTrackerMode, pCanvas)
{
}

void cQwtLinePlotPicker::setXUnit(const QString &qstrXUnit)
{
    m_qstrXUnit = qstrXUnit;
}

void cQwtLinePlotPicker::setYUnit(const QString &qstrYUnit)
{
    m_qstrYUnit = qstrYUnit;
}

//QwtText cQwtLinePlotPicker::trackerText(const QPoint &oPosition) const
//{
//    return QwtText(QString("%1 %2, %3 %4").arg(QString::number(oPosition.x())).arg(m_qstrXUnit).arg(QString::number(oPosition.y())).arg(m_qstrYUnit));
//}

QwtText cQwtLinePlotPicker::trackerTextF(const QPointF &oPosition) const
{
    return QwtText(QString("%1 %2, %3 %4").arg(QString::number(oPosition.x())).arg(m_qstrXUnit).arg(QString::number(oPosition.y())).arg(m_qstrYUnit));
}
