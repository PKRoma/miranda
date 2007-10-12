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
#include <io.h>

/*
 * Miranda headers
 */
#include "yahoo.h"
#include <m_protosvc.h>
#include <m_langpack.h>
#include <m_skin.h>
#include <m_popup.h>
#include <m_message.h>

#include "avatar.h"
#include "chat.h"
#include "webcam.h"
#include "file_transfer.h"
#include "im.h"
#include "search.h"
#include "ignore.h"

typedef struct {
	int id;
	char *label;
} yahoo_idlabel;

typedef struct {
	int id;
	char *who;
} yahoo_authorize_data;

yahoo_idlabel yahoo_status_codes[] = {
	{YAHOO_STATUS_AVAILABLE, ""},
	{YAHOO_STATUS_BRB, "BRB"},
	{YAHOO_STATUS_BUSY, "Busy"},
	{YAHOO_STATUS_NOTATHOME, "Not At Home"},
	{YAHOO_STATUS_NOTATDESK, "Not At my Desk"},
	{YAHOO_STATUS_NOTINOFFICE, "Not In The Office"},
	{YAHOO_STATUS_ONPHONE, "On The Phone"},
	{YAHOO_STATUS_ONVACATION, "On Vacation"},
	{YAHOO_STATUS_OUTTOLUNCH, "Out To Lunch"},
	{YAHOO_STATUS_STEPPEDOUT, "Stepped Out"},
	{YAHOO_STATUS_INVISIBLE, "Invisible"},
	{YAHOO_STATUS_IDLE, "Idle"},
	{YAHOO_STATUS_OFFLINE, "Offline"},
	{YAHOO_STATUS_CUSTOM, "[Custom]"},
//	{YAHOO_STATUS_NOTIFY, "Notify"},
	{0, NULL}
};

#define MAX_PREF_LEN 255

yahoo_local_account * ylad = NULL;

int do_yahoo_debug = 0;
#ifdef HTTP_GATEWAY
int iHTTPGateway = 0;
#endif
extern int poll_loop;
extern int gStartStatus;
extern char *szStartMsg;

char * yahoo_status_code(enum yahoo_status s)
{
	int i;

	for(i=0; yahoo_status_codes[i].label; i++)
		if(yahoo_status_codes[i].id == s)
			return yahoo_status_codes[i].label;
		
	return "Unknown";
}

int miranda_to_yahoo(int myyahooStatus)
{
    int ret = YAHOO_STATUS_OFFLINE;
	
    switch (myyahooStatus) {
    case ID_STATUS_ONLINE: 
                        ret = YAHOO_STATUS_AVAILABLE;
                        break;
    case ID_STATUS_NA:
                        ret = YAHOO_STATUS_BRB;
                        break;
    case ID_STATUS_OCCUPIED:
                        ret = YAHOO_STATUS_BUSY;
                        break;
    case ID_STATUS_AWAY:
                        ret = YAHOO_STATUS_NOTATDESK;
                        break;
    case ID_STATUS_ONTHEPHONE:
                        ret = YAHOO_STATUS_ONPHONE;
                        break;
    case ID_STATUS_OUTTOLUNCH:
                        ret = YAHOO_STATUS_OUTTOLUNCH;                            
                        break;
    case ID_STATUS_INVISIBLE:
                        ret = YAHOO_STATUS_INVISIBLE;
                        break;
    case ID_STATUS_DND:
                        ret = YAHOO_STATUS_STEPPEDOUT;
                        break;
    }                                                
    
    return ret;
}

void yahoo_set_status(int myyahooStatus, char *msg, int away)
{
	LOG(("yahoo_set_status myyahooStatus: %d, msg: %s, away: %d", myyahooStatus, msg, away));

	/* Safety check, don't dereference Invalid pointers */
	if (ylad->id > 0)  {
			
		if (YAHOO_CUSTOM_STATUS != myyahooStatus)
			yahoo_set_away(ylad->id, miranda_to_yahoo(myyahooStatus), msg, away);
		else
			yahoo_set_away(ylad->id, YAHOO_CUSTOM_STATUS, msg, away);
	}
}

void yahoo_stealth(const char *buddy, int add)
{
	LOG(("yahoo_stealth buddy: %s, add: %d", buddy, add));

	/* Safety check, don't dereference Invalid pointers */
	if (ylad->id > 0) {
		yahoo_set_stealth(ylad->id, buddy, add);
	}
}


int yahoo_to_miranda_status(int yahooStatus, int away)
{
    int ret = ID_STATUS_OFFLINE;
	
    switch (yahooStatus) {
    case YAHOO_STATUS_AVAILABLE: 
                        ret = ID_STATUS_ONLINE;
                        break;
    case YAHOO_STATUS_BRB:
                        ret = ID_STATUS_NA;
                        break;
    case YAHOO_STATUS_BUSY:
                        ret = ID_STATUS_OCCUPIED;
                        break;
    case YAHOO_STATUS_ONPHONE:
                        ret = ID_STATUS_ONTHEPHONE;
                        break;
    case YAHOO_STATUS_OUTTOLUNCH:
                        ret = ID_STATUS_OUTTOLUNCH;                            
                        break;
    case YAHOO_STATUS_INVISIBLE:
                        ret = ID_STATUS_INVISIBLE;
                        break;
    case YAHOO_STATUS_NOTATHOME:
                        ret = ID_STATUS_AWAY;
                        break;
    case YAHOO_STATUS_NOTATDESK:
                        ret = ID_STATUS_AWAY;
                        break;
    case YAHOO_STATUS_NOTINOFFICE:
                        ret = ID_STATUS_AWAY;
                        break;
    case YAHOO_STATUS_ONVACATION:
                        ret = ID_STATUS_AWAY;
                        break;
    case YAHOO_STATUS_STEPPEDOUT:
                        ret = ID_STATUS_DND;
                        break;
    case YAHOO_STATUS_IDLE:
                        ret = ID_STATUS_AWAY;
                        break;
    case YAHOO_STATUS_CUSTOM:
                        ret = (away ? ID_STATUS_AWAY:ID_STATUS_ONLINE);
                        break;
    }
    return ret;
}


void YAHOO_remove_buddy(const char *who)
{
    HANDLE hContact;
	DBVARIANT dbv;
	
	LOG(("YAHOO_remove_buddy '%s'", who));
	
	hContact = getbuddyH(who);

	//if ( DBGetContactSettingString( hContact, "Clist", "Group", &dbv ))
	//	return;
	if ( DBGetContactSettingString( hContact, yahooProtocolName, "YGroup", &dbv )) {
		LOG(("WARNING NO DATABASE GROUP  using 'miranda'!"));
		yahoo_remove_buddy(ylad->id, who, "miranda");
		return;
	}
	
	yahoo_remove_buddy(ylad->id, who, dbv.pszVal);
	DBFreeVariant( &dbv );
}

void YAHOO_sendtyping(const char *who, int stat)
{
	LOG(("[Yahoo_sendtyping] Sending Typing Notification to '%s', state: %d", who, stat));
	yahoo_send_typing(ylad->id, NULL, who, stat);
}

void YAHOO_accept(const char *who)
{
    yahoo_accept_buddy(ylad->id, who);
}

void YAHOO_reject(const char *who, const char *msg)
{
    yahoo_reject_buddy(ylad->id, who, msg);
}

