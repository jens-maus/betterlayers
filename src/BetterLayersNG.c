/***************************************************************************

 Version: MPL 1.1/GPL 2.0/LGPL 2.1

 The contents of this file are subject to the Mozilla Public License Version
 1.1 (the "License"); you may not use this file except in compliance with
 the License. You may obtain a copy of the License at
 http://www.mozilla.org/MPL/

 Software distributed under the License is distributed on an "AS IS" basis,
 WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 for the specific language governing rights and limitations under the
 License.

 The Original Code is the BetterLayers library.

 The Initial Developer of the Original Code is Allan Odgaard.
 Portions created by the Initial Developer are Copyright (C) 1999-2005
 the Initial Developer. All Rights Reserved.

 Contributor(s):
   BetterLayers Open Source Team - http://www.sf.net/projects/betterlayers

 Alternatively, the contents of this file may be used under the terms of
 either the GNU General Public License Version 2 or later (the "GPL"), or
 the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 in which case the provisions of the GPL or the LGPL are applicable instead
 of those above. If you wish to allow use of your version of this file only
 under the terms of either the GPL or the LGPL, and not to allow others to
 use your version of this file under the terms of the MPL, indicate your
 decision by deleting the provisions above and replace them with the notice
 and other provisions required by the GPL or the LGPL. If you do not delete
 the provisions above, a recipient may use your version of this file under
 the terms of any one of the MPL, the GPL or the LGPL.

 $Id$

***************************************************************************/

#include <string.h>
#include <clib/alib_protos.h>
#include <clib/macros.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <graphics/clip.h>
#include <graphics/layers.h>

#include "BetterLayersNG.h"
#include "ClipRects.h"
#include "Debug.h"
#include "Locks.h"
#include "PatchProtos.h"

struct Layer_Info *pNewLayerInfo (void)
{
	struct Layer_Info *li = new Layer_Info();

	li->top_layer = NULL;
	li->Flags = 0;
	li->LockLayersCount = 0;
	li->BlankHook = LAYERS_BACKFILL;

	memset(&li->Lock, 0, sizeof(SignalSemaphore));
	InitSemaphore(&li->Lock);

	NewList((List *)&li->gs_Head);

	D(DBF_LAYERS, ("LayerInfo 0x%lx", li))

	return li;
}

void pDisposeLayerInfo (REG(a0) struct Layer_Info *li)
{
	D(DBF_LAYERS, ("LayerInfo 0x%lx", li))
	struct Layer *last, *l = li->top_layer;
	while(last = l)
	{
		D(DBF_LAYERS, ("\tDisposing layer 0x%lx", l))
		l = l->back;
		pDeleteLayer(0, last);
	}
	delete li;
}

struct Layer *pCreateUpfrontHookLayer (REG(a0) struct Layer_Info *li, REG(a1) struct BitMap *bm, REG(d0) long x0, REG(d1) long y0, REG(d2) long x1, REG(d3) long y1, REG(d4) long flags, REG(a3) struct Hook *hook, REG(a2) struct BitMap *bm2)
{
	struct RastPort *rp = new RastPort;
	struct Layer *l = new Layer;

	InitRastPort(rp);
	rp->BitMap = bm;
	rp->Layer = l;

	l->rp = rp;
	l->bounds.MinX = x0;
	l->bounds.MinY = y0;
	l->bounds.MaxX = x1;
	l->bounds.MaxY = y1;
	l->Flags = flags & (LAYERSIMPLE | LAYERSMART | LAYERSUPER | LAYERBACKDROP);
	l->SuperBitMap = bm2;
	memset(&l->Lock, 0, sizeof(SignalSemaphore));
	InitSemaphore(&l->Lock);
	l->LayerInfo = li;
	l->Width = x1-x0 + 1;
	l->Height = y1-y0 + 1;
	l->BackFill = hook;
	l->DamageList = NewRegion();
	l->ClipRect = NewClipRects(l);
	l->ClipRegion = NULL;

	Enter(li);
	AddTail((List *)&li->gs_Head, &l->Lock.ss_Link);

	/* link */
	struct Layer *target;
	if((target = li->top_layer) && (l->Flags & LAYERBACKDROP) && !(target->Flags & LAYERBACKDROP))
	{
		while(target->back && !(target->back->Flags & LAYERBACKDROP))
			target = target->back;

		l->front = target;
		if(l->back = target->back)
			l->back->front = l;
		target->back = l;
	}
	else
	{
		l->front = NULL;
		if(l->back = li->top_layer)
			li->top_layer->front = l;
		li->top_layer = l;
	}

	SubFromClipRects(l->back, &l->bounds);

	D(DBF_LOCKS, ("Adjusting to LockLayersCount (%ld)", li->LockLayersCount))
	for(ULONG i = 0; i < li->LockLayersCount; i++)
		pLockLayer(0, l);
	Leave(li);

	pDoHookClipRects(hook, rp, NULL);

	D(DBF_LAYERS, ("li 0x%lx, l 0x%lx, rp 0x%lx", li, l, rp))
	return l;
}

