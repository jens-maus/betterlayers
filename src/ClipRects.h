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

#ifndef CLIPRECTS_H
#define CLIPRECTS_H

#define REG(x) register __ ## x
#define BETWEEN(a, min, max) (a >= min && a <= max)

#define SetExtraInfo(layer,cr) (layer->reserved1 = (ULONG)cr)
#define GetExtraInfo(layer) ((MyClipRect *)layer->reserved1)

struct MyClipRect
{
	MyClipRect ();
	MyClipRect (struct ClipRect *cr);
	~MyClipRect ();

	VOID Link (struct Row *row, struct Row *prev);
	VOID Unlink (struct Row *row);
	struct ClipRect *OrderClipRects ();

	VOID Sub (struct Layer *l, struct Rectangle *rect);
	VOID AddRectangle (struct Rectangle *rect);
	BOOL MoveToLayer (struct Layer *l);

	struct ClipRect *Intersect (struct Region *region);

	struct Row *FirstRow, *LastRow;
};

struct Cell
{
	SetClipRect (struct ClipRect *cr)
	{
		MinX = cr->bounds.MinX;
		MaxX = cr->bounds.MaxX;
		CR = cr;
	}

	LONG MinX, MaxX;
	struct Cell *Next, *Prev;
	struct ClipRect *CR;
};

struct Row
{
	Row () { FirstCell = LastCell = NULL; }

	Row (struct ClipRect *cr)
	{
		Next = Prev = NULL;

		FirstCell = LastCell = new Cell;
		FirstCell->Next = FirstCell->Prev = NULL;

		MinY = cr->bounds.MinY;
		MaxY = cr->bounds.MaxY;
		FirstCell->SetClipRect(cr);
	}

	VOID AddAsTail (struct Cell *cell)
	{
		cell->Next = NULL;
		if(cell->Prev = LastCell)
				LastCell = LastCell->Next = cell;
		else	FirstCell = LastCell = cell;
	}

	VOID Unlink (struct Cell *cell)
	{
		if(cell->Prev)
				cell->Prev->Next = cell->Next;
		else	FirstCell = cell->Next;

		if(cell->Next)
				cell->Next->Prev = cell->Prev;
		else	LastCell = cell->Prev;
	}

	LONG MinY, MaxY;
	struct Row *Next, *Prev;
	struct Cell *FirstCell, *LastCell;
};

VOID DeleteExtraInfo (struct Row *row);
VOID SubFromClipRects(struct Layer *l, struct Rectangle *rect);
VOID AddToClipRects (struct Layer *src, struct Layer *l);
struct ClipRect *NewClipRects (struct Layer *l);
VOID DeleteClipRects (struct ClipRect *cr);

#endif
