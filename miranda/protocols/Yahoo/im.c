/*
 * $Id$
 *
 * myYahoo Miranda Plugin 
 *
 * Authors: Gennady Feldman (aka Gena01) 
 *          Laurent Marechal (aka Peorth)
 *
 * This code is under GPL and is based on AIM, MSN and Miranda source code.
 * I want to thank Robert Rainwater and George Hazan for their code and support
 * and for answering some of my questions during development of this plugin.
 */
#include <time.h>
#include <malloc.h>
#include <sys/stat.h>

#include "yahoo.h"

#include <m_langpack.h>
#include <m_protosvc.h>
#include <m_system.h>

#include "avatar.h"
#include "im.h"
#include "ignore.h"

extern yahoo_local_account *ylad;
extern HANDLE   hYahooNudge;

static void yahoo_send_msg(const char *id, const char *msg, int utf8)
{
	int buddy_icon = 0;
	LOG(("yahoo_send_msg: %s: %s, utf: %d", id, msg, utf8));
	
	buddy_icon = (YAHOO_GetDword("AvatarHash", 0) != 0) ? 2: 0;
	
	yahoo_send_im(ylad->id, NULL, id, msg, utf8, buddy_icon);
}

void ext_yahoo_got_im(int id, const char *me, const char *who, const char *msg, long tm, int stat, int utf8, int buddy_icon)
{
    char 		*umsg;
	const char	*c = msg;
	int 		oidx = 0;
	wchar_t* 	tRealBody = NULL;
	int      	tRealBodyLen = 0;
	int 		msgLen;
	char* 		tMsgBuf = NULL;
	char* 		p = NULL;
	CCSDATA 		ccs;
	PROTORECVEVENT 	pre;
	HANDLE 			hContact;

	
    LOG(("YAHOO_GOT_IM id:%s %s: %s (len: %d) tm:%lu stat:%i utf8:%i buddy_icon: %i", me, who, msg, lstrlen(msg), tm, stat, utf8, buddy_icon));
   	
	if(stat == 2) {
		char z[1024];
		
		snprintf(z, sizeof z, "Error sending message to %s", who);
		LOG((z));
		YAHOO_ShowError(Translate("Yahoo Error"), z);
		return;
	}

	if(!msg) {
		LOG(("Empty Incoming Message, exiting."));
		return;
	}

	if (YAHOO_BuddyIgnored(who)) {
		LOG(("User '%s' on our Ignore List. Dropping Message.", who));
		return;
	}
		
	umsg = (char *) alloca(lstrlen(msg) * 2 + 1); /* double size to be on the safe side */
	while ( *c != '\0') {
			// Strip the font tag
        if (!strnicmp(c,"<font ",6) || !strnicmp(c,"</font>",6) ||
			// strip the fade tag
			!strnicmp(c, "<FADE ",6) || !strnicmp(c,"</FADE>",7) ||
			// strip the alternate colors tag
			!strnicmp(c, "<ALT ",5) || !strnicmp(c, "</ALT>",6)){ 
                while ((*c++ != '>') && (*c != '\0')); 
		} else
        // strip ANSI color combination
        if ((*c == 0x1b) && (*(c+1) == '[')){ 
               while ((*c++ != 'm') && (*c != '\0')); 
		} else
		
		if (*c != '\0'){
			umsg[oidx++] = *c;
			
			/* Adding \r to \r\n conversion */
			if (*c == '\r' && *(c + 1) != '\n') 
				umsg[oidx++] = '\n';
			
			c++;
		}
	}

	umsg[oidx++]= '\0';
		
	/* Need to strip off formatting stuff first. Then do all decoding/converting */
	if (utf8){	
		//Utf8Decode( umsg, 0, &tRealBody );
		mir_utf8decode( umsg, &tRealBody );
		tRealBodyLen = wcslen( tRealBody );
	} 

	LOG(("%s: %s", who, umsg));
	
	//if(!strcmp(umsg, "<ding>")) 
	//	:P("\a");
	
	if (utf8)
		msgLen = (lstrlen(umsg) + 1) * (sizeof(wchar_t) + 1);
	else
		msgLen = (lstrlen(umsg) + 1);
	
	tMsgBuf = ( char* )alloca( msgLen );
	p = tMsgBuf;

	// MSGBUF Blob:  <ASCII> \0 <UNICODE> \0 
	strcpy( p, umsg );
	
	p += lstrlen(umsg) + 1;

	if ( tRealBodyLen != 0 ) {
		memcpy( p, tRealBody, sizeof( wchar_t )*( tRealBodyLen+1 ));
		mir_free( tRealBody );
	} 

	ccs.szProtoService = PSR_MESSAGE;
	ccs.hContact = hContact = add_buddy(who, who, PALF_TEMPORARY);
	ccs.wParam = 0;
	ccs.lParam = (LPARAM) &pre;
	pre.flags = (utf8) ? PREF_UNICODE : 0;
	
	if (tm) {
		HANDLE hEvent = (HANDLE)CallService(MS_DB_EVENT_FINDLAST, (WPARAM)hContact, 0);
	
		if (hEvent) { // contact has events
			DBEVENTINFO dbei;
			DWORD dummy;
	
			dbei.cbSize = sizeof (DBEVENTINFO);
			dbei.pBlob = (char*)&dummy;
			dbei.cbBlob = 2;
			if (!CallService(MS_DB_EVENT_GET, (WPARAM)hEvent, (LPARAM)&dbei)) 
				// got that event, if newer than ts then reset to current time
				if (tm < dbei.timestamp) tm = time(NULL);
		}

		pre.timestamp = tm;
	} else
		pre.timestamp = time(NULL);
		
	pre.szMessage = tMsgBuf;
	pre.lParam = 0;
	
    // Turn off typing
    CallService(MS_PROTO_CONTACTISTYPING, (WPARAM) hContact, PROTOTYPE_CONTACTTYPING_OFF);
	CallService(MS_PROTO_CHAINRECV, 0, (LPARAM) & ccs);

	if (buddy_icon < 0) return;
	
	//?? Don't generate floods!!
	DBWriteContactSettingByte(hContact, yahooProtocolName, "AvatarType", buddy_icon);
	if (buddy_icon != 2) {
		yahoo_reset_avatar(hContact);
	} else if (DBGetContactSettingDword(hContact, yahooProtocolName,"PictCK", 0) == 0) {
		/* request the buddy image */
		YAHOO_request_avatar(who); 
	} 
}


