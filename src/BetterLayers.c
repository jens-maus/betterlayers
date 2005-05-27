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

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <clib/alib_protos.h>
#include <clib/macros.h>
#include <graphics/clip.h>
#include <graphics/layers.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <exec/memory.h>

#pragma header

#include "BetterLayers.h"
#include "Debug.h"
#include "Mask.h"
#include "PatchProtos.h"

//#define REUSE_MEMORY

#if 01

#define MyOrRectRegion OrRectRegion

#else

BOOL MyOrRectRegion (struct Region *region, struct Rectangle *rect)
{
	LONG offsetx = region->bounds.MinX, offsety = region->bounds.MinY;
	struct RegionRectangle *rr = region->RegionRectangle;
	while(rr)
	{
		rr->bounds.MinX -= offsetx;
		rr->bounds.MinY -= offsety;
		rr->bounds.MaxX -= offsetx;
		rr->bounds.MaxY -= offsety;
		rr = rr->Next;
	}

	region->bounds.MinX = region->bounds.MinY = 0;
	region->bounds.MaxX = MAX(region->bounds.MaxX, rect->MaxX);
	region->bounds.MaxY = MAX(region->bounds.MaxY, rect->MaxY);

	if(rr = (RegionRectangle *)AllocMem(sizeof(RegionRectangle), MEMF_ANY))
	{
		if(rr->Next = region->RegionRectangle)
			rr->Next->Prev = rr;
		rr->Prev = NULL;
		rr->bounds = *rect;

		region->RegionRectangle = rr;
	}
	return TRUE;
}
#endif

VOID ClearRect (struct Rectangle *rect, APTR userdata)
{
	struct BitMap *bmp = (struct BitMap *)userdata;
	BltBitMap(bmp, rect->MinX, rect->MinY, bmp, rect->MinX, rect->MinY, rect->MaxX-rect->MinX+1, rect->MaxY-rect->MinY+1, 0x000, -1, NULL);
}

VOID BackFillLayer (struct Rectangle *rect, struct Layer *l)
{
	switch((ULONG)l->BackFill)
	{
		case (ULONG)LAYERS_NOBACKFILL:
		break;

		case (ULONG)LAYERS_BACKFILL:
			ClearRect(rect, l->rp->BitMap);
		break;

		default:
			struct HookMsg hmsg = { l, rect->MinX, rect->MinY, rect->MaxX, rect->MaxY, rect->MinX - l->bounds.MinX, rect->MinY - l->bounds.MinY };
			CallHookA(l->BackFill, (Object *)l->rp, &hmsg);
		break;
	}
}

BOOL RectOverlap (struct Rectangle &r1, struct Rectangle &r2)
{
	return !((r1.MinX > r2.MaxX || r1.MaxX < r2.MinX) || (r1.MinY > r2.MaxY || r1.MaxY < r2.MinY));
}

VOID DelClipRects (struct Layer *l)
{
	struct ClipRect *cr = l->ClipRect;

	struct ClipRect *prev_cr;
	while(prev_cr = cr)
	{
		if(cr->BitMap)
		{
			WaitBlit();
			FreeBitMap(cr->BitMap);
		}

		cr = cr->Next;
		delete prev_cr;
	}

/*	struct ClipRect *cr = l->ClipRect;
	struct ClipRect *free_cr;
	if(!(free_cr = l->LayerInfo->FreeClipRects))
		free_cr = (ClipRect *)&l->LayerInfo->FreeClipRects;

	struct ClipRect *prev_cr;
	while(prev_cr = cr)
	{
		if(cr->BitMap)
		{
			WaitBlit();
			FreeBitMap(cr->BitMap);
		}

		free_cr->Next = cr;
		cr = cr->Next;
		prev_cr->Next = NULL;
	}
*/
}

struct ClipRect *NewClipRect (struct Layer_Info *li)
{
/*	struct ClipRect *cr;
	if(cr = li->FreeClipRects)
	{
		li->FreeClipRects = cr->Next;
		cr->prev = cr->Next = NULL;

		cr->lobs = NULL;
		cr->BitMap = NULL;
	}
	else
	{
		cr = new ClipRect();
	}
	return cr;
*/	return new ClipRect();
}

BOOL bBeginUpdate (struct Layer *l)
{
	struct ClipMask cliprect(l->_cliprects = l->ClipRect), *intersect1 = NULL, *intersect2 = NULL, *res = &cliprect;
	l->ClipRect = NULL;

	struct Region *region;
	if((l->Flags & LAYERUPDATING) && (region = l->DamageList) && region->RegionRectangle)
		res = intersect1 = res->Union(region, l->bounds.MinX, l->bounds.MinY);

	if((region = l->ClipRegion) && region->RegionRectangle)
		res = intersect2 = res->Union(region, l->bounds.MinX, l->bounds.MinY);

	if(intersect1 || intersect2)
	{
		res->ExportAsClipRect(l);
		delete intersect1;
		delete intersect2;
	}

/*	SetDrMd(l->rp, COMPLEMENT);
	for(UWORD i = 0; i < 4; i++)
	{
		RectFill(l->rp, 0, 0, l->Width-1, l->Height-1);
		if(FindTask(NULL)->tc_Node.ln_Type == NT_PROCESS)
			Delay(1);
	}
	SetDrMd(l->rp, JAM2);*/

	return TRUE;
}

