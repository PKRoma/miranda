// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright � 2000-2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright � 2001-2002 Jon Keating, Richard Hughes
// Copyright � 2002-2004 Martin �berg, Sam Kothari, Robert Rainwater
// Copyright � 2004-2010 Joe Kucera
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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


BYTE hashEmptyAvatar[9] = {0x00, 0x01, 0x00, 0x05, 0x02, 0x01, 0xD2, 0x04, 0x72};


avatars_request::avatars_request(int type)
{
	this->type = type;
}


avatars_request::~avatars_request()
{
	if (this)
	{
		switch (type)
		{
		case ART_UPLOAD:
			SAFE_FREE((void**)&pData);
			break;
		case ART_GET:
			SAFE_FREE((void**)&hash);
			SAFE_FREE(&szFile);
			break;
		case ART_BLOCK:
			break;
		}
	}
}


avatars_request* CIcqProto::ReleaseAvatarRequestInQueue(avatars_request *request)
{
	avatars_request *pNext = request->pNext;

	avatars_request **par = &m_avatarsQueue;
	avatars_request *ar = m_avatarsQueue;

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

	SAFE_DELETE(&request);

	return pNext;
}


void CIcqProto::InitAvatars()
{
	if (!bAvatarsFolderInited)
	{ // do it only once
		bAvatarsFolderInited = TRUE;

		if (ServiceExists(MS_FOLDERS_REGISTER_PATH))
		{ // check if it does make sense
			TCHAR tszPath[MAX_PATH * 2];
			TCHAR *tmpPath = Utils_ReplaceVarsT(_T("%miranda_avatarcache%"));
			null_snprintf(tszPath, MAX_PATH * 2, _T("%s\\") _T(TCHAR_STR_PARAM) _T("\\"), tmpPath, m_szModuleName);
			mir_free(tmpPath);

			hAvatarsFolder = FoldersRegisterCustomPathT(m_szModuleName, "Avatars Cache", tszPath);
		}
	}
}


TCHAR* CIcqProto::GetOwnAvatarFileName()
{
	DBVARIANT dbvFile = {DBVT_DELETED};

	if (!getSettingStringT(NULL, "AvatarFile", &dbvFile))
	{
		TCHAR tmp[MAX_PATH * 2];
		CallService(MS_UTILS_PATHTOABSOLUTET, (WPARAM)dbvFile.ptszVal, (LPARAM)tmp);
		ICQFreeVariant(&dbvFile);

		return null_strdup(tmp);
	}
	return NULL;
}


void CIcqProto::GetFullAvatarFileName(int dwUin, const char *szUid, int dwFormat, TCHAR *pszDest, int cbLen)
{
	GetAvatarFileName(dwUin, szUid, pszDest, cbLen);
	AddAvatarExt(dwFormat, pszDest);
}


void CIcqProto::GetAvatarFileName(int dwUin, const char *szUid, TCHAR *pszDest, int cbLen)
{
	TCHAR szPath[MAX_PATH * 2];
	FOLDERSGETDATA fgd = {0};

	InitAvatars();

	fgd.cbSize = sizeof(FOLDERSGETDATA);
	fgd.nMaxPathSize = MAX_PATH * 2;
	fgd.szPathT = szPath;
	if (CallService(MS_FOLDERS_GET_PATH, (WPARAM)hAvatarsFolder, (LPARAM)&fgd))
	{
		TCHAR *tmpPath = Utils_ReplaceVarsT(_T("%miranda_avatarcache%"));
		null_snprintf(szPath, MAX_PATH * 2, _T("%s\\") _T(TCHAR_STR_PARAM) _T("\\"), tmpPath, m_szModuleName);
		mir_free(tmpPath);
	}
	else
		_tcscat(szPath, _T("\\"));
	
	// fill the destination
	lstrcpyn(pszDest, szPath, cbLen - 1);
	int tPathLen = strlennull(pszDest);

	// make sure the avatar cache directory exists
	CallService(MS_UTILS_CREATEDIRTREET, 0, (LPARAM)szPath);

	if (dwUin != 0) 
	{
		_ltot(dwUin, pszDest + tPathLen, 10);
	}
	else if (szUid)
	{
		TCHAR* p = mir_a2t(szUid);
		_tcscpy(pszDest + tPathLen, p);
		mir_free( p );
	}
	else
	{
		TCHAR szBuf[MAX_PATH];

		if (CallService(MS_DB_GETPROFILENAMET, MAX_PATH, (LPARAM)szBuf))
			_tcscpy(pszDest + tPathLen, _T("avatar"));
		else 
		{
			TCHAR *szLastDot = _tcsrchr(szBuf, '.');
			if (szLastDot) szLastDot[0] = '\0';

			_tcscpy(pszDest + tPathLen, szBuf);
			_tcscat(pszDest + tPathLen, _T("_avt"));
		}
	}
}


void AddAvatarExt(int dwFormat, TCHAR *pszDest)
{
	if (dwFormat == PA_FORMAT_JPEG)
		_tcscat(pszDest, _T(".jpg"));
	else if (dwFormat == PA_FORMAT_GIF)
		_tcscat(pszDest, _T(".gif"));
	else if (dwFormat == PA_FORMAT_PNG)
		_tcscat(pszDest, _T(".png"));
	else if (dwFormat == PA_FORMAT_BMP)
		_tcscat(pszDest, _T(".bmp"));
	else if (dwFormat == PA_FORMAT_XML)
		_tcscat(pszDest, _T(".xml"));
	else if (dwFormat == PA_FORMAT_SWF)
		_tcscat(pszDest, _T(".swf"));
	else
		_tcscat(pszDest, _T(".dat"));
}


int DetectAvatarFormatBuffer(const char *pBuffer)
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


int DetectAvatarFormat(const TCHAR *tszFile)
{
	int src = _topen(tszFile, _O_BINARY | _O_RDONLY, 0);
	if (src == -1)
		return PA_FORMAT_UNKNOWN;

	char pBuf[32];
	_read(src, pBuf, 32);
	_close(src);

	return DetectAvatarFormatBuffer(pBuf);
}


#define MD5_BLOCK_SIZE 1024*1024 /* use 1MB blocks */

BYTE* calcMD5HashOfFile(const TCHAR *tszFile)
{
	BYTE *res = NULL;

	HANDLE hFile = NULL, hMap = NULL;

	if ((hFile = CreateFile(tszFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL )) != INVALID_HANDLE_VALUE)
	{
		if ((hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL)) != NULL)
		{
			long cbFileSize = GetFileSize(hFile, NULL);

			res = (BYTE*)SAFE_MALLOC(16 * sizeof(mir_md5_byte_t));
			if (cbFileSize != 0 && res)
			{
				mir_md5_state_t state;
				mir_md5_byte_t digest[16];
				int dwOffset = 0;

				mir_md5_init(&state);
				while (dwOffset < cbFileSize)
				{
					BYTE *ppMap = NULL;
					int dwBlockSize = min(MD5_BLOCK_SIZE, cbFileSize-dwOffset);

					if (!(ppMap = (BYTE*)MapViewOfFile(hMap, FILE_MAP_READ, 0, dwOffset, dwBlockSize)))
						break;
					mir_md5_append(&state, (const mir_md5_byte_t *)ppMap, dwBlockSize);
					UnmapViewOfFile(ppMap);
					dwOffset += dwBlockSize;
				}
				mir_md5_finish(&state, digest);
				memcpy(res, digest, 16 * sizeof(mir_md5_byte_t));
			}
		}

		if (hMap  != NULL) CloseHandle(hMap);
		if (hFile != NULL) CloseHandle(hFile);
	}

	return res;
}


