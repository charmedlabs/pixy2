#include <QDebug>
#include <QPainter>
#include <stdio.h>
#include <math.h>
#include "linemodule.h"
#include "equeue.h"
#include "interpreter.h"
#include "renderer.h"

#define MIN(a, b)  ((a)<(b) ? (a) : (b))

// declare module
MON_MODULE(LineModule);

QColor LineModule::m_colors[8] =
{
    QColor(0xff, 0x00, 0x00, 0xff), // red
    QColor(0xff, 0x80, 0x00, 0xff), // orange
    QColor(0xff, 0xff, 0x00, 0xff), // yellow
    QColor(0x00, 0xff, 0x80, 0xff), // lt green
    QColor(0x00, 0xff, 0x00, 0xff), // green
    QColor(0x00, 0xff, 0xff, 0xff), // lt blue
    QColor(0x00, 0x00, 0xff, 0xff), // blue
    QColor(0xff, 0x00, 0xff, 0xff)  // purple
};

LineModule::LineModule(Interpreter *interpreter) : MonModule(interpreter)
{
}

LineModule::~LineModule()
{
}

bool LineModule::render(uint32_t fourcc, const void *args[])
{
    if (fourcc==FOURCC('4', '0', '1', '4'))
    {
        render4014(*(uint8_t *)args[0], *(uint16_t *)args[1], *(uint16_t *)args[2], *(uint32_t *)args[3], (uint8_t *)args[4]);
        return true;
    }
    else if (fourcc==FOURCC('E', 'D', 'G', 'F'))
    {
        handleEDGF(*(uint8_t *)args[0], *(uint16_t *)args[1], *(uint16_t *)args[2]);
        return true;
    }
    else if (fourcc==FOURCC('E', 'D', 'G', 'S'))
    {
        handleEDGS(*(uint32_t *)args[0], (uint16_t *)args[1]);
        return true;
    }
    else if (fourcc==FOURCC('L', 'I', 'S', 'F'))
    {
        handleLISF(*(uint8_t *)args[0], (const char *)args[1], *(uint16_t *)args[2], *(uint16_t *)args[3]);
        return true;
    }
    else if (fourcc==FOURCC('L', 'I', 'S', 'S'))
    {
        handleLISS(*(uint8_t *)args[0], *(uint8_t *)args[1], *(uint16_t *)args[2], *(uint16_t *)args[3], *(uint16_t *)args[4], *(uint16_t *)args[5]);
        return true;
    }
    else if (fourcc==FOURCC('N', 'A', 'D', 'F'))
    {
        handleNADF(*(uint8_t *)args[0], (const char *)args[1], *(uint16_t *)args[2], *(uint16_t *)args[3]);
        return true;
    }
    else if (fourcc==FOURCC('N', 'A', 'D', 'S'))
    {
        handleNADS(*(uint32_t *)args[0], (uint8_t *)args[1]);
        return true;
    }
    else if (fourcc==FOURCC('L', 'I', 'N', 'G'))
    {
        renderLING(*(uint8_t *)args[0], *(uint16_t *)args[1], *(uint16_t *)args[2], *(uint32_t *)args[3], (uint16_t *)args[4]);
        return true;
    }
    else if (fourcc==FOURCC('L', 'I', 'S', 'G'))
    {
        renderLISG(*(uint8_t *)args[0], *(uint16_t *)args[1], *(uint16_t *)args[2], *(uint32_t *)args[3], (uint8_t *)args[4]);
        return true;
    }
    else if (fourcc==FOURCC('C', 'O', 'D', 'E'))
    {
        handleCODE(*(uint8_t *)args[0], *(uint16_t *)args[1], *(uint16_t *)args[2], *(uint32_t *)args[3], (uint8_t *)args[4]);
        return true;
    }
    else if (fourcc==FOURCC('B', 'C', '0', 'F'))
    {
        handleBC0F(*(uint8_t *)args[0], (const char *)args[1], *(uint16_t *)args[2], *(uint16_t *)args[3]);
        return true;
    }
    else if (fourcc==FOURCC('B', 'C', '0', 'S'))
    {
        handleBC0S(*(uint8_t *)args[0], *(uint16_t *)args[1], *(uint16_t *)args[2], *(uint16_t *)args[3], *(uint16_t *)args[4], *(uint16_t *)args[5]);
        return true;
    }
    else if (fourcc==FOURCC('P', 'V', 'I', '0'))
    {
        handlePVI0(*(uint8_t *)args[0], *(uint16_t *)args[1], *(uint16_t *)args[2], *(uint8_t *)args[3], *(uint8_t *)args[4], *(uint8_t *)args[5], *(uint8_t *)args[6], *(uint8_t *)args[7]);
        return true;
    }
    return false;
}


