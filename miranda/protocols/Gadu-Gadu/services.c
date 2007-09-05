////////////////////////////////////////////////////////////////////////////////
// Gadu-Gadu Plugin for Miranda IM
//
// Copyright (c) 2003-2006 Adam Strzelecki <ono+miranda@java.pl>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
////////////////////////////////////////////////////////////////////////////////


#include "gg.h"

pthread_mutex_t modeMsgsMutex;
pthread_mutex_t threadMutex;

uin_t nextUIN = 0;
unsigned long lastCRC = 0;

GGTHREAD *ggThread = NULL;
list_t ggThreadList = NULL;

//////////////////////////////////////////////////////////
// Status mode -> DB
char *gg_status2db(int status, const char *suffix)
{
	char *prefix;
	static char str[64];

	switch(status) {
		case ID_STATUS_AWAY:		prefix = "Away";	break;
		case ID_STATUS_NA:			prefix = "Na";	break;
		case ID_STATUS_DND:			prefix = "Dnd"; break;
		case ID_STATUS_OCCUPIED:	prefix = "Occupied"; break;
		case ID_STATUS_FREECHAT:	prefix = "FreeChat"; break;
		case ID_STATUS_ONLINE:		prefix = "On"; break;
		case ID_STATUS_OFFLINE:		prefix = "Off"; break;
		case ID_STATUS_INVISIBLE:	prefix = "Inv"; break;
		case ID_STATUS_ONTHEPHONE:	prefix = "Otp"; break;
		case ID_STATUS_OUTTOLUNCH:	prefix = "Otl"; break;
		default: return NULL;
	}
	strncpy(str, prefix, sizeof(str));
	strncat(str, suffix, sizeof(str) - strlen(str));
	return str;
}

//////////////////////////////////////////////////////////
// checks proto capabilities
static int gg_getcaps(WPARAM wParam, LPARAM lParam)
{
	int ret = 0;
	switch (wParam) {
		case PFLAGNUM_1:
			ret = PF1_IM | PF1_BASICSEARCH | PF1_EXTSEARCH | PF1_EXTSEARCHUI | PF1_SEARCHBYNAME |
				  PF1_SEARCHBYNAME | PF1_MODEMSG | PF1_NUMERICUSERID | PF1_VISLIST | PF1_FILE;
			return ret;
		case PFLAGNUM_2:
			return PF2_ONLINE | PF2_SHORTAWAY | PF2_INVISIBLE;
		case PFLAGNUM_3:
			return PF2_ONLINE | PF2_SHORTAWAY | PF2_INVISIBLE;
		case PFLAGNUM_4:
			return PF4_NOCUSTOMAUTH;
		case PFLAG_UNIQUEIDTEXT:
			return (int) Translate("Gadu-Gadu Number");
		case PFLAG_UNIQUEIDSETTING:
			return (int) GG_KEY_UIN;
			break;
	}
	return 0;
}

//////////////////////////////////////////////////////////
// gets protocol name
static int gg_getname(WPARAM wParam, LPARAM lParam)
{
	lstrcpyn((char *) lParam, GG_PROTONAME, wParam);
	return 0;
}

//////////////////////////////////////////////////////////
// loads protocol icon
static int gg_loadicon(WPARAM wParam, LPARAM lParam)
{
	if((wParam & 0xffff) == PLI_PROTOCOL)
		return (int) CopyIcon(LoadIconEx(IDI_GG));

	return (int) (HICON) NULL;
}

//////////////////////////////////////////////////////////
// gets protocol status
int gg_getstatus(WPARAM wParam, LPARAM lParam)
{
	return ggStatus;
}

//////////////////////////////////////////////////////////
// gets protocol status
GGINLINE char *gg_getstatusmsg(int status)
{
	switch(status)
	{
		case ID_STATUS_ONLINE:
		case ID_STATUS_FREECHAT:
			return ggModeMsg.szOnline;
			break;
		case ID_STATUS_INVISIBLE:
			return ggModeMsg.szInvisible;
			break;
		case ID_STATUS_AWAY:
		default:
			return ggModeMsg.szAway;
	}
}

