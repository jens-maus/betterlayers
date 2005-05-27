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

/*
		Create layer:
			OthersCR -= Bounds

		Dispose layer:
			OthersCR += Bounds - OthersBounds (also add as damage)

		Nove/size layer:
			OthersCR = ReCalc()
			OthersDamage += old - new
			OwnCR = ReCalc()
			OwnDamage += NewCr - OldCr

		Depth-arrange layer:
			Front
				OthersCR -= inverse(OwnCR)
				OwnDamage += inverse(OwnCR)
			Back
				OthersCR += OwnCR (also add as damage)
				OwnCR += OthersDamage

		BeginUpdate / Install Clip region
			OwnCR = union(OwnCR, OwnDamage)
*/

#include <string.h>
#include <clib/alib_protos.h>
#include <clib/macros.h>
#include <proto/graphics.h>
#include <graphics/clip.h>
#include <graphics/layers.h>
#include <graphics/rastport.h>
#include <intuition/classusr.h>

#include "BackFill.h"
#include "BetterLayersNG.h"
#include "ClipRects.h"
#include "Debug.h"

MyClipRect::MyClipRect ()
{
	FirstRow = LastRow = NULL;
}

MyClipRect::MyClipRect (struct ClipRect *cr)
{
	FirstRow = LastRow = new Row(cr);
}

MyClipRect::~MyClipRect ()
{
	struct Row *prev, *row = FirstRow;
	while(prev = row)
	{
		struct Cell *prev_cell, *cell = row->FirstCell;
		while(prev_cell = cell)
		{
			cell = cell->Next;
			delete prev_cell;
		}
		row = row->Next;
		delete prev;
	}
}

VOID MyClipRect::Link (struct Row *row, struct Row *prev)
{
	if(row->Prev = prev)
	{
		if(row->Next = prev->Next)
				row->Next->Prev = row;
		else	LastRow = row;
		prev->Next = row;
	}
	else
	{
		if(row->Next = FirstRow)
				row->Next->Prev = row;
		else	LastRow = row;
		FirstRow = row;
	}
}

VOID MyClipRect::Unlink (struct Row *row)
{
	if(row->Prev)
			row->Prev->Next = row->Next;
	else	FirstRow = row->Next;

	if(row->Next)
			row->Next->Prev = row->Prev;
	else	LastRow = row->Prev;
}

struct Cell *NewCell (LONG x0, LONG y0, LONG x1, LONG y1)
{
	D(DBF_MASKS, ("x0 %ld, y0 %ld, x1 %ld, y1 %ld", x0, y0, x1, y1))

	struct Cell *cell = new Cell;
	struct ClipRect *cr = new ClipRect;
#if 0
	memset(cr, 0, sizeof(ClipRect));
#else
	cr->Next = cr->prev = NULL;
	cr->lobs = NULL;
	cr->BitMap = NULL;
	cr->Flags = 0;
#endif
	cr->bounds.MinY = y0;
	cr->bounds.MaxY = y1;
	cr->bounds.MinX = x0;
	cr->bounds.MaxX = x1;
	cell->SetClipRect(cr);
	return cell;
}

struct ClipRect *NewClipRects (struct Layer *l)
{
	D(DBF_MASKS, ("layer 0x%lx", l))

	struct ClipRect *cr = new ClipRect;
	memset(cr, 0, sizeof(ClipRect));
	cr->bounds = l->bounds;
	SetExtraInfo(l, new MyClipRect(cr));

	return cr;
}

VOID DeleteClipRect (struct ClipRect *cr)
{
	D(DBF_MASKS, ("cr 0x%lx", cr))

	if(cr->BitMap)
	{
		WaitBlit();
		FreeBitMap(cr->BitMap);
	}
	delete cr;
}

VOID DeleteClipRects (struct ClipRect *cr)
{
	D(DBF_MASKS, ("cr 0x%lx", cr))

	struct ClipRect *prev;
	while(prev = cr)
	{
		cr = cr->Next;
		DeleteClipRect(prev);
	}
}

struct ClipRect *MyClipRect::OrderClipRects ()
{
	struct ClipRect *cr = NULL;
	struct Row *row = FirstRow;
	while(row)
	{
		struct Cell *cell = row->FirstCell;
		while(cell)
		{
			cell->CR->prev = cr;
			if(cr)
				cr->Next = cell->CR;

			cr = cell->CR;
			cr->bounds.MinY = row->MinY;
			cr->bounds.MaxY = row->MaxY;
			cr->bounds.MinX = cell->MinX;
			cr->bounds.MaxX = cell->MaxX;

			cell = cell->Next;
		}
		row = row->Next;
	}

