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
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/intuition.h>
#include <intuition/intuitionbase.h>

#pragma header

#include "BetterLayers.h"
#include "Debug.h"
#include "PatchProtos.h"

APTR Patches[] =
{
	pInitLayers,
	pCreateUpfrontLayer,
	pCreateBehindLayer,
	pUpfrontLayer,
	pBehindLayer,
	pMoveLayer,
	pSizeLayer,
	pScrollLayer,
	pBeginUpdate,
	pEndUpdate,
	pDeleteLayer,
	pLockLayer,
	pUnlockLayer,
	pLockLayers,
	pUnlockLayers,
	pLockLayerInfo,
	pSwapBitsRastPortClipRect,
	pWhichLayer,
	pUnlockLayerInfo,
	pNewLayerInfo,
	pDisposeLayerInfo,
	pFattenLayerInfo,
	pThinLayerInfo,
	pMoveLayerInFrontOf,
	pInstallClipRegion,
	pMoveSizeLayer,
	pCreateUpfrontHookLayer,
	pCreateBehindHookLayer,
	pInstallLayerHook,
	pInstallLayerInfoHook,
	pSortLayerCR,
	pDoHookClipRects,
	NULL,
};

#define LVO_ScrollRaster				-396
#define LVO_ClipBlit						-552
#define LVO_ScrollRasterBF				-1002
#define LVO_LockLayerRom				-432
#define LVO_UnlockLayerRom				-438

#define LVO_CreateUpfrontLayer			-36
#define LVO_CreateBehindLayer				-42
#define LVO_DeleteLayer						-90
#define LVO_LockLayer						-96
#define LVO_UnlockLayer						-102
#define LVO_LockLayers						-108
#define LVO_UnlockLayers					-114
#define LVO_LockLayerInfo					-120
#define LVO_SwapBitsRastPortClipRect	-126
#define LVO_WhichLayer						-132
#define LVO_UnlockLayerInfo				-138
#define LVO_CreateUpfrontHookLayer		-186
#define LVO_CreateBehindHookLayer		-192
#define LVO_DoHookClipRects				-216


VOID InstallPatch ()
{
	Forbid();

	APTR *patches = Patches;
	LONG offset = -30;
	while(*patches)
	{
		switch(offset)
		{
/*			case LVO_LockLayer:
			case LVO_UnlockLayer:
			case LVO_LockLayerInfo:
			case LVO_UnlockLayerInfo:
*/
/*			case LVO_LockLayers:
			case LVO_UnlockLayers:
			case LVO_CreateUpfrontLayer:
			case LVO_CreateUpfrontHookLayer:
			case LVO_CreateBehindLayer:
			case LVO_CreateBehindHookLayer:
			case LVO_DeleteLayer:
			case LVO_DoHookClipRects:
*/
			case LVO_SwapBitsRastPortClipRect:
//			case LVO_WhichLayer:
			break;

			default:
				SetFunction(LayersBase, offset, (ULONG(*)())*patches);
			break;
		}
		patches++;
		offset -= 6;
	}

	SetFunction((struct Library *)GfxBase, LVO_LockLayerRom, (ULONG(*)())pLockLayerRom);
	SetFunction((struct Library *)GfxBase, LVO_UnlockLayerRom, (ULONG(*)())pUnlockLayerRom);
	SetFunction((struct Library *)GfxBase, LVO_ScrollRaster, (ULONG(*)())pScrollRaster);
	SetFunction((struct Library *)GfxBase, LVO_ScrollRasterBF, (ULONG(*)())pScrollRasterBF);
	SetFunction((struct Library *)GfxBase, LVO_ClipBlit, (ULONG(*)())pClipBlit);

	Permit();
}
/*
VOID SwapLayers (struct Layer_Info *li, struct Screen *scr)
{
	struct Layer *l = scr->LayerInfo.top_layer;
	while(l->back)
		l = l->back;

	while(l)
	{
#if 0
		printf("L %p, RP %p\n", l, l->rp);

		struct Window *win;
		if(win = (struct Window *)l->Window)
			printf("W %p, L %p, RP %p, RP->L %p, BorderRPort %p, BorderRPort->L %p\n", win, win->WLayer, win->RPort, win->RPort->Layer, win->BorderRPort, win->BorderRPort ? win->BorderRPort->Layer : NULL);

		if(l == scr->BarLayer)
			printf("BarLayer!\n");
#else
		struct Layer *new_l = pCreateUpfrontHookLayer(li, l->rp->BitMap, l->bounds.MinX, l->bounds.MinY, l->bounds.MaxX, l->bounds.MaxY, l->Flags, l->BackFill, NULL);

		l->rp->Layer = new_l;
		new_l->rp = l->rp;
		if(new_l->Window = l->Window)
		{
			struct Window *win = (struct Window *)l->Window;
			win->RPort->Layer = new_l;
			win->WLayer = new_l;
			if(win->BorderRPort)
				win->BorderRPort->Layer = new_l;
		}

		if(l == scr->BarLayer)
			scr->BarLayer = l->rp->Layer;
#endif
		l = l->front;
	}
}

VOID Swap (struct Screen *scr)
{
	Forbid();
	while(scr)
	{
//		printf("Scr %p, RP %p, L %p\n", scr, &scr->RastPort, scr->RastPort.Layer);
		struct Layer_Info *li = pNewLayerInfo();
		SwapLayers(li, scr);
//		pDisposeLayerInfo(li);
		scr->LayerInfo = *li;
		scr = scr->NextScreen;
	}
	Permit();
}

struct Region *(*Old)(REG(a0) struct Layer *, REG(a1) struct Region *, REG(a6) struct Library *);

struct Region *Dummy (REG(a0) struct Layer *layer, REG(a1) struct Region *region)
{
	struct Region *res = Old(layer, region, LayersBase);
	kprintf("InstallClipRegion(0x%08lx, 0x%08lx) = 0x%08lx\n", layer, region, res);
	return res;
}
*/

VOID DumpMemoryInfo ();

VOID main ()
{
/*	if(LayersBase->lib_OpenCnt > 1)
		printf("Error: Open count is %d\n", LayersBase->lib_OpenCnt);
	else if(IntuitionBase->ActiveScreen)
		printf("Error: Open screen is %p\n", IntuitionBase->ActiveScreen);
	else*/

//	Swap(IntuitionBase->ActiveScreen);

	//kprintf("----------\n");

	InstallPatch();

#ifdef DEBUG
	BOOL running = TRUE;
	while(running)
	{
		ULONG mask = Wait(SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);

		if(mask & SIGBREAKF_CTRL_C)
		{
			DumpMemoryInfo();
//			running = FALSE;
		}

		if(mask & SIGBREAKF_CTRL_D)
			ReadDebugFlags();
	}
#else
	Wait(0);
#endif

/*	APTR old = SetFunction(LayersBase, -216, (ULONG(*)())pDoHookClipRects);
	Wait(4096);
	SetFunction(LayersBase, -216, (ULONG(*)())old);
*/
/*	Old = (struct Region *(*)(REG(a0) struct Layer *, REG(a1) struct Region *, REG(a6) struct Library *))SetFunction(LayersBase, -174, (ULONG(*)())Dummy);
	Wait(4096);
	SetFunction(LayersBase, -174, (ULONG(*)())Old);
*/
}