//////////////////////////////////////////////////////////
// sets specified protocol status
int gg_refreshstatus(int status)
{
	// Create main loop thread and connect when not connecting
	pthread_mutex_lock(&threadMutex);
	if(status == ID_STATUS_OFFLINE)
		gg_disconnect(FALSE);
	else if (!ggThread)
	{
		if(ggThread = (GGTHREAD *) malloc(sizeof(GGTHREAD)))
		{
			list_add(&ggThreadList, ggThread, 0);
			ZeroMemory(ggThread, sizeof(GGTHREAD));
			// We control this thread ourselves (bypassing Miranda safe threads)
			ggThread->id.hThread = (HANDLE) _beginthreadex(NULL, 0, (unsigned (__stdcall *) (void *)) gg_mainthread,
												(void *)ggThread, 0, (unsigned *) &ggThread->id.dwThreadId);
			// pthread_create(&ggThread->id, NULL, gg_mainthread, ggThread);
		}
	}
	else if(gg_isonline())
	{
		// Select proper msg
		char *szMsg = gg_getstatusmsg(status);
		if(szMsg)
		{
#ifdef DEBUGMODE
			gg_netlog("gg_refreshstatus(): Setting status and away message \"%s\".", szMsg);
#endif
			gg_change_status_descr(ggThread->sess, status_m2gg(status, szMsg != NULL), szMsg);
		}
		else
		{
#ifdef DEBUGMODE
			gg_netlog("gg_refreshstatus(): Setting just status.");
#endif
			gg_change_status(ggThread->sess, status_m2gg(status, 0));
		}
		gg_broadcastnewstatus(status);
	}
	pthread_mutex_unlock(&threadMutex);

	return TRUE;
}

//////////////////////////////////////////////////////////
// normalize gg status
int gg_normalizestatus(int status)
{
	switch(status)
	{
		case ID_STATUS_ONLINE:
		case ID_STATUS_FREECHAT:
			return ID_STATUS_ONLINE;
		case ID_STATUS_OFFLINE:
			return ID_STATUS_OFFLINE;
		case ID_STATUS_INVISIBLE:
			return ID_STATUS_INVISIBLE;
		default:
			return ID_STATUS_AWAY;
	}
}

//////////////////////////////////////////////////////////
// sets protocol status
int gg_setstatus(WPARAM wParam, LPARAM lParam)
{
	ggDesiredStatus = gg_normalizestatus((int) wParam);

	// Depreciated due status description changing
	// if (ggStatus == status) return 0;
#ifdef DEBUGMODE
	gg_netlog("gg_setstatus(): PS_SETSTATUS(%s) normalized to %s",
		(char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, wParam, 0),
		(char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, ggDesiredStatus, 0));
#endif
	// Status wicked code due Miranda incompatibility with status+descr changing in one shot
	// Status request is offline / just request disconnect
	if(ggDesiredStatus == ID_STATUS_OFFLINE)
		// Go offline
		gg_refreshstatus(ggDesiredStatus);
	// Miranda will always ask for a new status message
#ifdef DEBUGMODE
	else
		gg_netlog("gg_setstatus(): Postponed to gg_setawaymsg().");
#endif
	return 0;
}

//////////////////////////////////////////////////////////
// when messsage received
int gg_recvmessage(WPARAM wParam, LPARAM lParam)
{
	return CallService(MS_PROTO_RECVMSG, wParam, lParam);
}

