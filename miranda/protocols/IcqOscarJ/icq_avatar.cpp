// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright � 2000-2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright � 2001-2002 Jon Keating, Richard Hughes
// Copyright � 2002-2004 Martin �berg, Sam Kothari, Robert Rainwater
// Copyright � 2004-2009 Joe Kucera
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
//
// -----------------------------------------------------------------------------
//
// File name      : $URL$
// Revision       : $Revision$
// Last change on : $Date$
// Last change by : $Author$
//
// DESCRIPTION:
//
//  Manages Avatar connection, provides internal service for handling avatars
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"
#include "m_folders.h"

struct avatarthreadstartinfo
{
	CIcqProto* ppro;
	HANDLE hConnection;  // handle to the connection
	char* pCookie;
	WORD wCookieLen;
	HANDLE hAvatarPacketRecver;
	int stopThread; // horrible, but simple - signal for thread to stop
	WORD wLocalSequence;
	CRITICAL_SECTION localSeqMutex;
	int pendingLogin;
	int paused;
	HANDLE runContact[4];
	DWORD runTime[4];
	int runCount;
//
	rates *rates;
};

struct avatarrequest
{
	int type;
	DWORD dwUin;
	char *szUid;
	HANDLE hContact;
	BYTE *hash;
	unsigned int hashlen;
	char *szFile;
	char *pData;
	unsigned int cbData;
	WORD wRef;
	DWORD timeOut;
	avatarrequest* pNext; // invalid, but reused - spare realloc
};

#define ART_GET     1
#define ART_UPLOAD  2
#define ART_BLOCK   4

BYTE hashEmptyAvatar[9] = {0x00, 0x01, 0x00, 0x05, 0x02, 0x01, 0xD2, 0x04, 0x72};

static void FreeAvatarRequest(avatarrequest **request)
{
	avatarrequest *ar = *request;

	if (ar)
	{
		switch (ar->type)
		{
		case ART_UPLOAD:
			SAFE_FREE((void**)&ar->pData);
			break;
		case ART_GET:
			SAFE_FREE((void**)&ar->hash);
			SAFE_FREE(&ar->szUid);
			SAFE_FREE(&ar->szFile);
			break;
		case ART_BLOCK:
			break;
		}
	}
	SAFE_FREE((void**)request);
}

avatarrequest* CIcqProto::ReleaseAvatarRequestInQueue(avatarrequest *request)
{
	avatarrequest *pNext = request->pNext;

	avatarrequest **par = &pendingRequests;
	avatarrequest *ar = pendingRequests;

	while (ar)
	{
		if (ar == request)
		{ // found it, remove
			*par = ar->pNext;
			break;
		}
		par = &ar->pNext;
		ar = ar->pNext;
	}

	FreeAvatarRequest(&request);

	return pNext;
}

void CIcqProto::InitAvatars()
{
	if (!bAvatarsFolderInited)
	{ // do it only once
		bAvatarsFolderInited = TRUE;

		if (ServiceExists(MS_FOLDERS_REGISTER_PATH))
		{ // check if it does make sense
			char szPath[MAX_PATH];

            char *tmpPath = Utils_ReplaceVars("%miranda_avatarcache%");
			null_snprintf(szPath, MAX_PATH, "%s\\%s\\", tmpPath, m_szModuleName);
            mir_free(tmpPath);

			hAvatarsFolder = FoldersRegisterCustomPath(m_szModuleName, "Avatars Cache", szPath);
		}
	}
}

char* CIcqProto::loadMyAvatarFileName()
{
	DBVARIANT dbvFile = {0};

	if (!getSettingString(NULL, "AvatarFile", &dbvFile))
	{
		char tmp[MAX_PATH];;

		CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)dbvFile.pszVal, (LPARAM)tmp);
		ICQFreeVariant(&dbvFile);
		return null_strdup(tmp);
	}
	return NULL;
}

void CIcqProto::GetFullAvatarFileName(int dwUin, char* szUid, int dwFormat, char* pszDest, int cbLen)
{
	GetAvatarFileName(dwUin, szUid, pszDest, cbLen);
	AddAvatarExt(dwFormat, pszDest);
}

void CIcqProto::GetAvatarFileName(int dwUin, char* szUid, char* pszDest, int cbLen)
{
	int tPathLen;
	FOLDERSGETDATA fgd = {0};

	InitAvatars();

	fgd.cbSize = sizeof(FOLDERSGETDATA);
	fgd.nMaxPathSize = cbLen;
	fgd.szPath = pszDest;
	if (CallService(MS_FOLDERS_GET_PATH, (WPARAM)hAvatarsFolder, (LPARAM)&fgd))
	{
        char *tmpPath = Utils_ReplaceVars("%miranda_avatarcache%");
        
        lstrcpynA(pszDest, tmpPath, cbLen-1);
        mir_free(tmpPath);
		tPathLen = strlennull(pszDest);
		tPathLen += null_snprintf(pszDest + tPathLen, cbLen-tPathLen, "\\%s\\", m_szModuleName);
	}
	else
	{
		strcat(pszDest, "\\");
		tPathLen = strlennull(pszDest);
	}
	
	// make sure the avatar cache directory exists
	char *szDir = ansi_to_utf8(pszDest);
	MakeDirUtf(szDir);
	SAFE_FREE(&szDir);

	if (dwUin != 0) 
	{
		_ltoa(dwUin, pszDest + tPathLen, 10);
	}
	else if (szUid)
	{
		strcpy(pszDest + tPathLen, szUid);
	}
	else
	{
		char szBuf[MAX_PATH];

		if (CallService(MS_DB_GETPROFILENAME, 250 - tPathLen, (LPARAM)szBuf))
			strcpy(pszDest + tPathLen, "avatar" );
		else 
		{
			char* szLastDot = strrchr(szBuf, '.');
			if (szLastDot) szLastDot[0] = '\0';
			strcpy(pszDest + tPathLen, szBuf);
			strcat(pszDest + tPathLen, "_avt");
		}
	}
}

void AddAvatarExt(int dwFormat, char* pszDest)
{
	if (dwFormat == PA_FORMAT_JPEG)
		strcat(pszDest, ".jpg");
	else if (dwFormat == PA_FORMAT_GIF)
		strcat(pszDest, ".gif");
	else if (dwFormat == PA_FORMAT_PNG)
		strcat(pszDest, ".png");
	else if (dwFormat == PA_FORMAT_BMP)
		strcat(pszDest, ".bmp");
	else if (dwFormat == PA_FORMAT_XML)
		strcat(pszDest, ".xml");
	else if (dwFormat == PA_FORMAT_SWF)
		strcat(pszDest, ".swf");
	else
		strcat(pszDest, ".dat");
}

int DetectAvatarFormatBuffer(char* pBuffer)
{
	if (!strncmp(pBuffer, "%PNG", 4))
		return PA_FORMAT_PNG;

	if (!strncmp(pBuffer, "GIF8", 4))
		return PA_FORMAT_GIF;

	if (!_strnicmp(pBuffer, "<?xml", 5))
		return PA_FORMAT_XML;

	if ((((DWORD*)pBuffer)[0] == 0xE0FFD8FFul) || (((DWORD*)pBuffer)[0] == 0xE1FFD8FFul))
		return PA_FORMAT_JPEG;

	if (!strncmp(pBuffer, "BM", 2))
		return PA_FORMAT_BMP;

	return PA_FORMAT_UNKNOWN;
}

