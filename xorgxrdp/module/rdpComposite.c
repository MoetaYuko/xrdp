/*
Copyright 2005-2016 Jay Sorg

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

composite(alpha blending) calls

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* this should be before all X11 .h files */
#include <xorg-server.h>
#include <xorgVersion.h>

/* all driver need this */
#include <xf86.h>
#include <xf86_OSproc.h>

#include "mipict.h"
#include <picture.h>

#include "rdp.h"
#include "rdpDraw.h"
#include "rdpClientCon.h"
#include "rdpReg.h"
#include "rdpComposite.h"

/******************************************************************************/
#define LOG_LEVEL 1
#define LLOGLN(_level, _args) \
    do { if (_level < LOG_LEVEL) { ErrorF _args ; ErrorF("\n"); } } while (0)

/******************************************************************************/
static void
rdpCompositeOrg(PictureScreenPtr ps, rdpPtr dev,
                CARD8 op, PicturePtr pSrc, PicturePtr pMask, PicturePtr pDst,
                INT16 xSrc, INT16 ySrc, INT16 xMask, INT16 yMask,
                INT16 xDst, INT16 yDst, CARD16 width, CARD16 height)
{
    ps->Composite = dev->Composite;
    ps->Composite(op, pSrc, pMask, pDst, xSrc, ySrc, xMask, yMask,
                  xDst, yDst, width, height);
    ps->Composite = rdpComposite;
}

/******************************************************************************/
void
rdpComposite(CARD8 op, PicturePtr pSrc, PicturePtr pMask, PicturePtr pDst,
             INT16 xSrc, INT16 ySrc, INT16 xMask, INT16 yMask, INT16 xDst,
             INT16 yDst, CARD16 width, CARD16 height)
{
    ScreenPtr pScreen;
    rdpPtr dev;
    PictureScreenPtr ps;
    BoxRec box;
    RegionRec reg;

    LLOGLN(10, ("rdpComposite:"));
    pScreen = pDst->pDrawable->pScreen;
    dev = rdpGetDevFromScreen(pScreen);
    dev->counts.rdpCompositeCallCount++;
    box.x1 = xDst + pDst->pDrawable->x;
    box.y1 = yDst + pDst->pDrawable->y;
    box.x2 = box.x1 + width;
    box.y2 = box.y1 + height;
    rdpRegionInit(&reg, &box, 0);
    if (pDst->pCompositeClip != NULL)
    {
        rdpRegionIntersect(&reg, pDst->pCompositeClip, &reg);
    }
    ps = GetPictureScreen(pScreen);
    /* do original call */
    rdpCompositeOrg(ps, dev, op, pSrc, pMask, pDst, xSrc, ySrc,
                    xMask, yMask, xDst, yDst, width, height);
    rdpClientConAddAllReg(dev, &reg, pDst->pDrawable);
    rdpRegionUninit(&reg);
}