VOID bEndUpdate (struct Layer *l)
{
	DelClipRects(l);
	l->ClipRect = l->_cliprects;
	l->_cliprects = NULL;
}

VOID BuildClipRects (struct Layer_Info *li, struct Rectangle &damage)
{
	struct Layer *l = li->top_layer;
	struct ClipMask cm();
	cm.SetMask(l->rp->BitMap);
	while(l)
	{
		if(RectOverlap(damage, l->bounds))
			cm.ExportAsClipRect(l);
		cm.SubMask(l->bounds);
		l = l->back;
	}
}

VOID BuildClipRects (struct Layer_Info *li, struct Layer *l, struct Rectangle &damage)
{
	BOOL reached = FALSE;
	struct Layer *current = li->top_layer;
	struct ClipMask cm();
	cm.SetMask(current->rp->BitMap);
	while(current)
	{
		if(current == l)
			reached = TRUE;

		if(reached && RectOverlap(damage, current->bounds))
			cm.ExportAsClipRect(current);

		cm.SubMask(current->bounds);
		current = current->back;
	}
}

VOID AddDragDamage (struct Rectangle *rect, APTR userdata)
{
	struct Layer *l = (struct Layer *)userdata;
//	if(l->Flags & LAYERSIMPLE)
	{
		struct Rectangle r =
		{
			rect->MinX - l->bounds.MinX,
			rect->MinY - l->bounds.MinY,
			rect->MaxX - l->bounds.MinX,
			rect->MaxY - l->bounds.MinY
		};
		MyOrRectRegion(l->DamageList, &r);
		BackFillLayer(rect, l);
	}
}

VOID BuildClipRects (struct Layer *layer, struct ClipMask &cm, struct Rectangle &damage, BOOL ghost = FALSE)
{
	struct Layer_Info *li = layer->LayerInfo;
	struct Layer *l = li->top_layer;
	struct ClipMask cm2();
	cm2.SetMask(l->rp->BitMap);
	BOOL pass = FALSE;
	while(l)
	{
		if(l == layer)
		{
			pass = TRUE;
			if(ghost)
			{
				l = l->back;
				continue;
			}
		}

		if(RectOverlap(damage, l->bounds))
		{
			if(pass)
			{
				cm2.ExportAsClipRect(l);

				if((l != layer) && (l->Flags & LAYERSIMPLE)) // ?
				{
					cm.ForEach(l->bounds, AddDragDamage, l);
					if(l->DamageList->RegionRectangle)
						l->Flags |= LAYERREFRESH | LAYERIREFRESH | LAYERIREFRESH2;
				}
			}
		}
		cm.SubMask(l->bounds);
		cm2.SubMask(l->bounds);
		l = l->back;
	}

	struct Hook *bfill = (struct Hook *)li->BlankHook;
	struct MaskLine *line = cm.First();
	while(line)
	{
		struct MaskColumn *column = line->First();
		while(column)
		{
			struct HookMsg hmsg = { NULL, column->XStart, line->YStart, column->XStop, line->YStop, column->XStart, line->YStart };
			if(bfill == LAYERS_BACKFILL)
				ClearRect(&hmsg.bounds, layer->rp->BitMap);
			else if(bfill != LAYERS_NOBACKFILL)
				CallHookA(bfill, (Object *)layer->rp, &hmsg);
			column = (struct MaskColumn *)column->Next;
		}
		line = (struct MaskLine *)line->Next;
	}
}

VOID BuildClipRects (struct Layer *layer, struct Rectangle &damage)
{
	struct Layer_Info *li = layer->LayerInfo;
	struct Layer *l = li->top_layer;
	struct ClipMask cm();
	cm.SetMask(l->rp->BitMap);
	BOOL pass = FALSE;
	while(l)
	{
		if(l == layer)
			pass = TRUE;

		if(pass && RectOverlap(damage, l->bounds))
			cm.ExportAsClipRect(l);
		cm.SubMask(l->bounds);
		l = l->back;
	}
}

#ifdef REUSE_MEMORY
struct RastPort *RP_Cache = NULL;
struct Layer *Layer_Cache = NULL;
#endif

struct Layer *bCreateUpfrontHookLayer (struct Layer_Info *li, struct BitMap *bmp, LONG x0, LONG y0, LONG x1, LONG y1, LONG flags, struct Hook *hook, struct BitMap *super_bmp)
{
	struct RastPort *rp;
#ifdef REUSE_MEMORY
	if(rp = RP_Cache)
	{
		RP_Cache = (RastPort *)rp->Layer;
	}
	else
#endif
	{
		rp = new RastPort();
	}

	InitRastPort(rp);
	rp->BitMap = bmp;