int DetectAvatarFormat(const char *szFile)
{
	char pBuf[32];

	int src = _open(szFile, _O_BINARY | _O_RDONLY);

	if (src != -1)
	{
		_read(src, pBuf, 32);
		_close(src);

		return DetectAvatarFormatBuffer(pBuf);
	}
	else
		return PA_FORMAT_UNKNOWN;
}


#define MD5_BLOCK_SIZE 1024*1024 /* use 1MB blocks */

BYTE* calcMD5HashOfFile(const char *szFile)
{
	BYTE *res = NULL;

	if (szFile)
	{
		HANDLE hFile = NULL, hMap = NULL;
		BYTE *ppMap = NULL;

		if ((hFile = CreateFileA(szFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL )) != INVALID_HANDLE_VALUE)
			if ((hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL)) != NULL)
			{
				long cbFileSize = GetFileSize( hFile, NULL );

				res = (BYTE*)SAFE_MALLOC(16*sizeof(char));
				if (cbFileSize != 0 && res)
				{
					mir_md5_state_t state;
					mir_md5_byte_t digest[16];
					int dwOffset = 0;

					mir_md5_init(&state);
					while (dwOffset < cbFileSize)
					{
						int dwBlockSize = min(MD5_BLOCK_SIZE, cbFileSize-dwOffset);

						if (!(ppMap = (BYTE*)MapViewOfFile(hMap, FILE_MAP_READ, 0, dwOffset, dwBlockSize)))
							break;
						mir_md5_append(&state, (const mir_md5_byte_t *)ppMap, dwBlockSize);
						UnmapViewOfFile(ppMap);
						dwOffset += dwBlockSize;
					}
					mir_md5_finish(&state, digest);
					memcpy(res, digest, 16);
				}
			}

			if (hMap  != NULL) CloseHandle(hMap);
			if (hFile != NULL) CloseHandle(hFile);
	}

	return res;
}


int CIcqProto::IsAvatarChanged(HANDLE hContact, BYTE* pHash, int nHashLen)
{
	DBVARIANT dbvSaved = {0};

	if (!getSetting(hContact, "AvatarSaved", &dbvSaved))
	{
		if ((dbvSaved.cpbVal != nHashLen) || memcmp(dbvSaved.pbVal, pHash, nHashLen))
		{ // the hashes is different
			ICQFreeVariant(&dbvSaved);

			return 2;
		}
		ICQFreeVariant(&dbvSaved);

		return 0; // hash is there and is the same - Success
	}
	return 1; // saved Avatar hash is missing
}


void CIcqProto::StartAvatarThread(HANDLE hConn, char* cookie, WORD cookieLen) // called from event
{
	avatarthreadstartinfo* atsi = NULL;

	if (!hConn)
	{
		atsi = currentAvatarThread;
		if (atsi && atsi->pendingLogin) // this is not safe...
		{
			NetLog_Server("Avatar, Multiple start thread attempt, ignored.");
			SAFE_FREE((void**)&cookie);
			return;
		}
		m_pendingAvatarsStart = 0;
		NetLog_Server("Avatar: Connect failed");

		EnterCriticalSection(&cookieMutex); // wait for ready queue, reused cs
		{ // check if any upload request is not in the queue
			avatarrequest* ar = pendingRequests;
			int bYet = 0;

			while (ar)
			{
				if (ar->type == ART_UPLOAD)
				{ // we found it, return error
					if (!bYet)
					{
						icq_LogMessage(LOG_WARNING, LPGEN("Error uploading avatar to server, server temporarily unavailable."));
					}
					bYet = 1;
					// remove upload request from queue
					ar = ReleaseAvatarRequestInQueue(ar);
					continue;
				}
				ar = ar->pNext;
			}
		}
		LeaveCriticalSection(&cookieMutex);

		SAFE_FREE((void**)&cookie);

		return;
	}
	atsi = currentAvatarThread;
	if (atsi && atsi->pendingLogin) // this is not safe...
	{
		NetLog_Server("Avatar, Multiple start thread attempt, ignored.");
		NetLib_CloseConnection(&hConn, FALSE);
		SAFE_FREE((void**)&cookie);
		return;
	}

	AvatarsReady = FALSE; // the old connection should not be used anymore

	atsi = (avatarthreadstartinfo*)SAFE_MALLOC(sizeof(avatarthreadstartinfo));
	atsi->ppro = this;
	atsi->pendingLogin = 1;
	// Randomize sequence
	atsi->wLocalSequence = generate_flap_sequence();
	atsi->hConnection = hConn;
	atsi->pCookie = cookie;
	atsi->wCookieLen = cookieLen;
	currentAvatarThread = atsi; // we store only current thread
	ForkThread(( IcqThreadFunc )&CIcqProto::AvatarThread, atsi);
}

void CIcqProto::StopAvatarThread()
{
	AvatarsReady = FALSE; // the connection are about to close

	if (currentAvatarThread)
	{
		currentAvatarThread->stopThread = 1; // make the thread stop
		currentAvatarThread = NULL; // the thread will finish in background
	}
	return;
}

static void NetLog_Hash(CIcqProto* ppro, const char* pszIdent, BYTE *pHash, int nHashLen)
{
	if (nHashLen == 0x14)
		ppro->NetLog_Server("%s Hash: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", pszIdent, pHash[0], pHash[1], pHash[2], pHash[3], pHash[4], pHash[5], pHash[6], pHash[7], pHash[8], pHash[9], pHash[10], pHash[11], pHash[12], pHash[13], pHash[14], pHash[15], pHash[16], pHash[17], pHash[18], pHash[19]);
	else if (nHashLen == 0x09)
		ppro->NetLog_Server("%s Hash: %02X%02X%02X%02X%02X%02X%02X%02X%02X", pszIdent, pHash[0], pHash[1], pHash[2], pHash[3], pHash[4], pHash[5], pHash[6], pHash[7], pHash[8]);
	else
		ppro->NetLog_Server("%s Hash: Unknown hash format.", pszIdent);
}


