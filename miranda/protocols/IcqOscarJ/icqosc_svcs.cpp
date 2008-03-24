// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
//
// Copyright © 2000-2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001-2002 Jon Keating, Richard Hughes
// Copyright © 2002-2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004-2008 Joe Kucera
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
//  High-level code for exported API services
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"

int CIcqProto::AddServerContact(WPARAM wParam, LPARAM lParam)
{
	DWORD dwUin;
	uid_str szUid;

	if (!m_bSsiEnabled) return 0;

	// Does this contact have a UID?
	if (!getContactUid((HANDLE)wParam, &dwUin, &szUid) && !getSettingWord((HANDLE)wParam, "ServerId", 0) && !getSettingWord((HANDLE)wParam, "SrvIgnoreId", 0))
	{
		// Read group from DB
		char *pszGroup = getContactCListGroup((HANDLE)wParam);

    servlistAddContact((HANDLE)wParam, pszGroup);
		SAFE_FREE((void**)&pszGroup);
	}
	return 0;
}

int CIcqProto::ChangeInfoEx(WPARAM wParam, LPARAM lParam)
{
	if (icqOnline() && wParam)
	{
		PBYTE buf = NULL;
		int buflen = 0;
		BYTE b;

		// userinfo
		ppackTLVWord(&buf, &buflen, (WORD)GetACP(), TLV_CODEPAGE, 0);

		if (wParam & CIXT_CONTACT)
		{ // contact information
			b = !getSettingByte(NULL, "PublishPrimaryEmail", 0);
			ppackTLVLNTSBytefromDB(&buf, &buflen, "e-mail", b, TLV_EMAIL);
			ppackTLVLNTSBytefromDB(&buf, &buflen, "e-mail0", 0, TLV_EMAIL);
			ppackTLVLNTSBytefromDB(&buf, &buflen, "e-mail1", 0, TLV_EMAIL);

			ppackTLVByte(&buf, &buflen, getSettingByte(NULL, "AllowSpam", 0), TLV_ALLOWSPAM, 1);

			ppackTLVLNTSfromDB(&buf, &buflen, "Phone", TLV_PHONE);
			ppackTLVLNTSfromDB(&buf, &buflen, "Fax", TLV_FAX);
			ppackTLVLNTSfromDB(&buf, &buflen, "Cellular", TLV_MOBILE);
			ppackTLVLNTSfromDB(&buf, &buflen, "CompanyPhone", TLV_WORKPHONE);
			ppackTLVLNTSfromDB(&buf, &buflen, "CompanyFax", TLV_WORKFAX);
		}

		if (wParam & CIXT_BASIC)
		{ // upload basic user info
			ppackTLVLNTSfromDB(&buf, &buflen, "Nick", TLV_NICKNAME);
			ppackTLVLNTSfromDB(&buf, &buflen, "FirstName", TLV_FIRSTNAME);
			ppackTLVLNTSfromDB(&buf, &buflen, "LastName", TLV_LASTNAME);
			ppackTLVLNTSfromDB(&buf, &buflen, "About", TLV_ABOUT);
		}

		if (wParam & CIXT_MORE)
		{
			ppackTLVWord(&buf, &buflen, getSettingWord(NULL, "Age", 0), TLV_AGE, 1);
			b = getSettingByte(NULL, "Gender", 0);
			ppackTLVByte(&buf, &buflen, (BYTE)(b ? (b == 'M' ? 2 : 1) : 0), TLV_GENDER, 1);
			ppackLEWord(&buf, &buflen, TLV_BIRTH);
			ppackLEWord(&buf, &buflen, 0x06);
			ppackLEWord(&buf, &buflen, getSettingWord(NULL, "BirthYear", 0));
			ppackLEWord(&buf, &buflen, (WORD)getSettingByte(NULL, "BirthMonth", 0));
			ppackLEWord(&buf, &buflen, (WORD)getSettingByte(NULL, "BirthDay", 0));

			ppackTLVWord(&buf, &buflen, (WORD)StringToListItemId("Language1", 0), TLV_LANGUAGE, 1);
			ppackTLVWord(&buf, &buflen, (WORD)StringToListItemId("Language2", 0), TLV_LANGUAGE, 1);
			ppackTLVWord(&buf, &buflen, (WORD)StringToListItemId("Language3", 0), TLV_LANGUAGE, 1);

			ppackTLVByte(&buf, &buflen, getSettingByte(NULL, "MaritalStatus", 0), TLV_MARITAL, 1);
		}

		if (wParam & CIXT_WORK)
		{
			ppackTLVLNTSfromDB(&buf, &buflen, "CompanyDepartment", TLV_DEPARTMENT);
			ppackTLVLNTSfromDB(&buf, &buflen, "CompanyPosition", TLV_POSITION);
			ppackTLVLNTSfromDB(&buf, &buflen, "Company", TLV_COMPANY);
			ppackTLVLNTSfromDB(&buf, &buflen, "CompanyStreet", TLV_WORKSTREET);
			ppackTLVLNTSfromDB(&buf, &buflen, "CompanyState", TLV_WORKSTATE);
			ppackTLVLNTSfromDB(&buf, &buflen, "CompanyCity", TLV_WORKCITY);
			ppackTLVLNTSfromDB(&buf, &buflen, "CompanyHomepage", TLV_WORKURL);
			ppackTLVLNTSfromDB(&buf, &buflen, "CompanyZIP", TLV_WORKZIPCODE);
			ppackTLVWord(&buf, &buflen, getSettingWord(NULL, "CompanyCountry", 0), TLV_WORKCOUNTRY, 1);
			ppackTLVWord(&buf, &buflen, getSettingWord(NULL, "CompanyOccupation", 0), TLV_OCUPATION, 1);
		}

		if (wParam & CIXT_LOCATION)
		{
			ppackTLVLNTSfromDB(&buf, &buflen, "City", TLV_CITY);
			ppackTLVLNTSfromDB(&buf, &buflen, "State", TLV_STATE);
			ppackTLVWord(&buf, &buflen, getSettingWord(NULL, "Country", 0), TLV_COUNTRY, 1);
			ppackTLVLNTSfromDB(&buf, &buflen, "OriginCity", TLV_ORGCITY);
			ppackTLVLNTSfromDB(&buf, &buflen, "OriginState", TLV_ORGSTATE);
			ppackTLVWord(&buf, &buflen, getSettingWord(NULL, "OriginCountry", 0), TLV_ORGCOUNTRY, 1);
			ppackTLVLNTSfromDB(&buf, &buflen, "Street", TLV_STREET);
			ppackTLVLNTSfromDB(&buf, &buflen, "ZIP", TLV_ZIPCODE);

			ppackTLVLNTSfromDB(&buf, &buflen, "Homepage", TLV_URL);

			ppackTLVByte(&buf, &buflen, getSettingByte(NULL, "Timezone", 0), TLV_TIMEZONE, 1);
		}

		if (wParam & CIXT_BACKGROUND)
		{
			WORD w;

			w = StringToListItemId("Interest0Cat", 0);
			ppackTLVWordLNTSfromDB(&buf, &buflen, w, "Interest0Text", TLV_INTERESTS);
			w = StringToListItemId("Interest1Cat", 0);
			ppackTLVWordLNTSfromDB(&buf, &buflen, w, "Interest1Text", TLV_INTERESTS);
			w = StringToListItemId("Interest2Cat", 0);
			ppackTLVWordLNTSfromDB(&buf, &buflen, w, "Interest2Text", TLV_INTERESTS);
			w = StringToListItemId("Interest3Cat", 0);
			ppackTLVWordLNTSfromDB(&buf, &buflen, w, "Interest3Text", TLV_INTERESTS);

			w = StringToListItemId("Past0", 0);
			ppackTLVWordLNTSfromDB(&buf, &buflen, w, "Past0Text", TLV_PASTINFO);
			w = StringToListItemId("Past1", 0);
			ppackTLVWordLNTSfromDB(&buf, &buflen, w, "Past1Text", TLV_PASTINFO);
			w = StringToListItemId("Past2", 0);
			ppackTLVWordLNTSfromDB(&buf, &buflen, w, "Past2Text", TLV_PASTINFO);

			w = StringToListItemId("Affiliation0", 0);
			ppackTLVWordLNTSfromDB(&buf, &buflen, w, "Affiliation0Text", TLV_AFFILATIONS);
			w = StringToListItemId("Affiliation1", 0);
			ppackTLVWordLNTSfromDB(&buf, &buflen, w, "Affiliation1Text", TLV_AFFILATIONS);
			w = StringToListItemId("Affiliation2", 0);
			ppackTLVWordLNTSfromDB(&buf, &buflen, w, "Affiliation2Text", TLV_AFFILATIONS);
		}

		return icq_changeUserDetailsServ(META_SET_FULLINFO_REQ, (char*)buf, (WORD)buflen);
	}

	return 0; // Failure
}