int CIcqProto::IsAvatarChanged(HANDLE hContact, const BYTE *pHash, int nHashLen)
{
	DBVARIANT dbvSaved = {0};

	if (!getSetting(hContact, "AvatarSaved", &dbvSaved))
	{
		if ((dbvSaved.cpbVal != nHashLen) || memcmp(dbvSaved.pbVal, pHash, nHashLen))
		{ // the hashes are different
			ICQFreeVariant(&dbvSaved);

			return 2;
		}
		ICQFreeVariant(&dbvSaved);

		return 0; // hash is there and is the same - Success
	}
	return 1; // saved Avatar hash is missing
}


void CIcqProto::StartAvatarThread(HANDLE hConn, char *cookie, WORD cookieLen) // called from event
{
	if (!hConn)
	{
		icq_lock l(m_avatarsMutex); // place avatars lock

		if (m_avatarsConnection && m_avatarsConnection->isPending())
		{
			NetLog_Server("Avatar, Multiple start thread attempt, ignored.");
			SAFE_FREE((void**)&cookie);
			return;
		}
		NetLog_Server("Avatars: Connection failed");

		m_avatarsConnectionPending = FALSE;

		{ // check if any upload request are waiting in the queue
			avatars_request *ar = m_avatarsQueue;
			int bYet = 0;

			while (ar)
			{
				if (ar->type == ART_UPLOAD)
				{ // we found it, return error
					if (!bYet)
					{
						icq_LogMessage(LOG_WARNING, LPGEN("Error uploading avatar to server, server temporarily unavailable."));
						bYet = 1;
					}
					// remove upload request from queue
					ar = ReleaseAvatarRequestInQueue(ar);
					continue;
				}
				ar = ar->pNext;
			}
		}
		SAFE_FREE((void**)&cookie);

		return;
	}

	icq_lock l(m_avatarsMutex);

	if (m_avatarsConnection && m_avatarsConnection->isPending())
	{
		NetLog_Server("Avatar, Multiple start thread attempt, ignored.");

		NetLib_CloseConnection(&hConn, FALSE);

		SAFE_FREE((void**)&cookie);

		return;
	}
	else if (m_avatarsConnection)
		m_avatarsConnection->closeConnection();

	m_avatarsConnection = new avatars_server_connection(this, hConn, cookie, cookieLen); // the old connection should not be used anymore

	// connection object created, remove the connection request pending flag
	m_avatarsConnectionPending = FALSE;
}


void CIcqProto::StopAvatarThread()
{
	icq_lock l(m_avatarsMutex); // the connection is about to close

	if (m_avatarsConnection)
	{
		m_avatarsConnection->shutdownConnection(); // make the thread stop
	}

	return;
}


#ifdef _DEBUG
static void NetLog_Hash(CIcqProto *ppro, const char *pszIdent, const BYTE *pHash, int nHashLen)
{
	if (nHashLen == 0x14)
		ppro->NetLog_Server("Avatars: %s hash: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", pszIdent, pHash[0], pHash[1], pHash[2], pHash[3], pHash[4], pHash[5], pHash[6], pHash[7], pHash[8], pHash[9], pHash[10], pHash[11], pHash[12], pHash[13], pHash[14], pHash[15], pHash[16], pHash[17], pHash[18], pHash[19]);
	else if (nHashLen == 0x09)
		ppro->NetLog_Server("Avatars: %s hash: %02X%02X%02X%02X%02X%02X%02X%02X%02X", pszIdent, pHash[0], pHash[1], pHash[2], pHash[3], pHash[4], pHash[5], pHash[6], pHash[7], pHash[8]);
	else
		ppro->NetLog_Server("Avatars: %s hash: Unknown hash format.", pszIdent);
}
#endif