// handle Owner's avatar hash changes
void CIcqProto::handleAvatarOwnerHash(WORD wItemID, BYTE bFlags, BYTE *pData, BYTE nDataLen)
{
  if ((nDataLen >= 0x14) && m_bAvatarsEnabled)
  {
		switch (bFlags)
		{
			case 1: // our avatar is on the server
			{
				char *file;
				BYTE *hash;

				setSettingBlob(NULL, "AvatarHash", pData, 0x14);

				setUserInfo();
				// here we need to find a file, check its hash, if invalid get avatar from server
				file = loadMyAvatarFileName();
				if (!file)
				{ // we have no avatar file, download from server
					char szFile[MAX_PATH + 4];
#ifdef _DEBUG
					NetLog_Server("We have no avatar, requesting from server.");
#endif
					GetAvatarFileName(0, NULL, szFile, MAX_PATH);
					GetAvatarData(NULL, m_dwLocalUIN, NULL, pData, 0x14, szFile);
				}
				else
				{ // we know avatar filename
					hash = calcMD5HashOfFile(file);
					if (!hash)
					{ // hash could not be calculated - probably missing file, get avatar from server
						char szFile[MAX_PATH + 4];
#ifdef _DEBUG
						NetLog_Server("We have no avatar, requesting from server.");
#endif
						GetAvatarFileName(0, NULL, szFile, MAX_PATH);
						GetAvatarData(NULL, m_dwLocalUIN, NULL, pData, 0x14, szFile);
					} // check if we had set any avatar if yes set our, if not download from server
					else if (memcmp(hash, pData + 4, 0x10))
					{ // we have different avatar, sync that
						if (getSettingByte(NULL, "ForceOurAvatar", 1))
						{ // we want our avatar, update hash
							DWORD dwPaFormat = DetectAvatarFormat(file);
							BYTE* pHash = (BYTE*)_alloca(0x14);

							NetLog_Server("Our avatar is different, set our new hash.");

							pHash[0] = 0;
							pHash[1] = dwPaFormat == PA_FORMAT_XML ? AVATAR_HASH_FLASH : AVATAR_HASH_STATIC;
							pHash[2] = 1; // state of the hash
							pHash[3] = 0x10; // len of the hash
							memcpy(pHash + 4, hash, 0x10);
							updateServAvatarHash(pHash, 0x14);
						}
						else
						{ // get avatar from server
							char szFile[MAX_PATH + 4];
#ifdef _DEBUG
							NetLog_Server("We have different avatar, requesting new from server.");
#endif
							GetAvatarFileName(0, NULL, szFile, MAX_PATH);
							GetAvatarData(NULL, m_dwLocalUIN, NULL, pData, 0x14, szFile);
						}
					}
					SAFE_FREE((void**)&hash);
					SAFE_FREE(&file);
				}
				break;
			}
			case 0x41: // request to upload avatar data
			case 0x81:
			{ // request to re-upload avatar data
				if (!m_bSsiEnabled) break; // we could not change serv-list if it is disabled...

				char *file = loadMyAvatarFileName();
				if (!file)
				{ // we have no file to upload, remove hash from server
					NetLog_Server("We do not have avatar, removing hash.");
					SetMyAvatar(0, 0);
					break;
				}
				DWORD dwPaFormat = DetectAvatarFormat(file);
				BYTE *hash = calcMD5HashOfFile(file);

				if (!hash)
				{ // the hash could not be calculated, remove from server
					NetLog_Server("We could not obtain hash, removing hash.");
					SetMyAvatar(0, 0);
				}
				else if (!memcmp(hash, pData + 4, 0x10))
				{ // we have the right file
					HANDLE hFile = NULL, hMap = NULL;
					BYTE *ppMap = NULL;
					long cbFileSize = 0;

					NetLog_Server("Uploading our avatar data.");

					if ((hFile = CreateFileA(file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL )) != INVALID_HANDLE_VALUE)
						if ((hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL)) != NULL)
							if ((ppMap = (BYTE*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0)) != NULL)
								cbFileSize = GetFileSize( hFile, NULL );

					if (cbFileSize != 0)
					{
						SetAvatarData(NULL, (WORD)(dwPaFormat == PA_FORMAT_XML ? AVATAR_HASH_FLASH : AVATAR_HASH_STATIC), (char*)ppMap, cbFileSize);
					}

					if (ppMap != NULL) UnmapViewOfFile(ppMap);
					if (hMap  != NULL) CloseHandle(hMap);
					if (hFile != NULL) CloseHandle(hFile);
					SAFE_FREE((void**)&hash);
				}
				else
				{
					BYTE* pHash = (BYTE*)_alloca(0x14);

					NetLog_Server("Our file is different, set our new hash.");

					pHash[0] = 0;
					pHash[1] = dwPaFormat == PA_FORMAT_XML ? AVATAR_HASH_FLASH : AVATAR_HASH_STATIC;
					pHash[2] = 1; // state of the hash
					pHash[3] = 0x10; // len of the hash
					memcpy(pHash + 4, hash, 0x10);
					updateServAvatarHash(pHash, 0x14);

					SAFE_FREE((void**)&hash);
				}

				SAFE_FREE(&file);
				break;
      }
			default:
				NetLog_Server("Received UNKNOWN Avatar Status.");
		}
	}
}