	if(cr)
		cr->Next = NULL;

	return FirstRow ? FirstRow->FirstCell->CR : NULL;
}

/* subtract rectangle from layers ClipRect */
VOID MyClipRect::Sub (struct Layer *l, struct Rectangle *rect)
{
	LONG x0 = rect->MinX, y0 = rect->MinY, x1 = rect->MaxX, y1 = rect->MaxY;
	BOOL fix = FALSE;
	struct Row *row = FirstRow;
	while(row)
	{
		D(DBF_MASKS, ("%3ld-%3ld", row->MinY, row->MaxY))

		if(row->MinY > y1)
			break;

		if(row->MaxY < y0 || row->FirstCell->MinX > x1)
		{
			row = row->Next;
			continue;
		}

		struct Row *upper = NULL;
		if(row->MinY < y0)
		{
			upper = new Row;
			Link(upper, row->Prev);

			upper->MinY = row->MinY;
			upper->MaxY = y0 - 1;
			row->MinY = y0;

			fix = TRUE;
			D(DBF_MASKS, ("\tAdding upper row %3ld-%3ld", upper->MinY, upper->MaxY))
		}

		struct Row *lower = NULL, *next_row = row->Next;
		if(row->MaxY > y1)
		{
			lower = new Row;
			Link(lower, row);

			lower->MinY = y1 + 1;
			lower->MaxY = row->MaxY;
			row->MaxY = y1;

			fix = TRUE;
			D(DBF_MASKS, ("\tAdding lower row %3ld-%3ld", lower->MinY, lower->MaxY))
		}

		struct Cell *cell = row->FirstCell;
		while(cell)
		{
			if(upper)
				upper->AddAsTail(NewCell(cell->MinX, upper->MinY, cell->MaxX, upper->MaxY));

			if(lower)
				lower->AddAsTail(NewCell(cell->MinX, lower->MinY, cell->MaxX, lower->MaxY));

			if(cell->MinX > x1 || cell->MaxX < x0)
			{
				cell = cell->Next;
				continue;
			}

			BOOL needLeft  = cell->MinX < x0 ? 1 : 0;
			BOOL needRight = cell->MaxX > x1 ? 1 : 0;

			switch(needLeft + needRight)
			{
				case 0:
				{
					struct Cell *old = cell;
					cell = cell->Next;
					row->Unlink(old);
					DeleteClipRect(old->CR);
					delete old;

					if(!row->FirstCell)
					{
						Unlink(row);
						delete row;
					}

					fix = TRUE;
				}
				break;

				case 1:
				{
					if(needLeft)
							cell->MaxX = cell->CR->bounds.MaxX = x0-1;
					else	cell->MinX = cell->CR->bounds.MinX = x1+1;
					cell = cell->Next;
				}
				break;

				case 2:
				{
					struct Cell *right_cell = NewCell(x1+1, row->MinY, cell->MaxX, row->MaxY);
					right_cell->Prev = cell;
					if(right_cell->Next = cell->Next)
						cell->Next->Prev = right_cell;
					cell->Next = right_cell;

					cell->MaxX = cell->CR->bounds.MaxX = x0-1;

					cell = right_cell->Next;

					fix = TRUE;
				}
				break;
			}
		}
		row = next_row;
	}

	if(fix)
		l->ClipRect = OrderClipRects();
}

BOOL RectOverlap (struct Rectangle &r1, struct Rectangle &r2)
{
	return !((r1.MinX > r2.MaxX || r1.MaxX < r2.MinX) || (r1.MinY > r2.MaxY || r1.MaxY < r2.MinY));
}

VOID SubFromClipRects (struct Layer *l, struct Rectangle *rect)
{
	while(l)
	{
		D(DBF_MASKS, ("layer 0x%lx (x0 %ld, y0 %ld, x1 %ld, y1 %ld)", l, rect->MinX, rect->MinY, rect->MaxX, rect->MaxY))
		if(RectOverlap(l->bounds, *rect))
			GetExtraInfo(l)->Sub(l, rect);
		l = l->back;
	}
}