// handle Owner's avatar hash changes
void CIcqProto::handleAvatarOwnerHash(WORD wItemID, BYTE bFlags, BYTE *pData, BYTE nDataLen)
{
	if ((nDataLen >= 0x14) && m_bAvatarsEnabled)
	{
		switch (bFlags)
		{
		case 1: // our avatar is on the server
			{
				setSettingBlob(NULL, "AvatarHash", pData, 0x14); /// TODO: properly handle multiple avatar items (more formats)

				setUserInfo();
				// here we need to find a file, check its hash, if invalid get avatar from server
				TCHAR *file = GetOwnAvatarFileName();
				if (!file)
				{ // we have no avatar file, download from server
					TCHAR szFile[MAX_PATH * 2 + 4];
#ifdef _DEBUG
					NetLog_Server("We have no avatar, requesting from server.");
#endif
					GetAvatarFileName(0, NULL, szFile, MAX_PATH * 2);
					GetAvatarData(NULL, m_dwLocalUIN, NULL, pData, 0x14, szFile);
				}
				else
				{ // we know avatar filename
					BYTE *hash = calcMD5HashOfFile(file);

					if (!hash)
					{ // hash could not be calculated - probably missing file, get avatar from server
						TCHAR szFile[MAX_PATH * 2 + 4];
#ifdef _DEBUG
						NetLog_Server("We have no avatar, requesting from server.");
#endif
						GetAvatarFileName(0, NULL, szFile, MAX_PATH * 2);
						GetAvatarData(NULL, m_dwLocalUIN, NULL, pData, 0x14, szFile);
					} // check if we had set any avatar if yes set our, if not download from server
					else if (memcmp(hash, pData + 4, 0x10))
					{ // we have different avatar, sync that
						if (m_bSsiEnabled && getSettingByte(NULL, "ForceOurAvatar", 1))
						{ // we want our avatar, update hash
							DWORD dwPaFormat = DetectAvatarFormat(file);
							BYTE *pHash = (BYTE*)_alloca(0x14);

							NetLog_Server("Our avatar is different, setting our new hash.");

							pHash[0] = 0;
							pHash[1] = dwPaFormat == PA_FORMAT_XML ? AVATAR_HASH_FLASH : AVATAR_HASH_STATIC;
							pHash[2] = 1; // state of the hash
							pHash[3] = 0x10; // len of the hash
							memcpy(pHash + 4, hash, 0x10);
							updateServAvatarHash(pHash, 0x14);
						}
						else
						{ // get avatar from server
							TCHAR tszFile[MAX_PATH * 2 + 4];
#ifdef _DEBUG
							NetLog_Server("We have different avatar, requesting new from server.");
#endif
							GetAvatarFileName(0, NULL, tszFile, MAX_PATH * 2);
							GetAvatarData(NULL, m_dwLocalUIN, NULL, pData, 0x14, tszFile);
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

				TCHAR *file = GetOwnAvatarFileName();
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

					if ((hFile = CreateFile(file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL )) != INVALID_HANDLE_VALUE)
						if ((hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL)) != NULL)
							if ((ppMap = (BYTE*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0)) != NULL)
								cbFileSize = GetFileSize(hFile, NULL);

					if (cbFileSize != 0)
					{
						SetAvatarData(NULL, (WORD)(dwPaFormat == PA_FORMAT_XML ? AVATAR_HASH_FLASH : AVATAR_HASH_STATIC), ppMap, cbFileSize);
					}

					if (ppMap != NULL) UnmapViewOfFile(ppMap);
					if (hMap  != NULL) CloseHandle(hMap);
					if (hFile != NULL) CloseHandle(hFile);
					SAFE_FREE((void**)&hash);
				}
				else
				{
					BYTE *pHash = (BYTE*)_alloca(0x14);

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
void CIcqProto::handleAvatarContactHash(DWORD dwUIN, char *szUID, HANDLE hContact, BYTE *pHash, int nHashLen, WORD wOldStatus)
{
	int bJob = FALSE;
	BOOL avatarInfoPresent = FALSE;
	int avatarType = -1;
	BYTE *pAvatarHash = NULL;
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

		if (itemLen && memcmp(pHash + 4, emptyItem, itemLen > 0x10 ? 0x10 : itemLen))
		{ // Item types
			// 0000: AIM mini avatar
			// 0001: AIM/ICQ avatar ID/hash (len 5 or 16 bytes)
			// 0002: iChat online message
			// 0008: ICQ Flash avatar hash (16 bytes)
			// 0009: iTunes music store link
			// 000C: ICQ contact photo (16 bytes)
			// 000D: Last update time of online message
			// 000E: Status mood
			if (itemType == AVATAR_HASH_MINI && itemLen == 0x05 && avatarType == -1)
			{ // mini avatar
				pAvatarHash = pHash;
				cbAvatarHash = itemLen + 4;
				avatarType = itemType;
			}
			else if (itemType == AVATAR_HASH_STATIC && (itemLen == 0x05 || itemLen == 0x10) && (avatarType == -1 || avatarType == AVATAR_HASH_MINI))
			{ // normal avatar
				pAvatarHash = pHash;
				cbAvatarHash = itemLen + 4;
				avatarType = itemType;
			}
			else if (itemType == AVATAR_HASH_FLASH && itemLen == 0x10 && (avatarType == -1 || avatarType == AVATAR_HASH_MINI || avatarType == AVATAR_HASH_STATIC))
			{ // flash avatar
				pAvatarHash = pHash;
				cbAvatarHash = itemLen + 4;
				avatarType = itemType;
			}
			else if (itemType == AVATAR_HASH_PHOTO && itemLen == 0x10)
			{ // big avatar (ICQ 6+)
				pAvatarHash = pHash;
				cbAvatarHash = itemLen + 4;
				avatarType = itemType;
			}
		}
		else if ((itemLen == 0) && (itemType == AVATAR_HASH_MINI || itemType == AVATAR_HASH_STATIC || itemType == AVATAR_HASH_FLASH || itemType == AVATAR_HASH_PHOTO))
		{ // empty item - indicating that avatar of that type was removed
			avatarInfoPresent = TRUE;
		}

		pHash += itemLen + 4;
		nHashLen -= itemLen + 4;
	}

	if (avatarType != -1)
	{ // check settings, should we request avatar immediatelly?
		DBVARIANT dbv = {DBVT_DELETED};
		TCHAR tszAvatar[MAX_PATH * 2 +4];
		BYTE bAutoLoad = getSettingByte(NULL, "AvatarsAutoLoad", DEFAULT_LOAD_AVATARS);

		if ((avatarType == AVATAR_HASH_STATIC || avatarType == AVATAR_HASH_MINI) && cbAvatarHash == 0x09 && !memcmp(pAvatarHash + 4, hashEmptyAvatar + 4, 0x05))
		{ // empty avatar - unlink image, clear hash
			if (!getSetting(hContact, "AvatarHash", &dbv))
			{ // contact had avatar, clear hash, notify UI
#ifdef _DEBUG
				NetLog_Hash(this, "old", dbv.pbVal, dbv.cpbVal);
#endif
				ICQFreeVariant(&dbv);
				NetLog_Server("%s has removed Avatar.", strUID(dwUIN, szUID));

				deleteSetting(hContact, "AvatarHash");
				BroadcastAck(hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS, NULL, (LPARAM)NULL);
			}
#ifdef _DEBUG
			else
				NetLog_Server("%s has empty Avatar.", strUID(dwUIN, szUID));
#endif
			return;
		}

		if (getSetting(hContact, "AvatarHash", &dbv))
		{ // we did not find old avatar hash, i.e. get new avatar
			int avatarState = IsAvatarChanged(hContact, pAvatarHash, cbAvatarHash);

			// check saved hash and file, if equal only store hash
			if (!avatarState)
			{ // hashes are the same
				int dwPaFormat = getSettingByte(hContact, "AvatarType", PA_FORMAT_UNKNOWN);

				GetFullAvatarFileName(dwUIN, szUID, dwPaFormat, tszAvatar, MAX_PATH * 2);
				if (_taccess(tszAvatar, 0) == 0)
				{ // the file is there, link to contactphoto, save hash
					NetLog_Server("%s has published Avatar. Image was found in the cache.", strUID(dwUIN, szUID));
#ifdef _DEBUG
					NetLog_Hash(this, "new", pAvatarHash, cbAvatarHash);
#endif
					setSettingBlob(hContact, "AvatarHash", pAvatarHash, cbAvatarHash);
					BroadcastAck(hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS, NULL, (LPARAM)NULL);
				}
				else
				{ // the file was lost, request avatar again
					NetLog_Server("%s has published Avatar.", strUID(dwUIN, szUID));
#ifdef _DEBUG
					NetLog_Hash(this, "new", pAvatarHash, cbAvatarHash);
#endif
					bJob = TRUE;
				}
			}
			else
			{ // the hash is not the one we want, request avatar
				NetLog_Server("%s has published a new Avatar.", strUID(dwUIN, szUID));
#ifdef _DEBUG
				NetLog_Hash(this, "new", pAvatarHash, cbAvatarHash);
#endif
				bJob = TRUE;
			}
		}
		else
		{ // we found hash check if it changed or not
			if ((dbv.cpbVal != cbAvatarHash) || memcmp(dbv.pbVal, pAvatarHash, cbAvatarHash))
			{ // the hash is different, request new avatar
#ifdef _DEBUG
				NetLog_Hash(this, "old", dbv.pbVal, dbv.cpbVal);
#endif
				NetLog_Server("%s has changed Avatar.", strUID(dwUIN, szUID));
#ifdef _DEBUG
				NetLog_Hash(this, "new", pAvatarHash, cbAvatarHash);
#endif
				bJob = TRUE;
			}
			else
			{ // the hash was not changed, check if we have the correct file
				int avatarState = IsAvatarChanged(hContact, pAvatarHash, cbAvatarHash);

				// we should have file, check if the file really exists
				if (!avatarState)
				{
					int dwPaFormat = getSettingByte(hContact, "AvatarType", PA_FORMAT_UNKNOWN);
					if (dwPaFormat == PA_FORMAT_UNKNOWN)
					{ // we do not know the format, get avatar again
#ifdef _DEBUG
						NetLog_Hash(this, "current", dbv.pbVal, dbv.cpbVal);
#endif
						NetLog_Server("%s has Avatar. Image is missing.", strUID(dwUIN, szUID));
						bJob = 2;
					}
					else
					{
						GetFullAvatarFileName(dwUIN, szUID, dwPaFormat, tszAvatar, MAX_PATH * 2);
						if (_taccess(tszAvatar, 0) != 0)
						{ // the file was lost, get it again
#ifdef _DEBUG
							NetLog_Hash(this, "current", dbv.pbVal, dbv.cpbVal);
#endif
							NetLog_Server("%s has Avatar. Image is missing.", strUID(dwUIN, szUID));
							bJob = 2;
						}
#ifdef _DEBUG
						else
						{
							NetLog_Hash(this, "current", dbv.pbVal, dbv.cpbVal);

							NetLog_Server("%s has Avatar. Image was found in the cache.", strUID(dwUIN, szUID));
						}
#endif
					}
				}
				else
				{ // the hash is not the one we want, request avatar
#ifdef _DEBUG
					NetLog_Hash(this, "current", dbv.pbVal, dbv.cpbVal);
#endif
					NetLog_Server("%s has Avatar. Image was not retrieved yet.", strUID(dwUIN, szUID));
					bJob = 2;
				}
			}
			ICQFreeVariant(&dbv);
		}

		if (bJob)
		{
			if (bJob == TRUE)
			{ // Remove possible block - hash changed, try again.
				icq_lock l(m_avatarsMutex);

				avatars_request *ar = m_avatarsQueue;

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

			setSettingBlob(hContact, "AvatarHash", pAvatarHash, cbAvatarHash);

			BroadcastAck(hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS, NULL, (LPARAM)NULL);

			if (bAutoLoad)
			{ // auto-load is on, so request the avatar now, otherwise we are done
				GetAvatarFileName(dwUIN, szUID, tszAvatar, MAX_PATH * 2);
				GetAvatarData(hContact, dwUIN, szUID, pAvatarHash, cbAvatarHash, tszAvatar);
			} // avatar request sent or added to queue
		}
	}
	else if (avatarInfoPresent)
	{ // hash was not found, clear the hash
		DBVARIANT dbv = {DBVT_DELETED};

		if (!getSetting(hContact, "AvatarHash", &dbv))
		{ // contact had avatar, clear hash, notify UI
#ifdef _DEBUG
			NetLog_Hash(this, "old", dbv.pbVal, dbv.cpbVal);
#endif
			ICQFreeVariant(&dbv);
			NetLog_Server("%s has removed Avatar.", strUID(dwUIN, szUID));

			deleteSetting(hContact, "AvatarHash");
			BroadcastAck(hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS, NULL, (LPARAM)NULL);
		}
#ifdef _DEBUG
		else
			NetLog_Server("%s has no Avatar.", strUID(dwUIN, szUID));
#endif
	}
}


// request avatar data from server
int CIcqProto::GetAvatarData(HANDLE hContact, DWORD dwUin, const char *szUid, const BYTE *hash, unsigned int hashlen, const TCHAR *file)
{
	uid_str szUidData;
	char *pszUid = NULL;
	if (!dwUin && szUid) 
	{ // create a copy in local writable buffer
		strcpy(szUidData, szUid);
		pszUid = szUidData;
	}

	m_avatarsMutex->Enter();

	if (m_avatarsConnection && m_avatarsConnection->isReady()) // check if we are ready
	{	// check if requests for this user are not blocked
		DWORD dwNow = GetTickCount();
		avatars_request *ar = m_avatarsQueue;

		while (ar)
		{
			if (ar->hContact == hContact && ar->type == ART_BLOCK)
			{ // found a block item
				if (GetTickCount() > ar->timeOut)
				{ // remove timeouted block
					ar = ReleaseAvatarRequestInQueue(ar);
					continue;
				}
				m_avatarsMutex->Leave();
				NetLog_Server("Avatars: Requests for %s avatar are blocked.", strUID(dwUin, pszUid));
				return 0;
			}
			ar = ar->pNext;
		}

		avatars_server_connection *pConnection = m_avatarsConnection;

		pConnection->_Lock();
		m_avatarsMutex->Leave();

		DWORD dwCookie = pConnection->sendGetAvatarRequest(hContact, dwUin, pszUid, hash, hashlen, file);

		m_avatarsMutex->Enter();
		pConnection->_Release();

		if (dwCookie)
		{ // return now if the request was sent successfully
			m_avatarsMutex->Leave();
			return dwCookie;
		}
	}
	// we failed to send request, or avatar thread not ready

	// check if any request for this user is not already in the queue
	avatars_request *ar = m_avatarsQueue;

	while (ar)
	{
		if (ar->hContact == hContact)
		{ // we found it, return error
			if (ar->type == ART_BLOCK && GetTickCount() > ar->timeOut)
			{ // remove timeouted block
				ar = ReleaseAvatarRequestInQueue(ar);
				continue;
			}
			m_avatarsMutex->Leave();
			NetLog_Server("Avatars: Ignoring duplicate get %s avatar request.", strUID(dwUin, pszUid));

			// make sure avatar connection is in progress
			requestAvatarConnection();
			return 0;
		}
		ar = ar->pNext;
	}
	// add request to queue, processed after successful login
	ar = new avatars_request(ART_GET); // get avatar
	if (!ar)
	{ // out of memory, go away
		m_avatarsMutex->Leave();
		return 0;
	}
	ar->hContact = hContact;
	ar->dwUin = dwUin;
	if (!dwUin)
		strcpy(ar->szUid, szUid);
	ar->hash = (BYTE*)SAFE_MALLOC(hashlen);
	if (!ar->hash)
	{ // alloc failed
		m_avatarsMutex->Leave();
		SAFE_DELETE(&ar);
		return 0;
	}
	memcpy(ar->hash, hash, hashlen); // copy the data
	ar->hashlen = hashlen;
	ar->szFile = null_strdup(file); // duplicate the string
	ar->pNext = m_avatarsQueue;
	m_avatarsQueue = ar;
	m_avatarsMutex->Leave();

	NetLog_Server("Avatars: Request to get %s image added to queue.", strUID(dwUin, pszUid));

	// make sure avatar connection is in progress
	requestAvatarConnection();

	return -1; // we added to queue
}


// upload avatar data to server
int CIcqProto::SetAvatarData(HANDLE hContact, WORD wRef, const BYTE *data, unsigned int datalen)
{ 
	m_avatarsMutex->Enter();

	if (m_avatarsConnection && m_avatarsConnection->isReady()) // check if we are ready
	{
		avatars_server_connection *pConnection = m_avatarsConnection;

		pConnection->_Lock();
		m_avatarsMutex->Leave();

		DWORD dwCookie = pConnection->sendUploadAvatarRequest(hContact, wRef, data, datalen);

		m_avatarsMutex->Enter();
		pConnection->_Release();

		if (dwCookie)
		{ // return now if the request was sent successfully
			m_avatarsMutex->Leave();
			return dwCookie;
		}
	}
	// we failed to send request, or avatar thread not ready

	// check if any request for this user is not already in the queue
	avatars_request *ar = m_avatarsQueue;
	int bYet = 0;

	while (ar)
	{
		if (ar->hContact == hContact && ar->type == ART_UPLOAD)
		{ // we found it, return error
			m_avatarsMutex->Leave();
			NetLog_Server("Avatars: Ignoring duplicate upload avatar request.");

			// make sure avatar connection is in progress
			requestAvatarConnection();
			return 0;
		}
		ar = ar->pNext;
	}
	// add request to queue, processed after successful login
	ar = new avatars_request(ART_UPLOAD); // upload avatar
	if (!ar)
	{ // out of memory, go away
		m_avatarsMutex->Leave();
		return 0;
	}
	ar->hContact = hContact;
	ar->pData = (BYTE*)SAFE_MALLOC(datalen);
	if (!ar->pData)
	{ // alloc failed
		m_avatarsMutex->Leave();
		SAFE_DELETE(&ar);
		return 0;
	}
	memcpy(ar->pData, data, datalen); // copy the data
	ar->cbData = datalen;
	ar->wRef = wRef;
	ar->pNext = m_avatarsQueue;
	m_avatarsQueue = ar;
	m_avatarsMutex->Leave();

	NetLog_Server("Avatars: Request to upload image added to queue.");

	// make sure avatar connection is in progress
	requestAvatarConnection();

	return -1; // we added to queue
}


void CIcqProto::requestAvatarConnection()
{
	m_avatarsMutex->Enter();
	if (!m_avatarsConnectionPending && (!m_avatarsConnection || (!m_avatarsConnection->isPending() && !m_avatarsConnection->isReady())))
	{ // avatar connection is not pending, request new one
		m_avatarsConnectionPending = TRUE;
		m_avatarsMutex->Leave();

		icq_requestnewfamily(ICQ_AVATAR_FAMILY, &CIcqProto::StartAvatarThread);
	}
	else
		m_avatarsMutex->Leave();
}


void __cdecl CIcqProto::AvatarThread(avatars_server_connection *pInfo)
{
	NetLog_Server("%s thread started.", "Avatar");

	// Execute connection handler
	pInfo->connectionThread();

	{ // Remove connection reference
		icq_lock l(m_avatarsMutex);
		if (m_avatarsConnection == pInfo)
			m_avatarsConnection = NULL;
	}

	{ // Release connection handler
		icq_lock l(m_avatarsMutex);
		SAFE_DELETE(&pInfo);
	}

	NetLog_Server("%s thread ended.", "Avatar");
}


avatars_server_connection::avatars_server_connection(CIcqProto *ppro, HANDLE hConnection, char *pCookie, WORD wCookieLen): 
isLoggedIn(FALSE), stopThread(FALSE), isActive(FALSE)
{
	this->ppro = ppro;
	this->hConnection = hConnection;
	this->pCookie = pCookie;
	this->wCookieLen = wCookieLen;

	// Initialize packet sequence
	localSeqMutex = new icq_critical_section();
	wLocalSequence = generate_flap_sequence();

	// Initialize rates
	m_ratesMutex = new icq_critical_section();

	// Create connection thread
	ppro->ForkThread(( IcqThreadFunc )&CIcqProto::AvatarThread, this);
}


avatars_server_connection::~avatars_server_connection()
{
	SAFE_DELETE(&m_ratesMutex);
	SAFE_DELETE(&localSeqMutex);
}


int avatars_server_connection::NetLog_Server(const char *fmt,...)
{
	va_list va;
	char szText[1024 + 9];

	strcpy(szText, "Avatars: ");
	va_start(va, fmt);
	mir_vsnprintf(szText + 9, sizeof(szText) - 9, fmt, va);
	va_end(va);
	return CallService(MS_NETLIB_LOG,(WPARAM)ppro->m_hServerNetlibUser,(LPARAM)szText);
}


void avatars_server_connection::closeConnection()
{
	stopThread = TRUE;

	icq_lock l(localSeqMutex);
	if (hConnection)
		NetLib_SafeCloseHandle(&hConnection);
}


void avatars_server_connection::shutdownConnection()
{
	stopThread = TRUE;

	icq_lock l(localSeqMutex);
	if (hConnection)
		Netlib_Shutdown(hConnection);
}

DWORD avatars_server_connection::sendGetAvatarRequest(HANDLE hContact, DWORD dwUin, char *szUid, const BYTE *hash, unsigned int hashlen, const TCHAR *file)
{
	int i;
	DWORD dwNow = GetTickCount();

	ppro->m_avatarsMutex->Enter();

	for(i = 0; i < runCount;)
	{ // look for timeouted requests
		if (runTime[i] < dwNow)
		{ // found outdated, remove
			runContact[i] = runContact[runCount - 1];
			runTime[i] = runTime[runCount - 1];
			runCount--;
		}
		else
			i++;
	}

	for(i = 0; i < runCount; i++)
	{
		if (runContact[i] == hContact)
		{
			ppro->m_avatarsMutex->Leave();
			NetLog_Server("Ignoring duplicate get %s image request.", strUID(dwUin, szUid));

			return -1; // Success: request ignored
		}
	}

	if (runCount < 4)
	{ // 4 concurent requests at most
		int bSendNow = TRUE;

		{ // rate management
			icq_lock l(m_ratesMutex);
			WORD wGroup = m_rates->getGroupFromSNAC(ICQ_AVATAR_FAMILY, ICQ_AVATAR_GET_REQUEST);

			if (m_rates->getNextRateLevel(wGroup) < m_rates->getLimitLevel(wGroup, RML_ALERT))
			{ // we will be over quota if we send the request now, add to queue instead
				bSendNow = FALSE;
#ifdef _DEBUG
				NetLog_Server("Rates: Delay avatar request.");
#endif
			}
		}

		if (bSendNow)
		{
			runContact[runCount] = hContact;
			runTime[runCount] = GetTickCount() + 30000; // 30sec to complete request
			runCount++;

			ppro->m_avatarsMutex->Leave();

			int nUinLen = getUIDLen(dwUin, szUid);

			cookie_avatar *ack = (cookie_avatar*)SAFE_MALLOC(sizeof(cookie_avatar));
			if (!ack) return 0; // Failure: out of memory

			ack->dwUin = 1; //dwUin; // I should be damned for this - only to identify get request
			ack->hContact = hContact;
			ack->hash = (BYTE*)SAFE_MALLOC(hashlen);
			memcpy(ack->hash, hash, hashlen); // copy the data
			ack->hashlen = hashlen;
			ack->szFile = null_strdup(file); // duplicate the string

			DWORD dwCookie = ppro->AllocateCookie(CKT_AVATAR, ICQ_AVATAR_GET_REQUEST, hContact, ack);
			icq_packet packet;

			serverPacketInit(&packet, (WORD)(12 + nUinLen + hashlen));
			packFNACHeader(&packet, ICQ_AVATAR_FAMILY, ICQ_AVATAR_GET_REQUEST, 0, dwCookie);
			packUID(&packet, dwUin, szUid);
			packByte(&packet, 1); // unknown, probably type of request: 1 = get icon :)
			packBuffer(&packet, hash, (WORD)hashlen);

			if (sendServerPacket(&packet))
			{
				NetLog_Server("Request to get %s image sent.", strUID(dwUin, szUid));

				return dwCookie;
			}
			ppro->FreeCookie(dwCookie); // sending failed, free resources
			SAFE_FREE(&ack->szFile);
			SAFE_FREE((void**)&ack->hash);
			SAFE_FREE((void**)&ack);
		}
		else
			ppro->m_avatarsMutex->Leave();
	}
	else
		ppro->m_avatarsMutex->Leave();

	return 0; // Failure
}


DWORD avatars_server_connection::sendUploadAvatarRequest(HANDLE hContact, WORD wRef, const BYTE *data, unsigned int datalen)
{
	cookie_avatar *ack = (cookie_avatar*)SAFE_MALLOC(sizeof(cookie_avatar));
	if (!ack) return 0; // Failure: out of memory

	ack->hContact = hContact;

	DWORD dwCookie = ppro->AllocateCookie(CKT_AVATAR, ICQ_AVATAR_UPLOAD_REQUEST, 0, ack);
	icq_packet packet;

	serverPacketInit(&packet, (WORD)(14 + datalen));
	packFNACHeader(&packet, ICQ_AVATAR_FAMILY, ICQ_AVATAR_UPLOAD_REQUEST, 0, dwCookie);
	packWord(&packet, wRef); // unknown, probably reference
	packWord(&packet, (WORD)datalen);
	packBuffer(&packet, data, (WORD)datalen);

	if (sendServerPacket(&packet))
	{
		NetLog_Server("Upload image packet sent.");

		return dwCookie;
	}
	ppro->ReleaseCookie(dwCookie); // failed to send, free resources

	return 0;
}


void avatars_server_connection::checkRequestQueue()
{
#ifdef _DEBUG
	NetLog_Server("Checking request queue...");
#endif

	ppro->m_avatarsMutex->Enter();

	while (ppro->m_avatarsQueue && runCount < 3) // pick up an request and send it - happens immediatelly after login
	{ // do not fill queue to top, leave one place free
		avatars_request *pRequest = ppro->m_avatarsQueue;

		{ // rate management
			icq_lock l(m_ratesMutex);
			WORD wGroup = m_rates->getGroupFromSNAC(ICQ_AVATAR_FAMILY, (WORD)(pRequest->type == ART_UPLOAD ? ICQ_AVATAR_GET_REQUEST : ICQ_AVATAR_UPLOAD_REQUEST));

			if (m_rates->getNextRateLevel(wGroup) < m_rates->getLimitLevel(wGroup, RML_ALERT))
			{ // we are over rate, leave queue and wait
#ifdef _DEBUG
				NetLog_Server("Rates: Leaving avatar queue processing");
#endif
				break;
			}
		}

		if (pRequest->type == ART_BLOCK)
		{ // block contact processing
			avatars_request **ppRequest = &ppro->m_avatarsQueue;

			while (pRequest)
			{
				if (GetTickCount() > pRequest->timeOut)
				{ // expired contact block, remove
					*ppRequest = pRequest->pNext;
					SAFE_DELETE(&pRequest);
				}
				else // it is not time, move to next request
					ppRequest = &pRequest->pNext;

				pRequest = *ppRequest;
			}
			// end queue processing (only block requests follows)
			break;
		}
		else
			ppro->m_avatarsQueue = pRequest->pNext;

		ppro->m_avatarsMutex->Leave();

#ifdef _DEBUG
		NetLog_Server("Picked up the %s request from queue.", strUID(pRequest->dwUin, pRequest->szUid));
#endif
		switch (pRequest->type)
		{
		case ART_GET: // get avatar
			sendGetAvatarRequest(pRequest->hContact, pRequest->dwUin, pRequest->szUid, pRequest->hash, pRequest->hashlen, pRequest->szFile);
			break;

		case ART_UPLOAD: // set avatar
			sendUploadAvatarRequest(pRequest->hContact, pRequest->wRef, pRequest->pData, pRequest->cbData);
			break;
		}
		SAFE_DELETE(&pRequest);

		ppro->m_avatarsMutex->Enter();
	}

	ppro->m_avatarsMutex->Leave();
}


void avatars_server_connection::connectionThread()
{
	// This is the "infinite" loop that receives the packets from the ICQ avatar server
	NETLIBPACKETRECVER packetRecv = {0};
	DWORD wLastKeepAlive = 0; // we send keep-alive at most one per 30secs
	DWORD dwKeepAliveInterval = ppro->getSettingDword(NULL, "KeepAliveInterval", KEEPALIVE_INTERVAL);

	hPacketRecver = (HANDLE)CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)hConnection, 65536);
	packetRecv.cbSize = sizeof(packetRecv);
	packetRecv.dwTimeout = dwKeepAliveInterval < KEEPALIVE_INTERVAL ? dwKeepAliveInterval: KEEPALIVE_INTERVAL; // timeout - for stopThread to work
	while (!stopThread)
	{
		int recvResult = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM)hPacketRecver, (LPARAM)&packetRecv);

		if (recvResult == 0)
		{
			NetLog_Server("Clean closure of server socket");
			break;
		}

		if (recvResult == SOCKET_ERROR)
		{
			if (GetLastError() == ERROR_TIMEOUT)
			{  // timeout, check if we should be still running
				if (Miranda_Terminated())
				{ // we must stop here, cause due to a hack in netlib, we always get timeout, even if the connection is already dead
					stopThread = 1;
					continue;
				}
#ifdef _DEBUG
				else
					NetLog_Server("Thread is Idle.");
#endif
				if (GetTickCount() > wLastKeepAlive)
				{ // limit frequency (HACK: on some systems select() does not work well)
					if (!ppro->m_bGatewayMode && ppro->getSettingByte(NULL, "KeepAlive", DEFAULT_KEEPALIVE_ENABLED))
					{ // send keep-alive packet
						icq_packet packet;

						packet.wLen = 0;
						write_flap(&packet, ICQ_PING_CHAN);
						sendServerPacket(&packet);
					}
					wLastKeepAlive = GetTickCount() + dwKeepAliveInterval;
				}
				else
				{ // this is bad, the system does not handle select() properly
#ifdef _DEBUG
					NetLog_Server("Thread is Forcing Idle.");
#endif
					SleepEx(500, TRUE); // wait some time, can we do anything else ??
					if (Miranda_Terminated())
					{ 
						stopThread = 1;
						continue;
					}
				}
				// check if we got something to request
				checkRequestQueue();
				continue; 
			}
			if (!stopThread)
				NetLog_Server("Abortive closure of server socket, error: %d", GetLastError());
			else
				NetLog_Server("Connection closed.");
			break;
		}

		// Deal with the packet
		packetRecv.bytesUsed = handleServerPackets(packetRecv.buffer, packetRecv.bytesAvailable);

		if (isActive && (packetRecv.bytesAvailable == packetRecv.bytesUsed)) // no packets pending
		{ // process request queue
			checkRequestQueue();
		}
	}
	{ // release connection
		icq_lock l(localSeqMutex);
		NetLib_SafeCloseHandle(&hPacketRecver); // Close the packet receiver 
		NetLib_CloseConnection(&hConnection, FALSE); // Close the connection
	}

	{ // release rates
		icq_lock l(m_ratesMutex);
		SAFE_DELETE((void_struct**)&m_rates);
	}

	SAFE_FREE((void**)&pCookie);
}


int avatars_server_connection::sendServerPacket(icq_packet *pPacket)
{
	int lResult = 0;

	// This critsec makes sure that the sequence order doesn't get screwed up
	localSeqMutex->Enter();

	if (hConnection)
	{
		int nRetries;
		int nSendResult;

		// :IMPORTANT:
		// The FLAP sequence must be a WORD. When it reaches 0xFFFF it should wrap to
		// 0x0000, otherwise we'll get kicked by server.
		wLocalSequence++;

		// Pack sequence number
		pPacket->pData[2] = ((wLocalSequence & 0xff00) >> 8);
		pPacket->pData[3] = (wLocalSequence & 0x00ff);

		for (nRetries = 3; nRetries >= 0; nRetries--)
		{
			nSendResult = Netlib_Send(hConnection, (const char *)pPacket->pData, pPacket->wLen, 0);

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

			icq_lock l(m_ratesMutex);
			if (m_rates)
				m_rates->packetSent(pPacket);
		}
	}
	else
	{
		NetLog_Server("Error: Failed to send packet (no connection)");
	}

	localSeqMutex->Leave();

	SAFE_FREE((void**)&pPacket->pData);

	return lResult;
}


int avatars_server_connection::handleServerPackets(BYTE *buf, int buflen)
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
		NetLog_Server("Server FLAP: Channel %u, Seq %u, Length %u bytes", channel, sequence, datalen);
#endif

		switch (channel)
		{
		case ICQ_LOGIN_CHAN:
			handleLoginChannel(buf, datalen);
			break;

		case ICQ_DATA_CHAN:
			handleDataChannel(buf, datalen);
			break;

		default:
			NetLog_Server("Warning: Unhandled Server FLAP Channel: Channel %u, Seq %u, Length %u bytes", channel, sequence, datalen);
			break;
		}

		/* Increase pointers so we can check for more FLAPs */
		buf += datalen;
		buflen -= (datalen + 6);
		bytesUsed += (datalen + 6);
	}

	return bytesUsed;
}


