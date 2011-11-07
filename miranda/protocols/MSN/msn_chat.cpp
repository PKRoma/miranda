/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2006-2011 Boris Krasnovskiy.
Copyright (c) 2003-2005 George Hazan.
Copyright (c) 2002-2003 Richard Hughes (original version).

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "msn_global.h"
#include "msn_proto.h"
#include <m_history.h>

HANDLE CMsnProto::MSN_GetChatInernalHandle(HANDLE hContact)
{
	HANDLE result = hContact;
	int type = getByte(hContact, "ChatRoom", 0);
	if (type != 0) 
	{
		DBVARIANT dbv;
		if (getString(hContact, "ChatRoomID", &dbv) == 0)
		{
			result = (HANDLE)(-atol(dbv.pszVal));
			MSN_FreeVariant(&dbv);
		}	
	}
	return result;
}


int CMsnProto::MSN_ChatInit(WPARAM wParam, LPARAM)
{
	ThreadData *info = (ThreadData*)wParam;
	MyInterlockedIncrement(&sttChatID);
	_ltot(sttChatID, info->mChatID, 10);

	TCHAR szName[512];
	mir_sntprintf(szName, SIZEOF(szName), _T("%s %s%s"), 
		m_tszUserName, TranslateT("Chat #"), info->mChatID);

	GCSESSION gcw = {0};
	gcw.cbSize = sizeof(GCSESSION);
	gcw.dwFlags = GC_TCHAR;
	gcw.iType = GCW_CHATROOM;
	gcw.pszModule = m_szModuleName;
	gcw.ptszName = szName;
	gcw.ptszID = info->mChatID;
	CallServiceSync(MS_GC_NEWSESSION, 0, (LPARAM)&gcw);

	GCDEST gcd = { m_szModuleName, { NULL }, GC_EVENT_ADDGROUP };
	gcd.ptszID = info->mChatID;
	GCEVENT gce = {0};
	gce.cbSize = sizeof(GCEVENT);
	gce.dwFlags = GC_TCHAR;
	gce.pDest = &gcd;
	gce.ptszStatus = TranslateT("Me");
	CallServiceSync(MS_GC_EVENT, 0, (LPARAM)&gce);

	DBVARIANT dbv;
	int bError = getTString("Nick", &dbv);
	gce.ptszNick = bError ? _T("") : dbv.ptszVal;

	gcd.iType = GC_EVENT_JOIN;
	gce.ptszUID = mir_a2t(MyOptions.szEmail);
	gce.time = 0;
	gce.bIsMe = TRUE;
	CallServiceSync(MS_GC_EVENT, 0, (LPARAM)&gce);

	gcd.iType = GC_EVENT_ADDGROUP;
	gce.ptszStatus = TranslateT("Others");
	CallServiceSync(MS_GC_EVENT, 0, (LPARAM)&gce);

	gcd.iType = GC_EVENT_CONTROL;
	CallServiceSync(MS_GC_EVENT, SESSION_INITDONE, (LPARAM)&gce);
	CallServiceSync(MS_GC_EVENT, SESSION_ONLINE, (LPARAM)&gce);
	CallServiceSync(MS_GC_EVENT, WINDOW_VISIBLE, (LPARAM)&gce);

	if (!bError)
		MSN_FreeVariant(&dbv);
	mir_free((TCHAR*)gce.ptszUID);
	return 0;
}

void CMsnProto::MSN_ChatStart(ThreadData* info)
{
	if (info->mChatID[0] != 0)
		return;

	MSN_StartStopTyping(info, false);

	NotifyEventHooks(hInitChat, (WPARAM)info, 0);

	// add all participants onto the list
	GCDEST gcd = { m_szModuleName, { NULL }, GC_EVENT_JOIN };
	gcd.ptszID = info->mChatID;

	GCEVENT gce = {0};
	gce.cbSize = sizeof(GCEVENT);
	gce.dwFlags = GC_TCHAR | GCEF_ADDTOLOG;
	gce.pDest = &gcd;
	gce.ptszStatus = TranslateT("Others");
	gce.time = time(NULL);
	gce.bIsMe = FALSE;

	for (int j=0; j < info->mJoinedContactsWLID.getCount(); j++) 
	{
		HANDLE hContact = MSN_HContactFromEmail(info->mJoinedContactsWLID[j]);
		TCHAR *wlid = mir_a2t(info->mJoinedContactsWLID[j]);

		gce.ptszNick = hContact ? MSN_GetContactNameT(hContact) : wlid;
		gce.ptszUID = wlid;
		CallServiceSync(MS_GC_EVENT, 0, (LPARAM)&gce);

		mir_free(wlid);
	}	
}

