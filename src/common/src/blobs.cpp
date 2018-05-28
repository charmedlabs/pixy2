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

#include "pixy_init.h"
#include "misc.h"
#include "cameravals.h"
#include "chirp.hpp"


#include "blobs.h"

#define CC_SIGNATURE(s) (m_ccMode==CC_ONLY || m_clut.getType(s)==CL_MODEL_TYPE_COLORCODE)

Blobs::Blobs(Qqueue *qq, uint8_t *lut) : m_clut(lut)
{
    int i;

    m_mutex = false;
    m_minArea = MIN_AREA;
    m_maxBlobs = MAX_BLOBS;
    m_maxBlobsPerModel = MAX_BLOBS_PER_MODEL;  
    m_maxBlob = NULL;

	m_qq = qq;
    m_maxCodedDist = MAX_CODED_DIST;
    m_mergeDist = MAX_MERGE_DIST;
	m_qvals = NULL;

    m_ccMode = DISABLED;

    m_blobs = new (std::nothrow) BlobA[MAX_BLOBS];
    m_numBlobs = 0;
	m_numCCBlobs = 0;
    m_blobReadIndex = 0;
	m_timer = 0;
	
	m_sendDetectedPixels = false;

	m_blobTrackerIndex = 0;
	setBlobFiltering(BL_BLOB_FILTERING);
	setMaxBlobVelocity(BL_MAX_TRACKING_DIST);
	
    // reset blob assemblers
    for (i=0; i<CL_NUM_SIGNATURES; i++)
        m_assembler[i].Reset();
}

void Blobs::reset()
{
	m_blobTrackersList.clear();
	m_timer = 0;
}

void Blobs::sendDetectedPixels(bool send)
{
	m_sendDetectedPixels = send;
}

void Blobs::setMaxBlobs(uint16_t maxBlobs)
{
    if (maxBlobs<=MAX_BLOBS)
        m_maxBlobs = maxBlobs;
    else
        m_maxBlobs = MAX_BLOBS;
}

void Blobs::setMaxBlobsPerModel(uint16_t maxBlobsPerModel)
{
    m_maxBlobsPerModel = maxBlobsPerModel;
}

void Blobs::setMinArea(uint32_t minArea)
{
    m_minArea = minArea;
}

void Blobs::setColorCodeMode(ColorCodeMode ccMode)
{
    m_ccMode = ccMode;
}

void Blobs::setMaxMergeDist(uint16_t maxMergeDist)
{
	m_mergeDist = maxMergeDist;
}

Blobs::~Blobs()
{
    delete [] m_blobs;
}

void Blobs::sendQvals()
{
	CRP_SEND_XDATA(g_chirpUsb, HTYPE(FOURCC('C','C','Q','S')), UINTS32(m_numQvals, m_qvals), END);
	m_numQvals = 0;
}

void Blobs::addQval(uint32_t qval)
{
	if (m_sendDetectedPixels && m_qvals)
	{
		m_qvals[m_numQvals++] = qval;
		if (m_numQvals>=TEMP_QVAL_ARRAY_SIZE)
			sendQvals();
	}
}

int Blobs::handleSegment(uint8_t signature, uint16_t row, uint16_t startCol, uint16_t length)
{
	SSegment s;

    s.model = signature;
    s.row = row;
    s.startCol = startCol;
    s.endCol = startCol+length;

    uint32_t qval;

    qval = signature;
    qval |= startCol<<3;
    qval |= length<<12;

	addQval(qval);
    return m_assembler[signature-1].Add(s);
}

// Blob format:
// 0: model
// 1: left X edge
// 2: right X edge
// 3: top Y edge
// 4: bottom Y edge
int Blobs::runlengthAnalysis()
{
	uint32_t timer;
    int32_t row=-1, icount=0;
    uint32_t startCol, sig, segmentStartCol, segmentEndCol, segmentSig=0;
    Qval qval;
	register int32_t u, v, c;
	int res=0, res2=0;

	if (m_sendDetectedPixels)
	{
		m_qvals = (uint32_t *)malloc(TEMP_QVAL_ARRAY_SIZE*sizeof(uint32_t));
		if (m_qvals==NULL)
			return -3; // out of memory
		CRP_SEND_XDATA(g_chirpUsb, HTYPE(FOURCC('C','C','Q','F')), INT8(RENDER_FLAG_START), UINT16(CAM_RES2_WIDTH/2), UINT16(CAM_RES2_HEIGHT), END);
	}

    m_numQvals = 0;

	setTimer(&timer);
	
    while(1)
    {
        while (m_qq->dequeue(&qval)==0)
		{
			if (getTimer(timer)>100000) // shouldn't take more than 100ms
			{
				printf("to\n");
				res2 = -2; // timeout
				goto end;
			}
		}
        if (qval.m_col>=0xfffe)
		{
			if (qval.m_col==0xfffe) // error code, queue overrun
				res2 = -1; // queue overrun 
            goto end;
		}
		if (res<0)
			continue;
        if (qval.m_col==0)
        {
            if (segmentSig)
            {
                res = handleSegment(segmentSig, row, segmentStartCol-1, segmentEndCol - segmentStartCol+1);
                segmentSig = 0;
            }
            row++;
			addQval(0);
			if (icount++==5) // an interleave of every 5 lines or about every 175us seems good
			{
				g_chirpUsb->service();
				icount = 0;
			}
            continue;
        }

        sig = qval.m_col&0x07;

        u = qval.m_u;
        v = qval.m_v;

        u <<= CL_LUT_ENTRY_SCALE;
        v <<= CL_LUT_ENTRY_SCALE;
        c = qval.m_y;
        if (c==0)
            c = 1;
        u /= c;
        v /= c;

        if (m_clut.m_runtimeSigs[sig-1].m_uMin<u && u<m_clut.m_runtimeSigs[sig-1].m_uMax &&
                m_clut.m_runtimeSigs[sig-1].m_vMin<v && v<m_clut.m_runtimeSigs[sig-1].m_vMax && c>=(int32_t)m_clut.m_miny)
        {
         	qval.m_col >>= 3;
        	startCol = qval.m_col;

			if (segmentSig==0)
            {
                segmentSig = sig;
                segmentStartCol = startCol;
				segmentEndCol = startCol+1;
            }
			else if (segmentSig==sig)
			{
				if (startCol-segmentEndCol<=5)
					segmentEndCol = startCol+1;
				else
				{
					res = handleSegment(segmentSig, row, segmentStartCol, segmentEndCol - segmentStartCol);
					segmentStartCol = startCol;
					segmentEndCol = startCol+1;
				}
			}		
			else // segmentSig!=sig
			{
				if (startCol-segmentEndCol<=5)
					segmentEndCol = startCol;
                res = handleSegment(segmentSig, row, segmentStartCol, segmentEndCol - segmentStartCol);
                segmentSig = sig;
                segmentStartCol = startCol;
				segmentEndCol = startCol+1;
			}
        }
    }
	end:
	if (m_sendDetectedPixels)
	{
		sendQvals();
		CRP_SEND_XDATA(g_chirpUsb, HTYPE(FOURCC('C','C','Q','F')), INT8(RENDER_FLAG_BLEND), UINT16(CAM_RES2_WIDTH/2), UINT16(CAM_RES2_HEIGHT), END);
		free(m_qvals);
		m_qvals = NULL;
	}
	endFrame();
	
	return res2;
}