void avatars_server_connection::handleLoginChannel(BYTE *buf, WORD datalen)
{
	icq_packet packet;

	if (*(DWORD*)buf == 0x1000000)
	{  // here check if we received SRV_HELLO
		wLocalSequence = generate_flap_sequence(); 

		serverCookieInit(&packet, (LPBYTE)pCookie, wCookieLen);
		sendServerPacket(&packet);

#ifdef _DEBUG
		NetLog_Server("Sent CLI_IDENT to %s", "avatar server");
#endif

		SAFE_FREE((void**)&pCookie);
		wCookieLen = 0;
	}
	else
	{
		NetLog_Server("Invalid Server response, Channel 1.");
	}
}


void avatars_server_connection::handleDataChannel(BYTE *buf, WORD datalen)
{
	snac_header snacHeader = {0};

	if (!unpackSnacHeader(&snacHeader, &buf, &datalen) || !snacHeader.bValid)
	{
		NetLog_Server("Error: Failed to parse SNAC header");
	}
	else
	{
#ifdef _DEBUG
		if (snacHeader.wFlags & 0x8000)
			NetLog_Server(" Received SNAC(x%02X,x%02X), version %u", snacHeader.wFamily, snacHeader.wSubtype, snacHeader.wVersion);
		else
			NetLog_Server(" Received SNAC(x%02X,x%02X)", snacHeader.wFamily, snacHeader.wSubtype);
#endif

		switch (snacHeader.wFamily)
		{

		case ICQ_SERVICE_FAMILY:
			handleServiceFam(buf, datalen, &snacHeader);
			break;

		case ICQ_AVATAR_FAMILY:
			handleAvatarFam(buf, datalen, &snacHeader);
			break;

		default:
			NetLog_Server("Ignoring SNAC(x%02X,x%02X) - FAMILYx%02X not implemented", snacHeader.wFamily, snacHeader.wSubtype, snacHeader.wFamily);
			break;
		}
	}
}