// handle Contact's avatar hash
void CIcqProto::handleAvatarContactHash(DWORD dwUIN, char* szUID, HANDLE hContact, BYTE* pHash, int nHashLen, WORD wOldStatus)
{
	DBVARIANT dbv;
	int bJob = FALSE;
	char szAvatar[MAX_PATH];
	int dwPaFormat;
	int avatarType = -1;
	BYTE* pAvatarHash = NULL;
	int cbAvatarHash;
	BYTE emptyItem[0x10] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	if (!m_bAvatarsEnabled) return; // only if enabled

	if (nHashLen < 4) return; // nothing to work with

	while (nHashLen >= 4)
	{ // parse online message items one by one
		WORD itemType = pHash[0] << 8 | pHash[1];
		BYTE itemLen = pHash[3];
		BYTE itemFlags = pHash[2];

		// just some validity check
		if (itemLen + 4 > nHashLen) 
			itemLen = nHashLen - 4;

		if (memcmp(pHash+4, emptyItem, itemLen > 0x10 ? 0x10 : itemLen))
		{ // Item types
			// 0000: AIM mini avatar
			// 0001: AIM/ICQ avatar ID/hash (len 5 or 16 bytes)
			// 0002: iChat online message
			// 0008: ICQ Flash avatar hash (16 bytes)
			// 0009: iTunes music store link
			// 000C: ICQ contact photo (16 bytes)
			// 000D: Last update time of online message
			// 000E: Status mood
			if (itemType == AVATAR_HASH_STATIC && (itemLen == 0x05 || itemLen == 0x10) && avatarType == -1)
			{ // normal avatar
				pAvatarHash = pHash;
				cbAvatarHash = itemLen + 4;
				avatarType = itemType;
			}
			else if (itemType == AVATAR_HASH_FLASH && itemLen == 0x10 && avatarType == -1)
			{ // flash avatar
				pAvatarHash = pHash;
				cbAvatarHash = itemLen + 4;
				avatarType = itemType;
			}
			else if (itemType == AVATAR_HASH_PHOTO && itemLen == 0x10)
			{ // big avatar (ICQ 6)
				pAvatarHash = pHash;
				cbAvatarHash = itemLen + 4;
				avatarType = itemType;
			}
		}
		pHash += itemLen + 4;
		nHashLen -= itemLen + 4;
	}

	if (avatarType != -1)
	{ // check settings, should we request avatar immediatelly
		BYTE bAutoLoad = getSettingByte(NULL, "AvatarsAutoLoad", DEFAULT_LOAD_AVATARS);

		if (avatarType == AVATAR_HASH_STATIC && cbAvatarHash == 0x09 && !memcmp(pAvatarHash + 4, hashEmptyAvatar + 4, 0x05))
		{ // empty avatar - unlink image, clear hash
			deleteSetting(hContact, "AvatarHash");
			BroadcastAck(hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS, NULL, (LPARAM)NULL);
			return;
		}

		if (getSetting(hContact, "AvatarHash", &dbv))
		{ // we not found old hash, i.e. get new avatar
			int fileState = IsAvatarChanged(hContact, pAvatarHash, cbAvatarHash);

			// check saved hash and file, if equal only store hash
			if (!fileState)
			{ // hashes are the same
				dwPaFormat = getSettingByte(hContact, "AvatarType", PA_FORMAT_UNKNOWN);

				GetFullAvatarFileName(dwUIN, szUID, dwPaFormat, szAvatar, MAX_PATH);
				if (_access(szAvatar, 0) == 0)
				{ // the file is there, link to contactphoto, save hash
					NetLog_Server("Avatar is known, hash stored, linked to file.");

					setSettingBlob(hContact, "AvatarHash", pAvatarHash, cbAvatarHash);
					BroadcastAck(hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS, NULL, (LPARAM)NULL);
				}
				else // the file is lost, request avatar again
					bJob = TRUE;
			}
			else
			{ // the hash is not the one we want, request avatar
				bJob = TRUE;
			}
		}
		else
		{ // we found hash check if it changed or not
			NetLog_Hash(this, "Old", dbv.pbVal, dbv.cpbVal);
			if ((dbv.cpbVal != cbAvatarHash) || memcmp(dbv.pbVal, pAvatarHash, cbAvatarHash))
			{ // the hash is different, request new avatar
				bJob = TRUE;
			}
			else
			{ // the hash does not changed, check if we have correct file
				int fileState = IsAvatarChanged(hContact, pAvatarHash, cbAvatarHash);

				// we should have file, check if the file really exists
				if (!fileState)
				{
					dwPaFormat = getSettingByte(hContact, "AvatarType", PA_FORMAT_UNKNOWN);
					if (dwPaFormat == PA_FORMAT_UNKNOWN)
					{ // we do not know the format, get avatar again
						bJob = 2;
					}
					else
					{
						GetFullAvatarFileName(dwUIN, szUID, dwPaFormat, szAvatar, MAX_PATH);
						if (_access(szAvatar, 0) != 0)
						{ // the file was lost, get it again
							bJob = 2;
						}
					}
				}
				else
				{ // the hash is not the one we want, request avatar
					bJob = 2;
				}
			}
			ICQFreeVariant(&dbv);
		}

		if (bJob)
		{
			if (bJob == TRUE)
			{
				NetLog_Hash(this, "New", (LPBYTE)pAvatarHash, cbAvatarHash);
				NetLog_Server("User has Avatar, new hash stored.");

				// Remove possible block - hash changed, try again.
				EnterCriticalSection(&cookieMutex);
				{
					avatarrequest *ar = pendingRequests;

					while (ar)
					{
						if (ar->hContact == hContact && ar->type == ART_BLOCK)
						{ // found one, remove
							ReleaseAvatarRequestInQueue(ar);
							break;
						}
						ar = ar->pNext;
					}
				}
				LeaveCriticalSection(&cookieMutex);
			}
			else
				NetLog_Server("User has Avatar, file is missing.");

			setSettingBlob(hContact, "AvatarHash", pAvatarHash, cbAvatarHash);

			BroadcastAck(hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS, NULL, (LPARAM)NULL);

			if (bAutoLoad)
			{ // auto-load is on, so request the avatar now, otherwise we are done
				GetAvatarFileName(dwUIN, szUID, szAvatar, MAX_PATH);
				GetAvatarData(hContact, dwUIN, szUID, pAvatarHash, cbAvatarHash, szAvatar);
			} // avatar request sent or added to queue
		}
		else
		{
			NetLog_Server("User has Avatar.");
		}
	}
	else if (wOldStatus == ID_STATUS_OFFLINE)
	{ // if user were offline, and now hash not found, clear the hash
		deleteSetting(hContact, "AvatarHash");
	}
}

static avatarrequest* CreateAvatarRequest(int type)
{
	avatarrequest *ar;

	ar = (avatarrequest*)SAFE_MALLOC(sizeof(avatarrequest));
	if (ar)
		ar->type = type;

	return ar;
}

