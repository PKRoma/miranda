/*

Copyright 2005 Sam Kothari (egoDust)

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

#include <windows.h>
#include "miranda.h"
#include "oscar_svc.h"
#include "svc.h"
#include "itr.h"
#include "ackqueue.h"
#include "aim.h"

/*
caps unknown to libfaim: {563fc809-0b6f-41bd-9f79-422609dfa2f3}
ICQ MTN
*/

#define AIM_USER "screenname_or_icq#"
#define AIM_PASS "thepassword"

// from oscar.c
extern char g_Name[MAX_PATH]; 

typedef struct { // everything here can only be touched by the main thread
	ITRWORKER workerThread;
	unsigned int status;		// current ID_STATUS_*
	unsigned int RequestCookie; 
}  OSCAR_CLIENT_INFO;
static OSCAR_CLIENT_INFO OscarClientInfo;

// aim_session_t->aux_data
typedef struct {
	UINT hUserInfoRequestTimer;
	ACKQUEUE ackq;
} OSCAR_CLIENT_AUXDATA;


void __stdcall OscarClient_SetStatusMainThread(void * newstatus)
{
	unsigned int status = OscarClientInfo.status;
	OscarClientInfo.status = (unsigned int) newstatus;
	if ( OscarClientInfo.status == ID_STATUS_OFFLINE ) 
		CallProtoService(g_Name, PS_SETSTATUS, (WPARAM)ID_STATUS_OFFLINE, 0);
	ProtoBroadcastAck(g_Name, NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)status, (LPARAM) newstatus);
}

// can be called from any thread, always switches to the main thread
void OscarClient_SetStatus(unsigned int status)
{
	CallFunctionAsync(OscarClient_SetStatusMainThread, (void *) status);
}

#include "m_netlib.h"
static void aim_debug(aim_session_t *sess, int level, const char *format, va_list va)
{
	char szText[2048];

	_vsnprintf(szText,sizeof(szText),format,va);
	CallService(MS_NETLIB_LOG,(WPARAM)0,(LPARAM)szText);
}

static int debug_f(const char *fmt,...)
{
	va_list va;
	char szText[2048];

	va_start(va,fmt);
	_vsnprintf(szText,sizeof(szText),fmt,va);
	va_end(va);
	return CallService(MS_NETLIB_LOG,(WPARAM)0,(LPARAM)szText);
}

void Oscar_AddBosHandlers(aim_session_t * aim, aim_conn_t * connection_bos);
void Oscar_AddIconHandlers(aim_session_t * aim, aim_conn_t * ico);

/* libfaim callbacks */

/* Msg (SNAC 0x4's) */

static int Oscar_Msg_Incoming(aim_session_t * aim, aim_frame_t * frame, ...) /* SNAC(0x4,0x7) */
{
	va_list va;
	HANDLE hContact;
	char sn[128];
	fu16_t channel;
	aim_userinfo_t * ui;
	va_start(va, frame);
	channel = va_arg(va, int);
	ui = va_arg(va, aim_userinfo_t *);
	if ( !Miranda_Normalise(ui->sn, sn, sizeof(sn)) ) return 1;
	hContact=Miranda_FindContact(sn, 0);
	if ( hContact == NULL ) {
		debug_f("Oscar_Msg_Incoming: Did not add %s (%s) to the database, not found!", ui->sn, sn);
		return 1;
	}
	switch(channel) {
		case 1: // Normal
		{
			struct aim_incomingim_ch1_args * ch1 = va_arg(va, struct aim_incomingim_ch1_args *);
			debug_f("Oscar_Msg_Incoming: %s (%s), flags = %x, icbmflags = %x, msg = '%s', msglen = %u", sn, ui->sn, ui->flags, ch1->icbmflags, ch1->msg, ch1->msglen);
			{
				CCSDATA c = {0};
				PROTORECVEVENT pre = {0};
				c.hContact=hContact;
				c.szProtoService=PSR_MESSAGE;
				c.lParam=(LPARAM)&pre;
				pre.timestamp=time(NULL);
				pre.szMessage=ch1->msg;
				CallService(MS_PROTO_CHAINRECV, 0, (LPARAM)&c);
			}
			break;
		}
		case 2: // P2P
		{
			struct aim_incomingim_ch2_args * ch2 = va_arg(va, struct aim_incomingim_ch2_args *);
			debug_f("Oscar_Msg_Incoming: P2P: sts=%u, class=%x, proxy='%s', client='%s', verified='%s', port=%u, msg='%s'", 
				ch2->status, ch2->reqclass, ch2->proxyip, ch2->clientip, ch2->verifiedip, ch2->port, ch2->msg);
			break;
		}
		case 4: // Special
		{
			break;
		}
		default:
			debug_f("Oscar_Msg_Incoming: Unknown channel %u", channel);
	}
	va_end(va);
	return 1;
}