void yahoo_logout()
{
	LOG(("[yahoo_logout]"));
	
	/*yahooLoggedIn = FALSE; 
	
	if (ylad->id <= 0) 
		return;

	yahoo_logoff(ylad->id);
	yahoo_close(ylad->id);
	
	ylad->status = YAHOO_STATUS_OFFLINE;
	ylad->id = 0;

	poll_loop=0;
	LOG(("[yahoo_logout] Logged out"));	*/
	if (yahooLoggedIn)
		yahoo_logoff(ylad->id);
	
}

HANDLE getbuddyH(const char *yahoo_id)
{
	char  *szProto;
	HANDLE hContact;
		
	for ( hContact = ( HANDLE )YAHOO_CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
		   hContact != NULL;
			hContact = ( HANDLE )YAHOO_CallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM )hContact, 0 ))
	{
		szProto = ( char* )YAHOO_CallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM )hContact, 0 );
		if ( szProto != NULL && !lstrcmp( szProto, yahooProtocolName ))
		{
			DBVARIANT dbv;
			if ( DBGetContactSettingString( hContact, yahooProtocolName, YAHOO_LOGINID, &dbv ))
				continue;

			{	
                int tCompareResult = lstrcmpi( dbv.pszVal, yahoo_id );
				DBFreeVariant( &dbv );
				if ( tCompareResult )
					continue;
			}

			return hContact;
			}	
    }

	return NULL;
}

HANDLE add_buddy( const char *yahoo_id, const char *yahoo_name, DWORD flags )
{
	HANDLE hContact;
	
	hContact = getbuddyH(yahoo_id);
	if (hContact != NULL) {
		LOG(("[add_buddy] Found buddy id: %s, handle: %lu", yahoo_id, (DWORD)hContact));
		if ( !( flags & PALF_TEMPORARY ) && DBGetContactSettingByte( hContact, "CList", "NotOnList", 1 )) 
		{
			LOG(("[add_buddy] Making Perm id: %s, flags: %lu", yahoo_id, flags));
			DBDeleteContactSetting( hContact, "CList", "NotOnList" );
			DBDeleteContactSetting( hContact, "CList", "Hidden" );

        }
		return hContact;
    }

	//not already there: add
	LOG(("[add_buddy] Adding buddy id: %s, flags: %lu", yahoo_id, flags));
	hContact = ( HANDLE )YAHOO_CallService( MS_DB_CONTACT_ADD, 0, 0 );
	YAHOO_CallService( MS_PROTO_ADDTOCONTACT, ( WPARAM )hContact,( LPARAM )yahooProtocolName );
	YAHOO_SetString( hContact, YAHOO_LOGINID, yahoo_id );
	if (lstrlen(yahoo_name) > 0)
		YAHOO_SetStringUtf( hContact, "Nick", yahoo_name );
	else
	    YAHOO_SetStringUtf( hContact, "Nick", yahoo_id );
	    
	//DBWriteContactSettingWord(hContact, yahooProtocolName, "Status", ID_STATUS_OFFLINE);

   	if (flags & PALF_TEMPORARY ) {
		DBWriteContactSettingByte( hContact, "CList", "NotOnList", 1 );
		DBWriteContactSettingByte( hContact, "CList", "Hidden", 1 );
    }	
	return hContact;
}

const char *find_buddy( const char *yahoo_id)
{
	static char nick[128];
	HANDLE hContact;
	DBVARIANT dbv;
	
	hContact = getbuddyH(yahoo_id);
	if (hContact != NULL) {
		if ( DBGetContactSettingString( hContact, yahooProtocolName, "Nick", &dbv ))
			return NULL;
	
		strncpy(nick, dbv.pszVal, 128);
		DBFreeVariant( &dbv );
		return nick;
		
    }

	return NULL;
}

/* Required handlers bellow */

/* Other handlers */
void ext_yahoo_status_changed(int id, const char *who, int stat, const char *msg, int away, int idle, int mobile)
{
	HANDLE 	hContact = 0;
	time_t  idlets = 0;
	
	YAHOO_DebugLog("[ext_yahoo_status_changed] %s with msg %s (stat: %d, away: %d, idle: %d seconds)", who, msg, stat, away, idle);
	
	hContact = getbuddyH(who);
	if (hContact == NULL) {
		YAHOO_DebugLog("Buddy Not Found. Adding...");
		hContact = add_buddy(who, who, 0);
/*	} else {
		YAHOO_DebugLog("Buddy Found On My List! Buddy %p", hContact);*/
	}
	
	if (!mobile)
		YAHOO_SetWord(hContact, "Status", yahoo_to_miranda_status(stat,away));
	else
		YAHOO_SetWord(hContact, "Status", ID_STATUS_ONTHEPHONE);
	
	YAHOO_SetWord(hContact, "YStatus", stat);
	YAHOO_SetWord(hContact, "YAway", away);
	YAHOO_SetWord(hContact, "Mobile", mobile);

	if(msg) {
		YAHOO_DebugLog("[ext_yahoo_status_changed] %s custom message '%s'", who, msg);
		//YAHOO_SetString(hContact, "YMsg", msg);
		//DBWriteContactSettingString( hContact, "CList", "StatusMsg", msg);
		//DBWriteContactSettingStringUtf( hContact, "CList", "StatusMsg", msg);
		DBWriteContactSettingString( hContact, "CList", "StatusMsg", msg);
	} else {
		DBDeleteContactSetting(hContact, "CList", "StatusMsg" );
	}

	if ( (away == 2) || (stat == YAHOO_STATUS_IDLE) || (idle > 0)) {
		if (stat > 0) {
			YAHOO_DebugLog("[ext_yahoo_status_changed] %s idle for %d:%02d:%02d", who, idle/3600, (idle/60)%60, idle%60);
			
			time(&idlets);
			idlets -= idle;
		}
	} 
		
	DBWriteContactSettingDword(hContact, yahooProtocolName, "IdleTS", idlets);

	YAHOO_DebugLog("[ext_yahoo_status_changed] exiting");
}

void ext_yahoo_status_logon(int id, const char *who, int stat, const char *msg, int away, int idle, int mobile, int cksum, int buddy_icon, long client_version)
{
	HANDLE 	hContact = 0;
	char 	*s = NULL;
	
	YAHOO_DebugLog("[ext_yahoo_status_logon] %s with msg %s (stat: %d, away: %d, idle: %d seconds, checksum: %d buddy_icon: %d client_version: %ld)", who, msg, stat, away, idle, cksum, buddy_icon, client_version);
	
	ext_yahoo_status_changed(id, who, stat, msg, away, idle, mobile);
	hContact = getbuddyH(who);
	if (hContact == NULL) {
		YAHOO_DebugLog("[ext_yahoo_status_logon] Can't find handle for %s??? PANIC!!!", who);
		return;
	} 
	
	switch (client_version) {
		case 262651: 
				s = "libyahoo2"; 
				break;
		case 262655: 
				s = "< Yahoo 6.x (Yahoo 5.x?)"; 
				break;
		case 278527: 
				s = "Yahoo 6.x"; 
				break;
		case 524223: 
				s = "Yahoo 7.x"; 
				break;
		case 822543:  /* ? "Yahoo Version 3.0 beta 1 (build 18274) OSX" */
		case 1572799: /* 8.0.x ??  */ 
		case 2097087: /* 8.1.0.195 */ 
				s = "Yahoo 8.x"; 
				break;
	}
	
	if (s != NULL) 
		DBWriteContactSettingString( hContact, yahooProtocolName, "MirVer", s);
		
	/* Last thing check the checksum and request new one if we need to */
	if (buddy_icon == -1) {
		YAHOO_DebugLog("[ext_yahoo_status_logon] No avatar information in this packet? Not touching stuff!");
	} else {
		// we got some avatartype info
		DBWriteContactSettingByte(hContact, yahooProtocolName, "AvatarType", buddy_icon);
		
		if (cksum == 0 || cksum == -1){
			// no avatar
			DBWriteContactSettingDword(hContact, yahooProtocolName, "PictCK", 0);
		} else if (DBGetContactSettingDword(hContact, yahooProtocolName,"PictCK", 0) != cksum) {
			//char szFile[MAX_PATH];
			
			// Save new Checksum
			DBWriteContactSettingDword(hContact, yahooProtocolName, "PictCK", cksum);
			
			// Need to delete the Avatar File!!
			//GetAvatarFileName(hContact, szFile, sizeof szFile, 0);
			//DeleteFile(szFile);
		}
	
		// Cleanup the type? and reset things...
		yahoo_reset_avatar(hContact);
	}
	
	YAHOO_DebugLog("[ext_yahoo_status_logon] exiting");
}