int CIcqProto::GetAvatarCaps(WPARAM wParam, LPARAM lParam)
{
	if (wParam == AF_MAXSIZE)
	{
		POINT *size = (POINT*)lParam;

		if (size)
		{
			if (getSettingByte(NULL, "AvatarsAllowBigger", DEFAULT_BIGGER_AVATARS))
			{ // experimental server limits
				size->x = 128;
				size->y = 128;
			}
			else
			{ // default limits (older)
				size->x = 64;
				size->y = 64;
			}

			return 0;
		}
	}
	else if (wParam == AF_PROPORTION)
	{
		return PIP_NONE;
	}
	else if (wParam == AF_FORMATSUPPORTED)
	{
		if (lParam == PA_FORMAT_JPEG || lParam == PA_FORMAT_GIF || lParam == PA_FORMAT_XML || lParam == PA_FORMAT_BMP)
			return 1;
		else
			return 0;
	}
	else if (wParam == AF_ENABLED)
	{
		if (m_bSsiEnabled && m_bAvatarsEnabled)
			return 1;
		else
			return 0;
	}
	else if (wParam == AF_DONTNEEDDELAYS)
	{
		return 0;
	}
	else if (wParam == AF_MAXFILESIZE)
	{ // server accepts images of 7168 bytees, not bigger
		return 7168;
	}
	else if (wParam == AF_DELAYAFTERFAIL)
	{ // do not request avatar again if server gave an error
		return 1 * 60 * 60 * 1000; // one hour
	}
	return 0;
}

