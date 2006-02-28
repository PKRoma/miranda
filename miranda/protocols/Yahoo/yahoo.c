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
#include <sys/stat.h>
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
#include "utf8.h"

typedef struct {
	int id;
	char *label;
} yahoo_idlabel;

typedef struct {
	int id;
	char *who;
} yahoo_authorize_data;

yahoo_idlabel yahoo_status_codes[] = {
	{YAHOO_STATUS_AVAILABLE, "Available"},
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
	{YAHOO_STATUS_NOTIFY, "Notify"},
	{0, NULL}
};

#define MAX_PREF_LEN 255

yahoo_local_account * ylad = NULL;

int do_yahoo_debug = 0;
extern int poll_loop;
extern int gStartStatus;
extern char *szStartMsg;

void ext_yahoo_got_im(int id, char *me, char *who, char *msg, long tm, int stat, int utf8, int buddy_icon);
void yahoo_reset_avatar(HANDLE 	hContact, int bFail);

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
	//LOG(("yahoo_set_status myyahooStatus: %s (%d), msg: %s, away: %d", (char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, myyahooStatus, 0), myyahooStatus, msg, away));
	LOG(("yahoo_set_status myyahooStatus: %d, msg: %s, away: %d", myyahooStatus, msg, away));

	/* Safety check, don't dereference Invalid pointers */
	if ((ylad != NULL) && (ylad->id > 0) )  {
			
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
	if ((ylad != NULL) && (ylad->id > 0) )  {
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

void get_fd(int id, int fd, int error, void *data) 
{
    y_filetransfer *sf = (y_filetransfer*) data;
    char buf[1024];
    long size = 0;
	DWORD dw;
	struct _stat statbuf;
	
	if (fd < 0) {
		LOG(("[get_fd] Connect Failed!"));
		error = 1;
	}

	if (_stat( sf->filename, &statbuf ) != 0 )
		error = 1;
	
    if(!error) {
		 HANDLE myhFile    = CreateFile(sf->filename,
                                   GENERIC_READ,
                                   FILE_SHARE_READ|FILE_SHARE_WRITE,
			           NULL,
			           OPEN_EXISTING,
			           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
			           0);


		if(myhFile !=INVALID_HANDLE_VALUE) {
			PROTOFILETRANSFERSTATUS pfts;
			
			DWORD lNotify = GetTickCount();
			LOG(("proto: %s, hContact: %p", yahooProtocolName, sf->hContact));
			
			LOG(("Sending file: %s", sf->filename));
			//ProtoBroadcastAck(yahooProtocolName, sf->hContact, ACKTYPE_FILE, ACKRESULT_CONNECTING, sf, 0);
			ProtoBroadcastAck(yahooProtocolName, sf->hContact, ACKTYPE_FILE, ACKRESULT_NEXTFILE, sf, 0);
			ProtoBroadcastAck(yahooProtocolName, sf->hContact, ACKTYPE_FILE, ACKRESULT_CONNECTED, sf, 0);
			//ProtoBroadcastAck(yahooProtocolName, sf->hContact, ACKTYPE_FILE, ACKRESULT_SENTREQUEST, sf, 0);
			//ProtoBroadcastAck(yahooProtocolName, sf->hContact, ACKTYPE_FILE, ACKRESULT_INITIALISING, sf, 0);
						
			ZeroMemory(&pfts, sizeof(PROTOFILETRANSFERSTATUS));
			pfts.cbSize = sizeof(PROTOFILETRANSFERSTATUS);
			pfts.hContact = sf->hContact;
			pfts.sending = 1;
			pfts.files = &sf->filename;
			pfts.totalFiles = 1;
			pfts.currentFileNumber = 0;
			pfts.totalBytes = statbuf.st_size;
			pfts.workingDir = NULL;
			pfts.currentFile = sf->filename;
			pfts.currentFileSize = statbuf.st_size; 
			pfts.currentFileTime = 0;
			
			do {
				ReadFile(myhFile, buf, 1024, &dw, NULL);
			
				if (dw) {
					//dw = send(fd, buf, dw, 0);
					dw = Netlib_Send((HANDLE)fd, buf, dw, MSG_NODUMP);
					
					if (dw < 1) {
						LOG(("Upload Failed. Send error?"));
						error = 1;
						break;
					}
					
					size += dw;
					
					if(GetTickCount() >= lNotify + 500 || dw < 1024 || size == statbuf.st_size) {
						
					LOG(("DOING UI Notify. Got %lu/%lu", size, statbuf.st_size));
					pfts.totalProgress = size;
					pfts.currentFileProgress = size;
					
					ProtoBroadcastAck(yahooProtocolName, sf->hContact, ACKTYPE_FILE, ACKRESULT_DATA, sf, (LPARAM) & pfts);
					lNotify = GetTickCount();
					}
				}
				
				if (sf->cancel) {
					LOG(("Upload Cancelled! "));
					error = 1;
					break;
				}
			} while ( dw == 1024);
	    CloseHandle(myhFile);
		}
    }	

	//sf->state = FR_STATE_DONE;
    LOG(("File send complete!"));

	ProtoBroadcastAck(yahooProtocolName, sf->hContact, ACKTYPE_FILE, !error ? ACKRESULT_SUCCESS:ACKRESULT_FAILED, sf, 0);
}

void upload_avt(int id, int fd, int error, void *data) 
{
    y_filetransfer *sf = (y_filetransfer*) data;
    long size = 0;
	char buf[1024];
	DWORD dw;
	struct _stat statbuf;
	
	if (fd < 0) {
		LOG(("[get_fd] Connect Failed!"));
		error = 1;
	}

	if (_stat( sf->filename, &statbuf ) != 0 )
		error = 1;
	
    if(!error) {
		 HANDLE myhFile    = CreateFile(sf->filename,
                                   GENERIC_READ,
                                   FILE_SHARE_READ|FILE_SHARE_WRITE,
			           NULL,
			           OPEN_EXISTING,
			           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
			           0);


		if(myhFile !=INVALID_HANDLE_VALUE) {
			//LOG(("proto: %s, hContact: %p", yahooProtocolName, sf->hContact));
			
			LOG(("Sending file: %s", sf->filename));
			
			do {
				ReadFile(myhFile, buf, 1024, &dw, NULL);
			
				if (dw) {
					//dw = send(fd, buf, dw, 0);
					dw = Netlib_Send((HANDLE)fd, buf, dw, MSG_NODUMP);
					
					if (dw < 1) {
						LOG(("Upload Failed. Send error?"));
						error = 1;
						break;
					}
					
					size += dw;
				}
				
				if (sf->cancel) {
					LOG(("Upload Cancelled! "));
					error = 1;
					break;
				}
			} while ( dw == 1024);
	    CloseHandle(myhFile);
		}
    }	

	//sf->state = FR_STATE_DONE;
    LOG(("File send complete!"));
}

const YList* YAHOO_GetIgnoreList(void)
{
	if (!ylad)
		return NULL;
	
	return yahoo_get_ignorelist(ylad->id);
}

void YAHOO_IgnoreBuddy(const char *buddy, int ignore)
{
	if (!ylad)
		return;
	
	yahoo_ignore_buddy(ylad->id, buddy, ignore);
	//yahoo_get_list(ylad->id);
}

void YAHOO_SendFile(y_filetransfer *sf)
{
	long tFileSize = 0;
	{	struct _stat statbuf;
		if ( _stat( sf->filename, &statbuf ) == 0 )
			tFileSize += statbuf.st_size;
	}

	yahoo_send_file(ylad->id, sf->who, sf->msg, sf->filename, tFileSize, &get_fd, sf);
}

void YAHOO_SendAvatar(y_filetransfer *sf)
{
	long tFileSize = 0;
	{	struct _stat statbuf;
		if ( _stat( sf->filename, &statbuf ) == 0 )
			tFileSize += statbuf.st_size;
	}

	yahoo_send_avatar(ylad->id, sf->filename, tFileSize, &upload_avt, sf);
}

void YAHOO_FT_cancel(const char *buddy, const char *filename, const char *ft_token, int command)
{
	yahoo_ftdc_cancel(ylad->id, buddy, filename, ft_token, command);	
}

void get_url(int id, int fd, int error,	const char *filename, unsigned long size, void *data) 
{
    y_filetransfer *sf = (y_filetransfer*) data;
    char buf[1024];
    long rsize = 0;
	DWORD dw, c;

	if (fd < 0) {
		LOG(("[get_url] Connect Failed!"));
		
		if (sf->ftoken != NULL) {
			LOG(("[get_url] DC Detected: asking sender to upload to Yahoo FileServers!"));
			YAHOO_FT_cancel(sf->who, sf->filename, sf->ftoken, 3);	
		}
		
		error = 1;
	}
	
    if(!error) {
		HANDLE myhFile;
		PROTOFILETRANSFERSTATUS pfts;

		ZeroMemory(&pfts, sizeof(PROTOFILETRANSFERSTATUS));
		pfts.cbSize = sizeof(PROTOFILETRANSFERSTATUS);
		pfts.hContact = sf->hContact;
		pfts.sending = 0;
		pfts.files = (char**)&filename;
		pfts.totalFiles = 1;//ntohs(1);
		pfts.currentFileNumber = 0;
		pfts.totalBytes = size;
		
		pfts.workingDir = sf->savepath;//ft->savepath;
		pfts.currentFileSize = size; //ntohl(ft->hdr.size);
			
		LOG(("dir: %s, file: %s", sf->savepath, sf->filename ));
		wsprintf(buf, "%s\\%s", sf->savepath, sf->filename);
		
		pfts.currentFile = _strdup(buf);		
		LOG(("Saving: %s",  pfts.currentFile));
		
		if ( sf->hWaitEvent != INVALID_HANDLE_VALUE )
			CloseHandle( sf->hWaitEvent );
		
	    sf->hWaitEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

		if ( YAHOO_SendBroadcast( sf->hContact, ACKTYPE_FILE, ACKRESULT_FILERESUME, (void *)sf, ( LPARAM )&pfts )) {
			WaitForSingleObject( sf->hWaitEvent, INFINITE );
			
			switch(sf->action){
				case FILERESUME_RENAME:
				case FILERESUME_OVERWRITE:	
				case FILERESUME_RESUME:	
					// no action needed at this point, just break out of the switch statement
					break;

				case FILERESUME_CANCEL	:
					sf->cancel = 1;
					break;

				case FILERESUME_SKIP	:
				default:
					//delete this; // per usual dcc objects destroy themselves when they fail or when connection is closed
					//return FALSE; 
					sf->cancel = 2;
					break;
				}
		}

		free(pfts.currentFile);
		
		if (! sf->cancel) {
			
			if (sf->action != FILERESUME_RENAME ) {
				LOG(("dir: %s, file: %s", sf->savepath, sf->filename ));
			
				wsprintf(buf, "%s\\%s", sf->savepath, sf->filename);
			} else {
				LOG(("file: %s", sf->filename ));
			//wsprintf(buf, "%s\%s", sf->filename);
				lstrcpy(buf, sf->filename);
			}
			
			//pfts.files = &buf;
			pfts.currentFile = _strdup(buf);		
	
			LOG(("Getting file: %s", buf));
			myhFile    = CreateFile(buf,
									GENERIC_WRITE,
									FILE_SHARE_WRITE,
									NULL, OPEN_ALWAYS,  FILE_ATTRIBUTE_NORMAL,  0);
	
			if(myhFile !=INVALID_HANDLE_VALUE) {
				
						
				DWORD lNotify = GetTickCount();
				LOG(("proto: %s, hContact: %p", yahooProtocolName, sf->hContact));
				
				ProtoBroadcastAck(yahooProtocolName, sf->hContact, ACKTYPE_FILE, ACKRESULT_CONNECTED, sf, 0);
				
				do {
					dw = Netlib_Recv((HANDLE)fd, buf, 1024, MSG_NODUMP);
				
					if (dw > 0) {
						WriteFile(myhFile, buf, dw, &c, NULL);
						rsize += dw;
						
						/*LOG(("Got %d/%d", rsize, size));*/
						if(GetTickCount() >= lNotify + 500 || dw <= 0 || rsize == size) {
							
						LOG(("DOING UI Notify. Got %lu/%lu", rsize, size));
						
						pfts.totalProgress = rsize;
						pfts.currentFileTime = time(NULL);//ntohl(ft->hdr.modtime);
						pfts.currentFileProgress = rsize;
						
						ProtoBroadcastAck(yahooProtocolName, sf->hContact, ACKTYPE_FILE, ACKRESULT_DATA, sf, (LPARAM) & pfts);
						lNotify = GetTickCount();
						}
					} else if (dw < 0) {
						LOG(("Recv Failed! Socket Error?"));
						error = 1;
						break;
					}
					
					if (sf->cancel) {
						LOG(("Recv Cancelled! "));
						error = 1;
						break;
					}
				} while ( dw > 0 && rsize < size);
				
				LOG(("[Finished DL] Got %lu/%lu", rsize, size));
				CloseHandle(myhFile);
				
			} else {
				LOG(("Can not open file for writing: %s", buf));
			}
			
			free(pfts.currentFile);
		}
		
    }
	
	Netlib_CloseHandle((HANDLE)fd);
    LOG(("File download complete!"));

    ProtoBroadcastAck(yahooProtocolName, sf->hContact, ACKTYPE_FILE, !error ? ACKRESULT_SUCCESS:ACKRESULT_FAILED, sf, 0);
}

void YAHOO_RecvFile(y_filetransfer *ft)
{
	yahoo_get_url_handle(ylad->id, ft->url, &get_url, ft);
}

void YAHOO_basicsearch(const char *nick)
{
	yahoo_search(ylad->id, YAHOO_SEARCH_YID, nick, YAHOO_GENDER_NONE, YAHOO_AGERANGE_NONE, 0, 1);
	
}

void YAHOO_remove_buddy(const char *who)
{
    HANDLE hContact;
	DBVARIANT dbv;
	
	LOG(("YAHOO_remove_buddy '%s'", who));
	
	hContact = getbuddyH(who);

	//if ( DBGetContactSetting( hContact, "Clist", "Group", &dbv ))
	//	return;
	if ( DBGetContactSetting( hContact, yahooProtocolName, "YGroup", &dbv )) {
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

void YAHOO_reject(const char *who, const char *msg)
{
    yahoo_reject_buddy(ylad->id, who, msg);
    //YAHOO_remove_buddy(who);
    //yahoo_refresh(ylad->id);
}

void yahoo_logout()
{
	//poll_loop = 0; <- don't kill us before we do full cleanup!!!
	
	if (ylad == NULL)
        return;
        
	if (ylad->id <= 0) {
		return;
	}

	if (yahooLoggedIn) {
		//yahooLoggedIn = FALSE; 
		yahoo_logoff(ylad->id);
	} else {
		poll_loop = 0; /* we need to trigger server stop */
	}
	
	//pthread_mutex_lock(&connectionHandleMutex);
    
    
	//pthread_mutex_unlock(&connectionHandleMutex);
}

void ext_yahoo_cleanup(int id)
{
	LOG(("[ext_yahoo_cleanup] id: %d", id));
	
/*	if (!ylad)
		return;
	
	if (ylad->id != id) {
		return;
	}
	
	LOG(("[ext_yahoo_cleanup] id matched!!! Doing Cleanup!"));
	FREE(ylad);
	ylad = NULL;*/
}

void yahoo_send_msg(const char *id, const char *msg, int utf8)
{
	int buddy_icon = 0;
	LOG(("yahoo_send_msg: %s: %s, utf: %d", id, msg, utf8));
	
	buddy_icon = (YAHOO_GetDword("AvatarHash", 0) != 0) ? 2: 0;
	
    if (YAHOO_GetByte( "DisableUTF8", 0 ) ) { /* Send ANSI */
		/* need to convert it to ascii argh */
		char 	*umsg = (char *)msg;
		
		if (utf8) {
			wchar_t* tRealBody = NULL;
			
			umsg = (char *) alloca(lstrlen(msg) + 1);
			lstrcpy(umsg, msg);
			Utf8Decode( umsg, 0, &tRealBody );
			free( tRealBody );
		} 
			
		yahoo_send_im(ylad->id, NULL, id, umsg, 0, buddy_icon);
    } else { /* Send Unicode */
	    
		if (!utf8) {
			char *tmp;
			
			utf8_encode(msg, &tmp);
        	//tmp = y_str_to_utf8(msg);
			yahoo_send_im(ylad->id, NULL, id, tmp, 1, buddy_icon);
			free(tmp);
		} else 
			yahoo_send_im(ylad->id, NULL, id, msg, 1, buddy_icon);
    } 
   
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
			if ( DBGetContactSetting( hContact, yahooProtocolName, YAHOO_LOGINID, &dbv ))
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
	//char *szProto;
	HANDLE hContact;
	//DBVARIANT dbv;
	
	hContact = getbuddyH(yahoo_id);
	if (hContact != NULL) {
		if ( !( flags & PALF_TEMPORARY ) && DBGetContactSettingByte( hContact, "CList", "NotOnList", 1 )) 
		{
			LOG(("[add_buddy] Making Perm id: %s, flags: %lu", yahoo_id, flags));
			DBDeleteContactSetting( hContact, "CList", "NotOnList" );
			DBDeleteContactSetting( hContact, "CList", "Hidden" );

        }
		return hContact;
    }

	//not already there: add
	hContact = ( HANDLE )YAHOO_CallService( MS_DB_CONTACT_ADD, 0, 0 );
	YAHOO_CallService( MS_PROTO_ADDTOCONTACT, ( WPARAM )hContact,( LPARAM )yahooProtocolName );
	YAHOO_SetString( hContact, YAHOO_LOGINID, yahoo_id );
	if (lstrlen(yahoo_name) > 0)
		YAHOO_SetString( hContact, "Nick", yahoo_name );
	else
	    YAHOO_SetString( hContact, "Nick", yahoo_id );
	    
	//DBWriteContactSettingWord(hContact, yahooProtocolName, "Status", ID_STATUS_OFFLINE);

   	if (flags & PALF_TEMPORARY ) {
		DBWriteContactSettingByte( hContact, "CList", "NotOnList", 1 );
		DBWriteContactSettingByte( hContact, "CList", "Hidden", 1 );
    }	
	return hContact;
}

const char *find_buddy( const char *yahoo_id)
{
	//char  *szProto;
	static char nick[128];
	HANDLE hContact;
	DBVARIANT dbv;
	
	hContact = getbuddyH(yahoo_id);
	if (hContact != NULL) {
		if ( DBGetContactSetting( hContact, yahooProtocolName, "Nick", &dbv ))
			return NULL;
	
		strncpy(nick, dbv.pszVal, 128);
		DBFreeVariant( &dbv );
		return nick;
		
    }

	return NULL;
}

/* Required handlers bellow */

/* Conference handlers */
void ext_yahoo_got_conf_invite(int id, char *who, char *room, char *msg, YList *members)
{
	char z[1024];
	
	_snprintf(z, sizeof(z), Translate("[miranda] Got conference invite to room: %s with msg: %s"), room ?room:"", msg ?msg:"");
	LOG(("[ext_yahoo_got_conf_invite] %s", z));
	ext_yahoo_got_im(id, "me", who, z, 0, 0, 0, -1);
	
	yahoo_conference_decline(ylad->id, NULL, members, room, Translate("I am sorry, but i can't join your conference since this feature is not currently implemented in my client."));
}

void ext_yahoo_conf_userdecline(int id, char *who, char *room, char *msg)
{
}

void ext_yahoo_conf_userjoin(int id, char *who, char *room)
{
}

void ext_yahoo_conf_userleave(int id, char *who, char *room)
{
}

void ext_yahoo_conf_message(int id, char *who, char *room, char *msg, int utf8)
{
}

/* chat handlers */
void ext_yahoo_chat_cat_xml(int id, char *xml) 
{
}

void ext_yahoo_chat_join(int id, char *room, char * topic, YList *members, int fd)
{
}

void ext_yahoo_chat_userjoin(int id, char *room, struct yahoo_chat_member *who)
{
}

void ext_yahoo_chat_userleave(int id, char *room, char *who)
{
}
void ext_yahoo_chat_message(int id, char *who, char *room, char *msg, int msgtype, int utf8)
{
}

void YAHOO_request_avatar(const char* who)
{
	time_t  last_chk, cur_time;
	HANDLE 	hContact = 0;
	char    szFile[MAX_PATH];
	
	if (!YAHOO_GetByte( "ShowAvatars", 0 )) {
		LOG(("Avatars disabled, but available for: %s", who));
		return;
	}
	
	hContact = getbuddyH(who);
	
	if (!hContact)
		return;
	
	
	GetAvatarFileName(hContact, szFile, sizeof szFile, DBGetContactSettingByte(hContact, yahooProtocolName,"AvatarType", 0));
	DeleteFile(szFile);
	
	time(&cur_time);
	last_chk = DBGetContactSettingDword(hContact, yahooProtocolName, "PictLastCheck", 0);
	
	/*
	 * time() - in seconds ( 60*60 = 1 hour)
	 */
	if (DBGetContactSettingDword(hContact, yahooProtocolName,"PictCK", 0) == 0 || 
		last_chk == 0 || (cur_time - last_chk) < 300) {
			
		DBWriteContactSettingDword(hContact, yahooProtocolName, "PictLastCheck", cur_time);

		LOG(("Requesting Avatar for: %s", who));
		yahoo_request_buddy_avatar(ylad->id, who);
	} else {
		LOG(("Avatar Not Available for: %s (Flood Check in Effect)", who));
	}
}

/* Other handlers */
void ext_yahoo_status_changed(int id, const char *who, int stat, const char *msg, int away, int idle, int mobile, int cksum, int buddy_icon)
{
	HANDLE 	hContact = 0;
	time_t  idlets = 0;
	
	YAHOO_DebugLog("ext_yahoo_status_changed for %s with msg %s (stat: %d, away: %d, idle: %d seconds, checksum: %d buddy_icon: %d)", who, msg, stat, away, idle, cksum, buddy_icon);
	
	hContact = getbuddyH(who);
	if (hContact == NULL) {
		YAHOO_DebugLog("Buddy Not Found. Adding...");
		hContact = add_buddy(who, who, 0);
	} else {
		YAHOO_DebugLog("Buddy Found On My List! Buddy %p", hContact);
	}
	
	if (!mobile)
		YAHOO_SetWord(hContact, "Status", yahoo_to_miranda_status(stat,away));
	else
		YAHOO_SetWord(hContact, "Status", ID_STATUS_ONTHEPHONE);
	
	YAHOO_SetWord(hContact, "YStatus", stat);
	YAHOO_SetWord(hContact, "YAway", away);
	YAHOO_SetWord(hContact, "Mobile", mobile);

	if(msg) {
		LOG(("%s custom message '%s'", who, msg));
		//YAHOO_SetString(hContact, "YMsg", msg);
		DBWriteContactSettingString( hContact, "CList", "StatusMsg", msg);
	} else {
		//YAHOO_SetString(hContact, "YMsg", "");
		DBDeleteContactSetting(hContact, "CList", "StatusMsg" );
	}

	
	if ( (away == 2) || (stat == YAHOO_STATUS_IDLE) || (idle > 0)) {
		if (stat > 0) {
			LOG(("%s idle for %d:%02d:%02d", who, idle/3600, (idle/60)%60, idle%60));
			
			time(&idlets);
			idlets -= idle;
		}
	} 
		
	DBWriteContactSettingDword(hContact, yahooProtocolName, "IdleTS", idlets);

	/* Last thing check the checksum and request new one if we need to */
	if (!cksum || cksum == -1) {
		yahoo_reset_avatar(hContact, 0); 
	} else  if (DBGetContactSettingDword(hContact, yahooProtocolName,"PictCK", 0) != cksum) {
		// Reset Avatar first
		yahoo_reset_avatar(hContact, 0);
		// Save New Checksum
		DBWriteContactSettingDword(hContact, yahooProtocolName, "PictCK", cksum);
		//yahoo_reset_avatar(hContact);YAHOO_request_avatar(who);
		ProtoBroadcastAck(yahooProtocolName, hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS,NULL, 0);
	}
	
	if (buddy_icon != -1) {// we got some avatartype info
		DBWriteContactSettingByte(hContact, yahooProtocolName, "AvatarType", buddy_icon);
		ProtoBroadcastAck(yahooProtocolName, hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS,NULL, 0);
	}
	
	LOG(("[ext_yahoo_status_changed] exiting"));
}

struct avatar_info{
	char *who;
	char *pic_url;
	int cksum;
};

void get_picture(int id, int fd, int error,	const char *filename, unsigned long size, void *data) 
{
    PROTO_AVATAR_INFORMATION AI;
	char buf[4096];
    int rsize = 0;
	DWORD dw, c;
	HANDLE 	hContact = 0;
	char *pBuff = NULL;
	struct avatar_info *avt = (struct avatar_info *) data;
		
	LOG(("Getting file: %s size: %lu", filename, size));
	
	if (!size) /* empty file or some file loading error. don't crash! */
        error = 1;
    
	if (!error) {
		pBuff = malloc(size);
		if (!pBuff) 
			error = 1;
	}
	
	hContact = getbuddyH(avt->who);
		
	if (!hContact){
		error = 1;
	} else if (!error) {
		DBWriteContactSettingDword(hContact, yahooProtocolName, "PictCK", avt->cksum);
		DBWriteContactSettingDword(hContact, yahooProtocolName, "PictLoading", 1);
	}
	
    if(!error) {
				
				do {
					dw = Netlib_Recv((HANDLE)fd, buf, 4096, MSG_NODUMP);
				
					LOG(("Got %lu bytes!", dw));
					
					if (dw > 0) {
						CopyMemory(&pBuff[rsize], buf, dw);
						rsize += dw;
					} else if (dw < 0) {
						error = 1;
					}
					
				} while ( dw > 0 );
			
    }
	
	Netlib_CloseHandle((HANDLE)fd);
	
	if (DBGetContactSettingDword(hContact, yahooProtocolName, "PictCK", 0) != avt->cksum) {
		LOG(("WARNING: Checksum updated during download?!"));
		error = 1; /* don't use this one? */
	} 
    
	DBWriteContactSettingDword(hContact, yahooProtocolName, "PictLoading", 0);
	LOG(("File download complete!?"));

//    ProtoBroadcastAck(yahooProtocolName, sf->hContact, ACKTYPE_FILE, !error ? ACKRESULT_SUCCESS:ACKRESULT_FAILED, sf, 0);
	if (!error) {
			HANDLE myhFile;
            BITMAPINFOHEADER* pDib;
			
			//---- Converting memory buffer to bitmap and saving it to disk
			if ( !YAHOO_LoadPngModule() ) 
				return;

			if ( !png2dibConvertor( pBuff, rsize, &pDib )) {
				FREE(pBuff);
				return;
			}

			GetAvatarFileName(hContact, buf, 1024, DBGetContactSettingByte(hContact, yahooProtocolName,"AvatarType", 0));
			LOG(("Saving file: %s size: %lu", buf, size));
			myhFile    = CreateFile(buf,
									GENERIC_WRITE,
									FILE_SHARE_WRITE,
									NULL, OPEN_ALWAYS,  FILE_ATTRIBUTE_NORMAL,  0);
	
			if(myhFile !=INVALID_HANDLE_VALUE) {
				BITMAPFILEHEADER tHeader = { 0 };
				tHeader.bfType = 0x4d42;
				tHeader.bfOffBits = sizeof( tHeader ) + sizeof( BITMAPINFOHEADER );
				tHeader.bfSize = tHeader.bfOffBits + pDib->biSizeImage;
				WriteFile(myhFile, &tHeader, sizeof( tHeader ), &c, NULL);
				WriteFile(myhFile, pDib, sizeof( BITMAPINFOHEADER ), &c, NULL );
				WriteFile(myhFile, pDib+1, pDib->biSizeImage, &c, NULL );
				CloseHandle(myhFile);
				
				DBWriteContactSettingString(hContact, "ContactPhoto", "File", buf);
				DBWriteContactSettingDword(hContact, yahooProtocolName, "PictLastCheck", 0);
			} else {
				LOG(("Can not open file for writing: %s", buf));
				error = 1;
			}
			
			GlobalFree( pDib );
	} else {
			//GetAvatarFileName(hContact, buf, 1024);
			buf[0]='\0';
	}

	FREE(pBuff);

	AI.cbSize = sizeof AI;
	AI.format = PA_FORMAT_BMP;
	AI.hContact = hContact;
	lstrcpy(AI.filename,buf);

	ProtoBroadcastAck(yahooProtocolName, hContact, ACKTYPE_AVATAR, !error ? ACKRESULT_SUCCESS:ACKRESULT_FAILED,(HANDLE) &AI, 0);
}


static void __cdecl yahoo_recv_avatarthread(void *pavt) 
{
	struct avatar_info *avt = pavt;
	
//    ProtoBroadcastAck(yahooProtocolName, hContact, ACKTYPE_GETINFO, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
	if (avt == NULL) {
		YAHOO_DebugLog("AVT IS NULL!!!");
		return;
	}
	
	LOG(("yahoo_recv_avatarthread who:%s url:%s checksum: %d", avt->who, avt->pic_url, avt->cksum));
	yahoo_get_url_handle(ylad->id, avt->pic_url, &get_picture, avt);
	
	free(avt->who);
	free(avt->pic_url);
	free(avt);
}



void ext_yahoo_got_picture(int id, const char *me, const char *who, const char *pic_url, int cksum, int type)
{
	HANDLE 	hContact = 0;
		
	LOG(("ext_yahoo_got_picture for %s with url %s (checksum: %d) type: %d", who, pic_url, cksum, type));
	
	if (!YAHOO_GetByte( "ShowAvatars", 0 )) {
		LOG(("[ext_yahoo_got_picture] We are not using/showing avatars!"));
		return;
	}
	
	/*
	  Type:
	
		1 - Send Avatar Info
		2 - Got Avatar Info
		3 - YIM6 didn't like my avatar?
	 */
	if (type == 1) {
		int cksum=0;
		DBVARIANT dbv;
		
		/* need to send avatar info */
		LOG(("[ext_yahoo_got_picture] Getting ready to send info!"));
		/* need to read CheckSum */
		cksum = YAHOO_GetDword("AvatarHash", 0);
		if (cksum) {
			if (!DBGetContactSetting(NULL, yahooProtocolName, "AvatarURL", &dbv)) {
				LOG(("[ext_yahoo_got_picture] Sending url:%s checksum: %d to '%s'!", dbv.pszVal, cksum, who));
				//void yahoo_send_picture_info(int id, const char *me, const char *who, const char *pic_url, int cksum)
				yahoo_send_picture_info(id, who, dbv.pszVal, cksum);
				DBFreeVariant(&dbv);
			}
		}
		return;
	} if (type != 2) {
		LOG(("[ext_yahoo_got_picture] Unknown request/packet type exiting!"));
		return;
	}
	
	/* got avatar info, so set miranda up */
	hContact = getbuddyH(who);
	
	if (!hContact) {
		LOG(("[ext_yahoo_got_picture] Buddy not on my buddy list?."));
		return;
	}
	
	if (!cksum || cksum == -1) {
		LOG(("[ext_yahoo_got_picture] Resetting avatar."));
        yahoo_reset_avatar(hContact, 0);
		ProtoBroadcastAck(yahooProtocolName, hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS,NULL, 0);
	} else {
		char z[1024];
		
		if (pic_url == NULL) {
			LOG(("[ext_yahoo_got_picture] WARNING: Empty URL for avatar?"));
			return;
		}
		
		GetAvatarFileName(hContact, z, 1024, DBGetContactSettingByte(hContact, yahooProtocolName,"AvatarType", 0));
		
		if (DBGetContactSettingDword(hContact, yahooProtocolName,"PictCK", 0) != cksum || _access( z, 0 ) != 0 ) {
			struct avatar_info *avt;
			
			YAHOO_DebugLog("[ext_yahoo_got_picture] Checksums don't match or avatar file is missing. Current: %d, New: %d",(int)DBGetContactSettingDword(hContact, yahooProtocolName,"PictCK", 0), cksum);
			avt = malloc(sizeof(struct avatar_info));
			avt->who = _strdup(who);
			avt->pic_url = _strdup(pic_url);
			avt->cksum = cksum;
			
			pthread_create(yahoo_recv_avatarthread, (void *) avt);
		}
	}
	LOG(("ext_yahoo_got_picture exiting"));
}

void yahoo_reset_avatar(HANDLE 	hContact, int bFail)
{
    PROTO_AVATAR_INFORMATION AI;
        
	LOG(("[YAHOO_RESET_AVATAR]"));
	//DBDeleteContactSetting(hContact, yahooProtocolName, "PictCK" );
	DBWriteContactSettingDword(hContact, yahooProtocolName, "PictCK", 0);

	AI.cbSize = sizeof AI;
	AI.format = PA_FORMAT_BMP;
	AI.hContact = hContact;

	GetAvatarFileName(AI.hContact, AI.filename, sizeof AI.filename, DBGetContactSettingByte(hContact, yahooProtocolName,"AvatarType", 0));
	DeleteFile(AI.filename);
	
	// STUPID SCRIVER Doesn't listen to ACKTYPE_AVATAR. so remove the file reference!
	DBDeleteContactSetting(AI.hContact, "ContactPhoto", "File");	
	//AI.filename[0]='\0';
	if (bFail)
		ProtoBroadcastAck(yahooProtocolName, hContact, ACKTYPE_AVATAR, ACKRESULT_FAILED,(HANDLE) &AI, 0);
}

void ext_yahoo_got_picture_checksum(int id, const char *me, const char *who, int cksum)
{
	HANDLE 	hContact = 0;

	LOG(("ext_yahoo_got_picture_checksum for %s checksum: %d", who, cksum));

	hContact = getbuddyH(who);
	if (hContact == NULL) {
		LOG(("Buddy Not Found. Skipping avatar update"));
		return;
	}
	
	/* Last thing check the checksum and request new one if we need to */
	if (!cksum || cksum == -1) {
        yahoo_reset_avatar(hContact,0);
		ProtoBroadcastAck(yahooProtocolName, hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS,NULL, 0);
	} else {
		if (DBGetContactSettingDword(hContact, yahooProtocolName,"PictCK", 0) != cksum) {
			// Reset the avatar and cleanup.
			yahoo_reset_avatar(hContact,1);
			
			// Now save the new checksum. No rush requesting new avatar yet.
			DBWriteContactSettingDword(hContact, yahooProtocolName, "PictCK", cksum);
			//DBWriteContactSettingDword(hContact, yahooProtocolName, "PictLoading", 0);
			YAHOO_request_avatar(who);	
			//ProtoBroadcastAck(yahooProtocolName, hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS,NULL, 0);
		}
	}
	
}

void ext_yahoo_got_picture_update(int id, const char *me, const char *who, int buddy_icon)
{
	HANDLE 	hContact = 0;

	LOG(("ext_yahoo_got_picture_update for %s buddy_icon: %d", who, buddy_icon));

	hContact = getbuddyH(who);
	if (hContact == NULL) {
		LOG(("Buddy Not Found. Skipping avatar update"));
		return;
	}
	
	DBWriteContactSettingByte(hContact, yahooProtocolName, "AvatarType", buddy_icon);
	
	/* Last thing check the checksum and request new one if we need to */
	if (buddy_icon == 0 || buddy_icon == 1) {
		//yahoo_reset_avatar(hContact, 0);
		DBDeleteContactSetting(hContact, "ContactPhoto", "File");	
	} //else if (buddy_icon == 2) {
	//	YAHOO_request_avatar(who);	
		ProtoBroadcastAck(yahooProtocolName, hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS,NULL, 0);
	//}
	
}

void ext_yahoo_got_avatar_update(int id, const char *me, const char *who, int buddy_icon)
{
	HANDLE 	hContact = 0;

	LOG(("ext_yahoo_got_avatar_update for %s buddy_icon: %d", who, buddy_icon));

	hContact = getbuddyH(who);
	if (hContact == NULL) {
		LOG(("Buddy Not Found. Skipping avatar update"));
		return;
	}
	
	DBWriteContactSettingByte(hContact, yahooProtocolName, "AvatarType", buddy_icon);
	
	/* Last thing check the checksum and request new one if we need to */
	if (buddy_icon == 0 || buddy_icon == 1) {
		yahoo_reset_avatar(hContact, 0);
	} //else if (buddy_icon == 2) {
		/* weird cause we getting 2 notifications?? */
		//YAHOO_request_avatar(who);	
		ProtoBroadcastAck(yahooProtocolName, hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS,NULL, 0);
	//}

}

void YAHOO_set_avatar(int buddy_icon)
{
	yahoo_send_avatar_update(ylad->id,buddy_icon);
}

void ext_yahoo_got_audible(int id, const char *me, const char *who, const char *aud, const char *msg, const char *aud_hash)
{
	HANDLE 	hContact = 0;
	char z[1024];
	
	LOG(("ext_yahoo_got_audible for %s aud: %s msg:'%s' hash: %s", who, aud, msg, aud_hash));

	hContact = getbuddyH(who);
	if (hContact == NULL) {
		LOG(("Buddy Not Found. Skipping avatar update"));
		return;
	}
	
	_snprintf(z, sizeof(z), "[miranda-audible] %s", msg ?msg:"");
	ext_yahoo_got_im(id, (char*)me, (char*)who, z, 0, 0, 0, -1);
}

void ext_yahoo_got_picture_upload(int id, const char *me, const char *url,unsigned int ts)
{
	long cksum = 0;
	
	LOG(("ext_yahoo_got_picture_upload URL:%s timestamp: %d", url, ts));

	/* aud = file class name 
					GAIM: the audible, in foo.bar.baz format
			
					Actually this is the filename.
					Full URL:
				
					http://us.dl1.yimg.com/download.yahoo.com/dl/aud/us/aud.swf 
					
					where aud in foo.bar.baz format
					*/
	
	//DBWriteContactSettingDword(NULL, yahooProtocolName, "AvatarHash", hash);
	if (!url) {
		LOG(("[ext_yahoo_got_picture_upload] Problem with upload?"));
		return;
	}
	
	cksum = YAHOO_GetDword("AvatarHash", 0);
	if (cksum != 0) {
		char  *szProto;
		HANDLE hContact;

		LOG(("[ext_yahoo_got_picture_upload] My Checksum: %ld", cksum));
		
		YAHOO_SetString(NULL, "AvatarURL", url);
		YAHOO_SetDword("AvatarTS", ts);
		
		//yahoo_send_picture_checksum(id, NULL, cksum);
		
		/* need to get online buddies and then send then picture_checksum packets (ARGH YAHOO!)*/
		//yahoo_send_avatar_update(id,2);// YIM7 does this? YIM6 doesn't like this!
		
		/*for ( hContact = ( HANDLE )YAHOO_CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
			   hContact != NULL;
				hContact = ( HANDLE )YAHOO_CallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM )hContact, 0 ))
		{
			szProto = ( char* )YAHOO_CallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM )hContact, 0 );
			if ( szProto != NULL && !lstrcmp( szProto, yahooProtocolName ))
			{
				if (YAHOO_GetWord(hContact, "Status", ID_STATUS_OFFLINE)!=ID_STATUS_OFFLINE) {
								DBVARIANT dbv;

					if ( DBGetContactSetting( hContact, yahooProtocolName, YAHOO_LOGINID, &dbv ))
						continue;

					yahoo_send_picture_checksum(id, dbv.pszVal, cksum);
					DBFreeVariant( &dbv );
				}
			}
		}*/
		/* need to get online buddies and then send then picture_update packets (ARGH YAHOO!)*/
		//yahoo_send_avatar_update(id,2);// YIM7 does this? YIM6 doesn't like this!
		
		for ( hContact = ( HANDLE )YAHOO_CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
			   hContact != NULL;
				hContact = ( HANDLE )YAHOO_CallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM )hContact, 0 ))
		{
			szProto = ( char* )YAHOO_CallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM )hContact, 0 );
			if ( szProto != NULL && !lstrcmp( szProto, yahooProtocolName ))
			{
				if (YAHOO_GetWord(hContact, "Status", ID_STATUS_OFFLINE)!=ID_STATUS_OFFLINE) {
								DBVARIANT dbv;

					if ( DBGetContactSetting( hContact, yahooProtocolName, YAHOO_LOGINID, &dbv ))
						continue;

					yahoo_send_picture_update(id, dbv.pszVal, 2);
					DBFreeVariant( &dbv );
				}
			}
		}
				
		yahoo_send_picture_checksum(id, NULL, cksum);
	}
}

