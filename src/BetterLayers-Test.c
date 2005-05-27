#include <stdio.h>
#include <stdlib.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/layers.h>

#include "BetterLayers.h"
#include "Debug.h"
#include "TimeIt.h"

#define USE_NATIVE_LAYERS

#define BENCHMARK
	#define WINDOWS 50
	#define REPEATS 1
//	#define BACKWARDS_CLOSE

#define QUIT TRUE
//#define QUIT FALSE

#ifdef USE_NATIVE_LAYERS
#define pBeginUpdate					BeginUpdate
#define pBehindLayer					BehindLayer
#define pCreateUpfrontHookLayer	CreateUpfrontHookLayer
#define pDeleteLayer					DeleteLayer
#define pDisposeLayerInfo			DisposeLayerInfo
#define pDoHookClipRects			DoHookClipRects
#define pEndUpdate					EndUpdate
#define pInstallClipRegion			InstallClipRegion
#define pMoveLayerInFrontOf		MoveLayerInFrontOf
#define pMoveSizeLayer				MoveSizeLayer
#define pNewLayerInfo				NewLayerInfo
#define pUpfrontLayer				UpfrontLayer
#define pWhichLayer					WhichLayer
#define pLockLayer					LockLayer
#define pUnlockLayer					UnlockLayer
#else
#include "PatchProtos.h"
#endif

VOID BackFillCode (REG(a0) struct Hook *hook, REG(a1) struct HookMsg *msg, REG(a2) struct RastPort *rp)
{
	struct Layer *l = rp->Layer;
	rp->Layer = NULL;
	RectFill(rp, msg->bounds.MinX, msg->bounds.MinY, msg->bounds.MaxX, msg->bounds.MaxY);
	rp->Layer = l;
}

struct Hook BackFillHook = { { NULL, NULL }, (ULONG(*)())BackFillCode };

VOID DrawWindow (struct Layer *l)
{
	pLockLayer(0, l);

	ULONG x1 = 0, x2 = l->Width-1;
	ULONG y1 = 0, y2 = l->Height-1;

	if(l->BackFill == LAYERS_BACKFILL || l->BackFill == LAYERS_NOBACKFILL)
	{
		struct Rectangle r2 = { x1+1, y1+1, x2-1, y2-1 };
		SetAPen(l->rp, 3);
		pDoHookClipRects(&BackFillHook, l->rp, &r2);
	}
//	else pDoHookClipRects(l->BackFill, l->rp, NULL);

	SetAPen(l->rp, 2);
/*	for(UWORD i = 0; i < 10; i++)
	{
		l->rp->cp_x = 10;
		l->rp->cp_y = 10+(i*l->rp->TxHeight);
		Text(l->rp, "Hej med dig", 11);
	}
*/
	struct Rectangle r = { x1, y1, x2, y2 };
	r.MaxY = y1;
	pDoHookClipRects(&BackFillHook, l->rp, &r);
	r.MinY = y2;
	r.MaxY = y2;
	pDoHookClipRects(&BackFillHook, l->rp, &r);
	r.MinY = y1;
	r.MaxX = x1;
	pDoHookClipRects(&BackFillHook, l->rp, &r);
	r.MinX = x2;
	r.MaxX = x2;
	pDoHookClipRects(&BackFillHook, l->rp, &r);

	r.MinX = x2-10;
	r.MinY = y2-10;
	r.MaxX = x2-9;
	pDoHookClipRects(&BackFillHook, l->rp, &r);

	r.MaxX = x2;
	r.MaxY = y2-9;
	pDoHookClipRects(&BackFillHook, l->rp, &r);

	pUnlockLayer(l);
}

VOID Refresh (struct Layer_Info *li)
{
	struct Layer *l = li->top_layer;
	while(l)
	{
		if((l->Flags & LAYERREFRESH) && pBeginUpdate(l))
		{
			DrawWindow(l);
			pEndUpdate(l, TRUE);
			l->Flags &= ~(LAYERREFRESH | LAYERIREFRESH | LAYERIREFRESH2);
		}
		l = l->back;
	}
}

VOID DumpMemoryInfo ();

