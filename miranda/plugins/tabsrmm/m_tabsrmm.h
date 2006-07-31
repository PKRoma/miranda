/*
Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2003 Miranda ICQ/IM project, 
all portions of this codebase are copyrighted to the people 
listed in contributors.txt.

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
*/

#ifndef _M_TABSRMM_H
#define _M_TABSRMM_H

//brings up the send message dialog for a contact
//wParam=(WPARAM)(HANDLE)hContact
//lParam=(LPARAM)(char*)szText
//returns 0 on success or nonzero on failure
//returns immediately, just after the dialog is shown
//szText is the text to put in the edit box of the window (but not send)
//szText=NULL will not use any text
//szText!=NULL is only supported on v0.1.2.0+
//NB: Current versions of the convers plugin use the name
//"SRMsg/LaunchMessageWindow" instead. For compatibility you should call
//both names and the correct one will work.
#define MS_MSG_SENDMESSAGE  "SRMsg/SendCommand"

//brings up the send message dialog with the 'multiple' option open and no
//contact selected.                                                  v0.1.2.1+
//wParam=0
//lParam=(LPARAM)(char*)szText
//returns 0 on success or nonzero on failure
#define MS_MSG_FORWARDMESSAGE  "SRMsg/ForwardMessage"

#define ME_MSG_WINDOWEVENT "MessageAPI/WindowEvent"
//wParam=(WPARAM)(MessageWindowEventData*)hWindowEvent;
//lParam=0
//Event types
#define MSG_WINDOW_EVT_OPENING 1 //window is about to be opened (uType is not used)
#define MSG_WINDOW_EVT_OPEN    2 //window has been opened (uType is not used)
#define MSG_WINDOW_EVT_CLOSING 3 //window is about to be closed (uType is not used)
#define MSG_WINDOW_EVT_CLOSE   4 //window has been closed (uType is not used)
#define MSG_WINDOW_EVT_CUSTOM  5 //custom event for message plugins to use (uType may be used)

#define MSG_WINDOW_UFLAG_MSG_FROM 0x00000001
#define MSG_WINDOW_UFLAG_MSG_TO   0x00000002
#define MSG_WINDOW_UFLAG_MSG_BOTH 0x00000004
    
typedef struct {
   int          cbSize;
   HANDLE       hContact;
   HWND         hwndWindow; // top level window for the contact
   const char*  szModule; // used to get plugin type (which means you could use local if needed)
   unsigned int uType; // see event types above
   unsigned int uFlags; // might be needed for some event types
   void         *local; // used to store pointer to custom data
} MessageWindowEventData;

#define TEMPLATES_MODULE "tabSRMM_Templates"
#define RTLTEMPLATES_MODULE "tabSRMM_RTLTemplates"

//Checks if there is a message window opened
//wParam=(LPARAM)(HANDLE)hContact  - handle of the contact for which the window is searched. ignored if lParam
//is not zero.
//lParam=(LPARAM)(HWND)hwnd - a window handle - SET THIS TO 0 if you want to obtain the window handle
//from the hContact.
#define MS_MSG_MOD_MESSAGEDIALOGOPENED "SRMsg_MOD/MessageDialogOpened"

//obtain the message window flags
//wParam = hContact - ignored if lParam is given.
//lParam = hwnd
//returns struct MessageWindowData *dat, 0 if no window is found
#define MS_MSG_MOD_GETWINDOWFLAGS "SRMsg_MOD/GetWindowFlags"

// custom tabSRMM events

#define tabMSG_WINDOW_EVT_CUSTOM_BEFORESEND 1

struct TABSRMM_SessionInfo {
    unsigned int cbSize;
    unsigned short evtCode;
    HWND hwnd;              // handle of the message dialog (tab)
    HWND hwndContainer;     // handle of the parent container
    HWND hwndInput;         // handle of the input area (rich edit)
    UINT extraFlags;
    UINT extraFlagsEX;
    void *local;
};

#define MS_MSG_GETWINDOWAPI "MessageAPI/WindowAPI"
//wparam=0
//lparam=0
//Returns a dword with the current message api version 
//Current version is 0,0,0,3

#define MS_MSG_GETWINDOWCLASS "MessageAPI/WindowClass"
//wparam=(char*)szBuf
//lparam=(int)cbSize size of buffer
//Sets the window class name in wParam (ex. "SRMM" for srmm.dll)

typedef struct {
	int cbSize;
	HANDLE hContact;
	int uFlags; // see uflags above
} MessageWindowInputData;

#define MSG_WINDOW_STATE_EXISTS  0x00000001 // Window exists should always be true if hwndWindow exists
#define MSG_WINDOW_STATE_VISIBLE 0x00000002
#define MSG_WINDOW_STATE_FOCUS   0x00000004
#define MSG_WINDOW_STATE_ICONIC  0x00000008

typedef struct {
	int cbSize;
	HANDLE hContact;
	int uFlags;  // should be same as input data unless 0, then it will be the actual type
	HWND hwndWindow; //top level window for the contact or NULL if no window exists
	int uState; // see window states
	void *local; // used to store pointer to custom data
} MessageWindowOutputData;

#define MS_MSG_GETWINDOWDATA "MessageAPI/GetWindowData"
//wparam=(MessageWindowInputData*)
//lparam=(MessageWindowData*)
//returns 0 on success and returns non-zero (1) on error or if no window data exists for that hcontact

// callback for the user menu entry

#define MS_TABMSG_SETUSERPREFS "SRMsg_MOD/SetUserPrefs"

// show one of the tray menus
// wParam = 0 -> session list
// wParam = 1 -> tray menu
// lParam must be 0
// 
#define MS_TABMSG_TRAYSUPPORT "SRMsg_MOD/Show_TrayMenu"

#endif