int Blobs::blobify()
{
    uint32_t i, k;
    bool colorCode;
    CBlob *blob;
    BlobA *blobsStart;
    uint16_t numBlobsStart, invalid, invalid2;
    uint16_t left, top, right, bottom;
    //uint32_t timer, timer2=0;

	if (runlengthAnalysis()<0)
	{
   	 	for (i=0; i<CL_NUM_SIGNATURES; i++)
        	m_assembler[i].Reset();
    	m_numBlobs = 0;
		m_numCCBlobs = 0;
		return -1;
	}

    // copy blobs into memory
    invalid = 0;
    // mutex keeps interrupt routine from stepping on us
    m_mutex = true;

    m_maxBlob = NULL;

    for (i=0, m_numBlobs=0, m_numCCBlobs=0; i<CL_NUM_SIGNATURES; i++)
    {
        colorCode = CC_SIGNATURE(i+1);

        for (k=0, blobsStart=m_blobs+m_numBlobs, numBlobsStart=m_numBlobs, blob=m_assembler[i].finishedBlobs;
             blob && m_numBlobs<m_maxBlobs && k<m_maxBlobsPerModel; blob=blob->next, k++)
        {
            if ((colorCode && blob->GetArea()<MIN_COLOR_CODE_AREA) ||
                (!colorCode && blob->GetArea()<(int)m_minArea))
                continue;
            blob->getBBox((short &)left, (short &)top, (short &)right, (short &)bottom);
            if (bottom-top<=1) // blobs that are 1 line tall
                continue;
            m_blobs[m_numBlobs].m_model = i+1;
            m_blobs[m_numBlobs].m_left = left<<1;
            m_blobs[m_numBlobs].m_right = right<<1;
            m_blobs[m_numBlobs].m_top = top;
            m_blobs[m_numBlobs].m_bottom = bottom;
            m_numBlobs++;
        }
        //setTimer(&timer);
        if (!colorCode) // do not combine color code models
        {
            while(1)
            {
                invalid2 = combine2(blobsStart, m_numBlobs-numBlobsStart);
                if (invalid2==0)
                    break;
                invalid += invalid2;
            }
        }
        //timer2 += getTimer(timer);
    }
    //setTimer(&timer);
    invalid += combine(m_blobs, m_numBlobs);
    if (m_ccMode!=DISABLED)
    {
        m_ccBlobs = m_blobs + m_numBlobs;
        // calculate number of codedblobs left
        processCC();
    }
	// remove empty blobs -- note blobs can be made empty by CC algorithm
    if (invalid || m_ccMode!=DISABLED)
    {
        invalid2 = compress(m_blobs, m_numBlobs);
        m_numBlobs -= invalid2;
		// shift cc blobs down
		shift();
   }
    //timer2 += getTimer(timer);
    //cprintf("time=%d\n", timer2); // never seen this greater than 200us.  or 1% of frame period

    // reset read index-- new frame
    m_blobReadIndex = 0;

	handleBlobTracking();
    m_mutex = false;

    // free memory
    for (i=0; i<CL_NUM_SIGNATURES; i++)
        m_assembler[i].Reset();

#if 0
    static int frame = 0;
    if (m_numBlobs>0)
        cprintf("%d: blobs %d %d %d %d %d\n", frame, m_numBlobs, m_blobs[1], m_blobs[2], m_blobs[3], m_blobs[4]);
    else
        cprintf("%d: blobs 0\n", frame);
    frame++;
#endif
	return 0;
}


