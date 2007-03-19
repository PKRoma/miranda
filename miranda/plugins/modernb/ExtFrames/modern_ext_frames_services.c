/**************************************************************************\

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2007 Miranda ICQ/IM project,
all portions of this code base are copyrighted to Artem Shpynov and/or
the people listed in contributors.txt.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

****************************************************************************

Created: Mar 19, 2007

Author and Copyright:  Artem Shpynov aka FYR:  ashpynov@gmail.com

****************************************************************************

File contains realization of service procedures 
for modern_ext_frames.c module.

This file have to be excluded from compilation and need to be adde to project via
#include preprocessor derective in modern_ext_frames.c

\**************************************************************************/

#include "..\commonheaders.h"  //only for precompiled headers

#ifdef __modern_ext_frames_c__include_c_file   //protection from adding to compilation

/*
TODO services and events to be implemented:

#define MS_CLIST_FRAMES_ADDFRAME			"CListFrames/AddFrame"
#define MS_CLIST_FRAMES_REMOVEFRAME			"CListFrames/RemoveFrame"
#define MS_CLIST_FRAMES_SHOWALLFRAMES		"CListFrames/ShowALLFrames"
#define MS_CLIST_FRAMES_SHOWALLFRAMESTB		"CListFrames/ShowALLFramesTB"
#define MS_CLIST_FRAMES_HIDEALLFRAMESTB		"CListFrames/HideALLFramesTB"
#define MS_CLIST_FRAMES_SHFRAME				"CListFrames/SHFrame"
#define MS_CLIST_FRAMES_SHFRAMETITLEBAR		"CListFrame/SHFrameTitleBar"
#define MS_CLIST_FRAMES_ULFRAME				"CListFrame/ULFrame"
#define MS_CLIST_FRAMES_UCOLLFRAME			"CListFrame/UCOLLFrame"
#define MS_CLIST_FRAMES_SETUNBORDER			"CListFrame/SetUnBorder"
#define MS_CLIST_FRAMES_UPDATEFRAME			"CListFrame/UpdateFrame"
#define MS_CLIST_FRAMES_GETFRAMEOPTIONS		"CListFrame/GetFrameOptions"
#define MS_CLIST_FRAMES_SETFRAMEOPTIONS		"CListFrame/SetFrameOptions"
#define MS_CLIST_FRAMES_MENUNOTIFY			"CList/ContextFrameMenuNotify"

and Events

#define ME_CLIST_FRAMES_SB_SHOW_TOOLTIP		"CListFrames/StatusBarShowToolTip"
#define ME_CLIST_FRAMES_SB_HIDE_TOOLTIP		"CListFrames/StatusBarHideToolTip"

*/
#endif