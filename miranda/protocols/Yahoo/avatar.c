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
#include <io.h>
#include <fcntl.h>

#include "yahoo.h"
#include "resource.h"

#include <m_langpack.h>
#include <m_options.h>
#include <m_userinfo.h>
#include <m_png.h>
#include <m_system.h>

#include "avatar.h"
#include "file_transfer.h"

static BOOL CALLBACK AvatarDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

extern yahoo_local_account *ylad;

/*
 *31 bit hash function  - this is based on g_string_hash function from glib
 */

int YAHOO_avt_hash(const char *key, long ksize)
{
  const char *p = key;
  int h = *p;
  long l = 0;
  
  if (h)
	for (p += 1; l < ksize; p++, l++)
	  h = (h << 5) - h + *p;

  return h;
}

void upload_avt(int id, int fd, int error, void *data);

/**************** Send Avatar ********************/

void upload_avt(int id, int fd, int error, void *data)
{
    y_filetransfer *sf = (y_filetransfer*) data;
    long size = 0;
	char buf[1024];
	int rw;			/* */
	DWORD dw;		/* needed for ReadFile */
	HANDLE myhFile;
	
	if (fd < 1 || error) {
		LOG(("[get_fd] Connect Failed!"));
		return;
	}

    myhFile  = CreateFile(sf->filename,
                          GENERIC_READ,
                          FILE_SHARE_READ|FILE_SHARE_WRITE,
			           NULL,
			           OPEN_EXISTING,
			           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
			           0);

	if (myhFile == INVALID_HANDLE_VALUE) {
		LOG(("[get_fd] Can't open file for reading?!"));
		return;
	}
	
	LOG(("Sending file: %s size: %ld", sf->filename, sf->fsize));
	
	do {
		rw = ReadFile(myhFile, buf, sizeof(buf), &dw, NULL);
	
		if (rw != 0) {
			rw = Netlib_Send((HANDLE)fd, buf, dw, MSG_NODUMP);
			
			if (rw < 1) {
				LOG(("Upload Failed. Send error?"));
				YAHOO_ShowError(Translate("Yahoo Error"), Translate("Avatar upload failed!?!"));
				break;
			}
			
			size += rw;
		}
	} while (rw >= 0 && size < sf->fsize);
	
	CloseHandle(myhFile);
	
	do {
		rw = Netlib_Recv((HANDLE)fd, buf, sizeof(buf), 0);
		LOG(("Got: %d bytes", rw));
	} while (rw > 0);
	
    LOG(("File send complete!"));
}

void __cdecl yahoo_send_avt_thread(void *psf) 
{
	y_filetransfer *sf = psf;
	
	if (sf == NULL) {
		YAHOO_DebugLog("[yahoo_send_avt_thread] SF IS NULL!!!");
		return;
	}
	
	YAHOO_SetByte("AvatarUL", 1);
	yahoo_send_avatar(ylad->id, sf->filename, sf->fsize, &upload_avt, sf);

	free(sf->filename);
	free(sf);
	if (YAHOO_GetByte("AvatarUL", 1) == 1) YAHOO_SetByte("AvatarUL", 0);
}

void YAHOO_SendAvatar(const char *szFile)
{
	y_filetransfer *sf;
	struct _stat statbuf;
	
	if ( _stat( szFile, &statbuf ) != 0 ) {
		LOG(("[YAHOO_SendAvatar] Error reading File information?!"));
		return;
	}

	sf = (y_filetransfer*) malloc(sizeof(y_filetransfer));
	sf->filename = strdup(szFile);
	sf->cancel = 0;
	sf->fsize = statbuf.st_size;
	
	YAHOO_DebugLog("[Uploading avatar] filename: %s size: %ld", sf->filename, sf->fsize);

	mir_forkthread(yahoo_send_avt_thread, sf);
}

struct avatar_info{
	char *who;
	char *pic_url;
	int cksum;
};

