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
DWORD gg_getcaps(PROTO_INTERFACE *proto, int type, HANDLE hContact)
{
	int ret = 0;
	switch (type) {
		case PFLAGNUM_1:
			return PF1_IM | PF1_BASICSEARCH | PF1_EXTSEARCH | PF1_EXTSEARCHUI | PF1_SEARCHBYNAME |
			       PF1_SEARCHBYNAME | PF1_MODEMSG | PF1_NUMERICUSERID | PF1_VISLIST | PF1_FILE;
		case PFLAGNUM_2:
			return PF2_ONLINE | PF2_SHORTAWAY | PF2_INVISIBLE;
		case PFLAGNUM_3:
			return PF2_ONLINE | PF2_SHORTAWAY | PF2_INVISIBLE;
		case PFLAGNUM_4:
			return PF4_NOCUSTOMAUTH;
		case PFLAG_UNIQUEIDTEXT:
			return (DWORD) Translate("Gadu-Gadu Number");
		case PFLAG_UNIQUEIDSETTING:
			return (DWORD) GG_KEY_UIN;
			break;
	}
	return 0;
}

//////////////////////////////////////////////////////////
// gets protocol name
static int gg_getname(GGPROTO *gg, WPARAM wParam, LPARAM lParam)
{
	lstrcpyn((char *) lParam, GG_PROTONAME, wParam);
	return 0;
}

//////////////////////////////////////////////////////////
// loads protocol icon
HICON gg_loadicon(PROTO_INTERFACE *proto, int iconIndex)
{
	if((iconIndex & 0xffff) == PLI_PROTOCOL)
		return CopyIcon(LoadIconEx(IDI_GG));

	return (HICON) NULL;
}

//////////////////////////////////////////////////////////
// gets protocol status
int gg_getstatus(GGPROTO *gg, WPARAM wParam, LPARAM lParam)
{
	return gg->proto.m_iStatus;
}

//////////////////////////////////////////////////////////
// gets protocol status
GGINLINE char *gg_getstatusmsg(GGPROTO *gg, int status)
{
	switch(status)
	{
		case ID_STATUS_ONLINE:
		case ID_STATUS_FREECHAT:
			return gg->modemsg.online;
			break;
		case ID_STATUS_INVISIBLE:
			return gg->modemsg.invisible;
			break;
		case ID_STATUS_AWAY:
		default:
			return gg->modemsg.away;
	}
}