//////////////////////////////////////////////////////////
// when messsage sent
typedef struct
{
	HANDLE hContact;
	int seq;
} GG_SEQ_ACK;
static void *__stdcall gg_sendackthread(void *ack)
{
	SleepEx(100, FALSE);
	ProtoBroadcastAck(GG_PROTO, ((GG_SEQ_ACK *)ack)->hContact,
		ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, (HANDLE) ((GG_SEQ_ACK *)ack)->seq, 0);
	free(ack);
	return NULL;
}
int gg_sendmessage(WPARAM wParam, LPARAM lParam)
{
	CCSDATA *ccs = (CCSDATA *) lParam;
	pthread_t tid;
	uin_t uin;

	pthread_mutex_lock(&threadMutex);
	if(gg_isonline() && (uin = (uin_t)DBGetContactSettingDword(ccs->hContact, GG_PROTO, GG_KEY_UIN, 0)))
	{
		if(DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_MSGACK, GG_KEYDEF_MSGACK))
		{
			// Return normally
			HANDLE hRetVal = (HANDLE) gg_send_message(ggThread->sess, GG_CLASS_CHAT, uin, (char *) ccs->lParam);
			pthread_mutex_unlock(&threadMutex);
			return (int) hRetVal;
		}
		else
		{
			// Auto-ack message without waiting for server ack
			int seq = gg_send_message(ggThread->sess, GG_CLASS_CHAT, uin, (char *) ccs->lParam);
			GG_SEQ_ACK *ack = malloc(sizeof(GG_SEQ_ACK));
			if(ack)
			{
				pthread_create(&tid, NULL, gg_sendackthread, (void *) ack);
				pthread_detach(&tid);
			}
			pthread_mutex_unlock(&threadMutex);
			return seq;
		}
	}
	pthread_mutex_unlock(&threadMutex);
	return 0;
}

//////////////////////////////////////////////////////////
// when basic search
static void *__stdcall gg_searchthread(HANDLE hContact)
{
	SleepEx(100, FALSE);
#ifdef DEBUGMODE
	gg_netlog("gg_searchthread(): Failed search.");
#endif
	ProtoBroadcastAck(GG_PROTO, NULL, ACKTYPE_SEARCH, ACKRESULT_FAILED, (HANDLE) 1, 0);
	return NULL;
}
int gg_basicsearch(WPARAM wParam, LPARAM lParam)
{
	pthread_t tid;
	gg_pubdir50_t req;

	pthread_mutex_lock(&threadMutex);
	if(!gg_isonline())
	{
		pthread_mutex_unlock(&threadMutex);
		return 0;
	}

	if (!(req = gg_pubdir50_new(GG_PUBDIR50_SEARCH)))
	{ pthread_create(&tid, NULL, gg_searchthread, NULL); pthread_detach(&tid); return 1; }

	// Add uin and search it
	gg_pubdir50_add(req, GG_PUBDIR50_UIN, (char *) lParam);
	gg_pubdir50_seq_set(req, GG_SEQ_SEARCH);

	if(!gg_pubdir50(ggThread->sess, req))
	{ pthread_create(&tid, NULL, gg_searchthread, NULL); pthread_detach(&tid); return 1; }
#ifdef DEBUGMODE
	gg_netlog("gg_basicsearch(): Seq %d.", req->seq);
#endif
	gg_pubdir50_free(req);

	pthread_mutex_unlock(&threadMutex);
	return 1;
}
static int gg_searchbydetails(WPARAM wParam, LPARAM lParam)
{
	PROTOSEARCHBYNAME *psbn = (PROTOSEARCHBYNAME *) lParam;
	pthread_t tid;
	gg_pubdir50_t req;
	unsigned long crc;
	char data[512] = "\0";

	// Check if connected and if there's a search data
	pthread_mutex_lock(&threadMutex);
	if(!gg_isonline())
	{
		pthread_mutex_unlock(&threadMutex);
		return 0;
	}
	if(!psbn->pszNick && !psbn->pszFirstName && !psbn->pszLastName)
		return 0;

	if (!(req = gg_pubdir50_new(GG_PUBDIR50_SEARCH)))
	{ pthread_create(&tid, NULL, gg_searchthread, NULL); pthread_detach(&tid); return 1; }

	// Add uin and search it
	if(psbn->pszNick)
	{
		gg_pubdir50_add(req, GG_PUBDIR50_NICKNAME, psbn->pszNick);
		strncat(data, psbn->pszNick, sizeof(data) - strlen(data));
	}
	strncat(data, ".", sizeof(data) - strlen(data));

	if(psbn->pszFirstName)
	{
		gg_pubdir50_add(req, GG_PUBDIR50_FIRSTNAME, psbn->pszFirstName);
		strncat(data, psbn->pszFirstName, sizeof(data) - strlen(data));
	}
	strncat(data, ".", sizeof(data) - strlen(data));

	if(psbn->pszLastName)
	{
		gg_pubdir50_add(req, GG_PUBDIR50_LASTNAME, psbn->pszLastName);
		strncat(data, psbn->pszLastName, sizeof(data) - strlen(data));
	}
	strncat(data, ".", sizeof(data) - strlen(data));

	// Count crc & check if the data was equal if yes do same search with shift
	crc = crc_get(data);

	if(crc == lastCRC && nextUIN)
		gg_pubdir50_add(req, GG_PUBDIR50_START, ditoa(nextUIN));
	else
		lastCRC = crc;

	gg_pubdir50_seq_set(req, GG_SEQ_SEARCH);

	if(!gg_pubdir50(ggThread->sess, req))
	{ pthread_create(&tid, NULL, gg_searchthread, NULL); pthread_detach(&tid); return 1; }
#ifdef DEBUGMODE
	gg_netlog("gg_searchbyname(): Seq %d.", req->seq);
#endif
	gg_pubdir50_free(req);

	pthread_mutex_unlock(&threadMutex);
	return 1;
}