static void __cdecl yahoo_recv_avatarthread(void *pavt) 
{
	PROTO_AVATAR_INFORMATION AI;
	struct avatar_info *avt = pavt;
	int 	error = 0;
	HANDLE 	hContact = 0;
	char 	buf[4096];
	
	if (avt == NULL) {
		YAHOO_DebugLog("AVT IS NULL!!!");
		return;
	}

	if (!yahooLoggedIn) {
		YAHOO_DebugLog("We are not logged in!!!");
		return;
	}
	
//    ProtoBroadcastAck(yahooProtocolName, hContact, ACKTYPE_GETINFO, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
	
	LOG(("yahoo_recv_avatarthread who:%s url:%s checksum: %d", avt->who, avt->pic_url, avt->cksum));

	hContact = getbuddyH(avt->who);
		
	if (!hContact){
		LOG(("ERROR: Can't find buddy: %s", avt->who));
		error = 1;
	} else if (!error) {
		DBWriteContactSettingDword(hContact, yahooProtocolName, "PictCK", avt->cksum);
		DBWriteContactSettingDword(hContact, yahooProtocolName, "PictLoading", 1);
	}
	
    if(!error) {

    NETLIBHTTPREQUEST nlhr={0},*nlhrReply;
	
	nlhr.cbSize		= sizeof(nlhr);
	nlhr.requestType= REQUEST_GET;
	nlhr.flags		= NLHRF_NODUMP|NLHRF_GENERATEHOST|NLHRF_SMARTAUTHHEADER;
	nlhr.szUrl		= avt->pic_url;

	nlhrReply=(NETLIBHTTPREQUEST*)CallService(MS_NETLIB_HTTPTRANSACTION,(WPARAM)hNetlibUser,(LPARAM)&nlhr);

	if(nlhrReply) {
		
		if (nlhrReply->resultCode != 200) {
			LOG(("Update server returned '%d' instead of 200. It also sent the following: %s", nlhrReply->resultCode, nlhrReply->szResultDescr));
			// make sure it's a real problem and not a problem w/ our connection
			yahoo_send_picture_info(ylad->id, avt->who, 3, avt->pic_url, avt->cksum);
			error = 1;
		} else if (nlhrReply->dataLength < 1 || nlhrReply->pData == NULL) {
			LOG(("No data??? Got %d bytes.", nlhrReply->dataLength));
			error = 1;
		} else {
			HANDLE myhFile;

			GetAvatarFileName(hContact, buf, 1024, DBGetContactSettingByte(hContact, yahooProtocolName,"AvatarType", 0));
			DeleteFile(buf);
			
			LOG(("Saving file: %s size: %u", buf, nlhrReply->dataLength));
			myhFile = CreateFile(buf,
								GENERIC_WRITE,
								FILE_SHARE_WRITE,
								NULL, OPEN_ALWAYS,  FILE_ATTRIBUTE_NORMAL,  0);
	
			if(myhFile !=INVALID_HANDLE_VALUE) {
				DWORD c;
				
				WriteFile(myhFile, nlhrReply->pData, nlhrReply->dataLength, &c, NULL );
				CloseHandle(myhFile);
				
				DBWriteContactSettingDword(hContact, yahooProtocolName, "PictLastCheck", 0);
			} else {
				LOG(("Can not open file for writing: %s", buf));
				error = 1;
			}
		}
		CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT,0,(LPARAM)nlhrReply);
	}
	}
	
	if (DBGetContactSettingDword(hContact, yahooProtocolName, "PictCK", 0) != avt->cksum) {
		LOG(("WARNING: Checksum updated during download?!"));
		error = 1; /* don't use this one? */
	} 
    
	DBWriteContactSettingDword(hContact, yahooProtocolName, "PictLoading", 0);
	LOG(("File download complete!?"));

	if (error) 
		buf[0]='\0';
	
	free(avt->who);
	free(avt->pic_url);
	free(avt);
	
	AI.cbSize = sizeof AI;
	AI.format = PA_FORMAT_PNG;
	AI.hContact = hContact;
	lstrcpy(AI.filename,buf);

	if (error) 
		DBWriteContactSettingDword(hContact, yahooProtocolName, "PictCK", 0);
	
	ProtoBroadcastAck(yahooProtocolName, hContact, ACKTYPE_AVATAR, !error ? ACKRESULT_SUCCESS:ACKRESULT_FAILED,(HANDLE) &AI, 0);

}