LONG pDeleteLayer (REG(a0) long dummy, REG(a1) struct Layer *l)
{
	D(DBF_LAYERS, ("l 0x%lx", l))

	if(l->front)
			l->front->back = l->back;
	else	l->LayerInfo->top_layer = l->back;

	if(l->back)
		l->back->front = l->front;

	AddToClipRects(l, l->back);

	delete l->rp;
	DisposeRegion(l->DamageList);
	Remove(&l->Lock.ss_Link);
	DeleteClipRects(l->ClipRect);
	delete GetExtraInfo(l);

	delete l;

	return TRUE;
}

LONG pUpfrontLayer (REG(a0) long dummy, REG(a1) struct Layer *layer)
{
	return TRUE;
}

LONG pBehindLayer (REG(a0) long dummy, REG(a1) struct Layer *layer)
{
	return TRUE;
}

LONG pMoveSizeLayer (REG(a0) struct Layer *layer, REG(d0) long dx, REG(d1) long dy, REG(d2) long dw, REG(d3) long dh)
{
	return TRUE;
}

VOID BackFillCode (REG(a0) struct Hook *hook, REG(a1) struct HookMsg *msg, REG(a2) struct RastPort *rp)
{
	BltBitMap(rp->BitMap, msg->bounds.MinX, msg->bounds.MinY, rp->BitMap, msg->bounds.MinX, msg->bounds.MinY, msg->bounds.MaxX-msg->bounds.MinX+1, msg->bounds.MaxY-msg->bounds.MinY+1, 0x000, -1, NULL);
}

struct Hook BackFillHook = { { NULL, NULL }, (ULONG(*)())BackFillCode };

void pDoHookClipRects (REG(a0) struct Hook *hook, REG(a1) struct RastPort *rp, REG(a2) struct Rectangle *rect)
{
	switch((ULONG)hook)
	{
		case (ULONG)LAYERS_NOBACKFILL:
		break;

		case (ULONG)LAYERS_BACKFILL:
			hook = &BackFillHook;
		/* continue */

		default:
		{
			struct Layer *l;
			if(l = rp->Layer)
			{
				pLockLayer(0, l);
				struct Rectangle dummy;
				if(!rect)
				{
					dummy.MinX = dummy.MinY = 0;
					dummy.MaxX = l->Width-1;
					dummy.MaxY = l->Height-1;
					rect = &dummy;
				}

				LONG x1 = rect->MinX+l->bounds.MinX, y1 = rect->MinY+l->bounds.MinY;
				LONG x2 = rect->MaxX+l->bounds.MinX, y2 = rect->MaxY+l->bounds.MinY;

				struct HookMsg hmsg = { l };
				struct ClipRect *cr = l->ClipRect;
				while(cr)
				{
					hmsg.bounds.MinX = MAX(x1, cr->bounds.MinX);
					hmsg.bounds.MinY = MAX(y1, cr->bounds.MinY);
					hmsg.bounds.MaxX = MIN(x2, cr->bounds.MaxX);
					hmsg.bounds.MaxY = MIN(y2, cr->bounds.MaxY);

					if(hmsg.bounds.MinX <= hmsg.bounds.MaxX && hmsg.bounds.MinY <= hmsg.bounds.MaxY)
					{
						hmsg.offsetx = hmsg.bounds.MinX - l->bounds.MinX;
						hmsg.offsety = hmsg.bounds.MinY - l->bounds.MinY;
						CallHookA(hook, (Object *)rp, &hmsg);
					}
					cr = cr->Next;
				}
				pUnlockLayer(l);
			}
			else
			{
				struct HookMsg hmsg = { NULL, { rect->MinX, rect->MinY, rect->MaxX, rect->MaxY }, rect->MinX, rect->MinY };
				CallHookA(hook, (Object *)rp, &hmsg);
			}
		}
	}
}

struct Layer *pWhichLayer (REG(a0) struct Layer_Info *li, REG(d0) long x, REG(d1) long y)
{
	struct Layer *l = li->top_layer;
	while(l)
	{
		if(BETWEEN(x, l->bounds.MinX, l->bounds.MaxX) && BETWEEN(y, l->bounds.MinY, l->bounds.MaxY))
			break;

		l = l->back;
	}
	D(DBF_LAYERS, ("0x%lx", l))
	return l;
}

struct ClipRect *NewClipRect (LONG x0, LONG y0, LONG x1, LONG y1, struct ClipRect *next)
{
	struct ClipRect *cr = new ClipRect;
	if(cr->Next = next)
		next->prev = cr;
	cr->prev = NULL;
	cr->lobs = NULL;
	cr->BitMap = NULL;
	cr->bounds.MinX = x0;
	cr->bounds.MinY = y0;
	cr->bounds.MaxX = x1;
	cr->bounds.MaxY = y1;
	cr->Flags = 0;

	return cr;
}