VOID main ()
{
	struct Window *win;
	if(win = OpenWindowTags(NULL,
		WA_IDCMP,			IDCMP_CLOSEWINDOW | IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE,
//		WA_DragBar,			TRUE,
		WA_PubScreen,		NULL,
		WA_DepthGadget,	TRUE,
		WA_CloseGadget,	TRUE,
		WA_NoCareRefresh,	TRUE,
		WA_Activate,		TRUE,
		WA_RMBTrap,			TRUE,
		WA_ReportMouse,	TRUE,
		TAG_DONE))
	{
		struct BitMap *bmp = win->RPort->BitMap;

		ULONG width = win->Width - (win->BorderLeft+win->BorderRight);
		ULONG height = win->Height - (win->BorderTop+win->BorderBottom);

		struct RastPort rp = *win->RPort;
		struct Layer_Info *li = pNewLayerInfo();

#ifdef BENCHMARK
		UBYTE name[64];
		sprintf(name, "%dx%d windows", REPEATS, WINDOWS);
		struct TimeIt timer(name);
//		struct TimeIt timer(STR(REPEATS) "x" STR(WINDOWS) " windows");
		for(ULONG j = 0; j < REPEATS; j++)
		{
			for(ULONG i = 0; i < WINDOWS; i++)
			{
				ULONG w = rand() % 300 + 75;
				ULONG h = rand() % 200 + 75;
				ULONG x = rand() % (width-w);
				ULONG y = rand() % (height-h);

				x += win->BorderLeft;
				y += win->BorderTop;
				ULONG x2 = x+w-1, y2 = y+h-1;
//				D(DBF_ALWAYS, ("x0 %3ld, y0 %3ld, x1 %3ld, y1 %3ld", x, y, x2, y2))
				struct Layer *l = pCreateUpfrontHookLayer(li, bmp, x, y, x2, y2, LAYERSIMPLE, LAYERS_NOBACKFILL, NULL);
				SetFont(l->rp, win->IFont);
				SetDrMd(l->rp, JAM1);
				DrawWindow(l);
			}

#ifdef BACKWARDS_CLOSE
			struct Layer *prev, *l = li->top_layer;
			while(l->back)
				l = l->back;

			while(prev = l)
			{
				l = l->front;
				pDeleteLayer(0, prev);
				Refresh(li);
			}
#else

/*			pDisposeLayerInfo(li);
			li = pNewLayerInfo();
*/
			while(li->top_layer)
			{
				pDeleteLayer(0, li->top_layer);
				Refresh(li);
//				Delay(25);
			}
#endif
		}
#else
		li->BlankHook = win->RPort->Layer->LayerInfo->BlankHook;

		struct Layer *top = win->RPort->Layer->LayerInfo->top_layer;
		while(top->back)
			top = top->back;

		while(top->front)
		{
			ULONG x = top->bounds.MinX, y = top->bounds.MinY;
			ULONG x2 = top->bounds.MaxX, y2 = top->bounds.MaxY;
			struct Layer *l = pCreateUpfrontHookLayer(li, bmp, x, y, x2, y2, top->Flags, top->BackFill, NULL);
			SetFont(l->rp, win->IFont);
			SetDrMd(l->rp, JAM1);
			DrawWindow(l);
			Refresh(li);
			top = top->front;
		}
#endif

		struct IntuiMessage *msg;
		struct Layer *active = NULL;
		LONG offsetx, offsety, w, h;
		LONG secs = 0, micros = 0;

		BOOL quit = QUIT, size;
		while(!quit)
		{
			ULONG sigs = Wait((1<<win->UserPort->mp_SigBit) | SIGBREAKF_CTRL_D);
#ifdef DEBUG
			if(sigs & SIGBREAKF_CTRL_D)
				ReadDebugFlags();
#endif
			while(msg = (struct IntuiMessage *)GetMsg(win->UserPort))
			{
				LONG x = msg->MouseX, y = msg->MouseY;
				switch(msg->Class)
				{
					case IDCMP_CLOSEWINDOW:
						quit = TRUE;
					break;

					case IDCMP_MOUSEMOVE:
					{
						if(active)
						{
							LONG dx = 0, dy = 0, dw = 0, dh = 0;
							if(size)
							{
								dw = ((x-active->bounds.MinX) + (w-offsetx)) - active->Width;
								dh = ((y-active->bounds.MinY) + (h-offsety)) - active->Height;

								if(active->Width + dw < 10)
									dw = 10 - active->Width;

								if(active->bounds.MaxX + dw > 799)
									dw = 799 - active->bounds.MaxX;

								if(active->Height + dh < 10)
									dh = 10 - active->Height;

								if(active->bounds.MaxY + dh > 599)
									dh = 599 - active->bounds.MaxY;

								if(active->BackFill != LAYERS_NOBACKFILL)
								{
									if(dw > 0)
									{
										struct Rectangle r = { active->Width-1, 0, active->Width-1, active->Height-1 };
										pDoHookClipRects(active->BackFill, active->rp, &r);
									}

									if(dh > 0)
									{
										struct Rectangle r = { 0, active->Height-1, active->Width-1, active->Height-1 };
										pDoHookClipRects(active->BackFill, active->rp, &r);
									}

									if(dw || dh)
									{
										struct Rectangle r = { active->Width-11, active->Height-11, active->Width-2, active->Height-2 };
										pDoHookClipRects(active->BackFill, active->rp, &r);
									}
								}
							}
							else
							{
								LONG new_x = x - offsetx;
								LONG new_y = y - offsety;

								if(new_x < 0)
									new_x = 0;

								if(new_x + active->Width > 799)
									new_x = 800-active->Width;

								if(new_y < 0)
									new_y = 0;

								if(new_y + active->Height > 599)
									new_y = 600-active->Height;

								dx = new_x - active->bounds.MinX;
								dy = new_y - active->bounds.MinY;
							}
							pMoveSizeLayer(active, dx, dy, dw, dh);
							if(size)
								DrawWindow(active);
							Refresh(li);
						}
					}
					break;

					case IDCMP_MOUSEBUTTONS:
					{
						struct Layer *l;
						if(msg->Code == IECODE_UP_PREFIX + IECODE_LBUTTON)
						{
							active = NULL;
						}
						else if(l = pWhichLayer(li, x, y))
						{
							if(msg->Code == IECODE_LBUTTON)
							{
								if(DoubleClick(secs, micros, msg->Seconds, msg->Micros))
								{
									if(msg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
											pBehindLayer(0, l);
									else	pUpfrontLayer(0, l);

									Refresh(li);
									secs = 0;
									micros = 0;
								}
								else
								{
									secs = msg->Seconds;
									micros = msg->Micros;

									SetDrMd(l->rp, COMPLEMENT);
									l->rp->Layer = NULL;
									SetAPen(l->rp, 1);
									struct ClipRect *cr = l->ClipRect;
//									D(DBF_ALWAYS, ("ClipRect 0x%lx", cr))
									while(cr)
									{
										LONG x0 = cr->bounds.MinX, y0 = cr->bounds.MinY, x1 = cr->bounds.MaxX, y1 = cr->bounds.MaxY;
//										D(DBF_ALWAYS, ("\t%3ld, %3ld, %3ld, %3ld", x0, y0, x1, y1))

										RectFill(l->rp, x0, y0, x1, y0);
										RectFill(l->rp, x0, y1, x1, y1);
										RectFill(l->rp, x0, y0, x0, y1);
										RectFill(l->rp, x1, y0, x1, y1);
										cr = cr->Next;
									}
									l->rp->Layer = l;
									SetDrMd(l->rp, JAM1);
								}

								active = l;
								offsetx = x - l->bounds.MinX;
								offsety = y - l->bounds.MinY;
								w = l->Width;
								h = l->Height;
								size = x > l->bounds.MaxX-10 && y > l->bounds.MaxY-10;
							}
							else if(msg->Code == IECODE_RBUTTON && !(msg->Qualifier & IEQUALIFIER_LEFTBUTTON))
							{
								pDeleteLayer(0, l);
								Refresh(li);
							}
						}
						else
						{
							if(msg->Code == IECODE_LBUTTON)
							{
								struct Layer *l = li->top_layer;
								while(l)
								{
									l->Flags |= LAYERREFRESH | LAYERIREFRESH;
									l = l->back;
								}
								Refresh(li);
							}
						}
					}
					break;
				}
				ReplyMsg(&msg->ExecMessage);
			}
		}

		while(li->top_layer)
		{
			pDeleteLayer(0, li->top_layer);
			Refresh(li);
		}
		pDisposeLayerInfo(li);
		CloseWindow(win);
	}
}
