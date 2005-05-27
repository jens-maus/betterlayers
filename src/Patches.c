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
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <exec/memory.h>
#include <time.h>

#pragma header

#include "BetterLayers.h"
#include "Debug.h"
#include "PatchProtos.h"

//#define MYDEBUG

#ifdef MYDEBUG
VOID _INIT_9_DebugIO ()
{
	//kprintf("----------\n");

	struct tm *tp;
	time_t t;

	time(&t);
	tp = localtime(&t);
	//kprintf("Layer.library patch started: %s\n", asctime(tp));
}

#endif

STRPTR TaskName (struct Task *task);

VOID Enter (struct Layer_Info *li)
{
//	pLockLayerInfo(li);

#ifdef MYDEBUG
	struct Task *task = FindTask(NULL);
	//kprintf("%s: Stack (%04ld / %04ld)\n", TaskName(task), ((ULONG)task->tc_SPReg) - (ULONG)task->tc_SPLower, ((ULONG)task->tc_SPUpper) - (ULONG)task->tc_SPLower);

	if(debug)
	{
		kprintf("%s: Got layerinfo\n", TaskName(FindTask(NULL)));

		struct Task *this_task = FindTask(NULL);
		struct Layer *first = li->top_layer;
		while(first)
		{
			if(first->Lock.ss_NestCount && first->Lock.ss_Owner != this_task)
				kprintf("\tL: 0x%lx is owned by '%s'\n", first, TaskName(first->Lock.ss_Owner));
			first = first->back;
		}
	}
#endif
	pLockLayers(li);
#ifdef MYDEBUG
	if(debug)
		kprintf("%s: Got all layers\n", TaskName(FindTask(NULL)));
#endif
}

VOID Leave (struct Layer_Info *li)
{
	pUnlockLayers(li);
//	pUnlockLayerInfo(li);
#ifdef MYDEBUG
	if(debug)
		kprintf("%s: No longer owns layerinfo\n", TaskName(FindTask(NULL)));
#endif
}

void pInitLayers (REG(a0) struct Layer_Info *li)
{
	D(DBF_OBSOLETE, ("LayerInfo 0x%lx", li))

	memset(li, 0, sizeof(struct Layer_Info));
	NewList((struct List *)&li->gs_Head);
	InitSemaphore(&li->Lock);
}

struct Layer *pCreateUpfrontLayer (REG(a0) struct Layer_Info *li, REG(a1) struct BitMap *bm, REG(d0) long x0, REG(d1) long y0, REG(d2) long x1, REG(d3) long y1, REG(d4) long flags, REG(a2) struct BitMap *bm2)
{
	D(DBF_LAYERS, ("LayerInfo: 0x%lx", li))
	struct Layer *res = pCreateUpfrontHookLayer(li, bm, x0, y0, x1, y1, flags, LAYERS_BACKFILL, bm2);
	return res;
}

struct Layer *pCreateBehindLayer (REG(a0) struct Layer_Info *li, REG(a1) struct BitMap *bm, REG(d0) long x0, REG(d1) long y0, REG(d2) long x1, REG(d3) long y1, REG(d4) long flags, REG(a2) struct BitMap *bm2)
{
	D(DBF_LAYERS, ("LayerInfo: 0x%lx", li))
	struct Layer *res = pCreateBehindHookLayer(li, bm, x0, y0, x1, y1, flags, LAYERS_BACKFILL, bm2);
	return res;
}

VOID waste_time () { ; }

VOID FlashLayer (struct Layer *layer)
{
	if(pBeginUpdate(layer))
	{
		ULONG drmd = GetDrMd(layer->rp);
		SetDrMd(layer->rp, COMPLEMENT);
		for(UWORD i = 0; i < 10; i++)
		{
			RectFill(layer->rp, 0, 0, layer->Width-1, layer->Height-1);
			if(FindTask(NULL)->tc_Node.ln_Type == NT_PROCESS)
				Delay(1);
			else
			{
				for(UWORD j = 0; j < 50000; j++)
					waste_time();
			}
		}
		SetDrMd(layer->rp, drmd);
		pEndUpdate(layer, FALSE);
	}
}

