/*
 * $Id$
 *
 * Copyright © 2003 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "xfixesint.h"
#include "scrnintstr.h"
#ifdef RENDER
#include <picturestr.h>
extern int RenderErrBase;
#endif
#include <regionstr.h>
#include <gcstruct.h>
#include <window.h>

RESTYPE	    RegionResType;

static int
RegionResFree (void * data, XID id)
{
    RegionPtr    pRegion = (RegionPtr) data;

    RegionDestroy(pRegion);
    return Success;
}

RegionPtr
XFixesRegionCopy (RegionPtr pRegion)
{
    RegionPtr   pNew = RegionCreate(RegionExtents(pRegion),
				      RegionNumRects(pRegion));
    if (!pNew)
	return 0;
    if (!RegionCopy(pNew, pRegion))
    {
	RegionDestroy(pNew);
	return 0;
    }
    return pNew;
}

Bool
XFixesRegionInit (void)
{
    RegionResType = CreateNewResourceType(RegionResFree);
    return TRUE;
}

int
ProcXFixesCreateRegion (ClientPtr client)
{
    int		things;
    RegionPtr	pRegion;
    REQUEST (xXFixesCreateRegionReq);
    
    REQUEST_AT_LEAST_SIZE(xXFixesCreateRegionReq);
    LEGAL_NEW_RESOURCE (stuff->region, client);
    
    things = (client->req_len << 2) - sizeof (xXFixesCreateRegionReq);
    if (things & 4)
	return BadLength;
    things >>= 3;

    pRegion = RegionFromRects(things, (xRectangle *) (stuff + 1), CT_UNSORTED);
    if (!pRegion)
	return BadAlloc;
    if (!AddResource (stuff->region, RegionResType, (void *) pRegion))
	return BadAlloc;
    
    return(client->noClientException);
}

int
SProcXFixesCreateRegion (ClientPtr client)
{
    REQUEST(xXFixesCreateRegionReq);

    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xXFixesCreateRegionReq);
    swapl(&stuff->region);
    SwapRestS(stuff);
    return (*ProcXFixesVector[stuff->xfixesReqType]) (client);
}

int
ProcXFixesCreateRegionFromBitmap (ClientPtr client)
{
    RegionPtr	pRegion;
    PixmapPtr	pPixmap;
    REQUEST (xXFixesCreateRegionFromBitmapReq);

    REQUEST_SIZE_MATCH (xXFixesCreateRegionFromBitmapReq);
    LEGAL_NEW_RESOURCE (stuff->region, client);

    pPixmap = (PixmapPtr) SecurityLookupIDByType (client, stuff->bitmap,
						  RT_PIXMAP,
						  DixReadAccess);
    if (!pPixmap)
    {
	client->errorValue = stuff->bitmap;
	return BadPixmap;
    }
    if (pPixmap->drawable.depth != 1)
	return BadMatch;

    pRegion = BitmapToRegion(pPixmap->drawable.pScreen, pPixmap);

    if (!pRegion)
	return BadAlloc;
    
    if (!AddResource (stuff->region, RegionResType, (void *) pRegion))
	return BadAlloc;
    
    return(client->noClientException);
}

int
SProcXFixesCreateRegionFromBitmap (ClientPtr client)
{
    REQUEST(xXFixesCreateRegionFromBitmapReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXFixesCreateRegionFromBitmapReq);
    swapl(&stuff->region);
    swapl(&stuff->bitmap);
    return (*ProcXFixesVector[stuff->xfixesReqType]) (client);
}

int
ProcXFixesCreateRegionFromWindow (ClientPtr client)
{
    RegionPtr	pRegion;
    Bool	copy = TRUE;
    WindowPtr	pWin;
    REQUEST (xXFixesCreateRegionFromWindowReq);
    
    REQUEST_SIZE_MATCH (xXFixesCreateRegionFromWindowReq);
    LEGAL_NEW_RESOURCE (stuff->region, client);
    pWin = (WindowPtr) LookupIDByType (stuff->window, RT_WINDOW);
    if (!pWin)
    {
	client->errorValue = stuff->window;
	return BadWindow;
    }
    switch (stuff->kind) {
    case WindowRegionBounding:
#ifdef SHAPE
	pRegion = wBoundingShape(pWin);
	if (!pRegion)
#endif
	{
	    pRegion = CreateBoundingShape (pWin);
	    copy = FALSE;
	}
	break;
    case WindowRegionClip:
#ifdef SHAPE
	pRegion = wClipShape(pWin);
	if (!pRegion)
#endif
	{
	    pRegion = CreateClipShape (pWin);
	    copy = FALSE;
	}
	break;
    default:
	client->errorValue = stuff->kind;
	return BadValue;
    }
    if (copy && pRegion)
	pRegion = XFixesRegionCopy (pRegion);
    if (!pRegion)
	return BadAlloc;
    if (!AddResource (stuff->region, RegionResType, (void *) pRegion))
	return BadAlloc;
    
    return(client->noClientException);
}

int
SProcXFixesCreateRegionFromWindow (ClientPtr client)
{
    REQUEST(xXFixesCreateRegionFromWindowReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXFixesCreateRegionFromWindowReq);
    swapl(&stuff->region);
    swapl(&stuff->window);
    return (*ProcXFixesVector[stuff->xfixesReqType]) (client);
}

int
ProcXFixesCreateRegionFromGC (ClientPtr client)
{
    RegionPtr	pRegion, pClip;
    GCPtr	pGC;
#ifndef NXAGENT_SERVER
    int 	rc;
#endif
    REQUEST (xXFixesCreateRegionFromGCReq);

    REQUEST_SIZE_MATCH (xXFixesCreateRegionFromGCReq);
    LEGAL_NEW_RESOURCE (stuff->region, client);

#ifndef NXAGENT_SERVER
    rc = dixLookupGC(&pGC, stuff->gc, client, DixReadAccess);
    if (rc != Success)
	return rc;
#else
    SECURITY_VERIFY_GC(pGC, stuff->gc, client, DixReadAccess);
#endif
    
    switch (pGC->clientClipType) {
    case CT_PIXMAP:
	pRegion = BitmapToRegion(pGC->pScreen, (PixmapPtr) pGC->clientClip);
	if (!pRegion)
	    return BadAlloc;
	break;
    case CT_REGION:
	pClip = (RegionPtr) pGC->clientClip;
	pRegion = XFixesRegionCopy (pClip);
	if (!pRegion)
	    return BadAlloc;
	break;
    default:
	return BadImplementation;   /* assume sane server bits */
    }
    
    if (!AddResource (stuff->region, RegionResType, (void *) pRegion))
	return BadAlloc;
    
    return(client->noClientException);
}