uint16_t Blobs::getBlock(uint8_t *buf, uint32_t buflen)
{
    uint16_t *buf16 = (uint16_t *)buf;
    uint16_t temp, width, height;
    uint16_t checksum;
    uint16_t len = 7;  // default
	
    if (buflen<9*sizeof(uint16_t))
        return 0;

    if (m_mutex) // we're copying, so no blocks for now....
		return 0;

    if (m_blobReadIndex==0)	// beginning of frame, mark it with empty block
    {
		reloadBlobs();
        buf16[0] = BL_BEGIN_MARKER;
        len++;
        buf16++;
    }

	if (m_blobReadIndex>=m_numBlobs)
		return 0;
	
	buf16[0] = BL_BEGIN_MARKER;
		
    // model
    temp = m_blobs[m_blobReadIndex].m_model;
    checksum = temp;
    buf16[2] = temp;

    // width
    width = m_blobs[m_blobReadIndex].m_right - m_blobs[m_blobReadIndex].m_left;
    checksum += width;
    buf16[5] = width;

    // height
    height = m_blobs[m_blobReadIndex].m_bottom - m_blobs[m_blobReadIndex].m_top;
    checksum += height;
    buf16[6] = height;

    // x center
    temp = m_blobs[m_blobReadIndex].m_left + width/2;
    checksum += temp;
    buf16[3] = temp;

    // y center
    temp = m_blobs[m_blobReadIndex].m_top + height/2;
    checksum += temp;
    buf16[4] = temp;

	// deal with color code
	if (m_blobs[m_blobReadIndex].m_model>CL_NUM_SIGNATURES)
	{
		buf16[0] = BL_BEGIN_MARKER_CC; // use color code start code instead
		temp = m_ccBlobs[m_blobReadIndex].m_angle;
		checksum += temp;
		buf16[7] = temp;
		len++; // extend length to include angle
	}
	
    buf16[1] = checksum;

    // next blob
    m_blobReadIndex++;

    return len*sizeof(uint16_t);
}


BlobA *Blobs::getMaxBlob(uint16_t signature, uint16_t *numBlobs)
{
	SimpleListNode<Tracker<BlobA> > *i;
	
	uint16_t blobs;
    uint32_t area, maxArea;
    BlobA *blob, *maxBlob;

	if (m_mutex)
		return (BlobA *)-1;	 // busy!

    if (signature==0) // 0 means return the biggest regardless of signature number
    {
		if (numBlobs)
			*numBlobs = 1; // not really used in this mode, so return 1

        // if we've already found it, return it
        if (m_maxBlob)
            return m_maxBlob;

        // look through all blobs looking for the blob with the biggest area
        for (i=m_blobTrackersList.m_first, maxArea=0; i!=NULL; i=i->m_next)
        {
			blob = i->m_object.get();
			if (blob)
			{
				area = (blob->m_right - blob->m_left)*(blob->m_bottom - blob->m_top);
				if (area>maxArea)
				{
					maxArea = area;
					m_maxBlob = blob;
				}
			}
        }
		return m_maxBlob;
    }
	// look for a specific signature
    else
    {
		for (i=m_blobTrackersList.m_first, blobs=0, maxArea=0, maxBlob=NULL; i!=NULL; i=i->m_next)
		{
			blob = i->m_object.get();
			if (blob && blob->m_model==signature)
			{
				area = (blob->m_right - blob->m_left)*(blob->m_bottom - blob->m_top);
				if (area>maxArea)
				{
					maxArea = area;
                	maxBlob = blob;
					blobs++;
				}
			}
		}
		// return number of counted blobs that match signature
		if (numBlobs)
			*numBlobs = blobs;
		
		return maxBlob;
    }
}


int compAreaBlobC(const void *a, const void *b)
{
	BlobC *ba=(BlobC *)a, *bb=(BlobC *)b;
	// calc area of both blobs, return difference
	return bb->m_width*bb->m_height - ba->m_width*ba->m_height;
}


int Blobs::getBlobs(uint8_t sigmap, uint8_t n, uint8_t *buf, uint16_t len)
{
	BlobC *retBlobs;
	BlobA *blob;
	uint16_t bi;
	uint8_t sigbit;
	SimpleListNode<Tracker<BlobA> > *i;
	
	// if we're copying blobs over (m_mutex!=0), or if we've already "gotBlobs" (m_blobReadIndex!=0), return error
	if (m_mutex || m_blobReadIndex)
		return -1;
	
	retBlobs = (BlobC *)buf;
	len /= sizeof(BlobC);
	for (i=m_blobTrackersList.m_first, bi=0; i!=NULL && bi<len; i=i->m_next)
	{
		blob = i->m_object.get();
		if (blob)
		{
			sigbit = (1<<(blob->m_model-1));
			if ((blob->m_model>CL_NUM_SIGNATURES && sigmap&0x80) || sigbit&sigmap)
			{
				convertBlob(&retBlobs[bi], *blob);
				retBlobs[bi].m_index = i->m_object.m_index;
				retBlobs[bi].m_age = i->m_object.m_age;
				bi++;
			}
		}
	}

	// sort blobs by area
	qsort(retBlobs, bi, sizeof(BlobC), compAreaBlobC);
	m_blobReadIndex = 1; // flag that we "gotBlobs"
	
	// note, we need to create a decently-long list so we can sort (above) and then return the n biggest
	// entries which will be at the top of the list
	if (n<bi)
		return n*sizeof(BlobC);
	else
		return bi*sizeof(BlobC);		
}

SimpleList<Tracker<BlobA> > *Blobs::getBlobs()
{
	return &m_blobTrackersList;
}