static int Oscar_Msg_Mtn(aim_session_t * aim, aim_frame_t * frame, ...) /* SNAC(0x4,0x14) */
{
	char * sn = 0;
	fu16_t t1, t2;
	va_list arg;
	va_start(arg, frame);
	t1 = va_arg(arg, unsigned int);
	sn = va_arg(arg, char *);
	t2 = va_arg(arg, unsigned int);
	va_end(arg);
	debug_f("Oscar_Msg_Mtn: sn = %s, %u:%u", sn, t1, t2);
	return 1;
}

static int Oscar_Msg_AutoResponse(aim_session_t * aim, aim_frame_t * frame, ...) /* SNAC(0x4, 0xB) */
{
	va_list va;
	unsigned int channel;
	char * sn;
	unsigned int reason;
	aim_userinfo_t * ui = NULL;
	unsigned int state;
	char * msg;
	va_start(va, frame);
	channel=va_arg(va, unsigned int);
	sn=va_arg(va, char *);
	reason=va_arg(va, unsigned int);
	state=va_arg(va, unsigned int);
	msg=va_arg(va, char *);
	ui=aim_locate_finduserinfo(aim, sn);
	debug_f("Oscar_Msg_AutoResponse: channel = %u, sn = %s, reason = %x", channel, sn, reason);
	switch ( channel ) {
		case 0x0002:
		{
			//ret = userfunc(sess, rx, channel, sn, reason, state, msg);
			if ( ui != NULL ) debug_f("auto: aim_locate_finduserinfo: %s, %s, %s", ui->sn, ui->info, ui->away);
			debug_f("libfaim fix: extra: %u, msg = %s", state, msg);
			break;
		}
		case 0x0004: 
		{
			break;
		}
		default:
		{
			debug_f("Oscar_Msg_AutoResponse: Unknown channel %x", channel);
			break;
		}
	}
	va_end(va);
	return 1;
}

/* End of Msg's */


/* auth server callbacks */
//#define CLIENTINFO_AIM_KNOWNGOOD CLIENTINFO_AIM_5_1_3036
//#define CLIENTINFO_ICQ_KNOWNGOOD CLIENTINFO_ICQ_5_45_3777

static int Oscar_ServerAuthKeyReply(aim_session_t * aim, aim_frame_t * frame, ...) /* SNAC(0x17,0x6) */
{
	/* this callback is fired when the server replies to SNAC(0x17,06) that libfaim will send 
	via aim_request_login() with SN, we expect to get a md5 key, which can be used with 
	aim_send_login() which will issue SNAC (0x17, 02) for us */
	char * md5key = 0;
	va_list arg;
	struct client_info_s client_info = CLIENTINFO_AIM_KNOWNGOOD; 

	va_start(arg, frame);
	md5key = va_arg(arg, char *);
	va_end(arg);
	/* issue SNAC (0x17,02) */
	aim_send_login(aim, frame->conn, AIM_USER, AIM_PASS, &client_info, md5key);
	debug_f("Oscar_ServerAuthKeyReply: got MD5 key, %s", md5key ? md5key : "KEY MISSING");
	return 1;
}

static int Oscar_ServerAuthReply(aim_session_t * aim, aim_frame_t * frame, ...)
{
	/* this callback is fired when the server replies to SNAC(0x17, 02) to us with SNAC(0x17,03)
	which should contain BOS server IP and BOS cookie, and or error information if we were not successful. */
	va_list va;
	struct aim_authresp_info * info;
	va_start(va, frame);
	info = (struct aim_authresp_info *) va_arg(va, struct aim_autresp_info *);
	va_end(va);
	debug_f("Oscar_ServerAuthReply: sn = %s, error = %u, bos-ip = %s, regstatus = %u, email = %s, cookielen = %u \n", 
		info->sn, info->errorcode, info->bosip, info->regstatus, info->email, info->cookielen);
	if ( info != NULL ) {
		if ( info->errorcode == 0 ) {
			aim_conn_t * connection_bos = 0;
			aim_conn_kill(aim, &frame->conn);
			debug_f(">>> closing auth server connection and starting BOS connection");
			connection_bos = aim_newconn(aim, AIM_CONN_TYPE_BOS, info->bosip);
			/* setup SNAC family handlers for BOS */
			Oscar_AddBosHandlers(aim, connection_bos);
			/* send BOS the cookie we just got */
			aim_sendcookie(aim, connection_bos, info->cookielen, info->cookie);
		} else {
			/* XXX: something bad happened */
			debug_f("Oscar_ServerAuthReply: ERROR! = %x ", info->errorcode);		
			OscarClient_SetStatus(ID_STATUS_OFFLINE);
			return 1;
		}
	}
	return 1;
}

/* end of auth server callbacks */

/* start of gen service callbacks */

static int Oscar_Gen_Redirect(aim_session_t * aim, aim_frame_t * frame, ...) /* SNAC(0x0001,0x0005) */
{
	va_list va;
	struct aim_redirect_data * redir;
	va_start(va, frame);
	redir=va_arg(va, struct aim_redirect_data *);
	va_end(va);
	debug_f("Oscar_Gen_Redirect: group = %x, port = '%s', cookielen = %u", redir->group, redir->ip, redir->cookielen);
	switch ( redir->group ) {
		case AIM_CB_FAM_ICO:
		{
			// XXX: bad! the port is missing and new_conn
			aim_conn_t * ico = aim_newconn(aim, AIM_CONN_TYPE_ICON, redir->ip);
			if ( ico == NULL ) {
				debug_f("fuck! aim_newconn() returned NULL.");
				break;
			}
			// add handlers
			Oscar_AddIconHandlers(aim, ico);
			// this won't be transmitted til the connection is actually made.
			aim_sendcookie(aim, ico, redir->cookielen, redir->cookie);
			break;
		}
	}
	return 1;
}