void ext_yahoo_got_nick(int id, const char *nick)
{
	LOG(("[ext_yahoo_got_nick] nick: %s", nick));
	
	//YAHOO_SetString( NULL, "Nick", nick);
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
			if ( DBGetContactSetting( hContact, yahooProtocolName, YAHOO_LOGINID, &dbv ))
				continue;

			found = 0;
			
			for(s = stealth; s && *s; s++) {
				
				if (lstrcmpi(*s, dbv.pszVal) == 0) {
					LOG(("GOT id = %s", dbv.pszVal));
					found = 1;
					break;
				}
			}
			
			/* Check the stealth list */
			if (found) { /* we have him on our Stealth List */
				LOG(("Setting STEALTH for id = %s", dbv.pszVal));
				/* need to set the ApparentMode thingy */
				if (ID_STATUS_OFFLINE != DBGetContactSettingWord(hContact, yahooProtocolName, "ApparentMode", 0))
					DBWriteContactSettingWord(hContact, yahooProtocolName, "ApparentMode", (WORD) ID_STATUS_OFFLINE);
				
			} else { /* he is not on the Stealth List */
				LOG(("Resetting STEALTH for id = %s", dbv.pszVal));
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
			LOG(("id = %s name = %s", bud->id, bud->real_name));
		
		hContact = getbuddyH(bud->id);
		if (hContact == NULL)
			hContact = add_buddy(bud->id, (bud->real_name != NULL) ? bud->real_name : bud->id, 0);
		
		if (bud->group)
			YAHOO_SetString( hContact, "YGroup", bud->group);
		
		if (bud->yab_entry) {
		  LOG(("YAB_ENTRY"));
		  
		  if (bud->yab_entry->fname) 
		    YAHOO_SetString( hContact, "FirstName", bud->yab_entry->fname);
		  
		  
		  if (bud->yab_entry->lname) 
		      YAHOO_SetString( hContact, "LastName", bud->yab_entry->lname);
		  
		  
		  if (bud->yab_entry->nname) 
		      YAHOO_SetString( hContact, "Nick", bud->yab_entry->nname);
		  
		  
		  if (bud->yab_entry->email) 
			YAHOO_SetString( hContact, "e-mail", bud->yab_entry->email);
		  
		  if (bud->yab_entry->mphone) 
			YAHOO_SetString( hContact, "Cellular", bud->yab_entry->mphone);
		  
		}
		
		
	}

}