//////////////////////////////////////////////////////////
// sets specified protocol status
int gg_refreshstatus(GGPROTO *gg, int status)
{
	if(status == ID_STATUS_OFFLINE)
	{
		gg_disconnect(gg);
		return TRUE;
	}
	pthread_mutex_lock(&gg->sess_mutex);
	if(!gg_isonline(gg))
	{
		if(!gg->pth_sess.dwThreadId)
			pthread_create(&gg->pth_sess, NULL, gg_mainthread, gg);
	}
	else
	{
		// Select proper msg
		char *szMsg = gg_getstatusmsg(gg, status);
		if(szMsg)
		{
#ifdef DEBUGMODE
			gg_netlog(gg, "gg_refreshstatus(): Setting status and away message \"%s\".", szMsg);
#endif
			gg_change_status_descr(gg->sess, status_m2gg(gg, status, szMsg != NULL), szMsg);
		}
		else
		{
#ifdef DEBUGMODE
			gg_netlog(gg, "gg_refreshstatus(): Setting just status.");
#endif
			gg_change_status(gg->sess, status_m2gg(gg, status, 0));
		}
		gg_broadcastnewstatus(gg, status);
	}
	pthread_mutex_unlock(&gg->sess_mutex);

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
int gg_setstatus(PROTO_INTERFACE *proto, int iNewStatus)
{
	proto->m_iDesiredStatus = gg_normalizestatus(iNewStatus);

	// Depreciated due status description changing
	// if (gg->proto.m_iStatus == status) return 0;
#ifdef DEBUGMODE
	gg_netlog((GGPROTO *)proto, "gg_setstatus(): PS_SETSTATUS(%s) normalized to %s",
		(char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, iNewStatus, 0),
		(char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, proto->m_iDesiredStatus, 0));
#endif
	// Status wicked code due Miranda incompatibility with status+descr changing in one shot
	// Status request is offline / just request disconnect
	if(proto->m_iDesiredStatus == ID_STATUS_OFFLINE)
		// Go offline
		gg_refreshstatus((GGPROTO *)proto, proto->m_iDesiredStatus);
	// Miranda will always ask for a new status message
#ifdef DEBUGMODE
	else
		gg_netlog((GGPROTO *)proto, "gg_setstatus(): Postponed to gg_setawaymsg().");
#endif
	return 0;
}

//////////////////////////////////////////////////////////
// when messsage received
int gg_recvmessage(PROTO_INTERFACE *proto, HANDLE hContact, PROTORECVEVENT *pre)
{
	CCSDATA ccs = { hContact, PSR_MESSAGE, 0, ( LPARAM )pre };
	return CallService(MS_PROTO_RECVMSG, 0, ( LPARAM )&ccs);
}

//////////////////////////////////////////////////////////
// when messsage sent
typedef struct
{
	HANDLE hContact;
	int seq;
	GGPROTO *gg;
} GG_SEQ_ACK;
static void *__stdcall gg_sendackthread(void *ack)
{
	SleepEx(100, FALSE);
	ProtoBroadcastAck(((GG_SEQ_ACK *)ack)->gg->proto.m_szModuleName, ((GG_SEQ_ACK *)ack)->hContact,
		ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, (HANDLE) ((GG_SEQ_ACK *)ack)->seq, 0);
	free(ack);
	return NULL;
}
int gg_sendmessage(PROTO_INTERFACE *proto, HANDLE hContact, int flags, const char *msg)
{
	GGPROTO *gg = (GGPROTO *)proto;
	pthread_t tid;
	uin_t uin;

	pthread_mutex_lock(&gg->sess_mutex);
	if(gg_isonline(gg) && (uin = (uin_t)DBGetContactSettingDword(hContact, GG_PROTO, GG_KEY_UIN, 0)))
	{
		if(DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_MSGACK, GG_KEYDEF_MSGACK))
		{
			// Return normally
			HANDLE hRetVal = (HANDLE) gg_send_message(gg->sess, GG_CLASS_CHAT, uin, msg);
			pthread_mutex_unlock(&gg->sess_mutex);
			return (int) hRetVal;
		}
		else
		{
			// Auto-ack message without waiting for server ack
			int seq = gg_send_message(gg->sess, GG_CLASS_CHAT, uin, msg);
			GG_SEQ_ACK *ack = malloc(sizeof(GG_SEQ_ACK));
			if(ack)
			{
				ack->gg = gg;
				ack->seq = seq;
				ack->hContact = hContact;
				pthread_create(&tid, NULL, gg_sendackthread, (void *) ack);
				pthread_detach(&tid);
			}
			pthread_mutex_unlock(&gg->sess_mutex);
			return seq;
		}
	}
	pthread_mutex_unlock(&gg->sess_mutex);
	return 0;
}

