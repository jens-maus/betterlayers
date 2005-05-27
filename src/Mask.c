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
#include <clib/macros.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#pragma header

#include "BetterLayers.h"
#include "Debug.h"
#include "Mask.h"

VOID MyInsert (struct MyList &list, struct MyNode *n, struct MyNode *o)
{
	if(o)
	{
		if(n->Next = o->Next)
				o->Next->Prev = n;
		else	list.Last = n;
		o->Next = n;
		n->Prev = o;
	}
	else
	{
		if(n->Next = list.First)
				list.First->Prev = n;
		else	list.Last = n;
		list.First = n;
	}
}

VOID MyRemove (struct MyList &list, struct MyNode *n)
{
	if(list.Last == n)
	{
		if(!(list.Last = n->Prev))
			list.Last = (struct MyNode *)&list.First;
	}

	if(list.First == n)
	{
		if(!(list.First = n->Next))
			list.Last = (struct MyNode *)&list.First;
	}

	if(n->Prev)
		n->Prev->Next = n->Next;
	if(n->Next)
		n->Next->Prev = n->Prev;

	delete n;
}

MaskColumn::MaskColumn (LONG x1, LONG x2)
{
	XStart = x1;
	XStop = x2;
}

MaskColumn::MaskColumn (struct MaskColumn *src)
{
	XStart = src->XStart;
	XStop = src->XStop;
}

MaskLine::MaskLine (LONG x1, LONG y1, LONG x2, LONG y2) : Columns()
{
	YStart = y1;
	YStop = y2;
	Columns.MyAddTail(new MaskColumn(x1, x2));
	XMin = x1;
	XMax = x2;
}

MaskLine::MaskLine (struct MaskLine *src) : Columns()
{
	struct MaskColumn *c_src = src->First();
	while(c_src)
	{
		Columns.MyAddTail(new struct MaskColumn(c_src));
		c_src = (struct MaskColumn *)c_src->Next;
	}

	XMin = src->XMin;
	XMax = src->XMax;
}

MaskLine::~MaskLine ()
{
	struct MaskColumn *prev, *current = First();
	while(prev = current)
	{
		current = (MaskColumn *)current->Next;
		delete prev;
	}
}

struct MaskLine *MaskLine::MakeCopy ()
{
	return new MaskLine(this);
}

VOID ClipMask::ClearMask ()
{
	struct MaskLine *first = First(), *next;
	while(first)
	{
		next = (struct MaskLine *)first->Next;
		delete first;
		first = next;
	}
	Lines.Clear();
}

VOID ClipMask::SetMask (struct BitMap *bmp)
{
	SetMask(0, 0, GetBitMapAttr(bmp, BMA_WIDTH), GetBitMapAttr(bmp, BMA_HEIGHT));
}

VOID ClipMask::SetMask (LONG x0, LONG y0, LONG x1, LONG y1)
{
	ClearMask();
	Lines.MyAddTail(new MaskLine(x0, y0, x1, y1));
}