void ext_yahoo_got_ignore(int id, YList * igns)
{
    LOG(("ext_yahoo_got_ignore"));
}

void ext_yahoo_got_im(int id, char *me, char *who, char *msg, long tm, int stat, int utf8, int buddy_icon)
{
    char 		*umsg, *c = msg;
	int 		oidx = 0;
	wchar_t* 	tRealBody = NULL;
	int      	tRealBodyLen = 0;
	int 		msgLen;
	char* 		tMsgBuf = NULL;
	char* 		p = NULL;
	CCSDATA 		ccs;
	PROTORECVEVENT 	pre;
	HANDLE 			hContact;

	
    LOG(("YAHOO_GOT_IM id:%s %s: %s tm:%lu stat:%i utf8:%i buddy_icon: %i", me, who, msg, tm, stat, utf8, buddy_icon));
   	
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

	{
		YList *l;
		
		/* show our current ignore list */
		l = (YList *)YAHOO_GetIgnoreList();
		while (l != NULL) {
			struct yahoo_buddy *b = (struct yahoo_buddy *) l->data;
			
			//MessageBox(NULL, b->id, "ID", MB_OK);
			//SendMessage(GetDlgItem(hwndDlg,IDC_YIGN_LIST), LB_INSERTSTRING, 0, (LPARAM)b->id);
			if (lstrcmpi(b->id, who) == 0) {
				LOG(("User '%s' on our Ignore List. Dropping Message.", who));
				return;
			}
			l = l->next;
		}

	}
		
	umsg = (char *) alloca(lstrlen(msg) + 1);
	while ( *c != '\0') {
		        // Strip the font and font size tag
        if (!strnicmp(c,"<font face=",11) || !strnicmp(c,"<font size=",11) || 
			!strnicmp(c, "<font color=",12) || !strnicmp(c,"</font>",6) ||
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
			c++;
		}
	}

	umsg[oidx++]= '\0';
		
	/* Need to strip off formatting stuff first. Then do all decoding/converting */
	if (utf8){	
		Utf8Decode( umsg, 0, &tRealBody );
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
		free( tRealBody );
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
		yahoo_reset_avatar(hContact, 0);
	} else if (DBGetContactSettingDword(hContact, yahooProtocolName,"PictCK", 0) == 0) {
		/* request the buddy image */
		//DBWriteContactSettingDword(hContact, yahooProtocolName, "PictCK", 0);
		YAHOO_request_avatar(who); 
	} 
	
	ProtoBroadcastAck(yahooProtocolName, hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS,NULL, 0);
	
}