void CMsnProto::MSN_KillChatSession(TCHAR* id)
{
	GCDEST gcd = { m_szModuleName, { NULL }, GC_EVENT_CONTROL };
	gcd.ptszID = id;
	GCEVENT gce = {0};
	gce.cbSize = sizeof(GCEVENT);
	gce.dwFlags = GC_TCHAR;
	gce.pDest = &gcd;
	CallServiceSync(MS_GC_EVENT, SESSION_OFFLINE, (LPARAM)&gce);
	CallServiceSync(MS_GC_EVENT, SESSION_TERMINATE, (LPARAM)&gce);
}

static void ChatInviteUser(ThreadData* info, const char* email) 
{
	if (info->mJoinedContactsWLID.getCount())
	{
		for (int j=0; j < info->mJoinedContactsWLID.getCount(); j++) 
		{
			if (_stricmp(info->mJoinedContactsWLID[j], email) == 0) 
				return;
		}

		info->sendPacket("CAL", email);
		info->proto->MSN_ChatStart(info);
	}
}

static void ChatInviteSend(HANDLE hItem, HWND hwndList, STRLIST &str, CMsnProto *ppro)
{
	if (hItem == NULL)
		hItem = (HANDLE)SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_ROOT, 0);

	while (hItem) 
	{
		if (IsHContactGroup(hItem))
		{
			HANDLE hItemT = (HANDLE)SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_CHILD, (LPARAM)hItem);
			if (hItemT) ChatInviteSend(hItemT, hwndList, str, ppro);
		}
		else
		{
			int chk = SendMessage(hwndList, CLM_GETCHECKMARK, (WPARAM)hItem, 0);
			if (chk)
			{
				if (IsHContactInfo(hItem))
				{
					TCHAR buf[128] = _T("");
					SendMessage(hwndList, CLM_GETITEMTEXT, (WPARAM)hItem, (LPARAM)buf);

					if (buf[0]) str.insert(mir_t2a(buf));
				}
				else 
				{
					MsnContact *msc = ppro->Lists_Get(hItem);
					if (msc) str.insertn(msc->email);
				}
			}
		}
		hItem = (HANDLE)SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_NEXT, (LPARAM)hItem);
	}
}


static void ChatValidateContact(HANDLE hItem, HWND hwndList, CMsnProto* ppro)
{
	if (!ppro->MSN_IsMyContact(hItem) || ppro->getByte(hItem, "ChatRoom", 0) || ppro->MSN_IsMeByContact(hItem)) 
		SendMessage(hwndList, CLM_DELETEITEM, (WPARAM)hItem, 0);
}

static void ChatPrepare(HANDLE hItem, HWND hwndList, CMsnProto* ppro)
{
	if (hItem == NULL)
		hItem = (HANDLE)SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_ROOT, 0);

	while (hItem) 
	{
		HANDLE hItemN = (HANDLE)SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_NEXT, (LPARAM)hItem);

		if (IsHContactGroup(hItem))
		{
			HANDLE hItemT = (HANDLE)SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_CHILD, (LPARAM)hItem);
			if (hItemT) ChatPrepare(hItemT, hwndList, ppro);
		}
		else if (IsHContactContact(hItem))
			ChatValidateContact(hItem, hwndList, ppro);

		hItem = hItemN;
   }
}