	struct Layer *l;
#ifdef REUSE_MEMORY
	if(l = Layer_Cache)
	{
		Layer_Cache = l->front;
		l->ClipRect = NULL;
		l->ClipRegion = NULL;
		l->_cliprects = NULL;
//		memset(l, 0, offsetof(Layer, ClipRegion));//sizeof(Layer));
	}
	else
#endif
	{
		l = new Layer();
	}

	rp->Layer = l;
	l->rp = rp;
	l->bounds.MinX = x0;
	l->bounds.MinY = y0;
	l->bounds.MaxX = x1;
	l->bounds.MaxY = y1;
	l->Flags = flags & (LAYERSIMPLE | LAYERSMART | LAYERSUPER | LAYERBACKDROP);
	InitSemaphore(&l->Lock);
	AddTail((struct List *)&li->gs_Head, &l->Lock.ss_Link);
	l->LayerInfo = li;
	l->Width = x1-x0 + 1;
	l->Height = y1-y0 + 1;
	l->BackFill = hook;
	l->DamageList = NewRegion();

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

	BuildClipRects(li, l, l->bounds);
	bDoHookClipRects(hook, rp, NULL);
//	WaitBlit();

	return l;
}

struct Layer *bCreateBehindHookLayer (struct Layer_Info *li, struct BitMap *bmp, LONG x0, LONG y0, LONG x1, LONG y1, LONG flags, struct Hook *hook, struct BitMap *super_bmp)
{
	struct RastPort *rp;
#ifdef REUSE_MEMORY
	if(rp = RP_Cache)
	{
		RP_Cache = (RastPort *)rp->Layer;
	}
	else
#endif
	{
		rp = new RastPort();
	}

	InitRastPort(rp);
	rp->BitMap = bmp;

	struct Layer *l;
#ifdef REUSE_MEMORY
	if(l = Layer_Cache)
	{
		Layer_Cache = l->front;
		l->ClipRect = NULL;
		l->ClipRegion = NULL;
		l->_cliprects = NULL;
//		memset(l, 0, offsetof(Layer, ClipRegion));//sizeof(Layer));
	}
	else
#endif
	{
		l = new Layer();
	}

	rp->Layer = l;
	l->rp = rp;
	l->bounds.MinX = x0;
	l->bounds.MinY = y0;
	l->bounds.MaxX = x1;
	l->bounds.MaxY = y1;
	l->Flags = flags & (LAYERSIMPLE | LAYERSMART | LAYERSUPER | LAYERBACKDROP);
	InitSemaphore(&l->Lock);
	AddTail((struct List *)&li->gs_Head, &l->Lock.ss_Link);
	l->LayerInfo = li;
	l->Width = x1-x0 + 1;
	l->Height = y1-y0 + 1;
	l->BackFill = hook;
	l->DamageList = NewRegion();

	/* link */
	struct Layer *target = NULL, *current;
	if(current = li->top_layer)
	{
		do {
			target = current;
		} while((current = current->back) && !(current->Flags & LAYERBACKDROP));

		if(current && (l->Flags & LAYERBACKDROP))
		{
			do {
				target = current;
			} while((current = current->back));
		}

		l->front = target;
		if(l->back = target->back)
			l->back->front = l;
		target->back = l;
	}
	else
	{
		li->top_layer = l;
		l->front = l->back = NULL;
	}

	BuildClipRects(li, l, l->bounds);
	bDoHookClipRects(hook, rp, NULL);
//	WaitBlit();

	return l;
}

BOOL bDeleteLayer (LONG dummy, struct Layer *l)
{
	struct ClipMask cm();
	cm.SetMask(l->bounds.MinX, l->bounds.MinY, l->bounds.MaxX, l->bounds.MaxY);
	BuildClipRects(l, cm, l->bounds, TRUE);

	/* unlink */
	if(l->front)
			l->front->back = l->back;
	else	l->LayerInfo->top_layer = l->back;

	if(l->back)
		l->back->front = l->front;

#ifdef REUSE_MEMORY
	l->rp->Layer = (Layer *)RP_Cache;
	RP_Cache = l->rp;
#else
	delete l->rp;
#endif

	DisposeRegion(l->DamageList);
	Remove(&l->Lock.ss_Link);
	DelClipRects(l);

#ifdef REUSE_MEMORY
	l->front = Layer_Cache;
	Layer_Cache = l;
#else
	delete l;
#endif

	return TRUE;
}

VOID bDisposeLayerInfo (struct Layer_Info *li)
{
	struct Layer *last, *l = li->top_layer;
	while(last = l)
	{
		l = l->back;
		bDeleteLayer(0, last);
	}

#ifdef DEBUG
	if(li->FreeClipRects)
	{
		D(DBF_RESOURCES, ("LayerInfo had FreeClipRects (0x%lx)", li->FreeClipRects))

		struct ClipRect *prev_cr, *cr = li->FreeClipRects;
		while(prev_cr = cr)
		{
			cr = cr->Next;
			delete prev_cr;
		}
	}
#endif

	delete li;
}