void ext_yahoo_rejected(int id, char *who, char *msg)
{
   	char buff[1024]={0};
   	HANDLE hContact;
    LOG(("ext_yahoo_rejected"));
	snprintf(buff, sizeof(buff), Translate("%s has rejected your request and sent the following message:"), who);

	hContact = add_buddy(who, who, 0);
	
    YAHOO_CallService( MS_DB_CONTACT_DELETE, (WPARAM) hContact, 0);	
    YAHOO_remove_buddy(who);
        
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

void ext_yahoo_buddy_added(int id, char *myid, char *who, char *group, int status)
{
	LOG(("[ext_yahoo_buddy_added] %s authorized you as %s group: %s status: %d", who, myid, group, status));
	
}

void ext_yahoo_contact_added(int id, char *myid, char *who, char *msg)
{
	char *szBlob,*pCurBlob;
	char m[1024], m1[5];
   	DWORD dwUin;
	HANDLE hContact=NULL;
	//int found = 0;

	CCSDATA ccs;
	PROTORECVEVENT pre;

    LOG(("ext_yahoo_contact_added %s added you as %s w/ msg '%s'", who, myid, msg));
    
	hContact = add_buddy(who, who, PALF_TEMPORARY);
	
	ccs.szProtoService= PSR_AUTH;
	ccs.hContact=hContact;
	ccs.wParam=0;
	ccs.lParam=(LPARAM)&pre;
	pre.flags=0;
	pre.timestamp=time(NULL);
	
	m1[0]='\0';
	if (msg == NULL)
	  m[0]='\0';
    else
        strcpy(m, msg);
	 
	pre.lParam=sizeof(DWORD)+sizeof(HANDLE)+lstrlen(who)+lstrlen(m1)+lstrlen(m1)+lstrlen(who)+lstrlen(m)+5;
	
	pCurBlob=szBlob=(char *)malloc(pre.lParam);
    /*
       Added blob is: uin(DWORD), hcontact(HANDLE), nick(ASCIIZ), first(ASCIIZ), 
                  last(ASCIIZ), email(ASCIIZ)
                  
       Auth blob is: uin(DWORD),hcontact(HANDLE),nick(ASCIIZ),first(ASCIIZ),
                  last(ASCIIZ),email(ASCIIZ),reason(ASCIIZ)

	//blob is: 0(DWORD), nick(ASCIIZ), ""(ASCIIZ), ""(ASCIIZ), email(ASCIIZ), ""(ASCIIZ)

    */

	// UIN
	dwUin = 0;
    memcpy(pCurBlob,&dwUin,sizeof(DWORD)); 
    pCurBlob+=sizeof(DWORD);

    // hContact
	memcpy(pCurBlob,&hContact,sizeof(HANDLE)); 
    pCurBlob+=sizeof(HANDLE);
    
    // NICK
    strcpy((char *)pCurBlob,who); 
    pCurBlob+=lstrlen((char *)pCurBlob)+1;
    
    // FIRST
    strcpy((char *)pCurBlob,m1); 
    pCurBlob+=lstrlen((char *)pCurBlob)+1;
    
    // LAST
    strcpy((char *)pCurBlob,m1); 
    pCurBlob+=lstrlen((char *)pCurBlob)+1;
    
    // E-mail    
	strcpy((char *)pCurBlob,who); 
	pCurBlob+=lstrlen((char *)pCurBlob)+1;
	
	// Reason
	strcpy((char *)pCurBlob, m); 
	
	pre.szMessage=(char *)szBlob;
	CallService(MS_PROTO_CHAINRECV,0,(LPARAM)&ccs);
}

void ext_yahoo_typing_notify(int id, char *me, char *who, int stat)
{
    const char *c;
    HANDLE hContact;
    
    LOG(("[ext_yahoo_typing_notify] me: '%s' who: '%s' stat: %d ", me, who, stat));

	hContact = getbuddyH(who);
	if (!hContact) return;
	
	c = YAHOO_GetContactName(hContact);
	
	CallService(MS_PROTO_CONTACTISTYPING, (WPARAM)hContact, (LPARAM)stat?10:0);

    if (!YAHOO_GetByte("DisplayTyping", 0)) return;

	if(ServiceExists( MS_POPUP_ADDPOPUPEX ))
		YAHOO_ShowPopup( c, Translate( "typing..." ), YAHOO_ALLOW_MSGBOX + YAHOO_NOTIFY_POPUP );
}

void ext_yahoo_game_notify(int id, char *me, char *who, int stat, char *msg)
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
		char z[1024];
		char *c, *l = msg, *u = NULL;
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
			
			lstrcat(z, "\r\n\r\n");
			lstrcat(z, "http://games.yahoo.com/games/");
			c = strchr(u, 0x09);
			(*c) = '\0';
			lstrcat(z, u);
		}
		
		YAHOO_SetString(hContact, "YGMsg", z);
		
	} else {
		/* ? no information / reset custom message */
		YAHOO_SetString(hContact, "YGMsg", "");
	}
}

