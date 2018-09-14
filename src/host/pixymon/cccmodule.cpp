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

#include <QPainter>
#include <stdio.h>
#include "cccmodule.h"
#include "interpreter.h"
#include "renderer.h"
#include "qqueue.h"
#include "colorlut.h"
#include "calc.h"

// declare module
MON_MODULE(CccModule);

CccModule::CccModule(Interpreter *interpreter) : MonModule(interpreter)
{
    int i;

    m_crc = 0;
    m_qvals = new uint32_t[0x8000];
    m_numQvals = 0;


    for (i=0; i<CL_NUM_SIGNATURES; i++)
        m_palette[i] = Qt::black;
}

CccModule::~CccModule()
{
    delete [] m_qvals;
}

bool CccModule::render(uint32_t fourcc, const void *args[])
{
    if (fourcc==FOURCC('C', 'C', 'B', '1'))
    {
        renderCCB1(*(uint8_t *)args[0], *(uint16_t *)args[1], *(uint32_t *)args[2], *(uint32_t *)args[3], (uint8_t *)args[4]);
        return true;
    }
    else if (fourcc==FOURCC('C', 'C', 'B', '2'))
    {
        renderCCB2(*(uint8_t *)args[0], *(uint16_t *)args[1], *(uint32_t *)args[2], *(uint32_t *)args[3], (uint16_t *)args[4], *(uint32_t *)args[5], (uint16_t *)args[6]);
        return true;
    }
    else if (fourcc==FOURCC('C','C','Q','F'))
    {
        renderCCQF(*(uint8_t *)args[0], *(uint16_t *)args[1], *(uint16_t *)args[2]);
        return true;
    }
    else if (fourcc==FOURCC('C','C','Q','S'))
    {
        renderCCQS(*(uint32_t *)args[0], (uint32_t *)args[1]);
        return true;
    }
    return false;
}

bool CccModule::command(const QStringList &argv)
{
    return false;
}

uint16_t convert10to8(uint32_t signum)
{
    uint16_t res=0;
    uint32_t q;

    q = signum/10000;
    if (q)
    {
        res += q*8*8*8*8;
        signum -= q*10000;
    }
    q = signum/1000;
    if (q)
    {
        res += q*8*8*8;
        signum -= q*1000;
    }
    q = signum/100;
    if (q)
    {
        res += q*8*8;
        signum -= q*100;
    }
    q = signum/10;
    if (q)
    {
        res += q*8;
        signum -= q*10;
    }
    if (signum)
        res += signum;

    return res;
}

void CccModule::paramChange()
{
    int i;
    QVariant val;
    char id[128];
    uint32_t sigLen;
    uint8_t *sigData;
    QByteArray ba;
    bool setPalette;

    // check to see if any signatures have changed
    for (i=0, setPalette=false; i<CL_NUM_SIGNATURES; i++)
    {
        sprintf(id, "signature%d", i+1);
        if (pixyParameterChanged(id, &val))
        {
            ba = val.toByteArray();
            Chirp::deserialize((uint8_t *)ba.data(), val.toByteArray().size(), &sigLen, &sigData, END);
            if (sigLen==sizeof(ColorSignature))
            {
                memcpy(m_signatures+i, sigData, sizeof(ColorSignature));
                m_palette[i] = m_signatures[i].m_rgb;
                setPalette = true;
            }
        }
    }
    if (setPalette)
        m_renderer->setPalette(m_palette);

    // create label dictionary
    Parameters &params = m_interpreter->m_pixyParameters.parameters();
    QStringList words;
    uint32_t signum;
    m_labels.clear();
    // go through all parameters and find labels
    for (i=0; i<params.length(); i++)
    {
        if (params[i].id().startsWith("Signature label"))
        {
            words = params[i].id().split(QRegExp("\\s+"));
            if (words.length()<3) // bogus!
                continue;
            signum = words[2].toUInt();
            m_labels.push_back(QPair<uint16_t, QString>(convert10to8(signum), params[i].value().toString().remove(QRegExp("^\\s+")))); // remove leading whitespace
        }
    }
}


QString CccModule::lookup(uint16_t signum)
{
    int i;

    for (i=0; i<m_labels.length(); i++)
    {
        if (m_labels[i].first==signum)
            return m_labels[i].second;
    }
    return "";
}



void CccModule::renderCCQF(uint8_t renderFlags, uint16_t width, uint16_t height)
{
    if (renderFlags&RENDER_FLAG_START)
        m_numQvals = 0;
    else
        m_renderer->renderCCQ1(renderFlags, width, height, m_numQvals, m_qvals);
}

void CccModule::renderCCQS(uint32_t numVals, uint32_t *vals)
{
    uint32_t i;

    for (i=0; i<numVals && m_numQvals<0x8000; i++)
        m_qvals[m_numQvals++] = vals[i];
}