/* add rectangle to row */
VOID MyClipRect::AddRectangle (struct Rectangle *rect)
{
	LONG x0 = rect->MinX, y0 = rect->MinY, x1 = rect->MaxX, y1 = rect->MaxY;
	D(DBF_MASKS, ("x0 %3ld, y0 %3ld, x1 %3ld, y1 %3ld", x0, y0, x1, y1))

	struct Row *row = FirstRow;
	while(row)
	{
		if(y0 > row->MaxY)
		{
			row = row->Next;
			continue;
		}

		/* add new upper row */
		if(y0 < row->MinY)
		{
			if(y1+1 == row->MinY && !row->FirstCell->Next && row->FirstCell->MinX == x0 && row->FirstCell->MaxX == x1)
			{
				row->MinY = y0;
				break;
			}
			else
			{
				struct Row *upper = new Row;
				Link(upper, row->Prev);

				upper->MinY = y0;
				upper->MaxY = MIN(y1, row->MinY-1);
				upper->AddAsTail(NewCell(x0, upper->MinY, x1, upper->MaxY));

				if(upper->MaxY == y1)
					break;

				y0 = row->MinY;
			}
		}

		struct Row *upper = NULL;
		if(y0 > row->MinY)
		{
			Link(upper = new Row, row->Prev);

			upper->MinY = row->MinY;
			upper->MaxY = y0 - 1;
			row->MinY = y0;
		}

		struct Row *lower = NULL, *next_row = row->Next;
		if(y1 < row->MaxY)
		{
			Link(lower = new Row, row);

			lower->MinY = y1 + 1;
			lower->MaxY = row->MaxY;
			row->MaxY = y1;
		}

		struct Cell *cell = row->FirstCell;
		while(cell)
		{
			if(upper) upper->AddAsTail(NewCell(cell->MinX, upper->MinY, cell->MaxX, upper->MaxY));
			if(lower) lower->AddAsTail(NewCell(cell->MinX, lower->MinY, cell->MaxX, lower->MaxY));

			D(DBF_MASKS, ("Cell: %ld-%ld, Rect: %ld-%ld", cell->MinX, cell->MaxX, x0, x1))

			if(cell->MaxX == x0-1 && cell->Next && cell->Next->MinX == x1+1)
			{
				if(upper) upper->AddAsTail(NewCell(cell->Next->MinX, upper->MinY, cell->Next->MaxX, upper->MaxY));
				if(lower) lower->AddAsTail(NewCell(cell->Next->MinX, lower->MinY, cell->Next->MaxX, lower->MaxY));

				cell->MaxX = cell->Next->MaxX;

				struct Cell *rem = cell->Next;
				row->Unlink(rem);
				DeleteClipRect(rem->CR);
				delete rem;
			}
			else if(cell->MaxX == x0-1)
				cell->MaxX = x1;
			else if(cell->MinX == x1+1)
				cell->MinX = x0;
			else if((!cell->Next && x0 > cell->MaxX) || (cell->Next && x1+1 < cell->Next->MinX))
			{
				struct Cell *n_cell = NewCell(x0, row->MinY, x1, row->MaxY);
				n_cell->Prev = cell;
				if(n_cell->Next = cell->Next)
					cell->Next->Prev = n_cell;
				cell->Next = n_cell;

				cell = n_cell;
			}
			else if(!cell->Prev && x1+1 < cell->MinX)
			{
				struct Cell *n_cell = NewCell(x0, row->MinY, x1, row->MaxY);
				n_cell->Prev = NULL;
				n_cell->Next = cell;
				cell->Prev = n_cell;
				row->FirstCell = n_cell;
			}
			else
			{
				cell = cell->Next;
				continue;
			}

			D(DBF_MASKS, ("\tCell: %ld-%ld, Rect: %ld-%ld", cell->MinX, cell->MaxX, x0, x1))

			if(!row->FirstCell->Next)
			{
				struct Cell *prev = row->Prev ? row->Prev->FirstCell : NULL, *current = row->FirstCell;
				if(!upper && prev && !prev->Next && row->MinY-1 == row->Prev->MaxY && prev->MinX == current->MinX && prev->MaxX == current->MaxX)
				{
					struct Row *p = row->Prev;
					row->MinY = p->MinY;
					Unlink(p);

					struct Cell *prev_cell, *cur_cell = p->FirstCell;
					while(prev_cell = cur_cell)
					{
						cur_cell = cur_cell->Next;
						DeleteClipRect(prev_cell->CR);
						delete prev_cell;
					}

					delete p;
//					D(DBF_ALWAYS, ("Upper collapse"))
				}

				struct Cell *next = row->Next ? row->Next->FirstCell : NULL;
				if(!lower && next && !next->Next && row->MaxY+1 == row->Next->MinY && next->MinX == current->MinX && next->MaxX == current->MaxX)
				{
					struct Row *n = row->Next;
//					D(DBF_ALWAYS, ("Unlinking %ld-%ld (Lower)", n->MinY, n->MaxY))
					next_row = n->Next;
					row->MaxY = n->MaxY;
					Unlink(n);

					struct Cell *prev_cell, *cur_cell = n->FirstCell;
					while(prev_cell = cur_cell)
					{
						cur_cell = cur_cell->Next;
						DeleteClipRect(prev_cell->CR);
						delete prev_cell;
					}

					delete n;
				}
			}

			break;
		}

		if((upper || lower) && cell)
		{
			while(cell = cell->Next)
			{
				if(upper) upper->AddAsTail(NewCell(cell->MinX, upper->MinY, cell->MaxX, upper->MaxY));
				if(lower) lower->AddAsTail(NewCell(cell->MinX, lower->MinY, cell->MaxX, lower->MaxY));
			}
		}

		if(row->MaxY >= y1)
			break;

#ifdef DEBUG
		if(y1 < row->MaxY)
			D(DBF_ALWAYS, ("Assumption failed (%ld < %ld)", y1, row->MaxY))
#endif

		y0 = row->MaxY+1;
		row = next_row;
	}

	if(!LastRow || LastRow->MaxY < y1)
	{
		if(LastRow && !LastRow->FirstCell->Next && LastRow->MaxY+1 == y0 && LastRow->FirstCell->MinX == x0 && LastRow->FirstCell->MaxX == x1)
		{
			LastRow->MaxY = y1;
		}
		else
		{
			struct Row *bottom = new Row;

			bottom->MinY = LastRow ? MAX(y0, LastRow->MaxY+1) : y0;
			bottom->MaxY = y1;
			bottom->AddAsTail(NewCell(x0, bottom->MinY, x1, y1));

			Link(bottom, LastRow);
		}
	}
}