/* end of gen service callbacks */

/* start of location service callbacks */

static int Oscar_Loc_Limits(aim_session_t * aim, aim_frame_t * frame, ...) /* SNAC(0x0002, 0x0003) */
{
	//unsigned int capbits = AIM_CAPS_SENDFILE | AIM_CAPS_ICQUTF8 | AIM_CAPS_INTEROPERATE | AIM_CAPS_ICHAT;;
	//unsigned int capbits = AIM_CAPS_INTEROPERATE | AIM_CAPS_BUDDYICON | AIM_CAPS_DIRECTIM | AIM_CAPS_CHAT | 
	//	AIM_CAPS_GETFILE | AIM_CAPS_SENDFILE | AIM_CAPS_ICQUTF8;
	unsigned int capbits = AIM_CAPS_CHAT | AIM_CAPS_BUDDYICON | AIM_CAPS_DIRECTIM | AIM_CAPS_SENDFILE 
		| AIM_CAPS_INTEROPERATE | AIM_CAPS_ICHAT | AIM_CAPS_ICQUTF8 | AIM_CAPS_GENERICUNKNOWN 
		| AIM_CAPS_ICQSERVERRELAY | AIM_CAPS_ICQRTF;
	va_list arg;
	unsigned int profile_len = 0;
	/* XXX: this should be kept somewhere */
	va_start(arg, frame);
	profile_len = va_arg(arg, unsigned int);
	va_end(arg);

#ifndef DONTLOG
	debug_f("Oscar_Loc_Limits: profile length = %u", profile_len);
	debug_f("Sending SNAC(0x0002,0x0004) client capabilities and profile to server, caps = %x", capbits);
#endif
	/* send SNAC(0x0002,0x0004) */
	aim_locate_setcaps(aim, capbits);
	return 1;
}

static int Oscar_Loc_UserInfoTimeout(aim_session_t * aim, aim_frame_t * frame, ...)
{
	OSCAR_CLIENT_AUXDATA * aux = (OSCAR_CLIENT_AUXDATA *) aim->aux_data;
	KillTimer(NULL, aux->hUserInfoRequestTimer);
	aux->hUserInfoRequestTimer=SetTimer(NULL, 0, 1000 * 10, NULL);
	debug_f("Oscar_Loc_UserInfoTimeout: requesting data in 5 s");	
	return 1;
}

static int Oscar_Loc_UserInfo(aim_session_t * aim, aim_frame_t * frame, ...) /* SNAC(0x0002,0x0006) */
{
	OSCAR_CLIENT_AUXDATA * aux = (OSCAR_CLIENT_AUXDATA *) aim->aux_data;
	ACKINFO ackInfo = {0};
	aim_userinfo_t * ui = 0;
	va_list arg;
	char sn[128];
	char msg[0x2000];
	HANDLE hContact;
	va_start(arg, frame);
	ui = va_arg(arg, aim_userinfo_t *);
	va_end(arg);
	if (!Miranda_Normalise(ui->sn, sn, sizeof(sn))) return 1;
	hContact=Miranda_FindContact(sn, 0);
	if ( hContact == NULL ) {		
		debug_f("Oscar_Loc_UserInfo: did not add %s (%s) to database! not found", sn, ui->sn);
		return 1;
	}
	if ( !AckQueue_Remove(&aux->ackq, hContact, ACKTYPE_AWAYMSG, &ackInfo) ) {
		debug_f("Oscar_Loc_UserInfo: %s returned info, but no matching ack.", sn);
		return 1;
	}	
	if ( ui->away != NULL ) {
		lstrcpyn(msg, ui->away, ui->away_len + 1 < sizeof(msg) ? ui->away_len + 1 : sizeof(msg) );
		ProtoBroadcastAck(g_Name, hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, (HANDLE)ackInfo.hSeq, (LPARAM) msg);
	} else {
		debug_f("Oscar_Loc_UserInfo: '%s' doesnt have an away msg!.", sn);
		/* XXX: request an old style away msg */
		aim_im_sendch2_geticqaway(aim, sn, AIM_ICQ_STATE_AWAY);
	}
	return 1;
}

/* End of location service callbacks */

/* Start of buddy service callbacks */