VOID BackFillCode (REG(a0) struct Hook *hook, REG(a1) struct HookMsg *msg, REG(a2) struct RastPort *rp)
{
	ClearRect(&msg->bounds, rp->BitMap);

/*	struct Rectangle *b = &msg->bounds;
	rp->Layer = NULL;
	RectFill(rp, b->MinX, b->MinY, b->MaxX, b->MaxY);
	rp->Layer = msg->l;*/
}

struct Hook BackFillHook = { { NULL, NULL }, (ULONG(*)())BackFillCode };

VOID bDoHookClipRects (struct Hook *hook, struct RastPort *rp, struct Rectangle *rect)
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

VOID PrivateDoHookClipRects (struct Rectangle &bounds, struct Layer *l, VOID(*func)(struct Rectangle *, APTR), APTR userdata, BOOL reverse_x = FALSE, BOOL reverse_y = FALSE)
{
	struct ClipRect *y_cr, *x_cr;
	if(!l)
	{
		func(&bounds, userdata);
	}
	else if(y_cr = l->ClipRect)
	{
		if(reverse_y)
			y_cr = PrevRow(y_cr);

		while(y_cr)
		{
			struct Rectangle rect;
			rect.MinY = MAX(y_cr->bounds.MinY, bounds.MinY);
			rect.MaxY = MIN(y_cr->bounds.MaxY, bounds.MaxY);

			if(rect.MinY <= rect.MaxY)
			{
				x_cr = reverse_x ? LastInRow(y_cr) : y_cr;
				while(x_cr && y_cr->bounds.MinY == x_cr->bounds.MinY)
				{
					rect.MinX = MAX(x_cr->bounds.MinX, bounds.MinX);
					rect.MaxX = MIN(x_cr->bounds.MaxX, bounds.MaxX);

					if(rect.MinX <= rect.MaxX)
						func(&rect, userdata);

					x_cr = reverse_x ? x_cr->prev : x_cr->Next;
				}
			}

			if(reverse_y && y_cr == l->ClipRect)
				break;

			y_cr = reverse_y ? PrevRow(y_cr) : NextRow(y_cr);
		}
	}
}

VOID PrivateDoHookForInverseClipRects (struct Rectangle &bounds, struct Layer *l, VOID(*func)(struct Rectangle *, APTR), APTR userdata)
{
	struct ClipRect *y_cr, *x_cr;
	if(!l)
	{
		func(&bounds, userdata);
	}
	else if(y_cr = l->ClipRect)
	{
		LONG x1 = bounds.MinX, x2 = bounds.MaxX;
		LONG y1 = bounds.MinY, y2 = bounds.MaxY;

		if(y1 < y_cr->bounds.MinY)
		{
			struct Rectangle rect = { x1, y1, x2, MIN(y2, y_cr->bounds.MinY-1) };
			func(&rect, userdata);
		}

		while(y_cr)
		{
			struct Rectangle rect;
			rect.MinY = MAX(y_cr->bounds.MinY, y1);
			rect.MaxY = MIN(y_cr->bounds.MaxY, y2);

			if(rect.MinY <= rect.MaxY)
			{
				LONG start = x1, stop;
				x_cr = y_cr;

				do {

					BOOL more = x_cr && y_cr->bounds.MinY == x_cr->bounds.MinY;
					stop = more ? x_cr->bounds.MinX-1 : x2;

					rect.MinX = MAX(x1, start);
					rect.MaxX = MIN(x2, stop);
					if(rect.MinX <= rect.MaxX)
						func(&rect, userdata);

					if(more)
					{
						start = x_cr->bounds.MaxX+1;
						x_cr = x_cr->Next;
					}

				}	while(stop < x2);
			}

			LONG ystop = y_cr->bounds.MaxY, ystart;
			y_cr = NextRow(y_cr);

			if((ystart = y_cr ? y_cr->bounds.MinY-1 : y2) != ystop && y1 <= ystart && y2 > ystop)
			{
				struct Rectangle rect = { x1, MAX(y1, ystop+1), x2, MIN(y2, ystart) };
				func(&rect, userdata);
			}
		}
	}
}

VOID PrivateDoHookClipRects (struct ClipRect *cr, VOID(*func)(struct Rectangle *, APTR), APTR userdata, BOOL reverse_x = FALSE, BOOL reverse_y = FALSE)
{
	struct ClipRect *y_cr, *x_cr;
	if(y_cr = cr)
	{
		if(reverse_y)
			y_cr = PrevRow(y_cr);

		while(y_cr)
		{
			LONG y = y_cr->bounds.MinY;
			x_cr = reverse_x ? LastInRow(y_cr) : y_cr;

			while(x_cr && y == x_cr->bounds.MinY)
			{
				func(&x_cr->bounds, userdata);
				x_cr = reverse_x ? x_cr->prev : x_cr->Next;
			}

			if(reverse_y && y_cr == cr)
				break;

			y_cr = reverse_y ? PrevRow(y_cr) : NextRow(y_cr);
		}
	}
}

