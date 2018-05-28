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
#ifndef BLOBS_H
#define BLOBS_H

#include <stdint.h>
#include "blob.h"
#include "pixytypes.h"
#include "colorlut.h"
#include "qqueue.h"
#include "simplelist.h"
#include "tracker.h"

#define MAX_BLOBS             100
#define MAX_BLOBS_PER_MODEL   20
#define MAX_MERGE_DIST        7
#define MIN_AREA              20
#define MIN_COLOR_CODE_AREA   10
#define MAX_CODED_DIST        8
#define MAX_COLOR_CODE_MODELS 5

#define BL_BEGIN_MARKER	      0xaa55
#define BL_BEGIN_MARKER_CC    0xaa56

#define BL_FILTERING_MULTIPLIER    16
#define BL_BLOB_FILTERING          3
#define BL_MAX_TRACKING_DIST       65
#define BL_PERIOD                  16200  // microseconds per frame, assuming 60fps

#define TEMP_QVAL_ARRAY_SIZE  0x100

struct BlobA
{
    BlobA()
    {
        m_model = m_left = m_right = m_top = m_bottom = 0;
		m_tracker = NULL;
    }

    BlobA(uint16_t model, uint16_t left, uint16_t right, uint16_t top, uint16_t bottom)
    {
        m_model = model;
        m_left = left;
        m_right = right;
        m_top = top;
        m_bottom = bottom;
		m_tracker = NULL;
    }

    uint16_t m_model;
    uint16_t m_left;
    uint16_t m_right;
    uint16_t m_top;
    uint16_t m_bottom;
	int16_t m_angle;
	Tracker<BlobA> *m_tracker;
};


enum ColorCodeMode
{
    DISABLED = 0,
    ENABLED = 1,
    CC_ONLY = 2,
    MIXED = 3 // experimental
};

class Blobs
{
public:
    Blobs(Qqueue *qq, uint8_t *lut);
    ~Blobs();
	void reset();
    int blobify();
	void sendDetectedPixels(bool send);
    uint16_t getBlock(uint8_t *buf, uint32_t buflen);
    BlobA *getMaxBlob(uint16_t signature=0, uint16_t *numBlobs=NULL);
	int getBlobs(uint8_t sigmap, uint8_t n, uint8_t *buf, uint16_t len);
	SimpleList<Tracker<BlobA> > *getBlobs();
    int runlengthAnalysis();
	
	void setMaxBlobs(uint16_t maxBlobs);
	void setMaxBlobsPerModel(uint16_t maxBlobsPerModel);
	void setMinArea(uint32_t minArea);
	void setColorCodeMode(ColorCodeMode ccMode);
    void setBlobFiltering(uint8_t filtering);
	void setMaxBlobVelocity(uint16_t maxVel);
	void setMaxMergeDist(uint16_t maxMergeDist);

	ColorLUT m_clut;
    Qqueue *m_qq;

	static void convertBlob(BlobC *blobc, const BlobA &bloba);

private:
    int handleSegment(uint8_t signature, uint16_t row, uint16_t startCol, uint16_t length);
	void addQval(uint32_t qval);
	void sendQvals();
	void endFrame();
    uint16_t combine(BlobA *blobs, uint16_t numBlobs);
    uint16_t combine2(BlobA *blobs, uint16_t numBlobs);
    uint16_t compress(BlobA *blobs, uint16_t numBlobs);
	void shift();

    bool closeby(BlobA *blob0, BlobA *blob1);
    int16_t distance(BlobA *blob0, BlobA *blob1);
    void sort(BlobA *blobs[], uint16_t len, BlobA *firstBlob, bool horiz);
    int16_t angle(BlobA *blob0, BlobA *blob1);
    int16_t distance(BlobA *blob0, BlobA *blob1, bool horiz);
    void processCC();
    void cleanup(BlobA *blobs[], int16_t *numBlobs);
    void cleanup2(BlobA *blobs[], int16_t *numBlobs);
    bool analyzeDistances(BlobA *blobs0[], int16_t numBlobs0, BlobA *blobs[], int16_t numBlobs, BlobA **blobA, BlobA **blobB);
    void mergeClumps(uint16_t scount0, uint16_t scount1);
    uint16_t merge(uint16_t *M1, uint16_t *A0, uint16_t *B0, uint16_t *C0, uint16_t *D0,
               uint16_t *A1, uint16_t *B1, uint16_t *C1, uint16_t *D1);

	uint32_t compareBlobs(const BlobA &b0, const BlobA &b1);
	uint16_t handleBlobTracking2();
	void handleBlobTracking();
	void reloadBlobs();
	
    CBlobAssembler m_assembler[CL_NUM_SIGNATURES];

    BlobA *m_blobs;
    uint16_t m_numBlobs;	
    BlobA *m_ccBlobs;
    uint16_t m_numCCBlobs;

    bool m_mutex;
    uint16_t m_maxBlobs;
    uint16_t m_maxBlobsPerModel;

    uint16_t m_blobReadIndex;

    uint32_t m_minArea;
    uint16_t m_mergeDist;
    uint16_t m_maxCodedDist;
    ColorCodeMode m_ccMode;
    BlobA *m_maxBlob;

    uint32_t m_numQvals;
    uint32_t *m_qvals;

	bool m_sendDetectedPixels;
	
	SimpleList<Tracker<BlobA> > m_blobTrackersList;
	uint8_t m_blobTrackerIndex;
	uint8_t m_blobFiltering;	
	uint32_t m_maxTrackingVel2;
	uint32_t m_timer;
};



#endif // BLOBS_H