// info->sn is the formatted screen name
static int Oscar_Buddy_Oncoming(aim_session_t * aim, aim_frame_t * frame, ...) /* SNAC(0x0003,0x000B) */
{
	va_list va;
	aim_userinfo_t * ui;
	char sn[1024];
	HANDLE hContact;

	va_start(va, frame);
	ui = va_arg(va, aim_userinfo_t *);
	va_end(va);

	// normalise the screen name, if this fails then we'll never find the hContact anyway
	if ( !Miranda_Normalise(ui->sn, sn, sizeof(sn)) ) return 1;
	// find the contact, don't add if not present
	hContact=Miranda_FindContact(sn, 0);
	if ( hContact == NULL ) {
		debug_f("Oscar_Buddy_Oncoming: %s (%s) isnt in the database, did not add! ", sn, ui->sn);
		return 1;
	}
	debug_f("Oscar_Buddy_Oncoming: %s (%s) has %x present and %x caps, icq = %x \n", sn, ui->sn, ui->present, ui->capabilities, ui->icqinfo.status);
	// icq/aim status diff
	if ( ui->present & AIM_USERINFO_PRESENT_ICQEXTSTATUS ) {
		// ui->icqinfo.status is valid, AIM_ICQ_STATE_*
		DBWriteContactSettingWord(hContact, g_Name, "Status", Miranda_TransformStatus(ui->icqinfo.status));
	} else if ( ui->present & AIM_USERINFO_PRESENT_FLAGS ) {
		DBWriteContactSettingWord(hContact, g_Name, "Status", ui->flags & AIM_FLAG_AWAY ? ID_STATUS_AWAY : ID_STATUS_ONLINE );
	}
	// XXX: write the nickname! baaaaaaaaad.
	DBWriteContactSettingString(hContact, g_Name, "Nick", ui->sn);
	// mhmm.
	if ( ui->iconcsumlen > 0 ) {
		fu8_t icon[16] = {0};
		memcpy(icon, ui->iconcsum, 16);
		debug_f("%s has a buddy icon of hash: %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", sn, icon[0], icon[1], icon[2], icon[3], icon[4], icon[5], icon[6], icon[7], 
			icon[8], icon[9], icon[10], icon[11], icon[12], icon[13], icon[14], icon[15]);
	} else {
		debug_f("%s hasn't got a buddy icon.", sn);
	}
	return 1;
}

static int Oscar_Buddy_Offgoing(aim_session_t * aim, aim_frame_t * frame, ...) /* SNAC(0x0003, 0x000c */
{
	va_list va;
	aim_userinfo_t * ui;
	char sn[1024];
	HANDLE hContact;

	va_start(va, frame);
	ui = va_arg(va, aim_userinfo_t *);
	va_end(va);

	// normalise the screen name, if this fails then we'll never find the hContact anyway
	if ( !Miranda_Normalise(ui->sn, sn, sizeof(sn)) ) return 1;
	// find the contact, don't add if not present
	hContact=Miranda_FindContact(sn, 0);
	if ( hContact == NULL ) {
		debug_f("Oscar_Buddy_Offgoing: '%s' isnt in the database, did not add! ", ui->sn);
		return 1;
	}

	DBWriteContactSettingWord(hContact, g_Name, "Status", ID_STATUS_OFFLINE);	

	return 1;
}

/* End of buddy service callbacks */

/* Start of BOS/related callbacks */

static int Oscar_Bos_BuddyListRights(aim_session_t * aim, aim_frame_t * frame, ...) /* SNAC(0x0003,0x0003) */
{
	unsigned int buddies_max = 0;
	unsigned int watchers_max = 0;
	unsigned int online_max = 0;
	va_list arg;
	va_start(arg, frame);
	buddies_max = va_arg(arg, unsigned int);
	watchers_max = va_arg(arg, unsigned int);
	online_max = va_arg(arg, unsigned int);
	va_end(arg);
#ifndef DONTLOG
	debug_f("Oscar_Bos_BuddyListRights: buddies = %u, watchers = %u, online = %u", buddies_max, watchers_max, online_max);
#endif
	/* server will reply with SNAC(0x0003,0x000b) when sn goes online/changes state */
	aim_conn_addhandler(aim, frame->conn, AIM_CB_FAM_BUD, AIM_CB_BUD_ONCOMING, Oscar_Buddy_Oncoming, 0); 
	/* server will reply with SNAC(0x0003,0x000c) when sn goes offline */
	aim_conn_addhandler(aim, frame->conn, AIM_CB_FAM_BUD, AIM_CB_BUD_OFFGOING, Oscar_Buddy_Offgoing, 0);
	return 1;
}

static int Oscar_Bos_ICBMParamInfo(aim_session_t * aim, aim_frame_t * frame, ...) /* SNAC(0x0004,0x0005) */
{	
	struct aim_icbmparameters * icbm = 0;
	va_list arg;
	va_start(arg, frame);
	icbm = va_arg(arg, struct aim_icbmparameters *);
	va_end(arg);
	if ( icbm != 0 ) {
		debug_f("Oscar_Bos_ICBMParamInfo: max-chan: %u, max-len: %u, min-msg-interval: %u", 
		icbm->maxchan, icbm->maxmsglen, icbm->minmsginterval);
	} else {
		debug_f("Oscar_Bos_ICBMParamInfo: icbm arg was NULL");
	}
	// tell the server our shit
	// XXX: iCBM flags, 
	// 0001 = messages allowed for specific channel
	// 0010 = missed calls
	// 1000 = supports MTN
	icbm->flags = 0x0000000b;
	icbm->maxmsglen=8000;
	icbm->minmsginterval = 0;
	/* will generate SNAC(0x4,0x2) */
	aim_im_setparams(aim, icbm);
	
	// got ICBM information, hook up SNAC(0x4,0x7), SNAC(0x4,0x14), SNAC(0x4,0xB)
	aim_conn_addhandler(aim, frame->conn, AIM_CB_FAM_MSG, AIM_CB_MSG_INCOMING, Oscar_Msg_Incoming, 0);
	aim_conn_addhandler(aim, frame->conn, AIM_CB_FAM_MSG, AIM_CB_MSG_MTN, Oscar_Msg_Mtn, 0);
	aim_conn_addhandler(aim, frame->conn, AIM_CB_FAM_MSG, AIM_CB_MSG_CLIENTAUTORESP, Oscar_Msg_AutoResponse, 0);
	return 1;
}