struct Region *bInstallClipRegion (struct Layer *l, struct Region *region)
{
	struct Region *old_region;
	if((old_region = l->ClipRegion) || (l->Flags & LAYERUPDATING))
		bEndUpdate(l);

	if((l->ClipRegion = region) || (l->Flags & LAYERUPDATING))
		bBeginUpdate(l);

	return old_region;
}

VOID AddDamage (struct Rectangle *rect, APTR userdata)
{
	struct Layer *l = (struct Layer *)userdata;
//	if(l->Flags & LAYERSIMPLE)
	{
		struct Rectangle r =
		{
			rect->MinX - l->bounds.MinX,
			rect->MinY - l->bounds.MinY,
			rect->MaxX - l->bounds.MinX,
			rect->MaxY - l->bounds.MinY
		};
		MyOrRectRegion(l->DamageList, &r);
		BackFillLayer(rect, l);
	}
}

BOOL bMoveLayerInFrontOf (struct Layer *layertomove, struct Layer *targetlayer)
{
	if(layertomove == targetlayer || layertomove->back == targetlayer)
		return TRUE; //FALSE;

	struct ClipMask cm(), cm2();
	cm.SetMask(layertomove->rp->BitMap);
	cm2.SetMask(layertomove->bounds.MinX, layertomove->bounds.MinY, layertomove->bounds.MaxX, layertomove->bounds.MaxY);

	/* cm: full screen
		cm2: only the layer which gets moved
	*/

	struct Layer_Info *li = layertomove->LayerInfo;
	struct Layer *l = li->top_layer;

	while(l != layertomove && l != targetlayer)
	{
		cm.SubMask(l->bounds);
		cm2.SubMask(l->bounds);
		l = l->back;
	}

	if(l == layertomove)	// Move back
	{
		if(!layertomove->back)
			return FALSE;

		l = l->back;
		while(l != targetlayer)
		{
			if(RectOverlap(l->bounds, layertomove->bounds))
			{
				if(l->Flags & LAYERSIMPLE) // ?
				{
					cm2.ForEach(l->bounds, AddDamage, l);
					if(l->DamageList->RegionRectangle)
						l->Flags |= LAYERREFRESH | LAYERIREFRESH | LAYERIREFRESH2;
				}

				if(l->Flags & (LAYERREFRESH | LAYERSMART))
					cm.ExportAsClipRect(l);

				cm2.SubMask(l->bounds);
			}
			cm.SubMask(l->bounds);
			l = l->back;
		}

		cm.ExportAsClipRect(layertomove);

		layertomove->back->front = layertomove->front;
		if(layertomove->front)
				layertomove->front->back = layertomove->back;
		else	li->top_layer = layertomove->back;

		if(targetlayer)
		{
			layertomove->front = targetlayer->front;
			layertomove->back = targetlayer;
			targetlayer->front->back = layertomove;
			targetlayer->front = layertomove;
		}
		else
		{
			l = layertomove;
			while(l->back)
				l = l->back;

			l->back = layertomove;
			layertomove->front = l;
			layertomove->back = NULL;
		}
	}
	else						// Move forth
	{
		if(!layertomove->front)
			return FALSE;

		cm.ExportAsClipRect(layertomove);
		cm.SubMask(layertomove->bounds);

		struct ClipMask damage();
		while(l != layertomove)
		{
			if(RectOverlap(l->bounds, layertomove->bounds))
			{
				cm.ExportAsClipRect(l);
				damage.AddMask(l->bounds);
			}
			cm.SubMask(l->bounds);
			l = l->back;
		}

		if(layertomove->Flags & LAYERSIMPLE) // ?
		{
			/* here's a problem: damage is the union of all layers between
				previous and current position, so it's not clipped according
				to layers in front of it -- calling backfill hook! */
			damage.ForEach(layertomove->bounds, AddDamage, layertomove);
			if(layertomove->DamageList->RegionRectangle)
				layertomove->Flags |= LAYERREFRESH | LAYERIREFRESH | LAYERIREFRESH2;
		}

		/* remove layer */
		if(layertomove->back)
			layertomove->back->front = layertomove->front;
		layertomove->front->back = layertomove->back;

		if(targetlayer->front)
				targetlayer->front->back = layertomove;
		else	li->top_layer = layertomove;

		layertomove->back = targetlayer;
		layertomove->front = targetlayer->front;
		targetlayer->front = layertomove;
	}

	return TRUE;
}

struct UserData : ClipMask
{
	LONG dx, dy;
	struct BitMap *bmp;
	struct Layer *l;
	LONG srcx, srcy;
};

VOID BlockMove (struct Rectangle *rect, APTR userdata)
{
	struct UserData *udata = (struct UserData *)userdata;
	LONG dx = udata->dx, dy = udata->dy;
	ULONG w = rect->MaxX-rect->MinX+1, h = rect->MaxY-rect->MinY+1;

	BltBitMap(udata->bmp,
			rect->MinX-dx, rect->MinY-dy,
			udata->bmp,
			rect->MinX, rect->MinY,
			w, h, 0xc0, ~0, NULL);
}

