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

#ifndef PATCH_PROTOS_H
#define PATCH_PROTOS_H

/* Graphics.library */

void pScrollRaster (REG(a1) struct RastPort *rp, REG(d0) WORD dx, REG(d1) WORD dy, REG(d2) WORD xmin, REG(d3) WORD ymin, REG(d4) WORD xmax, REG(d5) WORD ymax);
void pScrollRasterBF (REG(a1) struct RastPort *rp, REG(d0) WORD dx, REG(d1) WORD dy, REG(d2) WORD xmin, REG(d3) WORD ymin, REG(d4) WORD xmax, REG(d5) WORD ymax);
void pClipBlit (REG(a0) struct RastPort *rp, REG(d0) WORD srcx, REG(d1) WORD srcy, REG(a1) struct RastPort *dstrp, REG(d2) WORD dstx, REG(d3) WORD dsty, REG(d4) WORD width, REG(d5) WORD height, REG(d6) UBYTE minterm);

extern "C" void pLockLayerRom (/* REG(a5) struct Layer *layer */);
extern "C" void pUnlockLayerRom (/* REG(a5) struct Layer *layer */);

/* Layers.library */

void pInitLayers (REG(a0) struct Layer_Info *li);
struct Layer *pCreateUpfrontLayer (REG(a0) struct Layer_Info *li, REG(a1) struct BitMap *bm, REG(d0) long x0, REG(d1) long y0, REG(d2) long x1, REG(d3) long y1, REG(d4) long flags, REG(a2) struct BitMap *bm2);
struct Layer *pCreateBehindLayer (REG(a0) struct Layer_Info *li, REG(a1) struct BitMap *bm, REG(d0) long x0, REG(d1) long y0, REG(d2) long x1, REG(d3) long y1, REG(d4) long flags, REG(a2) struct BitMap *bm2);
LONG pUpfrontLayer (REG(a0) long dummy, REG(a1) struct Layer *layer);
LONG pBehindLayer (REG(a0) long dummy, REG(a1) struct Layer *layer);
LONG pMoveLayer (REG(a0) long dummy, REG(a1) struct Layer *layer, REG(d0) long dx, REG(d1) long dy);
LONG pSizeLayer (REG(a0) long dummy, REG(a1) struct Layer *layer, REG(d0) long dw, REG(d2) long dh);
void pScrollLayer (REG(a0) long dummy, REG(a1) struct Layer *layer, REG(d0) long dx, REG(d1) long dy);
LONG pBeginUpdate (REG(a0) struct Layer *l);
void pEndUpdate (REG(a0) struct Layer *layer, REG(d0) unsigned long flag);
LONG pDeleteLayer (REG(a0) long dummy, REG(a1) struct Layer *layer);
void pLockLayer (REG(a0) long dummy, REG(a1) struct Layer *layer);
void pUnlockLayer (REG(a0) struct Layer *layer);
void pLockLayers (REG(a0) struct Layer_Info *li);
void pUnlockLayers (REG(a0) struct Layer_Info *li);
void pLockLayerInfo (REG(a0) struct Layer_Info *li);
void pSwapBitsRastPortClipRect (REG(a0) struct RastPort *rp, REG(a1) struct ClipRect *cr);
struct Layer *pWhichLayer (REG(a0) struct Layer_Info *li, REG(d0) long x, REG(d1) long y);
void pUnlockLayerInfo (REG(a0) struct Layer_Info *li);
struct Layer_Info *pNewLayerInfo (void);
void pDisposeLayerInfo (REG(a0) struct Layer_Info *li);
LONG pFattenLayerInfo (REG(a0) struct Layer_Info *li);
void pThinLayerInfo (REG(a0) struct Layer_Info *li);
LONG pMoveLayerInFrontOf (REG(a0) struct Layer *layer_to_move, REG(a1) struct Layer *other_layer);
struct Region *pInstallClipRegion (REG(a0) struct Layer *layer, REG(a1) struct Region *region);
LONG pMoveSizeLayer (REG(a0) struct Layer *layer, REG(d0) long dx, REG(d1) long dy, REG(d2) long dw, REG(d3) long dh);
struct Layer *pCreateUpfrontHookLayer (REG(a0) struct Layer_Info *li, REG(a1) struct BitMap *bm, REG(d0) long x0, REG(d1) long y0, REG(d2) long x1, REG(d3) long y1, REG(d4) long flags, REG(a3) struct Hook *hook, REG(a2) struct BitMap *bm2);
struct Layer *pCreateBehindHookLayer (REG(a0) struct Layer_Info *li, REG(a1) struct BitMap *bm, REG(d0) long x0, REG(d1) long y0, REG(d2) long x1, REG(d3) long y1, REG(d4) long flags, REG(a3) struct Hook *hook, REG(a2) struct BitMap *bm2);
struct Hook *pInstallLayerHook (REG(a0) struct Layer *layer, REG(a1) struct Hook *hook);
struct Hook *pInstallLayerInfoHook (REG(a0) struct Layer_Info *li, REG(a1) struct Hook *hook);
void pSortLayerCR (REG(a0) struct Layer *layer, REG(d0) long dx, REG(d1) long dy);
void pDoHookClipRects (REG(a0) struct Hook *hook, REG(a1) struct RastPort *rport, REG(a2) struct Rectangle *rect);

#endif
