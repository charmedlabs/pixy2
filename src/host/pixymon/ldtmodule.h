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
#ifndef LDTMODULE_H
#define LDTMODULE_H

#include "monmodule.h"
#include "qqueue.h"
#include "blobs.h"
#include "pixytypes.h"
#include <list>

#define LDT_EDATA_SIZE 0x10000
#define LDT_LDATA_SIZE 0x1000
#define LDT_EOL        0x4000
#define LDT_COL_MASK   0x01ff
#define LDT_NULL_FLAG  0x1000
#define LDT_EXPENSIVE  0x7fffffff

#define LDT_PQUEUE_SIZE   20
#define LDT_PQUEUE_ADIST  5

#define LDT_MMC_BITS       4
#define LDT_MMC_MIN_EDGES  (2+LDT_MMC_BITS)
#define LDT_MMC_MAX_EDGES  (2+(LDT_MMC_BITS*2))

#define LDT_MMC_CANDIDATE_BARCODES   32
#define LDT_MMC_VOTED_BARCODES  8
#define LDT_MMC_VTSIZE   8  // voting table size

struct BarCodeCluster
{
    BarCodeCluster()
    {
        m_n = 0;
    }

    void addCode(uint8_t index)
    {
        if (m_n>=LDT_MMC_CANDIDATE_BARCODES)
            return;
        m_indexes[m_n] = index;
        m_n++;
    }

    void updateWidth(uint16_t width)
    {
        m_width = (m_width*(m_n-1) + width)/m_n; // recursive averager
    }

    uint8_t m_indexes[LDT_MMC_CANDIDATE_BARCODES];
    uint8_t m_n;
    Point16 m_p0;
    Point16 m_p1;
    uint16_t m_width;
};

struct BarCode
{
    BarCode()
    {
        m_n = 0;
    }

    Point16 m_p0;
    uint16_t m_width;
    int16_t m_val;
    uint16_t m_edges[LDT_MMC_MAX_EDGES-1];
    uint8_t m_n;
};

struct DecodedBarCode
{
    RectA m_outline;
    int16_t m_val;
};

struct PQueue
{
    PQueue(uint8_t size, uint8_t adist);
    ~PQueue();

    void reset();
    void add(const Point16 &p);
    Point16 *get(uint8_t index);
    uint8_t get(int8_t index, int8_t offset);
    int32_t tda1000();
    bool adetect(int32_t athresh, Point16 *p0, Point16 *pv, Point16 *p1);
    void clear(uint8_t v);

    Point16 *m_points;
    uint8_t m_size;
    uint8_t m_count;
    uint8_t m_index;
    uint8_t m_tindex;
    uint8_t m_lockout;
    uint8_t m_adist;
    uint8_t m_acount;
};

struct TwoLineList;

struct TwoLine
{
    TwoLine();
    TwoLine(const Point16 &p);
    TwoLine(const Point16 &p0, const Point16 &p1);
    ~TwoLine();
    void reset();

    TwoLine *addLine(const TwoLine &line);
    TwoLine *addLine(const Point16 &point);
    uint32_t lines();
    uint32_t length2();

    Point16 m_p0;
    Point16 m_p1;
    Point16 m_phalf;
    TwoLineList *m_lines;
};

struct TwoLineListNode;

struct TwoLineListNode
{
    TwoLineListNode()
    {
        m_val = 0;
        m_bval = false;
        m_bval2 = false;
        m_next = NULL;
    }

    TwoLine m_line;
    int32_t m_val;
    bool m_bval;
    bool m_bval2;
    Point16 m_p;
    TwoLineListNode *m_next;
};

struct TwoLineList
{
    TwoLineList();
    ~TwoLineList();
    void clear();
    TwoLine *add(const TwoLine &line);
    TwoLine *add(const TwoLine &line, int32_t val, const Point16 &p);
    bool remove(TwoLineListNode *node);
    void merge(TwoLineList *list);