static int Oscar_Bos_ConnectionDone(aim_session_t * aim, aim_frame_t * frame, ...)
{
#ifndef DONTLOG
	debug_f("Oscar_Bos_ConnectionDone: Basic stuff done");
#endif
	// XXX: snac? get personal info
	aim_reqpersonalinfo(aim, frame->conn);
	// request SSI + data
	aim_ssi_reqrights(aim);
	aim_ssi_reqdata(aim);
	//
	aim_locate_reqrights(aim);
	aim_buddylist_reqrights(aim, frame->conn);
	aim_im_reqparams(aim);
	aim_bos_reqrights(aim, frame->conn);
	return 1;
}

static int Oscar_Bos_SSI_Rights(aim_session_t * aim, aim_frame_t * frame, ...) /* SNAC(0x0013,0x0003) */
{
#ifndef DONTLOG
	debug_f("Oscar_Bos_SSI_Rights: Got SSI information, buddies, groups, permits, denies.");
#endif
	return 1;
}

static int Oscar_Bos_SSI_List(aim_session_t * aim, aim_frame_t * frame, ...) /* SNAC(0x0013, 0x6) */
{
	va_list arg;
	unsigned int fmtver = 0;
	unsigned int numitems = 0;
	unsigned int timestamp  = 0;
	struct aim_ssi_item * item = 0;

	va_start(arg, frame);
	fmtver = va_arg(arg, int);
	numitems = va_arg(arg, int);
	item = va_arg(arg, struct aim_ssi_item *);
	timestamp = va_arg(arg, unsigned int);
	va_end(arg);
#ifndef DONTLOG
	debug_f("Oscar_Bos_SSI_List: Got SSI list, fmt = %u, timestamp = %u, items = %u", fmtver, timestamp, numitems);
#endif
	while ( item != 0 ) {
		if ( item->type == AIM_SSI_TYPE_BUDDY ) { /* type 0x000 are buddies */
			Miranda_FindContact(item->name, 1);
		}
		item = item->next;
	}
	/* tell the server we are okay with this list and to start using it, this gives away our status to other people */
	aim_ssi_enable(aim);
	return 1;
}

static int Oscar_Bos_Rights(aim_session_t * aim, aim_frame_t * frame, ...) /* SNAC(0x0009,0x0003) */
{
	/* after BOS init is done, SNAC (0x0009, 0002) is sent and this is the reply, it is the last request
		that we send so we should go online after this. */
	fu16_t max_permits = 0;
	fu16_t max_denies = 0;

	va_list arg;
	va_start(arg, frame);
	max_permits = (fu16_t) va_arg(arg, unsigned int);
	max_denies = (fu16_t) va_arg(arg, unsigned int);
	va_end(arg);
	
	debug_f("Oscar_Bos_Rights: permits %u, denies %u", max_permits, max_denies);

	/* tell BOS we are ready and set idle stuff */
	aim_clientready(aim, frame->conn);	
	aim_srv_setavailmsg(aim, NULL);
	aim_srv_setidle(aim, 0);
	// XXX: makes no difference to online state?
	aim_setextstatus(aim, AIM_ICQ_STATE_NORMAL);
	/* broadcast we're online */
	OscarClient_SetStatus(ID_STATUS_ONLINE);
	/* XXX: request other services? */

	debug_f("Issued aim_reqservice() for BOS.");
	// XXX: icon connection dropping causes all sorts of bad things to happen in the main loop, fix this.
	//aim_reqservice(aim, frame->conn, AIM_CONN_TYPE_ICON);
		
	return 1;
}

/* end of BOS server callbacks, etc */

/* start of icon callbacks */

static int Oscar_Icon_ConnDone(aim_session_t * aim, aim_frame_t * frame, ...)
{
	debug_f("Oscar_Icon_ConnDone: connected to icon server, aim_clientready()");
	aim_clientready(aim, frame->conn);
	return 1;
}

static int Oscar_Icon_ConnErr(aim_session_t * aim, aim_frame_t * frame, ...)
{
	debug_f("Oscar_Icon_ConnErr: BAD BAD BAD BAD!");
	return 1;
}

