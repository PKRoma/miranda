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

#include "yahoo.h"

#include <sys/stat.h>

#include <m_langpack.h>
#include "m_folders.h"
#include "avatar.h"
#include "resource.h"

/*
 *31 bit hash function  - this is based on g_string_hash function from glib
 */

int YAHOO_avt_hash(const char *key, DWORD len)
{
	/* 
		Thank you Pidgin and Kopete devs. It seems that both clients are using this code now.
	
	*/

	const unsigned char *p = (const unsigned char *)key;
	int checksum = 0, g, i = len;

	while(i--) {
		checksum = (checksum << 4) + *p++;

		if((g = (checksum & 0xf0000000)) != 0)
			checksum ^= g >> 23;

		checksum &= ~g;
	}
	
	return checksum;
}

/**************** Send Avatar ********************/

void upload_avt(int id, int fd, int error, void *data)
{
	struct yahoo_file_info *sf = (struct yahoo_file_info*) data;
	unsigned long size = 0;
	char buf[1024];
	int rw;			/* */
	DWORD dw;		/* needed for ReadFile */
	HANDLE myhFile;

	if (fd < 1 || error) {
		LOG(("[get_fd] Connect Failed!"));
		return;
	}

	myhFile  = CreateFileA(sf->filename,
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

	LOG(("Sending file: %s size: %ld", sf->filename, sf->filesize));

	do {
		rw = ReadFile(myhFile, buf, sizeof(buf), &dw, NULL);

		if (rw != 0) {
			rw = Netlib_Send((HANDLE)fd, buf, dw, MSG_NODUMP);

			if (rw < 1) {
				LOG(("Upload Failed. Send error?"));
				//ShowError(Translate("Yahoo Error"), Translate("Avatar upload failed!?!"));
				break;
			}

			size += rw;
		}
	} while (rw >= 0 && size < sf->filesize);

	CloseHandle(myhFile);

	do {
		rw = Netlib_Recv((HANDLE)fd, buf, sizeof(buf), 0);
		LOG(("Got: %d bytes", rw));
	} while (rw > 0);

	LOG(("File send complete!"));
}

void __cdecl CYahooProto::send_avt_thread(void *psf) 
{
	struct yahoo_file_info *sf = ( yahoo_file_info* )psf;
	if (sf == NULL) {
		DebugLog("[yahoo_send_avt_thread] SF IS NULL!!!");
		return;
	}

	SetByte("AvatarUL", 1);
	yahoo_send_avatar(m_id, sf->filename, sf->filesize, &upload_avt, sf);

	free(sf->filename);
	free(sf);

	if (GetByte("AvatarUL", 1) == 1)
		SetByte("AvatarUL", 0);
}

void CYahooProto::SendAvatar(const char *szFile)
{
	struct _stat statbuf;
	if ( _stat( szFile, &statbuf ) != 0 ) {
		LOG(("[YAHOO_SendAvatar] Error reading File information?!"));
		return;
	}

	yahoo_file_info *sf = y_new(struct yahoo_file_info, 1);
	sf->filename = strdup(szFile);
	sf->filesize = statbuf.st_size;

	DebugLog("[Uploading avatar] filename: %s size: %ld", sf->filename, sf->filesize);

	YForkThread(&CYahooProto::send_avt_thread, sf);
}

struct avatar_info{
	char *who;
	char *pic_url;
	int cksum;
};

void __cdecl CYahooProto::recv_avatarthread(void *pavt) 
{
	PROTO_AVATAR_INFORMATION AI;
	struct avatar_info *avt = ( avatar_info* )pavt;
	int 	error = 0;
	HANDLE hContact = 0;
	TCHAR  buf[4096];

	if (avt == NULL) {
		DebugLog("AVT IS NULL!!!");
		return;
	}

	if (!m_bLoggedIn) {
		DebugLog("We are not logged in!!!");
		return;
	}

	//    ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_GETINFO, ACKRESULT_SUCCESS, (HANDLE) 1, 0);

	LOG(("yahoo_recv_avatarthread who:%s url:%s checksum: %d", avt->who, avt->pic_url, avt->cksum));

	hContact = getbuddyH(avt->who);

	if (!hContact){
		LOG(("ERROR: Can't find buddy: %s", avt->who));
		error = 1;
	} else if (!error) {
		SetDword(hContact, "PictCK", avt->cksum);
		SetDword(hContact, "PictLoading", 1);
	}

	if(!error) {

		NETLIBHTTPREQUEST nlhr={0},*nlhrReply;

		nlhr.cbSize		= sizeof(nlhr);
		nlhr.requestType= REQUEST_GET;
		nlhr.flags		= NLHRF_NODUMP|NLHRF_GENERATEHOST|NLHRF_SMARTAUTHHEADER;
		nlhr.szUrl		= avt->pic_url;

		nlhrReply=(NETLIBHTTPREQUEST*)CallService(MS_NETLIB_HTTPTRANSACTION,(WPARAM)m_hNetlibUser,(LPARAM)&nlhr);

		if(nlhrReply) {

			if (nlhrReply->resultCode != 200) {
				LOG(("Update server returned '%d' instead of 200. It also sent the following: %s", nlhrReply->resultCode, nlhrReply->szResultDescr));
				// make sure it's a real problem and not a problem w/ our connection
				yahoo_send_picture_info(m_id, avt->who, 3, avt->pic_url, avt->cksum);
				error = 1;
			} else if (nlhrReply->dataLength < 1 || nlhrReply->pData == NULL) {
				LOG(("No data??? Got %d bytes.", nlhrReply->dataLength));
				error = 1;
			} else {
				HANDLE myhFile;

				GetAvatarFileName(hContact, buf, 1024, DBGetContactSettingByte(hContact, m_szModuleName,"AvatarType", 0));
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

					SetDword(hContact, "PictLastCheck", 0);
				} else {
					LOG(("Can not open file for writing: %s", buf));
					error = 1;
				}
			}
			CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT,0,(LPARAM)nlhrReply);
		}
	}

	if (GetDword(hContact, "PictCK", 0) != avt->cksum) {
		LOG(("WARNING: Checksum updated during download?!"));
		error = 1; /* don't use this one? */
	} 

	SetDword(hContact, "PictLoading", 0);
	LOG(("File download complete!?"));

	if (error) 
		buf[0]='\0';

	free(avt->who);
	free(avt->pic_url);
	free(avt);

	AI.cbSize = sizeof AI;
	AI.format = PA_FORMAT_PNG;
	AI.hContact = hContact;
	#if defined( _UNICODE )
		WideCharToMultiByte( CP_ACP, 0, buf, -1, AI.filename, sizeof AI.filename, 0, 0 );
	#else
		lstrcpyA( AI.filename, buf );
	#endif

	if (error) 
		SetDword(hContact, "PictCK", 0);

	ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_AVATAR, !error ? ACKRESULT_SUCCESS:ACKRESULT_FAILED,(HANDLE) &AI, 0);
}