void avatars_server_connection::handleServiceFam(BYTE *pBuffer, WORD wBufferLength, snac_header *pSnacHeader)
{
	icq_packet packet;

	switch (pSnacHeader->wSubtype)
	{

	case ICQ_SERVER_READY:
#ifdef _DEBUG
		NetLog_Server("Server is ready and is requesting my Family versions");
		NetLog_Server("Sending my Families");
#endif

		// Miranda mimics the behaviour of Icq5
		serverPacketInit(&packet, 18);
		packFNACHeader(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_FAMILIES);
		packDWord(&packet, 0x00010004);
		packDWord(&packet, 0x00100001);
		sendServerPacket(&packet);
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
		sendServerPacket(&packet);
		break;

	case ICQ_SERVER_RATE_INFO:
#ifdef _DEBUG
		NetLog_Server("Server sent Rate Info");
#endif
		/* init rates management */
		m_rates = new rates(ppro, pBuffer, wBufferLength);
		/* ack rate levels */
#ifdef _DEBUG
		NetLog_Server("Sending Rate Info Ack");
#endif
		m_rates->initAckPacket(&packet);
		sendServerPacket(&packet);

		// send cli_ready
		serverPacketInit(&packet, 26);
		packFNACHeader(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_READY);
		packDWord(&packet, 0x00010004); // mimic ICQ 6
		packDWord(&packet, 0x0010164f);
		packDWord(&packet, 0x00100001);
		packDWord(&packet, 0x0010164f);
		sendServerPacket(&packet);

		isActive = TRUE; // we are ready to process requests
		isLoggedIn = TRUE;

		NetLog_Server(" *** Yeehah, login sequence complete");
		break;

	default:
		NetLog_Server("Warning: Ignoring SNAC(x%02x,x%02x) - Unknown SNAC (Flags: %u, Ref: %u)", ICQ_SERVICE_FAMILY, pSnacHeader->wSubtype, pSnacHeader->wFlags, pSnacHeader->dwRef);
		break;
	}
}


