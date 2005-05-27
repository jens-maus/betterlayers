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

#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUG

#ifndef _PROTO_DOS_H
#include <proto/dos.h>
#endif

extern "C" kprintf (STRPTR, ...);

#define DBF_ALWAYS		(1<<0)
#define DBF_OBSOLETE		(1<<1)
#define DBF_MEMORY		(1<<2)
#define DBF_RESOURCES	(1<<3)
#define DBF_LAYERS		(1<<4)
#define DBF_IMPORTANT	(1<<5)
#define DBF_GRAPHICS		(1<<6)
#define DBF_MASKS			(1<<7)
#define DBF_LOCKS			(1<<8)
#define DBF_CLIPPING		(1<<9)
#define DBF_ALL			(~0)

STRPTR FunctionPart (STRPTR f);
BOOL TestFlag (ULONG flag);
VOID ReadDebugFlags ();

#define D(flag,x)								\
													\
	if(TestFlag(flag))						\
	{												\
		kprintf("%-18.18s %-30.30s: ",	\
			FilePart(__FILE__),				\
			FunctionPart(__FUNC__));		\
		kprintf x ;								\
		kprintf("\n");							\
	}

#else

#define D(flag,x) ;

#endif

#endif