uint16_t Blobs::compress(BlobA *blobs, uint16_t numBlobs)
{
    uint16_t i, invalid;
    BlobA *destination;

    // compress list
    for (i=0, destination=NULL, invalid=0; i<numBlobs; i++)
    {
        if (blobs[i].m_model==0)
        {
            if (destination==NULL)
                destination = blobs+i;
            invalid++;
            continue;
        }
        if (destination)
		{
			*destination = blobs[i];
            destination++;
        }
    }
    return invalid;
}

void Blobs::shift()
{
	uint16_t i;
	
	if (m_numCCBlobs==0 || m_blobs+m_numBlobs==m_ccBlobs)
		return;
	
	for (i=0; i<m_numCCBlobs; i++)
		m_blobs[m_numBlobs+i] = m_ccBlobs[i];
	
	m_ccBlobs = m_blobs+m_numBlobs;
}


uint16_t Blobs::combine(BlobA *blobs, uint16_t numBlobs)
{
    uint16_t i, j, left0, right0, top0, bottom0;
    uint16_t left, right, top, bottom;
    uint16_t invalid;

    // delete blobs that are fully enclosed by larger blobs
    for (i=0, invalid=0; i<numBlobs; i++)
    {
        if (blobs[i].m_model==0)
            continue;
        left0 = blobs[i].m_left;
        right0 = blobs[i].m_right;
        top0 = blobs[i].m_top;
        bottom0 = blobs[i].m_bottom;

        for (j=i+1; j<numBlobs; j++)
        {
            if (blobs[j].m_model==0)
                continue;
            left = blobs[j].m_left;
            right = blobs[j].m_right;
            top = blobs[j].m_top;
            bottom = blobs[j].m_bottom;

            if (left0<=left && right0>=right && top0<=top && bottom0>=bottom)
            {
                blobs[j].m_model = 0; // invalidate
                invalid++;
            }
            else if (left<=left0 && right>=right0 && top<=top0 && bottom>=bottom0)
            {
                blobs[i].m_model = 0; // invalidate
                invalid++;
            }
        }
    }

    return invalid;
}

uint16_t Blobs::merge(uint16_t *M1, uint16_t *A0, uint16_t *B0, uint16_t *C0, uint16_t *D0,
           uint16_t *A1, uint16_t *B1, uint16_t *C1, uint16_t *D1)
{
    bool c0, c1, c2, c3;
    uint16_t invalid = 0;

    c0 = (*A0 <= *B1 && *B1 <= *B0) || (*A0 >= *B1 && *A0 - *B1 <= m_mergeDist);
    c1 = *A0 >= *A1;
    if (c0 && c1)
    {
        c2 = *C0 <= *C1 && *C1 <= *D0;
        c3 = *C0 <= *D1 && *D1 <= *D0;
        if (c2 && !c3)
        {
            *A0 = *A1;
            *D0 = *D1;
            *M1 = 0;  /* invalidate */
            invalid++;
        }
        else if (!c2 && c3)
        {
            *A0 = *A1;
            *C0 = *C1;
            *M1 = 0;  /* invalidate */
            invalid++;
        }
        else if (c2 && c3)
        {
            *A0 = *A1;
            *M1 = 0;  /* invalidate */
            invalid++;
        }
        else
        {
            c2 = *C1 <= *C0 && *C0 <= *D1;
            c3 = *C1 <= *D0 && *D0 <= *D1;
            if (c2 && c3)
            {
                *A0 = *A1;
                *C0 = *C1;
                *D0 = *D1;
                *M1 = 0;  /* invalidate */
                invalid++;
            }
        }
    }
    else
    {
        c0 = (*A1 <= *B0 && *B0 <= *B1) || (*A1 >= *B0 && *A1 - *B0 <= m_mergeDist);
        c1 = *A1 >= *A0;
        if (c0 && c1)
        {
            c2 = *C1 <= *C0 && *C0 <= *D1;
            c3 = *C1 <= *D0 && *D0 <= *D1;
            if (c2 && !c3)
            {
                *B0 = *B1;
                *C0 = *C1;
                *M1 = 0;  /* invalidate */
                invalid++;
            }
            else if (!c2 && c3)
            {
                *B0 = *B1;
                *D0 = *D1;
                *M1 = 0;  /* invalidate */
                invalid++;
            }
            else if (c2 && c3)
            {
                *B0 = *B1;
                *C0 = *C1;
                *D0 = *D1;
                *M1 = 0;  /* invalidate */
                invalid++;
            }
            else
            {
                c2 = *C0 <= *C1 && *C1 <= *D0;
                c3 = *C0 <= *D1 && *D1 <= *D0;
                if (c2 && c3)
                {
                    *B0 = *B1;
                    *M1 = 0;  /* invalidate */
                    invalid++;
                }
            }
        }
    }
    return invalid;
}

uint16_t Blobs::combine2(BlobA *blobs, uint16_t numBlobs)
{
    uint16_t i, j, *left0, *right0, *top0, *bottom0;
    uint16_t *left1, *right1, *top1, *bottom1, *m1;
    uint16_t invalid;

    for (i=0, invalid=0; i<numBlobs; i++)
    {
        if (blobs[i].m_model==0)
            continue;
        left0 = &blobs[i].m_left;
        right0 = &blobs[i].m_right;
        top0 = &blobs[i].m_top;
        bottom0 = &blobs[i].m_bottom;

        for (j=i+1; j<numBlobs; j++)
        {
            m1 = &blobs[j].m_model;
            if (*m1==0)
                continue;
            left1 = &blobs[j].m_left;
            right1 = &blobs[j].m_right;
            top1 = &blobs[j].m_top;
            bottom1 = &blobs[j].m_bottom;

            invalid += merge(m1, left0, right0, top0, bottom0, left1, right1, top1, bottom1);
            invalid += merge(m1, top0, bottom0, left0, right0, top1, bottom1, left1, right1);
        }
    }

    return invalid;
}