int
SProcXFixesCreateRegionFromGC (ClientPtr client)
{
    REQUEST(xXFixesCreateRegionFromGCReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXFixesCreateRegionFromGCReq);
    swapl(&stuff->region);
    swapl(&stuff->gc);
    return (*ProcXFixesVector[stuff->xfixesReqType]) (client);
}

int
ProcXFixesCreateRegionFromPicture (ClientPtr client)
{
#ifdef RENDER
    RegionPtr	pRegion;
    PicturePtr	pPicture;
    REQUEST (xXFixesCreateRegionFromPictureReq);

    REQUEST_SIZE_MATCH (xXFixesCreateRegionFromPictureReq);
    LEGAL_NEW_RESOURCE (stuff->region, client);

    VERIFY_PICTURE(pPicture, stuff->picture, client, DixReadAccess,
		   RenderErrBase + BadPicture);
    
    switch (pPicture->clientClipType) {
    case CT_PIXMAP:
	pRegion = BitmapToRegion(pPicture->pDrawable->pScreen,
				   (PixmapPtr) pPicture->clientClip);
	if (!pRegion)
	    return BadAlloc;
	break;
    case CT_REGION:
	pRegion = XFixesRegionCopy ((RegionPtr) pPicture->clientClip);
	if (!pRegion)
	    return BadAlloc;
	break;
    default:
	return BadImplementation;   /* assume sane server bits */
    }
    
    if (!AddResource (stuff->region, RegionResType, (void *) pRegion))
	return BadAlloc;
    
    return(client->noClientException);
#else
    return BadRequest;
#endif
}

int
SProcXFixesCreateRegionFromPicture (ClientPtr client)
{
    REQUEST(xXFixesCreateRegionFromPictureReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXFixesCreateRegionFromPictureReq);
    swapl(&stuff->region);
    swapl(&stuff->picture);
    return (*ProcXFixesVector[stuff->xfixesReqType]) (client);
}

