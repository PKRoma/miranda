/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2006-2008 Boris Krasnovskiy.
Copyright (c) 2003-2005 George Hazan.
Copyright (c) 2002-2003 Richard Hughes (original version).

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "msn_global.h"

HANDLE  MSN_HContactFromEmail( const char* msnEmail, const char* msnNick, bool addIfNeeded, bool temporary )
{
	HANDLE hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
	while ( hContact != NULL )
	{
		if ( MSN_IsMyContact( hContact )) {
			char tEmail[ MSN_MAX_EMAIL_LEN ];
			if ( !MSN_GetStaticString( "e-mail", hContact, tEmail, sizeof( tEmail )))
				if ( !_stricmp( msnEmail, tEmail ))
					return hContact;
		}

		hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM )hContact, 0 );
	}

	if ( addIfNeeded )
	{
		hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_ADD, 0, 0 );
		MSN_CallService( MS_PROTO_ADDTOCONTACT, ( WPARAM )hContact, ( LPARAM )msnProtocolName );
		MSN_SetString( hContact, "e-mail", msnEmail );
		MSN_SetStringUtf( hContact, "Nick", ( char* )msnNick );
		if ( temporary )
			DBWriteContactSettingByte( hContact, "CList", "NotOnList", 1 );

		return hContact;
	}
	return NULL;
}


void MSN_SetContactDb(HANDLE hContact, const char *szEmail)
{
	int listId = Lists_GetMask( szEmail );
	TCHAR* szNonIm = TranslateT("Non IM Contacts");
	if (listId & LIST_FL)
	{
		if (DBGetContactSettingByte( hContact, "CList", "NotOnList", 0 ) == 1)
		{
			DBDeleteContactSetting( hContact, "CList", "NotOnList" );
			DBDeleteContactSetting( hContact, "CList", "Hidden" );
		}

		if (listId & (LIST_BL | LIST_AL)) 
		{
			WORD tApparentMode = MSN_GetWord( hContact, "ApparentMode", 0 );
			if (( listId & LIST_BL ) && tApparentMode == 0 )
				MSN_SetWord( hContact, "ApparentMode", ID_STATUS_OFFLINE );
			else if (( listId & LIST_AL ) && tApparentMode != 0 )
				DBDeleteContactSetting( hContact, msnProtocolName, "ApparentMode" );
		}

		int netId = Lists_GetNetId(szEmail);
		if (netId == NETID_EMAIL)
		{
			DBWriteContactSettingTString( hContact, "CList", "Group", szNonIm);
			MSN_SetWord( hContact, "Status", ID_STATUS_ONLINE );
			MSN_SetString( hContact, "MirVer", "E-Mail Only" );
		}
		else
		{
			DBVARIANT dbv;
			if (DBGetContactSettingTString( hContact, "CList", "Group", &dbv ) == 0 && 
				_tcscmp(dbv.ptszVal, szNonIm) == 0)
			{
				DBDeleteContactSetting(hContact, "CList", "Group" );
				MSN_FreeVariant( &dbv );
			}
		}

		if (netId == NETID_MOB)
		{
			MSN_SetWord( hContact, "Status", ID_STATUS_ONTHEPHONE );
			MSN_SetString( hContact, "MirVer", "SMS" );
		}
	}
	else
	{
		DBWriteContactSettingByte( hContact, "CList", "NotOnList", 1 );
		DBWriteContactSettingByte( hContact, "CList", "Hidden", 1 );
		DBWriteContactSettingTString( hContact, "CList", "Group", szNonIm);
	}
}


static void AddDelUserContList(const char* email, const int list, const int netId, const bool del)
{
	char buf[512];
	size_t sz;

	const char* dom = strchr(email, '@');
	if (dom == NULL)
	{
		sz = mir_snprintf(buf, sizeof(buf),
			"<ml><t><c n=\"%s\" l=\"%d\"/></t></ml>",
			email, list);
	}
	else
	{
		*(char*)dom = 0;
		sz = mir_snprintf(buf, sizeof(buf),
			"<ml><d n=\"%s\"><c n=\"%s\" l=\"%d\" t=\"%d\"/></d></ml>",
			dom+1, email, list, netId);
		*(char*)dom = '@';
	}
	msnNsThread->sendPacket(del ? "RML" : "ADL", "%d\r\n%s", sz, buf);

	if (del)
		Lists_Remove(list, email);
	else
		Lists_Add(list, netId, email);
}