int16_t Blobs::distance(BlobA *blob0, BlobA *blob1)
{
    int16_t left0, right0, top0, bottom0;
    int16_t left1, right1, top1, bottom1;

    left0 = blob0->m_left;
    right0 = blob0->m_right;
    top0 = blob0->m_top;
    bottom0 = blob0->m_bottom;
    left1 = blob1->m_left;
    right1 = blob1->m_right;
    top1 = blob1->m_top;
    bottom1 = blob1->m_bottom;

    if (left0>=left1 && ((top0<=top1 && top1<=bottom0) || (top0<=bottom1 && (bottom1<=bottom0 || top1<=top0))))
        return left0-right1;

    if (left1>=left0 && ((top0<=top1 && top1<=bottom0) || (top0<=bottom1 && (bottom1<=bottom0 || top1<=top0))))
        return left1-right0;

    if (top0>=top1 && ((left0<=left1 && left1<=right0) || (left0<=right1 && (right1<=right0 || left1<=left0))))
        return top0-bottom1;

    if (top1>=top0 && ((left0<=left1 && left1<=right0) || (left0<=right1 && (right1<=right0 || left1<=left0))))
        return top1-bottom0;

    return 0x7fff; // return a large number
}

bool Blobs::closeby(BlobA *blob0, BlobA *blob1)
{
    // check to see if blobs are invalid or equal
    if (blob0->m_model==0 || blob1->m_model==0 || blob0->m_model==blob1->m_model)
        return false;
    // check to see that the blobs are from color code models.  If they aren't both
    // color code blobs, we return false
    if (!CC_SIGNATURE(blob0->m_model&0x07) || !CC_SIGNATURE(blob1->m_model&0x07))
        return false;

    return distance(blob0, blob1)<=m_maxCodedDist;
}

int16_t Blobs::distance(BlobA *blob0, BlobA *blob1, bool horiz)
{
    int16_t dist;

    if (horiz)
        dist = (blob0->m_right+blob0->m_left)/2 - (blob1->m_right+blob1->m_left)/2;
    else
        dist = (blob0->m_bottom+blob0->m_top)/2 - (blob1->m_bottom+blob1->m_top)/2;

    if (dist<0)
        return -dist;
    else
        return dist;
}

int16_t Blobs::angle(BlobA *blob0, BlobA *blob1)
{
    int acx, acy, bcx, bcy;
    float res;

    acx = (blob0->m_right + blob0->m_left)/2;
    acy = (blob0->m_bottom + blob0->m_top)/2;
    bcx = (blob1->m_right + blob1->m_left)/2;
    bcy = (blob1->m_bottom + blob1->m_top)/2;

    res = atan2((float)(acy-bcy), (float)(bcx-acx))*180/3.1415f;

    return (int16_t)res;
}

void Blobs::sort(BlobA *blobs[], uint16_t len, BlobA *firstBlob, bool horiz)
{
    uint16_t i, td, distances[MAX_COLOR_CODE_MODELS*2];
    bool done;
    BlobA *tb;

    // create list of distances
    for (i=0; i<len && i<MAX_COLOR_CODE_MODELS*2; i++)
        distances[i] = distance(firstBlob, blobs[i], horiz);

    // sort -- note, we only have 5 maximum to sort, so no worries about efficiency
    while(1)
    {
        for (i=1, done=true; i<len && i<MAX_COLOR_CODE_MODELS*2; i++)
        {
            if (distances[i-1]>distances[i])
            {
                // swap distances
                td = distances[i];
                distances[i] = distances[i-1];
                distances[i-1] = td;
                // swap blobs
                tb = blobs[i];
                blobs[i] = blobs[i-1];
                blobs[i-1] = tb;

                done = false;
            }
        }
        if (done)
            break;
    }
}

bool Blobs::analyzeDistances(BlobA *blobs0[], int16_t numBlobs0, BlobA *blobs[], int16_t numBlobs, BlobA **blobA, BlobA **blobB)
{
    bool skip;
    bool result = false;
    int16_t dist, minDist, i, j, k;

    for (i=0, minDist=0x7fff; i<numBlobs0; i++)
    {
        for (j=0; j<numBlobs; j++)
        {
            for (k=0, skip=false; k<numBlobs0; k++)
            {
                if (blobs0[k]==blobs[j] || (blobs0[k]->m_model&0x07)==(blobs[j]->m_model&0x07))
                {
                    skip = true;
                    break;
                }
            }
            if (skip)
                continue;
            dist = distance(blobs0[i], blobs[j]);
            if (dist<minDist)
            {
                minDist = dist;
                *blobA = blobs0[i];
                *blobB = blobs[j];
                result = true;
            }
        }
    }
    return result;
}

#define TOL  800

