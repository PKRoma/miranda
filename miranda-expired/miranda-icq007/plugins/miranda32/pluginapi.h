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

/**********************************************************************************\
 *																			      *
 * This header file is required for writing plugins for Miranda ICQ version 0.0.6 *
 * For an updated version, visit http://miranda-icq.sourceforge.net				  *
 *																				  *
\**********************************************************************************/

#ifndef _MIRANDA_PLUGIN_API_H_
#define _MIRANDA_PLUGIN_API_H_

#include <windows.h>

#define MIRANDAPLUGIN_API __declspec(dllexport)

// Exported Functions - these functions are required

/* Load: Requests a plugin to load itself
 *
 * hwnd:  [I] Windows handle of miranda
 * hinst: [I] Instance handle of miranda
 * title: [O] Name of plugin, maximum MAX_PLUG_TITLE characters
 *
 * returns: TRUE if plugin loaded, FALSE if load failed
 */
MIRANDAPLUGIN_API int __stdcall Load(HWND hwnd, HINSTANCE hinst, char* title);

/* Unload: Requests a plugin to unload itself
 *
 * returns: TRUE if plugin unloaded, FALSE if unload failed
 */
MIRANDAPLUGIN_API int __stdcall Unload(void);

/* Notify: Informs plugin an events has occured
 *
 * msg: [I] ID of event
 * wp:  [I] Events specific data
 * lp:  [I] Events specific data
 *
 * returns: command specific data
 */
MIRANDAPLUGIN_API int __stdcall Notify(long msg, WPARAM wp, LPARAM lp);

// Maximum length for the title of the plugin
#define MAX_PLUG_TITLE 100  

/*
 * Events must return FALSE unless otherwise specified.
 /
/*
 * Events supported as of Miranda 0.0.6
 */
#define PM_SHOWOPTIONS			0	// Plugin should display options window
#define PM_STATUSCHANGE			1	// Users online status has changed, wParam contains status
#define PM_CONTACTSTATUSCHANGE	2	// A user's status as chagned, lParam is a pointer to the CONTACT struct of that user, wparam, users old status
#define PM_GOTMESSAGE			3	// Someone has sent us a message, lparam = pointer MESSAGE struct	[return TRUE to stop miranda from displaying msg]
#define PM_GOTURL				4	// Someone sent us a url, lparam=pointer to MESSAGE strcut			[return TRUE to stop miranda from displaying msg]
#define PM_ICQDEBUGMSG			5	// Arbitary debug msg from icqlib, lparam pointer to str, wparam len of str
#define PM_REGCALLBACK			6   // Miranda has send callback struct, make copy of function pointers to call Miranda
/*
 * Events supported as of Miranda 0.0.7
 */
#define PM_PING					7   // Happens every minutes or so, allows plugins to process without creating a thread
#define PM_SAVENOW				8   // Plug must save its options now! (even if already saved them)
#define PM_PROPSHEET			9   // Set up property sheet. wParam, int* of page in array, increment if plugin adds page. lParam is PROPSHEETPAGE*

// Status indicators
#define STATUS_OFFLINE     (-1L)
#define STATUS_ONLINE      0x0000L
#define STATUS_AWAY        0x0001L
#define STATUS_DND         0x0002L 
#define STATUS_NA          0x0004L 
#define STATUS_OCCUPIED    0x0010L 
#define STATUS_FREE_CHAT   0x0020L
#define STATUS_INVISIBLE   0x0100L

// If you do not include global.h, include this structure
#if 0
typedef struct {
	unsigned int		uin;
	char				nick[30];
	char				first[30];
	char				last[30];
	char				email[50];
	int					status;
	unsigned int		flags;
} CONTACT;
#endif

// If you do not include msgque.h, include this structure

//WARNING/NOTE/IMPORTANT!!!!!!!: the struct below is out of date, use msgque.h instead
#if 0
typedef struct
{  
	BOOL isurl;
	BOOL beingread; 
	unsigned long uin;
	unsigned char hour;
	unsigned char minute;
	unsigned char day;
	unsigned char month;
	unsigned short year;
	char *msg;				//becomes desc in URL
	char *url; 
} MESSAGE;
#endif

// Callback structure, used to call miranda
typedef struct
{
	// send and icq message
	void (*pSendMessage)(int uin, char *msg);
	// send and icq url
	void (*pSendURL)(int uin, char *desc, char *url);
	// get the contact info for an UIN
	CONTACT* (*pGetContact)(int uin);
	// change the users icq status
	void (*pChangeStatus)(int status);
	// get the users icq status
	int (*pGetStatus)(void);
	// get an int from the reg
	void (*pGetPluginInt)(char *plugin, int *val, char *name, int def);
	// get a str from the reg
	void (*pGetPluginStr)(char *plugin, char *val, char *name, char *def);
	// set int in reg
	void (*pSetPluginInt)(char *plugin, int val, char *name);
	// set str in reg
	void (*pSetPluginStr)(char *plugin, char *val, char *name);
} PI_CALLBACK;

#endif
