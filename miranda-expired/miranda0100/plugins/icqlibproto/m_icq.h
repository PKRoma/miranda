/*
Miranda ICQ: the free icq client for MS Windows 
Copyright (C) 2000-2  Richard Hughes, Roland Rabien & Tristan Van de Vreede

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

/*********** NEW STUFF CREATED BY 0.1.1.0 PROTOCOL PLUGIN CHANGE ***********/
#include "../../miranda32/protocols/protocols/m_protosvc.h"

//extra database event type
#define ICQEVENTTYPE_WEBPAGER      1003

//extra flags for PSS_MESSAGE
#define PIMF_ROUTE_DEFAULT    0
#define PIMF_ROUTE_DIRECT     0x10000
#define PIMF_ROUTE_THRUSERVER 0x20000
#define PIMF_ROUTE_BESTWAY    0x30000
#define PIMF_ROUTE_MASK       0x30000

//start a search of all ICQ users by e-mail
//wParam=0
//lParam=(LPARAM)(const char*)email
//returns a handle to the search on success, NULL on failure
//Results are returned using the same scheme documented in PSS_BASICSEARCH
typedef struct {   //extended search result structure, used for all searches
	PROTOSEARCHRESULT hdr;
	DWORD uin;
	BYTE auth;
} ICQSEARCHRESULT;
#define MS_ICQ_SEARCHBYEMAIL   "ICQ/SearchByEmail"

//start a search of all ICQ users by details. results are returned with
//icq/searchresult and icq/searchdone events
//wParam=0
//lParam=(LPARAM)(ICQDETAILSSEARCH*)&ids
//returns a handle to the search on success, NULL on failure
//Results are returned using the same scheme documented in PSS_BASICSEARCH
typedef struct {
	char *nick;
	char *firstName;
	char *lastName;
} ICQDETAILSSEARCH;
#define MS_ICQ_SEARCHBYDETAILS   "ICQ/SearchByDetails"

//a logging message was sent from icqlib
//wParam=(WPARAM)(BYTE)level
//lParam=(LPARAM)(char*)pszMsg
//level is a detail level as defined by icqlib
#define ME_ICQ_LOG           "ICQ/Log"

//all the missing events that you were expecting to see here are elsewhere.
//incoming messages, urls, etc are added straight to the database so you should
//pick up db/event/added
//online status changes are also added to the database, but as contact settings
//so db/contact/settingchanged events are sent

/************ DEPRECATED IN 0.1.1.0 BY PROTOCOL PLUGIN CHANGES *************/

//triggered when the server acknowledges that a message or url or whatever has
//gone, sent loads of times during file transfers
//wParam=event id, returned when you initiated the send
//lParam=message type, see icq_notify_* in icq.h
//***DEPRECATED: Use ME_PROTO_ACK instead
#ifndef ICQ_NOTIFY_SUCCESS
#define ICQ_NOTIFY_SUCCESS        0
#define ICQ_NOTIFY_FAILED         1
#define ICQ_NOTIFY_SENT           4
#define ICQ_NOTIFY_ACK            5
#endif
#define ME_ICQ_EVENTSENT     "ICQ/EventSent"

//send a message
//wParam=0
//lParam=(LPARAM)(ICQSENDMESSAGE*)ism
//hook icq/eventsent to check that the message went
//returns an id that should correspond to wParam of icq/eventsent
//***DEPRECATED: Use PSS_MESSAGE instead
typedef struct {
	int cbSize;
	DWORD uin;
	char *pszMessage;
	int routeOverride;
} ICQSENDMESSAGE;
#define ISMF_ROUTE_DEFAULT    0
#define ISMF_ROUTE_DIRECT     1
#define ISMF_ROUTE_THRUSERVER 2
#define ISMF_ROUTE_BESTWAY    3   //v0.1.0.1+
#define MS_ICQ_SENDMESSAGE         "ICQ/SendMessage"

/*
Many services obsoleted and removed in 0.1.1.0 by protocol plugin changes. I
don't know of anyone who was using them, but if you were, tell me about it and
I'll put them back in
*/