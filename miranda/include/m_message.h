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

#ifndef M_MESSAGE_H__
#define M_MESSAGE_H__ 1

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

#define ME_MSG_WINDOWEVENT "MessageAPI/WindowEvent"
//wParam=(WPARAM)(MessageWindowEventData*)hWindowEvent;
//lParam=0
//Event types
#define MSG_WINDOW_EVT_OPENING 1 //window is about to be opened (uType is not used)
#define MSG_WINDOW_EVT_OPEN    2 //window has been opened (uType is not used)
#define MSG_WINDOW_EVT_CLOSING 3 //window is about to be closed (uType is not used)
#define MSG_WINDOW_EVT_CLOSE   4 //window has been closed (uType is not used)
#define MSG_WINDOW_EVT_CUSTOM  5 //custom event for message plugins to use (uType may be used)

typedef struct {
   int cbSize;
   HANDLE hContact;
   HWND hwndWindow; // top level window for the contact
   const char* szModule; // used to get plugin type (which means you could use local if needed)
   unsigned int uType; // see event types above
   unsigned int uFlags; // might be needed for some event types
   void *local; // used to store pointer to custom data
} MessageWindowEventData;

#define MS_MSG_GETWINDOWAPI "MessageAPI/WindowAPI"
//wparam=0
//lparam=0
//Returns a dword with the current message api version 
//Current version is 0,0,0,1

#endif // M_MESSAGE_H__