static int Oscar_Icon_Response(aim_session_t * aim, aim_frame_t * frame, ...) /* SNAC(0x0010, 0x0005) */
{
	debug_f("Oscar_Icon_Response! ");
	return 1;
}

static int Oscar_Icon_Error(aim_session_t * aim, aim_frame_t * frame, ...) /* SNAC(0x0010, 0x0001) */
{
	debug_f("Oscar_Icon_Error! ");
	return 1;
}

void Oscar_AddIconHandlers(aim_session_t * aim, aim_conn_t * ico)
{
	aim_conn_addhandler(aim, ico, AIM_CB_FAM_ICO, AIM_CB_ICO_RESPONSE, Oscar_Icon_Response, 0);
	aim_conn_addhandler(aim, ico, AIM_CB_FAM_ICO, AIM_CB_ICO_ERROR, Oscar_Icon_Error, 0);
	aim_conn_addhandler(aim, ico, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNINITDONE, Oscar_Icon_ConnDone, 0);
	aim_conn_addhandler(aim, ico, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNERR, Oscar_Icon_ConnErr, 0);
}

/* end of icon callbacks */

/* start of ICQ callbacks  */

static int Oscar_Icq_Info(aim_session_t * aim, aim_frame_t * frame, ...) /* SNAC(0x0015,0x0003) */
{
	va_list va;
	struct aim_icq_info * ui;
	va_start(va, frame);
	ui = va_arg(va, struct aim_icq_info *);
	va_end(va);
	debug_f("Oscar_Icq_Info: Got response!, nick = %s, first = %s, last = %s, email = %s, info = %s \n", 
		ui->nick, ui->first, ui->last, ui->email, ui->info);
	return 1;
}

static int Oscar_Icq_InfoShort(aim_session_t * aim, aim_frame_t * frame, ...) /* SNAC(0x0015,0x0003) (meta info short) */
{
	va_list va;
	struct aim_icq_info * ui;
	char sn[64];	
	HANDLE hContact=INVALID_HANDLE_VALUE;
	va_start(va, frame);
	ui = va_arg(va, struct aim_icq_info *);
	va_end(va);
	debug_f("Oscar_Icq_InfoShort: nick = %s, first = %s, last = %s, email = %s \n", 
		ui->nick, ui->first, ui->last, ui->email);
	_snprintf(sn,sizeof(sn),"%u", ui->uin);
	hContact=Miranda_FindContact(sn, 0);
	if ( hContact == NULL ) {
		debug_f("Oscar_Icq_InfoShort: %s isnt in the database, not adding \n", sn);
		return 1;
	}
	DBWriteContactSettingString(hContact, g_Name, "Nick", ui->nick);
	DBWriteContactSettingString(hContact, g_Name, "FirstName", ui->first);
	return 1;
}

/* end of ICQ callbacks */

/* libfaim special callbacks */

static int Oscar_Conn_Err(aim_session_t * aim, aim_frame_t * frame, ...) // AIM_CB_SPECIAL_CONNERR
{
	va_list va;
	fu16_t code;
	char * msg;
	va_start(va, frame);
	code=va_arg(va, int);
	msg=va_arg(va, char *);
	va_end(va);
	debug_f("Oscar_Conn_Err: code = %x, msg = '%s', conn-type = %x", code, msg ? msg : "", frame->conn->type);
	OscarClient_SetStatus(ID_STATUS_OFFLINE);
	return 1;
}

/* end of libfaim special callbacks */

/* end of libfaim callbacks */

void Oscar_AddBosHandlers(aim_session_t * aim, aim_conn_t * connection_bos)
{
	/* generically called if there are any errors during BOS */
	aim_conn_addhandler(aim, connection_bos, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNERR, Oscar_Conn_Err, 0);
	/* libfaim will issue this callback once we've sent a BOS cookie and it's requested the service list,
	version numbers, rate information, rate limits */
	aim_conn_addhandler(aim, connection_bos, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNINITDONE, Oscar_Bos_ConnectionDone, 0);
	/* The server will reply with location service limits when we issue SNAC(0x0002,0x0002) inside connection done */
	aim_conn_addhandler(aim, connection_bos, AIM_CB_FAM_LOC, AIM_CB_LOC_RIGHTSINFO, Oscar_Loc_Limits, 0);
	/* libfaim will call this when a timeout occurs and we should update data */
	aim_conn_addhandler(aim, connection_bos, AIM_CB_FAM_LOC, AIM_CB_LOC_REQUESTINFOTIMEOUT, Oscar_Loc_UserInfoTimeout, 0);
	/*  The server will reply with SNAC(0x0002,0x0006) with user info */
	aim_conn_addhandler(aim, connection_bos, AIM_CB_FAM_LOC, AIM_CB_LOC_USERINFO, Oscar_Loc_UserInfo, 0);
	/* The server will reply with buddy list service limits when we issue SNAC (0x0003,0x002) with 0x0003 */
	aim_conn_addhandler(aim, connection_bos, AIM_CB_FAM_BUD, AIM_CB_BUD_RIGHTSINFO, Oscar_Bos_BuddyListRights, 0);
	/* The server will reply with ICBM args when we issue SNAC(0x0004,0x0004) with 0x0005 */
	aim_conn_addhandler(aim, connection_bos, AIM_CB_FAM_MSG, AIM_CB_MSG_PARAMINFO, Oscar_Bos_ICBMParamInfo, 0);
	/* The server will reply with SSI limits when we issue SNAC(0x0013,0x002) with 0x0003 */
	aim_conn_addhandler(aim, connection_bos, AIM_CB_FAM_SSI, AIM_CB_SSI_RIGHTSINFO, Oscar_Bos_SSI_Rights, 0);
	/* The server will reply with SSI data */
	aim_conn_addhandler(aim, connection_bos, AIM_CB_FAM_SSI, AIM_CB_SSI_LIST, Oscar_Bos_SSI_List, 0);
	/* The server will reply with BOS rights */
	aim_conn_addhandler(aim, connection_bos, AIM_CB_FAM_BOS, AIM_CB_BOS_RIGHTS, Oscar_Bos_Rights, 0);
	/* The server will reply with SNAC(0x0015,0x0003) for ICQ meta info */
	aim_conn_addhandler(aim, connection_bos, AIM_CB_FAM_ICQ, AIM_CB_ICQ_INFO, Oscar_Icq_Info, 0);
	aim_conn_addhandler(aim, connection_bos, AIM_CB_FAM_ICQ, AIM_CB_ICQ_ALIAS, Oscar_Icq_InfoShort, 0);
	/* when aim_reqservice() is used to ask for icon server (on clientready), it will send info via a redirect for the group with IP+cookie */
	aim_conn_addhandler(aim, connection_bos, AIM_CB_FAM_GEN, AIM_CB_GEN_REDIRECT, Oscar_Gen_Redirect, 0);
}