BOOL MaskLine::AddMask (LONG x1, LONG x2)
{
/*	printf("BEFORE: %d, %d\n", x1, x2);
	struct MaskColumn *first = First();
	while(first)
	{
		printf("\t%d, %d (%p)\n", first->XStart, first->XStop, first);
		first = (struct MaskColumn *)first->Next;
	}
	printf("--------------\n");
*/
	BOOL find = TRUE;
	struct MaskColumn *column = First();
	while(column)
	{
		LONG start = column->XStart, stop = column->XStop;
		if(find)
		{
//			printf("%d <= %d && %d >= %d = %d\n", x1, stop+1, x2, start-1, x1 <= stop+1 && x2 >= start-1);
			if(x1 <= stop+1 && x2 >= start-1)
			{
				column->XStart = MIN(x1, start);
				column->XStop = MAX(x2, stop);
				find = FALSE;
			}
			else
			{
//				printf("%d < %d = %d\n", x2, start, x2 < start);
				if(x2 < start)
				{
					struct MaskColumn *prev = (struct MaskColumn *)column->Prev;

//					printf("%d == %d = %d\n", column->XStart, x2+1, column->XStart == x2+1);
					if(column->XStart == x2+1)
					{
						prev->XStop = stop;
						MyRemove(Columns, column);
					}
					else
					{
						MyInsert(Columns, new MaskColumn(x1, x2), prev);
					}
					find = FALSE;
					break;
				}
			}
		}
		else
		{
			if(x2 >= stop)
			{
				struct MaskColumn *dummy = column;
				column = (struct MaskColumn *)column->Next;
				MyRemove(Columns, dummy);
				continue;
			}
			else
			{
				if(start <= x2)
				{
					struct MaskColumn *prev = (struct MaskColumn *)column->Prev;
					prev->XStop = stop;

					struct MaskColumn *dummy = column;
					column = (struct MaskColumn *)column->Next;
					MyRemove(Columns, dummy);
				}
				break;
			}
		}

		column = (struct MaskColumn *)column->Next;
	}

	if(find)
		Columns.MyAddTail(new MaskColumn(x1, x2));

/*	printf("AFTER: %d, %d\n", x1, x2);
	first = First();
	while(first)
	{
		printf("\t%d, %d (%p)\n", first->XStart, first->XStop, first);
		first = (struct MaskColumn *)first->Next;
	}
	printf("--------------\n");
*/
	BOOL res;
	if(res = Columns.First ? TRUE : FALSE)
	{
		XMin = First()->XStart;
		XMax = MAX(x2, XMax);

		struct MaskColumn *first = First();
		LONG last_x = -1;
		while(first)
		{
/*			if(first->XStart <= last_x)
			{
				D(("X add error: %d, %d\n", x1, x2));

				struct MaskColumn *first = First();
				while(first)
				{
					D(("\t%d, %d\n", first->XStart, first->XStop));
					first = (struct MaskColumn *)first->Next;
				}
			}
			else */last_x = first->XStop;
			first = (struct MaskColumn *)first->Next;
		}
	}
	return res;
}

BOOL MaskLine::SubMask (LONG x1, LONG x2)
{
	struct MaskColumn *column = First();
	while(column)
	{
		LONG start = column->XStart, stop = column->XStop;
		if(x1 <= stop && x2 >= start)
		{
			if(x1 <= start)
			{
				if(x2 < stop)
				{
					column->XStart = x2+1;
				}
				else
				{
					struct MaskColumn *dummy = column;
					column = (struct MaskColumn *)column->Next;
					MyRemove(Columns, dummy);
					continue;
				}
			}
			else if(x2 >= stop)
			{
				column->XStop = x1-1;
			}
			else
			{
				struct MaskColumn *n = new MaskColumn(column);
				MyInsert(Columns, n, column);
				n->XStart = x2+1;

				column->XStop = x1-1;
				column = n;
			}
		}
		else if(start >= x2)
		{
			break;
		}

		column = (struct MaskColumn *)column->Next;
	}

	BOOL res;
	if(res = Columns.First ? TRUE : FALSE)
	{
		XMin = First()->XStart;
		if(x2 > XMax)
			XMax = x1-1;

		struct MaskColumn *first = First();
		LONG last_x = -1;
		while(first)
		{
/*			if(first->XStart <= last_x)
			{
				D(("X sub error: %d, %d\n", last_x, first->XStart));

				struct MaskColumn *first = First();
				while(first)
				{
					D(("\t%d, %d\n", first->XStart, first->XStop));
					first = (struct MaskColumn *)first->Next;
				}
			}
			else */last_x = first->XStop;
			first = (struct MaskColumn *)first->Next;
		}
	}
	return res;
}

BOOL MaskLine::operator == (struct MaskLine *other)
{
	struct MaskColumn *column = First(), *o_column = other->First();
	if(XMax == other->XMax)
	{
		while(column && o_column)
		{
			if(column->XStart != o_column->XStart || column->XStop != o_column->XStop)
				break;

			column = (struct MaskColumn *)column->Next;
			o_column = (struct MaskColumn *)o_column->Next;
		}
	}
	return !(column || o_column);
}