void ext_yahoo_got_audible(int id, const char *me, const char *who, const char *aud, const char *msg, const char *aud_hash)
{
	HANDLE 	hContact = 0;
	char	z[1028];
	
	/* aud = file class name 
					GAIM: the audible, in foo.bar.baz format
			
					Actually this is the filename.
					Full URL:
				
					http://us.dl1.yimg.com/download.yahoo.com/dl/aud/us/aud.swf 
					
					where aud in foo.bar.baz format
	*/
	
	LOG(("ext_yahoo_got_audible for %s aud: %s msg:'%s' hash: %s", who, aud, msg, aud_hash));

	hContact = getbuddyH(who);
	if (hContact == NULL) {
		LOG(("Buddy Not Found. Skipping avatar update"));
		return;
	}
	
	mir_snprintf(z, sizeof(z), "[miranda-audible] %s", msg ?msg:"");
	ext_yahoo_got_im(id, (char*)me, (char*)who, z, 0, 0, 1, -1);
}

void ext_yahoo_got_calendar(int id, const char *url, int type, const char *msg, int svc)
{
	LOG(("[ext_yahoo_got_calendar] URL:%s type: %d msg: %s svc: %d", url, type, msg, svc));
	
	if (!YAHOO_ShowPopup( Translate("Calendar Reminder"), msg, url))
		YAHOO_shownotification(Translate("Calendar Reminder"), msg, NIIF_INFO);
}

void ext_yahoo_got_picture_upload(int id, const char *me, const char *url,unsigned int ts)
{
	int cksum = 0;
	DBVARIANT dbv;	
	
	LOG(("[ext_yahoo_got_picture_upload] url: %s timestamp: %d", url, ts));

	if (!url) {
		LOG(("[ext_yahoo_got_picture_upload] Problem with upload?"));
		return;
	}
	
	
	cksum = YAHOO_GetDword("TMPAvatarHash", 0);
	if (cksum != 0) {
		LOG(("[ext_yahoo_got_picture_upload] Updating Checksum to: %d", cksum));
		YAHOO_SetDword("AvatarHash", cksum);
		DBDeleteContactSetting(NULL, yahooProtocolName, "TMPAvatarHash");
		
		// This is only meant for message sessions, but we don't got those in miranda yet
		YAHOO_bcast_picture_checksum(cksum);
		// need to tell the stupid Yahoo that our icon updated
		YAHOO_bcast_picture_update(2);
	}else
		cksum = YAHOO_GetDword("AvatarHash", 0);
		
	YAHOO_SetString(NULL, "AvatarURL", url);
	//YAHOO_SetDword("AvatarExpires", ts);

	if  (!DBGetContactSettingString(NULL, yahooProtocolName, "AvatarInv", &dbv) ){
		LOG(("[ext_yahoo_got_picture_upload] Buddy: %s told us this is bad??", dbv.pszVal));

		LOG(("[ext_yahoo_got_picture] Sending url: %s checksum: %d to '%s'!", url, cksum, dbv.pszVal));
		//void yahoo_send_picture_info(int id, const char *me, const char *who, const char *pic_url, int cksum)
		yahoo_send_picture_info(id, dbv.pszVal, 2, url, cksum);

		DBDeleteContactSetting(NULL, yahooProtocolName, "AvatarInv");
		DBFreeVariant(&dbv);
	}
}

void ext_yahoo_got_avatar_share(int id, int buddy_icon)
{
	LOG(("[ext_yahoo_got_avatar_share] buddy icon: %d", buddy_icon));
	
	YAHOO_SetByte( "ShareAvatar", buddy_icon );
}

void ext_yahoo_got_stealth(int id, char *stealthlist)
{
	char **s;
	int found = 0;
	char **stealth = NULL;
	char  *szProto;
	HANDLE hContact;

	LOG(("[ext_yahoo_got_stealth] list: %s", stealthlist));
	
	if (stealthlist)
		stealth = y_strsplit(stealthlist, ",", -1);
	

	for ( hContact = ( HANDLE )YAHOO_CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
		   hContact != NULL;
			hContact = ( HANDLE )YAHOO_CallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM )hContact, 0 ))
	{
		szProto = ( char* )YAHOO_CallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM )hContact, 0 );
		if ( szProto != NULL && !lstrcmp( szProto, yahooProtocolName )) {
			DBVARIANT dbv;
			if ( DBGetContactSettingString( hContact, yahooProtocolName, YAHOO_LOGINID, &dbv ))
				continue;

			found = 0;
			
			for(s = stealth; s && *s; s++) {
				
				if (lstrcmpi(*s, dbv.pszVal) == 0) {
					YAHOO_DebugLog("GOT id = %s", dbv.pszVal);
					found = 1;
					break;
				}
			}
			
			/* Check the stealth list */
			if (found) { /* we have him on our Stealth List */
				YAHOO_DebugLog("Setting STEALTH for id = %s", dbv.pszVal);
				/* need to set the ApparentMode thingy */
				if (ID_STATUS_OFFLINE != DBGetContactSettingWord(hContact, yahooProtocolName, "ApparentMode", 0))
					DBWriteContactSettingWord(hContact, yahooProtocolName, "ApparentMode", (WORD) ID_STATUS_OFFLINE);
				
			} else { /* he is not on the Stealth List */
				//LOG(("Resetting STEALTH for id = %s", dbv.pszVal));
				/* need to delete the ApparentMode thingy */
				if (DBGetContactSettingWord(hContact, yahooProtocolName, "ApparentMode", 0))
					DBDeleteContactSetting(hContact, yahooProtocolName, "ApparentMode");
			}
				
			DBFreeVariant( &dbv );
		}		
    }
}


