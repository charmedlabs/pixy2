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
#include <math.h>
#include <QDebug>
#include <QPainter>
#include <stdio.h>
#include "ldtmodule.h"
#include "interpreter.h"
#include "renderer.h"

// declare module
MON_MODULE(LdtModule);

static bool g_debug;

int32_t abs(int32_t v)
{
    if (v<0)
        return -v;
    return v;
}

int32_t sign(int32_t v)
{
    if (v==0)
        return 0;
    else if (v<0)
        return -1;
    return 1;
}

int32_t tanDiffAbs1000(const TwoLine &l0, const TwoLine &l1)
{
    // find tangent of angle difference between the two lines
    int16_t xdiff0, ydiff0;
    int16_t xdiff1, ydiff1;
    int32_t x, y, res;

    xdiff0 = l0.m_p1.m_x - l0.m_p0.m_x;
    ydiff0 = l0.m_p1.m_y - l0.m_p0.m_y;
    ydiff0 *= 4;
    xdiff1 = l1.m_p1.m_x - l1.m_p0.m_x;
    ydiff1 = l1.m_p1.m_y - l1.m_p0.m_y;
    ydiff1 *= 4;

    x = xdiff0*xdiff1 + ydiff0*ydiff1;
    y = ydiff0*xdiff1 - ydiff1*xdiff0;

    if (x<=0)
        return 1000000;

    res = abs(y*1000/x);
    return res;
}

uint32_t length2(const Point16 &p0, const Point16 &p1)
{
    int32_t lenx, leny;

    lenx = p0.m_x - p1.m_x;
    lenx *= lenx;
    leny = p0.m_y - p1.m_y;
    leny *= 4;
    leny *= leny;

    return lenx + leny;
}

uint16_t length1(const Point16 &p0, const Point16 &p1)
{
    int32_t lenx, leny;

    lenx = p0.m_x - p1.m_x;
    if (lenx<0)
        lenx = -lenx;
    leny = p0.m_y - p1.m_y;
    leny *= 4;
    if (leny<0)
        leny = -leny;

    if (lenx>=leny)
        return lenx;
    else
        return leny;
}

void intersect(Point16 *p, const TwoLine &line0, const TwoLine &line1)
{
    int32_t x, y, d, x1, y1, x2, y2, x3, y3, x4, y4;

    x1 = line0.m_p0.m_x;
    y1 = line0.m_p0.m_y;
    x2 = line0.m_p1.m_x;
    y2 = line0.m_p1.m_y;
    x3 = line1.m_p0.m_x;
    y3 = line1.m_p0.m_y;
    x4 = line1.m_p1.m_x;
    y4 = line1.m_p1.m_y;

    d = (x1 - x2)*(y3 - y4) - (y1 - y2)*(x3 - x4);
    if (d!=0)
    {
        x = ((x1*y2 - y1*x2)*(x3 - x4) - (x1 - x2)*(x3*y4 - y3*x4))/d;
        y = ((x1*y2 - y1*x2)*(y3 - y4) - (y1 - y2)*(x3*y4 - y3*x4))/d;
        *p = Point16(x, y);
    }
    else
        *p = Point16(0, 0);

}

int32_t distPoint2(const TwoLine &line, const Point16 &p)
{
    int32_t x0, y0, x1, y1, x2, y2, v0, v1, n, d;

    x0 = p.m_x;
    y0 = p.m_y;
    x1 = line.m_p0.m_x;
    y1 = line.m_p0.m_y;
    x2 = line.m_p1.m_x;
    y2 = line.m_p1.m_y;

    v0 = x2 - x1;
    v1 = y2 - y1;

    n = v1*x0 - v0*y0 + x2*y1 - y2*x1;
    n *= n*1000; // square, scale by 1000
    d = v1*v1 + v0*v0;

    return n/d;
}

PQueue::PQueue(uint8_t size, uint8_t adist)
{
    m_size = size;
    m_adist = adist;
    m_points = new Point16[m_size];
    reset();
}

PQueue::~PQueue()
{
    delete [] m_points;
}

void PQueue::reset()
{
    m_count = 0;
    m_tindex = 0;
    m_acount = 0;
    m_index = 0;
    m_lockout = 0;
}

void PQueue::add(const Point16 &p)
{
    m_points[m_index] = p;
    if (m_count<m_size)
        m_count++;
    m_index++;
    if (m_index>=m_size)
        m_index = 0;

}

Point16 *PQueue::get(uint8_t index)
{
    int8_t i;

    if (index+1>m_count)
        return NULL;

    i = m_index-index-1;

    if (i<0)
        i += m_size;

    return &m_points[i];
}

uint8_t PQueue::get(int8_t index, int8_t offset)
{
    index += offset;
    if (index<0)
        index += m_size;
    else if (index>=m_size)
        index -= m_size;

    return index;
}

int32_t PQueue::tda1000()
{
    Point16 *p0, *p1, *p2;

    p2 = get(2*m_adist);
    if (p2==NULL)
        return 0;
    p1 = get(m_adist);
    p0 = get(0);

    return tanDiffAbs1000(TwoLine(*p2, *p1), TwoLine(*p1, *p0));
}

bool PQueue::adetect(int32_t athresh, Point16 *p0, Point16 *pv, Point16 *p1)
{
    int8_t index;
    int32_t a=tda1000();

    if (g_debug)
        qDebug("  ad: %d %d", a, athresh);
#if 0
        if (a>athresh)
        {
            Point16 *p0, *p1, *p2;

            p2 = get(2*m_adist);
            p1 = get(m_adist);
            p0 = get(0);

            a = tanDiffAbs1000(TwoLine(*p2, *p1), TwoLine(*p1, *p0));
            qDebug("** %d 0 %d %d 1 %d %d 2 %d %d", a, p0->m_x, p0->m_y, p1->m_x, p1->m_y, p2->m_x, p2->m_y);
        }
#endif

    if (m_lockout>0)
        m_lockout--;

    if (a>athresh)
    {
        m_acount++;
        if (m_tindex==0 && m_lockout==0)
        {
            m_tindex = get(m_index, -m_adist-2);
            m_acount = 0;
        }
    }
    else if (m_tindex)
    {
        if (m_acount<1)
            m_tindex = 0;
        else
        {
            index = get(m_index, -m_adist-2);
            if (index>=m_tindex)
                index = (index+m_tindex)/2;
            else
            {
                index += m_size;
                index = (index+m_tindex)/2;
                if (index>=m_size)
                    index -= m_size;
            }
            *p0 = m_points[get(index, -m_adist)];
            *pv = m_points[index];
            *p1 = *get(0);
            m_tindex = 0;
            return true;
        }
    }

    return false;
}

void PQueue::clear(uint8_t v) // "shift" the older entries out, ie, throw them out
{
    // just change count. Index doesn't change because the most recent entries stay put
    if (v>m_count)
        m_count = 0;
    else
        m_count -= v;

    m_tindex = 0;
    m_lockout = 2*m_adist;
}

TwoLine::TwoLine()
{
    m_lines = NULL;
    reset();
}

TwoLine::TwoLine(const Point16 &p)
{
    m_lines = NULL;
    reset();
    m_p0 = p;
}