void ext_yahoo_mail_notify(int id, char *from, char *subj, int cnt)
{
    SkinPlaySound( Translate( "mail" ) );

    if (!YAHOO_GetByte( "DisableYahoomail", 0)) {    
        LOG(("ext_yahoo_mail_notify"));
        
        if(ServiceExists( MS_POPUP_ADDPOPUPEX )) {
	        char z[256], title[31];
    
			if (from == NULL) {
                strcpy(title, Translate("New Mail"));
                wsprintf(z, Translate("You Have %i unread msgs"), cnt);
			} else {
                wsprintf(title, Translate("New Mail (%i msgs)"), cnt);
                _snprintf(z, 256, Translate("From: %s\nSubject: %s"), from, subj);
			}

            YAHOO_ShowPopup( title, z, YAHOO_ALLOW_ENTER + YAHOO_ALLOW_MSGBOX + YAHOO_MAIL_POPUP );
		}
    }
}    
    
/* WEBCAM callbacks */
void ext_yahoo_got_webcam_image(int id, const char *who,
		const unsigned char *image, unsigned int image_size, unsigned int real_size,
		unsigned int timestamp)
{
    LOG(("ext_yahoo_got_webcam_image"));
}

void ext_yahoo_webcam_viewer(int id, char *who, int connect)
{
    LOG(("ext_yahoo_webcam_viewer"));
}