// impose weak size constraint
void Blobs::cleanup(BlobA *blobs[], int16_t *numBlobs)
{
    int i, j;
    bool set;
    uint16_t maxEqual, numEqual, numNewBlobs;
    BlobA *newBlobs[MAX_COLOR_CODE_MODELS*2];
    uint32_t area0, area1, lowerArea, upperArea, maxEqualArea;

    for (i=0, maxEqual=0, set=false; i<*numBlobs; i++)
    {
        area0 = (blobs[i]->m_right-blobs[i]->m_left) * (blobs[i]->m_bottom-blobs[i]->m_top);
        lowerArea = (area0*100)/(100+TOL);
        upperArea = area0 + (area0*TOL)/100;

        for (j=0, numEqual=0; j<*numBlobs; j++)
        {
            if (i==j)
                continue;
            area1 = (blobs[j]->m_right-blobs[j]->m_left) * (blobs[j]->m_bottom-blobs[j]->m_top);
            if (lowerArea<=area1 && area1<=upperArea)
                numEqual++;
        }
        if (numEqual>maxEqual)
        {
            maxEqual = numEqual;
            maxEqualArea = area0;
            set = true;
        }
    }

    if (!set)
        *numBlobs = 0;

    for (i=0, numNewBlobs=0; i<*numBlobs && numNewBlobs<MAX_COLOR_CODE_MODELS*2; i++)
    {
        area0 = (blobs[i]->m_right-blobs[i]->m_left) * (blobs[i]->m_bottom-blobs[i]->m_top);
        lowerArea = (area0*100)/(100+TOL);
        upperArea = area0 + (area0*TOL)/100;
        if (lowerArea<=maxEqualArea && maxEqualArea<=upperArea)
            newBlobs[numNewBlobs++] = blobs[i];
    }

    // copy new blobs over
    for (i=0; i<numNewBlobs; i++)
        blobs[i] = newBlobs[i];
    *numBlobs = numNewBlobs;
}


// eliminate duplicate and adjacent signatures
void Blobs::cleanup2(BlobA *blobs[], int16_t *numBlobs)
{
    BlobA *newBlobs[MAX_COLOR_CODE_MODELS*2];
    int i, j;
    uint16_t numNewBlobs;
    bool set;

    for (i=0, numNewBlobs=0, set=false; i<*numBlobs && numNewBlobs<MAX_COLOR_CODE_MODELS*2; i=j)
    {
        newBlobs[numNewBlobs++] = blobs[i];
        for (j=i+1; j<*numBlobs; j++)
        {
            if ((blobs[j]->m_model&0x07)==(blobs[i]->m_model&0x07))
                set = true;
            else
                break;
        }
    }
    if (set)
    {
        // copy new blobs over
        for (i=0; i<numNewBlobs; i++)
            blobs[i] = newBlobs[i];
        *numBlobs = numNewBlobs;
    }
}

void Blobs::mergeClumps(uint16_t scount0, uint16_t scount1)
{
    int i;
    BlobA *blobs = (BlobA *)m_blobs;
    for (i=0; i<m_numBlobs; i++)
    {
        if ((blobs[i].m_model&~0x07)==scount1)
            blobs[i].m_model = (blobs[i].m_model&0x07) | scount0;
    }
}