//////////////////////////////////////////////////////////
// when basic search
static void *__stdcall gg_searchthread(void *empty)
{
	GGPROTO *gg = (GGPROTO *)empty;
	SleepEx(100, FALSE);
#ifdef DEBUGMODE
	gg_netlog(gg, "gg_searchthread(): Failed search.");
#endif
	ProtoBroadcastAck(GG_PROTO, NULL, ACKTYPE_SEARCH, ACKRESULT_FAILED, (HANDLE)1, 0);
	return NULL;
}
HANDLE gg_basicsearch(PROTO_INTERFACE *proto, const char *id)
{
	GGPROTO *gg = (GGPROTO *)proto;
	pthread_t tid;
	gg_pubdir50_t req;

	pthread_mutex_lock(&gg->sess_mutex);
	if(!gg_isonline(gg))
	{
		pthread_mutex_unlock(&gg->sess_mutex);
		return (HANDLE)0;
	}

	if (!(req = gg_pubdir50_new(GG_PUBDIR50_SEARCH)))
	{ pthread_create(&tid, NULL, gg_searchthread, gg); pthread_detach(&tid); return (HANDLE)1; }

	// Add uin and search it
	gg_pubdir50_add(req, GG_PUBDIR50_UIN, id);
	gg_pubdir50_seq_set(req, GG_SEQ_SEARCH);

	if(!gg_pubdir50(gg->sess, req))
	{ pthread_create(&tid, NULL, gg_searchthread, gg); pthread_detach(&tid); return (HANDLE)1; }
#ifdef DEBUGMODE
	gg_netlog(gg, "gg_basicsearch(): Seq %d.", req->seq);
#endif
	gg_pubdir50_free(req);

	pthread_mutex_unlock(&gg->sess_mutex);
	return (HANDLE)1;
}
static HANDLE gg_searchbydetails(PROTO_INTERFACE *proto, const char *nick, const char *firstName, const char *lastName)
{
	GGPROTO *gg = (GGPROTO *)proto;
	pthread_t tid;
	gg_pubdir50_t req;
	unsigned long crc;
	char data[512] = "\0";

	// Check if connected and if there's a search data
	pthread_mutex_lock(&gg->sess_mutex);
	if(!gg_isonline(gg))
	{
		pthread_mutex_unlock(&gg->sess_mutex);
		return 0;
	}
	if(!nick && !firstName && !lastName)
		return 0;

	if (!(req = gg_pubdir50_new(GG_PUBDIR50_SEARCH)))
	{ pthread_create(&tid, NULL, gg_searchthread, gg); pthread_detach(&tid); return (HANDLE)1; }

	// Add uin and search it
	if(nick)
	{
		gg_pubdir50_add(req, GG_PUBDIR50_NICKNAME, nick);
		strncat(data, nick, sizeof(data) - strlen(data));
	}
	strncat(data, ".", sizeof(data) - strlen(data));

	if(firstName)
	{
		gg_pubdir50_add(req, GG_PUBDIR50_FIRSTNAME, firstName);
		strncat(data, firstName, sizeof(data) - strlen(data));
	}
	strncat(data, ".", sizeof(data) - strlen(data));

	if(lastName)
	{
		gg_pubdir50_add(req, GG_PUBDIR50_LASTNAME, lastName);
		strncat(data, lastName, sizeof(data) - strlen(data));
	}
	strncat(data, ".", sizeof(data) - strlen(data));

	// Count crc & check if the data was equal if yes do same search with shift
	crc = crc_get(data);

	if(crc == gg->last_crc && gg->next_uin)
		gg_pubdir50_add(req, GG_PUBDIR50_START, ditoa(gg->next_uin));
	else
		gg->last_crc = crc;

	gg_pubdir50_seq_set(req, GG_SEQ_SEARCH);

	if(!gg_pubdir50(gg->sess, req))
	{ pthread_create(&tid, NULL, gg_searchthread, gg); pthread_detach(&tid); return (HANDLE)1; }
#ifdef DEBUGMODE
	gg_netlog(gg, "gg_searchbyname(): Seq %d.", req->seq);
#endif
	gg_pubdir50_free(req);

	pthread_mutex_unlock(&gg->sess_mutex);
	return (HANDLE)1;
}

//////////////////////////////////////////////////////////
// when contact is added to list
HANDLE gg_addtolist(PROTO_INTERFACE *proto, int flags, PROTOSEARCHRESULT *psr)
{
	GGPROTO *gg = (GGPROTO *)proto;
	GGSEARCHRESULT *sr = (GGSEARCHRESULT *)psr;
	return gg_getcontact(gg, sr->uin, 1, flags & PALF_TEMPORARY ? 0 : 1, sr->hdr.nick);
}