int CIcqProto::GetAvatarInfo(WPARAM wParam, LPARAM lParam)
{
	PROTO_AVATAR_INFORMATION* pai = (PROTO_AVATAR_INFORMATION*)lParam;
	DWORD dwUIN;
	uid_str szUID;
	DBVARIANT dbv;
	int dwPaFormat;

	if (!m_bAvatarsEnabled) return GAIR_NOAVATAR;

	if (getSetting(pai->hContact, "AvatarHash", &dbv) || dbv.type != DBVT_BLOB || (dbv.cpbVal != 0x14 && dbv.cpbVal != 0x09))
		return GAIR_NOAVATAR; // we did not found avatar hash or hash invalid - no avatar available

	if (getContactUid(pai->hContact, &dwUIN, &szUID))
	{
		ICQFreeVariant(&dbv);

		return GAIR_NOAVATAR; // we do not support avatars for invalid contacts
	}

	dwPaFormat = getSettingByte(pai->hContact, "AvatarType", PA_FORMAT_UNKNOWN);
	if (dwPaFormat != PA_FORMAT_UNKNOWN)
	{ // we know the format, test file
		GetFullAvatarFileName(dwUIN, szUID, dwPaFormat, pai->filename, MAX_PATH);

		pai->format = dwPaFormat;

		if (!IsAvatarSaved(pai->hContact, dbv.pbVal, dbv.cpbVal))
		{ // hashes are the same
			if (access(pai->filename, 0) == 0)
			{
				ICQFreeVariant(&dbv);

				return GAIR_SUCCESS; // we have found the avatar file, whoala
			}
		}
	}

	if (IsAvatarSaved(pai->hContact, dbv.pbVal, dbv.cpbVal))
	{ // we didn't received the avatar before - this ensures we will not request avatar again and again
		if ((wParam & GAIF_FORCE) != 0 && pai->hContact != 0)
		{ // request avatar data
			GetAvatarFileName(dwUIN, szUID, pai->filename, MAX_PATH);
			GetAvatarData(pai->hContact, dwUIN, szUID, dbv.pbVal, dbv.cpbVal, pai->filename);
			ICQFreeVariant(&dbv);

			return GAIR_WAITFOR;
		}
	}
	ICQFreeVariant(&dbv);

	return GAIR_NOAVATAR;
}

int CIcqProto::GetMyAvatar(WPARAM wParam, LPARAM lParam)
{
	if (!m_bAvatarsEnabled) return -2;

	if (!wParam) return -3;

	char* file = loadMyAvatarFileName();
	if (file) strncpy((char*)wParam, file, (int)lParam);
	SAFE_FREE((void**)&file);
	if (!access((char*)wParam, 0)) return 0;
	return -1;
}