    TwoLineListNode *m_first;
    TwoLineListNode *m_last;
    uint16_t m_size;
};

int32_t tanDiffAbs1000(const TwoLine &l0, const TwoLine &l1);

// line detect and track
class LdtModule : public MonModule
{
public:
    LdtModule(Interpreter *interpreter);
    ~LdtModule();

    virtual bool render(uint32_t fourcc, const void *args[]);
    virtual bool command(const QStringList &argv);
    virtual void paramChange();

private:

    void renderEX01(uint8_t renderFlags, uint16_t width, uint16_t height, uint32_t frameLen, uint8_t *frame);
    void hScan(uint8_t *line, uint16_t width);
    void vScan(uint8_t *line, uint16_t width);
    void extractLines();
    void processLines();
    void processLines(TwoLine *line, const Point16 &pRef);
    void processLine(TwoLine *line, const Point16 &pRef, bool angleDetect);
    void cleanupLine(TwoLine *line);
    void cleanupLine2(TwoLine *line);

    void detectCodes();
    bool detectCode(uint16_t *edges, bool begin, BarCode *bc);
    bool decodeCode(BarCode *bc);
    int decodeCode(BarCode *bc, uint16_t dec);
    int16_t voteCodes(BarCodeCluster *cluster);
    void clusterCodes();
    void renderCodes(QImage *img, float scalex, float scaley);

    void printLines();

    uint16_t *findl(const Point16 &p, bool horiz, uint16_t excludeFlags=0);

    void search(uint16_t radius, const Point16 &pRef, const Point16 &p, TwoLineList *list, uint8_t costFunc, uint16_t excludeFlags=0, uint16_t paintFlags=0);
    void setFlag(const Point16 &p, uint16_t flag);

    bool pvalid(const Point16 &p);
    bool xdirection(const Point16 &p0, const Point16 &p1);

    void renderLines(QPainter *p, const TwoLine &line, float scalex, float scaley);

    void costSelect(uint8_t costFunc, const Point16 &pRef, const Point16 &p0, const Point16 &p1, int32_t *cost, bool *test);
    void cost1(const Point16 &pRef, const Point16 &p0, const Point16 &p1, int32_t *cost, bool *test);
    void cost2(const Point16 &pRef, const Point16 &p0, const Point16 &p1, int32_t *cost, bool *test);

    uint16_t m_width;
    uint16_t m_height;
    uint16_t m_hIndex[0x200];
    uint16_t m_vIndex[0x200];
    uint16_t m_eData[LDT_EDATA_SIZE];
    uint8_t m_lineStore[0x10000];
    uint16_t m_index;
    
    uint16_t m_dist;
    uint16_t m_threshold;
    uint16_t m_hThreshold;
    uint16_t m_minLineWidth;
    uint16_t m_maxLineWidth;
    uint16_t m_maxSearchRadius;
    int32_t m_angleThresh0Tan1000; // far search
    int32_t m_angleThresh1Tan1000; // between lines
    uint16_t m_longSearchRadius;
    uint16_t m_maxMergeDist;
    bool m_linePosNeg;

    uint16_t m_maxCodeDist;

    uint16_t m_m0;
    uint16_t m_m1;
    uint16_t m_minLineLength2;

    TwoLine m_line;

    uint16_t m_lhIndex[0x200];
    uint16_t m_lvIndex[0x200];
    uint16_t m_lData[LDT_LDATA_SIZE];

    BarCode *m_candidateBarcodes[LDT_MMC_CANDIDATE_BARCODES];
    uint8_t m_barcodeIndex;
    DecodedBarCode m_votedBarcodes[LDT_MMC_VOTED_BARCODES];
    uint8_t m_votedBarcodeIndex;

    bool m_verticalEdges;
    bool m_horizontalEdges;
    bool m_verticalLines;
    bool m_horizontalLines;

    bool m_cu1;
    bool m_cu2;
    bool m_cu3;
    bool m_cu4;
};

#endif // LDTMODULE_H