void ext_yahoo_got_buddies(int id, YList * buds)
{
    LOG(("ext_yahoo_got_buddies"));
    
	if (buds == NULL) {
		LOG(("No Buddies to work on!"));
		return;
	}
	
	LOG(("Walking buddy list..."));
	for(; buds; buds = buds->next) {
	    HANDLE hContact;
	    
		struct yahoo_buddy *bud = buds->data;
		if (bud == NULL) {
			LOG(("EMPTY BUDDY LIST??"));
			continue;
		}
		
		if (bud->real_name)
			YAHOO_DebugLog("id = %s name = %s", bud->id, bud->real_name);
		
		hContact = getbuddyH(bud->id);
		if (hContact == NULL)
			hContact = add_buddy(bud->id, (bud->real_name != NULL) ? bud->real_name : bud->id, 0);
		
		if (bud->group)
			YAHOO_SetString( hContact, "YGroup", bud->group);
		
		if (bud->yab_entry) {
		  //LOG(("YAB_ENTRY"));
		  
		  if (bud->yab_entry->fname) 
		    YAHOO_SetStringUtf( hContact, "FirstName", bud->yab_entry->fname);
		  
		  
		  if (bud->yab_entry->lname) 
		      YAHOO_SetStringUtf( hContact, "LastName", bud->yab_entry->lname);
		  
		  
		  if (bud->yab_entry->nname) 
		      YAHOO_SetStringUtf( hContact, "Nick", bud->yab_entry->nname);
		  
		  
		  if (bud->yab_entry->email) 
			YAHOO_SetString( hContact, "e-mail", bud->yab_entry->email);
		  
		  if (bud->yab_entry->mphone) 
			YAHOO_SetString( hContact, "Cellular", bud->yab_entry->mphone);

		  if (bud->yab_entry->hphone) 
			YAHOO_SetString( hContact, "Phone", bud->yab_entry->hphone);
		  
  		  if (bud->yab_entry->wphone) 
			YAHOO_SetString( hContact, "CompanyPhone", bud->yab_entry->wphone);
		  
		  YAHOO_SetWord( hContact, "YabID", bud->yab_entry->dbid);

		}
		
		
	}

}

void ext_yahoo_rejected(int id, const char *who, const char *msg)
{
   	char buff[1024];
   	HANDLE hContact;
	
    LOG(("[ext_yahoo_rejected] who: %s  msg: %s", who, msg));
	
	hContact = getbuddyH(who);
	
	if (hContact != NULL) {
		/*
		 * Make sure the contact is temporary so we could delete it w/o extra traffic
		 */ 
		DBWriteContactSettingByte( hContact, "CList", "NotOnList", 1 );
		YAHOO_CallService( MS_DB_CONTACT_DELETE, (WPARAM) hContact, 0);	
	} else {
		LOG(("[ext_yahoo_rejected] Buddy not on our buddy list"));
	}
    
	mir_snprintf(buff, sizeof(buff), Translate("%s has rejected your request and sent the following message:"), who);    
    MessageBox( NULL, msg, buff, MB_OK | MB_ICONINFORMATION );
}

void YAHOO_add_buddy(const char *who, const char *group, const char *msg)
{
	/* We adding a buddy to our list.
	  2 Stages.
	1. We send add buddy packet.
	2. We get a packet back from the server confirming add.
	
	No refresh needed. */
    yahoo_add_buddy(ylad->id, who, group, msg);
}

void ext_yahoo_buddy_added(int id, char *myid, char *who, char *group, int status, int auth)
{
	LOG(("[ext_yahoo_buddy_added] %s authorized you as %s group: %s status: %d auth: %d", who, myid, group, status, auth));
	
}

void ext_yahoo_buddy_group_changed(int id, char *myid, char *who, char *old_group, char *new_group)
{
	LOG(("[ext_yahoo_buddy_group_changed] %s has been moved from group: %s to: %s", who, old_group, new_group));
}

void ext_yahoo_contact_added(int id, char *myid, char *who, char *fname, char *lname, char *msg)
{
	char		*szBlob, *pCurBlob, nick[128];
	HANDLE		hContact = NULL;
	CCSDATA 	ccs;
	PROTORECVEVENT pre;

	/* NOTE: Msg is actually in UTF8 unless stated otherwise!! */
    LOG(("[ext_yahoo_contact_added] %s %s (%s) added you as %s w/ msg '%s'", fname, lname, who, myid, msg));
    
	if (YAHOO_BuddyIgnored(who)) {
		LOG(("User '%s' on our Ignore List. Dropping Authorization Request.", who));
		return;
	}
	
	if (fname == NULL && lname == NULL) {
		mir_snprintf(nick, sizeof(nick), who);
	} else {
		mir_snprintf(nick, sizeof(nick), "%s %s", fname, lname);
	}
	
	hContact = add_buddy(who, nick, PALF_TEMPORARY);
	
	ccs.szProtoService	= PSR_AUTH;
	ccs.hContact		= hContact;
	ccs.wParam			= 0;
	ccs.lParam			= (LPARAM) &pre;
	pre.flags			= 0;
	pre.timestamp		= time(NULL);
	
	pre.lParam = sizeof(DWORD)*2+lstrlen(who)+lstrlen(nick)+5;
	
	if (fname != NULL)
		pre.lParam += lstrlen(fname);
	
	if (lname != NULL)
		pre.lParam += lstrlen(lname);
	
	if (msg != NULL)
		pre.lParam += lstrlen(msg);
	
	pCurBlob=szBlob=(char *)malloc(pre.lParam);
    /*
       Auth blob is: uin(DWORD),hcontact(HANDLE),nick(ASCIIZ),first(ASCIIZ),
                  last(ASCIIZ),email(ASCIIZ),reason(ASCIIZ)

	  blob is: 0(DWORD), hContact(HANDLE), nick(ASCIIZ), fname (ASCIIZ), lname (ASCIIZ), email(ASCIIZ), msg(ASCIIZ)

    */

	// UIN
    *( PDWORD )pCurBlob = 0;
    pCurBlob+=sizeof(DWORD);

    // hContact
	*( PDWORD )pCurBlob = ( DWORD )hContact; 
    pCurBlob+=sizeof(DWORD);
    
    // NICK
	lstrcpy((char *)pCurBlob, nick); 

	pCurBlob+=lstrlen((char *)pCurBlob)+1;
    
    // FIRST
    lstrcpy((char *)pCurBlob, (fname != NULL) ? fname : ""); 
    pCurBlob+=lstrlen((char *)pCurBlob)+1;
    
    // LAST
    lstrcpy((char *)pCurBlob, (lname != NULL) ? lname : ""); 
    pCurBlob+=lstrlen((char *)pCurBlob)+1;
    
    // E-mail    
	lstrcpy((char *)pCurBlob,who); 
	pCurBlob+=lstrlen((char *)pCurBlob)+1;
	
	// Reason
	lstrcpy((char *)pCurBlob, (msg != NULL) ? msg : "" ); 
	
	pre.szMessage = (char *)szBlob;
	
	CallService(MS_PROTO_CHAINRECV,0,(LPARAM)&ccs);
}

void ext_yahoo_typing_notify(int id, const char *me, const char *who, int stat)
{
    const char *c;
    HANDLE hContact;
    
    LOG(("[ext_yahoo_typing_notify] me: '%s' who: '%s' stat: %d ", me, who, stat));

	hContact = getbuddyH(who);
	if (!hContact) return;
	
	c = YAHOO_GetContactName(hContact);
	
	CallService(MS_PROTO_CONTACTISTYPING, (WPARAM)hContact, (LPARAM)stat?10:0);
}