// request avatar data from server
int CIcqProto::GetAvatarData(HANDLE hContact, DWORD dwUin, char *szUid, BYTE *hash, unsigned int hashlen, char *file)
{
	avatarthreadstartinfo* atsi;

	atsi = currentAvatarThread; // we take avatar thread - horrible, but realiable
	if (AvatarsReady && !atsi->paused) // check if we are ready
	{
		icq_packet packet;
		BYTE nUinLen;
		int i;
		DWORD dwNow = GetTickCount();

		EnterCriticalSection(&cookieMutex); // reused...
		{ // check if requests for this user are not blocked
			avatarrequest* ar = pendingRequests;

			while (ar)
			{
				if (ar->hContact == hContact && ar->type == ART_BLOCK)
				{ // found a block item
					if (GetTickCount() > ar->timeOut)
					{ // remove timeouted block
						ar = ReleaseAvatarRequestInQueue(ar);
						continue;
					}
					LeaveCriticalSection(&cookieMutex);
					NetLog_Server("Requests for %d avatar are blocked.", dwUin);

					return 0;
				}
				ar = ar->pNext;
			}
		}

		for(i = 0; i < atsi->runCount;)
		{ // look for timeouted requests
			if (atsi->runTime[i] < dwNow)
			{ // found outdated, remove
				atsi->runContact[i] = atsi->runContact[atsi->runCount - 1];
				atsi->runTime[i] = atsi->runTime[atsi->runCount - 1];
				atsi->runCount--;
			}
			else
				i++;
		}

		for(i = 0; i < atsi->runCount; i++)
		{
			if (atsi->runContact[i] == hContact)
			{
				LeaveCriticalSection(&cookieMutex);
				NetLog_Server("Ignoring duplicate get %d avatar request.", dwUin);

				return 0;
			}
		}

		if (atsi->runCount < 4)
		{ // 4 concurent requests at most
			int bSendNow = TRUE;

			EnterCriticalSection(&ratesMutex);
			{ // rate management
				WORD wGroup = atsi->rates->getGroupFromSNAC(ICQ_AVATAR_FAMILY, ICQ_AVATAR_GET_REQUEST);

				if (atsi->rates->getNextRateLevel(wGroup) < atsi->rates->getLimitLevel(wGroup, RML_ALERT))
				{ // we will be over quota if we send the request now, add to queue instead
					bSendNow = FALSE;
#ifdef _DEBUG
					NetLog_Server("Rates: Delay avatar request.");
#endif
				}
			}
			LeaveCriticalSection(&ratesMutex);

			if (bSendNow)
			{
				atsi->runContact[atsi->runCount] = hContact;
				atsi->runTime[atsi->runCount] = GetTickCount() + 30000; // 30sec to complete request
				atsi->runCount++;
				LeaveCriticalSection(&cookieMutex);

				nUinLen = getUIDLen(dwUin, szUid);

        cookie_avatar *ack = (cookie_avatar*)SAFE_MALLOC(sizeof(cookie_avatar));
				if (!ack) return 0; // out of memory, go away
				ack->dwUin = 1; //dwUin; // I should be damned for this - only to identify get request
				ack->hContact = hContact;
				ack->hash = (BYTE*)SAFE_MALLOC(hashlen);
				memcpy(ack->hash, hash, hashlen); // copy the data
				ack->hashlen = hashlen;
				ack->szFile = null_strdup(file); // we duplicate the string

				DWORD dwCookie = AllocateCookie(CKT_AVATAR, ICQ_AVATAR_GET_REQUEST, hContact, ack);

				serverPacketInit(&packet, (WORD)(12 + nUinLen + hashlen));
				packFNACHeader(&packet, ICQ_AVATAR_FAMILY, ICQ_AVATAR_GET_REQUEST, 0, dwCookie);
				packUID(&packet, dwUin, szUid);
				packByte(&packet, 1); // unknown, probably type of request: 1 = get icon :)
				packBuffer(&packet, (LPBYTE)hash, (unsigned short)hashlen);

				if (sendAvatarPacket(&packet, atsi))
				{
					NetLog_Server("Request to get %d avatar image sent.", dwUin);

					return dwCookie;
				}
				FreeCookie(dwCookie); // sending failed, free resources
				SAFE_FREE(&ack->szFile);
				SAFE_FREE((void**)&ack->hash);
				SAFE_FREE((void**)&ack);
			}
			else
				LeaveCriticalSection(&cookieMutex);
		}
		else
			LeaveCriticalSection(&cookieMutex);
	}
	// we failed to send request, or avatar thread not ready
	EnterCriticalSection(&cookieMutex); // wait for ready queue, reused cs
	{ // check if any request for this user is not already in the queue
		avatarrequest* ar = pendingRequests;
		int bYet = 0;

		while (ar)
		{
			if (ar->hContact == hContact)
			{ // we found it, return error
				if (ar->type == ART_BLOCK && GetTickCount() > ar->timeOut)
				{ // remove timeouted block
					ar = ReleaseAvatarRequestInQueue(ar);
					continue;
				}
				LeaveCriticalSection(&cookieMutex);
				NetLog_Server("Ignoring duplicate get %d avatar request.", dwUin);

				if (!AvatarsReady && !m_pendingAvatarsStart)
				{
					icq_requestnewfamily(ICQ_AVATAR_FAMILY, &CIcqProto::StartAvatarThread);
					m_pendingAvatarsStart = 1;
				}
				return 0;
			}
			ar = ar->pNext;
		}
		// add request to queue, processed after successful login
		ar = CreateAvatarRequest(ART_GET); // get avatar
		if (!ar)
		{ // out of memory, go away
			LeaveCriticalSection(&cookieMutex);
			return 0;
		}
		ar->dwUin = dwUin;
		ar->szUid = null_strdup(szUid);
		ar->hContact = hContact;
		ar->hash = (BYTE*)SAFE_MALLOC(hashlen);
		memcpy(ar->hash, hash, hashlen); // copy the data
		ar->hashlen = hashlen;
		ar->szFile = null_strdup(file); // duplicate the string
		ar->pNext = pendingRequests;
		pendingRequests = ar;
	}
	LeaveCriticalSection(&cookieMutex);

	NetLog_Server("Request to get %d avatar image added to queue.", dwUin);

	if (!AvatarsReady && !m_pendingAvatarsStart)
	{
		icq_requestnewfamily(ICQ_AVATAR_FAMILY, &CIcqProto::StartAvatarThread);
		m_pendingAvatarsStart = 1;
	}

	return -1; // we added to queue
}

// upload avatar data to server
int CIcqProto::SetAvatarData(HANDLE hContact, WORD wRef, char* data, unsigned int datalen)
{ 
	avatarthreadstartinfo* atsi;

	atsi = currentAvatarThread; // we take avatar thread - horrible, but realiable
	if (AvatarsReady && !atsi->paused) // check if we are ready
	{
		icq_packet packet;

    cookie_avatar* ack = (cookie_avatar*)SAFE_MALLOC(sizeof(cookie_avatar));
		if (!ack) return 0; // out of memory, go away
		ack->hContact = hContact;

		DWORD dwCookie = AllocateCookie(CKT_AVATAR, ICQ_AVATAR_UPLOAD_REQUEST, 0, ack);

		serverPacketInit(&packet, (WORD)(14 + datalen));
		packFNACHeader(&packet, ICQ_AVATAR_FAMILY, ICQ_AVATAR_UPLOAD_REQUEST, 0, dwCookie);
		packWord(&packet, wRef); // unknown, probably reference
		packWord(&packet, (WORD)datalen);
		packBuffer(&packet, (LPBYTE)data, (unsigned short)datalen);

		if (sendAvatarPacket(&packet, atsi))
		{
			NetLog_Server("Upload avatar packet sent.");

			return dwCookie;
		}
		ReleaseCookie(dwCookie); // failed to send, free resources
	}
	// we failed to send request, or avatar thread not ready
	EnterCriticalSection(&cookieMutex); // wait for ready queue, reused cs
	{ // check if any request for this user is not already in the queue
		avatarrequest* ar;
		int bYet = 0;
		ar = pendingRequests;
		while (ar)
		{
			if (ar->hContact == hContact && ar->type == ART_UPLOAD)
			{ // we found it, return error
				LeaveCriticalSection(&cookieMutex);
				NetLog_Server("Ignoring duplicate upload avatar request.");

				if (!AvatarsReady && !m_pendingAvatarsStart)
				{
					icq_requestnewfamily(ICQ_AVATAR_FAMILY, &CIcqProto::StartAvatarThread);
					m_pendingAvatarsStart = 1;
				}
				return 0;
			}
			ar = ar->pNext;
		}
		// add request to queue, processed after successful login
		ar = CreateAvatarRequest(ART_UPLOAD); // upload avatar
		if (!ar)
		{ // out of memory, go away
			LeaveCriticalSection(&cookieMutex);
			return 0;
		}
		ar->hContact = hContact;
		ar->pData = (char*)SAFE_MALLOC(datalen);
		if (!ar->pData)
		{ // alloc failed
			LeaveCriticalSection(&cookieMutex);
			SAFE_FREE((void**)&ar);
			return 0;
		}
		memcpy(ar->pData, data, datalen); // copy the data
		ar->cbData = datalen;
		ar->wRef = wRef;
		ar->pNext = pendingRequests;
		pendingRequests = ar;
	}
	LeaveCriticalSection(&cookieMutex);

	NetLog_Server("Request to upload avatar image added to queue.");

	if (!AvatarsReady && !m_pendingAvatarsStart)
	{
		m_pendingAvatarsStart = 1;
		icq_requestnewfamily(ICQ_AVATAR_FAMILY, &CIcqProto::StartAvatarThread);
	}

	return -1; // we added to queue
}