LONG pUpfrontLayer (REG(a0) long dummy, REG(a1) struct Layer *layer)
{
	Enter(layer->LayerInfo);

	struct Layer *target;
	if(layer->Flags & LAYERBACKDROP) // actually, this operation isn't supported according to autodoc
	{
		target = layer;
		while(target->front && target->front->Flags & LAYERBACKDROP)
			target = target->front;
	}
	else
	{
		target = layer->LayerInfo->top_layer;
	}

	LONG res = bMoveLayerInFrontOf(layer, target);

	Leave(layer->LayerInfo);
	return res;
}

LONG pBehindLayer (REG(a0) long dummy, REG(a1) struct Layer *layer)
{
	Enter(layer->LayerInfo);
	D(DBF_LAYERS, ("Layer: 0x%lx", layer))

	struct Layer *target;
	if(layer->Flags & LAYERBACKDROP)
	{
		target = NULL;
	}
	else
	{
		target = layer;
		if(target->back)
		{
			do {
				target = target->back;
			} while(target && !(target->Flags & LAYERBACKDROP));
		}
	}

	LONG res = bMoveLayerInFrontOf(layer, target);

	Leave(layer->LayerInfo);
	return res;
}

LONG pMoveLayer (REG(a0) long dummy, REG(a1) struct Layer *layer, REG(d0) long dx, REG(d1) long dy)
{
	D(DBF_OBSOLETE, ("Layer 0x%lx (obsolete?)", layer))
	return pMoveSizeLayer(layer, dx, dy, 0, 0);
}

LONG pSizeLayer (REG(a0) long dummy, REG(a1) struct Layer *layer, REG(d0) long dw, REG(d2) long dh)
{
	D(DBF_OBSOLETE, ("Layer 0x%lx (obsolete?)", layer))
	return pMoveSizeLayer(layer, 0, 0, dw, dh);
}

void pScrollLayer (REG(a0) long dummy, REG(a1) struct Layer *layer, REG(d0) long dx, REG(d1) long dy)
{
	//kprintf("ScrollLayer()\n");
}

LONG pBeginUpdate (REG(a0) struct Layer *l)
{
	pLockLayer(0, l);
	//kprintf("BeginUpdate(0x%08lx)\n", l);
	if(l->ClipRegion)
		bEndUpdate(l);

	l->Flags |= LAYERUPDATING;
	LONG res = bBeginUpdate(l);
	return res;
}

void pEndUpdate (REG(a0) struct Layer *layer, REG(d0) unsigned long flag)
{
	if(flag)
		ClearRegion(layer->DamageList);

	//kprintf("EndUpdate(0x%08lx, %ld)\n", layer, flag);
	bEndUpdate(layer);
	layer->Flags &= ~LAYERUPDATING;

	if(layer->ClipRegion)
		bBeginUpdate(layer);

	pUnlockLayer(layer);
}

LONG pDeleteLayer (REG(a0) long dummy, REG(a1) struct Layer *layer)
{
	D(DBF_LAYERS, ("Layer: 0x%lx", layer))

	struct Layer_Info *li;
	Enter(li = layer->LayerInfo);
	LONG res = bDeleteLayer(dummy, layer);
	Leave(li);

	return res;
}

#if 0
void pLockLayer (REG(a0) long dummy, REG(a1) struct Layer *layer)		{ ; }
void pUnlockLayer (REG(a0) struct Layer *layer)								{ ; }
void pLockLayers (REG(a0) struct Layer_Info *li)							{ ; }
void pUnlockLayers (REG(a0) struct Layer_Info *li)							{ ; }
void pLockLayerInfo (REG(a0) struct Layer_Info *li)						{ ; }
void pUnlockLayerInfo (REG(a0) struct Layer_Info *li)						{ ; }
#else

#ifdef MYDEBUG
STRPTR TaskName (struct Task *task)
{
	STRPTR res = NULL;;
	if(task)
	{
		res = task->tc_Node.ln_Name;
		if(task->tc_Node.ln_Type == NT_PROCESS)
		{
			struct Process *p = (struct Process *)task;
			if(p->pr_CLI)
			{
				struct CommandLineInterface *cli = (struct CommandLineInterface *)(p->pr_CLI << 2);
				STRPTR name = (STRPTR)(cli->cli_CommandName << 2);
			}
		}
	}

	return res ? res : "none";
}
#endif