int
ProcXFixesDestroyRegion (ClientPtr client)
{
    REQUEST (xXFixesDestroyRegionReq);
    RegionPtr	pRegion;

    REQUEST_SIZE_MATCH(xXFixesDestroyRegionReq);
    VERIFY_REGION(pRegion, stuff->region, client, DixWriteAccess);
    FreeResource (stuff->region, RT_NONE);
    return(client->noClientException);
}

int
SProcXFixesDestroyRegion (ClientPtr client)
{
    REQUEST(xXFixesDestroyRegionReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXFixesDestroyRegionReq);
    swapl(&stuff->region);
    return (*ProcXFixesVector[stuff->xfixesReqType]) (client);
}

int
ProcXFixesSetRegion (ClientPtr client)
{
    int		things;
    RegionPtr	pRegion, pNew;
    REQUEST (xXFixesSetRegionReq);

    REQUEST_AT_LEAST_SIZE(xXFixesSetRegionReq);
    VERIFY_REGION(pRegion, stuff->region, client, DixWriteAccess);
    
    things = (client->req_len << 2) - sizeof (xXFixesCreateRegionReq);
    if (things & 4)
	return BadLength;
    things >>= 3;

    pNew = RegionFromRects(things, (xRectangle *) (stuff + 1), CT_UNSORTED);
    if (!pNew)
	return BadAlloc;
    if (!RegionCopy(pRegion, pNew))
    {
	RegionDestroy(pNew);
	return BadAlloc;
    }
    RegionDestroy(pNew);
    return(client->noClientException);
}

int
SProcXFixesSetRegion (ClientPtr client)
{
    REQUEST(xXFixesSetRegionReq);

    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xXFixesSetRegionReq);
    swapl(&stuff->region);
    SwapRestS(stuff);
    return (*ProcXFixesVector[stuff->xfixesReqType]) (client);
}

int
ProcXFixesCopyRegion (ClientPtr client)
{
    RegionPtr	pSource, pDestination;
    REQUEST (xXFixesCopyRegionReq);
    
    VERIFY_REGION(pSource, stuff->source, client, DixReadAccess);
    VERIFY_REGION(pDestination, stuff->destination, client, DixWriteAccess);
    
    if (!RegionCopy(pDestination, pSource))
	return BadAlloc;

    return(client->noClientException);
}

int
SProcXFixesCopyRegion (ClientPtr client)
{
    REQUEST(xXFixesCopyRegionReq);

    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xXFixesCopyRegionReq);
    swapl(&stuff->source);
    swapl(&stuff->destination);
    return (*ProcXFixesVector[stuff->xfixesReqType]) (client);
}

int
ProcXFixesCombineRegion (ClientPtr client)
{
    RegionPtr	pSource1, pSource2, pDestination;
    int		ret = Success;
    REQUEST (xXFixesCombineRegionReq);

    REQUEST_SIZE_MATCH (xXFixesCombineRegionReq);
    VERIFY_REGION(pSource1, stuff->source1, client, DixReadAccess);
    VERIFY_REGION(pSource2, stuff->source2, client, DixReadAccess);
    VERIFY_REGION(pDestination, stuff->destination, client, DixWriteAccess);
    
    switch (stuff->xfixesReqType) {
    case X_XFixesUnionRegion:
	if (!RegionUnion(pDestination, pSource1, pSource2))
	    ret = BadAlloc;
	break;
    case X_XFixesIntersectRegion:
	if (!RegionIntersect(pDestination, pSource1, pSource2))
	    ret = BadAlloc;
	break;
    case X_XFixesSubtractRegion:
	if (!RegionSubtract(pDestination, pSource1, pSource2))
	    ret = BadAlloc;
	break;
    }
    
    if (ret == Success) 
	ret = client->noClientException;
    return ret;
}

int
SProcXFixesCombineRegion (ClientPtr client)
{
    REQUEST(xXFixesCombineRegionReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXFixesCombineRegionReq);
    swapl(&stuff->source1);
    swapl(&stuff->source2);
    swapl(&stuff->destination);
    return (*ProcXFixesVector[stuff->xfixesReqType]) (client);
}