//////////////////////////////////////////////////////////
// when contact is added to list
int gg_addtolist(WPARAM wParam, LPARAM lParam)
{
	GGSEARCHRESULT *sr = (GGSEARCHRESULT *) lParam;
	return (int) gg_getcontact(sr->uin, 1, wParam & PALF_TEMPORARY ? 0 : 1, sr->hdr.nick);
}

//////////////////////////////////////////////////////////
// THREAD: info thread
static void *__stdcall gg_cmdgetinfothread(HANDLE hContact)
{
	SleepEx(100, FALSE);
#ifdef DEBUGMODE
	gg_netlog("gg_cmdgetinfothread(): Failed info retreival.");
#endif
	ProtoBroadcastAck(GG_PROTO, hContact, ACKTYPE_GETINFO, ACKRESULT_FAILED, (HANDLE) 1, 0);
	return NULL;
}
int gg_getinfo(WPARAM wParam, LPARAM lParam)
{
	CCSDATA *ccs = (CCSDATA *) lParam;
	pthread_t tid;
	gg_pubdir50_t req;

	// Custom contact info
	if(ccs->hContact)
	{
		if (!(req = gg_pubdir50_new(GG_PUBDIR50_SEARCH)))
		{
			pthread_create(&tid, NULL, gg_cmdgetinfothread, ccs->hContact);
			pthread_detach(&tid);
			return 1;
		}

		// Add uin and search it
		gg_pubdir50_add(req, GG_PUBDIR50_UIN, ditoa((uin_t)DBGetContactSettingDword(ccs->hContact, GG_PROTO, GG_KEY_UIN, 0)));
		gg_pubdir50_seq_set(req, GG_SEQ_INFO);

#ifdef DEBUGMODE
		gg_netlog("gg_getinfo(): Requesting user info.", req->seq);
#endif
		pthread_mutex_lock(&threadMutex);
		if(gg_isonline() && !gg_pubdir50(ggThread->sess, req))
		{
			pthread_create(&tid, NULL, gg_cmdgetinfothread, ccs->hContact);
			pthread_detach(&tid);
			pthread_mutex_unlock(&threadMutex);
			return 1;
		}
		pthread_mutex_unlock(&threadMutex);
	}
	// Own contact info
	else
	{
		if (!(req = gg_pubdir50_new(GG_PUBDIR50_READ)))
		{
			pthread_create(&tid, NULL, gg_cmdgetinfothread, ccs->hContact);
			pthread_detach(&tid);
			return 1;
		}

		// Add seq
		gg_pubdir50_seq_set(req, GG_SEQ_CHINFO);

#ifdef DEBUGMODE
		gg_netlog("gg_getinfo(): Requesting owner info.", req->seq);
#endif
		pthread_mutex_lock(&threadMutex);
		if(gg_isonline() && !gg_pubdir50(ggThread->sess, req))
		{
			pthread_create(&tid, NULL, gg_cmdgetinfothread, ccs->hContact);
			pthread_detach(&tid);
			pthread_mutex_unlock(&threadMutex);
			return 1;
		}
		pthread_mutex_unlock(&threadMutex);
	}
#ifdef DEBUGMODE
	gg_netlog("gg_getinfo(): Seq %d.", req->seq);
#endif
	gg_pubdir50_free(req);

	return 1;
}