void pLockLayer (REG(a0) long dummy, REG(a1) struct Layer *layer)
{
	//kprintf("LockLayer(0x%08lx)\n", layer);
	struct Layer_Info *li = layer->LayerInfo;

#ifdef MYDEBUG
	struct Task *this_task = FindTask(NULL);
	if(this_task != li->Lock.ss_Owner && li->Lock.ss_Owner)
	{
		struct Layer *first = li->top_layer;
		LONG cnt = 0;
		while(first)
		{
			if(first->Lock.ss_Owner == this_task && first != layer)
				cnt++;
			first = first->back;
		}

		if(cnt)
			kprintf("%s already owns %ld layer(s) (LayerInfo is owned by %s)\n", TaskName(this_task), cnt, TaskName(li->Lock.ss_Owner));
	}

	STRPTR other = TaskName(layer->Lock.ss_Owner);
	STRPTR layerinfo = TaskName(li->Lock.ss_Owner);
	if(!AttemptSemaphore(&layer->Lock))
	{
		//kprintf("Potential deadlock from '%s', layer is currently owned by '%s' - LayerInfo is owned by '%s'\n", TaskName(this_task), other, layerinfo);
		ObtainSemaphore(&layer->Lock);
		//kprintf("%s got the semaphore\n", TaskName(this_task));
	}
#else
	ObtainSemaphore(&layer->Lock);
#endif
}

void pUnlockLayer (REG(a0) struct Layer *layer)
{
	//kprintf("UnlockLayer(0x%08lx)\n", layer);
	ReleaseSemaphore(&layer->Lock);
}

void pLockLayers (REG(a0) struct Layer_Info *li)
{
	//kprintf("LockLayers(0x%08lx, %ld)\n", li, li->LockLayersCount);
	pLockLayerInfo(li);
	ObtainSemaphoreList((struct List *)&li->gs_Head);
	if(++li->LockLayersCount == 1)
	{
		for(struct Layer *l = li->top_layer; l; l = l->back)
		{
			//kprintf("\tLayer lock cnt: %ld (0x%lx)\n", l->Lock.ss_NestCount, l);
			if(l->ClipRegion)
				bEndUpdate(l);
		}
	}
}

void pUnlockLayers (REG(a0) struct Layer_Info *li)
{
	//kprintf("UnlockLayers(0x%08lx, %ld)\n", li, li->LockLayersCount);
	if(--li->LockLayersCount == 0)
	{
		for(struct Layer *l = li->top_layer; l; l = l->back)
		{
			//kprintf("\tLayer lock cnt: %ld (0x%lx)\n", l->Lock.ss_NestCount, l);
			if(l->ClipRegion)
				bBeginUpdate(l);
		}
	}

	ReleaseSemaphoreList((struct List *)&li->gs_Head);
	pUnlockLayerInfo(li);
}

void pLockLayerInfo (REG(a0) struct Layer_Info *li)
{
	//kprintf("LockLayerInfo(0x%08lx)\n", li);

#ifdef MYDEBUG
	struct Task *this_task = FindTask(NULL);
	if(this_task != li->Lock.ss_Owner)
	{
		struct Layer *first = li->top_layer;
		LONG cnt = 0;
		while(first)
		{
			if(first->Lock.ss_Owner == this_task)
				cnt++;
			first = first->back;
		}

		if(cnt)
			kprintf("%s already owns %ld layer(s), when trying to obtain LayerInfo\n", TaskName(this_task), cnt);
	}

	STRPTR layerinfo = TaskName(li->Lock.ss_Owner);
	if(!AttemptSemaphore(&li->Lock))
	{
		//kprintf("Potential deadlock from '%s', layerInfo is currently owned by '%s'\n", TaskName(this_task), layerinfo);
		ObtainSemaphore(&li->Lock);
		//kprintf("%s got the layerinfo semaphore\n", TaskName(this_task));
	}
#else
	ObtainSemaphore(&li->Lock);
#endif
}