void ext_yahoo_webcam_closed(int id, char *who, int reason)
{
    LOG(("ext_yahoo_webcam_closed"));
}

void ext_yahoo_webcam_data_request(int id, int send)
{
    LOG(("ext_yahoo_webcam_data_request"));
}

void ext_yahoo_webcam_invite(int id, char *me, char *from)
{
    LOG(("ext_yahoo_webcam_invite"));
	
	ext_yahoo_got_im(id, me, from, Translate("[miranda] Got webcam invite. (not currently supported)"), 0, 0, 0, -1);
}

void ext_yahoo_webcam_invite_reply(int id, char *me, char *from, int accept)
{
    LOG(("ext_yahoo_webcam_invite_reply"));
}

void ext_yahoo_system_message(int id, char *msg)
{
	LOG(("Yahoo System Message: %s", msg));
}

void ext_yahoo_got_file(int id, char *me, char *who, char *url, long expires, char *msg, char *fname, unsigned long fesize, char *ft_token)
{
    CCSDATA ccs;
    PROTORECVEVENT pre;
	HANDLE hContact;
	char *szBlob;
	y_filetransfer *ft;
	
    LOG(("[ext_yahoo_got_file] id: %i, ident:%s, who: %s, url: %s, expires: %lu, msg: %s, fname: %s, fsize: %lu ftoken: %s", id, me, who, url, expires, msg, fname, fesize, ft_token == NULL ? "NULL" : ft_token));
	
	hContact = getbuddyH(who);
	if (hContact == NULL) 
		hContact = add_buddy(who, who, PALF_TEMPORARY);
	
	ft= (y_filetransfer*) malloc(sizeof(y_filetransfer));
	ft->who = strdup(who);
	ft->hWaitEvent = INVALID_HANDLE_VALUE;
	if (msg != NULL)
		ft->msg = strdup(msg);
	else
		ft->msg = strdup("[no description given]");
	
	ft->hContact = hContact;
	if (fname != NULL)
		ft->filename = strdup(fname);
	else {
		char *start, *end;
		
		/* based on how gaim does this */
		start = strrchr(url, '/');
		if (start)
			start++;
		
		end = strrchr(url, '?');
		
		if (start && *start && end) {
			/* argh WINDOWS SUCKS!!! */
			//ft->filename = strndup(start, end-start);
			ft->filename = (char *)malloc(end - start + 1);
			strncpy(ft->filename, start, end-start);
			ft->filename[end-start] = '\0';
		} else 
			ft->filename = strdup("filename.ext");
	}
	
	ft->url = strdup(url);
	ft->fsize = fesize;
	ft->cancel = 0;
	ft->ftoken = (ft_token == NULL) ? NULL : strdup(ft_token);
	
    // blob is DWORD(*ft), ASCIIZ(filenames), ASCIIZ(description)
    szBlob = (char *) malloc(sizeof(DWORD) + lstrlen(ft->filename) + lstrlen(ft->msg) + 2);
    *((PDWORD) szBlob) = (DWORD) ft;
    strcpy(szBlob + sizeof(DWORD), ft->filename);
    strcpy(szBlob + sizeof(DWORD) + lstrlen(ft->filename) + 1, ft->msg);

	pre.flags = 0;
	pre.timestamp = time(NULL);
    pre.szMessage = szBlob;
    pre.lParam = 0;
    ccs.szProtoService = PSR_FILE;
    ccs.hContact = ft->hContact;
    ccs.wParam = 0;
    ccs.lParam = (LPARAM) & pre;
    CallService(MS_PROTO_CHAINRECV, 0, (LPARAM) & ccs);
	free(szBlob);
}