int CccModule::renderCCB1(uint8_t renderFlags, uint16_t width, uint16_t height, uint32_t numBlobs, uint8_t *blobs)
{
    float scale = (float)m_renderer->m_video->activeWidth()/width;
    QImage img(width*scale, height*scale, QImage::Format_ARGB32);

    if (renderFlags&RENDER_FLAG_BLEND) // if we're blending, we should be transparent
        img.fill(0x00000000);
    else
        img.fill(0xff000000); // otherwise, we're just black

    numBlobs /= sizeof(BlobC);
    renderBlobsC(renderFlags&RENDER_FLAG_BLEND, &img, scale, (BlobC *)blobs, numBlobs);

    m_renderer->emitImage(img, renderFlags, "CCC Blobs");

    return 0;
}

int CccModule::renderCCB2(uint8_t renderFlags, uint16_t width, uint16_t height, uint32_t numBlobs, uint16_t *blobs, uint32_t numCCBlobs, uint16_t *ccBlobs)
{
    float scale = (float)m_renderer->m_video->activeWidth()/width;
    QImage img(width*scale, height*scale, QImage::Format_ARGB32);

    if (renderFlags&RENDER_FLAG_BLEND) // if we're blending, we should be transparent
        img.fill(0x00000000);
    else
        img.fill(0xff000000); // otherwise, we're just black

    numBlobs /= sizeof(BlobA2)/sizeof(uint16_t);
    numCCBlobs /= sizeof(BlobA2)/sizeof(uint16_t);
    renderBlobsA(renderFlags&RENDER_FLAG_BLEND, &img, scale, (BlobA2 *)blobs, numBlobs);
    renderBlobsA(renderFlags&RENDER_FLAG_BLEND, &img, scale, (BlobA2 *)ccBlobs, numCCBlobs);

    m_renderer->emitImage(img, renderFlags, "CCC Blobs");

    return 0;
}

void CccModule::renderBlobsA(bool blend, QImage *image, float scale, BlobA2 *blobs, uint32_t numBlobs)
{
    QPainter p;
    QString str, modelStr;
    uint x, y, w, h, i;

    if (!p.begin(image))
        return;

    for (i=0; i<numBlobs; i++)
    {
        if (blobs[i].m_model==0)
            continue;
        x = scale*blobs[i].m_left;
        y = scale*blobs[i].m_top;
        w = scale*blobs[i].m_right - x;
        h = scale*blobs[i].m_bottom - y;

        //DBG("%d %d %d %d", left, right, top, bottom);
        if (blend || blobs[i].m_model>CL_NUM_SIGNATURES+1)
            Renderer::drawRect(&p, QRect(x, y, w, h), QColor(Qt::white), 0x40);
        else
            Renderer::drawRect(&p, QRect(x, y, w, h), m_palette[blobs[i].m_model-1], 0xff);
        // color code
        if (blobs[i].m_model>CL_NUM_SIGNATURES+1)
        {
            if ((str=lookup(blobs[i].m_model))=="")
            {
                modelStr = QString::number(blobs[i].m_model, 8);
                str = "s=" + modelStr + ", " + QChar(0xa6, 0x03) + "=" + "0";//QString::number(blobs[i].m_angle);
            }
            else
                str += QString(", ") + QChar(0xa6, 0x03) + "=" + "0"; //QString::number(blobs[i].m_angle);

        }
        else if ((str=lookup(blobs[i].m_model))=="")
            str = str.sprintf("s=%d", blobs[i].m_model);

       Renderer::drawText(&p, x+w/2, y+h/2, str);
    }
    p.end();
}

void CccModule::renderBlobsC(bool blend, QImage *image, float scale, BlobC *blobs, uint32_t numBlobs)
{
    QPainter p;
    QString str, modelStr;
    uint x, y, w, h, i;

    if (!p.begin(image))
        return;

    for (i=0; i<numBlobs; i++)
    {
        if (blobs[i].m_model==0)
            continue;
        w = scale*blobs[i].m_width;
        h = scale*blobs[i].m_height;
        x = scale*blobs[i].m_x-w/2;
        y = scale*blobs[i].m_y-h/2;

        //DBG("%d %d %d %d", left, right, top, bottom);
        if (blend || blobs[i].m_model>CL_NUM_SIGNATURES+1)
            Renderer::drawRect(&p, QRect(x, y, w, h), QColor(Qt::white), 0x40);
        else
            Renderer::drawRect(&p, QRect(x, y, w, h), m_palette[blobs[i].m_model-1], 0xff);
        // color code
        if (blobs[i].m_model>CL_NUM_SIGNATURES+1)
        {
            if ((str=lookup(blobs[i].m_model))=="")
            {
                modelStr = QString::number(blobs[i].m_model, 8);
                str = "s=" + modelStr + ", " + QChar(0xa6, 0x03) + "=" + QString::number(blobs[i].m_angle);
            }
            else
                str += QString(", ") + QChar(0xa6, 0x03) + "=" + QString::number(blobs[i].m_angle);

        }
        else if ((str=lookup(blobs[i].m_model))=="")
            str = str.sprintf("s=%d", blobs[i].m_model);

       Renderer::drawText(&p, x+w/2, y+h/2, str);
    }
    p.end();
}