int
ProcXFixesInvertRegion (ClientPtr client)
{
    RegionPtr	pSource, pDestination;
    BoxRec	bounds;
    int		ret = Success;
    REQUEST(xXFixesInvertRegionReq);

    REQUEST_SIZE_MATCH(xXFixesInvertRegionReq);
    VERIFY_REGION(pSource, stuff->source, client, DixReadAccess);
    VERIFY_REGION(pDestination, stuff->destination, client, DixWriteAccess);

    /* Compute bounds, limit to 16 bits */
    bounds.x1 = stuff->x;
    bounds.y1 = stuff->y;
    if ((int) stuff->x + (int) stuff->width > MAXSHORT)
	bounds.x2 = MAXSHORT;
    else
	bounds.x2 = stuff->x + stuff->width;
    
    if ((int) stuff->y + (int) stuff->height > MAXSHORT)
	bounds.y2 = MAXSHORT;
    else
	bounds.y2 = stuff->y + stuff->height;

    if (!RegionInverse(pDestination, pSource, &bounds))
	ret = BadAlloc;

    if (ret == Success)
	ret = client->noClientException;
    return ret;
}

int
SProcXFixesInvertRegion (ClientPtr client)
{
    REQUEST(xXFixesInvertRegionReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXFixesInvertRegionReq);
    swapl(&stuff->source);
    swaps(&stuff->x);
    swaps(&stuff->y);
    swaps(&stuff->width);
    swaps(&stuff->height);
    swapl(&stuff->destination);
    return (*ProcXFixesVector[stuff->xfixesReqType]) (client);
}

int
ProcXFixesTranslateRegion (ClientPtr client)
{
    RegionPtr	pRegion;
    REQUEST(xXFixesTranslateRegionReq);

    REQUEST_SIZE_MATCH(xXFixesTranslateRegionReq);
    VERIFY_REGION(pRegion, stuff->region, client, DixWriteAccess);

    RegionTranslate(pRegion, stuff->dx, stuff->dy);
    return (client->noClientException);
}

int
SProcXFixesTranslateRegion (ClientPtr client)
{
    REQUEST(xXFixesTranslateRegionReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXFixesTranslateRegionReq);
    swapl(&stuff->region);
    swaps(&stuff->dx);
    swaps(&stuff->dy);
    return (*ProcXFixesVector[stuff->xfixesReqType]) (client);
}

int
ProcXFixesRegionExtents (ClientPtr client)
{
    RegionPtr	pSource, pDestination;
    REQUEST(xXFixesRegionExtentsReq);

    REQUEST_SIZE_MATCH(xXFixesRegionExtentsReq);
    VERIFY_REGION(pSource, stuff->source, client, DixReadAccess);
    VERIFY_REGION(pDestination, stuff->destination, client, DixWriteAccess);

    RegionReset(pDestination, RegionExtents(pSource));

    return (client->noClientException);
}

int
SProcXFixesRegionExtents (ClientPtr client)
{
    REQUEST(xXFixesRegionExtentsReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXFixesRegionExtentsReq);
    swapl(&stuff->source);
    swapl(&stuff->destination);
    return (*ProcXFixesVector[stuff->xfixesReqType]) (client);
}

int
ProcXFixesFetchRegion (ClientPtr client)
{
    RegionPtr		    pRegion;
    xXFixesFetchRegionReply *reply;
    xRectangle		    *pRect;
    BoxPtr		    pExtent;
    BoxPtr		    pBox;
    int			    i, nBox;
    REQUEST(xXFixesFetchRegionReq);

    REQUEST_SIZE_MATCH(xXFixesFetchRegionReq);
    VERIFY_REGION(pRegion, stuff->region, client, DixReadAccess);

    pExtent = RegionExtents(pRegion);
    pBox = RegionRects (pRegion);
    nBox = RegionNumRects (pRegion);
    
    reply = malloc (sizeof (xXFixesFetchRegionReply) +
		    nBox * sizeof (xRectangle));
    if (!reply)
	return BadAlloc;
    reply->type = X_Reply;
    reply->sequenceNumber = client->sequence;
    reply->length = nBox << 1;
    reply->x = pExtent->x1;
    reply->y = pExtent->y1;
    reply->width = pExtent->x2 - pExtent->x1;
    reply->height = pExtent->y2 - pExtent->y1;

    pRect = (xRectangle *) (reply + 1);
    for (i = 0; i < nBox; i++)
    {
	pRect[i].x = pBox[i].x1;
	pRect[i].y = pBox[i].y1;
	pRect[i].width = pBox[i].x2 - pBox[i].x1;
	pRect[i].height = pBox[i].y2 - pBox[i].y1;
    }
    if (client->swapped) {
	swaps(&reply->sequenceNumber);
	swapl(&reply->length);
	swaps(&reply->x);
	swaps(&reply->y);
	swaps(&reply->width);
	swaps(&reply->height);
	SwapShorts((INT16 *) pRect, nBox * 4);
    }
    WriteToClient(client, sizeof (xXFixesFetchRegionReply) +
			 nBox * sizeof (xRectangle), reply);
    free (reply);
    return (client->noClientException);
}