void YAHOO_get_avatar(const char *who, const char *pic_url, long cksum)
{
	struct avatar_info *avt;
	
	avt = malloc(sizeof(struct avatar_info));
	avt->who = _strdup(who);
	avt->pic_url = _strdup(pic_url);
	avt->cksum = cksum;
	
	mir_forkthread(yahoo_recv_avatarthread, (void *) avt);
}

void ext_yahoo_got_picture(int id, const char *me, const char *who, const char *pic_url, int cksum, int type)
{
	HANDLE 	hContact = 0;
		
	LOG(("[ext_yahoo_got_picture] for %s with url %s (checksum: %d) type: %d", who, pic_url, cksum, type));
	
	
	/*
	  Type:
	
		1 - Send Avatar Info
		2 - Got Avatar Info
		3 - YIM6 didn't like my avatar? Expired? We need to invalidate and re-load
	 */
	switch (type) {
	case 1: 
		{
			int cksum=0;
			DBVARIANT dbv;
			
			/* need to send avatar info */
			if (!YAHOO_GetByte( "ShowAvatars", 0 )) {
				LOG(("[ext_yahoo_got_picture] We are not using/showing avatars!"));
				yahoo_send_picture_update(id, who, 0); // no avatar (disabled)
				return;
			}
		
			LOG(("[ext_yahoo_got_picture] Getting ready to send info!"));
			/* need to read CheckSum */
			cksum = YAHOO_GetDword("AvatarHash", 0);
			if (cksum) {
				if (!DBGetContactSetting(NULL, yahooProtocolName, "AvatarURL", &dbv)) {
					LOG(("[ext_yahoo_got_picture] Sending url: %s checksum: %d to '%s'!", dbv.pszVal, cksum, who));
					//void yahoo_send_picture_info(int id, const char *me, const char *who, const char *pic_url, int cksum)
					yahoo_send_picture_info(id, who, 2, dbv.pszVal, cksum);
					DBFreeVariant(&dbv);
					break;
				} else
					LOG(("No AvatarURL???"));
				
				/*
				 * Try to re-upload the avatar
				 */
				if (YAHOO_GetByte("AvatarUL", 0) != 1){
					// NO avatar URL??
					if (!DBGetContactSetting(NULL, yahooProtocolName, "AvatarFile", &dbv)) {
						struct _stat statbuf;
						
						if (_stat( dbv.pszVal, &statbuf ) != 0 ) {
							LOG(("[ext_yahoo_got_picture] Avatar File Missing? Can't find file: %s", dbv.pszVal));
						} else {
							DBWriteContactSettingString(NULL, yahooProtocolName, "AvatarInv", who);
							YAHOO_SendAvatar(dbv.pszVal);
						}
						
						DBFreeVariant(&dbv);
					} else {
						LOG(("[ext_yahoo_got_picture] No Local Avatar File??? "));
					}
				} else 
						LOG(("[ext_yahoo_got_picture] Another avatar upload in progress?"));
			}
		}
		break;
	case 2: /*
		     * We got Avatar Info for our buddy. 
		     */
			if (!YAHOO_GetByte( "ShowAvatars", 0 )) {
				LOG(("[ext_yahoo_got_picture] We are not using/showing avatars!"));
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
				DBWriteContactSettingDword(hContact, yahooProtocolName, "PictCK", 0);
				
				yahoo_reset_avatar(hContact);
			} else {
				char z[1024];
				
				if (pic_url == NULL) {
					LOG(("[ext_yahoo_got_picture] WARNING: Empty URL for avatar?"));
					return;
				}
				
				GetAvatarFileName(hContact, z, 1024, DBGetContactSettingByte(hContact, yahooProtocolName,"AvatarType", 0));
				
				if (DBGetContactSettingDword(hContact, yahooProtocolName,"PictCK", 0) != cksum || _access( z, 0 ) != 0 ) {
					
					YAHOO_DebugLog("[ext_yahoo_got_picture] Checksums don't match or avatar file is missing. Current: %d, New: %d",(int)DBGetContactSettingDword(hContact, yahooProtocolName,"PictCK", 0), cksum);

					YAHOO_get_avatar(who, pic_url, cksum);
				}
			}

		break;
	case 3: 
		/*
		 * Our Avatar is not good anymore? Need to re-upload??
		 */
		 /* who, pic_url, cksum */
		{
			int mcksum=0;
			DBVARIANT dbv;
			
			/* need to send avatar info */
			if (!YAHOO_GetByte( "ShowAvatars", 0 )) {
				LOG(("[ext_yahoo_got_picture] We are not using/showing avatars!"));
				yahoo_send_picture_update(id, who, 0); // no avatar (disabled)
				return;
			}
		
			LOG(("[ext_yahoo_got_picture] Getting ready to send info!"));
			/* need to read CheckSum */
			mcksum = YAHOO_GetDword("AvatarHash", 0);
			if (mcksum == 0) {
				/* this should NEVER Happen??? */
				LOG(("[ext_yahoo_got_picture] No personal checksum? and Invalidate?!"));
				yahoo_send_picture_update(id, who, 0); // no avatar (disabled)
				return;
			}
			
			LOG(("[ext_yahoo_got_picture] My Checksum: %d", mcksum));
			
			if (!DBGetContactSetting(NULL, yahooProtocolName, "AvatarURL", &dbv)){
					if (lstrcmpi(pic_url, dbv.pszVal) == 0) {
						DBVARIANT dbv2;
						/*time_t  ts;
						DWORD	ae;*/
						
						if (mcksum != cksum)
							LOG(("[ext_yahoo_got_picture] WARNING: Checksums don't match!"));	
						
						/*time(&ts);
						ae = YAHOO_GetDword("AvatarExpires", 0);
						
						if (ae != 0 && ae > (ts - 300)) {
							LOG(("[ext_yahoo_got_picture] Current Time: %lu Expires: %lu ", ts, ae));
							LOG(("[ext_yahoo_got_picture] We just reuploaded! Stop screwing with Yahoo FT. "));
							
							// don't leak stuff
							DBFreeVariant(&dbv);

							break;
						}*/
						
						LOG(("[ext_yahoo_got_picture] Buddy: %s told us this is bad??Expired??. Re-uploading", who));
						DBDeleteContactSetting(NULL, yahooProtocolName, "AvatarURL");
						
						if (!DBGetContactSetting(NULL, yahooProtocolName, "AvatarFile", &dbv2)) {
							DBWriteContactSettingString(NULL, yahooProtocolName, "AvatarInv", who);
							YAHOO_SendAvatar(dbv2.pszVal);
							DBFreeVariant(&dbv2);
						} else {
							LOG(("[ext_yahoo_got_picture] No Local Avatar File??? "));
						}
					} else {
						LOG(("[ext_yahoo_got_picture] URL doesn't match? Tell them the right thing!!!"));
						yahoo_send_picture_info(id, who, 2, dbv.pszVal, mcksum);
					}
					// don't leak stuff
					DBFreeVariant(&dbv);
			} else {
				LOG(("[ext_yahoo_got_picture] no AvatarURL?"));
			}
		}
		break;
	default:
		LOG(("[ext_yahoo_got_picture] Unknown request/packet type exiting!"));
	}
	
	LOG(("ext_yahoo_got_picture exiting"));
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
		DBWriteContactSettingDword(hContact, yahooProtocolName, "PictCK", 0);
		
        yahoo_reset_avatar(hContact);
	} else {
		if (DBGetContactSettingDword(hContact, yahooProtocolName,"PictCK", 0) != cksum) {
			//char szFile[MAX_PATH];

			// Now save the new checksum. No rush requesting new avatar yet.
			DBWriteContactSettingDword(hContact, yahooProtocolName, "PictCK", cksum);
			
			// Need to delete the Avatar File!!
			/*GetAvatarFileName(hContact, szFile, sizeof szFile, 0);
			DeleteFile(szFile);
			
			// Reset the avatar and cleanup.
			yahoo_reset_avatar(hContact);*/
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
	yahoo_reset_avatar(hContact);
}

void ext_yahoo_got_picture_status(int id, const char *me, const char *who, int buddy_icon)
{
	HANDLE 	hContact = 0;

	LOG(("ext_yahoo_got_picture_status for %s buddy_icon: %d", who, buddy_icon));

	hContact = getbuddyH(who);
	if (hContact == NULL) {
		LOG(("Buddy Not Found. Skipping avatar update"));
		return;
	}
	
	DBWriteContactSettingByte(hContact, yahooProtocolName, "AvatarType", buddy_icon);
	
	/* Last thing check the checksum and request new one if we need to */
	yahoo_reset_avatar(hContact);
}

void yahoo_reset_avatar(HANDLE 	hContact)
{
	LOG(("[YAHOO_RESET_AVATAR]"));

	ProtoBroadcastAck(yahooProtocolName, hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS, NULL, 0);
}

void YAHOO_request_avatar(const char* who)
{
	time_t  last_chk, cur_time;
	HANDLE 	hContact = 0;
	//char    szFile[MAX_PATH];
	
	if (!YAHOO_GetByte( "ShowAvatars", 0 )) {
		LOG(("Avatars disabled, but available for: %s", who));
		return;
	}
	
	hContact = getbuddyH(who);
	
	if (!hContact)
		return;
	
	/*GetAvatarFileName(hContact, szFile, sizeof szFile, DBGetContactSettingByte(hContact, yahooProtocolName,"AvatarType", 0));
	DeleteFile(szFile);*/
	
	time(&cur_time);
	last_chk = DBGetContactSettingDword(hContact, yahooProtocolName, "PictLastCheck", 0);
	
	/*
	 * time() - in seconds ( 60*60 = 1 hour)
	 */
	if (DBGetContactSettingDword(hContact, yahooProtocolName,"PictCK", 0) == 0 || 
		last_chk == 0 || (cur_time - last_chk) > 60) {
			
		DBWriteContactSettingDword(hContact, yahooProtocolName, "PictLastCheck", cur_time);

		LOG(("Requesting Avatar for: %s", who));
		yahoo_request_buddy_avatar(ylad->id, who);
	} else {
		LOG(("Avatar Not Available for: %s Last Check: %ld Current: %ld (Flood Check in Effect)", who, last_chk, cur_time));
	}
}

void YAHOO_bcast_picture_update(int buddy_icon)
{
	HANDLE hContact;
	char *szProto;
	
	/* need to get online buddies and then send then picture_update packets (ARGH YAHOO!)*/
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

				yahoo_send_picture_update(ylad->id, dbv.pszVal, buddy_icon);
				DBFreeVariant( &dbv );
			}
		}
	}
}