TwoLine::TwoLine(const Point16 &p0, const Point16 &p1)
{
    m_lines = NULL;
    reset();
    m_p0 = p0;
    m_p1 = p1;
}

TwoLine::~TwoLine()
{
    reset();
}

void TwoLine::reset()
{
    m_p0.m_x = m_p0.m_y = 0;
    m_p1 = m_p0;
    if (m_lines)
    {
        delete m_lines;
        m_lines = NULL;
    }
}

TwoLine *TwoLine::addLine(const TwoLine &line)
{
    if (m_lines==NULL)
        m_lines = new TwoLineList;
    return m_lines->add(line);
}

TwoLine *TwoLine::addLine(const Point16 &point)
{
    TwoLine line;
    line.m_p0 = point;
    return addLine(line);
}

uint32_t TwoLine::lines()
{
    if (m_lines==NULL)
        return 0;
    return m_lines->m_size;
}

uint32_t TwoLine::length2()
{
    return ::length2(m_p0, m_p1);
}

TwoLineList::TwoLineList()
{
    m_first = m_last = NULL;
    m_size = 0;
}

TwoLineList::~TwoLineList()
{
    clear();
}

void TwoLineList::clear()
{
    TwoLineListNode *n, *temp;

    n = m_first;
    while(n)
    {
        temp = n->m_next;
        delete n;
        n = temp;
    }
    m_first = m_last = NULL;
    m_size = 0;
}

TwoLine *TwoLineList::add(const TwoLine &line)
{
    TwoLineListNode *node = new TwoLineListNode;

    m_size++;
    node->m_line = line;
    if (m_first==NULL)
        m_first = node;
    else
        m_last->m_next = node;
    m_last = node;

    return &node->m_line;
}

TwoLine *TwoLineList::add(const TwoLine &line, int32_t val, const Point16 &p)
{
    TwoLineListNode *n, *nprev, *node = new TwoLineListNode;

    m_size++;
    node->m_line = line;
    node->m_val = val;
    node->m_p = p;

    // empty list
    if (m_first==NULL)
    {
        m_first = m_last = node;
        goto end;
    }
    // before first
    if (val<m_first->m_val)
    {
        nprev = m_first;
        m_first = node;
        node->m_next = nprev;
        goto end;
    }
    // middle
    for (n=m_first->m_next, nprev=m_first; n; nprev=n, n=n->m_next)
    {
        if (val<=n->m_val)
        {
            nprev->m_next = node;
            node->m_next = n;
            goto end;
        }
    }

    // last
    m_last->m_next = node;
    m_last = node;

    end:
    return &node->m_line;

}


bool TwoLineList::remove(TwoLineListNode *node)
{
    TwoLineListNode *n, *nprev=NULL;
    bool result = false;
    n = m_first;
    while(n)
    {
        if (n==node)
        {
            if (node==m_first)
                m_first = node->m_next;
            if (node==m_last)
                m_last = nprev;
            if (nprev)
                nprev->m_next = n->m_next;
            delete n;
            result = true;
            m_size--;
            break;
        }
        nprev = n;
        n = n->m_next;
    }
    return result;
}

void TwoLineList::merge(TwoLineList *list)
{
    if (list && list->m_first)
    {
        m_last->m_next = list->m_first;
        m_last = list->m_last;
        m_size += list->m_size;
        list->m_first = NULL; // prevent list from being destroyed when it is deleted
        list->m_last = NULL;
    }
}


void LdtModule::cost1(const Point16 &pRef, const Point16 &p0, const Point16 &p1, int32_t *cost, bool *test)
{
    // vector formed by pRef->p0, calc the angle with respect to that vector and p0->p1
    // the greater the angle the greater the cost
    int32_t td;

    td = tanDiffAbs1000(TwoLine(pRef, p0), TwoLine(p0, p1));
    *cost = td;
    *test = true;
#if 1
    // look behind us
    td = tanDiffAbs1000(TwoLine(p0, pRef), TwoLine(p0, p1));
    if (td<1000)
    {
        *cost = LDT_EXPENSIVE;
        *test = false;
    }
#endif
}

void LdtModule::cost2(const Point16 &pRef, const Point16 &p0, const Point16 &p1, int32_t *cost, bool *test)
{
    int32_t td;

    td = tanDiffAbs1000(TwoLine(pRef, p0), TwoLine(p0, p1));

    if (td<m_angleThresh0Tan1000)
    {
        *cost = LDT_EXPENSIVE;
        *test = false;
    }
    else
    {
        *cost = 1000000/(distPoint2(TwoLine(pRef, p0), p1)+1);
        *test = true;
    }


}

void LdtModule::costSelect(uint8_t costFunc, const Point16 &pRef, const Point16 &p0, const Point16 &p1, int32_t *cost, bool *test)
{
    if (costFunc==1)
        cost1(pRef, p0, p1, cost, test);
    else // costFunc==2
        cost2(pRef, p0, p1, cost, test);
}