void CYahooProto::ext_got_picture(const char *me, const char *who, const char *pic_url, int cksum, int type)
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
			if (!GetByte( "ShowAvatars", 1 )) {
				LOG(("[ext_yahoo_got_picture] We are not using/showing avatars!"));
				yahoo_send_picture_update(m_id, who, 0); // no avatar (disabled)
				return;
			}
		
			LOG(("[ext_yahoo_got_picture] Getting ready to send info!"));
			/* need to read CheckSum */
			cksum = GetDword("AvatarHash", 0);
			if (cksum) {
				if (!DBGetContactSettingString(NULL, m_szModuleName, "AvatarURL", &dbv)) {
					LOG(("[ext_yahoo_got_picture] Sending url: %s checksum: %d to '%s'!", dbv.pszVal, cksum, who));
					//void yahoo_send_picture_info(int id, const char *me, const char *who, const char *pic_url, int cksum)
					yahoo_send_picture_info(m_id, who, 2, dbv.pszVal, cksum);
					DBFreeVariant(&dbv);
					break;
				} else
					LOG(("No AvatarURL???"));
				
				/*
				 * Try to re-upload the avatar
				 */
				if (GetByte("AvatarUL", 0) != 1){
					// NO avatar URL??
					if (!DBGetContactSettingString(NULL, m_szModuleName, "AvatarFile", &dbv)) {
						struct _stat statbuf;
						
						if (_stat( dbv.pszVal, &statbuf ) != 0 ) {
							LOG(("[ext_yahoo_got_picture] Avatar File Missing? Can't find file: %s", dbv.pszVal));
						} else {
							DBWriteContactSettingString(NULL, m_szModuleName, "AvatarInv", who);
							SendAvatar(dbv.pszVal);
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
			if (!GetByte( "ShowAvatars", 1 )) {
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
				DBWriteContactSettingDword(hContact, m_szModuleName, "PictCK", 0);
				
				reset_avatar(hContact);
			} else {
				if (pic_url == NULL) {
					LOG(("[ext_yahoo_got_picture] WARNING: Empty URL for avatar?"));
					return;
				}
				
				TCHAR z[1024];
				GetAvatarFileName(hContact, z, 1024, DBGetContactSettingByte(hContact, m_szModuleName,"AvatarType", 0));
				
				if (DBGetContactSettingDword(hContact, m_szModuleName,"PictCK", 0) != cksum || _taccess( z, 0 ) != 0 ) {
					
					DebugLog("[ext_yahoo_got_picture] Checksums don't match or avatar file is missing. Current: %d, New: %d",(int)DBGetContactSettingDword(hContact, m_szModuleName,"PictCK", 0), cksum);

					struct avatar_info *avt = ( avatar_info* )malloc(sizeof(struct avatar_info));
					avt->who = strdup(who);
					avt->pic_url = strdup(pic_url);
					avt->cksum = cksum;

					YForkThread(&CYahooProto::recv_avatarthread, avt);
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
			if (!GetByte( "ShowAvatars", 1 )) {
				LOG(("[ext_yahoo_got_picture] We are not using/showing avatars!"));
				yahoo_send_picture_update(m_id, who, 0); // no avatar (disabled)
				return;
			}
		
			LOG(("[ext_yahoo_got_picture] Getting ready to send info!"));
			/* need to read CheckSum */
			mcksum = GetDword("AvatarHash", 0);
			if (mcksum == 0) {
				/* this should NEVER Happen??? */
				LOG(("[ext_yahoo_got_picture] No personal checksum? and Invalidate?!"));
				yahoo_send_picture_update(m_id, who, 0); // no avatar (disabled)
				return;
			}
			
			LOG(("[ext_yahoo_got_picture] My Checksum: %d", mcksum));
			
			if (!DBGetContactSettingString(NULL, m_szModuleName, "AvatarURL", &dbv)){
					if (lstrcmpiA(pic_url, dbv.pszVal) == 0) {
						DBVARIANT dbv2;
						/*time_t  ts;
						DWORD	ae;*/
						
						if (mcksum != cksum)
							LOG(("[ext_yahoo_got_picture] WARNING: Checksums don't match!"));	
						
						/*time(&ts);
						ae = GetDword("AvatarExpires", 0);
						
						if (ae != 0 && ae > (ts - 300)) {
							LOG(("[ext_yahoo_got_picture] Current Time: %lu Expires: %lu ", ts, ae));
							LOG(("[ext_yahoo_got_picture] We just reuploaded! Stop screwing with Yahoo FT. "));
							
							// don't leak stuff
							DBFreeVariant(&dbv);

							break;
						}*/
						
						LOG(("[ext_yahoo_got_picture] Buddy: %s told us this is bad??Expired??. Re-uploading", who));
						DBDeleteContactSetting(NULL, m_szModuleName, "AvatarURL");
						
						if (!DBGetContactSettingString(NULL, m_szModuleName, "AvatarFile", &dbv2)) {
							DBWriteContactSettingString(NULL, m_szModuleName, "AvatarInv", who);
							SendAvatar(dbv2.pszVal);
							DBFreeVariant(&dbv2);
						} else {
							LOG(("[ext_yahoo_got_picture] No Local Avatar File??? "));
						}
					} else {
						LOG(("[ext_yahoo_got_picture] URL doesn't match? Tell them the right thing!!!"));
						yahoo_send_picture_info(m_id, who, 2, dbv.pszVal, mcksum);
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

void CYahooProto::ext_got_picture_checksum(const char *me, const char *who, int cksum)
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
		DBWriteContactSettingDword(hContact, m_szModuleName, "PictCK", 0);

		reset_avatar(hContact);
	}
	else {
		if (DBGetContactSettingDword(hContact, m_szModuleName,"PictCK", 0) != cksum) {
			// Now save the new checksum. No rush requesting new avatar yet.
			DBWriteContactSettingDword(hContact, m_szModuleName, "PictCK", cksum);

			// Need to delete the Avatar File!!
			TCHAR szFile[MAX_PATH];
			GetAvatarFileName(hContact, szFile, sizeof szFile, 0);
			DeleteFile(szFile);

			// Reset the avatar and cleanup.
			reset_avatar(hContact);

			// Request new avatar here... (might also want to check the sharing status?)

			if (GetByte( "ShareAvatar", 0 ) == 2)
				request_avatar(who);
		}
	}
}

void CYahooProto::ext_got_picture_update(const char *me, const char *who, int buddy_icon)
{
	HANDLE 	hContact = 0;

	LOG(("ext_got_picture_update for %s buddy_icon: %d", who, buddy_icon));

	hContact = getbuddyH(who);
	if (hContact == NULL) {
		LOG(("Buddy Not Found. Skipping avatar update"));
		return;
	}

	DBWriteContactSettingByte(hContact, m_szModuleName, "AvatarType", buddy_icon);

	/* Last thing check the checksum and request new one if we need to */
	reset_avatar(hContact);
}

void CYahooProto::ext_got_picture_status(const char *me, const char *who, int buddy_icon)
{
	HANDLE 	hContact = 0;

	LOG(("ext_yahoo_got_picture_status for %s buddy_icon: %d", who, buddy_icon));

	hContact = getbuddyH(who);
	if (hContact == NULL) {
		LOG(("Buddy Not Found. Skipping avatar update"));
		return;
	}

	DBWriteContactSettingByte(hContact, m_szModuleName, "AvatarType", buddy_icon);

	/* Last thing check the checksum and request new one if we need to */
	reset_avatar(hContact);
}

void CYahooProto::ext_got_picture_upload(const char *me, const char *url,unsigned int ts)
{
	int cksum = 0;
	DBVARIANT dbv;	

	LOG(("[ext_yahoo_got_picture_upload] url: %s timestamp: %d", url, ts));

	if (!url) {
		LOG(("[ext_yahoo_got_picture_upload] Problem with upload?"));
		return;
	}


	cksum = GetDword("TMPAvatarHash", 0);
	if (cksum != 0) {
		LOG(("[ext_yahoo_got_picture_upload] Updating Checksum to: %d", cksum));
		SetDword("AvatarHash", cksum);
		DBDeleteContactSetting(NULL, m_szModuleName, "TMPAvatarHash");

		// This is only meant for message sessions, but we don't got those in miranda yet
		//YAHOO_bcast_picture_checksum(cksum);
		yahoo_send_picture_checksum(m_id, NULL, cksum);

		// need to tell the stupid Yahoo that our icon updated
		//YAHOO_bcast_picture_update(2);
	}else
		cksum = GetDword("AvatarHash", 0);

	SetString(NULL, "AvatarURL", url);
	//YAHOO_SetDword("AvatarExpires", ts);

	if  (!DBGetContactSettingString(NULL, m_szModuleName, "AvatarInv", &dbv) ){
		LOG(("[ext_yahoo_got_picture_upload] Buddy: %s told us this is bad??", dbv.pszVal));

		LOG(("[ext_yahoo_got_picture] Sending url: %s checksum: %d to '%s'!", url, cksum, dbv.pszVal));
		//void yahoo_send_picture_info(int id, const char *me, const char *who, const char *pic_url, int cksum)
		yahoo_send_picture_info(m_id, dbv.pszVal, 2, url, cksum);

		DBDeleteContactSetting(NULL, m_szModuleName, "AvatarInv");
		DBFreeVariant(&dbv);
	}
}

void CYahooProto::ext_got_avatar_share(int buddy_icon)
{
	LOG(("[ext_yahoo_got_avatar_share] buddy icon: %d", buddy_icon));

	SetByte( "ShareAvatar", buddy_icon );
}

void CYahooProto::reset_avatar(HANDLE hContact)
{
	LOG(("[YAHOO_RESET_AVATAR]"));

	ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS, NULL, 0);
}

void CYahooProto::request_avatar(const char* who)
{
	time_t  last_chk, cur_time;
	HANDLE 	hContact = 0;
	//char    szFile[MAX_PATH];

	if (!GetByte( "ShowAvatars", 1 )) {
		LOG(("Avatars disabled, but available for: %s", who));
		return;
	}

	hContact = getbuddyH(who);

	if (!hContact)
		return;

	/*GetAvatarFileName(hContact, szFile, sizeof szFile, DBGetContactSettingByte(hContact, m_szModuleName,"AvatarType", 0));
	DeleteFile(szFile);*/

	time(&cur_time);
	last_chk = DBGetContactSettingDword(hContact, m_szModuleName, "PictLastCheck", 0);

	/*
	* time() - in seconds ( 60*60 = 1 hour)
	*/
	if (DBGetContactSettingDword(hContact, m_szModuleName,"PictCK", 0) == 0 || 
		last_chk == 0 || (cur_time - last_chk) > 60) {

			DBWriteContactSettingDword(hContact, m_szModuleName, "PictLastCheck", (DWORD)cur_time);

			LOG(("Requesting Avatar for: %s", who));
			yahoo_request_buddy_avatar(m_id, who);
	} else {
		LOG(("Avatar Not Available for: %s Last Check: %ld Current: %ld (Flood Check in Effect)", who, last_chk, cur_time));
	}
}

void CYahooProto::InitCustomFolders(void)
{
	if ( InitCstFldRan ) return; 

	InitCstFldRan = true;
	if ( ServiceExists( MS_FOLDERS_REGISTER_PATH )) 
	{
		TCHAR AvatarsFolder[MAX_PATH];
		TCHAR *tmpPath = Utils_ReplaceVarsT( _T("%miranda_avatarcache%"));
		mir_sntprintf(AvatarsFolder, MAX_PATH, _T("%s\\") _T(TCHAR_STR_PARAM), tmpPath, m_szModuleName);
		hYahooAvatarsFolder = FoldersRegisterCustomPathT(m_szModuleName, "Avatars", AvatarsFolder);
		mir_free(tmpPath);
	}
}

void CYahooProto::GetAvatarFileName(HANDLE hContact, TCHAR* pszDest, int cbLen, int type)
{
	size_t tPathLen;

	InitCustomFolders();

	TCHAR* path = ( TCHAR* )alloca( sizeof(TCHAR)*( cbLen+1 ));
	if ( hYahooAvatarsFolder != NULL && !FoldersGetCustomPathT( hYahooAvatarsFolder, path, (int)cbLen, _T("")))
	{
		_tcscpy( pszDest, path );
		tPathLen = _tcslen( pszDest );
	} else {
		TCHAR *tmpPath = Utils_ReplaceVarsT( _T("%miranda_avatarcache%"));
		tPathLen = mir_sntprintf(pszDest, cbLen, _T("%s\\") _T(TCHAR_STR_PARAM), tmpPath, m_szModuleName);
		mir_free(tmpPath);
	}

	if ( _taccess(pszDest, 0))
		YAHOO_CallService(MS_UTILS_CREATEDIRTREE, 0, (LPARAM)pszDest);

	if (hContact != NULL) {
		int ck_sum = DBGetContactSettingDword(hContact, m_szModuleName,"PictCK", 0);
		tPathLen += mir_sntprintf(pszDest + tPathLen, cbLen - tPathLen, _T("\\%lX"), ck_sum);
	} else {
		tPathLen += mir_sntprintf(pszDest + tPathLen, cbLen - tPathLen, _T("\\") _T(TCHAR_STR_PARAM) _T(" avatar"), m_szModuleName);
	}
	
	mir_sntprintf(pszDest + tPathLen, cbLen - tPathLen, _T("%s"), type == 1 ? _T(".swf") :  _T(".png"));
}

INT_PTR __cdecl CYahooProto::GetAvatarInfo(WPARAM wParam,LPARAM lParam)
{
	PROTO_AVATAR_INFORMATION* AI = ( PROTO_AVATAR_INFORMATION* )lParam;
	DBVARIANT dbv;
	int avtType;

	if (!DBGetContactSettingString(AI->hContact, m_szModuleName, YAHOO_LOGINID, &dbv)) {
		DebugLog("[YAHOO_GETAVATARINFO] For: %s", dbv.pszVal);
		DBFreeVariant(&dbv);
	}else {
		DebugLog("[YAHOO_GETAVATARINFO]");
	}

	if (!GetByte( "ShowAvatars", 1 ) || !m_bLoggedIn) {
		DebugLog("[YAHOO_GETAVATARINFO] %s", m_bLoggedIn ? "We are not using/showing avatars!" : "We are not logged in. Can't load avatars now!");

		return GAIR_NOAVATAR;
	}

	avtType  = DBGetContactSettingByte(AI->hContact, m_szModuleName,"AvatarType", 0);
	DebugLog("[YAHOO_GETAVATARINFO] Avatar Type: %d", avtType);

	if ( avtType != 2) {
		if (avtType != 0)
			DebugLog("[YAHOO_GETAVATARINFO] Not handling this type yet!");

		return GAIR_NOAVATAR;
	}

	if (DBGetContactSettingDword(AI->hContact, m_szModuleName,"PictCK", 0) == 0) 
		return GAIR_NOAVATAR;

	TCHAR tszFileName[ MAX_PATH ];
	GetAvatarFileName(AI->hContact, tszFileName, SIZEOF(tszFileName), DBGetContactSettingByte(AI->hContact, m_szModuleName,"AvatarType", 0));
	#if defined( _UNICODE )
		WideCharToMultiByte( CP_ACP, 0, tszFileName, -1, AI->filename, sizeof AI->filename, 0, 0 );
	#else
		strcpy( AI->filename, tszFileName );
	#endif
	AI->format = PA_FORMAT_PNG;
	DebugLog("[YAHOO_GETAVATARINFO] filename: %s", AI->filename);

	if (_taccess( tszFileName, 0 ) == 0 ) 
		return GAIR_SUCCESS;

	if (( wParam & GAIF_FORCE ) != 0 && AI->hContact != NULL ) {		
		/* need to request it again? */
		if (GetDword(AI->hContact, "PictLoading", 0) != 0 &&
			(time(NULL) - GetDword(AI->hContact, "PictLastCheck", 0) < 500)) {
				DebugLog("[YAHOO_GETAVATARINFO] Waiting for avatar to load!");
				return GAIR_WAITFOR;
		} else if ( m_bLoggedIn ) {
			DBVARIANT dbv;

			if (!DBGetContactSettingString(AI->hContact, m_szModuleName, YAHOO_LOGINID, &dbv)) {
				DebugLog("[YAHOO_GETAVATARINFO] Requesting avatar!");

				request_avatar(dbv.pszVal);
				DBFreeVariant(&dbv);

				return GAIR_WAITFOR;
			} else {
				DebugLog("[YAHOO_GETAVATARINFO] Can't retrieve user id?!");
			}
		}
	}

	DebugLog("[YAHOO_GETAVATARINFO] NO AVATAR???");
	return GAIR_NOAVATAR;
}

/*
 * --=[ AVS / LoadAvatars API/Services ]=--
 */
INT_PTR __cdecl CYahooProto::GetAvatarCaps(WPARAM wParam, LPARAM lParam)
{
	int res = 0;

	switch (wParam) {
	case AF_MAXSIZE: 
		LOG(("[YahooGetAvatarCaps] AF_MAXSIZE"));

		((POINT*)lParam)->x = 96;
		((POINT*)lParam)->y = 96;

		break;

	case AF_PROPORTION: 
		LOG(("[YahooGetAvatarCaps] AF_PROPORTION"));

		res = PIP_NONE;
		break;

	case AF_FORMATSUPPORTED:
		LOG(("[YahooGetAvatarCaps] AF_FORMATSUPPORTED"));
		res = lParam == PA_FORMAT_PNG;
		break;

	case AF_ENABLED:
		LOG(("[YahooGetAvatarCaps] AF_ENABLED"));

		res = (GetByte( "ShowAvatars", 1 )) ? 1 : 0;
		break;

	case AF_DONTNEEDDELAYS:
		res = 1; /* don't need to delay avatar loading */
		break;

	case AF_MAXFILESIZE:
		res = 0; /* no max filesize for now */
		break;

	case AF_DELAYAFTERFAIL:
		res = 15 * 60 * 1000; /* 15 mins */
		break;

	default:
		LOG(("[YahooGetAvatarCaps] Unknown: %d", wParam));
	}

	return res;
}

/*
Service: /GetMyAvatar
wParam=(char *)Buffer to file name
lParam=(int)Buffer size
return=0 on success, else on error
*/
INT_PTR __cdecl CYahooProto::GetMyAvatar(WPARAM wParam, LPARAM lParam)
{
	char *buffer = (char *)wParam;
	int size = (int)lParam;

	DebugLog("[YahooGetMyAvatar]");

	if (buffer == NULL || size <= 0)
		return -1;

	if (!GetByte( "ShowAvatars", 1 ))
		return -2;

	DBVARIANT dbv;
	int ret = -3;

	if (GetDword("AvatarHash", 0)){
		if (!DBGetContactSettingString(NULL, m_szModuleName, "AvatarFile", &dbv)){
			if (_access(dbv.pszVal, 0) == 0){
				strncpy(buffer, dbv.pszVal, size-1);
				buffer[size-1] = '\0';

				ret = 0;
			}
			DBFreeVariant(&dbv);
		}
	}

	return ret;
}

/*
#define PS_SETMYAVATAR "/SetMyAvatar"
wParam=0
lParam=(const char *)Avatar file name
return=0 for sucess
*/

INT_PTR __cdecl CYahooProto::SetMyAvatar(WPARAM wParam, LPARAM lParam)
{
	char* szFile = ( char* )lParam;
	TCHAR szMyFile[MAX_PATH+1];

	GetAvatarFileName(NULL, szMyFile, MAX_PATH, 2);

	if (szFile == NULL) {
		DebugLog("[Deleting Avatar Info]");	

		/* remove ALL our Avatar Info Keys */
		DBDeleteContactSetting(NULL, m_szModuleName, "AvatarFile");	
		DBDeleteContactSetting(NULL, m_szModuleName, "AvatarHash");
		DBDeleteContactSetting(NULL, m_szModuleName, "AvatarURL");	
		DBDeleteContactSetting(NULL, m_szModuleName, "AvatarTS");	

		/* Send a Yahoo packet saying we don't got an avatar anymore */
		yahoo_send_picture_status(m_id, 0);

		SetByte("ShareAvatar",0);

		DeleteFile(szMyFile);
	} else {
		DWORD  dwPngSize, dw;
		BYTE* pResult;
		unsigned int hash;
		HANDLE  hFile;

		hFile = CreateFileA(szFile, 
			GENERIC_READ, 
			FILE_SHARE_READ|FILE_SHARE_WRITE, 
			NULL, 
			OPEN_EXISTING, 
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 
			0 );

		if ( hFile ==  INVALID_HANDLE_VALUE )
			return 1;

		dwPngSize = GetFileSize( hFile, NULL );
		if (( pResult = ( BYTE* )malloc( dwPngSize )) == NULL )
			return 2;

		ReadFile( hFile, pResult, dwPngSize, &dw, NULL );
		CloseHandle( hFile );

		hFile = CreateFile(szMyFile, 
			GENERIC_WRITE, 
			FILE_SHARE_WRITE, 
			NULL, 
			OPEN_ALWAYS, 
			FILE_ATTRIBUTE_NORMAL, 0);
		if ( hFile ==  INVALID_HANDLE_VALUE ) 
			return 1;

		WriteFile( hFile, pResult, dwPngSize, &dw, NULL );
		SetEndOfFile( hFile);
		CloseHandle( hFile );

		hash = YAHOO_avt_hash(( const char* )pResult, dwPngSize);
		free( pResult );

		if ( hash ) {
			LOG(("[YAHOO_SetAvatar] File: '%s' CK: %d", szMyFile, hash));	

			/* now check and make sure we don't reupload same thing over again */
			if (hash != GetDword("AvatarHash", 0)) {
				SetStringT(NULL, "AvatarFile", szMyFile);
				DBWriteContactSettingDword(NULL, m_szModuleName, "TMPAvatarHash", hash);

				/*	Set Sharing to ON if it's OFF */
				if (GetByte( "ShareAvatar", 0 ) != 2) {
					SetByte( "ShareAvatar", 2 );
					yahoo_send_picture_status(m_id, 2);
				}

				#if defined( _UNICODE )
					char* tmp = mir_t2a(szMyFile);
					SendAvatar(tmp);
					mir_free( tmp );
				#else
					SendAvatar(szMyFile);
				#endif
			} 
			else LOG(("[YAHOO_SetAvatar] Same checksum and avatar on YahooFT. Not Reuploading."));	  
	}	}

	return 0;
}

/*
 * --=[ ]=--
 */
