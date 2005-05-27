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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <exec/memory.h>

struct TimeIt
{
	struct DateStamp t1, t2;
	STRPTR Name;

	TimeIt (STRPTR name) : Name(name)
	{
		fprintf(stdout, "Testing: %s...", name);
		fflush(stdout);

		DateStamp(&t1);
	}

	~TimeIt ()
	{
		DateStamp(&t2);

		LONG ticks = t2.ds_Tick - t1.ds_Tick;
		LONG seconds = ticks / 50 + 60 * (t2.ds_Minute - t1.ds_Minute);
		if(ticks < 0)
			ticks += TICKS_PER_SECOND * 60;

		LONG spaces = 30-(12+strlen(Name));
		while(spaces-- > 0)
			fprintf(stdout, " ");

		fprintf(stdout, "%3d.%02d seconds\n", seconds, 2 * (ticks%50));
	}
};

struct BitMap *MyAllocBitMap (ULONG w, ULONG h, ULONG d, ULONG flags, struct BitMap *friend_bitmap)
{
//	struct BitMap *res = (BitMap *)AllocVec(sizeof(BitMap), MEMF_ANY);
	struct BitMap *res = new BitMap;
	res->Planes[0] = (PLANEPTR)AllocVec(RASSIZE(w, h) * d, MEMF_ANY);
	return res;
}

VOID MyFreeBitMap (struct BitMap *bmp)
{
	FreeVec(bmp->Planes[0]);
	delete bmp;
//	FreeVec(bmp);
}

VOID OpenWindows (STRPTR name, ULONG count, ULONG repeat, struct Screen *scr)
{
//	struct BitMap **windows = new BitMap *[count];
	struct Window **windows = new Window *[count];
	struct TimeIt timer(name);
	for(ULONG i = 0; i < repeat; i++)
	{
		for(ULONG j = 0; j < count; j++)
//			windows[j] = MyAllocBitMap(rand() % scr->Width/2, rand() % scr->Height/2, GetBitMapAttr(scr->RastPort.BitMap, BMA_DEPTH), 0L, scr->RastPort.BitMap);
//			windows[j] = AllocBitMap(rand() % scr->Width/2, rand() % scr->Height/2, GetBitMapAttr(scr->RastPort.BitMap, BMA_DEPTH), 0L, scr->RastPort.BitMap);
//			windows[j] = (BitMap *)new UBYTE [RASSIZE(rand() % scr->Width/2, rand() % scr->Height/2) * GetBitMapAttr(scr->RastPort.BitMap, BMA_DEPTH)];
//			windows[j] = (BitMap *)AllocVec(RASSIZE(rand() % scr->Width/2, rand() % scr->Height/2) * GetBitMapAttr(scr->RastPort.BitMap, BMA_DEPTH), MEMF_ANY);
			windows[j] = OpenWindowTags(NULL, WA_PubScreen, scr, WA_SimpleRefresh, TRUE, WA_Left, rand() % scr->Width/2, WA_Top, rand() % scr->Height/2, WA_InnerWidth, rand() % scr->Width/2, WA_InnerHeight, rand() % scr->Height/2, TAG_DONE);

//		for(j = 0; j < count; j++)
		while(j--)
//			MyFreeBitMap(windows[j]);
//			FreeBitMap(windows[j]);
//			delete windows[j];
//			FreeVec(windows[j]);
			CloseWindow(windows[j]);
	}
	delete windows;
}

VOID main ()
{
	struct Screen *scr;
	if(scr = OpenScreenTags(NULL, SA_LikeWorkbench, TRUE, TAG_DONE))// LockPubScreen(NULL))
	{
		UBYTE name[64], repeats = 3;

		for(ULONG i = 10; i < 60; i += 10)
		{
			sprintf(name, "%3d windows", i);
			OpenWindows(name, i, repeats, scr);
		}

		CloseScreen(scr);
//		UnlockPubScreen(NULL, scr);
	}
}