LdtModule::LdtModule(Interpreter *interpreter) : MonModule(interpreter)
{
    QStringList scriptlet;

    scriptlet << "runprogArg 8 101";
    m_interpreter->emitActionScriptlet("Line detect/track", scriptlet);
    m_interpreter->m_pixymonParameters->addSlider("Distance2", 4, 1, 10, "Edge distance", "LDT");
    m_interpreter->m_pixymonParameters->addSlider("Threshold2", 20, 1, 75, "Edge threshold", "LDT");
    m_interpreter->m_pixymonParameters->addSlider("Minimum line width2", 0, 0, 50, "Minimum detected line width", "LDT");
    m_interpreter->m_pixymonParameters->addSlider("Maximum line width2", 50, 1, 150, "Maximum detected line width", "LDT");
    m_interpreter->m_pixymonParameters->addSlider("Minimum line length2", 40, 1, 100, "Minimum line length", "LDT");
    m_interpreter->m_pixymonParameters->addSlider("M0", 3, 1, 10, "M0", "LDT");
    m_interpreter->m_pixymonParameters->addSlider("M1", 3, 1, 10, "M1", "LDT");

    m_interpreter->m_pixymonParameters->addCheckbox("White line2", false, "White line", "LDT");

    m_interpreter->m_pixymonParameters->addCheckbox("Horizontal edges2", false, "Horizontal edges", "LDT");
    m_interpreter->m_pixymonParameters->addCheckbox("Vertical edges2", false, "Vertical edges", "LDT");
    m_interpreter->m_pixymonParameters->addCheckbox("Horizontal lines2", false, "Horizontal lines", "LDT");
    m_interpreter->m_pixymonParameters->addCheckbox("Vertical lines2", false, "Vertical lines", "LDT");
    m_interpreter->m_pixymonParameters->addCheckbox("Debug", false, "Debug", "LDT");

    m_interpreter->m_pixymonParameters->addCheckbox("cu1", true, "cu1", "LDT");
    m_interpreter->m_pixymonParameters->addCheckbox("cu2", true, "cu2", "LDT");
    m_interpreter->m_pixymonParameters->addCheckbox("cu3", true, "cu3", "LDT");
    m_interpreter->m_pixymonParameters->addCheckbox("cu4", true, "cu4", "LDT");

    m_maxSearchRadius = 9;
    uint16_t angle = 10; // for cost2
    m_angleThresh0Tan1000 = tan(M_PI*angle/180)*1000;
    angle = 46; // for determining kink (note, setting this to 45 captures lots of angle noise)
    m_angleThresh1Tan1000 = tan(M_PI*angle/180)*1000;
    m_longSearchRadius = 11;
    m_maxMergeDist = 80*80;

    m_maxCodeDist = 15*15;

#if 0
    Point16 p0 = Point16(0, 0);
    Point16 p1 = Point16(10, 0);
    Point16 p2 = Point16(11, 10);
    int32_t res;

    res = tanDiffAbs1000(TwoLine(p0, p1), TwoLine(p1, p2));
   qDebug("%d", res);
#endif
#if 0
    PQueue pq(11);
    Point16 *a;
    volatile int32_t athresh;

    angle = 30;
    athresh = tan(M_PI*angle/180)*1000;

    pq.add(Point16(50, 54));
    a = pq.adetect(athresh);
    pq.add(Point16(50, 53));
    a = pq.adetect(athresh);
    pq.add(Point16(50, 52));
    a = pq.adetect(athresh);
    pq.add(Point16(50, 51));
    a = pq.adetect(athresh);
    pq.add(Point16(50, 50));
    a = pq.adetect(athresh);
    pq.add(Point16(50, 49));
    a = pq.adetect(athresh);
    pq.add(Point16(50, 48));
    a = pq.adetect(athresh);
    pq.add(Point16(50, 47));
    a = pq.adetect(athresh);
    pq.add(Point16(50, 46));
    a = pq.adetect(athresh);
    pq.add(Point16(50, 45));
    a = pq.adetect(athresh);
    pq.add(Point16(50, 44));
    a = pq.adetect(athresh);
    pq.add(Point16(50, 43));
    a = pq.adetect(athresh);
    pq.add(Point16(54, 42));
    a = pq.adetect(athresh);
    pq.add(Point16(58, 41));
    a = pq.adetect(athresh);
    pq.add(Point16(62, 40));
    a = pq.adetect(athresh);
    pq.add(Point16(66, 39));
    a = pq.adetect(athresh);
    pq.add(Point16(70, 38));
    a = pq.adetect(athresh);
    pq.add(Point16(74, 37)); // *
    a = pq.adetect(athresh);
    Point16 *center = pq.get(11/2+1);
    pq.add(Point16(78, 36));
    a = pq.adetect(athresh);
    pq.add(Point16(82, 35));
    a = pq.adetect(athresh);
    pq.add(Point16(86, 34));
    a = pq.adetect(athresh);
    pq.add(Point16(90, 33));
    a = pq.adetect(athresh);
    pq.add(Point16(94, 32));
    a = pq.adetect(athresh);

#endif
}


LdtModule::~LdtModule()
{
}

void LdtModule::search(uint16_t radius, const Point16 &pRef, const Point16 &p, TwoLineList *list, uint8_t costFunc, uint16_t excludeFlags, uint16_t paintFlags)
{
    uint16_t *lph, *lpv, i, yrad = (radius+3)/4;
    int32_t cost;
    Point16 pt;
    bool bval;

    for (i=p.m_x-radius; i<=p.m_x+radius; i++)
    {
        pt = Point16(i, p.m_y-yrad);
        lph = findl(pt, true, excludeFlags);
        lpv = findl(pt, false, excludeFlags);
        if (lph || lpv)
        {
            costSelect(costFunc, pRef, p, pt, &cost, &bval);
            if (bval)
                list->add(TwoLine(pt), cost, p);
        }
        if (lph)
            *lph |= paintFlags;
        if (lpv)
            *lpv |= paintFlags;

        pt = Point16(i, p.m_y+yrad);
        lph = findl(pt, true, excludeFlags);
        lpv = findl(pt, false, excludeFlags);
        if (lph || lpv)
        {
            costSelect(costFunc, pRef, p, pt, &cost, &bval);
            if (bval)
                list->add(TwoLine(pt), cost, p);
        }
        if (lph)
            *lph |= paintFlags;
        if (lpv)
            *lpv |= paintFlags;
    }
    for (i=p.m_y-yrad+1; i<=p.m_y+yrad-1; i++)
    {
        pt = Point16(p.m_x-radius, i);
        lph = findl(pt, true, excludeFlags);
        lpv = findl(pt, false, excludeFlags);
        if (lph || lpv)
        {
            costSelect(costFunc, pRef, p, pt, &cost, &bval);
            if (bval)
                list->add(TwoLine(pt), cost, p);
        }
        if (lph)
            *lph |= paintFlags;
        if (lpv)
            *lpv |= paintFlags;

        pt = Point16(p.m_x+radius, i);
        lph = findl(pt, true, excludeFlags);
        lpv = findl(pt, false, excludeFlags);
        if (lph || lpv)
        {
            costSelect(costFunc, pRef, p, pt, &cost, &bval);
            if (bval)
                list->add(TwoLine(pt), cost, p);
        }
        if (lph)
            *lph |= paintFlags;
        if (lpv)
            *lpv |= paintFlags;
    }
}


bool LdtModule::render(uint32_t fourcc, const void *args[])
{
    if (fourcc==FOURCC('E','X','0','1'))
    {
        renderEX01(*(uint8_t *)args[0], *(uint16_t *)args[1], *(uint16_t *)args[2], *(uint32_t *)args[3], (uint8_t *)args[4]);
        return true;
    }
    return false;
}

bool LdtModule::command(const QStringList &argv)
{
    return false;
}

void LdtModule::paramChange()
{
    int t;

    m_dist = m_interpreter->m_pixymonParameters->value("Distance2").toUInt();
    m_threshold = m_interpreter->m_pixymonParameters->value("Threshold2").toUInt();
    m_hThreshold = m_threshold*3/5;

    m_minLineWidth = m_interpreter->m_pixymonParameters->value("Minimum line width2").toUInt();
    m_maxLineWidth = m_interpreter->m_pixymonParameters->value("Maximum line width2").toUInt();
    m_linePosNeg = m_interpreter->m_pixymonParameters->value("White line").toBool();
    m_m0 = m_interpreter->m_pixymonParameters->value("M0").toUInt();
    m_m1 = m_interpreter->m_pixymonParameters->value("M1").toUInt();
    t = m_interpreter->m_pixymonParameters->value("Minimum line length2").toUInt();
    m_minLineLength2 = t*t;

    m_horizontalEdges = m_interpreter->m_pixymonParameters->value("Horizontal edges2").toBool();
    m_verticalEdges = m_interpreter->m_pixymonParameters->value("Vertical edges2").toBool();
    m_horizontalLines = m_interpreter->m_pixymonParameters->value("Horizontal lines2").toBool();
    m_verticalLines = m_interpreter->m_pixymonParameters->value("Vertical lines2").toBool();

    g_debug = m_interpreter->m_pixymonParameters->value("Debug").toBool();

    m_cu1 = m_interpreter->m_pixymonParameters->value("cu1").toBool();
    m_cu2 = m_interpreter->m_pixymonParameters->value("cu2").toBool();
    m_cu3 = m_interpreter->m_pixymonParameters->value("cu3").toBool();
    m_cu4 = m_interpreter->m_pixymonParameters->value("cu4").toBool();
}

