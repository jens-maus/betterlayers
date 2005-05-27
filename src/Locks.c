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

#include <proto/exec.h>
#include <graphics/clip.h>
#include <graphics/layers.h>

#include "BetterLayersNG.h"
#include "Debug.h"
#include "PatchProtos.h"

VOID Enter (struct Layer_Info *li)
{
	pLockLayers(li);
}

VOID Leave (struct Layer_Info *li)
{
	pUnlockLayers(li);
}

void pLockLayer (REG(a0) long dummy, REG(a1) struct Layer *layer)
{
	D(DBF_LOCKS, ("layer 0x%lx", layer))
	ObtainSemaphore(&layer->Lock);
}

void pUnlockLayer (REG(a0) struct Layer *layer)
{
	ReleaseSemaphore(&layer->Lock);
	D(DBF_LOCKS, ("layer 0x%lx", layer))
}

void pLockLayers (REG(a0) struct Layer_Info *li)
{
	pLockLayerInfo(li);
	D(DBF_LOCKS, ("LockLayersCount %ld", li->LockLayersCount))
	ObtainSemaphoreList((List *)&li->gs_Head);
	if(++li->LockLayersCount == 1)
	{
		for(struct Layer *l = li->top_layer; l; l = l->back)
		{
			if(l->ClipRegion)
				CoreEndUpdate(l);
		}
	}
}

void pUnlockLayers (REG(a0) struct Layer_Info *li)
{
	D(DBF_LOCKS, ("LockLayersCount %ld", li->LockLayersCount))
	if(--li->LockLayersCount == 0)
	{
		for(struct Layer *l = li->top_layer; l; l = l->back)
		{
			if(l->ClipRegion)
				CoreBeginUpdate(l);
		}
	}

	ReleaseSemaphoreList((List *)&li->gs_Head);
	pUnlockLayerInfo(li);
}

void pLockLayerInfo (REG(a0) struct Layer_Info *li)
{
	D(DBF_LOCKS, ("li 0x%lx", li))
	ObtainSemaphore(&li->Lock);
}

void pUnlockLayerInfo (REG(a0) struct Layer_Info *li)
{
	ReleaseSemaphore(&li->Lock);
	D(DBF_LOCKS, ("li 0x%lx", li))
}
