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

#ifndef MASK_H
#define MASK_H

struct MyNode
{
	virtual ~MyNode () { ; }
	struct MyNode *Next, *Prev;
};

struct MyList
{
	MyList ()									{ Clear(); }
	VOID MyAddTail (struct MyNode *n)	{ n->Prev = First ? Last : NULL; Last = Last->Next = n; }
	VOID Clear ()								{ First = NULL; Last = (struct MyNode *)&First; }

	struct MyNode *First, *Last;
};

struct MaskColumn : public MyNode
{
	MaskColumn (LONG x1, LONG x2);
	MaskColumn (struct MaskColumn *src);

	LONG XStart, XStop;
};

struct MaskLine : public MyNode
{
	MaskLine (LONG x1, LONG y1, LONG x2, LONG y2);
	MaskLine (struct MaskLine *src);
	~MaskLine ();
	struct MaskLine *MakeCopy ();
	BOOL SubMask (LONG x1, LONG x2);
	BOOL AddMask (LONG x1, LONG x2);
	BOOL operator == (struct MaskLine *);

	MaskLine (struct ClipRect *cr);

	LONG YStart, YStop, XMin, XMax;
	struct MyList Columns;
	struct MaskColumn *First ()	{ return (struct MaskColumn *)Columns.First; }
	struct MaskColumn *Last ()		{ return (struct MaskColumn *)Columns.Last; }
};

struct ClipMask
{
	ClipMask () : Lines() { ; }
	~ClipMask () { ClearMask(); }

	VOID ClearMask ();
	VOID SetMask (struct BitMap *bmp);
	VOID SetMask (LONG x0, LONG y0, LONG x1, LONG y1);
	BOOL SubMask (struct Rectangle &bounds);
	BOOL AddMask (struct Rectangle &bounds);
	VOID ForEach (struct Rectangle &rect, VOID(*func)(struct Rectangle *, APTR), APTR userdata, BOOL reverse_x = FALSE, BOOL reverse_y = FALSE);
	VOID ForEachInverse (struct Rectangle &rect, VOID(*func)(struct Rectangle *, APTR), APTR userdata);

	VOID ExportAsClipRect (struct Layer *l);

	ClipMask (struct ClipRect *cr);
	struct ClipMask *Union (struct Region *region, LONG offsetx, LONG offsety);

#ifdef DEBUG
	VOID Dump ();
#endif

	struct MyList Lines;
	struct MaskLine *First ()		{ return (struct MaskLine *)Lines.First; }
	struct MaskLine *Last ()		{ return (struct MaskLine *)Lines.Last; }
};

#endif
