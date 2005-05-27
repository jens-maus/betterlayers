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
#include <clib/alib_protos.h>
#include <datatypes/animationclass.h>
#include <datatypes/datatypes.h>
#include <datatypes/pictureclass.h>
#include <proto/datatypes.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

VOID main ()
{
	struct Screen *scr;
	if(scr = LockPubScreen(NULL))
	{
		Object *dt;
		if(dt = NewDTObject("Homer:XXX/Exhibitionist/exhib12.jpg",
			DTA_GroupID,				GID_PICTURE,
			PDTA_Remap,					TRUE,
			PDTA_FreeSourceBitMap,	TRUE,
			PDTA_Screen,				scr,
			TAG_DONE))
		{
			DoMethod(dt, DTM_PROCLAYOUT);

			struct BitMap *bmp;
			struct BitMapHeader *header;
			GetDTAttrs(dt, PDTA_BitMapHeader, &header, PDTA_DestBitMap, &bmp, TAG_DONE);

			ULONG width = header->bmh_Width, height = header->bmh_Height;

			struct Window *win;
			if(win = OpenWindowTags(NULL,
				WA_InnerWidth,			width,
				WA_InnerHeight,		height,
				WA_PubScreen,			scr,
				WA_IDCMP,				IDCMP_REFRESHWINDOW | IDCMP_CLOSEWINDOW | IDCMP_INTUITICKS,
				WA_DragBar,				TRUE,
				WA_DepthGadget,		TRUE,
				WA_CloseGadget,		TRUE,
				WA_SimpleRefresh,		TRUE,
				WA_BackFill,			LAYERS_NOBACKFILL,
				TAG_DONE))
			{
//				BltBitMapRastPort(bmp, 0, 0, win->RPort, win->BorderLeft, win->BorderTop, width, height, 0xc0);

				BOOL quit = FALSE, x = 0;
				do {

					WaitPort(win->UserPort);
					struct IntuiMessage *msg;
					while(msg = (struct IntuiMessage *)GetMsg(win->UserPort))
					{
						switch(msg->Class)
						{
							case IDCMP_INTUITICKS:
							{
								x = (x+10) % width;
								ScrollRaster(win->RPort, -5, 0, win->BorderLeft, win->BorderTop, win->BorderLeft+width-1, win->BorderTop+height-1);
								ScrollRaster(win->RPort, -5, 0, win->BorderLeft, win->BorderTop, win->BorderLeft+width-1, win->BorderTop+height-1);
								BltBitMapRastPort(bmp, (width-x) % width, 0, win->RPort, win->BorderLeft, win->BorderTop, 10, height, 0xc0);
								if(win->RPort->Layer->Flags & LAYERREFRESH)
								{
									BeginRefresh(win);
									BltBitMapRastPort(bmp, 0, 0, win->RPort, win->BorderLeft+x, win->BorderTop, width-x, height, 0xc0);
									BltBitMapRastPort(bmp, (width-x) % width, 0, win->RPort, win->BorderLeft, win->BorderTop, x, height, 0xc0);
									EndRefresh(win, TRUE);
								}
							}
							break;

							case IDCMP_CLOSEWINDOW:
								quit = TRUE;
							break;

							case IDCMP_REFRESHWINDOW:
							{
								BeginRefresh(win);
								BltBitMapRastPort(bmp, 0, 0, win->RPort, win->BorderLeft+x, win->BorderTop, width-x, height, 0xc0);
								BltBitMapRastPort(bmp, (width-x) % width, 0, win->RPort, win->BorderLeft, win->BorderTop, x, height, 0xc0);
								EndRefresh(win, TRUE);
							}
							break;
						}
						ReplyMsg(&msg->ExecMessage);
					}

				} while(!quit);

				CloseWindow(win);
			}
			DisposeDTObject(dt);
		}
		UnlockPubScreen(NULL, scr);
	}
}