void LdtModule::hScan(uint8_t *line, uint16_t width)
{
    int16_t i;
    int16_t end, diff;

    i = -1;
    end = width - m_dist;

    // state 0, looking for either edge
loop0:
    i++;
    if (i>=end)
        goto loopex;
    diff = line[i+m_dist]-line[i];
    if (-m_threshold>=diff)
        goto edge0;
    if (diff>=m_threshold)
        goto edge1;
    goto loop0;

    // found neg edge
edge0:
    m_eData[m_index] = i | 0x8000;
    m_index++;
    //i++; // skip a pixel to save time

    // state 1, looking for end of edge or pos edge
loop1:
    i++;
    if (i>=end)
        goto loopex;
    diff = line[i+m_dist]-line[i];
    if (-m_hThreshold<diff)
        goto loop0;
    if (diff>=m_threshold)
        goto edge1;
    goto loop1;

    // found pos edge
edge1:
    m_eData[m_index] = i;
    m_index++;
    //i++; // skip a pixel to save time

    // state 2, looking for end of edge or neg edge
loop2:
    i++;
    if (i>=end)
        goto loopex;
    diff = line[i+m_dist]-line[i];
    if (diff<m_hThreshold)
        goto loop0;
    if (-m_threshold>=diff)
        goto edge0;
    goto loop2;

loopex:
    return;
}


void LdtModule::vScan(uint8_t *line, uint16_t width)
{
    int16_t i;
    int16_t diff;
    uint8_t *line0;

    i = -1;
    if (m_dist<2)
        line0 = line - width;
    else
        line0 = line - m_dist/2*width;

loop:
    i++;
    if (i>=width)
        goto loopex;
    diff = line[i]-line0[i];
    if (-m_threshold>=diff)
        goto edge0;
    if (diff>=m_threshold)
        goto edge1;

    goto loop;

edge0:
    m_eData[m_index] = (i>>1) | 0x8000;
    i |= 1;
    m_index++;
    goto loop;

edge1:
    m_eData[m_index] = i>>1;
    i |= 1;
    m_index++;
    goto loop;

loopex:
    return;
}

void LdtModule::printLines()
{
    int16_t i, j;

    qDebug("Begin lines");
    // start at bottom of image and work up
    for (i=m_height-1; i>=0; i--)
    {
        for (j=m_lhIndex[i]; m_lData[j]!=LDT_EOL; j++)
        {
            qDebug("  h: %d, %d", m_lData[j], i);
        }
        for (j=m_lvIndex[i]; m_lData[j]!=LDT_EOL; j++)
        {
            qDebug("  v: %d, %d", m_lData[j], i);
        }
    }
    qDebug("end lines");
}

void LdtModule::extractLines()
{
    uint16_t i, j, bit0, bit1, col0, col1, lineWidth;
    int16_t rowOffs;
    uint8_t vstate[m_width/2];

    for (i=0; i<m_width/2; i++)
        vstate[i] = 0;

    // row offset for vertical scan lines
    rowOffs = m_dist/2-m_minLineWidth/8;

    // look through horizontal scan data
    for (i=0, m_index=0; i<m_height; i++)
    {
        if (m_index>=LDT_LDATA_SIZE-m_width/2)
            break;

        m_lhIndex[i] = m_index;
        // copy a lot of code to reduce branching, make it faster
        if (m_linePosNeg) // pos neg
        {
            for (j=m_hIndex[i]; m_eData[j]!=LDT_EOL && m_eData[j+1]!=LDT_EOL; j++)
            {
                bit0 = m_eData[j]&0x8000;
                bit1 = m_eData[j+1]&0x8000;
                col0 = m_eData[j]&~0x8000;
                col1 = m_eData[j+1]&~0x8000;
                if (bit0==0 && bit1!=0)
                {
                    lineWidth = col1 - col0;
                    if (m_minLineWidth<lineWidth && lineWidth<m_maxLineWidth)
                        m_lData[m_index++] = ((col0+col1)>>1) + m_dist;
                }
            }
        }
        else // neg pos
        {
            // hscan
            for (j=m_hIndex[i]; m_eData[j]!=LDT_EOL && m_eData[j+1]!=LDT_EOL; j++)
            {
                bit0 = m_eData[j]&0x8000;
                bit1 = m_eData[j+1]&0x8000;
                col0 = m_eData[j]&~0x8000;
                col1 = m_eData[j+1]&~0x8000;
                if (bit0!=0 && bit1==0)
                {
                    lineWidth = col1 - col0;
                    if (m_minLineWidth<lineWidth && lineWidth<m_maxLineWidth)
                        m_lData[m_index++] = ((col0+col1)>>1) + m_dist;
                }
            }
        }
        m_lData[m_index++] = LDT_EOL; // terminate

        // vscan

        if (i-rowOffs>=0)
            m_lvIndex[i-rowOffs] = m_index;
        if (m_linePosNeg)
        {
            for (j=m_vIndex[i]; m_eData[j]!=LDT_EOL; j++)
            {
                bit0 = m_eData[j]&0x8000;
                col0 = m_eData[j]&~0x8000;
                if (bit0==0) // pos
                {
                    //if (vstate[col0]==0)
                        vstate[col0] = i+1;
                }
                else // bit0!=0, neg
                {
                    if (vstate[col0]!=0)
                    {
                        lineWidth = (i - (vstate[col0]-1))<<2; // multiply by 4 because vertical is subsampled by 4
                        if (m_minLineWidth<lineWidth && lineWidth<m_maxLineWidth)
                            m_lData[m_index++] = col0<<1; // multiply by 2 because horizontal is subsampled by 2
                        vstate[col0] = 0;
                    }
                }
            }
        }
        else
        {
            for (j=m_vIndex[i]; m_eData[j]!=LDT_EOL; j++)
            {
                bit0 = m_eData[j]&0x8000;
                col0 = m_eData[j]&~0x8000;
                if (bit0!=0) // neg
                {
                    //if (vstate[col0]==0)
                        vstate[col0] = i+1;
                }
                else // bit0==0, pos
                {
                    if (vstate[col0]!=0)
                    {
                        lineWidth = (i - (vstate[col0]-1))<<2; // multiply by 4 because vertical is subsampled by 4
                        if (m_minLineWidth<lineWidth && lineWidth<m_maxLineWidth)
                            m_lData[m_index++] = col0<<1; // multiply by 2 because horizontal is subsampled by 2
                        vstate[col0] = 0;
                    }
                }
            }
        }
        m_lData[m_index++] = LDT_EOL; // terminate
    }
    // add back empty rows for vertical scan lines
    for (i=m_height-rowOffs; i<m_height; i++)
    {
        m_lvIndex[i] = m_index;
        m_lData[m_index++] = LDT_EOL;
    }


}