void LineModule::handleEDGF(uint8_t renderFlags, uint16_t width, uint16_t height)
{
    uint16_t x;

    if (renderFlags==RENDER_FLAG_START)
        m_edgeLen = 0;
    else // render
    {
        uint32_t i, y;

        QImage imgh(width, height, QImage::Format_ARGB32);
        QImage imgv(width/4, height, QImage::Format_ARGB32);
        imgh.fill(0x00000000);
        imgv.fill(0x00000000);

        for (y=0, i=0; y<height; y++)
        {
            // render horizontal scan edges (vertical edges)
            if (m_edgeData[i++]!=EQ_HSCAN_LINE_START)
            {
                qDebug("horiz edge error");
                return;
            }
            for (; m_edgeData[i]<EQ_HSCAN_LINE_START; i++)
            {
                if (m_edgeData[i]&EQ_NEGATIVE)
                {
                    x = (m_edgeData[i]&~EQ_NEGATIVE)+m_dist;
                    if (x<width)
                        imgh.setPixel(x, y, 0xc000ff00);
                }
                else
                {
                    x = m_edgeData[i]+m_dist;
                    if (x<width)
                        imgh.setPixel(x, y, 0xc0ffff00);
                }
            }

            // render vertical scan edges (horizontal edges)
            if (m_edgeData[i]==EQ_VSCAN_LINE_START)
            {
                i++; // skip VSCAN code
                for (; m_edgeData[i]<EQ_HSCAN_LINE_START; i++)
                {
                    if (m_edgeData[i]&EQ_NEGATIVE)
                        imgv.setPixel((m_edgeData[i]&~EQ_NEGATIVE)>>2, y, 0xc0ff8000);
                    else
                        imgv.setPixel(m_edgeData[i]>>2, y, 0xc0ff00ff);
                }
            }
        }

        m_renderer->emitImage(imgh, renderFlags&~RENDER_FLAG_FLUSH, "V edges");
        m_renderer->emitImage(imgv, renderFlags, "H edges");
    }
}

void LineModule::handleEDGS(uint32_t len, uint16_t *data)
{
    uint i;

    // array not big enough
    if (m_edgeLen+len>LINE_EDGE_DATA_SIZE)
        return;

    // copy data into array
    for (i=0; i<len; i++, m_edgeLen++)
        m_edgeData[m_edgeLen] = data[i];
}

void LineModule::handleLISF(uint8_t renderFlags, const char *desc, uint16_t width, uint16_t height)
{
    if (renderFlags&RENDER_FLAG_START)
    {
        m_scale = (float)m_renderer->m_video->activeWidth()/width;
        m_img = QImage(width*m_scale, height*m_scale, QImage::Format_ARGB32);
        m_img.fill(0x00000000);
        m_painter.begin(&m_img);
    }
    else if (m_painter.isActive())
    {
        m_painter.end();
        m_renderer->emitImage(m_img, renderFlags, QString(desc));
    }
}

