/*
Miranda ICQ: the free icq client for MS Windows 
Copyright (C) 2000  Roland Rabien

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

For more information, e-mail figbug@users.sourceforge.net
*/
//#define WINVER 0x0500
//#define _WIN32_WINNT 0x0500

#include "miranda.h"
#include "msgque.h"
#include "../icqlib/icq.h"

HINSTANCE				ghInstance;
HINSTANCE				ghIcons;					//hinst of DLL containing Icons
HANDLE					ghInstMutex;				//mutex for this instance/profile
HWND					ghwnd;
HWND					hwndModeless;
HWND					hProp;
OPTIONS					opts;
OPTIONS_RT				rto;
ICQLINK					link, *plink;
char					mydir[MAX_PATH];			//the dir we are operating from (look for files in etc).
													//MUST have a trailing slash (\)
char					myprofile[MAX_PROFILESIZE]; //contains the name of the profile (default is 'default')

int						plugincnt;

MESSAGE					*msgque;
int						msgquecnt;

PROPSHEETPAGE			*ppsp;

TmySetLayeredWindowAttributes mySetLayeredWindowAttributes;