void ext_yahoo_got_identities(int id, YList * ids)
{
    LOG(("ext_yahoo_got_identities"));
    /* FIXME - Not implemented - Got list of Yahoo! identities */
    /* We currently only use the default identity */
    /* Also Stubbed in Sample Client */
	
}

extern char *Bcookie;
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
    YAHOO_CallService(MS_NETLIB_SETSTICKYHEADERS, (WPARAM)hNetlibUser, (LPARAM)z);*/
	
	if (YAHOO_GetByte( "UseYAB", 1 )) {
		LOG(("GET YAB [Before final check] "));
		if (yahooStatus != ID_STATUS_OFFLINE)
			yahoo_get_yab(id);
	}
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
		//int status;
	
		LOG(("[ext_yahoo_got_ping] We are connecting. Checking for different status. Start: %d, Current: %d", gStartStatus, yahooStatus));
		//status = DBGetContactSettingWord(NULL, yahooProtocolName, "StartupStatus", ID_STATUS_ONLINE);
		if (gStartStatus != yahooStatus) {
			LOG(("[COOKIES] Updating Status to %d ", gStartStatus));    
			yahoo_util_broadcaststatus(gStartStatus);
			
			if (gStartStatus != ID_STATUS_INVISIBLE) {// don't generate a bogus packet for Invisible state
				if (szStartMsg != NULL) {
					yahoo_set_status(YAHOO_STATUS_CUSTOM, szStartMsg, (gStartStatus != ID_STATUS_ONLINE) ? 1 : 0);
				} else
				    yahoo_set_status(gStartStatus, NULL, (gStartStatus != ID_STATUS_ONLINE) ? 1 : 0);
			}
			
			yahooLoggedIn=TRUE;
		}
	}
}

void ext_yahoo_login_response(int id, int succ, char *url)
{
	char buff[1024];

	LOG(("ext_yahoo_login_response"));
	
	if(succ == YAHOO_LOGIN_OK) {
		ylad->status = yahoo_current_status(id);
		LOG(("logged in status-> %d", ylad->status));
		return;
	} else if(succ == YAHOO_LOGIN_UNAME) {

		snprintf(buff, sizeof(buff), Translate("Could not log into Yahoo service - username not recognised.  Please verify that your username is correctly typed."));
	} else if(succ == YAHOO_LOGIN_PASSWD) {

		snprintf(buff, sizeof(buff), Translate("Could not log into Yahoo service - password incorrect.  Please verify that your username and password are correctly typed."));

	} else if(succ == YAHOO_LOGIN_LOCK) {
		
		snprintf(buff, sizeof(buff), Translate("Could not log into Yahoo service.  Your account has been locked.\nVisit %s to reactivate it."), url);

	} else if(succ == YAHOO_LOGIN_DUPL) {
		snprintf(buff, sizeof(buff), Translate("You have been logged out of the yahoo service, possibly due to a duplicate login."));
		ProtoBroadcastAck(yahooProtocolName, NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_OTHERLOCATION);
	}else if(succ == -1) {
		/// Can't Connect or got disconnected.
		if (yahooStatus == ID_STATUS_CONNECTING)
			snprintf(buff, sizeof(buff), Translate("Could not connect to the Yahoo service. Check your server/port and proxy settings."));	
		else
			return;
	} else {
		snprintf(buff, sizeof(buff),Translate("Could not log in, unknown reason: %d."), succ);
	}

	YAHOO_DebugLog(buff);
	
	//poll_loop = 0; -- do we need this??
	/*
       yahoo_logout(); -- The following Line MAKES us LOOP and CPU 100%
     */
	//
	// Show Error Message
	//
	YAHOO_ShowError(Translate("Yahoo Login Error"), buff);
}