void LineModule::handleLISS(uint8_t renderMode, uint8_t index, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    if (!m_painter.isActive())
        return;

    Qt::PenStyle style;

    if (renderMode==1)
        style = Qt::DashLine;
    else if (renderMode==2)
        style = Qt::DotLine;
    else // if (renderMode==0)
        style = Qt::SolidLine;
    Renderer::drawLine(&m_painter, m_colors[index*(LINE_NUM_COLORS/2-1)%LINE_NUM_COLORS], x0*m_scale+0.5, y0*m_scale+0.5, x1*m_scale+0.5, y1*m_scale+0.5, 3, style);
}


void LineModule::handleNADF(uint8_t renderFlags, const char *desc, uint16_t width, uint16_t height)
{
    if (renderFlags&RENDER_FLAG_START)
    {
        m_index = 0;
        m_scale = (float)m_renderer->m_video->activeWidth()/width;
        m_img = QImage(width*m_scale, height*m_scale, QImage::Format_ARGB32);
        m_img.fill(0x00000000);
        m_painter.begin(&m_img);
    }
    else if (m_painter.isActive())
    {

        m_painter.end();
        m_renderer->emitImage(m_img, renderFlags, QString(desc));
    }
}



void LineModule::handleNADS(uint32_t len, uint8_t *pointData)
{
    if (!m_painter.isActive())
        return;

    uint32_t i;
    QColor color;
    Point *points = (Point *)pointData;
    int x=points[0].m_x*m_scale+0.5;
    int y=points[0].m_y*m_scale+0.5;
    color = m_colors[m_index*(LINE_NUM_COLORS/2-1)%LINE_NUM_COLORS];
    drawPoint(m_index, x, y);
    m_index++;
    len >>= 1;

    m_painter.setBrush(QBrush(color));
    for (i=1; i<len; i++)
        m_painter.drawEllipse(QPoint(points[i].m_x*m_scale+0.5, points[i].m_y*m_scale+0.5), 3, 3);
}


void LineModule::renderLISG(uint8_t renderFlags, uint16_t width, uint16_t height, uint32_t len, uint8_t *data)
{
    uint32_t i;
    LineSeg *segs = (LineSeg *)data;
    float scale;

    len /= sizeof(LineSeg);

    scale = (float)m_renderer->m_video->activeWidth()/width;

    QImage img(width*scale, height*scale, QImage::Format_ARGB32);
    img.fill(0x00000000);
    if (!m_painter.begin(&img))
        return;

    for (i=0; i<len; i++)
        Renderer::drawLine(&m_painter, colorLookup(segs[i].m_line), segs[i].m_p0.m_x*scale, segs[i].m_p0.m_y*scale, segs[i].m_p1.m_x*scale, segs[i].m_p1.m_y*scale);

    m_painter.end();
    m_renderer->emitImage(img, renderFlags, "Line segments");
}

void  LineModule::renderLING(uint8_t renderFlags, uint16_t width, uint16_t height, uint32_t len, uint16_t *gridData)
{
    uint16_t i;
    uint16_t entry;

    QImage imgh(width, height, QImage::Format_ARGB32);
    QImage imgv(width, height, QImage::Format_ARGB32);
    imgh.fill(0x00000000);
    imgv.fill(0x00000000);

    for (i=0; i<width*height; i++)
    {
        entry = gridData[i];
        if (entry&EQ_NODE_FLAG_HLINE)
            imgh.setPixel(i%width, i/width, 0xffff0000);
        if (entry&EQ_NODE_FLAG_VLINE)
            imgv.setPixel(i%width, i/width, 0xff0000ff);
    }
    m_renderer->emitImage(imgh, renderFlags&~RENDER_FLAG_FLUSH, "V line points");
    m_renderer->emitImage(imgv, renderFlags, "H line points");
}