void __cdecl CIcqProto::AvatarThread(avatarthreadstartinfo *atsi)
{
	// This is the "infinite" loop that receives the packets from the ICQ avatar server
	int recvResult;
	NETLIBPACKETRECVER packetRecv = {0};
	DWORD wLastKeepAlive = 0; // we send keep-alive at most one per 30secs
	DWORD dwKeepAliveInterval = getSettingDword(NULL, "KeepAliveInterval", KEEPALIVE_INTERVAL);

	InitializeCriticalSection(&atsi->localSeqMutex);

	atsi->hAvatarPacketRecver = (HANDLE)CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)atsi->hConnection, 32768);
	packetRecv.cbSize = sizeof(packetRecv);
	packetRecv.dwTimeout = dwKeepAliveInterval < KEEPALIVE_INTERVAL ? dwKeepAliveInterval: KEEPALIVE_INTERVAL; // timeout - for stopThread to work
	while(!atsi->stopThread)
	{
		recvResult = CallService(MS_NETLIB_GETMOREPACKETS,(WPARAM)atsi->hAvatarPacketRecver, (LPARAM)&packetRecv);

		if (recvResult == 0)
		{
			NetLog_Server("Clean closure of avatar socket");
			break;
		}

		if (recvResult == SOCKET_ERROR)
		{
			if (GetLastError() == ERROR_TIMEOUT)
			{  // timeout, check if we should be still running
				if (Miranda_Terminated())
					atsi->stopThread = 1; // we must stop here, cause due to a hack in netlib, we always get timeout, even if the connection is already dead
#ifdef _DEBUG
				else
					NetLog_Server("Avatar Thread is Idle.");
#endif
				if (GetTickCount() > wLastKeepAlive)
				{ // limit frequency (HACK: on some systems select() does not work well)
					if (getSettingByte(NULL, "KeepAlive", 0))
					{ // send keep-alive packet
						icq_packet packet;

						packet.wLen = 0;
						write_flap(&packet, ICQ_PING_CHAN);
						sendAvatarPacket(&packet, atsi);
					}
					wLastKeepAlive = GetTickCount() + dwKeepAliveInterval;
				}
				else
				{ // this is bad, the system does not handle select() properly
#ifdef _DEBUG
					NetLog_Server("Avatar Thread is Forcing Idle.");
#endif
					SleepEx(500, TRUE); // wait some time, can we do anything else ??
				} // FIXME: we should check the avatar queue now
				continue; 
			}
			NetLog_Server("Abortive closure of avatar socket");
			break;
		}

		// Deal with the packet
		packetRecv.bytesUsed = handleAvatarPackets(packetRecv.buffer, packetRecv.bytesAvailable, atsi);

		if ((AvatarsReady == TRUE) && (packetRecv.bytesAvailable == packetRecv.bytesUsed) && !atsi->paused) // no packets pending
		{ // process request queue
			EnterCriticalSection(&cookieMutex);

			while (pendingRequests && atsi->runCount < 3) // pick up an request and send it - happens immediatelly after login
			{ // do not fill queue to top, leave one place free
				avatarrequest *reqdata = pendingRequests;

				EnterCriticalSection(&ratesMutex);
				{ // rate management
					WORD wGroup = atsi->rates->getGroupFromSNAC(ICQ_AVATAR_FAMILY, (WORD)(reqdata->type == ART_UPLOAD ? ICQ_AVATAR_GET_REQUEST : ICQ_AVATAR_UPLOAD_REQUEST));

					if (atsi->rates->getNextRateLevel(wGroup) < atsi->rates->getLimitLevel(wGroup, RML_ALERT))
					{ // we are over rate, leave queue and wait
#ifdef _DEBUG
						NetLog_Server("Rates: Leaving avatar queue processing");
#endif
						LeaveCriticalSection(&ratesMutex);
						break;
					}
				}
				LeaveCriticalSection(&ratesMutex);

				pendingRequests = reqdata->pNext;

#ifdef _DEBUG
				NetLog_Server("Picked up the %d request from queue.", reqdata->dwUin);
#endif
				switch (reqdata->type)
				{
				case ART_GET: // get avatar
					GetAvatarData(reqdata->hContact, reqdata->dwUin, reqdata->szUid, reqdata->hash, reqdata->hashlen, reqdata->szFile);
					break;
				case ART_UPLOAD: // set avatar
					SetAvatarData(reqdata->hContact, reqdata->wRef, reqdata->pData, reqdata->cbData);
					break;
				case ART_BLOCK: // block contact processing
					if (GetTickCount() < reqdata->timeOut)
					{ // it is not time, keep request in queue
						if (pendingRequests)
						{ // the queue contains items, jump the queue
							reqdata->pNext = pendingRequests->pNext;
							pendingRequests->pNext = reqdata;
						}
						else
							pendingRequests = reqdata;
						reqdata = NULL;
					}
					break;
				}
				FreeAvatarRequest(&reqdata);

				if (pendingRequests && pendingRequests->type == ART_BLOCK) break; // leave the loop 
			}

			LeaveCriticalSection(&cookieMutex);
		}
	}
	NetLib_SafeCloseHandle(&atsi->hAvatarPacketRecver); // Close the packet receiver 
	NetLib_CloseConnection(&atsi->hConnection, FALSE); // Close the connection

	// release rates
	EnterCriticalSection(&ratesMutex);
  delete atsi->rates;
  atsi->rates = NULL;
	LeaveCriticalSection(&ratesMutex);

	DeleteCriticalSection(&atsi->localSeqMutex);

	SAFE_FREE((void**)&atsi->pCookie);
	if (currentAvatarThread == atsi) // if we stoped by error or unexpectedly, clear global variable
	{
		AvatarsReady = FALSE; // we are not ready
		m_pendingAvatarsStart = 0;
		currentAvatarThread = NULL; // this is horrible, rewrite
	}
	SAFE_FREE((void**)&atsi);

	NetLog_Server("%s thread ended.", "Avatar");
}

int CIcqProto::handleAvatarPackets(BYTE *buf, int buflen, avatarthreadstartinfo *atsi)
{
	BYTE channel;
	WORD sequence;
	WORD datalen;
	int bytesUsed = 0;

	while (buflen > 0)
	{
		// All FLAPS begin with 0x2a
		if (*buf++ != FLAP_MARKER)
			break;

		if (buflen < 6)
			break;

		unpackByte(&buf, &channel);
		unpackWord(&buf, &sequence);
		unpackWord(&buf, &datalen);

		if (buflen < 6 + datalen)
			break;

#ifdef _DEBUG
		NetLog_Server("Avatar FLAP: Channel %u, Seq %u, Length %u bytes", channel, sequence, datalen);
#endif

		switch (channel)
		{
		case ICQ_LOGIN_CHAN:
			handleAvatarLogin(buf, datalen, atsi);
			break;

		case ICQ_DATA_CHAN:
			handleAvatarData(buf, datalen, atsi);
			break;

		default:
			NetLog_Server("Warning: Unhandled %s FLAP Channel: Channel %u, Seq %u, Length %u bytes", "Avatar", channel, sequence, datalen);
			break;
		}

		/* Increase pointers so we can check for more FLAPs */
		buf += datalen;
		buflen -= (datalen + 6);
		bytesUsed += (datalen + 6);
	}

	return bytesUsed;
}