uint16_t *LdtModule::findl(const Point16 &p, bool horiz, uint16_t excludeFlags)
{
    uint16_t i, *index;
    int16_t x, y;

    x = p.m_x;
    y = p.m_y;

    if (horiz)
        index = m_lhIndex;
    else
        index = m_lvIndex;

    if (x<0)
        return NULL;
    if (x>=m_width)
        return NULL;
    if (y<0)
        return NULL;
    if (y>=m_height)
        return NULL;

    if (horiz)
    {
        for (i=index[y]; m_lData[i]!=LDT_EOL; i++)
        {
            if ((m_lData[i]&LDT_COL_MASK)==x && (m_lData[i]&excludeFlags)==0)
                return &m_lData[i];
        }
    }
    else
    {
        for (i=index[y]; m_lData[i]!=LDT_EOL; i++)
        {
            if ((m_lData[i]&LDT_COL_MASK)==(x&0xfffe) && (m_lData[i]&excludeFlags)==0)
                return &m_lData[i];
        }
    }
    return NULL;
}



bool LdtModule::pvalid(const Point16 &p)
{
    if (p.m_x==0 && p.m_y==0)
        return false;
    if (p.m_x>=m_width)
        return false;
    if (p.m_y>=m_height)
        return false;

    return true;
}

bool LdtModule::xdirection(const Point16 &p0, const Point16 &p1)
{
    int16_t xdiff, ydiff;

    xdiff = p1.m_x - p0.m_x;
    ydiff = p1.m_y - p0.m_y;
    ydiff *= 4;

    return abs(xdiff)>abs(ydiff);
}

void LdtModule::setFlag(const Point16 &p, uint16_t flag)
{
    uint16_t *lph, *lpv;

    lph = findl(p, true);
    lpv = findl(p, false);

    if (lph)
        *lph |= flag;
    if (lpv)
        *lpv |= flag;
}


int32_t LdtModule::decodeCode(BarCode *bc, uint16_t dec)
{
    uint8_t i, bits;
    uint16_t val, bit, width, minWidth, maxWidth;
    bool flag;

    for (i=1, bits=0, val=0, flag=false, minWidth=0xffff, maxWidth=0; i<bc->m_n && bits<LDT_MMC_BITS; i++)
    {
        bit = bc->m_edges[i]&0x8000;
        if ((bit==0 && (i&1)) || (bit && (i&1)==0))
            return -1;

        width = bc->m_edges[i]&~0x8000;
        if (width<minWidth)
            minWidth = width;
        if (width>maxWidth)
            maxWidth = width;
        if (width<dec)
        {
            if (flag)
            {
                val <<= 1;
                if (bit==0)
                    val |= 1;
                bits++;
                flag = false;
            }
            else
                flag = true;
        }
        else if (flag) // wide with flag, must be error
            return 0;
        else // wide
        {
            val <<= 1;
            if (bit==0)
                val |= 1;
            bits++;
        }
    }
    if (bits!=LDT_MMC_BITS)
        return 0;
    if (maxWidth/minWidth>10)
        return -2;
    bc->m_val = val;
    return 1;
}

bool LdtModule::decodeCode(BarCode *bc)
{
    uint8_t i;
    int32_t res;
    uint16_t inc, dec;

    bc->m_val = -1; // set value to invalid
    inc = bc->m_edges[0]>>2; // 1/4
    if (inc==0)
        inc = 1;

    // try values between 1.25 and 2.0
    // it's important to start low and move up because if we assume all edges
    // are short, it results in no errors.
    for (i=0, dec=bc->m_edges[0]+inc; i<4; dec+=inc, i++)
    {
        res = decodeCode(bc, dec);
        if (res==1)
            return true;
        else if (res<0) // error
            return false;
    }

    return false;
}

bool LdtModule::detectCode(uint16_t *edges, bool begin, BarCode *bc)
{
    uint16_t col00, col0, col1, col01, prev, width0, width, qWidth;
    uint8_t e;

    col00 = edges[0]&~0x8000;
    col01 = edges[1]&~0x8000;
    width0 = col01 - col00;
    qWidth = width0<<2;

    // check front quiet period
    if (!begin)
    {
        width = col00 - (edges[-1]&~0x8000);
        if (width<qWidth)
            return false;
    }


    // first determine if we have enough edges
    for (e=1, prev=col00, bc->m_n=0; edges[e]!=LDT_EOL; e++, prev=col0)
    {
        if  (e>=LDT_MMC_MAX_EDGES)
            return false; // too many edges for valid code
        col0 = edges[e]&~0x8000;
        // correct
        if ((edges[e]&0x8000)==0)
            col0--;
        // save edge
        bc->m_edges[e-1] = (col0-prev) | (edges[e]&0x8000);
        bc->m_n++;
        bc->m_width = col0 - col00;
        if  (edges[e+1]==LDT_EOL)
            break;
        col1 = edges[e+1]&~0x8000;

        width = col1 - col0;
        if (width>qWidth)
            break;
    }

    if (e<LDT_MMC_MIN_EDGES-1)
        return false;

    bc->m_p0.m_x = col00;
    return true;
}


int16_t LdtModule::voteCodes(BarCodeCluster *cluster)
{
    uint8_t votes[LDT_MMC_VTSIZE];
    int16_t vals[LDT_MMC_VTSIZE];
    int16_t val;
    uint16_t i, j;
    uint8_t max, maxIndex;

    for (i=0; i<LDT_MMC_VTSIZE; i++)
        votes[i] = 0;

    // tally votes
    for (i=0; i<cluster->m_n; i++)
    {
        val = m_candidateBarcodes[cluster->m_indexes[i]]->m_val;
        if (val<0)
            continue;
        // find index or empty location
        for (j=0; j<LDT_MMC_VTSIZE; j++)
        {
            if (votes[j]==0)
            {
                vals[j] = val;
                break;
            }
            if (vals[j]==val)
                break;
        }
        if (j>=LDT_MMC_VTSIZE)
            continue;
        // add vote
        votes[j]++;
    }

    // find winner
    for (i=0, max=0; i<LDT_MMC_VTSIZE; i++)
    {
        if (votes[i]==0) // we've reached end
            break;
        if (votes[i]>max)
        {
            max = votes[i];
            maxIndex = i;
        }
    }

    if (max==0) // no valid codes
        return -1;
    return vals[maxIndex];
}