void YAHOO_set_avatar(int buddy_icon)
{
	yahoo_send_picture_status(ylad->id,buddy_icon);
	
	YAHOO_bcast_picture_update(buddy_icon);
}

void YAHOO_bcast_picture_checksum(int cksum)
{
	HANDLE hContact;
	char *szProto;
	
	yahoo_send_picture_checksum(ylad->id, NULL, cksum);
	
	/* need to get online buddies and then send then picture_update packets (ARGH YAHOO!)*/
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

				yahoo_send_picture_checksum(ylad->id, dbv.pszVal, cksum);
				DBFreeVariant( &dbv );
			}
		}
	}
}

void GetAvatarFileName(HANDLE hContact, char* pszDest, int cbLen, int type)
{
  CallService(MS_DB_GETPROFILEPATH, cbLen, (LPARAM)pszDest);

  lstrcat(pszDest, "\\");
  lstrcat(pszDest, yahooProtocolName);
  CreateDirectory(pszDest, NULL);

  if (hContact != NULL) {
	int ck_sum = DBGetContactSettingDword(hContact, yahooProtocolName,"PictCK", 0);
	
	_snprintf(pszDest, cbLen, "%s\\%lX", pszDest, ck_sum);
  }else {
	lstrcat(pszDest, "\\avatar");
  }

  if (type == 1) {
	lstrcat(pszDest, ".swf" );
  } else
	lstrcat(pszDest, ".png" );
  
}