INT_PTR CALLBACK DlgInviteToChat(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	InviteChatParam* param = (InviteChatParam*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

	switch (msg) 
	{
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);

		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
		param = (InviteChatParam*)lParam;

//		WindowSetIcon(hwndDlg, "msn");
		break;

	case WM_CLOSE:
		EndDialog(hwndDlg, 0);
		break;

	case WM_NCDESTROY:
//		WindowFreeIcon(hwndDlg);
		delete param;
		break;

	case WM_NOTIFY:
	{
		NMCLISTCONTROL* nmc = (NMCLISTCONTROL*)lParam;
		if (nmc->hdr.idFrom == IDC_CCLIST)
		{
			switch (nmc->hdr.code) 
			{
			case CLN_NEWCONTACT:
				if (param && (nmc->flags & (CLNF_ISGROUP | CLNF_ISINFO)) == 0) 
					ChatValidateContact(nmc->hItem, nmc->hdr.hwndFrom, param->ppro);
				break;

			case CLN_LISTREBUILT:
				if (param) 
					ChatPrepare(NULL, nmc->hdr.hwndFrom, param->ppro);
				break; 
			}
		}
	}
	break;

	case WM_COMMAND:
		{
			switch (LOWORD(wParam)) 
			{
			case IDC_ADDSCR:
				if (param->ppro->msnLoggedIn)
				{
					TCHAR email[MSN_MAX_EMAIL_LEN];
					GetDlgItemText(hwndDlg, IDC_EDITSCR, email, SIZEOF(email));

					CLCINFOITEM cii = {0};
					cii.cbSize = sizeof(cii);
					cii.flags = CLCIIF_CHECKBOX | CLCIIF_BELOWCONTACTS;
					cii.pszText = _tcslwr(email);

					HANDLE hItem = (HANDLE)SendDlgItemMessage(hwndDlg, IDC_CCLIST, CLM_ADDINFOITEM, 0, (LPARAM)&cii);
					SendDlgItemMessage(hwndDlg, IDC_CCLIST, CLM_SETCHECKMARK, (LPARAM)hItem, 1);
				}
				break;

			case IDOK:
				{
					char tEmail[MSN_MAX_EMAIL_LEN] = "";
					ThreadData *info = NULL;
					if (param->id)
						info = param->ppro->MSN_GetThreadByChatId(param->id);
					else if (param->hContact)
					{
						if (!param->ppro->MSN_IsMeByContact(param->hContact, tEmail))
							info =  param->ppro->MSN_GetThreadByContact(tEmail);
					}

					HWND hwndList = GetDlgItem(hwndDlg, IDC_CCLIST);
					STRLIST *cont = new STRLIST;
					ChatInviteSend(NULL, hwndList, *cont, param->ppro);

					if (info)
					{
						for (int i = 0; i < cont->getCount(); ++i)
							ChatInviteUser(info, (*cont)[i]);
						delete cont;
					}
					else 
					{
						if (tEmail[0]) cont->insertn(tEmail);
						param->ppro->MsgQueue_Add("chat", 'X', NULL, 0, NULL, 0, cont);
						if (param->ppro->msnLoggedIn)
							param->ppro->msnNsThread->sendPacket("XFR", "SB");
					}
				}

				EndDialog(hwndDlg, IDOK);
				break;

			case IDCANCEL:
				EndDialog(hwndDlg, IDCANCEL);
				break;
			}
		}
		break;
	}
	return FALSE;
}