void ext_yahoo_game_notify(int id, const char *me, const char *who, int stat, const char *msg)
{
	HANDLE hContact;
	
	/* There's also game invite packet:
	[17:36:44 YAHOO] libyahoo2/libyahoo2.c:3093: debug: 
[17:36:44 YAHOO] Yahoo Service: (null) (0xb7) Status: YAHOO_STATUS_BRB (1)
[17:36:44 YAHOO]  
[17:36:44 YAHOO] libyahoo2/libyahoo2.c:863: debug: 
[17:36:44 YAHOO] [Reading packet] len: 88
[17:36:44 YAHOO]  
[17:36:44 YAHOO] Key: From (4)  	Value: 'xxxxx'
[17:36:44 YAHOO]  
[17:36:44 YAHOO] Key: To (5)  	Value: 'zxzxxx'
[17:36:44 YAHOO]  
[17:36:44 YAHOO] Key: (null) (180)  	Value: 'pl'
[17:36:44 YAHOO]  
[17:36:44 YAHOO] Key: (null) (183)  	Value: ''
[17:36:44 YAHOO]  
[17:36:44 YAHOO] Key: (null) (181)  	Value: ''
[17:36:44 YAHOO]  
[17:36:44 YAHOO] Key: session (11)  	Value: 'o8114ik_lixyxtdfrxbogw--'
[17:36:44 YAHOO]  
[17:36:44 YAHOO] Key: stat/location (13)  	Value: '1'
[17:36:44 YAHOO]  
[17:36:44 YAHOO] libyahoo2/libyahoo2.c:908: debug: 
[17:36:44 YAHOO] [Reading packet done]

	 */
    LOG(("[ext_yahoo_game_notify] id: %d, me: %s, who: %s, stat: %d, msg: %s", id, me, who, stat, msg));
    /* FIXME - Not Implemented - this informs you someone else is playing on Yahoo! Games */
    /* Also Stubbed in Sample Client */
	hContact = getbuddyH(who);
	if (!hContact) return;
	
	if (stat == 2) 
		YAHOO_SetString(hContact, "YGMsg", "");
	else if (msg) {
		const char *l = msg, *u = NULL;
		char *z, *c;
		int i = 0;
		
		/* Parse and Set a custom Message 
		 *
		 * Format: 1 [09] ygamesp [09] 1 [09] 0 [09] ante?room=yahoo_1078798506&follow=rrrrrrr	
		 * [09] Yahoo! Poker\nRoom: Intermediate Lounge 2
		 *
		 * Sign-in:
		 * [17:13:42 YAHOO] [ext_yahoo_game_notify] id: 1, me: xxxxx, who: rrrrrrr, 
		 * stat: 1, msg: 1	ygamesa	1	0	ante?room=yahoo_1043183792&follow=lotesdelere	
		 * Yahoo! Backgammon Room: Social Lounge 12 
		 *
		 * Sign-out:
		 * [17:18:38 YAHOO] [ext_yahoo_game_notify] id: 1, me: xxxxx, who: rrrrr, 
		 *	stat: 2, msg: 1	ygamesa	2
		 */
		z = (char *) _alloca(lstrlen(l) + 50);
		
		z[0]='\0';
		do{
			c = strchr(l, 0x09);
			i++;
			if (c != NULL) {
				l = c;
				l++;
				if (i == 4)
					u = l;
			}
		} while (c != NULL && i < 5);
		
		if (c != NULL) {
			// insert \r before \n
			do{
				c =	strchr(l, '\n');
				
				if (c != NULL) {
					(*c) = '\0';
					lstrcat(z, l);
					lstrcat(z, "\r\n");
					l = c + 1;
				} else {
					lstrcat(z, l);
				}
			} while (c != NULL);
			
			lstrcat(z, "\r\n\r\nhttp://games.yahoo.com/games/");
			lstrcat(z, u);
			c = strchr(z, 0x09);
			(*c) = '\0';
		}
		
		YAHOO_SetString(hContact, "YGMsg", z);
		
	} else {
		/* ? no information / reset custom message */
		YAHOO_SetString(hContact, "YGMsg", "");
	}
}

int mUnreadMessages;

void ext_yahoo_mail_notify(int id, const char *from, const char *subj, int cnt)
{
	LOG(("[ext_yahoo_mail_notify] from: %s subject: %s count: %d", from, subj, cnt));
	
	if (cnt > 0) {
		SkinPlaySound( Translate( "mail" ) );
	
		if (!YAHOO_GetByte( "DisableYahoomail", 0)) {    
			char z[MAX_SECONDLINE], title[MAX_CONTACTNAME];
			
			LOG(("ext_yahoo_mail_notify"));
		
			if (from == NULL) {
				snprintf(title, sizeof(title), "%s: %s", yahooProtocolName, Translate("New Mail"));
				snprintf(z, sizeof(z), Translate("You Have %i unread msgs"), cnt);
			} else {
				snprintf(title, sizeof(title), Translate("New Mail (%i msgs)"), cnt);
				snprintf(z, sizeof(z), Translate("From: %s\nSubject: %s"), from, subj);
			}
	
			if(!YAHOO_ShowPopup( title, z, "http://mail.yahoo.com" ))
				YAHOO_shownotification(title, z, NIIF_INFO);
		}
	}
	
	mUnreadMessages = cnt;
	YAHOO_SendBroadcast( NULL, ACKTYPE_EMAIL, ACKRESULT_STATUS, NULL, 0 );
}    
    
void ext_yahoo_system_message(int id, const char *me, const char *who, const char *msg)
{
	LOG(("Yahoo System Message to: %s from: %s msg: %s", me, who, msg));
	
	if (strncmp(msg, "A user on Windows Live", lstrlen("A user on Windows Live")) != 0
		&& strncmp(msg, "Your contact is using Windows Live", lstrlen("Your contact is using Windows Live")) != 0)
		YAHOO_ShowPopup( (who != NULL) ? who : "Yahoo System Message", msg, NULL);
}

void ext_yahoo_got_identities(int id, YList * ids)
{
    LOG(("ext_yahoo_got_identities"));
    /* FIXME - Not implemented - Got list of Yahoo! identities */
    /* We currently only use the default identity */
    /* Also Stubbed in Sample Client */
	
}

void __cdecl yahoo_get_yab_thread(void *psf) 
{
	int id = (int)psf;
	
	yahoo_get_yab(id);
}

char * getcookie(char *rawcookie);