static void __cdecl yahoo_im_sendacksuccess(HANDLE hContact)
{
    ProtoBroadcastAck(yahooProtocolName, hContact, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
}

static void __cdecl yahoo_im_sendackfail(HANDLE hContact)
{
    SleepEx(1000, TRUE);
    ProtoBroadcastAck(yahooProtocolName, hContact, ACKTYPE_MESSAGE, ACKRESULT_FAILED, (HANDLE) 1, 
						(LPARAM) Translate("The message send timed out."));
}

static void __cdecl yahoo_im_sendackfail_longmsg(HANDLE hContact)
{
    SleepEx(1000, TRUE);
    ProtoBroadcastAck(yahooProtocolName, hContact, ACKTYPE_MESSAGE, ACKRESULT_FAILED, (HANDLE) 1, 
						(LPARAM)Translate("Message is too long: Yahoo messages are limited by 800 UTF8 chars"));
}

//=======================================================
//Send a message
//=======================================================
int YahooSendMessage(WPARAM wParam, LPARAM lParam)
{
    CCSDATA *ccs = (CCSDATA *) lParam;
    DBVARIANT dbv;
    char *msg;
	int  bANSI;
		
	bANSI = YAHOO_GetByte( "DisableUTF8", 0 );

    if (!yahooLoggedIn) {/* don't send message if we not connected! */
        mir_forkthread(yahoo_im_sendackfail, ccs->hContact);
        return 1;
    }

	if (bANSI) {
		/* convert to ANSI */
		msg = ( char* )ccs->lParam;
	} else if ( ccs->wParam & PREF_UNICODE ) {
		/* convert to utf8 */
		char* p = ( char* )ccs->lParam;
		msg = mir_utf8encodeW(( wchar_t* )&p[ strlen(p)+1 ] );
	} else {
		msg = mir_utf8encode(( char* )ccs->lParam );
	}
        
    if (lstrlen(msg) > 800) {
        mir_forkthread(	yahoo_im_sendackfail_longmsg, ccs->hContact);
        return 1;
    }

	if (!DBGetContactSetting(ccs->hContact, yahooProtocolName, YAHOO_LOGINID, &dbv)) {
		yahoo_send_msg(dbv.pszVal, msg, (!bANSI) ? 1 : 0);
		
		if (!bANSI)
			mir_free(msg);
		
		mir_forkthread(yahoo_im_sendacksuccess, ccs->hContact);
		
        DBFreeVariant(&dbv);
        return 1;
    }
    
    return 0;
}

//=======================================================
//Receive a message
//=======================================================
int YahooRecvMessage(WPARAM wParam, LPARAM lParam)
{
    DBEVENTINFO dbei;
    CCSDATA *ccs = (CCSDATA *) lParam;
    PROTORECVEVENT *pre = (PROTORECVEVENT *) ccs->lParam;

    DBDeleteContactSetting(ccs->hContact, "CList", "Hidden");

	// NUDGES
    if( !lstrcmp(pre->szMessage, "<ding>")  && ServiceExists("NUDGE/Send")){
		YAHOO_DebugLog("[YahooRecvMessage] Doing Nudge Service!");
		NotifyEventHooks(hYahooNudge, (WPARAM) ccs->hContact, pre->timestamp);
		return 0;
    } 
	
    ZeroMemory(&dbei, sizeof(dbei));
    dbei.cbSize = sizeof(dbei);
    dbei.szModule = yahooProtocolName;
    dbei.timestamp = pre->timestamp;
    dbei.flags = (pre->flags & PREF_CREATEREAD) ? DBEF_READ : 0;
    dbei.eventType = EVENTTYPE_MESSAGE;
    dbei.cbBlob = (!lstrcmp(pre->szMessage, "<ding>"))? lstrlen("BUZZ!!!")+1:lstrlen(pre->szMessage) + 1;
	if ( pre->flags & PREF_UNICODE )
		dbei.cbBlob *= ( sizeof( wchar_t )+1 );

	
    dbei.pBlob = (PBYTE) (!lstrcmp(pre->szMessage, "<ding>"))? "BUZZ!!!":pre->szMessage;
    CallService(MS_DB_EVENT_ADD, (WPARAM) ccs->hContact, (LPARAM) & dbei);
    return 0;
}

//=======================================================
//Send a nudge
//=======================================================
int YahooSendNudge(WPARAM wParam, LPARAM lParam)
{
    HANDLE hContact = (HANDLE) wParam;
    DBVARIANT dbv;
    
	YAHOO_DebugLog("[YAHOO_SENDNUDGE]");
	
    if (!yahooLoggedIn) {/* don't send nudge if we not connected! */
        mir_forkthread(yahoo_im_sendackfail, hContact);
        return 1;
    }

    if (!DBGetContactSetting(hContact, yahooProtocolName, YAHOO_LOGINID, &dbv)) {
        yahoo_send_msg(dbv.pszVal, "<ding>", 0);
        DBFreeVariant(&dbv);

        mir_forkthread(yahoo_im_sendacksuccess, hContact);
    
        return 1;
    }
    
    return 0;
}