//////////////////////////////////////////////////////////
// when away message is requested
static void *__stdcall gg_getawaymsgthread(HANDLE hContact)
{
	DBVARIANT dbv;

	SleepEx(100, FALSE);
	if (!DBGetContactSettingString(hContact, "CList", GG_KEY_STATUSDESCR, &dbv))
	{
		ProtoBroadcastAck(GG_PROTO, hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, (HANDLE) 1, (LPARAM) dbv.pszVal);

#ifdef DEBUGMODE
		gg_netlog("gg_getawaymsg(): Reading away msg \"%s\".", dbv.pszVal);
#endif
		DBFreeVariant(&dbv);
	}
	else
		ProtoBroadcastAck(GG_PROTO, hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, (HANDLE) 1, (LPARAM) NULL);
	return NULL;
}
int gg_getawaymsg(WPARAM wParam, LPARAM lParam)
{
	CCSDATA *ccs = (CCSDATA *) lParam;
	pthread_t tid;

	pthread_create(&tid, NULL, gg_getawaymsgthread, ccs->hContact);
	pthread_detach(&tid);

	return 1;
}

//////////////////////////////////////////////////////////
// when away message is beging set
int gg_setawaymsg(WPARAM wParam, LPARAM lParam)
{
	int status = gg_normalizestatus((int) wParam);
	char **szMsg;

#ifdef DEBUGMODE
	gg_netlog("gg_setawaymsg(): Requesting away message set to \"%s\".", (char *) lParam);
#endif
	pthread_mutex_lock(&modeMsgsMutex);

	// Select proper msg
	switch(status)
	{
		case ID_STATUS_ONLINE:
			szMsg = &ggModeMsg.szOnline;
			break;
		case ID_STATUS_AWAY:
			szMsg = &ggModeMsg.szAway;
			break;
		case ID_STATUS_INVISIBLE:
			szMsg = &ggModeMsg.szInvisible;
			break;
		default:
			pthread_mutex_unlock(&modeMsgsMutex);
			return 1;
	}

	// Check if we change status here somehow
	if(*szMsg && (char *) lParam && !strcmp(*szMsg, (char *) lParam)
		|| !*szMsg && (!(char *)lParam || !*((char *) lParam)))
	{
		if(status == ggDesiredStatus && ggDesiredStatus == ggStatus)
		{
#ifdef DEBUGMODE
			gg_netlog("gg_setawaymsg(): Message hasn't been changed, return.");
#endif
			pthread_mutex_unlock(&modeMsgsMutex);
			return 0;
		}
	}
	else
	{
		if(*szMsg)
			free(*szMsg);
		*szMsg = (char *) lParam && *((char *) lParam) ? _strdup((char *) lParam) : NULL;
#ifdef DEBUGMODE
		gg_netlog("gg_setawaymsg(): Message changed.");
#endif
	}

	// Change the status only if it was desired by PS_SETSTATUS
	if(status == ggDesiredStatus)
		gg_refreshstatus(status);
#ifdef DEBUGMODE
	else
	{
		char error[128];
		mir_snprintf(error, sizeof(error), Translate("GG: PS_AWAYMSG was called without previous PS_SETSTATUS for status %d, desired %d, current %d."),
			status, ggDesiredStatus, ggStatus);
		PUShowMessage(error, SM_WARNING);
	}
#endif
	pthread_mutex_unlock(&modeMsgsMutex);

	return 0;
}