int
SProcXFixesFetchRegion (ClientPtr client)
{
    REQUEST(xXFixesFetchRegionReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXFixesFetchRegionReq);
    swapl(&stuff->region);
    return (*ProcXFixesVector[stuff->xfixesReqType]) (client);
}

int
ProcXFixesSetGCClipRegion (ClientPtr client)
{
    GCPtr	pGC;
    RegionPtr	pRegion;
    XID		vals[2];
#ifndef NXAGENT_SERVER
    int		rc;
#endif
    REQUEST(xXFixesSetGCClipRegionReq);

    REQUEST_SIZE_MATCH(xXFixesSetGCClipRegionReq);

#ifndef NXAGENT_SERVER
    rc = dixLookupGC(&pGC, stuff->gc, client, DixWriteAccess);
    if (rc != Success)
	return rc;
#else
    SECURITY_VERIFY_GC(pGC, stuff->gc, client, DixWriteAccess);
#endif
    VERIFY_REGION_OR_NONE (pRegion, stuff->region, client, DixReadAccess);

    if (pRegion)
    {
	pRegion = XFixesRegionCopy (pRegion);
	if (!pRegion)
	    return BadAlloc;
    }
    
    vals[0] = stuff->xOrigin;
    vals[1] = stuff->yOrigin;
    DoChangeGC (pGC, GCClipXOrigin|GCClipYOrigin, vals, 0);
    (*pGC->funcs->ChangeClip)(pGC, pRegion ? CT_REGION : CT_NONE, (void *)pRegion, 0);

    return (client->noClientException);
}

int
SProcXFixesSetGCClipRegion (ClientPtr client)
{
    REQUEST(xXFixesSetGCClipRegionReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXFixesSetGCClipRegionReq);
    swapl(&stuff->gc);
    swapl(&stuff->region);
    swaps(&stuff->xOrigin);
    swaps(&stuff->yOrigin);
    return (*ProcXFixesVector[stuff->xfixesReqType]) (client);
}

typedef	RegionPtr (*CreateDftPtr)(WindowPtr pWin);

int
ProcXFixesSetWindowShapeRegion (ClientPtr client)
{
#ifdef SHAPE
    WindowPtr	    pWin;
    ScreenPtr	    pScreen;
    RegionPtr	    pRegion;
    RegionPtr	    *pDestRegion;
    REQUEST(xXFixesSetWindowShapeRegionReq);

    REQUEST_SIZE_MATCH(xXFixesSetWindowShapeRegionReq);
    pWin = (WindowPtr) LookupIDByType (stuff->dest, RT_WINDOW);
    if (!pWin)
    {
	client->errorValue = stuff->dest;
	return BadWindow;
    }
    VERIFY_REGION_OR_NONE(pRegion, stuff->region, client, DixWriteAccess);
    pScreen = pWin->drawable.pScreen;
    switch (stuff->destKind) {
    case ShapeBounding:
    case ShapeClip:
    case ShapeInput:
	break;
    default:
	client->errorValue = stuff->destKind;
	return BadValue;
    }
    if (pRegion)
    {
	pRegion = XFixesRegionCopy (pRegion);
	if (!pRegion)
	    return BadAlloc;
	if (!pWin->optional)
	    MakeWindowOptional (pWin);
	switch (stuff->destKind) {
	default:
	case ShapeBounding:
	    pDestRegion = &pWin->optional->boundingShape;
	    break;
	case ShapeClip:
	    pDestRegion = &pWin->optional->clipShape;
	    break;
	case ShapeInput:
	    pDestRegion = &pWin->optional->inputShape;
	    break;
	}
	if (stuff->xOff || stuff->yOff)
	    RegionTranslate(pRegion, stuff->xOff, stuff->yOff);
    }
    else
    {
	if (pWin->optional)
	{
	    switch (stuff->destKind) {
	    default:
	    case ShapeBounding:
		pDestRegion = &pWin->optional->boundingShape;
		break;
	    case ShapeClip:
		pDestRegion = &pWin->optional->clipShape;
		break;
	    case ShapeInput:
		pDestRegion = &pWin->optional->inputShape;
		break;
	    }
	}
	else
	    pDestRegion = &pRegion; /* a NULL region pointer */
    }
    if (*pDestRegion)
	RegionDestroy(*pDestRegion);
    *pDestRegion = pRegion;
    (*pScreen->SetShape) (pWin);
    SendShapeNotify (pWin, stuff->destKind);
    return (client->noClientException);
#else
    return BadRequest;
#endif
}