int CIcqProto::sendAvatarPacket(icq_packet* pPacket, avatarthreadstartinfo* atsi)
{
	int lResult = 0;

	// This critsec makes sure that the sequence order doesn't get screwed up
	EnterCriticalSection(&atsi->localSeqMutex);

	if (atsi->hConnection)
	{
		int nRetries;
		int nSendResult;

		// :IMPORTANT:
		// The FLAP sequence must be a WORD. When it reaches 0xFFFF it should wrap to
		// 0x0000, otherwise we'll get kicked by server.
		atsi->wLocalSequence++;

		// Pack sequence number
		pPacket->pData[2] = ((atsi->wLocalSequence & 0xff00) >> 8);
		pPacket->pData[3] = (atsi->wLocalSequence & 0x00ff);

		for (nRetries = 3; nRetries >= 0; nRetries--)
		{
			nSendResult = Netlib_Send(atsi->hConnection, (const char *)pPacket->pData, pPacket->wLen, 0);

			if (nSendResult != SOCKET_ERROR)
				break;

			Sleep(1000);
		}

		// Send error
		if (nSendResult == SOCKET_ERROR)
		{ // thread stops automatically
			NetLog_Server("Your connection with the ICQ avatar server was abortively closed");
		}
		else
		{
			lResult = 1; // packet sent successfully

			EnterCriticalSection(&ratesMutex); // TODO: we should have our own mutex
			if (atsi->rates)
				atsi->rates->packetSent(pPacket);
			LeaveCriticalSection(&ratesMutex);
		}
	}
	else
	{
		NetLog_Server("Error: Failed to send packet (no connection)");
	}

	LeaveCriticalSection(&atsi->localSeqMutex);

	SAFE_FREE((void**)&pPacket->pData);

	return lResult;
}

void CIcqProto::handleAvatarLogin(BYTE *buf, WORD datalen, avatarthreadstartinfo *atsi)
{
	icq_packet packet;

	if (*(DWORD*)buf == 0x1000000)
	{  // here check if we received SRV_HELLO
		atsi->wLocalSequence = generate_flap_sequence(); 

		serverCookieInit(&packet, (LPBYTE)atsi->pCookie, atsi->wCookieLen);
		sendAvatarPacket(&packet, atsi);

#ifdef _DEBUG
		NetLog_Server("Sent CLI_IDENT to %s server", "avatar");
#endif

		SAFE_FREE((void**)&atsi->pCookie);
		atsi->wCookieLen = 0;
	}
	else
	{
		NetLog_Server("Invalid Avatar Server response, Ch1.");
	}
}

void CIcqProto::handleAvatarData(BYTE *pBuffer, WORD wBufferLength, avatarthreadstartinfo *atsi)
{
	snac_header snacHeader = {0};

	if (!unpackSnacHeader(&snacHeader, &pBuffer, &wBufferLength) || !snacHeader.bValid)
	{
		NetLog_Server("Error: Failed to parse SNAC header");
	}
	else
	{
#ifdef _DEBUG
		NetLog_Server(" Received SNAC(x%02X,x%02X)", snacHeader.wFamily, snacHeader.wSubtype);
#endif

		switch (snacHeader.wFamily)
		{

		case ICQ_SERVICE_FAMILY:
			handleAvatarServiceFam(pBuffer, wBufferLength, &snacHeader, atsi);
			break;

		case ICQ_AVATAR_FAMILY:
			handleAvatarFam(pBuffer, wBufferLength, &snacHeader, atsi);
			break;

		default:
			NetLog_Server("Ignoring SNAC(x%02X,x%02X) - FAMILYx%02X not implemented", snacHeader.wFamily, snacHeader.wSubtype, snacHeader.wFamily);
			break;
		}
	}
}

void CIcqProto::handleAvatarServiceFam(BYTE *pBuffer, WORD wBufferLength, snac_header *pSnacHeader, avatarthreadstartinfo *atsi)
{
	icq_packet packet;

	switch (pSnacHeader->wSubtype)
	{

	case ICQ_SERVER_READY:
#ifdef _DEBUG
		NetLog_Server("Avatar server is ready and is requesting my Family versions");
		NetLog_Server("Sending my Families");
#endif

		// Miranda mimics the behaviour of Icq5
		serverPacketInit(&packet, 18);
		packFNACHeader(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_FAMILIES);
		packDWord(&packet, 0x00010004);
		packDWord(&packet, 0x00100001);
		sendAvatarPacket(&packet, atsi);
		break;

	case ICQ_SERVER_FAMILIES2:
		/* This is a reply to CLI_FAMILIES and it tells the client which families and their versions that this server understands.
		* We send a rate request packet */
#ifdef _DEBUG
		NetLog_Server("Server told me his Family versions");
		NetLog_Server("Requesting Rate Information");
#endif
		serverPacketInit(&packet, 10);
		packFNACHeader(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_REQ_RATE_INFO);
		sendAvatarPacket(&packet, atsi);
		break;

	case ICQ_SERVER_RATE_INFO:
#ifdef _DEBUG
		NetLog_Server("Server sent Rate Info");
		NetLog_Server("Sending Rate Info Ack");
#endif
		/* init rates management */
		atsi->rates = new rates(this, pBuffer, wBufferLength);
		/* ack rate levels */
    atsi->rates->initAckPacket(&packet);
		sendAvatarPacket(&packet, atsi);

		// send cli_ready
		serverPacketInit(&packet, 26);
		packFNACHeader(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_READY);
		packDWord(&packet, 0x00010004); // mimic ICQ 6
		packDWord(&packet, 0x0010164f);
		packDWord(&packet, 0x00100001);
		packDWord(&packet, 0x0010164f);
		sendAvatarPacket(&packet, atsi);

		AvatarsReady = TRUE; // we are ready to process requests
		m_pendingAvatarsStart = 0;
		atsi->pendingLogin = 0;

		NetLog_Server(" *** Yeehah, avatar login sequence complete");
		break;

	default:
		NetLog_Server("Warning: Ignoring SNAC(x%02x,x%02x) - Unknown SNAC (Flags: %u, Ref: %u)", ICQ_SERVICE_FAMILY, pSnacHeader->wSubtype, pSnacHeader->wFlags, pSnacHeader->dwRef);
		break;
	}
}