int CIcqProto::GetName(WPARAM wParam, LPARAM lParam)
{
	if ( lParam ) {
		strncpy(( char* )lParam, ICQTranslate(m_szModuleName), wParam);
		return 0; // Success
	}

	return 1; // Failure
}

int CIcqProto::GetStatus(WPARAM wParam, LPARAM lParam)
{
	return m_iStatus;
}

int CIcqProto::GrantAuthorization(WPARAM wParam, LPARAM lParam)
{
	if (m_iStatus != ID_STATUS_OFFLINE && m_iStatus != ID_STATUS_CONNECTING && wParam != 0)
	{
		DWORD dwUin;
		uid_str szUid;

		if (getContactUid((HANDLE)wParam, &dwUin, &szUid))
			return 0; // Invalid contact

		// send without reason, do we need any ?
		icq_sendGrantAuthServ(dwUin, szUid, NULL);
		// auth granted, remove contact menu item
		deleteSetting((HANDLE)wParam, "Grant");
	}

	return 0;
}

int CIcqProto::OnIdleChanged(WPARAM wParam, LPARAM lParam)
{
	int bIdle = (lParam&IDF_ISIDLE);
	int bPrivacy = (lParam&IDF_PRIVACY);

	if (bPrivacy) return 0;

	setSettingDword(NULL, "IdleTS", bIdle ? time(0) : 0);

	if (m_bTempVisListEnabled) // remove temporary visible users
		sendEntireListServ(ICQ_BOS_FAMILY, ICQ_CLI_REMOVETEMPVISIBLE, BUL_TEMPVISIBLE);

	icq_setidle(bIdle ? 1 : 0);

	return 0;
}

int CIcqProto::RevokeAuthorization(WPARAM wParam, LPARAM lParam)
{
	if (m_iStatus != ID_STATUS_OFFLINE && m_iStatus != ID_STATUS_CONNECTING && wParam != 0)
	{
		DWORD dwUin;
		uid_str szUid;

		if (getContactUid((HANDLE)wParam, &dwUin, &szUid))
			return 0; // Invalid contact

		if (MessageBoxUtf(NULL, LPGEN("Are you sure you want to revoke user's authorization (this will remove you from his/her list on some clients) ?"), LPGEN("Confirmation"), MB_ICONQUESTION | MB_YESNO) != IDYES)
			return 0;

		icq_sendRevokeAuthServ(dwUin, szUid);
	}

	return 0;
}

int CIcqProto::SearchByDetails(WPARAM wParam, LPARAM lParam)
{
	if (lParam && icqOnline())
	{
		PROTOSEARCHBYNAME *psbn=(PROTOSEARCHBYNAME*)lParam;


		if (psbn->pszNick || psbn->pszFirstName || psbn->pszLastName)
		{
			// Success
			return SearchByNames(psbn->pszNick, psbn->pszFirstName, psbn->pszLastName);
		}
	}

	return 0; // Failure
}

int CIcqProto::SendSms(WPARAM wParam, LPARAM lParam)
{
	if (icqOnline() && wParam && lParam)
		return icq_sendSMSServ((const char *)wParam, (const char *)lParam);

	return 0; // Failure
}

int CIcqProto::SendYouWereAdded(WPARAM wParam, LPARAM lParam)
{
	if (lParam && icqOnline())
	{
		CCSDATA* ccs = (CCSDATA*)lParam;
		if (ccs->hContact)
		{
			DWORD dwUin, dwMyUin;

			if (getContactUid(ccs->hContact, &dwUin, NULL))
				return 1; // Invalid contact

			dwMyUin = getContactUin(NULL);

			if (dwUin)
			{
				icq_sendYouWereAddedServ(dwUin, dwMyUin);
				return 0; // Success
			}
		}
	}

	return 1; // Failure
}