void check_for_update(void)
{
    NETLIBHTTPREQUEST nlhr={0},*nlhrReply;
	NETLIBHTTPHEADER httpHeaders[3];
	
	nlhr.cbSize=sizeof(nlhr);
	nlhr.requestType=REQUEST_GET;
	nlhr.flags=NLHRF_DUMPASTEXT|NLHRF_GENERATEHOST|NLHRF_SMARTAUTHHEADER|NLHRF_HTTP11;
	nlhr.szUrl="http://update.messenger.yahoo.com/msgrcli7.html";//url.sz;
	nlhr.headers = httpHeaders;
	nlhr.headersCount= 3;
	
	httpHeaders[0].szName="Accept";
	httpHeaders[0].szValue="*/*";
	httpHeaders[1].szName="User-Agent";
	httpHeaders[1].szValue="Mozilla Compatible/2.0 (WinNT; I; NCC/2.0)";
	httpHeaders[2].szName="Pragma";
	httpHeaders[2].szValue="no-cache";

	nlhrReply=(NETLIBHTTPREQUEST*)CallService(MS_NETLIB_HTTPTRANSACTION,(WPARAM)hNetlibUser,(LPARAM)&nlhr);

	if(nlhrReply) {
		int i;
		
		if (nlhrReply->resultCode != 200) {
			LOG(("Update server returned '%d' instead of 200. It also sent the following: %s", nlhrReply->resultCode, nlhrReply->szResultDescr));
			return;
		}
		
		LOG(("Got %d headers!", nlhrReply->headersCount));
		
		for (i=0; i < nlhrReply->headersCount; i++) {
			LOG(("%s: %s", nlhrReply->headers[i].szName, nlhrReply->headers[i].szValue));
			
			if (lstrcmpi(nlhrReply->headers[i].szName, "Set-Cookie") == 0) {
				LOG(("Found Cookie... Yum yum..."));
				
				if (nlhrReply->headers[i].szValue[0] == 'B' && nlhrReply->headers[i].szValue[1] == '=') {
					char *b;
					
					b = getcookie(nlhrReply->headers[i].szValue);
					
					LOG(("Got B Cookie: %s", b));
				}
			}
		}
		CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT,0,(LPARAM)nlhrReply);
	}
}
void ext_yahoo_got_cookies(int id)
{
//    char z[1024];

    LOG(("ext_yahoo_got_cookies "));
/*    LOG(("Y Cookie: '%s'", yahoo_get_cookie(id, "y")));
    LOG(("T Cookie: '%s'", yahoo_get_cookie(id, "t")));
    LOG(("C Cookie: '%s'", yahoo_get_cookie(id, "c")));
    LOG(("Login Cookie: '%s'", yahoo_get_cookie(id, "login")));
    
    //wsprintf(z, "Cookie: %s; C=%s; Y=%s; T=%s", Bcookie, yahoo_get_cookie(id, "c"), yahoo_get_cookie(id, "y"), yahoo_get_cookie(id, "t"));
    //wsprintf(z, "Cookie: %s; Y=%s", Bcookie, yahoo_get_cookie(id, "y"), yahoo_get_cookie(id, "t"));    
    wsprintf(z, "Cookie: Y=%s; T=%s", yahoo_get_cookie(id, "y"), yahoo_get_cookie(id, "t"));    
    LOG(("Our Cookie: '%s'", z));
    YAHOO_CallService(MS_NETLIB_SETSTICKYHEADERS, (WPARAM)hnuMain, (LPARAM)z);*/

#ifdef HTTP_GATEWAY	
	if (iHTTPGateway) {
		char z[1024];
		
		// need to add Cookie header to our requests or we get booted w/ "Bad Cookie" message.
		mir_snprintf(z, sizeof(z), "Cookie: Y=%s; T=%s; C=%s", yahoo_get_cookie(id, "y"), 
				yahoo_get_cookie(id, "t"), yahoo_get_cookie(id, "c"));    
		LOG(("Our Cookie: '%s'", z));
		YAHOO_CallService(MS_NETLIB_SETSTICKYHEADERS, (WPARAM)hNetlibUser, (LPARAM)z);
	}
#endif
	
	/*if (YAHOO_GetByte( "UseYAB", 1 )) {
		LOG(("GET YAB [Before final check] "));
		if (yahooStatus != ID_STATUS_OFFLINE)
			//yahoo_get_yab(id);
			check_for_update();
			mir_forkthread(yahoo_get_yab_thread, (void *)id);
	}*/
}

void ext_yahoo_got_ping(int id, const char *errormsg)
{
    LOG(("[ext_yahoo_got_ping]"));
	
	if (errormsg) {
			LOG(("[ext_yahoo_got_ping] Error msg: %s", errormsg));
			YAHOO_ShowError(Translate("Yahoo Ping Error"), errormsg);
			return;
	}
    
	LOG(("[ext_yahoo_got_ping] Status Check current: %d, CONNECTING: %d ", yahooStatus, ID_STATUS_CONNECTING));
	
	if (yahooStatus == ID_STATUS_CONNECTING) {
		LOG(("[ext_yahoo_got_ping] We are connecting. Checking for different status. Start: %d, Current: %d", gStartStatus, yahooStatus));
		if (gStartStatus != yahooStatus) {
			LOG(("[COOKIES] Updating Status to %d ", gStartStatus));    
			
			if (gStartStatus != ID_STATUS_INVISIBLE) {// don't generate a bogus packet for Invisible state
				if (szStartMsg != NULL) {
					yahoo_set_status(YAHOO_STATUS_CUSTOM, szStartMsg, (gStartStatus != ID_STATUS_ONLINE) ? 1 : 0);
				} else
				    yahoo_set_status(gStartStatus, NULL, (gStartStatus != ID_STATUS_ONLINE) ? 1 : 0);
			}
			
			yahoo_util_broadcaststatus(gStartStatus);
			yahooLoggedIn=TRUE;
		}
	}
}

void ext_yahoo_login_response(int id, int succ, const char *url)
{
	char buff[1024];

	LOG(("[ext_yahoo_login_response] succ: %d, url: %s", succ, url));
	
	if(succ == YAHOO_LOGIN_OK) {
		ylad->status = yahoo_current_status(id);
		LOG(("logged in status-> %d", ylad->status));
		
		if (YAHOO_GetByte( "UseYAB", 1 )) {
			LOG(("GET YAB [Before final check] "));
			if (yahooStatus != ID_STATUS_OFFLINE)
				mir_forkthread(yahoo_get_yab_thread, (void *)id);
		}

		return;
	} else if(succ == YAHOO_LOGIN_UNAME) {

		snprintf(buff, sizeof(buff), Translate("Could not log into Yahoo service - username not recognised.  Please verify that your username is correctly typed."));
		ProtoBroadcastAck(yahooProtocolName, NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_BADUSERID);
	} else if(succ == YAHOO_LOGIN_PASSWD) {

		snprintf(buff, sizeof(buff), Translate("Could not log into Yahoo service - password incorrect.  Please verify that your username and password are correctly typed."));
		ProtoBroadcastAck(yahooProtocolName, NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_WRONGPASSWORD);
	} else if(succ == YAHOO_LOGIN_LOCK) {
		
		snprintf(buff, sizeof(buff), Translate("Could not log into Yahoo service.  Your account has been locked.\nVisit %s to reactivate it."), url);

	} else if(succ == YAHOO_LOGIN_DUPL) {
		snprintf(buff, sizeof(buff), Translate("You have been logged out of the yahoo service, possibly due to a duplicate login."));
		ProtoBroadcastAck(yahooProtocolName, NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_OTHERLOCATION);
	}else if(succ == YAHOO_LOGIN_LOGOFF) {
		//snprintf(buff, sizeof(buff), Translate("You have been logged out of the yahoo service."));
		//ProtoBroadcastAck(yahooProtocolName, NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_OTHERLOCATION);
		return; // we logged out.. so just sign-off..
	}else if(succ == -1) {
		/// Can't Connect or got disconnected.
		if (yahooStatus == ID_STATUS_CONNECTING)
			snprintf(buff, sizeof(buff), Translate("Could not connect to the Yahoo service. Check your server/port and proxy settings."));	
		else
			return;
	} else {
		snprintf(buff, sizeof(buff),Translate("Could not log in, unknown reason: %d."), succ);
	}

	YAHOO_DebugLog("ERROR: %s", buff);
	
	/*
	 * Show Error Message
	 */
	YAHOO_ShowError(Translate("Yahoo Login Error"), buff);
}

void ext_yahoo_error(int id, const char *err, int fatal, int num)
{
	char buff[1024];
	
	LOG(("Yahoo Error: id: %d, fatal: %d, num: %d, %s", id, fatal, num, err));
        
	switch(num) {
		case E_UNKNOWN:
			snprintf(buff, sizeof(buff), Translate("Unknown error %s"), err);
			break;
		case E_CUSTOM:
			snprintf(buff, sizeof(buff), Translate("Custom error %s"), err);
			break;
		case E_CONFNOTAVAIL:
			snprintf(buff, sizeof(buff), Translate("%s is not available for the conference"), err);
			break;
		case E_IGNOREDUP:
			snprintf(buff, sizeof(buff), Translate("%s is already ignored"), err);
			break;
		case E_IGNORENONE:
			snprintf(buff, sizeof(buff), Translate("%s is not in the ignore list"), err);
			break;
		case E_IGNORECONF:
			snprintf(buff, sizeof(buff), Translate("%s is in buddy list - cannot ignore "), err);
			break;
		case E_SYSTEM:
			snprintf(buff, sizeof(buff), Translate("System Error: %s"), err);
			break;
		case E_CONNECTION:
			snprintf(buff, sizeof(buff), Translate("Server Connection Error: %s"), err);
			YAHOO_DebugLog("Error: %s", buff);
			return;
	}
	
	YAHOO_DebugLog("Error: %s", buff);
	
	/*
	 * Show Error Message
	 */
	YAHOO_ShowError(Translate("Yahoo Error"), buff);
}