VOID MoveLayerFunction (struct Rectangle *rect, APTR userdata)
{
	struct UserData *udata = (struct UserData *)userdata;
	struct Rectangle new_rect = { rect->MinX + udata->dx, rect->MinY + udata->dy, rect->MaxX + udata->dx, rect->MaxY + udata->dy };
	udata->ForEach(new_rect, BlockMove, udata, udata->dx > 0, udata->dy > 0);
}

VOID AddMoveDamage2 (struct Rectangle *rect, APTR userdata)
{
	struct UserData *udata = (struct UserData *)userdata;
	struct Layer *l = udata->l;
	if(l->Flags & LAYERSIMPLE)
	{
		struct Rectangle r =
		{
			rect->MinX - l->bounds.MinX,
			rect->MinY - l->bounds.MinY,
			rect->MaxX - l->bounds.MinX,
			rect->MaxY - l->bounds.MinY
		};
		MyOrRectRegion(l->DamageList, &r);
		BackFillLayer(rect, l);
	}
}

VOID AddMoveDamage (struct Rectangle *rect, APTR userdata)
{
	struct UserData *udata = (struct UserData *)userdata;
	struct Rectangle rect2 = { rect->MinX + udata->dx, rect->MinY + udata->dy, rect->MaxX + udata->dx, rect->MaxY + udata->dy };
	udata->ForEach(rect2, AddMoveDamage2, udata);
}

BOOL bMoveSizeLayer (struct Layer *layer, LONG dx, LONG dy, LONG dw, LONG dh)
{
	struct ClipMask cm();		/* Used for damage-clipping */
	struct Rectangle org_frame = layer->bounds, damage = org_frame;

	if(dw || dh)
	{
		if(dw < 0)
		{
			if(dx < -dw && dx >= 0)
			{
				struct Rectangle rect = layer->bounds;
				rect.MinX = rect.MaxX + dw;
				cm.AddMask(rect);
			}
		}
		else
		{
			damage.MaxX += dx < 0 ? MAX(0, dw+dx) : dw;

			if(dw)
			{
				struct Rectangle r = { layer->Width, 0, layer->Width+dw-1, layer->Height-1 };
				MyOrRectRegion(layer->DamageList, &r);
			}
		}

		if(dh < 0)
		{
			if(dy < -dh && dy >= 0)
			{
				struct Rectangle rect = layer->bounds;
				rect.MinY = rect.MaxY + dh;
				cm.AddMask(rect);
			}
		}
		else
		{
			damage.MaxY += dy < 0 ? MAX(0, dh+dy) : dh;

			if(dh)
			{
				struct Rectangle r = { 0, layer->Height, layer->Width+dw-1, layer->Height+dh-1 };
				MyOrRectRegion(layer->DamageList, &r);
			}
		}

		if(layer->DamageList->RegionRectangle)
			layer->Flags |= LAYERREFRESH | LAYERIREFRESH | LAYERIREFRESH2;

		layer->bounds.MaxX += dw;
		layer->Width += dw;
		layer->bounds.MaxY += dh;
		layer->Height += dh;
	}

	if(dx || dy)
	{
		struct UserData udata();
		struct Layer *l = layer->LayerInfo->top_layer;
		udata.SetMask(l->rp->BitMap);
		while(l != layer)
		{
			udata.SubMask(l->bounds);
			l = l->back;
		}

		udata.dx = dx;
		udata.dy = dy;
		udata.bmp = layer->rp->BitMap;
		udata.l = layer;

		if(dw || dh)
			udata.ExportAsClipRect(layer);

		PrivateDoHookClipRects(layer->ClipRect, MoveLayerFunction, &udata, dx > 0, dy > 0);

		struct Rectangle old_bounds = layer->bounds;
		layer->bounds.MinX += dx;
		layer->bounds.MinY += dy;
		layer->bounds.MaxX += dx;
		layer->bounds.MaxY += dy;

//		if(!(dw || dh))
		{
			udata.ForEachInverse(old_bounds, AddMoveDamage, &udata);
			if(layer->DamageList->RegionRectangle)
				layer->Flags |= LAYERREFRESH | LAYERIREFRESH | LAYERIREFRESH2;
		}

		if(ABS(dx) >= layer->Width || ABS(dy) >= layer->Height)
		{
			BuildClipRects(layer->LayerInfo, layer->bounds);
			cm.AddMask(damage);
		}
		else
		{
			struct Rectangle rect1, rect2 = rect1 = org_frame;

			if(dx > 0)
			{
				rect1.MaxX = org_frame.MinX-1 + dx;
				damage.MaxX += dx;
			}
			else
			{
				rect1.MinX = org_frame.MaxX+1 + dx;
				if(dw < 0)
					rect1.MinX += dw;
				damage.MinX += dx;
			}

			if(dy > 0)
			{
				rect2.MaxY = org_frame.MinY+1 + dy;
				damage.MaxY += dy;
			}
			else
			{
				rect2.MinY = org_frame.MaxY+1 + dy;
				if(dh < 0)
					rect2.MinY += dh;
				damage.MinY += dy;
			}

			if(dx)
				cm.AddMask(rect1);
			if(dy)
				cm.AddMask(rect2);
		}
	}

	if(cm.Lines.First)
		BuildClipRects(layer, cm, damage);
	else if(dw || dh)
		BuildClipRects(layer, damage);

	if(dw > 0)
	{
		struct Rectangle rect = { layer->Width - dw, 0, layer->Width - 1, layer->Height - 1 };
		bDoHookClipRects(layer->BackFill, layer->rp, &rect);
	}

	if(dh > 0)
	{
		struct Rectangle rect = { 0, layer->Height - dh, layer->Width - (1+dw), layer->Height - 1 };
		bDoHookClipRects(layer->BackFill, layer->rp, &rect);
	}

	return TRUE;
}