void pUnlockLayerInfo (REG(a0) struct Layer_Info *li)
{
	//kprintf("UnlockLayerInfo(0x%08lx)\n", li);
	ReleaseSemaphore(&li->Lock);
}
#endif

void pSwapBitsRastPortClipRect (REG(a0) struct RastPort *rp, REG(a1) struct ClipRect *cr)
{
	//kprintf("SwapBitsRastPortClipRect()\n");
}

struct Layer *pWhichLayer (REG(a0) struct Layer_Info *li, REG(d0) long x, REG(d1) long y)
{
	struct Layer *res = bWhichLayer(li, x, y);
	return res;
}

struct Layer_Info *pNewLayerInfo (void)
{
	struct Layer_Info *res = bNewLayerInfo();
	D(DBF_LAYERS, ("LayerInfo 0x%lx", res))
	return res;
}

void pDisposeLayerInfo (REG(a0) struct Layer_Info *li)
{
	D(DBF_LAYERS, ("LayerInfo 0x%lx", li))
	bDisposeLayerInfo(li);
}

LONG pFattenLayerInfo (REG(a0) struct Layer_Info *li)
{
	D(DBF_OBSOLETE, ("LayerInfo 0x%lx", li))
	return TRUE; /* ??? */
}

void pThinLayerInfo (REG(a0) struct Layer_Info *li)
{
	D(DBF_OBSOLETE, ("LayerInfo 0x%lx", li))
}

LONG pMoveLayerInFrontOf (REG(a0) struct Layer *layer_to_move, REG(a1) struct Layer *other_layer)
{
	D(DBF_LAYERS, ("0x%lx in front of 0x%lx", layer_to_move, other_layer))

	Enter(layer_to_move->LayerInfo);
	LONG res = bMoveLayerInFrontOf(layer_to_move, other_layer);
	Leave(layer_to_move->LayerInfo);
	return res;
}

struct Region *pInstallClipRegion (REG(a0) struct Layer *layer, REG(a1) struct Region *region)
{
	pLockLayer(0, layer);
	struct Region *res = bInstallClipRegion(layer, region);
	//kprintf("InstallClipRegion(0x%08lx, 0x%08lx) = 0x%08lx\n", layer, region, res);
	pUnlockLayer(layer);
	return res;
}

LONG pMoveSizeLayer (REG(a0) struct Layer *layer, REG(d0) long dx, REG(d1) long dy, REG(d2) long dw, REG(d3) long dh)
{
	D(DBF_LAYERS, ("0x%08lx, %ld, %ld, %ld, %ld", layer, dx, dy, dw, dh))

	Enter(layer->LayerInfo);
	LONG res = bMoveSizeLayer(layer, dx, dy, dw, dh);
	Leave(layer->LayerInfo);
	return res;
}

struct Layer *pCreateUpfrontHookLayer (REG(a0) struct Layer_Info *li, REG(a1) struct BitMap *bm, REG(d0) long x0, REG(d1) long y0, REG(d2) long x1, REG(d3) long y1, REG(d4) long flags, REG(a3) struct Hook *hook, REG(a2) struct BitMap *bm2)
{
	D(DBF_LAYERS, ("LayerInfo: 0x%lx", li))
	if(bm2)	D(DBF_IMPORTANT, ("SuperBitMap is not supported"))

	Enter(li);
	struct Layer *res = bCreateUpfrontHookLayer(li, bm, x0, y0, x1, y1, flags, hook, bm2);
	for(UWORD i = 0; i < li->LockLayersCount; i++)
		pLockLayer(0, res);
	Leave(li);

	WaitBlit();
	return res;
}

/* Intuition call this function when opening a window */
struct Layer *pCreateBehindHookLayer (REG(a0) struct Layer_Info *li, REG(a1) struct BitMap *bm, REG(d0) long x0, REG(d1) long y0, REG(d2) long x1, REG(d3) long y1, REG(d4) long flags, REG(a3) struct Hook *hook, REG(a2) struct BitMap *bm2)
{
	D(DBF_LAYERS, ("LayerInfo: 0x%lx", li))
	if(bm2)	D(DBF_IMPORTANT, ("SuperBitMap is not supported"))