void ITR_GetStatusMsg_Handler(aim_session_t * aim, HANDLE hContact, HANDLE hSeq) /* handle WM_ITR_GETSTATUSMSG */
{
	OSCAR_CLIENT_AUXDATA * aux = (OSCAR_CLIENT_AUXDATA *) aim->aux_data;
	aim_userinfo_t * ui = NULL;
	char sn[128];
	if (!Miranda_ReverseFindContact(hContact,sn,sizeof(sn))) {
		debug_f("ITR_GetStatusMsg_Handler: Unable to find screen name for %x (cookie = %u)", hContact, hSeq);
		return;
	}
	/* XXX: We send this to ICQ people too, newer ICQs reply the way AIM does, if this fails then we have to send
	a channel 2 ICBM for the status */
	aim_locate_getinfoshort(aim, sn, 0x00000006);
	AckQueue_Insert(&aux->ackq, hContact, ACKTYPE_AWAYMSG, hSeq);
}

void OscarServerThread(HANDLE hHeap, void * pExtra)
{
	aim_session_t aim;
	OSCAR_CLIENT_AUXDATA * aux;
	aim_conn_t * auth_conn = NULL;
	MSG msg;
	BOOL dying=FALSE;

	/* start aim to use blocking sockets, give debug info and not use a queue */
	/// XXX: we using NON blocking sockets!
	aim_session_init(&aim, FALSE, 2000); 
	aim_setdebuggingcb(&aim, aim_debug);
	aim_tx_setenqueue(&aim, AIM_TX_IMMEDIATE, NULL);
	/* setup internal data */
	aim.aux_data=aux=calloc(sizeof(OSCAR_CLIENT_AUXDATA),1);
	/* broadcast the fact we're connecting */
	OscarClient_SetStatus(ID_STATUS_CONNECTING);	
	/* need a connection to add handlers to */
	auth_conn = aim_newconn(&aim, AIM_CONN_TYPE_AUTH, "login.oscar.aol.com:5190");
	if ( auth_conn != NULL ) 
	{
		/* generically handle any disconnection */
		aim_conn_addhandler(&aim, auth_conn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNERR, Oscar_Conn_Err, 0);
		/* these two SNACs handle server replies for BOS connection info and BOS auth key */
		aim_conn_addhandler(&aim, auth_conn, 0x0017, 0x0003, Oscar_ServerAuthReply, 0);
		aim_conn_addhandler(&aim, auth_conn, 0x0017, 0x0007, Oscar_ServerAuthKeyReply, 0);		
		/* this at the libfaim level requests login via SNAC 0x0017,0x0002 */
		aim_request_login(&aim, auth_conn, AIM_USER);
	} 
	else dying++;	
	// loop
	for (;!dying;) {
		while(PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
			DispatchMessage(&msg);
			switch ( msg.message ) {	
				case WM_TIMER: 
				{
					if ( msg.wParam == aux->hUserInfoRequestTimer ) {
						KillTimer(NULL, aux->hUserInfoRequestTimer);
						aux->hUserInfoRequestTimer=0;
						debug_f("got delay, aim_locate_dorequest()");
						//aim_locate_dorequest(&aim);
					}
					break;
				}
				/* fetch the status message for contact, wParam = hContact, lParam = seq */
				case WM_ITR_GETSTATUSMSG:
				{
					if ( !dying ) ITR_GetStatusMsg_Handler(&aim, (HANDLE) msg.wParam, (HANDLE) msg.lParam);
					break;
				}
				case WM_ITR_QUIT:
				{
					dying=TRUE;
					break;
				}
			} // switch
		} //while
		aim_conn_t * conn;
		struct timeval tv;
		int status;
		tv.tv_sec=1;
		tv.tv_usec=0; 
		//tv.tv_usec=500000; 
		conn=aim_select(&aim, &tv, &status);
		if ( conn != NULL && aim_get_command(&aim, conn) == 0 )	aim_rxdispatch(&aim);
		aim_tx_flushqueue(&aim);		
	} //for(;;)
	Miranda_SetContactsOffline();
	KillTimer(NULL, aux->hUserInfoRequestTimer);
	free(aux);
	aim_logoff(&aim);
	aim_session_kill(&aim);	
	HeapDestroy(hHeap);
	OscarClient_SetStatus(ID_STATUS_OFFLINE);
}