int CMsnProto::MSN_GCEventHook(WPARAM, LPARAM lParam) 
{
	GCHOOK *gch = (GCHOOK*) lParam;
	if (!gch)
		return 1;

	if (_stricmp(gch->pDest->pszModule, m_szModuleName)) return 0;

	switch (gch->pDest->iType) 
	{
		case GC_SESSION_TERMINATE: 
		{
 			ThreadData* thread = MSN_GetThreadByChatId(gch->pDest->ptszID);
			if (thread != NULL) 
				thread->sendTerminate();
			break;
		}

		case GC_USER_MESSAGE:
			if (gch->ptszText && gch->ptszText[0]) 
			{
				ThreadData* thread = MSN_GetThreadByChatId(gch->pDest->ptszID);
				if (thread)
				{
					rtrim(gch->ptszText); // remove the ending linebreak
					TCHAR* pszMsg = UnEscapeChatTags(NEWTSTR_ALLOCA(gch->ptszText));
					thread->sendMessage('N', NULL, NETID_MSN, UTF8(pszMsg), 0);

					DBVARIANT dbv;
					int bError = getTString("Nick", &dbv);

					GCDEST gcd = { m_szModuleName, { NULL }, GC_EVENT_MESSAGE };
					gcd.ptszID = gch->pDest->ptszID;

					GCEVENT gce = {0};
					gce.cbSize = sizeof(GCEVENT);
					gce.dwFlags = GC_TCHAR | GCEF_ADDTOLOG;
					gce.pDest = &gcd;
					gce.ptszNick = bError ? _T("") : dbv.ptszVal;
					gce.ptszUID = mir_a2t(MyOptions.szEmail);
					gce.time = time(NULL);
					gce.ptszText = gch->ptszText;
					gce.bIsMe = TRUE;
					CallServiceSync(MS_GC_EVENT, 0, (LPARAM)&gce);

					mir_free((void*)gce.ptszUID);
					if (!bError)
						MSN_FreeVariant(&dbv);
				}
			}
			break;

		case GC_USER_CHANMGR: 
			DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_CHATROOM_INVITE), NULL, DlgInviteToChat, 
				LPARAM(new InviteChatParam(gch->pDest->ptszID, NULL, this)));
			break;

		case GC_USER_PRIVMESS: 
		{
			char *email = mir_t2a(gch->ptszUID);
			HANDLE hContact = MSN_HContactFromEmail(email);
			MSN_CallService(MS_MSG_SENDMESSAGE, (WPARAM)hContact, 0);
			mir_free(email);
			break;
		}

		case GC_USER_LOGMENU:
			switch(gch->dwData) 
			{
			case 10: 
				DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_CHATROOM_INVITE), NULL, DlgInviteToChat, 
					LPARAM(new InviteChatParam(gch->pDest->ptszID, NULL, this)));
				break;

			case 20:
				MSN_KillChatSession(gch->pDest->ptszID);
				break;
			}
			break;

		case GC_USER_NICKLISTMENU: 
		{
			char *email = mir_t2a(gch->ptszUID);
			HANDLE hContact = MSN_HContactFromEmail(email);
			mir_free(email);

			switch(gch->dwData) 
			{
			case 10:
				MSN_CallService(MS_USERINFO_SHOWDIALOG, (WPARAM)hContact, 0);
				break;

			case 20:
				MSN_CallService(MS_HISTORY_SHOWCONTACTHISTORY, (WPARAM)hContact, 0);
				break;

			case 110:
				MSN_KillChatSession(gch->pDest->ptszID);
				break;
			}
			break;
		}
/*	haven't implemented in chat.dll
		case GC_USER_TYPNOTIFY: 
		{
			int chatID = atoi(p);
			ThreadData* thread = MSN_GetThreadByContact((HANDLE)-chatID);
			for (int j=0; j < thread->mJoinedCount; j++) 
			{
				if ((long)thread->mJoinedContacts[j] > 0)
					CallService(MS_PROTO_SELFISTYPING, (WPARAM) thread->mJoinedContacts[j], (LPARAM) PROTOTYPE_SELFTYPING_ON);
			}
			break;
		}
*/
	}

	return 0;
}

int CMsnProto::MSN_GCMenuHook(WPARAM, LPARAM lParam) 
{
	GCMENUITEMS *gcmi= (GCMENUITEMS*) lParam;

	if (gcmi == NULL || _stricmp(gcmi->pszModule, m_szModuleName)) return 0;

	if (gcmi->Type == MENU_ON_LOG) 
	{
		static const struct gc_item Items[] = 
		{
			{ TranslateT("&Invite user..."), 10, MENU_ITEM, FALSE },
			{ TranslateT("&Leave chat session"), 20, MENU_ITEM, FALSE }
		};
		gcmi->nItems = SIZEOF(Items);
		gcmi->Item = (gc_item*)Items;
	}
	else if (gcmi->Type == MENU_ON_NICKLIST) 
	{
		char* email = mir_t2a(gcmi->pszUID);
		if (!_stricmp(MyOptions.szEmail, email)) 
		{
			static const struct gc_item Items[] = 
			{
				{ TranslateT("User &details"), 10, MENU_ITEM, FALSE },
				{ TranslateT("User &history"), 20, MENU_ITEM, FALSE },
				{ _T(""), 100, MENU_SEPARATOR, FALSE },
				{ TranslateT("&Leave chat session"), 110, MENU_ITEM, FALSE }
			};
			gcmi->nItems = SIZEOF(Items);
			gcmi->Item = (gc_item*)Items;
		}
		else 
		{
			static const struct gc_item Items[] = 
			{
				{ TranslateT("User &details"), 10, MENU_ITEM, FALSE },
				{ TranslateT("User &history"), 20, MENU_ITEM, FALSE }
			};
			gcmi->nItems = SIZEOF(Items);
			gcmi->Item = (gc_item*)Items;
		}
		mir_free(email);
	}

	return 0;
}