void avatars_server_connection::handleAvatarFam(BYTE *pBuffer, WORD wBufferLength, snac_header *pSnacHeader)
{
	switch (pSnacHeader->wSubtype) {

	case ICQ_AVATAR_GET_REPLY:  // received avatar data, store to file
		{ // handle new avatar, notify
			cookie_avatar *pCookieData;

			if (ppro->FindCookie(pSnacHeader->dwRef, NULL, (void**)&pCookieData))
			{
				PROTO_AVATAR_INFORMATIONT ai = {0};
				BYTE bResult;

				{ // remove from active request list
					icq_lock l(ppro->m_avatarsMutex);
					for(int i = 0; i < runCount; i++)
					{ // look for our record
						if (runContact[i] == pCookieData->hContact)
						{ // found, remove
							runContact[i] = runContact[runCount - 1];
							runTime[i] = runTime[runCount - 1];
							runCount--;
							break;
						}
					}
				}

				ai.cbSize = sizeof(PROTO_AVATAR_INFORMATION);
				ai.format = PA_FORMAT_JPEG; // this is for error only
				ai.hContact = pCookieData->hContact;
				lstrcpyn(ai.filename, pCookieData->szFile, SIZEOF(ai.filename)); // Avatar API does not support unicode :-(
				AddAvatarExt(PA_FORMAT_JPEG, ai.filename); 

				ppro->FreeCookie(pSnacHeader->dwRef);

				BYTE len;
				WORD datalen;

				unpackByte(&pBuffer, &len);
				if (wBufferLength < ((pCookieData->hashlen)<<1)+4+len)
				{
					NetLog_Server("Received invalid avatar reply.");

					ppro->BroadcastAck(pCookieData->hContact, ACKTYPE_AVATAR, ACKRESULT_FAILED, (HANDLE)&ai, 0);

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
					int aValid = 1;

					if (pCookieData->hashlen == 0x14 && pCookieData->hash[3] == 0x10 && ppro->getSettingByte(NULL, "StrictAvatarCheck", DEFAULT_AVATARS_CHECK))
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

						int dwPaFormat = DetectAvatarFormatBuffer((char*)pBuffer);
						TCHAR *tszImageFile = (TCHAR*)_alloca(sizeof(TCHAR)*(strlennull(pCookieData->szFile) + 6));

						_tcscpy(tszImageFile, pCookieData->szFile);
						AddAvatarExt(dwPaFormat, tszImageFile);

						ppro->setSettingByte(pCookieData->hContact, "AvatarType", (BYTE)dwPaFormat);
						ai.format = dwPaFormat; // set the format
						lstrcpyn(ai.filename, tszImageFile, SIZEOF(ai.filename));

						int out = _topen(tszImageFile, _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY, _S_IREAD | _S_IWRITE);
						if (out != -1) 
						{
							DBVARIANT dbv = {DBVT_DELETED};

							_write(out, pBuffer, datalen);
							_close(out);

							if (!pCookieData->hContact) // our avatar, set filename
							{
								TCHAR tmp[MAX_PATH * 2];
								CallService(MS_UTILS_PATHTORELATIVET, (WPARAM)tszImageFile, (LPARAM)tmp);
								ppro->setSettingStringT(NULL, "AvatarFile", tmp);
							}
							else
							{ // contact's avatar set hash
								if (!ppro->getSetting(pCookieData->hContact, "AvatarHash", &dbv))
								{
									if (ppro->setSettingBlob(pCookieData->hContact, "AvatarSaved", dbv.pbVal, dbv.cpbVal))
										NetLog_Server("Failed to set file hash.");

									ICQFreeVariant(&dbv);
								}
								else
								{
									NetLog_Server("Warning: DB error (no hash in DB).");
									// the hash was lost, try to fix that
									if (ppro->setSettingBlob(pCookieData->hContact, "AvatarSaved", pCookieData->hash, pCookieData->hashlen) ||
										ppro->setSettingBlob(pCookieData->hContact, "AvatarHash", pCookieData->hash, pCookieData->hashlen))
									{
										NetLog_Server("Failed to save avatar hash to DB");
									}
								}
							}

							ppro->BroadcastAck(pCookieData->hContact, ACKTYPE_AVATAR, ACKRESULT_SUCCESS, (HANDLE)&ai, 0);
						}  
					}
					else
					{ // avatar is broken
						NetLog_Server("Error: Avatar data does not match avatar hash, ignoring.");

						if (pCookieData->hContact)
						{
							avatars_request *ar = new avatars_request(ART_BLOCK);

							icq_lock l(ppro->m_avatarsMutex);

							if (ar)
							{
								avatars_request *last = ppro->m_avatarsQueue;

								ar->hContact = pCookieData->hContact;
								ar->timeOut = GetTickCount() + 14400000; // do not allow re-request four hours

								// add it to the end of queue, i.e. do not block other requests
								while (last && last->pNext) last = last->pNext;
								if (last)
									last->pNext = ar;
								else
									ppro->m_avatarsQueue = ar;
							}
						}
						ppro->BroadcastAck(pCookieData->hContact, ACKTYPE_AVATAR, ACKRESULT_FAILED, (HANDLE)&ai, 0);
					}
				}
				else
				{ // the avatar is empty
					NetLog_Server("Received empty avatar, nothing written (error 0x%x).", bResult);

					ppro->BroadcastAck(pCookieData->hContact, ACKTYPE_AVATAR, ACKRESULT_FAILED, (HANDLE)&ai, 0);
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
				if (ppro->FindCookie(pSnacHeader->dwRef, NULL, (void**)&pCookieData))
				{
					// here we store the local hash
					ppro->ReleaseCookie(pSnacHeader->dwRef);
				}
				else
				{
					NetLog_Server("Warning: Received unexpected Upload Avatar Reply SNAC(x10,x03).");
				}
			}
			else if (res)
			{
				NetLog_Server("Error uploading avatar to server, #%d", res);

				ppro->icq_LogMessage(LOG_WARNING, LPGEN("Error uploading avatar to server, server refused to accept the image."));
			}
			else
				NetLog_Server("Received invalid upload avatar ack.");

			break;
		}
	case ICQ_ERROR:
		{
			WORD wError;
			cookie_avatar *pCookieData;

			if (ppro->FindCookie(pSnacHeader->dwRef, NULL, (void**)&pCookieData))
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
				ppro->ReleaseCookie(pSnacHeader->dwRef);
			}

			if (wBufferLength >= 2)
				unpackWord(&pBuffer, &wError);
			else 
				wError = 0;

			ppro->LogFamilyError(ICQ_AVATAR_FAMILY, wError);
			break;
		}
	default:
		NetLog_Server("Warning: Ignoring SNAC(x%02x,x%02x) - Unknown SNAC (Flags: %u, Ref: %u)", ICQ_AVATAR_FAMILY, pSnacHeader->wSubtype, pSnacHeader->wFlags, pSnacHeader->dwRef);
		break;

	}
}