void Blobs::processCC()
{
    int16_t i, j, k;
    uint16_t scount, scount1, count = 0;
    int16_t left, right, top, bottom;
    uint16_t codedModel0, codedModel;
    int32_t width, height, avgWidth, avgHeight;
    BlobA *codedBlob, *endBlobCC;
    BlobA *blob0, *blob1, *endBlob;
    BlobA *blobs[MAX_COLOR_CODE_MODELS*2];

#if 0
    BlobA b0(1, 1, 20, 40, 50);
    BlobA b1(1, 1, 20, 52, 60);
    BlobA b2(1, 1, 20, 62, 70);
    BlobA b3(2, 22, 30, 40, 50);
    BlobA b4(2, 22, 30, 52, 60);
    BlobA b5(3, 32, 40, 40, 50);
    BlobA b6(4, 42, 50, 40, 50);
    BlobA b7(4, 42, 50, 52, 60);
    BlobA b8(6, 22, 30, 52, 60);
    BlobA b9(6, 22, 30, 52, 60);
    BlobA b10(7, 22, 30, 52, 60);

    BlobA *testBlobs[] =
    {
        &b0, &b1, &b2, &b3, &b4, &b5, &b6, &b7 //, &b8, &b9, &b10
    };
    int16_t ntb = 8;
    cleanup(testBlobs, &ntb);
#endif

    endBlob = (BlobA *)m_blobs + m_numBlobs;

    // 1st pass: mark all closeby blobs
    for (blob0=(BlobA *)m_blobs; blob0<endBlob; blob0++)
    {
        for (blob1=(BlobA *)blob0+1; blob1<endBlob; blob1++)
        {
            if (closeby(blob0, blob1))
            {
                if (blob0->m_model<=CL_NUM_SIGNATURES && blob1->m_model<=CL_NUM_SIGNATURES)
                {
                    count++;
                    scount = count<<3;
                    blob0->m_model |= scount;
                    blob1->m_model |= scount;
                }
                else if (blob0->m_model>CL_NUM_SIGNATURES && blob1->m_model<=CL_NUM_SIGNATURES)
                {
                    scount = blob0->m_model & ~0x07;
                    blob1->m_model |= scount;
                }
                else if (blob1->m_model>CL_NUM_SIGNATURES && blob0->m_model<=CL_NUM_SIGNATURES)
                {
                    scount = blob1->m_model & ~0x07;
                    blob0->m_model |= scount;
                }
            }
        }
    }

#if 1
    // 2nd pass: merge blob clumps
    for (blob0=(BlobA *)m_blobs; blob0<endBlob; blob0++)
    {
        if (blob0->m_model<=CL_NUM_SIGNATURES) // skip normal blobs
            continue;
        scount = blob0->m_model&~0x07;
        for (blob1=(BlobA *)blob0+1; blob1<endBlob; blob1++)
        {
            if (blob1->m_model<=CL_NUM_SIGNATURES)
                continue;

            scount1 = blob1->m_model&~0x07;
            if (scount!=scount1 && closeby(blob0, blob1))
                mergeClumps(scount, scount1);
        }
    }
#endif

    // 3rd and final pass, find each blob clean it up and add it to the table
    endBlobCC = m_blobs + MAX_BLOBS;
    for (i=1, codedBlob = m_ccBlobs, m_numCCBlobs=0; i<=count && codedBlob<endBlobCC; i++)
    {
        scount = i<<3;
        // find all blobs with index i
        for (j=0, blob0=(BlobA *)m_blobs; blob0<endBlob && j<MAX_COLOR_CODE_MODELS*2; blob0++)
        {
            if ((blob0->m_model&~0x07)==scount)
                blobs[j++] = blob0;
        }

#if 1
        // cleanup blobs, deal with cases where there are more blobs than models
        cleanup(blobs, &j);
#endif

        if (j<2)
            continue;

        // find left, right, top, bottom of color coded block
        for (k=0, left=right=top=bottom=avgWidth=avgHeight=0; k<j; k++)
        {
            //DBG("* cc %x %d i %d: %d %d %d %d %d", blobs[k], m_numCCBlobs, k, blobs[k]->m_model, blobs[k]->m_left, blobs[k]->m_right, blobs[k]->m_top, blobs[k]->m_bottom);
            if (blobs[left]->m_left > blobs[k]->m_left)
                left = k;
            if (blobs[top]->m_top > blobs[k]->m_top)
                top = k;
            if (blobs[right]->m_right < blobs[k]->m_right)
                right = k;
            if (blobs[bottom]->m_bottom < blobs[k]->m_bottom)
                bottom = k;
            avgWidth += blobs[k]->m_right - blobs[k]->m_left;
            avgHeight += blobs[k]->m_bottom - blobs[k]->m_top;
        }
        avgWidth /= j;
        avgHeight /= j;
        codedBlob->m_left = blobs[left]->m_left;
        codedBlob->m_right = blobs[right]->m_right;
        codedBlob->m_top = blobs[top]->m_top;
        codedBlob->m_bottom = blobs[bottom]->m_bottom;

#if 1
        // is it more horizontal than vertical?
        width = (blobs[right]->m_right - blobs[left]->m_left)*100;
        width /= avgWidth; // scale by average width because our swatches might not be square
        height = (blobs[bottom]->m_bottom - blobs[top]->m_top)*100;
        height /= avgHeight; // scale by average height because our swatches might not be square

        if (width > height)
            sort(blobs, j, blobs[left], true);
        else
            sort(blobs, j, blobs[top], false);

#if 1
        cleanup2(blobs, &j);
        if (j<2)
            continue;
        else if (j>5)
            j = 5;
#endif
        // create new blob, compare the coded models, pick the smaller one
        for (k=0, codedModel0=0; k<j; k++)
        {
            codedModel0 <<= 3;
            codedModel0 |= blobs[k]->m_model&0x07;
        }
        for (k=j-1, codedModel=0; k>=0; k--)
        {
            codedModel <<= 3;
            codedModel |= blobs[k]->m_model&0x07;
            blobs[k]->m_model = 0; // invalidate
        }

        if (codedModel0<codedModel)
        {
            codedBlob->m_model = codedModel0;
            codedBlob->m_angle = angle(blobs[0], blobs[j-1]);
        }
        else
        {
            codedBlob->m_model = codedModel;
            codedBlob->m_angle = angle(blobs[j-1], blobs[0]);
        }
#endif
        //DBG("cc %d %d %d %d %d", m_numCCBlobs, codedBlob->m_left, codedBlob->m_right, codedBlob->m_top, codedBlob->m_bottom);
        codedBlob++;
        m_numCCBlobs++;
    }

    // 3rd pass, invalidate blobs
    for (blob0=(BlobA *)m_blobs; blob0<endBlob; blob0++)
    {
        if (m_ccMode==MIXED)
        {
            if (blob0->m_model>CL_NUM_SIGNATURES)
                blob0->m_model = 0;
        }
        else if (blob0->m_model>CL_NUM_SIGNATURES || CC_SIGNATURE(blob0->m_model))
            blob0->m_model = 0; // invalidate-- not part of a color code
    }
}

void Blobs::endFrame()
{
    int i;
    for (i=0; i<CL_NUM_SIGNATURES; i++)
    {
        m_assembler[i].EndFrame();
        m_assembler[i].SortFinished();
    }
}

uint32_t Blobs::compareBlobs(const BlobA &b0, const BlobA &b1)
{
	int32_t xcenter, ycenter, left, right, top, bottom, vel2;
	
	// different values are different
	if (b0.m_model!=b1.m_model)
		return TR_MAXVAL;
	
	xcenter = (b0.m_left+b0.m_right)>>1;
	ycenter = (b0.m_top+b0.m_bottom)>>1;
	xcenter -= (b1.m_left+b1.m_right)>>1;
	ycenter -= (b1.m_top+b1.m_bottom)>>1;
	
	vel2 = (xcenter*xcenter + ycenter*ycenter)>>1; // calc dist
	vel2 *= BL_PERIOD;
	vel2 /= m_timer; // normalize with how much time has passed to get velocity
	
	if (vel2>m_maxTrackingVel2)
		return TR_MAXVAL; 
	
	// find distance between 
	left = b0.m_left - b1.m_left;
	right = b0.m_right - b1.m_right;
	top = b0.m_top - b1.m_top;
	bottom = b0.m_bottom - b1.m_bottom;
	
	return (left*left + right*right + top*top + bottom*bottom)>>2;
}