int ext_yahoo_connect(const char *h, int p, int type)
{
	NETLIBOPENCONNECTION ncon = {0};
    HANDLE con;
    
	LOG(("[ext_yahoo_connect] %s:%d type: %d", h, p, type));
	
    ncon.cbSize = sizeof(ncon); 
	ncon.szHost = h;
    ncon.wPort = p;
    	
	if (type != YAHOO_CONNECTION_PAGER)
		ncon.flags = NLOCF_HTTP;
	
	
    con = (HANDLE) CallService(MS_NETLIB_OPENCONNECTION, (WPARAM) hNetlibUser, (LPARAM) & ncon);
    if (con == NULL)  {
		LOG(("ERROR: Connect Failed!"));
        return -1;
	}

	LOG(("[ext_yahoo_connect] Got: %d", (int)con));
    return (int)con;
}

void ext_yahoo_send_http_request(int id, const char *method, const char *url, const char *cookies, long content_length,
		yahoo_get_fd_callback callback, void *callback_data)
{
/*	if (lstrcmpi(method, "GET") == 0) 
		yahoo_http_get(id, url, cookies, callback, callback_data);
	else if (lstrcmpi(method, "POST") == 0) 
		yahoo_http_post(id, url, cookies, content_length, callback, callback_data);
	else 
		LOG(("ERROR: Unknown method: %s", method));
*/
    NETLIBHTTPREQUEST nlhr={0};
	NETLIBHTTPHEADER httpHeaders[5];
	int fd, error = 0;

	char host[255];
	int port = 80;
	char path[255];
	char z[1024];
	
	LOG(("[ext_yahoo_send_http_request] method: %s, url: %s, cookies: %s, content length: %ld",
		method, url, cookies, content_length));
	
	if(!url_to_host_port_path(url, host, &port, path))
		return;

	fd = ext_yahoo_connect(host, port, YAHOO_CONNECTION_FT);
	
	if (fd < 0) {
		LOG(("[ext_yahoo_send_http_request] Can't connect?? Exiting..."));
		//return;
	} else {
		nlhr.cbSize=sizeof(nlhr);
		nlhr.requestType=(lstrcmpi(method, "GET") == 0) ? REQUEST_GET : REQUEST_POST;
		nlhr.flags=NLHRF_DUMPASTEXT|NLHRF_GENERATEHOST|NLHRF_SMARTREMOVEHOST|NLHRF_SMARTAUTHHEADER|NLHRF_HTTP11;
		nlhr.szUrl=(char *)url;
		nlhr.headers = httpHeaders;
		nlhr.headersCount = 3;
		
		httpHeaders[0].szName="Accept";
		httpHeaders[0].szValue="*/*";
		httpHeaders[1].szName="User-Agent";
		httpHeaders[1].szValue="Mozilla/4.0 (compatible; MSIE 5.5)";
		httpHeaders[2].szName="Pragma";
		httpHeaders[2].szValue="no-cache";
		
		if (cookies != NULL && cookies[0] != '\0') {
			httpHeaders[3].szName="Cookie";
			httpHeaders[3].szValue=(char *)cookies;
			nlhr.headersCount = 4;
		}
		
		if (nlhr.requestType == REQUEST_POST) {
			httpHeaders[nlhr.headersCount].szName="Content-Length";
			mir_snprintf(z, 1024, "%d", content_length);
			httpHeaders[nlhr.headersCount].szValue=z;
	
			nlhr.headersCount++;
		}
		
		error = CallService(MS_NETLIB_SENDHTTPREQUEST,(WPARAM)fd,(LPARAM)&nlhr);
	}
	
	callback(id, fd, error == SOCKET_ERROR, callback_data);
}

/*************************************
 * Callback handling code starts here
 */
YList *connections = NULL;
static int connection_tags=0;

int ext_yahoo_add_handler(int id, int fd, yahoo_input_condition cond, void *data)
{
	struct _conn *c = y_new0(struct _conn, 1);
	
	LOG(("[ext_yahoo_add_handler] fd:%d id:%d tag %d", fd, id, c->tag));
	
	c->tag = ++connection_tags;
	c->id = id;
	c->fd = fd;
	c->cond = cond;
	c->data = data;

	connections = y_list_prepend(connections, c);

	return c->tag;
}

void ext_yahoo_remove_handler(int id, int tag)
{
	YList *l;
	LOG(("[ext_yahoo_remove_handler] id:%d tag:%d ", id, tag));
	
	for(l = connections; l; l = y_list_next(l)) {
		struct _conn *c = l->data;
		if(c->tag == tag) {
			/* don't actually remove it, just mark it for removal */
			/* we'll remove when we start the next poll cycle */
			LOG(("Marking id:%d fd:%d tag:%d for removal", c->id, c->fd, c->tag));
			c->remove = 1;
			return;
		}
	}
}

struct connect_callback_data {
	yahoo_connect_callback callback;
	void * callback_data;
	int id;
	int tag;
};

static void connect_complete(void *data, int source, yahoo_input_condition condition)
{
	struct connect_callback_data *ccd = data;
	int error = 0;//, err_size = sizeof(error);
    NETLIBSELECT tSelect = {0};

	ext_yahoo_remove_handler(0, ccd->tag);
	
	// We Need to read the Socket error
	//getsockopt(source, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&err_size);
	
	tSelect.cbSize = sizeof( tSelect );
	//tSelect.dwTimeout = T->mGatewayTimeout * 1000;
	tSelect.dwTimeout = 1;
	tSelect.hReadConns[ 0 ] = ( HANDLE )source;
	error = YAHOO_CallService( MS_NETLIB_SELECT, 0, ( LPARAM )&tSelect );

	if(error) {
		//close(source);
		Netlib_CloseHandle((HANDLE)source);
		source = -1;
	}

	LOG(("Connected fd: %d, error: %d", source, error));

	ccd->callback(source, error, ccd->callback_data);
	FREE(ccd);
}

void yahoo_callback(struct _conn *c, yahoo_input_condition cond)
{
	int ret=1;

	//LOG(("[yahoo_callback] id: %d, fd: %d tag: %d", c->id, c->fd, c->tag));
	if(c->id < 0) {
		connect_complete(c->data, c->fd, cond);
	} else if (c->fd > 0) {
	
		if(cond & YAHOO_INPUT_READ)
			ret = yahoo_read_ready(c->id, c->fd, c->data);
		if(ret>0 && cond & YAHOO_INPUT_WRITE)
			ret = yahoo_write_ready(c->id, c->fd, c->data);

		if (ret == -1) {
			LOG(("Yahoo read error (%d): %s", errno, strerror(errno)));
		} else if(ret == 0)
			LOG(("Yahoo read error: Server closed socket"));
	}
	
	//LOG(("[yahoo_callback] id: %d exiting...", c->id));
}

int ext_yahoo_connect_async(int id, const char *host, int port, int type, 
		yahoo_connect_callback callback, void *data)
{
    int res;
    
    LOG(("ext_yahoo_connect_async %s:%d type: %d", host, port, type));
    
    res = ext_yahoo_connect(host, port, type);

	/*
	 * need to call the callback so we could handle the failure condition!!!
	 * fd = -1 in case of an error
	 */
	callback(res, 0, data);
	
	/*
	 * Return proper thing: 0 - ok, -1 - failed, >0 - pending connect
	 */
	return (res <= 0) ? -1 : 0;
}
/*
 * Callback handling code ends here
 ***********************************/