//////////////////////////////////////////////////////////
// user info request
static void *__stdcall gg_cmdgetinfothread(void *empty)
{
	GGCONTEXT *ctx = (GGCONTEXT *)empty;
	SleepEx(100, FALSE);
#ifdef DEBUGMODE
	gg_netlog(ctx->gg, "gg_cmdgetinfothread(): Failed info retreival.");
#endif
	ProtoBroadcastAck(ctx->gg->proto.m_szModuleName, ctx->hContact, ACKTYPE_GETINFO, ACKRESULT_FAILED, (HANDLE) 1, 0);
	return NULL;
}
int gg_getinfo(PROTO_INTERFACE *proto, HANDLE hContact, int infoType)
{
	GGPROTO *gg = (GGPROTO *)proto;
	pthread_t tid;
	gg_pubdir50_t req;

	// Custom contact info
	if(hContact)
	{
		if (!(req = gg_pubdir50_new(GG_PUBDIR50_SEARCH)))
		{
			GGCONTEXT *ctx = (GGCONTEXT *)malloc(sizeof(GGCONTEXT));
			ctx->hContact = hContact; ctx->gg = gg;
			pthread_create(&tid, NULL, gg_cmdgetinfothread, ctx);
			pthread_detach(&tid);
			return 1;
		}

		// Add uin and search it
		gg_pubdir50_add(req, GG_PUBDIR50_UIN, ditoa((uin_t)DBGetContactSettingDword(hContact, GG_PROTO, GG_KEY_UIN, 0)));
		gg_pubdir50_seq_set(req, GG_SEQ_INFO);

#ifdef DEBUGMODE
		gg_netlog(gg, "gg_getinfo(): Requesting user info.", req->seq);
#endif
		pthread_mutex_lock(&gg->sess_mutex);
		if(gg_isonline(gg) && !gg_pubdir50(gg->sess, req))
		{
			GGCONTEXT *ctx = (GGCONTEXT *)malloc(sizeof(GGCONTEXT));
			ctx->hContact = hContact; ctx->gg = gg;
			pthread_create(&tid, NULL, gg_cmdgetinfothread, ctx);
			pthread_detach(&tid);
			pthread_mutex_unlock(&gg->sess_mutex);
			return 1;
		}
		pthread_mutex_unlock(&gg->sess_mutex);
	}
	// Own contact info
	else
	{
		if (!(req = gg_pubdir50_new(GG_PUBDIR50_READ)))
		{
			pthread_create(&tid, NULL, gg_cmdgetinfothread, hContact);
			pthread_detach(&tid);
			return 1;
		}

		// Add seq
		gg_pubdir50_seq_set(req, GG_SEQ_CHINFO);

#ifdef DEBUGMODE
		gg_netlog(gg, "gg_getinfo(): Requesting owner info.", req->seq);
#endif
		pthread_mutex_lock(&gg->sess_mutex);
		if(gg_isonline(gg) && !gg_pubdir50(gg->sess, req))
		{
			GGCONTEXT *ctx = (GGCONTEXT *)malloc(sizeof(GGCONTEXT));
			ctx->hContact = hContact; ctx->gg = gg;
			pthread_create(&tid, NULL, gg_cmdgetinfothread, ctx);
			pthread_detach(&tid);
			pthread_mutex_unlock(&gg->sess_mutex);
			return 1;
		}
		pthread_mutex_unlock(&gg->sess_mutex);
	}
#ifdef DEBUGMODE
	gg_netlog(gg, "gg_getinfo(): Seq %d.", req->seq);
#endif
	gg_pubdir50_free(req);

	return 1;
}