//////////////////////////////////////////////////////////
// visible lists
static int gg_setapparentmode(WPARAM wParam, LPARAM lParam)
{
	CCSDATA *ccs=(CCSDATA*)lParam;
	DBWriteContactSettingWord(ccs->hContact, GG_PROTO, GG_KEY_APPARENT, (WORD)ccs->wParam);
	if(gg_isonline()) gg_notifyuser(ccs->hContact, 1);
	return 0;
}

//////////////////////////////////////////////////////////
// create adv search dialog proc
BOOL CALLBACK gg_advancedsearchdlgproc(HWND hwndDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
	switch(message)
	{
		case WM_INITDIALOG:
			TranslateDialogDefault(hwndDlg);
			SendDlgItemMessage(hwndDlg, IDC_GENDER, CB_ADDSTRING, 0, (LPARAM)Translate(""));		// 0
			SendDlgItemMessage(hwndDlg, IDC_GENDER, CB_ADDSTRING, 0, (LPARAM)Translate("Male"));	// 1
			SendDlgItemMessage(hwndDlg, IDC_GENDER, CB_ADDSTRING, 0, (LPARAM)Translate("Female"));	// 2
			return TRUE;
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDOK:
						SendMessage(GetParent(hwndDlg), WM_COMMAND,MAKEWPARAM(IDOK,BN_CLICKED), (LPARAM)GetDlgItem(GetParent(hwndDlg),IDOK));
						break;
				case IDCANCEL:
//						  CheckDlgButton(GetParent(hwndDlg),IDC_ADVANCED,BST_UNCHECKED);
//						  SendMessage(GetParent(hwndDlg),WM_COMMAND,MAKEWPARAM(IDC_ADVANCED,BN_CLICKED),(LPARAM)GetDlgItem(GetParent(hwndDlg),IDC_ADVANCED));
						break;
			}
			break;
	}
	return FALSE;
}

//////////////////////////////////////////////////////////
// create adv search dialog
int gg_createadvsearchui(WPARAM wParam, LPARAM lParam)
{
	return (int)CreateDialog(hInstance,
		MAKEINTRESOURCE(IDD_GGADVANCEDSEARCH), (HWND)lParam, gg_advancedsearchdlgproc);
}

