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

#ifndef SLICEREGION_H
#define SLICEREGION_H

#include <graphics/gfx.h>

#define REG(x) register __ ## x
#define BETWEEN(a, min, max) (a >= min && a <= max)

#define NextRow(cr)   ((struct ClipRect *)cr->_p1)
#define PrevRow(cr)   ((struct ClipRect *)cr->_p2)
#define LastInRow(cr) ((struct ClipRect *)cr->reserved)

#define SetNextRow(cr1,cr2)   (cr1->_p1 = cr2)
#define SetPrevRow(cr1,cr2)   (cr1->_p2 = cr2)
#define SetLastInRow(cr1,cr2) (cr1->reserved = (LONG)cr2)

struct HookMsg
{
	struct Layer *l;
	struct Rectangle bounds;
	LONG offsetx, offsety;
};

struct ClipRect *NewClipRect (struct Layer_Info *li);
VOID DelClipRects (struct Layer *l);

VOID bScrollRaster (struct RastPort *rp, WORD dx, WORD dy, WORD xmin, WORD ymin, WORD xmax, WORD ymax, VOID(*filler)(struct RastPort *rp, struct Rectangle &bound));
VOID bClipBlit (struct RastPort *rp, WORD srcx, WORD srcy, struct RastPort *dstrp, WORD dstx, WORD dsty, WORD width, WORD height, UBYTE minterm);

BOOL bBeginUpdate (struct Layer *l);
struct Layer *bCreateUpfrontHookLayer (struct Layer_Info *li, struct BitMap *bmp, LONG x0, LONG y0, LONG x1, LONG y1, LONG flags, struct Hook *hook, struct BitMap *super_bmp);
struct Layer *bCreateBehindHookLayer (struct Layer_Info *li, struct BitMap *bmp, LONG x0, LONG y0, LONG x1, LONG y1, LONG flags, struct Hook *hook, struct BitMap *super_bmp);
BOOL bDeleteLayer (LONG dummy, struct Layer *l);
VOID bDisposeLayerInfo (struct Layer_Info *li);
VOID bDoHookClipRects (struct Hook *hook, struct RastPort *rp, struct Rectangle *rect);
VOID bEndUpdate (struct Layer *l);
struct Region *bInstallClipRegion (struct Layer *l, struct Region *region);
BOOL bMoveLayerInFrontOf (struct Layer *layertomove, struct Layer *targetlayer);
BOOL bMoveSizeLayer (struct Layer *layer, LONG dx, LONG dy, LONG dw, LONG dh);
struct Layer_Info *bNewLayerInfo ();
struct Layer *bWhichLayer (struct Layer_Info *li, WORD x, WORD y);

#endif