struct Layer_Info *bNewLayerInfo ()
{
	struct Layer_Info *li = new Layer_Info();

	NewList((struct List *)&li->gs_Head);
	InitSemaphore(&li->Lock);

	return li;
}

struct Layer *bWhichLayer (struct Layer_Info *li, WORD x, WORD y)
{
	struct Layer *l = li->top_layer;
	while(l)
	{
		if(BETWEEN(x, l->bounds.MinX, l->bounds.MaxX) && BETWEEN(y, l->bounds.MinY, l->bounds.MaxY))
			break;

		l = l->back;
	}
//	D(("0x%lx\n", l))
	return l;
}

struct RasterScrollData
{
	WORD dx, dy;
	struct RastPort *rp;
	struct Layer *l;
	VOID(*filler)(struct RastPort *, struct Rectangle &);
};

VOID RasterScrollDamage2 (struct Rectangle *rect, APTR userdata)
{
	struct RasterScrollData *udata = (struct RasterScrollData *)userdata;
	struct Rectangle &b = udata->l->bounds;
	struct Rectangle new_rect = { rect->MinX - b.MinX, rect->MinY - b.MinY, rect->MaxX - b.MinX, rect->MaxY - b.MinY };
	MyOrRectRegion(udata->l->DamageList, &new_rect);
	udata->filler(udata->rp, new_rect);
//	BackFillLayer(rect, udata->l);
}

VOID RasterScrollDamage1 (struct Rectangle *rect, APTR userdata)
{
	struct RasterScrollData *udata = (struct RasterScrollData *)userdata;
	struct Rectangle new_rect = { rect->MinX - udata->dx, rect->MinY - udata->dy, rect->MaxX - udata->dx, rect->MaxY - udata->dy };
	PrivateDoHookClipRects(new_rect, udata->l, RasterScrollDamage2, userdata);
}

VOID RasterScrollFunction2 (struct Rectangle *rect, APTR userdata)
{
	struct RasterScrollData *udata = (struct RasterScrollData *)userdata;
	BltBitMap(udata->rp->BitMap,
			rect->MinX + udata->dx, rect->MinY + udata->dy,
			udata->rp->BitMap,
			rect->MinX, rect->MinY,
			rect->MaxX - rect->MinX + 1, rect->MaxY - rect->MinY + 1, 0xc0, udata->l->rp->Mask, NULL);
}

VOID RasterScrollFunction1 (struct Rectangle *rect, APTR userdata)
{
	struct RasterScrollData *udata = (struct RasterScrollData *)userdata;
	struct Rectangle new_rect = { rect->MinX - udata->dx, rect->MinY - udata->dy, rect->MaxX - udata->dx, rect->MaxY - udata->dy };
	PrivateDoHookClipRects(new_rect, udata->l, RasterScrollFunction2, userdata, udata->dx < 0, udata->dy < 0);
}

VOID ScrollDamageList (struct Layer *l, WORD dx, WORD dy, WORD minx, WORD miny, WORD maxx, WORD maxy)
{
//	D(("ScrollDamage: 0x%lx\n", l->DamageList->RegionRectangle))
	if(l->DamageList && l->DamageList->RegionRectangle)
	{
		struct Rectangle *r = &l->DamageList->bounds, rect = { minx, miny, maxx, maxy };
		if(RectOverlap(*r, rect))
		{
//			D(("ScrollDamage: %ld, %ld, %ld, %ld\n", r->MinX, r->MinY, r->MaxX, r->MaxY))
//			LONG minx = -1, maxx = 0, miny = -1, maxy = 0;

			rect.MinX -= r->MinX;
			rect.MaxY -= r->MinY;
			rect.MinX -= r->MinX;
			rect.MaxY -= r->MinY;

			r->MinX -= dx;
			r->MinY -= dy;
			r->MaxX -= dx;
			r->MaxY -= dy;

			struct RegionRectangle *first = l->DamageList->RegionRectangle;
			while(first)
			{
				struct Rectangle *r = &first->bounds;
				if(!RectOverlap(*r, rect))
				{
					r->MinX += dx;
					r->MinY += dy;
					r->MaxX += dx;
					r->MaxY += dy;
				}
				first = first->Next;
			}
		}
	}
}