void LineModule::handleCODE(uint8_t renderFlags, uint16_t width, uint16_t height, uint32_t len, uint8_t *data)
{
    uint8_t i;
    uint16_t x, y, w, h;
    QPainter p;
    QString str;
    float scale;
    RectA *rect;
    int16_t *val;

    scale = (float)m_renderer->m_video->activeWidth()/width;

    QImage img(m_renderer->m_video->activeWidth(), height*scale*4, QImage::Format_ARGB32);
    img.fill(0x00000000);

    len /= sizeof(RectA) + 8;
    if (!p.begin(&img))
        return;

    for (i=0; i<len; i++)
    {
        rect = (RectA *)data;
        data += sizeof(RectA);
        val = (int16_t *)data;
        data += 8;
        str = QString::number(*val);
        x = rect->m_xOffset*scale;
        y = rect->m_yOffset*scale*4;
        w = rect->m_width*scale;
        h = rect->m_height*scale*4;
        Renderer::drawRect(&p, QRect(x, y, w, h));
        Renderer::drawText(&p, x+w/2, y+h/2, str);
    }
    p.end();

    m_renderer->emitImage(img, renderFlags, "Codes");
}

void LineModule::handleBC0F(uint8_t renderFlags, const char *desc, uint16_t width, uint16_t height)
{
    if (renderFlags&RENDER_FLAG_START)
    {
        m_index = 0;
        m_scale = (float)m_renderer->m_video->activeWidth()/width;
        m_img = QImage(width*m_scale, height*m_scale*4, QImage::Format_ARGB32);
        m_img.fill(0x00000000);
        m_painter.begin(&m_img);
        m_painter.setPen(QPen(QColor(0xff, 0xff, 0xff, 0xff)));
    }
    else if (m_painter.isActive())
    {
        m_painter.end();
        m_renderer->emitImage(m_img, renderFlags, QString(desc));
    }
}

void LineModule::handleBC0S(uint8_t index, uint16_t val, uint16_t xoffset, uint16_t yoffset, uint16_t width, uint16_t height)
{
    if (!m_painter.isActive())
        return;

    uint16_t x, y, w, h;
    QString str;
    QColor color;

    if ((str=lookup(val))=="")
        str = QString::number(val);
    x = xoffset*m_scale;
    y = yoffset*m_scale*4;
    color = colorLookup(index);

    x = xoffset*m_scale;
    y = yoffset*m_scale*4;
    w = width*m_scale;
    h = height*m_scale*4;
    Renderer::drawRect(&m_painter, QRect(x, y, w, h), color, 0x40);
    Renderer::drawText(&m_painter, x+w/2, y+h/2, str);
}

#define D0 12.0
#define D1 12.0
#define D2 24.0
#define D3 4.0

void LineModule::handlePVI0(uint8_t renderFlags, uint16_t width, uint16_t height, uint8_t xSrc, uint8_t ySrc, uint8_t xDest, uint8_t yDest, uint8_t intersectionDest)
{
    QString label;
    QPointF points[5];
    QPointF pSrc, pDest;
    float angle, ca, sa;

    m_scale = (float)m_renderer->m_video->activeWidth()/width;
    m_img = QImage(width*m_scale, height*m_scale, QImage::Format_ARGB32);
    m_img.fill(0x00000000);
    if (!m_painter.begin(&m_img))
        return;

    if (xSrc!=0 || ySrc!=0 || xDest!=0 || yDest!=0)
    {
        pSrc = QPointF(xSrc*m_scale+0.5, ySrc*m_scale+0.5);
        pDest = QPointF(xDest*m_scale+0.5, yDest*m_scale+0.5);
        Renderer::drawLine(&m_painter, colorLookup(0), pSrc.x(), pSrc.y(), pDest.x(), pDest.y());
        // draw arrowhead for vector so it points toward the destination
        angle = atan2(pDest.y()-pSrc.y(),pDest.x()-pSrc.x());
        ca = cos(angle);
        sa = sin(angle);
        points[0].rx() = pDest.x() + ca*D3;
        points[0].ry() = pDest.y() + sa*D3;
        points[2].rx() = pDest.x() - ca*D0;
        points[2].ry() = pDest.y() - sa*D0;

        points[4].rx() = pDest.x() - ca*D2;
        points[4].ry() = pDest.y() - sa*D2;

        points[1].rx() = points[4].x()-sa*D1;
        points[1].ry() = points[4].y()+ca*D1;
        points[3].rx() = points[4].x()+sa*D1;
        points[3].ry() = points[4].y()-ca*D1;

        m_painter.setBrush(QBrush(m_colors[0]));
        m_painter.setPen(QPen(m_colors[0]));
        m_painter.drawPolygon(points, 4);

        Renderer::drawText(&m_painter, abs(pDest.x()+pSrc.x())/2, abs(pDest.y()+pSrc.y())/2, "Vector");

        if (intersectionDest)
        {
            label.sprintf("%d-way intersection", intersectionDest);
            drawPoint(0, pDest.x(), pDest.y(), label);
        }
    }
    m_painter.end();
    m_renderer->emitImage(m_img, renderFlags, "Primary features");
}