int YahooGetAvatarInfo(WPARAM wParam,LPARAM lParam)
{
	PROTO_AVATAR_INFORMATION* AI = ( PROTO_AVATAR_INFORMATION* )lParam;
	DBVARIANT dbv;
	int avtType;
	
	if (!DBGetContactSetting(AI->hContact, yahooProtocolName, YAHOO_LOGINID, &dbv)) {
		YAHOO_DebugLog("[YAHOO_GETAVATARINFO] For: %s", dbv.pszVal);
		DBFreeVariant(&dbv);
	}else {
		YAHOO_DebugLog("[YAHOO_GETAVATARINFO]");
	}

	if (!YAHOO_GetByte( "ShowAvatars", 0 ) || !yahooLoggedIn) {
		YAHOO_DebugLog("[YAHOO_GETAVATARINFO] %s", yahooLoggedIn ? "We are not using/showing avatars!" : "We are not logged in. Can't load avatars now!");
		
		return GAIR_NOAVATAR;
	}
	
	avtType  = DBGetContactSettingByte(AI->hContact, yahooProtocolName,"AvatarType", 0);
	YAHOO_DebugLog("[YAHOO_GETAVATARINFO] Avatar Type: %d", avtType);
	
	if ( avtType != 2) {
		if (avtType != 0)
			YAHOO_DebugLog("[YAHOO_GETAVATARINFO] Not handling this type yet!");
		
		return GAIR_NOAVATAR;
	}
	
	if (DBGetContactSettingDword(AI->hContact, yahooProtocolName,"PictCK", 0) == 0) 
		return GAIR_NOAVATAR;
	
	GetAvatarFileName(AI->hContact, AI->filename, sizeof AI->filename,DBGetContactSettingByte(AI->hContact, yahooProtocolName,"AvatarType", 0));
	AI->format = PA_FORMAT_PNG;
	YAHOO_DebugLog("[YAHOO_GETAVATARINFO] filename: %s", AI->filename);
	
	if (_access( AI->filename, 0 ) == 0 ) 
		return GAIR_SUCCESS;
	
	if (( wParam & GAIF_FORCE ) != 0 && AI->hContact != NULL ) {		
		/* need to request it again? */
		if (YAHOO_GetWord(AI->hContact, "PictLoading", 0) != 0 &&
			(time(NULL) - YAHOO_GetWord(AI->hContact, "PictLastCK", 0) < 500)) {
			YAHOO_DebugLog("[YAHOO_GETAVATARINFO] Waiting for avatar to load!");
			return GAIR_WAITFOR;
		} else if ( yahooLoggedIn ) {
			DBVARIANT dbv;
  
			if (!DBGetContactSetting(AI->hContact, yahooProtocolName, YAHOO_LOGINID, &dbv)) {
				YAHOO_DebugLog("[YAHOO_GETAVATARINFO] Requesting avatar!");
				
				YAHOO_request_avatar(dbv.pszVal);
				DBFreeVariant(&dbv);
				
				return GAIR_WAITFOR;
			} else {
				YAHOO_DebugLog("[YAHOO_GETAVATARINFO] Can't retrieve user id?!");
			}
		}
	}
	
	YAHOO_DebugLog("[YAHOO_GETAVATARINFO] NO AVATAR???");
	return GAIR_NOAVATAR;
}