void CIcqProto::handleAvatarFam(BYTE *pBuffer, WORD wBufferLength, snac_header *pSnacHeader, avatarthreadstartinfo *atsi)
{
	switch (pSnacHeader->wSubtype) {

	case ICQ_AVATAR_GET_REPLY:  // received avatar data, store to file
		{ // handle new avatar, notify
			cookie_avatar *pCookieData;

			if (FindCookie(pSnacHeader->dwRef, NULL, (void**)&pCookieData))
			{
				BYTE len;
				WORD datalen;
				int out;
				char* szMyFile = (char*)_alloca(strlennull(pCookieData->szFile)+10);
				PROTO_AVATAR_INFORMATION ai;
				int i;
				BYTE bResult;

				EnterCriticalSection(&cookieMutex);
				for(i = 0; i < atsi->runCount; i++)
				{ // look for our record
					if (atsi->runContact[i] == pCookieData->hContact)
					{ // found remove
						atsi->runContact[i] = atsi->runContact[atsi->runCount - 1];
						atsi->runTime[i] = atsi->runTime[atsi->runCount - 1];
						atsi->runCount--;
						break;
					}
				}
				LeaveCriticalSection(&cookieMutex);

				strcpy(szMyFile, pCookieData->szFile);

				ai.cbSize = sizeof ai;
				ai.format = PA_FORMAT_JPEG; // this is for error only
				ai.hContact = pCookieData->hContact;
				strcpy(ai.filename, pCookieData->szFile);
				AddAvatarExt(PA_FORMAT_JPEG, ai.filename); 

				FreeCookie(pSnacHeader->dwRef);
				unpackByte(&pBuffer, &len);
				if (wBufferLength < ((pCookieData->hashlen)<<1)+4+len)
				{
					NetLog_Server("Received invalid avatar reply.");

					BroadcastAck(pCookieData->hContact, ACKTYPE_AVATAR, ACKRESULT_FAILED, (HANDLE)&ai, 0);

					SAFE_FREE(&pCookieData->szFile);
					SAFE_FREE((void**)&pCookieData->hash);
					SAFE_FREE((void**)&pCookieData);

					break;
				}

				pBuffer += len;
				pBuffer += pCookieData->hashlen;
				unpackByte(&pBuffer, &bResult);
				pBuffer += pCookieData->hashlen;
				unpackWord(&pBuffer, &datalen);

				wBufferLength -= 4 + len + (pCookieData->hashlen<<1);
				if (datalen > wBufferLength)
				{
					datalen = wBufferLength;
					NetLog_Server("Avatar reply broken, trying to do my best.");
				}

				if (datalen > 4)
				{ // store to file...
					int dwPaFormat;
					int aValid = 1;

					if (pCookieData->hashlen == 0x14 && pCookieData->hash[3] == 0x10 && getSettingByte(NULL, "StrictAvatarCheck", DEFAULT_AVATARS_CHECK))
					{ // check only standard hashes
						mir_md5_state_t state;
						mir_md5_byte_t digest[16];

						mir_md5_init(&state);
						mir_md5_append(&state, (const mir_md5_byte_t *)pBuffer, datalen);
						mir_md5_finish(&state, digest);
						// check if received data corresponds to specified hash
						if (memcmp(pCookieData->hash+4, digest, 0x10)) aValid = 0;
					}

					if (aValid)
					{
						NetLog_Server("Received user avatar, storing (%d bytes).", datalen);

						dwPaFormat = DetectAvatarFormatBuffer((char*)pBuffer);
						setSettingByte(pCookieData->hContact, "AvatarType", (BYTE)dwPaFormat);
						ai.format = dwPaFormat; // set the format
						AddAvatarExt(dwPaFormat, szMyFile);
						strcpy(ai.filename, szMyFile);

						out = _open(szMyFile, _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY, _S_IREAD | _S_IWRITE);
						if (out) 
						{
							DBVARIANT dbv;

							_write(out, pBuffer, datalen);
							_close(out);

							if (!pCookieData->hContact) // our avatar, set filename
							{
								char tmp[MAX_PATH];
								CallService(MS_UTILS_PATHTORELATIVE, (WPARAM)szMyFile, (LPARAM)tmp);
								setSettingString(NULL, "AvatarFile", tmp);
							}
							else
							{ // contact's avatar set hash
								if (!getSetting(pCookieData->hContact, "AvatarHash", &dbv))
								{
									if (setSettingBlob(pCookieData->hContact, "AvatarSaved", dbv.pbVal, dbv.cpbVal))
										NetLog_Server("Failed to set file hash.");

									ICQFreeVariant(&dbv);
								}
								else
								{
									NetLog_Server("Warning: DB error (no hash in DB).");
									// the hash was lost, try to fix that
									if (setSettingBlob(pCookieData->hContact, "AvatarSaved", pCookieData->hash, pCookieData->hashlen) ||
										setSettingBlob(pCookieData->hContact, "AvatarHash", pCookieData->hash, pCookieData->hashlen))
									{
										NetLog_Server("Failed to save avatar hash to DB");
									}
								}
							}

							BroadcastAck(pCookieData->hContact, ACKTYPE_AVATAR, ACKRESULT_SUCCESS, (HANDLE)&ai, 0);
						}  
					}
					else
					{ // avatar is broken
						NetLog_Server("Error: Avatar data does not match avatar hash, ignoring.");

						if (pCookieData->hContact)
						{
							avatarrequest *ar = CreateAvatarRequest(ART_BLOCK);

							EnterCriticalSection(&cookieMutex);
							if (ar)
							{
								avatarrequest *last = pendingRequests;

								ar->hContact = pCookieData->hContact;
								ar->timeOut = GetTickCount() + 14400000; // do not allow re-request four hours

								// add it to the end of queue, i.e. do not block other requests
								while (last && last->pNext) last = last->pNext;
								if (last)
									last->pNext = ar;
								else
									pendingRequests = ar;
							}
							LeaveCriticalSection(&cookieMutex);
						}
						BroadcastAck(pCookieData->hContact, ACKTYPE_AVATAR, ACKRESULT_FAILED, (HANDLE)&ai, 0);
					}
				}
				else
				{ // the avatar is empty
					NetLog_Server("Received empty avatar, nothing written (error 0x%x).", bResult);

					BroadcastAck(pCookieData->hContact, ACKTYPE_AVATAR, ACKRESULT_FAILED, (HANDLE)&ai, 0);
				}
				SAFE_FREE(&pCookieData->szFile);
				SAFE_FREE((void**)&pCookieData->hash);
				SAFE_FREE((void**)&pCookieData);
			}
			else
			{
				NetLog_Server("Warning: Received unexpected Avatar Reply SNAC(x10,x07).");
			}

			break;
		}
	case ICQ_AVATAR_UPLOAD_ACK:
		{
			// upload completed, notify
			BYTE res;
			unpackByte(&pBuffer, &res);
			if (!res && (wBufferLength == 0x15))
			{
				cookie_avatar *pCookieData;
				if (FindCookie(pSnacHeader->dwRef, NULL, (void**)&pCookieData))
				{
					// here we store the local hash
					ReleaseCookie(pSnacHeader->dwRef);
				}
				else
				{
					NetLog_Server("Warning: Received unexpected Upload Avatar Reply SNAC(x10,x03).");
				}
			}
			else if (res)
			{
				NetLog_Server("Error uploading avatar to server, #%d", res);
				icq_LogMessage(LOG_WARNING, LPGEN("Error uploading avatar to server, server refused to accept the image."));
			}
			else
				NetLog_Server("Received invalid upload avatar ack.");

			break;
		}
	case ICQ_ERROR:
		{
			WORD wError;
			cookie_avatar *pCookieData;

			if (FindCookie(pSnacHeader->dwRef, NULL, (void**)&pCookieData))
			{
				if (pCookieData->dwUin)
				{
					NetLog_Server("Error: Avatar request failed");
					SAFE_FREE(&pCookieData->szFile);
					SAFE_FREE((void**)&pCookieData->hash);
				}
				else
				{
					NetLog_Server("Error: Avatar upload failed");
				}
				ReleaseCookie(pSnacHeader->dwRef);
			}

			if (wBufferLength >= 2)
				unpackWord(&pBuffer, &wError);
			else 
				wError = 0;

			LogFamilyError(ICQ_AVATAR_FAMILY, wError);
			break;
		}
	default:
		NetLog_Server("Warning: Ignoring SNAC(x%02x,x%02x) - Unknown SNAC (Flags: %u, Ref: %u)", ICQ_AVATAR_FAMILY, pSnacHeader->wSubtype, pSnacHeader->wFlags, pSnacHeader->dwRef);
		break;

	}
}