int CIcqProto::SetMyAvatar(WPARAM wParam, LPARAM lParam)
{
	char* szFile = (char*)lParam;
	int iRet = -1;

	if (!m_bAvatarsEnabled || !m_bSsiEnabled) return -2;

	if (szFile)
	{ // set file for avatar
		char szMyFile[MAX_PATH+1];
		int dwPaFormat = DetectAvatarFormat(szFile);
		BYTE *hash;
		HBITMAP avt;

		if (dwPaFormat != PA_FORMAT_XML)
		{ // if it should be image, check if it is valid
			avt = (HBITMAP)CallService(MS_UTILS_LOADBITMAP, 0, (WPARAM)szFile);
			if (!avt) return iRet;
			DeleteObject(avt);
		}
		GetFullAvatarFileName(0, NULL, dwPaFormat, szMyFile, MAX_PATH);
		// if not in our storage, copy
		if (strcmpnull(szFile, szMyFile) && !CopyFileA(szFile, szMyFile, FALSE))
		{
			NetLog_Server("Failed to copy our avatar to local storage.");
			return iRet;
		}

		hash = calcMD5Hash(szMyFile);
		if (hash)
		{
			BYTE* ihash = (BYTE*)_alloca(0x14);
			// upload hash to server
			ihash[0] = 0;    //unknown
			ihash[1] = dwPaFormat == PA_FORMAT_XML ? AVATAR_HASH_FLASH : AVATAR_HASH_STATIC; //hash type
			ihash[2] = 1;    //hash status
			ihash[3] = 0x10; //hash len
			memcpy(ihash+4, hash, 0x10);
			updateServAvatarHash(ihash, 0x14);

			if (setSettingBlob(NULL, "AvatarHash", ihash, 0x14))
			{
				NetLog_Server("Failed to save avatar hash.");
			}

			char tmp[MAX_PATH];
			CallService(MS_UTILS_PATHTORELATIVE, (WPARAM)szMyFile, (LPARAM)tmp);
			setSettingString(NULL, "AvatarFile", tmp);

			iRet = 0;

			SAFE_FREE((void**)&hash);
		}
	}
	else
	{ // delete user avatar
		deleteSetting(NULL, "AvatarFile");
		setSettingBlob(NULL, "AvatarHash", hashEmptyAvatar, 9);
		updateServAvatarHash(hashEmptyAvatar, 9); // set blank avatar
		iRet = 0;
	}

	return iRet;
}

int CIcqProto::SetNickName(WPARAM wParam, LPARAM lParam)
{
	if (icqOnline())
	{
		setSettingString(NULL, "Nick", (char*)lParam);

		return ChangeInfoEx(CIXT_BASIC, 0);
	}

	return 0; // Failure
}

int CIcqProto::SetPassword(WPARAM wParam, LPARAM lParam)
{
	char *pwd = (char*)lParam;
	int len = strlennull(pwd);

	if (len && len <= 8)
	{
		strcpy(m_szPassword, pwd);
		m_bRememberPwd = 1;
	}
	return 0;
}

// TODO: Adding needs some more work in general

HANDLE CIcqProto::AddToListByUIN(DWORD dwUin, DWORD dwFlags)
{
	int bAdded;
	HANDLE hContact = HContactFromUIN(dwUin, &bAdded);
	if (hContact)
	{
		if ((!dwFlags & PALF_TEMPORARY) && DBGetContactSettingByte(hContact, "CList", "NotOnList", 1))
		{
			DBDeleteContactSetting(hContact, "CList", "NotOnList");
			setContactHidden(hContact, 0);
		}

		return hContact; // Success
	}

	return NULL; // Failure
}

HANDLE CIcqProto::AddToListByUID(char *szUID, DWORD dwFlags)
{
	int bAdded;
	HANDLE hContact = HContactFromUID(0, szUID, &bAdded);
	if (hContact)
	{
		if ((!dwFlags & PALF_TEMPORARY) && DBGetContactSettingByte(hContact, "CList", "NotOnList", 1))
		{
			DBDeleteContactSetting(hContact, "CList", "NotOnList");
			setContactHidden(hContact, 0);
		}

		return hContact; // Success
	}

	return NULL; // Failure
}

/////////////////////////////////////////////////////////////////////////////////////////

void CIcqProto::ICQAddRecvEvent(HANDLE hContact, WORD wType, PROTORECVEVENT* pre, DWORD cbBlob, PBYTE pBlob, DWORD flags)
{
	if (pre->flags & PREF_CREATEREAD) 
		flags |= DBEF_READ;

	if (hContact && DBGetContactSettingByte(hContact, "CList", "Hidden", 0))
	{
		DWORD dwUin;
		uid_str szUid;

		setContactHidden(hContact, 0);
		// if the contact was hidden, add to client-list if not in server-list authed
		if (!getSettingWord(hContact, "ServerId", 0) || getSettingByte(hContact, "Auth", 0))
		{
			getContactUid(hContact, &dwUin, &szUid);
			icq_sendNewContact(dwUin, szUid);
		}
	}

	AddEvent(hContact, wType, pre->timestamp, flags, cbBlob, pBlob);
}