//////////////////////////////////////////////////////////
// when away message is requested
static void *__stdcall gg_getawaymsgthread(void *empty)
{
	GGCONTEXT *ctx = (GGCONTEXT *)empty;
	DBVARIANT dbv;

	SleepEx(100, FALSE);
	if (!DBGetContactSettingString(ctx->hContact, "CList", GG_KEY_STATUSDESCR, &dbv))
	{
		ProtoBroadcastAck(ctx->gg->proto.m_szModuleName, ctx->hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, (HANDLE) 1, (LPARAM) dbv.pszVal);

#ifdef DEBUGMODE
		gg_netlog(ctx->gg, "gg_getawaymsg(): Reading away msg \"%s\".", dbv.pszVal);
#endif
		DBFreeVariant(&dbv);
	}
	else
		ProtoBroadcastAck(ctx->gg->proto.m_szModuleName, ctx->hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, (HANDLE) 1, (LPARAM) NULL);
	free(ctx);
	return NULL;
}
int gg_getawaymsg(PROTO_INTERFACE *proto, HANDLE hContact)
{
	pthread_t tid;

	GGCONTEXT *ctx = malloc(sizeof(GGCONTEXT));
	ctx->hContact = hContact; ctx->gg = (GGPROTO *)proto;
	pthread_create(&tid, NULL, gg_getawaymsgthread, ctx);
	pthread_detach(&tid);

	return 1;
}

//////////////////////////////////////////////////////////
// when away message is beging set
int gg_setawaymsg(PROTO_INTERFACE *proto, int iStatus, const char *msg)
{
	GGPROTO *gg = (GGPROTO *)proto;
	int status = gg_normalizestatus(iStatus);
	char **szMsg;

#ifdef DEBUGMODE
	gg_netlog(gg, "gg_setawaymsg(): Requesting away message set to \"%s\".", msg);
#endif
	pthread_mutex_lock(&gg->sess_mutex);

	// Select proper msg
	switch(status)
	{
		case ID_STATUS_ONLINE:
			szMsg = &gg->modemsg.online;
			break;
		case ID_STATUS_AWAY:
			szMsg = &gg->modemsg.away;
			break;
		case ID_STATUS_INVISIBLE:
			szMsg = &gg->modemsg.invisible;
			break;
		default:
			pthread_mutex_unlock(&gg->sess_mutex);
			return 1;
	}

	// Check if we change status here somehow
	if(*szMsg && msg && !strcmp(*szMsg, msg)
		|| !*szMsg && (!msg || !*msg))	
	{
		if(status == gg->proto.m_iDesiredStatus && gg->proto.m_iDesiredStatus == gg->proto.m_iStatus)
		{
#ifdef DEBUGMODE
			gg_netlog(gg, "gg_setawaymsg(): Message hasn't been changed, return.");
#endif
			pthread_mutex_unlock(&gg->sess_mutex);
			return 0;
		}
	}
	else
	{
		if(*szMsg)
			free(*szMsg);
		*szMsg = msg && *msg ? _strdup(msg) : NULL;
#ifdef DEBUGMODE
		gg_netlog(gg, "gg_setawaymsg(): Message changed.");
#endif
	}
	pthread_mutex_unlock(&gg->sess_mutex);

	// Change the status only if it was desired by PS_SETSTATUS
	if(status == gg->proto.m_iDesiredStatus)
		gg_refreshstatus(gg, status);
#ifdef DEBUGMODE
	else
	{
		char error[128];
		mir_snprintf(error, sizeof(error), Translate("GG: PS_AWAYMSG was called without previous PS_SETSTATUS for status %d, desired %d, current %d."),
			status, gg->proto.m_iDesiredStatus, gg->proto.m_iStatus);
		PUShowMessage(error, SM_WARNING);
	}
#endif

	return 0;
}