/*
 * --=[ AVS / LoadAvatars API/Services ]=--
 */

/*
Optional. Will pass PNG or BMP if this is not found
wParam = 0
lParam = PA_FORMAT_*   // avatar format
return = 1 (supported) or 0 (not supported)
*/
int YahooAvatarFormatSupported(WPARAM wParam, LPARAM lParam)
{
  YAHOO_DebugLog("[YahooAvatarFormatSupported]");

	return (lParam == PA_FORMAT_PNG) ? 1 : 0;
}

/*
Service: /GetMyAvatarMaxSize
wParam=(int *)max width of avatar
lParam=(int *)max height of avatar
return=0
*/
int YahooGetAvatarSize(WPARAM wParam, LPARAM lParam)
{
	YAHOO_DebugLog("[YahooGetAvatarSize]");
	
	if (wParam != 0) *((int*) wParam) = 96;
	if (lParam != 0) *((int*) lParam) = 96;

	return 0;
}

/*
Service: /GetMyAvatar
wParam=(char *)Buffer to file name
lParam=(int)Buffer size
return=0 on success, else on error
*/
int YahooGetMyAvatar(WPARAM wParam, LPARAM lParam)
{
	char *buffer = (char *)wParam;
	int size = (int)lParam;

	YAHOO_DebugLog("[YahooGetMyAvatar]");
	
	if (buffer == NULL || size <= 0)
		return -1;
	

	if (!YAHOO_GetByte( "ShowAvatars", 0 ))
		return -2;
	
	{
		DBVARIANT dbv;
		int ret = -3;

		if (YAHOO_GetDword("AvatarHash", 0)){
			
			if (!DBGetContactSetting(NULL, yahooProtocolName, "AvatarFile", &dbv)){
				if (access(dbv.pszVal, 0) == 0){
					strncpy(buffer, dbv.pszVal, size-1);
					buffer[size-1] = '\0';

					ret = 0;
				}
				DBFreeVariant(&dbv);
			}
		}

		return ret;
	}
}

