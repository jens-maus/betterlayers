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

#define RELY_ON_SIZE
//#define CLEAR_MEM

struct SignalSemaphore MemorySemaphore;
UBYTE *MemPool;
ULONG FragmentSize = 32576, Current;
struct MemoryChunk
{
/*	union
	{
*/		ULONG Size;
		struct MemoryChunk *Next;
//	};
} *MemChunks[100];

#ifdef DEBUG
ULONG AllocCnt;

VOID DumpMemoryInfo ()
{
	ULONG total = 0;
	D(DBF_ALWAYS, ("Memory cache contains:"))
	for(ULONG i = 0; i < 100; i++)
	{
		ULONG cnt = 0;
		struct MemoryChunk *first = MemChunks[i];
		while(first)
		{
			cnt++;
			first = first->Next;
		}

		if(cnt)
		{
			ULONG size = i << 3;
			total += cnt * size;
			D(DBF_ALWAYS, ("%8ld chunk(s) available of %3ld bytes", cnt, size))
		}
	}
	D(DBF_ALWAYS, ("Cached %ld bytes, chunk has %ld bytes left, %ld bytes in use", total, Current, AllocCnt))
}
#endif

APTR operator new (size_t bytes)
{
#ifdef DEBUG
	AllocCnt += bytes;
#endif

	struct MemoryChunk *mem;

#ifdef RELY_ON_SIZE
	bytes = (bytes+7) & ~7;
	ULONG i = bytes >> 3;
#else
	bytes += sizeof(MemoryChunk);
	ULONG i = bytes >> 1;
#endif

#ifdef DEBUG
	if(bytes & 1)	D(DBF_MEMORY, ("Unaligned memory size (%ld)", bytes))
	if(i >= 100)	D(DBF_MEMORY, ("Memory size to big (%ld)", bytes))
#endif

	ObtainSemaphore(&MemorySemaphore);
	if(mem = MemChunks[i])
	{
		MemChunks[i] = mem->Next;
#ifdef CLEAR_MEM
		memset(mem, 0, bytes);
#else
#ifdef DEBUG
		memset(mem, -1, bytes);
#endif
#endif
	}
	else
	{
		if(bytes > Current)
		{
#ifdef DEBUG
			if(MemPool) D(DBF_MEMORY, ("0x%lx First pool exceeded (%ld > %ld -- %ld)", MemPool, bytes, Current, FragmentSize))
#endif
#ifdef CLEAR_MEM
			MemPool = (UBYTE *)AllocMem(FragmentSize, MEMF_ANY | MEMF_CLEAR);
#else
			MemPool = (UBYTE *)AllocMem(FragmentSize, MEMF_ANY);
#ifdef DEBUG
			memset(MemPool, -1, FragmentSize);
#endif
#endif
			Current = FragmentSize;

			if(((ULONG)MemPool) & 7)
			{
				D(DBF_MEMORY, ("0x%lx Memory pool is not aligned", MemPool))
				Current -= ((ULONG)MemPool) & 7;
			}
		}
		mem = (MemoryChunk *)&MemPool[Current -= bytes];
	}
	ReleaseSemaphore(&MemorySemaphore);

#ifdef RELY_ON_SIZE
#ifdef DEBUG
	if(((ULONG)mem) & 7) D(DBF_MEMORY, ("Unaligned memory 0x%lx (%ld)", mem, bytes))
#endif
	return mem;
#else
	mem->Size = bytes;
   return (((UBYTE *)mem)+sizeof(MemoryChunk));
#endif
}

VOID operator delete (APTR mem, size_t bytes)
{
	if(mem)
	{
#ifdef DEBUG
		AllocCnt -= bytes;
#endif

#ifdef RELY_ON_SIZE
		struct MemoryChunk *chunk = (MemoryChunk *)mem;
		bytes = (bytes+7) & ~7;
		ULONG i = bytes >> 3;
#else
		struct MemoryChunk *chunk = (MemoryChunk *)(((UBYTE *)mem)-sizeof(MemoryChunk));
		ULONG i = chunk->Size >> 1;
		if(chunk->Size-sizeof(MemoryChunk) != bytes)
			D(("size %ld, bytes %ld\n", chunk->Size-sizeof(MemoryChunk), bytes))
#endif

		ObtainSemaphore(&MemorySemaphore);
		chunk->Next = MemChunks[i];
		MemChunks[i] = chunk;
		ReleaseSemaphore(&MemorySemaphore);
	}
}

VOID _INIT_4_InitMem ()
{
	InitSemaphore(&MemorySemaphore);
}

VOID _EXIT_4_CleanupMem ()
{
#ifdef DEBUG
	if(AllocCnt)		D(DBF_MEMORY, ("Allocations left for %ld bytes", AllocCnt))
#endif

	if(MemPool)
		FreeMem(MemPool, FragmentSize);
}