BOOL ClipMask::SubMask (struct Rectangle &bounds)
{
	LONG x1 = bounds.MinX, y1 = bounds.MinY;
	LONG x2 = bounds.MaxX, y2 = bounds.MaxY;

	struct MaskLine *line = First();
	while(line)
	{
		LONG start = line->YStart, stop = line->YStop;
		if(y1 <= stop && y2 >= start && x1 <= line->XMax && x2 >= line->XMin)
		{
			if(y1 > start)
			{
				struct MaskLine *n = line->MakeCopy();
				MyInsert(Lines, n, line->Prev);
				n->YStart = start;
				n->YStop = y1-1;

				line->YStart = y1;
			}

			struct MaskLine *t_line = line;

			if(y2 < stop)
			{
				struct MaskLine *n = line->MakeCopy();
				MyInsert(Lines, n, line);
				n->YStart = y2+1;
				n->YStop = stop;
				line->YStop = y2;
				line = n;
			}

			if(!t_line->SubMask(x1, x2))
			{
				line = (struct MaskLine *)t_line->Next;
				MyRemove(Lines, t_line);
				continue;
			}

			/* try to colapse lines */
/*			struct MaskLine *prev = (struct MaskLine *)line->Prev;
			if(prev && *line == prev)
			{
				line->YStart = prev->YStart;
				MyRemove(Lines, prev);
			}
*/		}
		else if(start >= y2)
		{
			break;
		}

		line = (struct MaskLine *)line->Next;
	}

	if(Lines.First)
	{
		struct MaskLine *first = First();
		LONG last_y = -1;
		while(first)
		{
/*			if(first->YStart <= last_y)
					D(("Y sub error: %d, %d\n", last_y, first->YStart))
			else	*/last_y = first->YStop;
			first = (struct MaskLine *)first->Next;
		}
	}

	return Lines.First ? TRUE : FALSE;
}

BOOL ClipMask::AddMask (struct Rectangle &bounds)
{
	LONG x1 = bounds.MinX, y1 = bounds.MinY;
	LONG x2 = bounds.MaxX, y2 = bounds.MaxY;
	LONG last_stop = -1;

	struct MaskLine *line = First();

	if(!line)
		Lines.MyAddTail(new MaskLine(x1, y1, x2, y2));

	while(line)
	{
		LONG start = line->YStart, stop = line->YStop;

		if(last_stop+1 < start && y1 < start && y2 > last_stop+1)
			MyInsert(Lines, new MaskLine(x1, MAX(y1, last_stop+1), x2, MIN(y2, start-1)), line->Prev);

		if(y1 <= stop && y2 >= start)
		{
			if(y1 > start)
			{
				struct MaskLine *n = line->MakeCopy();
				MyInsert(Lines, n, line->Prev);
				n->YStart = start;
				n->YStop = y1-1;
				line->YStart = y1;
			}

			struct MaskLine *t_line = line;

			if(y2 < stop)
			{
				struct MaskLine *n = line->MakeCopy();
				MyInsert(Lines, n, line);
				n->YStart = y2+1;
				n->YStop = stop;
				line->YStop = y2;
				line = n;
			}

			t_line->AddMask(x1, x2);
		}
		else if(start >= y2)
		{
			break;
		}

		last_stop = stop;
		line = (struct MaskLine *)line->Next;
	}

	if(!line)
	{
		line = Last();
		if(line->YStop < y2)
			Lines.MyAddTail(new MaskLine(x1, MAX(y1, line->YStop+1), x2, y2));
	}


	struct MaskLine *first = First();
	LONG last_y = -1;
	while(first)
	{
/*		if(first->YStart <= last_y)
		{
			D(("Y add error: %d, %d\n", last_y, first->YStart));

			struct MaskLine *first = First();
			while(first)
			{
				D(("\t%d, %d\n", first->YStart, first->YStop));
				first = (struct MaskLine *)first->Next;
			}
		}
		else */last_y = first->YStop;
		first = (struct MaskLine *)first->Next;
	}

	return Lines.First ? TRUE : FALSE;
}

