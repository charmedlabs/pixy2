//
// begin license header
//
// This file is part of Pixy CMUcam5 or "Pixy" for short
//
// All Pixy source code is provided under the terms of the
// GNU General Public License v2 (http://www.gnu.org/licenses/gpl-2.0.html).
// Those wishing to use Pixy source code, software and/or
// technologies under different licensing terms should contact us at
// cmucam@cs.cmu.edu. Such licensing terms are available for
// all portions of the Pixy codebase presented here.
//
// end license header
//

#ifndef LINEMODULE_H
#define LINEMODULE_H

#include <QPainter>
#include "monmodule.h"
#include "chirp.hpp"

#define LINE_EDGE_DATA_SIZE       0x2000
#define LINE_NUM_COLORS           8

// color connected components
class LineModule : public MonModule
{
public:
    LineModule(Interpreter *interpreter);
    ~LineModule();

    virtual bool render(uint32_t fourcc, const void *args[]);
    virtual bool command(const QStringList &argv);
    virtual void paramChange();

private:
    void render4014(uint8_t renderFlags, uint16_t width, uint16_t height, uint32_t frameLen, uint8_t *frame);
    void handleEDGF(uint8_t renderFlags, uint16_t width, uint16_t height);
    void handleEDGS(uint32_t len, uint16_t *data);
    void handleLISF(uint8_t renderFlags, const char *desc, uint16_t width, uint16_t height);
    void handleLISS(uint8_t renderMode, uint8_t index, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
    void handleNADF(uint8_t renderFlags, const char *desc, uint16_t width, uint16_t height);
    void handleNADS(uint32_t len, uint8_t *pointData);
    void renderLING(uint8_t renderFlags, uint16_t width, uint16_t height, uint32_t len, uint16_t *gridData);
    void renderLISG(uint8_t renderFlags, uint16_t width, uint16_t height, uint32_t len, uint8_t *data);
    void handleCODE(uint8_t renderFlags, uint16_t width, uint16_t height, uint32_t len, uint8_t *data);
    void handleBC0F(uint8_t renderFlags, const char *desc, uint16_t width, uint16_t height);
    void handleBC0S(uint8_t index, uint16_t val, uint16_t xoffset, uint16_t yoffset, uint16_t width, uint16_t height);
    void handlePVI0(uint8_t renderFlags, uint16_t width, uint16_t height, uint8_t xSrc, uint8_t ySrc, uint8_t xDest, uint8_t yDest, uint8_t intersectionDest);
    void drawLine(uint index, int x1, int y1, int x2, int y2, Qt::PenStyle style=Qt::SolidLine, const QString &text="");
    void drawPoint(uint index, int x, int y, const QString &text="");
    QString lookup(uint16_t barcodeNum);
    QColor colorLookup(uint index);

    float m_scale;
    QImage m_img;
    QPainter m_painter;
    uint m_index;

    uint32_t m_edgeLen;
    uint16_t m_edgeData[LINE_EDGE_DATA_SIZE];
    uint16_t m_dist;
    QList<QPair<uint16_t, QString> > m_labels;

    static QColor m_colors[8];
};

#endif // LINEMODULE_H