struct ClipRect *AddIntersection (struct Row *row, struct Rectangle *rect, struct ClipRect *cr)
{
	LONG x0 = rect->MinX, y0 = rect->MinY, x1 = rect->MaxX, y1 = rect->MaxY;
	while(row)
	{
		if(row->MaxY < y0)
		{
			row = row->Next;
			continue;
		}

		if(row->MinY > y1)
			break;

		struct Cell *cell = row->FirstCell;
		while(cell)
		{
			if(cell->MaxX < x0)
			{
				cell = cell->Next;
				continue;
			}

			if(cell->MinX > x1)
				break;

			cr = NewClipRect(
				MAX(x0, cell->MinX),
				MAX(y0, row->MinY),
				MIN(x1, cell->MaxX),
				MIN(y1, row->MaxY),
				cr);

			cell = cell->Next;
		}
		row = row->Next;
	}
	return cr;
}

struct ClipRect *MyClipRect::Intersect (struct Region *region)
{
	struct ClipRect *cr = NULL;
	LONG x = region->bounds.MinX, y = region->bounds.MinY;

	struct RegionRectangle *rect = region->RegionRectangle;
	while(rect)
	{
		struct Rectangle r = { x + rect->bounds.MinX, y + rect->bounds.MinY, x + rect->bounds.MaxX, y + rect->bounds.MaxY };
		cr = AddIntersection(FirstRow, &r, cr);
		rect = rect->Next;
	}

	return cr;
}

VOID CoreBeginUpdate (struct Layer *l)
{
	D(DBF_CLIPPING, ("layer 0x%lx", l))

#ifdef DEBUG
{	struct ClipRect *cr = l->ClipRect;
	D(DBF_CLIPPING, ("Old ClipRect:"))
	while(cr)
	{
		D(DBF_CLIPPING, ("\t%3ld, %3ld, %3ld, %3ld", cr->bounds.MinX, cr->bounds.MinY, cr->bounds.MaxX, cr->bounds.MaxY))
		cr = cr->Next;
	}

	if((l->Flags & LAYERUPDATING) && l->DamageList)
	{
		D(DBF_CLIPPING, ("Damage:"))
		LONG x = l->DamageList->bounds.MinX, y = l->DamageList->bounds.MinY;
		struct RegionRectangle *rect = l->DamageList->RegionRectangle;
		while(rect)
		{
			D(DBF_CLIPPING, ("\t%3ld, %3ld, %3ld, %3ld", x+rect->bounds.MinX, y+rect->bounds.MinY, x+rect->bounds.MaxX, y+rect->bounds.MaxY))
			rect = rect->Next;
		}
	}
	else D(DBF_CLIPPING, ("No damage"))

	if(l->ClipRegion && l->ClipRegion->RegionRectangle)
	{
		D(DBF_CLIPPING, ("ClipRegion:"))
		LONG x = l->ClipRegion->bounds.MinX, y = l->ClipRegion->bounds.MinY;
		struct RegionRectangle *rect = l->ClipRegion->RegionRectangle;
		while(rect)
		{
			D(DBF_CLIPPING, ("\t%3ld, %3ld, %3ld, %3ld", x+rect->bounds.MinX, y+rect->bounds.MinY, x+rect->bounds.MaxX, y+rect->bounds.MaxY))
			rect = rect->Next;
		}
	}
	else D(DBF_CLIPPING, ("No clip region"))
}
#endif

	struct MyClipRect *cr = GetExtraInfo(l);
	l->_cliprects = l->ClipRect;
	l->ClipRect = NULL;

	if((l->Flags & LAYERUPDATING) && l->DamageList)
		l->ClipRect = cr->Intersect(l->DamageList);

/*	if((region = l->ClipRegion) && region->RegionRectangle)
		res = intersect2 = res->Union(region, l->bounds.MinX, l->bounds.MinY);
*/

#ifdef DEBUG
{	struct ClipRect *cr = l->ClipRect;
	D(DBF_CLIPPING, ("New ClipRect:"))
	while(cr)
	{
		D(DBF_CLIPPING, ("\t%3ld, %3ld, %3ld, %3ld", cr->bounds.MinX, cr->bounds.MinY, cr->bounds.MaxX, cr->bounds.MaxY))
		cr = cr->Next;
	}
}
#endif
}

VOID CoreEndUpdate (struct Layer *l)
{
	D(DBF_CLIPPING, ("layer 0x%lx", l))

	DeleteClipRects(l->ClipRect);
	l->ClipRect = l->_cliprects;
	l->_cliprects = NULL;
}

LONG pBeginUpdate (REG(a0) struct Layer *l)
{
	D(DBF_CLIPPING, ("layer 0x%lx", l))
	pLockLayer(0, l);

	if(l->ClipRegion)
		CoreEndUpdate(l);

	l->Flags |= LAYERUPDATING;

	CoreBeginUpdate(l);

	return TRUE;
}

void pEndUpdate (REG(a0) struct Layer *l, REG(d0) unsigned long flag)
{
	D(DBF_CLIPPING, ("layer 0x%lx, flag %ld", l, flag))
	if(flag)
		ClearRegion(l->DamageList);

	CoreEndUpdate(l);

	l->Flags &= ~LAYERUPDATING;

	if(l->ClipRegion)
		CoreBeginUpdate(l);

	pUnlockLayer(l);
}