VOID ClipMask::ForEach (struct Rectangle &rect, VOID(*func)(struct Rectangle *, APTR), APTR userdata, BOOL reverse_x, BOOL reverse_y)
{
	LONG	x1 = rect.MinX, y1 = rect.MinY,
			x2 = rect.MaxX, y2 = rect.MaxY;

	struct MaskLine *line = reverse_y ? Last() : First();
	while(line)
	{
		LONG ystart, ystop, start = line->YStart, stop = line->YStop;
		if(y1 <= stop && y2 >= start)
		{
			ystart = MAX(y1, start);
			ystop = MIN(y2, stop);

			struct MaskColumn *column = reverse_x ? line->Last() : line->First();
			while(column)
			{
				LONG xstart, xstop, start = column->XStart, stop = column->XStop;
				if(x1 <= stop && x2 >= start)
				{
					xstart = MAX(x1, start);
					xstop = MIN(x2, stop);

					struct Rectangle rect = { xstart, ystart, xstop, ystop };
					func(&rect, userdata);
				}
				else if(reverse_x ? stop < x1 : start > x2)
				{
					break;
				}
				column = (struct MaskColumn *)(reverse_x ? column->Prev : column->Next);
			}
		}
		else if(reverse_y ? stop < y1 : start > y2)
		{
			break;
		}
		line = (struct MaskLine *)(reverse_y ? line->Prev : line->Next);
	}
}

VOID ClipMask::ForEachInverse (struct Rectangle &rect, VOID(*func)(struct Rectangle *, APTR), APTR userdata)
{
	LONG	x1 = rect.MinX, y1 = rect.MinY,
			x2 = rect.MaxX, y2 = rect.MaxY;

	struct MaskLine *line = First();

	LONG ystart, ystop;
	if(y1 < (ystart = line ? line->YStart : y2+1))
	{
		struct Rectangle rect = { x1, y1, x2, MIN(y2, ystart-1) };
		func(&rect, userdata);
	}

	while(line)
	{
		LONG start = line->YStart, stop = line->YStop;
		if(y1 <= stop && y2 >= start)
		{
			ystart = MAX(y1, start);
			ystop = MIN(y2, stop);

			struct MaskColumn *column = line->First();
			LONG start = x1, stop;

			do {

				stop = column ? column->XStart-1 : x2;
				if(x1 <= stop && x2 >= start)
				{
					struct Rectangle rect = { MAX(x1, start), ystart, MIN(x2, stop), ystop };
					if(rect.MinX <= rect.MaxX && rect.MinY <= rect.MaxY)
						func(&rect, userdata);
				}
				else if(start > x2)
				{
					break;
				}

				if(column)
				{
					start = column->XStop+1;
					column = (struct MaskColumn *)column->Next;
				}

			}	while(stop < x2);

		}
		else if(start > y2)
		{
			break;
		}

		line = (struct MaskLine *)line->Next;

		if((ystop = line ? line->YStart-1 : y2) != stop && y1 <= ystop && y2 > stop)
		{
			struct Rectangle rect = { x1, MAX(y1, stop+1), x2, MIN(y2, ystop) };
			func(&rect, userdata);
		}
	}
}

struct BuildCRData
{
	struct Layer_Info *li;
	struct ClipRect *first;
	struct ClipRect *last;
	struct ClipRect *start_of_row;
};

VOID AddClipRect (struct Rectangle *rect, APTR userdata)
{
	struct BuildCRData *udata = (struct BuildCRData *)userdata;

	struct ClipRect *cr = NewClipRect(udata->li);
	cr->bounds = *rect;

	if(!udata->first)
	{
		udata->first = udata->start_of_row = cr;
	}
	else
	{
		udata->last->Next = cr;
		cr->prev = udata->last;

		if(cr->bounds.MinY > udata->last->bounds.MinY)
		{
			SetLastInRow(udata->start_of_row, udata->last);
			SetNextRow(udata->start_of_row, cr);
			SetPrevRow(cr, udata->start_of_row);
			udata->start_of_row = cr;
		}
	}
	udata->last = cr;
}