//////////////////////////////////////////////////////////
// search by advanced
int gg_searchbyadvanced(WPARAM wParam, LPARAM lParam)
{
	pthread_t tid;
	gg_pubdir50_t req;
	HWND hwndDlg = (HWND)lParam;
	char text[64], data[512] = "\0";
	unsigned long crc;

	// Check if connected
	if(!gg_isonline()) return 0;

	if (!(req = gg_pubdir50_new(GG_PUBDIR50_SEARCH)))
	{ pthread_create(&tid, NULL, gg_searchthread, NULL); pthread_detach(&tid); return 1; }

	// Fetch search data
	GetDlgItemText(hwndDlg, IDC_FIRSTNAME, text, sizeof(text));
	if(strlen(text))
	{
		gg_pubdir50_add(req, GG_PUBDIR50_FIRSTNAME, text);
		strncat(data, text, sizeof(data) - strlen(data));
	}
	/* 1 */ strncat(data, ".", sizeof(data) - strlen(data));

	GetDlgItemText(hwndDlg, IDC_LASTNAME, text, sizeof(text));
	if(strlen(text))
	{
		gg_pubdir50_add(req, GG_PUBDIR50_LASTNAME, text);
		strncat(data, text, sizeof(data) - strlen(data));
	}
	/* 2 */ strncat(data, ".", sizeof(data) - strlen(data));

	GetDlgItemText(hwndDlg, IDC_NICKNAME, text, sizeof(text));
	if(strlen(text))
	{
		gg_pubdir50_add(req, GG_PUBDIR50_NICKNAME, text);
		strncat(data, text, sizeof(data) - strlen(data));
	}
	/* 3 */ strncat(data, ".", sizeof(data) - strlen(data));

	GetDlgItemText(hwndDlg, IDC_CITY, text, sizeof(text));
	if(strlen(text))
	{
		gg_pubdir50_add(req, GG_PUBDIR50_CITY, text);
		strncat(data, text, sizeof(data) - strlen(data));
	}
	/* 4 */ strncat(data, ".", sizeof(data) - strlen(data));

	GetDlgItemText(hwndDlg, IDC_AGEFROM, text, sizeof(text));
	if(strlen(text))
	{
		int yearTo = atoi(text);
		int yearFrom;
		time_t t = time(NULL);
		struct tm *lt = localtime(&t);
		int ay = lt->tm_year + 1900;
		char age[16];

		GetDlgItemText(hwndDlg, IDC_AGETO, age, sizeof(age));
		yearFrom = atoi(age);

		// Count & fix ranges
		if(!yearTo)
			yearTo = ay;
		else
			yearTo = ay - yearTo;
		if(!yearFrom)
			yearFrom = 0;
		else
			yearFrom = ay - yearFrom;
		mir_snprintf(text, sizeof(text), "%d %d", yearFrom, yearTo);

		gg_pubdir50_add(req, GG_PUBDIR50_BIRTHYEAR, text);
		strncat(data, text, sizeof(data) - strlen(data));
	}
	/* 5 */ strncat(data, ".", sizeof(data) - strlen(data));

	switch(SendDlgItemMessage(hwndDlg, IDC_GENDER, CB_GETCURSEL, 0, 0))
	{
		case 1:
			gg_pubdir50_add(req, GG_PUBDIR50_GENDER, GG_PUBDIR50_GENDER_MALE);
			strncat(data, GG_PUBDIR50_GENDER_FEMALE, sizeof(data) - strlen(data));
			break;
		case 2:
			gg_pubdir50_add(req, GG_PUBDIR50_GENDER, GG_PUBDIR50_GENDER_FEMALE);
			strncat(data, GG_PUBDIR50_GENDER_MALE, sizeof(data) - strlen(data));
			break;
	}
	/* 6 */ strncat(data, ".", sizeof(data) - strlen(data));

	if(IsDlgButtonChecked(hwndDlg, IDC_ONLYCONNECTED))
	{
		gg_pubdir50_add(req, GG_PUBDIR50_ACTIVE, GG_PUBDIR50_ACTIVE_TRUE);
		strncat(data, GG_PUBDIR50_ACTIVE_TRUE, sizeof(data) - strlen(data));
	}
	/* 7 */ strncat(data, ".", sizeof(data) - strlen(data));

	// No data entered
	if(strlen(data) <= 7 || (strlen(data) == 8 && IsDlgButtonChecked(hwndDlg, IDC_ONLYCONNECTED))) return 0;

	// Count crc & check if the data was equal if yes do same search with shift
	crc = crc_get(data);

	if(crc == lastCRC && nextUIN)
		gg_pubdir50_add(req, GG_PUBDIR50_START, ditoa(nextUIN));
	else
		lastCRC = crc;

	gg_pubdir50_seq_set(req, GG_SEQ_SEARCH);

	pthread_mutex_lock(&threadMutex);
	if(gg_isonline() && !gg_pubdir50(ggThread->sess, req))
	{
		pthread_create(&tid, NULL, gg_searchthread, NULL);
		pthread_detach(&tid);
		pthread_mutex_unlock(&threadMutex);
		return 1;
	}
	pthread_mutex_unlock(&threadMutex);
#ifdef DEBUGMODE
	gg_netlog("gg_searchbyadvanced(): Seq %d.", req->seq);
#endif
	gg_pubdir50_free(req);

	return 1;
}