void ext_yahoo_error(int id, char *err, int fatal, int num)
{
	char buff[1024];
	
	LOG(("Yahoo Error: id: %d, fatal: %d, num: %d, %s", id, fatal, num, err));
        
	switch(num) {
		case E_UNKNOWN:
			snprintf(buff, sizeof(buff), Translate("unknown error %s"), err);
			break;
		case E_CUSTOM:
			snprintf(buff, sizeof(buff), Translate("custom error %s"), err);
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
			snprintf(buff, sizeof(buff), Translate("system error %s"), err);
			break;
		case E_CONNECTION:
			snprintf(buff, sizeof(buff), Translate("server connection error %s"), err);
			break;
	}
	
	
	if(fatal)
		yahoo_logout();
	
	YAHOO_DebugLog(buff);
	
	//poll_loop = 0; -- do we need this??
	/*
       yahoo_logout(); -- The following Line MAKES us LOOP and CPU 100%
     */
	//
	// Show Error Message
	//
	if (yahooStatus != ID_STATUS_OFFLINE) {
		// Show error only if we are not offline. [manual status changed]
		YAHOO_ShowError(Translate("Yahoo Error"), buff);
	}

}

int ext_yahoo_connect(char *h, int p)
{
	NETLIBOPENCONNECTION ncon = {0};
    HANDLE con;
    
	LOG(("ext_yahoo_connect %s:%d", h, p));
	
    ncon.cbSize = sizeof(ncon); 
	ncon.szHost = h;
    ncon.wPort = p;
    
    con = (HANDLE) CallService(MS_NETLIB_OPENCONNECTION, (WPARAM) hNetlibUser, (LPARAM) & ncon);
    if (con == NULL)  {
		LOG(("ERROR: Connect Failed!"));
        return -1;
	}

    return (int)con;
}

/*************************************
 * Callback handling code starts here
 */
YList *connections = NULL;
static int connection_tags=0;

int ext_yahoo_add_handler(int id, int fd, yahoo_input_condition cond, void *data)
{
	struct _conn *c = y_new0(struct _conn, 1);
	c->tag = ++connection_tags;
	c->id = id;
	c->fd = fd;
	c->cond = cond;
	c->data = data;

	LOG(("Add %d for %d, tag %d", fd, id, c->tag));

	connections = y_list_prepend(connections, c);

	return c->tag;
}

void ext_yahoo_remove_handler(int id, int tag)
{
	YList *l;
	LOG(("ext_yahoo_remove_handler id:%d tag:%d ", id, tag));
	
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

	LOG(("[yahoo_callback] id: %d, fd: %d", c->id, c->fd));
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
}

int ext_yahoo_connect_async(int id, char *host, int port, 
		yahoo_connect_callback callback, void *data)
{
    int res;
    
    LOG(("ext_yahoo_connect_async %s:%d", host, port));
    
    res = ext_yahoo_connect(host, port);

	//if(res >= 0 ) {
		//LOG(("Connected fd: %d, error: %d", hServerConn, error));
		
		callback(res, 0, data);
		return 0;
	/*} else {
		//close(servfd);
		
		return -1;
	}*/

    
    
}
/*
 * Callback handling code ends here
 ***********************************/
void ext_yahoo_chat_yahoologout(int id)
{ 
 	LOG(("got chat logout"));
}
void ext_yahoo_chat_yahooerror(int id)
{ 
 	LOG(("got chat error"));
}

void ext_yahoo_got_search_result(int id, int found, int start, int total, YList *contacts)
{
    PROTOSEARCHRESULT psr;
	struct yahoo_found_contact *yct=NULL;
	char *c;
    YList *en=contacts;

	LOG(("got search result: "));
	
	LOG(("ID: %d", id));
	LOG(("Found: %d", found));
	LOG(("Start: %d", start));
	LOG(("Total: %d", total));
		
	//int i;
	
/*    if (aim_util_isme(sn)) {
        ProtoBroadcastAck(AIM_PROTO, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
        return;
    }
    */
	
    ZeroMemory(&psr, sizeof(psr));
    psr.cbSize = sizeof(psr);
	
	while (en) {
		yct = en->data;
    //psr.nick = (char *)snsearch;
		psr.nick = (char *)yct->id;
		c = (char *)malloc(10);
		itoa(yct->age, c,10);
		psr.firstName = yct->gender;
		psr.lastName = (char *)c;
		psr.email = (char *)yct->location;
    //psr.email = (char *)snsearch;
    
	//void yahoo_search(int id, enum yahoo_search_type t, const char *text, enum yahoo_search_gender g, enum yahoo_search_agerange ar, 
	//	int photo, int yahoo_only)

		YAHOO_SendBroadcast(NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE) 1, (LPARAM) & psr);
		en = y_list_next(en);
	}
    YAHOO_SendBroadcast(NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
}


void register_callbacks()
{
	static struct yahoo_callbacks yc;

	yc.ext_yahoo_login_response = ext_yahoo_login_response;
	yc.ext_yahoo_got_buddies = ext_yahoo_got_buddies;
	yc.ext_yahoo_got_ignore = ext_yahoo_got_ignore;
	yc.ext_yahoo_got_identities = ext_yahoo_got_identities;
	yc.ext_yahoo_got_cookies = ext_yahoo_got_cookies;
	yc.ext_yahoo_status_changed = ext_yahoo_status_changed;
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

	yc.ext_yahoo_got_stealthlist = ext_yahoo_got_stealth;
	yc.ext_yahoo_got_ping  = ext_yahoo_got_ping;
	yc.ext_yahoo_got_picture  = ext_yahoo_got_picture;
	yc.ext_yahoo_got_picture_checksum = ext_yahoo_got_picture_checksum;
	yc.ext_yahoo_got_picture_update = ext_yahoo_got_picture_update;
	yc.ext_yahoo_got_nick = ext_yahoo_got_nick;
	
	yc.ext_yahoo_buddy_added = ext_yahoo_buddy_added;
	yc.ext_yahoo_cleanup = ext_yahoo_cleanup;
	yc.ext_yahoo_got_picture_upload = ext_yahoo_got_picture_upload;
	yc.ext_yahoo_got_avatar_update = ext_yahoo_got_avatar_update;
	yc.ext_yahoo_got_audible = ext_yahoo_got_audible;
	
	yahoo_register_callbacks(&yc);
	
}

void YAHOO_ping(void) 
{
	LOG(("[TIMER] yahoo_ping_timeout"));
	
	if (yahooLoggedIn && (ylad != NULL) && (ylad->id > 0) ) {
		LOG(("[TIMER] Sending a keep alive message"));
		yahoo_keepalive(ylad->id);
	} 
}

void ext_yahoo_login(int login_mode)
{
	char host[128];
	int port=0;
    DBVARIANT dbv;

	LOG(("ext_yahoo_login"));

	if (!DBGetContactSetting(NULL, yahooProtocolName, YAHOO_LOGINSERVER, &dbv)) {
        _snprintf(host, sizeof(host), "%s", dbv.pszVal);
        DBFreeVariant(&dbv);
    }
    else {
        //_snprintf(host, sizeof(host), "%s", pager_host);
       	if (YAHOO_hasnotification())
       		 YAHOO_shownotification(Translate("Yahoo Login Error"), Translate("Please enter Yahoo server to Connect to in Options."), NIIF_ERROR);
	    else
	         MessageBox(NULL, Translate("Please enter Yahoo server to Connect to in Options."), Translate("Yahoo Login Error"), MB_OK | MB_ICONINFORMATION);

        return;
    }

	port = DBGetContactSettingWord(NULL, yahooProtocolName, YAHOO_LOGINPORT, 5050);
	
	//ylad->id = yahoo_init(ylad->yahoo_id, ylad->password);
	ylad->id = yahoo_init_with_attributes(ylad->yahoo_id, ylad->password, 
			"pager_host", host,
			"pager_port", port,
			"picture_checksum", YAHOO_GetDword("AvatarHash", -1),
			NULL);

	ylad->status = YAHOO_STATUS_OFFLINE;
	yahoo_login(ylad->id, login_mode);

	if (ylad == NULL || ylad->id <= 0) {
		LOG(("Could not connect to Yahoo server.  Please verify that you are connected to the net and the pager host and port are correctly entered."));
		return;
	}

	//rearm(&pingTimer, 600);
}

void YAHOO_refresh() 
{
	yahoo_refresh(ylad->id);
}