VOID ClipMask::ExportAsClipRect (struct Layer *l)
{
	DelClipRects(l);
	struct BuildCRData build = { l->LayerInfo, NULL };
	ForEach(l->bounds, AddClipRect, &build);
	if(l->ClipRect = build.first)
	{
		SetPrevRow(build.first, build.start_of_row);
		SetLastInRow(build.start_of_row, build.last);
	}
}

MaskLine::MaskLine (struct ClipRect *cr) : Columns()
{
	LONG y = YStart = cr->bounds.MinY;
	YStop = cr->bounds.MaxY;
	XMin = cr->bounds.MinX;

	while(cr && cr->bounds.MinY == y)
	{
		Columns.MyAddTail(new MaskColumn(cr->bounds.MinX, cr->bounds.MaxX));
		XMax = cr->bounds.MaxX;
		cr = cr->Next;
	}
}

ClipMask::ClipMask (struct ClipRect *cr) : Lines()
{
	while(cr)
	{
		Lines.MyAddTail(new MaskLine(cr));
		cr = NextRow(cr);
	}
}

VOID AddIntersectionFunction (struct Rectangle *rect, APTR userdata)
{
	struct ClipMask *mask = (struct ClipMask *)userdata;
	mask->AddMask(*rect);
}

struct ClipMask *ClipMask::Union (struct Region *region, LONG offsetx, LONG offsety)
{
	struct ClipMask *mask = new ClipMask();

	struct RegionRectangle *rect = region->RegionRectangle;
	offsetx += region->bounds.MinX;
	offsety += region->bounds.MinY;
	while(rect)
	{
		struct Rectangle bound = rect->bounds;
		bound.MinX += offsetx;
		bound.MinY += offsety;
		bound.MaxX += offsetx;
		bound.MaxY += offsety;
		ForEach(bound, AddIntersectionFunction, mask);

		rect = rect->Next;
	}

	return mask;
}

#ifdef DEBUG

VOID ClipMask::Dump ()
{
	D(DBF_ALWAYS, ("--- ClipMask Dump ---"))
	struct MaskLine *line = First();
	while(line)
	{
		struct MaskColumn *column = line->First();
		while(column)
		{
			D(DBF_ALWAYS, ("%p: %d, %d, %d, %d",  line, column->XStart, line->YStart, column->XStop, line->YStop))
			column = (struct MaskColumn *)column->Next;
		}
		line = (struct MaskLine *)line->Next;
	}
	D(DBF_ALWAYS, ("---"));
}

#endif

/*
VOID MyScreen::Fill (LONG x1, LONG y1, LONG x2, LONG y2, VOID(*filler)(struct RastPort *, LONG, LONG, LONG, LONG, LONG, LONG, APTR), APTR userdata, ULONG flags)
{
	if(InverseMask)
	{
		InverseMask = FALSE;
		invFill(x1, y1, x2, y2, filler, userdata, flags);
		InverseMask = TRUE;
		return;
	}

	struct MaskLine *line = (flags & FLG_FillReverseY) ? Last() : First();
	while(line)
	{
		LONG ystart, ystop, start = line->YStart, stop = line->YStop;
		if(y1 <= stop && y2 >= start)
		{
			ystart = MAX(y1, start);
			ystop = MIN(y2, stop);

			struct MaskColumn *column = (flags & FLG_FillReverseX) ? line->Last() : line->First();
			while(column)
			{
				LONG xstart, xstop, start = column->XStart, stop = column->XStop;
				if(x1 <= stop && x2 >= start)
				{
					xstart = MAX(x1, start);
					xstop = MIN(x2, stop);

					filler(&RPort, XOffset+xstart, YOffset+ystart, XOffset+xstop, YOffset+ystop, xstart-x1+invOffsetX, ystart-y1+invOffsetY, userdata);
				}
				else if((flags & FLG_FillReverseX) ? stop < x1 : start > x2)
				{
					break;
				}

				column = (struct MaskColumn *)((flags & FLG_FillReverseX) ? column->Prev : column->Next);
			}
		}
		else if((flags & FLG_FillReverseY) ? stop < y1 : start > y2)
		{
			break;
		}

		line = (struct MaskLine *)((flags & FLG_FillReverseY) ? line->Prev : line->Next);
	}
}
*/