void LdtModule::clusterCodes()
{
    BarCodeCluster *clusters[LDT_MMC_VOTED_BARCODES];
    uint8_t i, j, numClusters = 0;
    int16_t val;
    int32_t dist;

    for (i=0; i<m_barcodeIndex; i++)
    {
        for (j=0; j<numClusters; j++)
        {
            dist = length2(clusters[j]->m_p1, m_candidateBarcodes[i]->m_p0);
            if (dist<m_maxCodeDist)
                break;
        }
        if (j>=LDT_MMC_VOTED_BARCODES) // table is full, move onto next code
            continue;
        if (j>=numClusters) // new entry
        {
            clusters[j] = new BarCodeCluster;
            // reset positions
            clusters[j]->m_p0 = m_candidateBarcodes[i]->m_p0;
            clusters[j]->m_p1 = m_candidateBarcodes[i]->m_p0;
            numClusters++;
        }
//        qDebug(" add %d %d, %d %d %d %d", j, i, m_candidateBarcodes[i]->m_p0.m_x, m_candidateBarcodes[i]->m_p0.m_y,
//               clusters[j]->m_p1.m_x, clusters[j]->m_p1.m_y);
        clusters[j]->addCode(i);
        // update width, position
        clusters[j]->updateWidth(m_candidateBarcodes[i]->m_width);
        clusters[j]->m_p1 = m_candidateBarcodes[i]->m_p0;
    }

    // vote
    for (i=0, m_votedBarcodeIndex=0; i<numClusters; i++)
    {
        if (m_votedBarcodeIndex>=LDT_MMC_VOTED_BARCODES)
            break; // out of table space
        val = voteCodes(clusters[i]);
        if (val<0)
            continue;
        m_votedBarcodes[m_votedBarcodeIndex].m_val = val;
        m_votedBarcodes[m_votedBarcodeIndex].m_outline.m_xOffset = clusters[i]->m_p0.m_x + m_dist;
        m_votedBarcodes[m_votedBarcodeIndex].m_outline.m_yOffset = clusters[i]->m_p0.m_y;
        m_votedBarcodes[m_votedBarcodeIndex].m_outline.m_width = clusters[i]->m_width + m_dist + 1;
        m_votedBarcodes[m_votedBarcodeIndex].m_outline.m_height = clusters[i]->m_p1.m_y - clusters[i]->m_p0.m_y + 1;
        m_votedBarcodeIndex++;
    }

    for (i=0; i<m_votedBarcodeIndex; i++)
        qDebug("* %d, %d %d %d %d", m_votedBarcodes[i].m_val,
               m_votedBarcodes[i].m_outline.m_xOffset, m_votedBarcodes[i].m_outline.m_yOffset,
               m_votedBarcodes[i].m_outline.m_width, m_votedBarcodes[i].m_outline.m_height);

    for (i=0; i<numClusters; i++)
        delete clusters[i];

    for (i=0; i<m_barcodeIndex; i++)
        delete m_candidateBarcodes[i];
}

void LdtModule::detectCodes()
{
    uint16_t bit0, bit1, i, j, k;
    uint8_t e;
    bool begin;
    BarCode *bc;
    int32_t res;

    // look through horizontal scan data
    for (i=0, m_barcodeIndex=0, bc=new BarCode; i<m_height; i++)
    {
        // find number of edges -- put in e
        for (j=m_hIndex[i], e=0; m_eData[j]!=LDT_EOL; j++, e++);
        if (e<LDT_MMC_MIN_EDGES)
            continue;

        for (j=m_hIndex[i], begin=true, k=0; m_eData[j]!=LDT_EOL && m_eData[j+1]!=LDT_EOL; j++, begin=false, k++)
        {
            bit0 = m_eData[j]&0x8000;
            bit1 = m_eData[j+1]&0x8000;
            if (bit0!=0 && bit1==0 && e>=LDT_MMC_MIN_EDGES-1+k)
            {
                bc->m_p0.m_y = i;

                if (detectCode(&m_eData[j], begin, bc))
                {
                    res = decodeCode(bc);
                    qDebug("%d %d: %d %d: %d, %d %d %d %d %d %d %d %d %d", bc->m_p0.m_x, bc->m_p0.m_y, res, bc->m_val,
                           bc->m_n, bc->m_edges[0]&~0x8000, bc->m_edges[1]&~0x8000, bc->m_edges[2]&~0x8000, bc->m_edges[3]&~0x8000, bc->m_edges[4]&~0x8000,
                            bc->m_edges[5]&~0x8000, bc->m_edges[6]&~0x8000, bc->m_edges[7]&~0x8000, bc->m_edges[8]&~0x8000);
                    m_candidateBarcodes[m_barcodeIndex++] = bc;
                    if (m_barcodeIndex>=LDT_MMC_CANDIDATE_BARCODES)
                        return;
                    bc = new BarCode;

                }
            }
        }
    }
    delete bc;

    clusterCodes();
}


void LdtModule::processLine(TwoLine *line, const Point16 &pRef, bool angleDetect)
{
    uint16_t i;
    bool pvertex = false;
    Point16 p, pr, *pmiddle, *pend, p0, pv, p1;
    PQueue pqueue(LDT_PQUEUE_SIZE, LDT_PQUEUE_ADIST);
    TwoLineList list;

    p = line->m_p0;
    // update p1
    line->m_p1 = p;
    setFlag(p, LDT_NULL_FLAG);
    if (g_debug)
        qDebug("* pl x:%d y:%d", pRef.m_x, pRef.m_y);
    while(1)
    {
        if (line->m_lines==NULL)
            line->m_lines = new TwoLineList;

        pqueue.add(p);
        pmiddle = pqueue.get(LDT_PQUEUE_ADIST);
        pend = pqueue.get(LDT_PQUEUE_ADIST*2);
        if (angleDetect)
            pvertex = pqueue.adetect(m_angleThresh1Tan1000, &p0, &pv, &p1);

        if (g_debug)
        {
            qDebug("start x:%d y:%d", p.m_x, p.m_y);
            if(pmiddle)
                qDebug("  pmiddle: x:%d y:%d", pmiddle->m_x, pmiddle->m_y);
        }

        if (pmiddle==NULL)
            pr = pRef;
        else
        {
            pr = *pmiddle;
            if (!pvalid(line->m_phalf))
                line->m_phalf = p;
        }

        for (i=1; i<=m_maxSearchRadius; i++)
        {
            list.clear();
            search(i, pr, p, &list, 1, LDT_NULL_FLAG, LDT_NULL_FLAG);
            if (g_debug && i>1)
                qDebug("  searching %d", i);
            if (list.m_size>0)
                break;
        }

        if (list.m_size==0)
        {
            line->m_p1 = p;
            if (g_debug)
                qDebug("end");
            return;
        }

        // take first point, make it part of our line
        p = list.m_first->m_line.m_p0;

        // handle kink if it exists
        if (pvertex)
        {
            line->m_p1 = p0; // stop this segment before chaotic intersection of lines
            if (g_debug)
            {
                qDebug("* vertex px:%d py:%d", p.m_x, p.m_y);

#if 0
                tl = line->addLine(Point16(p0));
                tl->m_p1 = Point16(pv);
                tl = line->addLine(pv);
                tl->m_p1 = Point16(p1);
#endif
            }

            line = line->addLine(p); // new line becomes current line
            pqueue.clear(LDT_PQUEUE_ADIST);
        }

        // long search
        else if (pend)
            search(m_longSearchRadius, *pend, *pmiddle, line->m_lines, 2, LDT_NULL_FLAG, 0);

    }
}


void LdtModule::renderLines(QPainter *p, const TwoLine &line, float scalex, float scaley)
{
    TwoLineListNode *n;

    if (!pvalid(line.m_p1))
        return;
    p->drawLine(QPoint(line.m_p0.m_x*scalex, line.m_p0.m_y*scaley), QPoint(line.m_p1.m_x*scalex, line.m_p1.m_y*scaley));
    if (line.m_lines==NULL)
        return;
    for (n=line.m_lines->m_first; n; n=n->m_next)
        renderLines(p, n->m_line, scalex, scaley);
}