int
SProcXFixesSetWindowShapeRegion (ClientPtr client)
{
    REQUEST(xXFixesSetWindowShapeRegionReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXFixesSetWindowShapeRegionReq);
    swapl(&stuff->dest);
    swaps(&stuff->xOff);
    swaps(&stuff->yOff);
    swapl(&stuff->region);
    return (*ProcXFixesVector[stuff->xfixesReqType]) (client);
}

int
ProcXFixesSetPictureClipRegion (ClientPtr client)
{
#ifdef RENDER
    PicturePtr		pPicture;
    RegionPtr		pRegion;
    REQUEST(xXFixesSetPictureClipRegionReq);
    
    REQUEST_SIZE_MATCH (xXFixesSetPictureClipRegionReq);
    VERIFY_PICTURE(pPicture, stuff->picture, client, DixWriteAccess,
		   RenderErrBase + BadPicture);
    VERIFY_REGION_OR_NONE(pRegion, stuff->region, client, DixReadAccess);
    
    return SetPictureClipRegion (pPicture, stuff->xOrigin, stuff->yOrigin,
				 pRegion);
#else
    return BadRequest;
#endif
}

int
SProcXFixesSetPictureClipRegion (ClientPtr client)
{
    REQUEST(xXFixesSetPictureClipRegionReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXFixesSetPictureClipRegionReq);
    swapl(&stuff->picture);
    swapl(&stuff->region);
    swaps(&stuff->xOrigin);
    swaps(&stuff->yOrigin);
    return (*ProcXFixesVector[stuff->xfixesReqType]) (client);
}

int
ProcXFixesExpandRegion (ClientPtr client)
{
    RegionPtr	pSource, pDestination;
    int		ret = Success;
    REQUEST (xXFixesExpandRegionReq);
    BoxPtr	pTmp;
    BoxPtr	pSrc;
    int		nBoxes;
    int		i;

    REQUEST_SIZE_MATCH (xXFixesExpandRegionReq);
    VERIFY_REGION(pSource, stuff->source, client, DixReadAccess);
    VERIFY_REGION(pDestination, stuff->destination, client, DixWriteAccess);
    
    nBoxes = RegionNumRects(pSource);
    pSrc = RegionRects(pSource);
    if (nBoxes)
    {
	pTmp = malloc (nBoxes * sizeof (BoxRec));
	if (!pTmp)
	    return BadAlloc;
	for (i = 0; i < nBoxes; i++)
	{
	    pTmp[i].x1 = pSrc[i].x1 - stuff->left;
	    pTmp[i].x2 = pSrc[i].x2 + stuff->right;
	    pTmp[i].y1 = pSrc[i].y1 - stuff->top;
	    pTmp[i].y2 = pSrc[i].y2 + stuff->bottom;
	}
	RegionEmpty(pDestination);
	for (i = 0; i < nBoxes; i++)
	{
	    RegionRec	r;
	    RegionInit(&r, &pTmp[i], 0);
	    RegionUnion(pDestination, pDestination, &r);
	}
	free(pTmp);
    }
    if (ret == Success) 
	ret = client->noClientException;
    return ret;
}

int
SProcXFixesExpandRegion (ClientPtr client)
{
    REQUEST(xXFixesExpandRegionReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXFixesExpandRegionReq);
    swapl(&stuff->source);
    swapl(&stuff->destination);
    swaps(&stuff->left);
    swaps(&stuff->right);
    swaps(&stuff->top);
    swaps(&stuff->bottom);
    return (*ProcXFixesVector[stuff->xfixesReqType]) (client);
}
