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

#include <clib/alib_protos.h>
#include <proto/graphics.h>
#include <graphics/layers.h>
#include <intuition/classusr.h>

#include "BackFill.h"
#include "BetterLayersNG.h"

VOID ClearRect (struct Rectangle *rect, APTR userdata)
{
	struct BitMap *bmp = (struct BitMap *)userdata;
	BltBitMap(bmp, rect->MinX, rect->MinY, bmp, rect->MinX, rect->MinY, rect->MaxX-rect->MinX+1, rect->MaxY-rect->MinY+1, 0x000, -1, NULL);
}

VOID BackFillLayer (struct Layer *l, struct Rectangle *rect)
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