/* Move l->bounds from this layer to l */
BOOL MyClipRect::MoveToLayer (struct Layer *l)
{
	BOOL changed = FALSE;
	LONG x0 = l->bounds.MinX, y0 = l->bounds.MinY, x1 = l->bounds.MaxX, y1 = l->bounds.MaxY;
	struct Row *row = FirstRow;
	while(row)
	{
		D(DBF_MASKS, ("%3ld-%3ld", row->MinY, row->MaxY))

		if(row->MinY > y1)
			break;

		if(row->MaxY < y0 || row->FirstCell->MinX > x1)
		{
			row = row->Next;
			continue;
		}

		struct Row *upper = NULL;
		if(row->MinY < y0)
		{
			upper = new Row;
			upper->MinY = row->MinY;
			upper->MaxY = y0 - 1;
			Link(upper, row->Prev);
			row->MinY = y0;

			D(DBF_MASKS, ("\tAdding upper row %3ld-%3ld", upper->MinY, upper->MaxY))
		}

		struct Row *lower = NULL, *next_row = row->Next;
		if(row->MaxY > y1)
		{
			lower = new Row;
			lower->MinY = y1 + 1;
			lower->MaxY = row->MaxY;
			Link(lower, row);
			row->MaxY = y1;

			D(DBF_MASKS, ("\tAdding lower row %3ld-%3ld", lower->MinY, lower->MaxY))
		}

		struct Cell *cell = row->FirstCell;
		while(cell)
		{
			if(upper) upper->AddAsTail(NewCell(cell->MinX, upper->MinY, cell->MaxX, upper->MaxY));
			if(lower) lower->AddAsTail(NewCell(cell->MinX, lower->MinY, cell->MaxX, lower->MaxY));

			if(cell->MinX > x1 || cell->MaxX < x0)
			{
				cell = cell->Next;
				continue;
			}

			struct Rectangle r = { MAX(x0, cell->MinX), row->MinY, MIN(x1, cell->MaxX), row->MaxY };
#ifdef DEBUG_CRAP
			l->rp->Layer = NULL;
			SetAPen(l->rp, 1);
			RectFill(l->rp, r.MinX, r.MinY, r.MaxX, r.MaxY);
			SetAPen(l->rp, 2);
			if(r.MinX+1 < r.MaxX && r.MinY+1 < r.MaxY)
				RectFill(l->rp, r.MinX+1, r.MinY+1, r.MaxX-1, r.MaxY-1);
			l->rp->Layer = l;
#endif
			GetExtraInfo(l)->AddRectangle(&r);
			BackFillLayer(l, &r);
			OrRectRegion(l->DamageList, &r);
			changed = TRUE;

			BOOL needLeft  = cell->MinX < x0 ? 1 : 0;
			BOOL needRight = cell->MaxX > x1 ? 1 : 0;

			switch(needLeft + needRight)
			{
				case 0:
				{
					struct Cell *old = cell;
					cell = cell->Next;
					row->Unlink(old);
					DeleteClipRect(old->CR);
					delete old;

					if(!row->FirstCell)
					{
						Unlink(row);
						delete row;
					}
				}
				break;

				case 1:
				{
					if(needLeft)
							cell->MaxX = cell->CR->bounds.MaxX = x0-1;
					else	cell->MinX = cell->CR->bounds.MinX = x1+1;
					cell = cell->Next;
				}
				break;

				case 2:
				{
					struct Cell *right_cell = NewCell(x1+1, row->MinY, cell->MaxX, row->MaxY);
					right_cell->Prev = cell;
					if(right_cell->Next = cell->Next)
						cell->Next->Prev = right_cell;
					cell->Next = right_cell;

					cell->MaxX = cell->CR->bounds.MaxX = x0-1;

					cell = right_cell->Next;
				}
				break;
			}
		}
		row = next_row;
	}
	return changed;
}