//////////////////////////////////////////////////////////
// visible lists
int gg_setapparentmode(PROTO_INTERFACE *proto, HANDLE hContact, int mode)
{
	GGPROTO *gg = (GGPROTO *)proto;
	DBWriteContactSettingWord(hContact, proto->m_szModuleName, GG_KEY_APPARENT, (WORD)mode);
	pthread_mutex_lock(&gg->sess_mutex);
	if(gg_isonline(gg)) gg_notifyuser(gg, hContact, 1);
	pthread_mutex_unlock(&gg->sess_mutex);
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
HWND gg_createadvsearchui(PROTO_INTERFACE *proto, HWND owner)
{
	return CreateDialogParam(hInstance,
		MAKEINTRESOURCE(IDD_GGADVANCEDSEARCH), owner, gg_advancedsearchdlgproc, (LPARAM)(GGPROTO *)proto);
}

//////////////////////////////////////////////////////////
// search by advanced
HWND gg_searchbyadvanced(PROTO_INTERFACE *proto, HWND hwndDlg)
{
	GGPROTO *gg = (GGPROTO *)proto;
	pthread_t tid;
	gg_pubdir50_t req;
	char text[64], data[512] = "\0";
	unsigned long crc;

	// Check if connected
	if(!gg_isonline(gg)) return (HWND)0;

	if (!(req = gg_pubdir50_new(GG_PUBDIR50_SEARCH)))
	{ pthread_create(&tid, NULL, gg_searchthread, gg); pthread_detach(&tid); return (HWND)1; }

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
	if(strlen(data) <= 7 || (strlen(data) == 8 && IsDlgButtonChecked(hwndDlg, IDC_ONLYCONNECTED))) return (HWND)0;

	// Count crc & check if the data was equal if yes do same search with shift
	crc = crc_get(data);

	if(crc == gg->last_crc && gg->next_uin)
		gg_pubdir50_add(req, GG_PUBDIR50_START, ditoa(gg->next_uin));
	else
		gg->last_crc = crc;

	gg_pubdir50_seq_set(req, GG_SEQ_SEARCH);

	pthread_mutex_lock(&gg->sess_mutex);
	if(gg_isonline(gg) && !gg_pubdir50(gg->sess, req))
	{
		pthread_create(&tid, NULL, gg_searchthread, gg);
		pthread_detach(&tid);
		pthread_mutex_unlock(&gg->sess_mutex);
		return (HWND)1;
	}
	pthread_mutex_unlock(&gg->sess_mutex);
#ifdef DEBUGMODE
	gg_netlog(gg, "gg_searchbyadvanced(): Seq %d.", req->seq);
#endif
	gg_pubdir50_free(req);

	return (HWND)1;
}

//////////////////////////////////////////////////////////
// Dummies for function that have to be implemented

HANDLE gg_dummy_addtolistbyevent(PROTO_INTERFACE *proto, int flags, int iContact, HANDLE hDbEvent) { return NULL; }
int    gg_dummy_authorize(PROTO_INTERFACE *proto, HANDLE hContact) { return 0; }
int    gg_dummy_authdeny(PROTO_INTERFACE *proto, HANDLE hContact, const char *szReason) { return 0; }
int    gg_dummy_authrecv(PROTO_INTERFACE *proto, HANDLE hContact, PROTORECVEVENT *pre) { return 0; }
int    gg_dummy_authrequest(PROTO_INTERFACE *proto, HANDLE hContact, const char *szMessage) { return 0; }
HANDLE gg_dummy_changeinfo(PROTO_INTERFACE *proto, int iInfoType, void *pInfoData) { return NULL; }
int    gg_dummy_fileresume(PROTO_INTERFACE *proto, HANDLE hTransfer, int *action, const char **szFilename) { return 0; }
HANDLE gg_dummy_searchbyemail(PROTO_INTERFACE *proto, const char *email) { return NULL; }
int    gg_dummy_recvcontacts(PROTO_INTERFACE *proto, HANDLE hContact, PROTORECVEVENT *pre) { return 0; }
int    gg_dummy_recvurl(PROTO_INTERFACE *proto, HANDLE hContact, PROTORECVEVENT *pre) { return 0; }
int    gg_dummy_sendcontacts(PROTO_INTERFACE *proto, HANDLE hContact, int flags, int nContacts, HANDLE *hContactsList) { return 0; }
int    gg_dummy_sendurl(PROTO_INTERFACE *proto, HANDLE hContact, int flags, const char *url) { return 0; }
int    gg_dummy_recvawaymsg(PROTO_INTERFACE *proto, HANDLE hContact, int mode, PROTORECVEVENT *evt) { return 0; }
int    gg_dummy_sendawaymsg(PROTO_INTERFACE *proto, HANDLE hContact, HANDLE hProcess, const char *msg) { return 0; }
int    gg_dummy_useristyping(PROTO_INTERFACE *proto, HANDLE hContact, int type) { return 0; }

//////////////////////////////////////////////////////////
// Register services
void gg_registerservices(GGPROTO *gg)
{
	gg->proto.vtbl->AddToList              = gg_addtolist;
	gg->proto.vtbl->AddToListByEvent       = gg_dummy_addtolistbyevent;

	gg->proto.vtbl->Authorize              = gg_dummy_authorize;
	gg->proto.vtbl->AuthDeny               = gg_dummy_authdeny;
	gg->proto.vtbl->AuthRecv               = gg_dummy_authrecv;
	gg->proto.vtbl->AuthRequest            = gg_dummy_authrequest;

	gg->proto.vtbl->ChangeInfo             = gg_dummy_changeinfo;

	gg->proto.vtbl->FileAllow              = gg_fileallow;
	gg->proto.vtbl->FileCancel             = gg_filecancel;
	gg->proto.vtbl->FileDeny               = gg_filedeny;
	gg->proto.vtbl->FileResume             = gg_dummy_fileresume;

	gg->proto.vtbl->GetCaps                = gg_getcaps;
	gg->proto.vtbl->GetIcon                = gg_loadicon;
	gg->proto.vtbl->GetInfo                = gg_getinfo;

	gg->proto.vtbl->SearchBasic            = gg_basicsearch;
	gg->proto.vtbl->SearchByEmail          = gg_dummy_searchbyemail;
	gg->proto.vtbl->SearchByName           = gg_searchbydetails;
	gg->proto.vtbl->SearchAdvanced         = gg_searchbyadvanced;
	gg->proto.vtbl->CreateExtendedSearchUI = gg_createadvsearchui;

	gg->proto.vtbl->RecvContacts           = gg_dummy_recvcontacts;
	gg->proto.vtbl->RecvFile               = gg_recvfile;
	gg->proto.vtbl->RecvMsg                = gg_recvmessage;
	gg->proto.vtbl->RecvUrl                = gg_dummy_recvurl;

	gg->proto.vtbl->SendContacts           = gg_dummy_sendcontacts;
	gg->proto.vtbl->SendFile               = gg_sendfile;
	gg->proto.vtbl->SendMsg                = gg_sendmessage;
	gg->proto.vtbl->SendUrl                = gg_dummy_sendurl;

	gg->proto.vtbl->SetApparentMode        = gg_setapparentmode;
	gg->proto.vtbl->SetStatus              = gg_setstatus;

	gg->proto.vtbl->GetAwayMsg             = gg_getawaymsg;
	gg->proto.vtbl->RecvAwayMsg            = gg_dummy_recvawaymsg;
	gg->proto.vtbl->SendAwayMsg            = gg_dummy_sendawaymsg;
	gg->proto.vtbl->SetAwayMsg             = gg_setawaymsg;

	gg->proto.vtbl->UserIsTyping           = gg_dummy_useristyping;
                                            
	gg->proto.vtbl->OnEvent                = gg_event;
	
	CreateProtoService(PS_GETSTATUS, gg_getstatus, gg);
	CreateProtoService(PS_GETNAME, gg_getname, gg);
}
