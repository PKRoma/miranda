/*
Scriver

Copyright 2000-2007 Miranda ICQ/IM project,

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
#include "commonheaders.h"

extern HINSTANCE g_hInst;

static MessageSendQueueItem *global_sendQueue = NULL;

static char *MsgServiceName(HANDLE hContact)
{
#ifdef _UNICODE
    char szServiceName[100];
    char *szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
    if (szProto == NULL)
        return PSS_MESSAGE;

    mir_snprintf(szServiceName, sizeof(szServiceName), "%s%sW", szProto, PSS_MESSAGE);
    if (ServiceExists(szServiceName))
        return PSS_MESSAGE "W";
#endif
    return PSS_MESSAGE;
}

TCHAR * GetSendBufferMsg(MessageSendQueueItem *item) {
    TCHAR *szMsg = NULL;
    int len = strlen(item->sendBuffer);
#if defined( _UNICODE )
    if (item->flags & PREF_UTF) {
        mir_utf8decode(item->sendBuffer, &szMsg);
    } else {
        szMsg = (TCHAR *)mir_alloc(item->sendBufferSize - len - 1);
        memcpy(szMsg, item->sendBuffer + len + 1, item->sendBufferSize - len - 1);
    }
#else
    szMsg = (char *)mir_alloc(item->sendBufferSize);
    memcpy(szMsg, item->sendBuffer, len + 1);
#endif
    return szMsg;
}

MessageSendQueueItem* CreateSendQueueItem(HWND hwndSender) {
	/*TODO: Make it synchronized */
	MessageSendQueueItem *item = (MessageSendQueueItem *) mir_alloc(sizeof(MessageSendQueueItem));
	ZeroMemory(item, sizeof(MessageSendQueueItem));
	item->hwndSender = hwndSender;
	item->next = global_sendQueue;
	global_sendQueue = item;
	return item;
}

MessageSendQueueItem* FindOldestSendQueueItem(HWND hwndSender) {
	/*TODO: Make it synchronized */
	MessageSendQueueItem *item, *found = NULL;
	for (item = global_sendQueue; item != NULL; item = item->next) {
		if (item->hwndSender == hwndSender) {
			found = item;
		}
	}
	return found;
}

MessageSendQueueItem* FindSendQueueItem(HANDLE hSendId) {
	/*TODO: Make it synchronized */
	MessageSendQueueItem *item;
	for (item = global_sendQueue; item != NULL; item = item->next) {
		if (item->hSendId == hSendId) {
			return item;
		}
	}
	return NULL;
}

BOOL RemoveSendQueueItem(MessageSendQueueItem* item) {
	/*TODO: Make it synchronized */
	HWND hwndSender = item->hwndSender;
	if (item->prev != NULL) {
		item->prev->next = item->next;
	} else {
		global_sendQueue = item->next;
	}
	if (item->next != NULL) {
		item->next->prev = item->prev;
	}
	if (item->sendBuffer) {
 		mir_free(item->sendBuffer);
	}
	if (item->proto) {
 		mir_free(item->proto);
	}
	for (item = global_sendQueue; item != NULL; item = item->next) {
		if (item->hwndSender == hwndSender) return FALSE;
	}
	return TRUE;
}

void ReportSendQueueTimeouts(HWND hwnd) {
	/*TODO: Make it synchronized */
	MessageSendQueueItem *item;
	int timeout = DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_MSGTIMEOUT, SRMSGDEFSET_MSGTIMEOUT);
	for (item = global_sendQueue; item != NULL; item = item->next) {
		if (item->hwndErrorDlg == NULL && item->timeout < timeout && item->hwndSender == hwnd) {
			item->timeout += 1000;
			if (item->timeout >= timeout) {
				ErrorWindowData *ewd = (ErrorWindowData *) mir_alloc(sizeof(ErrorWindowData));
				ewd->szName = GetNickname(item->hContact, item->proto);
				ewd->szDescription = mir_tstrdup(TranslateT("The message send timed out."));
				ewd->szText = GetSendBufferMsg(item);
				ewd->hwndParent = hwnd;
				ewd->queueItem = item;
				if (hwnd != NULL) {
					SendMessage(hwnd, DM_STOPMESSAGESENDING, 0, 0);
				}
				item->hwndErrorDlg = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_MSGSENDERROR), hwnd, ErrorDlgProc, (LPARAM) ewd);
			}
		}
	}
}

void ReleaseSendQueueItems(HWND hwndSender) {
	/*TODO: Make it synchronized */
	MessageSendQueueItem *item;
	for (item = global_sendQueue; item != NULL; item = item->next) {
		if (item->hwndSender == hwndSender) {
			item->hwndSender = NULL;
		}
	}
}

void RemoveAllSendQueueItems() {
	/*TODO: Make it synchronized */
	MessageSendQueueItem *item, *item2;
	for (item = global_sendQueue; item != NULL; item = item2) {
		item2 = item->next;
		RemoveSendQueueItem(item);
	}
}

void SendSendQueueItem(MessageSendQueueItem* item) {
	item->timeout = 0;
	item->hSendId = (HANDLE) CallContactService(item->hContact, MsgServiceName(item->hContact), item->flags, (LPARAM) item->sendBuffer);
}