void LdtModule::cleanupLine2(TwoLine *line)
{
    bool inside, xdir;
    int32_t dist, a;
    Point16 p, ptemp;
    uint16_t v0, v1, xavg, yavg, min;
    TwoLineListNode *n, *m;
    bool flag, flag2;

    // line logic...
    xdir = xdirection(line->m_p0, line->m_p1);
    if (xdir)
    {
        if (line->m_p0.m_x<line->m_p1.m_x)
        {
            v0 = line->m_p0.m_x;
            v1 = line->m_p1.m_x;
        }
        else
        {
            v0 = line->m_p1.m_x;
            v1 = line->m_p0.m_x;
        }
    }
    else
    {
        if (line->m_p0.m_y<line->m_p1.m_y)
        {
            v0 = line->m_p0.m_y;
            v1 = line->m_p1.m_y;
        }
        else
        {
            v0 = line->m_p1.m_y;
            v1 = line->m_p0.m_y;
        }
    }

    // mark all inside lines
    for (n=line->m_lines->m_first; n; n=n->m_next)
    {
        // look for small angle condition
        a = tanDiffAbs1000(*line, n->m_line);

        // is it a small angle?
        n->m_bval2 = a<m_angleThresh0Tan1000;
        if (n->m_bval2) // if so, skip, deal with later
            continue;

        // find intersection
        intersect(&p, *line, n->m_line);
        if (!pvalid(p))
            continue;

        // determine if line intersection is "inside" or "outside" the main line
        if (xdir)
            inside = v0<=p.m_x && p.m_x<=v1;
        else
            inside = v0<=p.m_y && p.m_y<=v1;

        n->m_bval = inside;
        n->m_val = 1;
        if (inside)
        {
            a = tanDiffAbs1000(TwoLine(line->m_p0, n->m_p), n->m_line);
            // is it a small angle?
            n->m_bval2 = a<m_angleThresh0Tan1000;
            if (n->m_bval2) // if so, skip, deal with later
                continue;

            // find new intersection
            intersect(&p, TwoLine(line->m_p0, n->m_p), n->m_line);
            if (pvalid(p))
                n->m_line.m_p0 = p;
            else
                n->m_line.m_p0 = n->m_p;
        }
        else
            n->m_line.m_p0 = p; // remember intersection
    }

    while(1)
    {
        for (n=line->m_lines->m_first, flag=false; n; n=n->m_next)
        {
            if (n->m_bval2) // skip small angle
                continue;
            for (m=line->m_lines->m_first; m; m=m->m_next)
            {
                if (n==m || m->m_bval2)
                    continue;
                dist = length2(n->m_line.m_p0, m->m_line.m_p0);
                if (dist!=0 && dist<=m_maxMergeDist)
                {
                    xavg = (n->m_line.m_p0.m_x + m->m_line.m_p0.m_x)/2;
                    yavg = (n->m_line.m_p0.m_y + m->m_line.m_p0.m_y)/2;
                    p = Point16(xavg, yavg);
                    n->m_line.m_p0 = p;
                    m->m_line.m_p0 = p;
                    flag = true;
                }
            }
        }
        if (flag==false)
            break;
    }

    // We're only interested in the first intersection, so find the closest point to m_p0.
    for (n=line->m_lines->m_first, min=0xffff, flag=false, flag2=false; n; n=n->m_next)
    {
        if (n->m_bval2) // if we're not a small angle...
            continue;
        dist = length1(line->m_p0, n->m_line.m_p0);
        if (dist<min)
        {
            min = dist;
            p = n->m_line.m_p0;
            flag = n->m_bval;
            flag2 = true;
        }
    }

    // split line if first intersection is an inside line
    if (flag)
    {
        ptemp = line->m_p1;
        line->m_p1 = p;
        line->addLine(TwoLine(p, ptemp));
    }
    else if (flag2)
        line->m_p1 = p; // make main line endpoint the first intersection

    // deal with small angle
    for (n=line->m_lines->m_first; n; n=n->m_next)
    {
        if (n->m_bval2 && flag2)
            n->m_line.m_p0 = p;
    }

}

void LdtModule::cleanupLine(TwoLine *line)
{
    TwoLineListNode *n;
    bool flag;

    while(1)
    {
        flag = false;
        n = line->m_lines->m_first;
        while(n)
        {
            if (pvalid(n->m_line.m_p1))
            {
                flag = true;
                if (length2(n->m_line.m_p0, n->m_line.m_p1)<m_minLineLength2 ||
                        tanDiffAbs1000(*line, n->m_line)<m_angleThresh1Tan1000)
                {
                    // make this line's endpoint our endpoint
                    line->m_p1 = n->m_line.m_p1;
                    // append its list to our list
                    line->m_lines->merge(n->m_line.m_lines);
                    // remove line from list
                    line->m_lines->remove(n);
                }
                else
                    // move onto next segment
                    line = &n->m_line;
                break;
            }
            n = n->m_next;
        }
        if (flag==false)
            break;
    }
}

void LdtModule::processLines(TwoLine *line, const Point16 &pRef)
{
    int32_t len2;
    Point16 p;
    TwoLineListNode *n, *temp;

    processLine(line, pRef, true);
    if (m_cu1)
        cleanupLine(line);

    // look at first line for HO's

    n = line->m_lines->m_first;
    if (m_cu2)
    {
        while(n)
        {
            // process HO
            if (!pvalid(n->m_line.m_p1))
                processLine(&n->m_line, n->m_p, false);

            if (m_cu3)
            {
                // is the line too short?  if so, delete
                len2 = length2(n->m_line.m_p0, n->m_line.m_p1);

                if (len2<=m_minLineLength2)
                {
                    temp = n->m_next;
                    line->m_lines->remove(n);
                    n = temp;
                    continue;
                }
                else
                {
                    // is the line long enough to shorten and get a better line estimate?
                    if (len2>3*LDT_PQUEUE_ADIST*3*LDT_PQUEUE_ADIST && pvalid(n->m_line.m_phalf))
                        n->m_line.m_p0 = n->m_line.m_phalf;
                }
            }
            n = n->m_next;
        }
    }

    if (m_cu4)
        cleanupLine2(line);
}

void LdtModule::processLines()
{
    int16_t i, j, k;

    m_line.reset();
    if (g_debug)
        qDebug("*****");
    // start at bottom of image and work up
    for (i=m_height-2; i>=0; i--)
    {
        j=m_lhIndex[i];
        k=m_lvIndex[i];
        while(1)
        {
            if (m_lData[j]!=LDT_EOL)
            {
                m_line.m_p0.m_x = m_lData[j];
                m_line.m_p0.m_y = i;
                processLines(&m_line, Point16(m_lData[j], i+1));
                return;
                j++;
            }

            if (m_lData[k]!=LDT_EOL)
            {
                m_line.m_p0.m_x = m_lData[k];
                m_line.m_p0.m_y = i;
                processLines(&m_line, Point16(m_lData[k], i+1));
                return;
                k++;
            }

            if (m_lData[j]==LDT_EOL && m_lData[k]==LDT_EOL)
                break;
        }
    }
}