bool LineModule::command(const QStringList &argv)
{
    return false;
}

void LineModule::paramChange()
{
    QVariant val;
    int i;

    if (pixyParameterChanged("Edge distance", &val))
        m_dist = val.toUInt();

    // create label dictionary
    Parameters &params = m_interpreter->m_pixyParameters.parameters();
    QStringList words;
    uint32_t barcodeNum;
    m_labels.clear();
    // go through all parameters and find labels
    for (i=0; i<params.length(); i++)
    {
        if (params[i].id().startsWith("Barcode label"))
        {
            words = params[i].id().split(QRegExp("\\s+"));
            if (words.length()<3) // bogus!
                continue;
            barcodeNum = words[2].toUInt();
            m_labels.push_back(QPair<uint16_t, QString>(barcodeNum, params[i].value().toString().remove(QRegExp("^\\s+")))); // remove leading whitespace
        }
    }
}

void LineModule::drawPoint(uint index, int x, int y, const QString &text)
{
    QColor color;
    color = m_colors[index*(LINE_NUM_COLORS/2-1)%LINE_NUM_COLORS];

    m_painter.setBrush(QBrush(QColor(0, 0, 0, 0xff)));
    m_painter.setPen(QPen(QColor(0, 0, 0, 0xff), 1));
    m_painter.drawEllipse(QPoint(x, y), 8, 8);

    m_painter.setBrush(QBrush(QColor(0xff, 0xff, 0xff, 0xff)));
    m_painter.setPen(QPen(color, 3));
    m_painter.drawEllipse(QPoint(x, y), 7, 7);

    if (text!="")
        Renderer::drawText(&m_painter, x, y-TEXT_HEIGHT-4, text);
}


QString LineModule::lookup(uint16_t barcodeNum)
{
    int i;

    for (i=0; i<m_labels.length(); i++)
    {
        if (m_labels[i].first==barcodeNum)
            return m_labels[i].second;
    }
    return "";
}

QColor LineModule::colorLookup(uint index)
{
    return m_colors[index*(LINE_NUM_COLORS/2-1)%LINE_NUM_COLORS];
}

void LineModule::render4014(uint8_t renderFlags, uint16_t width, uint16_t height, uint32_t frameLen, uint8_t *frame)
{
    uint32_t i;
    uint32_t *line;
    uint16_t y, x;

    QImage img(width, height, QImage::Format_ARGB32);

    for (y=0, i=0; y<height; y++)
    {
        line = (unsigned int *)img.scanLine(y);
        for (x=0; x<width; x++, i++)
            *line++ = (0xff<<24) | (frame[i]<<16) | (frame[i]<<8) | (frame[i]<<0);

    }

    QImage img2 = img.scaled(width, height*4);
    m_renderer->emitImage(img2, renderFlags, "Background");
}

