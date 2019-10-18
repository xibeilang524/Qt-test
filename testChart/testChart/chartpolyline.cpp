#include "chartpolyline.h"

ChartPolyLine::ChartPolyLine()
{


}

void ChartPolyLine::slot_timeout_chartAnnimation()
{
    int bufflen = buff.count();
    int testlen = testData.count();

    if(bufflen < testlen)
        buff.append(testData.at(bufflen));
    else
        chartAnimationTimer.stop();

    update();
}

void ChartPolyLine::startChartAnimation(int ms)
{

    buff.clear();

    chartAnimationTimer.start(ms/testData.count());
}

void ChartPolyLine::drawChart(QPainter *painter)
{

    for(int i=1;i<buff.size();i++){


        QPointF prePtf = realCoordinateTransformation(buff.at(i-1).mPt);
        QPointF curPtf = realCoordinateTransformation(buff.at(i).mPt);

        painter->setPen(QPen(QBrush(buff.at(i).mColor),2));

        painter->drawLine(prePtf,curPtf);

    }

}