/*
#define PS_SETMYAVATAR "/SetMyAvatar"
wParam=0
lParam=(const char *)Avatar file name
return=0 for sucess
*/

int YahooSetMyAvatar(WPARAM wParam, LPARAM lParam)
{
	char* szFile = ( char* )lParam;

	char szMyFile[MAX_PATH+1];
	GetAvatarFileName(NULL, szMyFile, MAX_PATH, 2);

	if (szFile == NULL) {
		remove(szMyFile);
		DBDeleteContactSetting( NULL, yahooProtocolName, "AvatarHash" );
	}
	else {
		long  dwPngSize;
		BYTE* pResult;
		unsigned int hash;
		int fileId = _open(szFile, _O_RDONLY | _O_BINARY, _S_IREAD);
		if ( fileId == -1 )
			return 1;

		dwPngSize = filelength( fileId );
		if (( pResult = ( BYTE* )malloc( dwPngSize )) == NULL )
			return 2;

		_read( fileId, pResult, dwPngSize );
		_close( fileId );

		fileId = _open( szMyFile, _O_CREAT | _O_TRUNC | _O_WRONLY | O_BINARY,  _S_IREAD | _S_IWRITE );
		if ( fileId > -1 ) {
			_write( fileId, pResult, dwPngSize );
			_close( fileId );
		}

		hash = YAHOO_avt_hash(pResult, dwPngSize);
		if ( hash ) {
			LOG(("[YAHOO_SetAvatar] File: '%s' CK: %d", szMyFile, hash));	

			/* now check and make sure we don't reupload same thing over again */
			if (hash != YAHOO_GetDword("AvatarHash", 0)) {
				YAHOO_SetString(NULL, "AvatarFile", szMyFile);
				DBWriteContactSettingDword(NULL, yahooProtocolName, "TMPAvatarHash", hash);

				YAHOO_SendAvatar(szMyFile);
			} 
			else LOG(("[YAHOO_SetAvatar] Same checksum and avatar on YahooFT. Not Reuploading."));	  
	}	}

	return 0;
}

/*
 * --=[ ]=--
 */