int OscarGetCaps(WPARAM wParam, LPARAM lParam)
{
	switch ( wParam ) {
		case PFLAGNUM_1:
		{
			return PF1_IM /*| PF1_SERVERCLIST*/ | PF1_AUTHREQ | PF1_PEER2PEER | PF1_MODEMSGRECV;
		}
		case PFLAGNUM_2:
		{
			return PF2_ONLINE | PF2_SHORTAWAY;
		}
		case PFLAGNUM_3:
		{
			return PF2_SHORTAWAY | PF2_LONGAWAY;
		}
		case PFLAG_UNIQUEIDSETTING:
		{
			return (int) (char *) OSCAR_CONTACT_KEY;
		}
	}
	return 0;
}

int OscarGetName(WPARAM wParam, LPARAM lParam)
{
	char * p = (char *) lParam;
	if ( p != 0 ) {
		strncpy(p, g_Name, wParam);
		return 0;
	}
	return 1;
}

int OscarGetStatus(WPARAM wParam, LPARAM lParam)
{
	return OscarClientInfo.status;
}

int OscarSetStatus(WPARAM wParam, LPARAM lParam)
{
	if ( ITR_ActiveWorker(&OscarClientInfo.workerThread) ) {
		// have a connection!
		if ( wParam == ID_STATUS_OFFLINE ) {
			// go offline
			ITR_FreeWorker(&OscarClientInfo.workerThread);
		} else {
			// change mode
			return 1;
		}
		return 0;
	} else if ( wParam != ID_STATUS_OFFLINE ) {
		// don't have a connection and told to go online
		ITR_NewWorker(&OscarClientInfo.workerThread, OscarServerThread, NULL);
		return 0;
	}
	return 1;
}

int OscarRecvMessage(WPARAM wParam, LPARAM lParam)
{
	CCSDATA * c=(CCSDATA *) lParam;
	PROTORECVEVENT * pre = (PROTORECVEVENT *) c->lParam;
	DBEVENTINFO dbei = {0};
	dbei.cbSize=sizeof(dbei);
	dbei.szModule=g_Name;
	dbei.timestamp=pre->timestamp;
	dbei.eventType=EVENTTYPE_MESSAGE;
	dbei.cbBlob=strlen(pre->szMessage) + 1;
	dbei.pBlob = (PBYTE) pre->szMessage;
	CallService(MS_DB_EVENT_ADD, (WPARAM) c->hContact, (LPARAM) &dbei);
	return 0;
}

#if 0
int OscarRecvStatusMessage(WPARAM wParam, LPARAM lParam)
{
	//CCSDATA * c = (CCSDATA *)lParam;
	//PROTORECVEVENT * pre = (PROTORECVEVENT *) c->lParam;
	//ProtoBroadcastAck(g_Name, c->hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, (HANDLE)0, (LPARAM) pre->szMessage);
	return 0;
}
#endif

int OscarRequestStatusMessage(WPARAM wParam, LPARAM lParam)
{
	CCSDATA * c = (CCSDATA *) lParam;
	HANDLE hSeq;
	hSeq=(HANDLE)++OscarClientInfo.RequestCookie;
	if (ITR_PostEvent(&OscarClientInfo.workerThread, WM_ITR_GETSTATUSMSG, (WPARAM) c->hContact, (LPARAM) hSeq))
		return (int)hSeq;
	return 0;
}

void LoadServices(void)
{	
	OscarClientInfo.status = ID_STATUS_OFFLINE;
	CreateProtoServiceFunction(g_Name, PS_GETCAPS, OscarGetCaps);
	CreateProtoServiceFunction(g_Name, PS_GETNAME, OscarGetName);
	CreateProtoServiceFunction(g_Name, PS_GETSTATUS, OscarGetStatus);
	CreateProtoServiceFunction(g_Name, PS_SETSTATUS, OscarSetStatus);
	CreateProtoServiceFunction(g_Name, PSS_GETAWAYMSG, OscarRequestStatusMessage);
	//CreateProtoServiceFunction(g_Name, PSR_AWAYMSG, OscarRecvStatusMessage); 
	CreateProtoServiceFunction(g_Name, PSR_MESSAGE, OscarRecvMessage);
}

void UnloadServices(void)
{
}