VOID bScrollRaster (struct RastPort *rp, WORD dx, WORD dy, WORD minx, WORD miny, WORD maxx, WORD maxy, VOID(*filler)(struct RastPort *, struct Rectangle &))
{
	if(dx > 0)
			minx += dx;
	else	maxx += dx;

	if(dy > 0)
			miny += dy;
	else	maxy += dy;

	struct Layer *l;
	if(l = rp->Layer)
	{
		ScrollDamageList(l, dx, dy, minx, miny, maxx, maxy);

		struct RasterScrollData userdata = { dx, dy, rp, l, filler };
		struct Rectangle bounds = { minx + l->bounds.MinX, miny + l->bounds.MinY, maxx + l->bounds.MinX, maxy + l->bounds.MinY };
		PrivateDoHookClipRects(bounds, l, RasterScrollFunction1, &userdata, dx < 0, dy < 0);

		if(l->Flags & LAYERSIMPLE)
		{
			PrivateDoHookForInverseClipRects(bounds, l, RasterScrollDamage1, &userdata);
			if(l->DamageList->RegionRectangle)
				l->Flags |= LAYERREFRESH | LAYERIREFRESH | LAYERIREFRESH2;
		}
	}
	else
	{
		BltBitMap(rp->BitMap, minx, miny, rp->BitMap, minx+dx, miny+dy, maxx-minx + 1, maxy-miny + 1, 0xc0, rp->Mask, NULL);
	}

	struct Rectangle rect = { minx, miny, maxx, maxy };
	if(dx > 0)
	{
		rect.MinX = maxx-(dx-1);
		filler(rp, rect);
		rect.MinX = minx;
	}
	else if(dx < 0)
	{
		rect.MaxX = minx-(dx+1);
		filler(rp, rect);
		rect.MaxX = maxx;
	}

	if(dy > 0)
	{
		rect.MinY = maxy-(dy-1);
		filler(rp, rect);
	}
	else if(dy < 0)
	{
		rect.MaxY = miny-(dy+1);
		filler(rp, rect);
	}
}

struct ClipBlitData
{
	BOOL reverse_x, reverse_y;
	WORD dx, dy;
	struct BitMap *srcbmp;
	struct RastPort *dstrp;
	struct Layer *dstlayer;
	UBYTE minterm;
};

VOID ClipBlitFunction2 (struct Rectangle *rect, APTR userdata)
{
	struct ClipBlitData *udata = (struct ClipBlitData *)userdata;
	BltBitMap(udata->srcbmp,
			rect->MinX + udata->dx, rect->MinY + udata->dy,
			udata->dstrp->BitMap,
			rect->MinX, rect->MinY,
			rect->MaxX - rect->MinX + 1, rect->MaxY - rect->MinY + 1, udata->minterm, udata->dstrp->Mask, NULL);
}

VOID ClipBlitFunction1 (struct Rectangle *rect, APTR userdata)
{
	struct ClipBlitData *udata = (struct ClipBlitData *)userdata;
	struct Rectangle new_rect = { rect->MinX - udata->dx, rect->MinY - udata->dy, rect->MaxX - udata->dx, rect->MaxY - udata->dy };
	PrivateDoHookClipRects(new_rect, udata->dstlayer, ClipBlitFunction2, userdata, udata->reverse_x, udata->reverse_y);
}

VOID bClipBlit (struct RastPort *srcrp, WORD srcx, WORD srcy, struct RastPort *dstrp, WORD dstx, WORD dsty, WORD width, WORD height, UBYTE minterm)
{
	struct Layer *s = srcrp->Layer, *d = dstrp->Layer;
	struct Rectangle bounds = { srcx, srcy, srcx + width - 1, srcy + height - 1 };

	BOOL double_lock = s && d && s != d && s->LayerInfo == d->LayerInfo;
	if(double_lock)
		pLockLayerInfo(s->LayerInfo);

	if(s)
	{
 		pLockLayer(0, s);
		bounds.MinX += s->bounds.MinX;
		bounds.MinY += s->bounds.MinY;
		bounds.MaxX += s->bounds.MinX;
		bounds.MaxY += s->bounds.MinY;
	}

	BOOL reverse_x = srcx < dstx, reverse_y = srcy < dsty;
	struct ClipBlitData userdata = { reverse_x, reverse_y, bounds.MinX - dstx, bounds.MinY - dsty, srcrp->BitMap, dstrp, d, minterm };

	if(d)
	{
		pLockLayer(0, d);
	 	userdata.dx -= d->bounds.MinX;
	 	userdata.dy -= d->bounds.MinY;
	}

	if(double_lock)
		pUnlockLayerInfo(s->LayerInfo);

	PrivateDoHookClipRects(bounds, s, ClipBlitFunction1, &userdata, reverse_x, reverse_y);

	if(d)	pUnlockLayer(d);
	if(s)	pUnlockLayer(s);
}