uint16_t Blobs::handleBlobTracking2()
{
	uint32_t val, min;
	uint16_t n=0;
	SimpleListNode<Tracker<BlobA> > *i;
	uint8_t j;
	BlobA *minBlob;
	
	// go through list, find best candidates
	for (i=m_blobTrackersList.m_first; i!=NULL; i=i->m_next)
	{
		// already found minimum, continue
		if (i->m_object.m_minVal!=TR_MAXVAL)
			continue;
		
		// find min
		for (j=0, min=TR_MAXVAL, minBlob=NULL; j<m_numBlobs+m_numCCBlobs; j++)
		{
			val = compareBlobs(i->m_object.m_object, m_blobs[j]);
			// find minimum, but if line is already chosen, make sure we're a better match
			if (val<min && i->m_object.swappable(val, &m_blobs[j]))  
			{
				min = val;
				minBlob = &m_blobs[j];
			}
		}
		if (minBlob)
		{
			// if this minimum line already has a tracker, see which is better
			if (minBlob->m_tracker)
			{
				minBlob->m_tracker->resetMin(); // reset tracker pointed to by current minline
				minBlob->m_tracker = &i->m_object; 
				i->m_object.setMin(minBlob, min);
				n++;
			}
			else
			{
				minBlob->m_tracker = &i->m_object; // update tracker pointer			
				i->m_object.setMin(minBlob, min);
				n++;
			}
		}
	}
	return n;
}

void Blobs::handleBlobTracking()
{
	SimpleListNode<Tracker<BlobA> > *i, *inext;
	uint16_t j;
	
	if (m_timer==0)
		m_timer = BL_PERIOD;
	else
		m_timer = getTimer(m_timer);
	
	for (j=0; j<m_numBlobs+m_numCCBlobs; j++)
		m_blobs[j].m_tracker = NULL;
	
	// reset tracking table
	// Note, we don't need to reset g_linesList entries (e.g. m_tracker) because these are renewed  
	for (i=m_blobTrackersList.m_first; i!=NULL; i=i->m_next) 
		i->m_object.resetMin();
	
	// do search, find minimums
	while(handleBlobTracking2());
	
	// go through, update tracker, remove entries that are no longer valid
	for (i=m_blobTrackersList.m_first; i!=NULL; i=inext)
	{
		inext = i->m_next;
		
		if (i->m_object.update()&TR_EVENT_INVALIDATED)
			m_blobTrackersList.remove(i); // no longer valid?  remove from tracker list
	}	
	
	// find new candidates
	for (j=0; j<m_numBlobs+m_numCCBlobs; j++)
	{
		if (m_blobs[j].m_tracker==NULL)
		{
			SimpleListNode<Tracker<BlobA> > *n;
			uint16_t leading, trailing;
			leading = m_blobFiltering*BL_FILTERING_MULTIPLIER;
			trailing = (leading+1)>>1;
			n = m_blobTrackersList.add(Tracker<BlobA>(m_blobs[j], m_blobTrackerIndex++, leading, trailing));
			if (n==NULL)
			{
				cprintf(0, "hlt\n");
				break;
			}
			m_blobs[j].m_tracker = &n->m_object; // point back to tracker
		}
	}
	
	setTimer(&m_timer);
}

int compAreaBlobA(const void *a, const void *b)
{
	BlobA *ba=(BlobA *)a, *bb=(BlobA *)b;
	// calc area of both blobs, return difference
	return (bb->m_right - bb->m_left)*(bb->m_bottom - bb->m_top) - (ba->m_right - ba->m_left)*(ba->m_bottom - ba->m_top);
}

void Blobs::reloadBlobs()
{
	BlobA *blob;
	uint8_t sig;
	uint16_t n; // number of *valid* blocks m_blobTrackersList
	uint16_t bi; // current index into m_blocks
	uint16_t si; // number of blocks for current signature
	SimpleListNode<Tracker<BlobA> > *i;
	
	for (sig=1, bi=0; sig<=CL_NUM_SIGNATURES+1 && bi<MAX_BLOBS; sig++)
	{
		// look through m_blobTrackersList for signature==sig, copy into m_blocks
		for (i=m_blobTrackersList.m_first, n=0, si=0; i!=NULL; i=i->m_next)
		{
			blob = i->m_object.get();
			if (blob)
			{
				n++;
				if (blob->m_model==sig || /*sig==8 is color code case */ (sig==CL_NUM_SIGNATURES+1 && blob->m_model>CL_NUM_SIGNATURES))
				{
					m_blobs[bi++] = *blob;
					si++;
				}
			}
		}
		// sort blocks for that signature, or CC
		qsort(&m_blobs[bi-si], si, sizeof(BlobA), compAreaBlobA);

		// exit early?
		if (bi>=n)
			break;  // we're done
	}
	m_numBlobs = n;
}

void Blobs::setBlobFiltering(uint8_t filtering)
{
	m_blobFiltering = filtering;	
}

void Blobs::setMaxBlobVelocity(uint16_t maxVel)
{
	m_maxTrackingVel2 = maxVel*maxVel;	
}


void Blobs::convertBlob(BlobC *blobc, const BlobA &bloba)
{
	blobc->m_model = bloba.m_model;
	blobc->m_x = (bloba.m_right + bloba.m_left)>>1;
	blobc->m_y = (bloba.m_bottom + bloba.m_top)>>1;
	blobc->m_width = bloba.m_right - bloba.m_left;
	blobc->m_height = bloba.m_bottom - bloba.m_top;
	blobc->m_angle = bloba.m_angle;
}