void register_callbacks()
{
	static struct yahoo_callbacks yc;

	yc.ext_yahoo_login_response = ext_yahoo_login_response;
	yc.ext_yahoo_got_buddies = ext_yahoo_got_buddies;
	yc.ext_yahoo_got_ignore = ext_yahoo_got_ignore;
	yc.ext_yahoo_got_identities = ext_yahoo_got_identities;
	yc.ext_yahoo_got_cookies = ext_yahoo_got_cookies;
	yc.ext_yahoo_status_changed = ext_yahoo_status_changed;
	yc.ext_yahoo_status_logon = ext_yahoo_status_logon;
	yc.ext_yahoo_got_im = ext_yahoo_got_im;
	yc.ext_yahoo_got_conf_invite = ext_yahoo_got_conf_invite;
	yc.ext_yahoo_conf_userdecline = ext_yahoo_conf_userdecline;
	yc.ext_yahoo_conf_userjoin = ext_yahoo_conf_userjoin;
	yc.ext_yahoo_conf_userleave = ext_yahoo_conf_userleave;
	yc.ext_yahoo_conf_message = ext_yahoo_conf_message;
	yc.ext_yahoo_chat_cat_xml = ext_yahoo_chat_cat_xml;
	yc.ext_yahoo_chat_join = ext_yahoo_chat_join;
	yc.ext_yahoo_chat_userjoin = ext_yahoo_chat_userjoin;
	yc.ext_yahoo_chat_userleave = ext_yahoo_chat_userleave;
	yc.ext_yahoo_chat_message = ext_yahoo_chat_message;
	yc.ext_yahoo_chat_yahoologout = ext_yahoo_chat_yahoologout;
	yc.ext_yahoo_chat_yahooerror = ext_yahoo_chat_yahooerror;
	yc.ext_yahoo_got_webcam_image = ext_yahoo_got_webcam_image;
	yc.ext_yahoo_webcam_invite = ext_yahoo_webcam_invite;
	yc.ext_yahoo_webcam_invite_reply = ext_yahoo_webcam_invite_reply;
	yc.ext_yahoo_webcam_closed = ext_yahoo_webcam_closed;
	yc.ext_yahoo_webcam_viewer = ext_yahoo_webcam_viewer;
	yc.ext_yahoo_webcam_data_request = ext_yahoo_webcam_data_request;
	yc.ext_yahoo_got_file = ext_yahoo_got_file;
	yc.ext_yahoo_got_file7info = ext_yahoo_got_file7info;
	yc.ext_yahoo_contact_added = ext_yahoo_contact_added;
	yc.ext_yahoo_rejected = ext_yahoo_rejected;
	yc.ext_yahoo_typing_notify = ext_yahoo_typing_notify;
	yc.ext_yahoo_game_notify = ext_yahoo_game_notify;
	yc.ext_yahoo_mail_notify = ext_yahoo_mail_notify;
	yc.ext_yahoo_got_search_result = ext_yahoo_got_search_result;
	yc.ext_yahoo_system_message = ext_yahoo_system_message;
	yc.ext_yahoo_error = ext_yahoo_error;
	yc.ext_yahoo_log = YAHOO_DebugLog;
	yc.ext_yahoo_add_handler = ext_yahoo_add_handler;
	yc.ext_yahoo_remove_handler = ext_yahoo_remove_handler;
	yc.ext_yahoo_connect = ext_yahoo_connect;
	yc.ext_yahoo_connect_async = ext_yahoo_connect_async;
	yc.ext_yahoo_send_http_request = ext_yahoo_send_http_request;

	yc.ext_yahoo_got_stealthlist = ext_yahoo_got_stealth;
	yc.ext_yahoo_got_ping  = ext_yahoo_got_ping;
	yc.ext_yahoo_got_picture  = ext_yahoo_got_picture;
	yc.ext_yahoo_got_picture_checksum = ext_yahoo_got_picture_checksum;
	yc.ext_yahoo_got_picture_update = ext_yahoo_got_picture_update;
	yc.ext_yahoo_got_avatar_share = ext_yahoo_got_avatar_share;
	
	yc.ext_yahoo_buddy_added = ext_yahoo_buddy_added;
	yc.ext_yahoo_got_picture_upload = ext_yahoo_got_picture_upload;
	yc.ext_yahoo_got_picture_status = ext_yahoo_got_picture_status;
	yc.ext_yahoo_got_audible = ext_yahoo_got_audible;
	yc.ext_yahoo_got_calendar = ext_yahoo_got_calendar;
	yc.ext_yahoo_buddy_group_changed = ext_yahoo_buddy_group_changed;
	
	yahoo_register_callbacks(&yc);
	
}

void ext_yahoo_login(int login_mode)
{
	char host[128], fthost[128];
	int port=0;
    DBVARIANT dbv;
#ifdef HTTP_GATEWAY				
	NETLIBUSERSETTINGS nlus = { 0 };
#endif
	
	LOG(("ext_yahoo_login"));

	if (!DBGetContactSettingString(NULL, yahooProtocolName, YAHOO_LOGINSERVER, &dbv)) {
        mir_snprintf(host, sizeof(host), "%s", dbv.pszVal);
        DBFreeVariant(&dbv);
    }
    else {
        //_snprintf(host, sizeof(host), "%s", pager_host);
       	YAHOO_ShowError(Translate("Yahoo Login Error"), Translate("Please enter Yahoo server to Connect to in Options."));
	    
        return;
    }

	lstrcpyn(fthost,YAHOO_GetByte("YahooJapan",0)?"filetransfer.msg.yahoo.co.jp":"filetransfer.msg.yahoo.com" , sizeof(fthost));
	port = DBGetContactSettingWord(NULL, yahooProtocolName, YAHOO_LOGINPORT, 5050);
	
#ifdef HTTP_GATEWAY			
	nlus.cbSize = sizeof( nlus );
	if (CallService(MS_NETLIB_GETUSERSETTINGS, (WPARAM) hNetlibUser, (LPARAM) &nlus) == 0) {
		LOG(("ERROR: Problem retrieving miranda network settings!!!"));
	}
	
	iHTTPGateway = (nlus.useProxy && nlus.proxyType == PROXYTYPE_HTTP) ? 1:0;
	LOG(("Proxy Type: %d HTTP Gateway: %d", nlus.proxyType, iHTTPGateway));
#endif

	//ylad->id = yahoo_init(ylad->yahoo_id, ylad->password);
	ylad->id = yahoo_init_with_attributes(ylad->yahoo_id, ylad->password, 
			"pager_host", host,
			"pager_port", port,
			"filetransfer_host", fthost,
			"picture_checksum", YAHOO_GetDword("AvatarHash", -1),
#ifdef HTTP_GATEWAY			
			"web_messenger", iHTTPGateway,
#endif
			NULL);
	
	ylad->status = YAHOO_STATUS_OFFLINE;
	yahoo_login(ylad->id, login_mode);

	if (ylad->id <= 0) {
		LOG(("Could not connect to Yahoo server.  Please verify that you are connected to the net and the pager host and port are correctly entered."));
		YAHOO_ShowError(Translate("Yahoo Login Error"), Translate("Could not connect to Yahoo server.  Please verify that you are connected to the net and the pager host and port are correctly entered."));
		return;
	}

	//rearm(&pingTimer, 600);
}

void YAHOO_refresh() 
{
	yahoo_refresh(ylad->id);
}