/////////////////////////////////////////////////////////////////////////////////////////
// MSN_AddUser - adds a e-mail address to one of the MSN server lists

bool MSN_AddUser( HANDLE hContact, const char* email, int netId, int flags )
{
	bool needRemove = (flags & LIST_REMOVE) != 0;
	flags &= 0xFF;

	bool res = false;
	if (flags == LIST_FL) 
	{
		if (needRemove) 
		{
			if (!Lists_IsInList(flags, email)) return true;

			if (hContact == NULL)
			{
				hContact = MSN_HContactFromEmail( email, NULL, false, false );
				if ( hContact == NULL ) return false;
			}

			char id[ MSN_GUID_LEN ];
			if ( !MSN_GetStaticString( "ID", hContact, id, sizeof( id ))) 
			{
				res = MSN_ABAddDelContactGroup(id , NULL, "ABContactDelete");
				if (res) AddDelUserContList(email, flags, Lists_GetNetId(email), true);
			}
		}
		else 
		{
			if (Lists_IsInList(flags, email) && Lists_GetNetId(email) != NETID_EMAIL)
				return true;

			DBVARIANT dbv = {0};
			if ( !strcmp( email, MyOptions.szEmail ))
				DBGetContactSettingStringUtf( NULL, msnProtocolName, "Nick", &dbv );

			unsigned res1 = MSN_ABContactAdd(email, dbv.pszVal, netId, false);
			if (netId == NETID_MSN && res1 == 2)
			{
				netId = NETID_LCS;
				res = MSN_ABContactAdd(email, dbv.pszVal, netId, false) == 0;
			}
			else if (netId == NETID_MSN && res1 == 3)
			{
				char szContactID[100];
				if (MSN_GetStaticString("ID", hContact, szContactID, sizeof(szContactID)) == 0)
				{
					MSN_ABUpdateProperty(szContactID, "isMessengerUser", "1");
					res = true;
				}
			}

			else
				res = (res1 == 0);

			if (res)
				AddDelUserContList(email, flags, netId, false);
			else
			{
				if (netId == 1 && strstr(email, "@yahoo.com") != 0)
					MSN_FindYahooUser(email);
			}
			MSN_FreeVariant( &dbv );
		}
	}
	else 
	{
		if (needRemove != Lists_IsInList(flags, email))
			return true;

		res = MSN_SharingAddDelMember(email, flags, needRemove ? "DeleteMember" : "AddMember");
		if (res) AddDelUserContList(email, flags, Lists_GetNetId(email), needRemove);
		if ((flags & LIST_BL) && !needRemove)
		{
			if (hContact == NULL)
				hContact = MSN_HContactFromEmail( email, NULL, false, false );

			ThreadData* thread =  MSN_GetThreadByContact(hContact, SERVER_SWITCHBOARD);
			thread->sendPacket( "OUT", NULL );
		}
	}
	return res;
}


void MSN_FindYahooUser(const char* email)
{
	const char* dom = strchr(email, '@');
	if (dom)
	{
		char buf[512];
		size_t sz;

		*(char*)dom = '\0';
		sz = mir_snprintf(buf, sizeof(buf), "<ml><d n=\"%s\"><c n=\"%s\"/></d></ml>", dom+1, email);
		*(char*)dom = '@';
		msnNsThread->sendPacket("FQY", "%d\r\n%s", sz, buf);
	}
}

bool MSN_RefreshContactList(void)
{
	Lists_Wipe();
	if (!MSN_SharingFindMembership()) return false;
	if (!MSN_ABGetFull()) return false;
	MSN_CleanupLists();

	msnLoggedIn = true;

	MSN_CreateContList();
	MSN_StoreGetProfile();
	return true;
}
