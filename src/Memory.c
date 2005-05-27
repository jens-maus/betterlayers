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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include "Debug.h"

#define MEMWATCH
//#define STATISTICS
//#define WASTEMEM

struct SignalSemaphore MemorySemaphore;
APTR MemoryPool = NULL;

#ifdef MEMWATCH
struct MemNode
{
	struct MemNode *next;
	APTR addr;
};

ULONG Allocations = 0;

struct MemNode *MemoryList = NULL;
struct MemNode *LastNode = (MemNode *)&MemoryList;
#endif

#ifdef WASTEMEM
UBYTE *Mem; ULONG Size = 64*1024, Current = 0;
#endif

#ifdef STATISTICS
ULONG Sizes[1024], CntSizes[1024], MaxSizes[1024], Total = 0, Max = 0;
#endif

APTR operator new (size_t bytes)
{
	ObtainSemaphore(&MemorySemaphore);

#ifdef STATISTICS
	ULONG i = bytes< 1024 ? bytes : 0;
	Sizes[i]++;
	CntSizes[i]++;
	if(MaxSizes[i] < CntSizes[i])
		MaxSizes[i] = CntSizes[i];

	Total += bytes;
	if(Total > Max)
		Max = Total;
#endif

#ifdef MEMWATCH
	ULONG *mem;
	if(mem = (ULONG *)AllocPooled(MemoryPool, bytes+12))
	{
		struct MemNode *newnode;
		if(newnode = (MemNode *)AllocPooled(MemoryPool, sizeof(MemNode)))
		{
			newnode->next = NULL;
			newnode->addr = mem;
			LastNode->next = newnode;
			LastNode = newnode;
		}

		*mem++ = (ULONG)bytes;
		*mem++ = 0xC0DEDBAD;
		STRPTR end = (STRPTR)mem;
		end += bytes;
		*end++ = 0xC0;
		*end++ = 0xDE;
		*end++ = 0xDB;
		*end++ = 0xAD;

		Allocations++;
	}
	else
	{
		DisplayBeep(NULL);
		D(DBF_MEMORY, ("Allocation at %d bytes failed!", bytes))
	}
#else

#ifdef WASTEMEM
	UBYTE *mem;
	if(bytes > Size)
	{
		mem = (UBYTE *)AllocMem(bytes, MEMF_ANY | MEMF_CLEAR);
	}
	else
	{
		if(bytes > Current)
		{
			Mem = (UBYTE *)AllocMem(Size, MEMF_ANY | MEMF_CLEAR);
			Current = Size;
		}
		mem = &Mem[Current -= bytes];
	}
#else
	ULONG *mem;
	if(mem = (ULONG *)AllocPooled(MemoryPool, bytes+4))
		*mem++ = (ULONG)bytes;
#endif

#endif

	ReleaseSemaphore(&MemorySemaphore);
   return((APTR)mem);
}

VOID operator delete (APTR mem, size_t bytes)
{
#ifndef WASTEMEM
	if(mem)
	{
		ObtainSemaphore(&MemorySemaphore);
		ULONG *tmp = (ULONG *)mem;

#ifdef MEMWATCH
		ULONG check1 = *--tmp;
		bytes = *--tmp;

		struct MemNode *previous = (MemNode *)&MemoryList;
		struct MemNode *memlist = MemoryList;
		BOOL match = FALSE;
		while(memlist && !match)
		{
			if(memlist->addr == tmp)
			{
				if(LastNode == memlist)
					LastNode = previous;
				previous->next = memlist->next;
				FreePooled(MemoryPool, memlist, sizeof(MemNode));
				match = TRUE;
			}
			previous = memlist;
			memlist = memlist->next;
		}

		if(!match)
		{
			DisplayBeep(NULL);
			D(DBF_MEMORY, ("Unmatched memory free (0x%lx, size %ld)", tmp, bytes))
		}
		else
		{
			STRPTR end = ((STRPTR)mem)+bytes;
			ULONG check2 = *end++ << 24;
			check2 |= *end++ << 16;
			check2 |= *end++ << 8;
			check2 |= *end++;

			if(check1 != 0xC0DEDBAD)
			{
				DisplayBeep(NULL);
				D(DBF_MEMORY, ("Front wall of allocation at %ld bytes is trashed! (0x%lx)", bytes, tmp))
			}

			if(check2 != 0xC0DEDBAD)
			{
				DisplayBeep(NULL);
				D(DBF_MEMORY, ("Back wall of allocation at %ld bytes is trashed! (0x%lx: 0x%08lx)", bytes, mem, check2))
			}

			Allocations--;
			memset(tmp, 0xff, bytes+12);
			FreePooled(MemoryPool, (APTR)tmp, bytes+12);
		}
#else
		bytes = *--tmp;
		FreePooled(MemoryPool, (APTR)tmp, bytes+4);
#endif

#ifdef STATISTICS
		Total -= bytes;
		CntSizes[bytes< 1024 ? bytes : 0]--;
#endif

		ReleaseSemaphore(&MemorySemaphore);
	}
#endif
}

VOID _INIT_4_InitMem ()
{
	memset(&MemorySemaphore, 0, sizeof(SignalSemaphore));
	InitSemaphore(&MemorySemaphore);
	MemoryPool = CreatePool(MEMF_CLEAR | MEMF_ANY, 10*1024, 5*1024);
//	MemoryPool = CreatePool(MEMF_CLEAR | MEMF_ANY, 512, 64);
}

VOID _EXIT_4_CleanupMem ()
{
#ifdef STATISTICS
	printf("%10s == %3d\n", "RastPort", sizeof(RastPort));
	printf("%10s == %3d\n", "Layer", sizeof(Layer));
	printf("%10s == %3d\n", "ClipRect", sizeof(ClipRect));
	printf("%10s == %3d\n", "Layer_Info", sizeof(Layer_Info));

	for(ULONG i = 0; i < 1024; i++)
		if(Sizes[i])
			printf("%5d: %6d %6d\n", i, Sizes[i], MaxSizes[i]);
	printf("Max: %6d\n", Max);
#endif

#ifdef MEMWATCH
	if(Allocations)
	{
		DisplayBeep(NULL);
		D(DBF_MEMORY, ("%ld allocation(s) left", Allocations))
		if(Allocations < 30)
		{
			struct MemNode *memlist = MemoryList;
			for(UWORD i = 0; i < Allocations; i++)
			{
				D(DBF_MEMORY, ("\t%2ld: At $%lx - size %ld", i+1, memlist->addr, *((ULONG *)memlist->addr)))
				memlist = memlist->next;
			}
//			Wait(4096);
		}
	}
#endif

	if(MemoryPool)
		DeletePool(MemoryPool);
}