	Enter(li);
	struct Layer *res = bCreateBehindHookLayer(li, bm, x0, y0, x1, y1, flags, hook, bm2);
	for(UWORD i = 0; i < li->LockLayersCount; i++)
		pLockLayer(0, res);
	Leave(li);

	WaitBlit();
	return res;
}

struct Hook *pInstallLayerHook (REG(a0) struct Layer *layer, REG(a1) struct Hook *hook)
{
	D(DBF_LAYERS, ("Layer 0x%lx, hook 0x%lx", layer, hook))

	struct Hook *old_hook = NULL;
	if(layer)
	{
		pLockLayer(0, layer);
		old_hook = layer->BackFill;
		layer->BackFill = hook;
		pUnlockLayer(layer);
	}

	return old_hook;
}

struct Hook *pInstallLayerInfoHook (REG(a0) struct Layer_Info *li, REG(a1) struct Hook *hook)
{
	D(DBF_LAYERS, ("LayerInfo 0x%lx, hook 0x%lx", li, hook))

	Enter(li);
	struct Hook *oldHook = (struct Hook *)li->BlankHook;
	li->BlankHook = hook;
	Leave(li);
	return oldHook;
}

void pSortLayerCR (REG(a0) struct Layer *layer, REG(d0) long dx, REG(d1) long dy)
{
	D(DBF_OBSOLETE, ("Layer 0x%lx, dx %ld, dy %ld", layer, dx, dy))
}

void pDoHookClipRects (REG(a0) struct Hook *hook, REG(a1) struct RastPort *rport, REG(a2) struct Rectangle *rect)
{
	bDoHookClipRects(hook, rport, rect);
}

VOID RectFillStub (struct RastPort *rp, struct Rectangle &rect)
{
	RectFill(rp, rect.MinX, rect.MinY, rect.MaxX, rect.MaxY);
}

void pScrollRaster (REG(a1) struct RastPort *rp, REG(d0) WORD dx, REG(d1) WORD dy, REG(d2) WORD xmin, REG(d3) WORD ymin, REG(d4) WORD xmax, REG(d5) WORD ymax)
{
	D(DBF_GRAPHICS, ("RP 0x%lx, dx %ld, dy %ld", rp, dx, dy))

	if(rp->Layer)
		pLockLayer(0, rp->Layer);

	ULONG apen = GetAPen(rp);
	SetAPen(rp, rp->BgPen);
	bScrollRaster(rp, dx, dy, xmin, ymin, xmax, ymax, RectFillStub);
	SetAPen(rp, apen);

	if(rp->Layer)
		pUnlockLayer(rp->Layer);
}

VOID EraseRectStub (struct RastPort *rp, struct Rectangle &rect)
{
	EraseRect(rp, rect.MinX, rect.MinY, rect.MaxX, rect.MaxY);
}

void pScrollRasterBF (REG(a1) struct RastPort *rp, REG(d0) WORD dx, REG(d1) WORD dy, REG(d2) WORD xmin, REG(d3) WORD ymin, REG(d4) WORD xmax, REG(d5) WORD ymax)
{
	D(DBF_GRAPHICS, ("RP 0x%lx, dx %ld, dy %ld", rp, dx, dy))

	if(rp->Layer)
		pLockLayer(0, rp->Layer);

	bScrollRaster(rp, dx, dy, xmin, ymin, xmax, ymax, EraseRectStub);

	if(rp->Layer)
		pUnlockLayer(rp->Layer);
}

void pClipBlit (REG(a0) struct RastPort *rp, REG(d0) WORD srcx, REG(d1) WORD srcy, REG(a1) struct RastPort *dstrp, REG(d2) WORD dstx, REG(d3) WORD dsty, REG(d4) WORD width, REG(d5) WORD height, REG(d6) UBYTE minterm)
{
	D(DBF_GRAPHICS, ("RP 0x%lx/0x%lx, x %ld -> %ld, y %ld -> %ld (%ld, %ld)", rp, dstrp, srcx, dstx, srcy, dsty, width, height))
	bClipBlit(rp, srcx, srcy, dstrp, dstx, dsty, width, height, minterm);
}