/* l: first layer to get new cliprect, src: layer being deleted, rect: src->bounds */
VOID AddToClipRects (struct Layer *src, struct Layer *l)
{
	struct MyClipRect *cr = GetExtraInfo(src);
	struct Row *row;
	if(row = cr->FirstRow)
	{
#ifdef DEBUG_CRAP
		while(row)
		{
			LONG y0 = row->MinY, y1 = row->MaxY;
			struct Cell *cell = row->FirstCell;
			while(cell)
			{
				LONG x0 = cell->MinX, x1 = cell->MaxX;
				BltBitMap(src->rp->BitMap, x0, y0, src->rp->BitMap, x0, y0, x1-x0+1, y1-y0+1, 0x000, -1, NULL);
				cell = cell->Next;
			}
			row = row->Next;
		}
		row = cr->FirstRow;
#endif

		while(l)// && cr->FirstRow) /* 500 windows: 11.60 vs 11.84 seconds */
		{
			D(DBF_MASKS, ("layer 0x%lx (x0 %ld, y0 %ld, x1 %ld, y1 %ld)", l, l->bounds.MinX, l->bounds.MinY, l->bounds.MaxX, l->bounds.MaxY))
			if(RectOverlap(src->bounds, l->bounds))
			{
				if(cr->MoveToLayer(l))
				{
					l->ClipRect = GetExtraInfo(l)->OrderClipRects();
					l->Flags |= LAYERREFRESH | LAYERIREFRESH;
				}
			}
			l = l->back;
		}

		src->ClipRect = cr->OrderClipRects();

		struct Hook *bfill = (Hook *)src->LayerInfo->BlankHook;
		if(bfill != LAYERS_NOBACKFILL)
		{
//			pDoHookClipRects(bfill, (Object *)src->rp, NULL);

			row = cr->FirstRow;
			while(row)
			{
				LONG y0 = row->MinY, y1 = row->MaxY;
				struct Cell *cell = row->FirstCell;
				while(cell)
				{
					LONG x0 = cell->MinX, x1 = cell->MaxX;
					if(bfill == LAYERS_BACKFILL)
					{
						BltBitMap(src->rp->BitMap, x0, y0, src->rp->BitMap, x0, y0, x1-x0+1, y1-y0+1, 0x000, -1, NULL);
					}
					else
					{
						struct HookMsg hmsg = { NULL, x0, y0, x1, y1, x0, y0 };
						CallHookA((Hook *)src->LayerInfo->BlankHook, (Object *)src->rp, &hmsg);
					}
					cell = cell->Next;
				}
				row = row->Next;
			}
		}
	}
}
