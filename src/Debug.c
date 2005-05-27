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

#ifdef DEBUG

#include <string.h>
#include <stdio.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "Debug.h"

#define DEBUG_VAR "BLayers"

ULONG DebugFlags;

struct NotifyRequest nr;

VOID _INIT_9_NotifyEnvironment ()
{
	nr.nr_Name = "Env:" DEBUG_VAR;
	nr.nr_Flags = NRF_SEND_SIGNAL;
	nr.nr_stuff.nr_Signal.nr_Task = FindTask(NULL);
	nr.nr_stuff.nr_Signal.nr_SignalNum = SIGBREAKB_CTRL_D;

	StartNotify(&nr);
	ReadDebugFlags();
}

VOID _EXIT_9_NotifyEnvironment ()
{
	EndNotify(&nr);
}

VOID ReadDebugFlags ()
{
	static struct { STRPTR Keyword; ULONG Flag; } Keywords[] =
	{
		{ "obsolete",	DBF_OBSOLETE },
		{ "memory",		DBF_MEMORY },
		{ "resources",	DBF_RESOURCES },
		{ "layers",		DBF_LAYERS },
		{ "important",	DBF_IMPORTANT },
		{ "graphics",	DBF_GRAPHICS },
		{ "masks",		DBF_MASKS },
		{ "locks",		DBF_LOCKS },
		{ "clipping",	DBF_CLIPPING },
		{ "all",			DBF_ALL },
		{ NULL, 0 }
	};

	DebugFlags = DBF_ALWAYS;

	UBYTE buf[256];
	if(GetVar(DEBUG_VAR, buf, sizeof(buf), 0) != -1)
	{
		STRPTR token = strtok(buf, " ,|");
		while(token)
		{
			for(ULONG i = 0; Keywords[i].Keyword; i++)
			{
				if(!stricmp(token, Keywords[i].Keyword))
					DebugFlags |= Keywords[i].Flag;
			}
			token = strtok(NULL, " ,|");
		}
	}
}

STRPTR FunctionPart (STRPTR f)
{
	STRPTR p;
	if(p = strchr(f, '('))
		*p = '\0';
	return f;
}

BOOL TestFlag (ULONG flag)
{
	return (DebugFlags & flag) ? TRUE : FALSE;
}

#else

/* the compiler complains about "empty source" if this is omitted! */
void crap ();

#endif
