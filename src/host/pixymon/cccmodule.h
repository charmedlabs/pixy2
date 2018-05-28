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

#ifndef CCCMODULE_H
#define CCCMODULE_H

#include "monmodule.h"
#include "qqueue.h"
#include "pixytypes.h"


// color connected components
class CccModule : public MonModule
{
public:
    CccModule(Interpreter *interpreter);
    ~CccModule();

    virtual bool render(uint32_t fourcc, const void *args[]);
    virtual bool command(const QStringList &argv);
    virtual void paramChange();

private:

    int renderCCB1(uint8_t renderFlags, uint16_t width, uint16_t height, uint32_t numBlobs, uint8_t *blobs);
    int renderCCB2(uint8_t renderFlags, uint16_t width, uint16_t height, uint32_t numBlobs, uint16_t *blobs, uint32_t numCCBlobs, uint16_t *ccBlobs);
    void renderCCQF(uint8_t renderFlags, uint16_t width, uint16_t height);
    void renderCCQS(uint32_t numVals, uint32_t *vals);
    void renderBlobsA(bool blend, QImage *image, float scale, BlobA2 *blobs, uint32_t numBlobs);
    void renderBlobsC(bool blend, QImage *image, float scale, BlobC *blobs, uint32_t numBlobs);
    void resetBlobs();
    QString lookup(uint16_t signum);

    uint32_t m_crc;
    uint32_t m_palette[CL_NUM_SIGNATURES];
    ColorSignature m_signatures[CL_NUM_SIGNATURES];
    QList<QPair<uint16_t, QString> > m_labels;
    uint32_t m_numQvals;
    uint32_t *m_qvals;

};

#endif // CCCMODULE_H