//////////////////////////////////////////////////////////
// Register services
void gg_registerservices()
{
	// Bind table
	char service[128];

	mir_snprintf(service, sizeof(service), "%s%s", GG_PROTO, PS_GETCAPS);
	CreateServiceFunction(service, gg_getcaps);

	mir_snprintf(service, sizeof(service), "%s%s", GG_PROTO, PS_GETNAME);
	CreateServiceFunction(service, gg_getname);

	mir_snprintf(service, sizeof(service), "%s%s", GG_PROTO, PS_LOADICON);
	CreateServiceFunction(service, gg_loadicon);

	mir_snprintf(service, sizeof(service), "%s%s", GG_PROTO, PS_GETSTATUS);
	CreateServiceFunction(service, gg_getstatus);

	mir_snprintf(service, sizeof(service), "%s%s", GG_PROTO, PS_SETSTATUS);
	CreateServiceFunction(service, gg_setstatus);

	mir_snprintf(service, sizeof(service), "%s%s", GG_PROTO, PSR_MESSAGE);
	CreateServiceFunction(service, gg_recvmessage);

	mir_snprintf(service, sizeof(service), "%s%s", GG_PROTO, PSS_MESSAGE);
	CreateServiceFunction(service, gg_sendmessage);

	mir_snprintf(service, sizeof(service), "%s%s", GG_PROTO, PS_BASICSEARCH);
	CreateServiceFunction(service, gg_basicsearch);

	mir_snprintf(service, sizeof(service), "%s%s", GG_PROTO, PS_SEARCHBYNAME);
	CreateServiceFunction(service, gg_searchbydetails);

	mir_snprintf(service, sizeof(service), "%s%s", GG_PROTO, PS_ADDTOLIST);
	CreateServiceFunction(service, gg_addtolist);

	mir_snprintf(service, sizeof(service), "%s%s", GG_PROTO, PSS_GETINFO);
	CreateServiceFunction(service, gg_getinfo);

	mir_snprintf(service, sizeof(service), "%s%s", GG_PROTO, PS_SETAWAYMSG);
	CreateServiceFunction(service, gg_setawaymsg);

	mir_snprintf(service, sizeof(service), "%s%s", GG_PROTO, PSS_GETAWAYMSG);
	CreateServiceFunction(service, gg_getawaymsg);

	mir_snprintf(service, sizeof(service), "%s%s", GG_PROTO, PSS_SETAPPARENTMODE);
	CreateServiceFunction(service, gg_setapparentmode);

	mir_snprintf(service, sizeof(service), "%s%s", GG_PROTO, PS_CREATEADVSEARCHUI);
	CreateServiceFunction(service, gg_createadvsearchui);

	mir_snprintf(service, sizeof(service), "%s%s", GG_PROTO, PS_SEARCHBYADVANCED);
	CreateServiceFunction(service, gg_searchbyadvanced);

	mir_snprintf(service, sizeof(service), "%s%s", GG_PROTO, PSS_FILEALLOW);
	CreateServiceFunction(service, gg_fileallow);

	mir_snprintf(service, sizeof(service), "%s%s", GG_PROTO, PSS_FILEDENY);
	CreateServiceFunction(service, gg_filedeny);

	mir_snprintf(service, sizeof(service), "%s%s", GG_PROTO, PSS_FILECANCEL);
	CreateServiceFunction(service, gg_filecancel);

	mir_snprintf(service, sizeof(service), "%s%s", GG_PROTO, PSS_FILE);
	CreateServiceFunction(service, gg_sendfile);

	mir_snprintf(service, sizeof(service), "%s%s", GG_PROTO, PSR_FILE);
	CreateServiceFunction(service, gg_recvfile);

	mir_snprintf(service, sizeof(service), GGS_RECVIMAGE, GG_PROTO);
	CreateServiceFunction(service, gg_img_recvimage);
}