void LdtModule::renderCodes(QImage *img, float scalex, float scaley)
{
    uint8_t i;
    uint16_t x, y;
    QPainter p;
    QString str;

    p.begin(img);
    p.setPen(QPen(QColor(0xff, 0xff, 0xff, 0xff)));

#ifdef __MACOS__
    QFont font("verdana", 18);
#else
    QFont font("verdana", 12);
#endif
    p.setFont(font);
    for (i=0; i<m_votedBarcodeIndex; i++)
    {
        str = QString::number(m_votedBarcodes[i].m_val);
        x = m_votedBarcodes[i].m_outline.m_xOffset*scalex;
        y = m_votedBarcodes[i].m_outline.m_yOffset*scaley;
        p.drawRect(x, y, m_votedBarcodes[i].m_outline.m_width*scalex, m_votedBarcodes[i].m_outline.m_height*scaley);
        p.setPen(QPen(QColor(0, 0, 0, 0xff)));
        p.drawText(x+1, y+1, str);
        p.setPen(QPen(QColor(0xff, 0xff, 0xff, 0xff)));
        p.drawText(x, y, str);
    }
    p.end();
}

// bg
// gr
void LdtModule::renderEX01(uint8_t renderFlags, uint16_t width, uint16_t height, uint32_t frameLen, uint8_t *frame)
{
    uint16_t i, j, x, y, lineStoreIndex;

    // M0 processing
    for (i=0, y=0, lineStoreIndex=0, m_index=0; y<height; y+=4, lineStoreIndex+=width, i++)
    {
        for (x=0; x<width; x+=2)
        {
            m_lineStore[lineStoreIndex+x+0] = frame[(y+1)*width+x];
            m_lineStore[lineStoreIndex+x+1] = frame[(y+0)*width+x+1];
        }
        // check to see if we have enough space in the edge data array
        if (m_index>=LDT_EDATA_SIZE-2*width)
            break;

        // horizontal scan
        m_hIndex[i] = m_index;
        hScan(m_lineStore+lineStoreIndex, width);
        m_eData[m_index++] = LDT_EOL; // terminate data

        // vertical scan
        m_vIndex[i] = m_index;
        if (i>=m_dist/2)
            vScan(m_lineStore+lineStoreIndex, width);
        m_eData[m_index++] = LDT_EOL; // terminate data

    }

    // M4 processing
    m_width = width;
    m_height = height/4;
    extractLines();


    // rendering
    QPainter p;
    float scale = (float)m_renderer->m_video->activeWidth()/m_width;
    QImage imgLine(width*scale, height*scale, QImage::Format_ARGB32);
    QImage imgCode(width*scale, height*scale, QImage::Format_ARGB32);
    QImage imgh(width, height/4, QImage::Format_ARGB32);
    QImage imgv(width/2, height/4, QImage::Format_ARGB32);
    QImage imglh(width, height/4, QImage::Format_ARGB32);
    QImage imglv(width, height/4, QImage::Format_ARGB32);
    imgLine.fill(0x00000000);
    imgCode.fill(0x00000000);
    imgh.fill(0x00000000);
    imgv.fill(0x00000000);
    imglh.fill(0x00000000);
    imglv.fill(0x00000000);

    for (i=0; i<height/4; i++)
    {
        // render horizontal scan edges (vertical edges)
        if (m_verticalEdges)
        {
            for (j=m_hIndex[i]; m_eData[j]!=LDT_EOL; j++)
            {
                if (m_eData[j]&0x8000)
                    imgh.setPixel((m_eData[j]&~0x8000)+m_dist, i, 0x80ff0000);
                else
                    imgh.setPixel(m_eData[j]+m_dist, i, 0x800000ff);
            }
        }

        // render vertical scan edges (horizontal edges)
        if (m_horizontalEdges)
        {
            for (j=m_vIndex[i]; m_eData[j]!=LDT_EOL; j++)
            {
                if (m_eData[j]&0x8000)
                    imgv.setPixel((m_eData[j]&~0x8000), i, 0x8000ff00);
                else
                    imgv.setPixel(m_eData[j], i, 0x80ffff00);
            }
        }

        // render horizontal scan lines (vertical lines)
        if(m_verticalLines)
        {
            for (j=m_lhIndex[i]; m_lData[j]!=LDT_EOL; j++)
               imglh.setPixel(m_lData[j], i, 0xffffffff);
        }

        // render vertical scan lines (horizontal lines)
        if (m_horizontalLines)
        {
            for (j=m_lvIndex[i]; m_lData[j]!=LDT_EOL; j++)
            {
                imglv.setPixel(m_lData[j], i, 0xffc0c0c0); // subtract minimum width /4 because we're subsampled by 4, and and additional /2 because we want to take half
                imglv.setPixel(m_lData[j]+1, i, 0xffc0c0c0); // add another pixel since width we're subsampled
            }
        }

    }

    if (g_debug)
        printLines();
#if 1
    processLines();
#endif
#if 0
    detectCodes();
#endif
#if 0
    m_line.m_p1.m_x = 10;
    m_line.m_p1.m_y = 10;
    TwoLine line;
    line.m_p0.m_x = 10;
    line.m_p0.m_y = 20;
    line.m_p1.m_x = 10;
    line.m_p1.m_y = 5;
    m_line.addLine(line);

    TwoLine line1;
    line1.m_p0.m_x = 1;
    line1.m_p0.m_y = 1;
    line1.m_p1.m_x = 2;
    line1.m_p1.m_y = 2;
    TwoLine line2;
    line2.m_p0.m_x = 5;
    line2.m_p0.m_y = 6;
    line2.m_p1.m_x = 7;
    line2.m_p1.m_y = 8;

    TwoLineList list;
    list.add(line2, 6);
    list.add(line1, 7);
    list.add(line, 5);

    Point16 pt(0, 0);
    volatile int32_t d = distPoint2(line, pt);
    pt = Point16(10, 1000);
    d = distPoint2(line, pt);
    pt = Point16(5, -1000);
    d = distPoint2(line, pt);
    pt = Point16(-10, -10);
    d = distPoint2(line1, pt);
    pt = Point16(-10, -11);
    d = distPoint2(line1, pt);
    pt = Point16(-10, -9);
    d = distPoint2(line1, pt);

#if 0
    for (TwoLineListNode *n=list.m_first; n; n=n->m_next)
    {
        if (n->m_line.m_p0.m_x==1)
        {
            list.remove(n);
            break;
        }
    }
    list.remove(list.m_first);
    list.remove(list.m_last);
    list.remove(list.m_first);
#endif
#endif

    p.begin(&imgLine);
    p.setPen(QPen(QColor(0x00, 0xff, 0x40, 0xff), 3));
    renderLines(&p, m_line, scale, scale*4);
    p.end();

    //renderCodes(&imgCode, scale, scale*4);

    m_renderer->renderBA81(RENDER_FLAG_BLEND, width, height, frameLen, frame);
    m_renderer->emit image(imgLine, RENDER_FLAG_BLEND);
    m_renderer->emit image(imgCode, RENDER_FLAG_BLEND);
    m_renderer->emit image(imgh, RENDER_FLAG_BLEND);
    m_renderer->emit image(imgv, RENDER_FLAG_BLEND);
    m_renderer->emit image(imglh, RENDER_FLAG_BLEND);
    m_renderer->emit image(imglv, renderFlags|RENDER_FLAG_BLEND);
}

