/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-08  George Hazan
Copyright ( C ) 2007     Maxim Mluhov

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or ( at your option ) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

File name      : $URL: https://miranda.svn.sourceforge.net/svnroot/miranda/trunk/miranda/protocols/JabberG/jabber_svc.cpp $
Revision       : $Revision: 7044 $
Last change on : $Date: 2008-01-04 22:42:50 +0300 (Пт, 04 янв 2008) $
Last change by : $Author: m_mluhov $

*/

#include "jabber.h"

#include <fcntl.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "resource.h"
#include "jabber_list.h"
#include "jabber_iq.h"
#include "jabber_caps.h"
#include "m_file.h"
#include "m_addcontact.h"
#include "jabber_disco.h"
#include "sdk/m_proto_listeningto.h"

/////////////////////////////////////////////////////////////////////////////////////////
// OnContactDeleted - processes a contact deletion

int CJabberProto::OnContactDeleted( WPARAM wParam, LPARAM lParam )
{
	if( !jabberOnline )	// should never happen
		return 0;

	char* szProto = ( char* )JCallService( MS_PROTO_GETCONTACTBASEPROTO, wParam, 0 );
	if ( szProto==NULL || strcmp( szProto, m_szProtoName ))
		return 0;

	DBVARIANT dbv;
	if ( !JGetStringT(( HANDLE ) wParam, JGetByte( (HANDLE ) wParam, "ChatRoom", 0 )?(char*)"ChatRoomID":(char*)"jid", &dbv )) {
		if ( JabberListExist( LIST_ROSTER, dbv.ptszVal )) {
			if ( !_tcschr( dbv.ptszVal, _T( '@' )))
			{
				XmlNodeIq iq( "set", JabberSerialNext(), dbv.ptszVal );
				XmlNode* query = iq.addQuery( JABBER_FEAT_REGISTER );
				query->addChild( "remove" );
				jabberThreadInfo->send( iq );
			}
			{
				// Remove from roster, server also handles the presence unsubscription process.
				XmlNodeIq iq( "set" ); iq.addAttrID( JabberSerialNext());
				XmlNode* query = iq.addQuery( JABBER_FEAT_IQ_ROSTER );
				XmlNode* item = query->addChild( "item" ); item->addAttr( "jid", dbv.ptszVal ); item->addAttr( "subscription", "remove" );
				jabberThreadInfo->send( iq );
			}
		}

		JFreeVariant( &dbv );
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberDbSettingChanged - process database changes

static TCHAR* sttSettingToTchar( DBCONTACTWRITESETTING* cws )
{
	switch( cws->value.type ) {
	case DBVT_ASCIIZ:
		#if defined( _UNICODE )
			return mir_a2u( cws->value.pszVal );
		#else
			return mir_strdup( cws->value.pszVal );
		#endif

	case DBVT_UTF8:
		#if defined( _UNICODE )
		{	TCHAR* result;
			mir_utf8decode( NEWSTR_ALLOCA(cws->value.pszVal), &result );
			return result;
		}
		#else
			return mir_strdup( mir_utf8decode( NEWSTR_ALLOCA(cws->value.pszVal), NULL ));
		#endif
	}
	return NULL;
}

void __cdecl CJabberProto::OnRenameGroup( DBCONTACTWRITESETTING* cws, HANDLE hContact )
{
	DBVARIANT jid, dbv;
	if ( JGetStringT( hContact, "jid", &jid ))
		return;

	JABBER_LIST_ITEM* item = JabberListGetItemPtr( LIST_ROSTER, jid.ptszVal );
	JFreeVariant( &jid );
	if ( item == NULL )
		return;

	TCHAR* nick;
	if ( !DBGetContactSettingTString( hContact, "CList", "MyHandle", &dbv )) {
		nick = mir_tstrdup( dbv.ptszVal );
		JFreeVariant( &dbv );
	}
	else if ( !JGetStringT( hContact, "Nick", &dbv )) {
		nick = mir_tstrdup( dbv.ptszVal );
		JFreeVariant( &dbv );
	}
	else nick = JabberNickFromJID( item->jid );
	if ( nick == NULL )
		return;

	if ( cws->value.type == DBVT_DELETED ) {
		if ( item->group != NULL ) {
			JabberLog( "Group set to nothing" );
			JabberAddContactToRoster( item->jid, nick, NULL, item->subscription );
		}
	}
	else {
		TCHAR* p = sttSettingToTchar( cws );
		if ( cws->value.pszVal != NULL && lstrcmp( p, item->group )) {
			JabberLog( "Group set to " TCHAR_STR_PARAM, p );
			if ( p )
				JabberAddContactToRoster( item->jid, nick, p, item->subscription );
		}
		mir_free( p );
	}
	mir_free( nick );
}

void __cdecl CJabberProto::OnRenameContact( DBCONTACTWRITESETTING* cws, HANDLE hContact )
{
	DBVARIANT jid;
	if ( JGetStringT( hContact, "jid", &jid ))
		return;

	JABBER_LIST_ITEM* item = JabberListGetItemPtr( LIST_ROSTER, jid.ptszVal );
	JFreeVariant( &jid );
	if ( item == NULL )
		return;

	if ( cws->value.type == DBVT_DELETED ) {
		TCHAR* nick = ( TCHAR* )JCallService( MS_CLIST_GETCONTACTDISPLAYNAME, ( WPARAM )hContact, GCDNF_NOMYHANDLE | GCDNF_TCHAR );
		JabberAddContactToRoster( item->jid, nick, item->group, item->subscription );
		mir_free(nick);
		return;
	}

	TCHAR* newNick = sttSettingToTchar( cws );
	if ( newNick ) {
		if ( lstrcmp( item->nick, newNick )) {
			JabberLog( "Renaming contact " TCHAR_STR_PARAM ": " TCHAR_STR_PARAM " -> " TCHAR_STR_PARAM, item->jid, item->nick, newNick );
			JabberAddContactToRoster( item->jid, newNick, item->group, item->subscription );
		}
		mir_free( newNick );
}	}

void __cdecl CJabberProto::OnAddContactForever( DBCONTACTWRITESETTING* cws, HANDLE hContact )
{
	if ( cws->value.type != DBVT_DELETED && !( cws->value.type==DBVT_BYTE && cws->value.bVal==0 ))
		return;

	DBVARIANT jid, dbv;
	if ( JGetStringT( hContact, "jid", &jid ))
		return;

	TCHAR *nick;
	JabberLog( "Add " TCHAR_STR_PARAM " permanently to list", jid.pszVal );
	if ( !DBGetContactSettingTString( hContact, "CList", "MyHandle", &dbv )) {
		nick = mir_tstrdup( dbv.ptszVal );
		JFreeVariant( &dbv );
	}
	else if ( !JGetStringT( hContact, "Nick", &dbv )) {
		nick = mir_tstrdup( dbv.ptszVal );
		JFreeVariant( &dbv );
	}
	else nick = JabberNickFromJID( jid.ptszVal );
	if ( nick == NULL ) {
		JFreeVariant( &jid );
		return;
	}

	JABBER_LIST_ITEM* item = JabberListGetItemPtr( LIST_ROSTER, jid.ptszVal );
	JABBER_SUBSCRIPTION subscription = ( item == NULL ) ? SUB_NONE : item->subscription;

	if ( !DBGetContactSettingTString( hContact, "CList", "Group", &dbv )) {
		JabberAddContactToRoster( jid.ptszVal, nick, dbv.ptszVal, subscription );
		JFreeVariant( &dbv );
	}
	else JabberAddContactToRoster( jid.ptszVal, NULL, NULL, subscription );

	XmlNode presence( "presence" ); presence.addAttr( "to", jid.ptszVal ); presence.addAttr( "type", "subscribe" );
	jabberThreadInfo->send( presence );

	JabberSendGetVcard( jid.ptszVal );

	mir_free( nick );
	DBDeleteContactSetting( hContact, "CList", "Hidden" );
	JFreeVariant( &jid );
}

int __cdecl CJabberProto::OnDbSettingChanged( WPARAM wParam, LPARAM lParam )
{
	HANDLE hContact = ( HANDLE ) wParam;
	if ( hContact == NULL || !jabberConnected )
		return 0;

	DBCONTACTWRITESETTING* cws = ( DBCONTACTWRITESETTING* )lParam;
	if ( strcmp( cws->szModule, "CList" ))
		return 0;

	char* szProto = ( char* )JCallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM ) hContact, 0 );
	if ( szProto == NULL || strcmp( szProto, m_szProtoName ))
		return 0;

	if ( !strcmp( cws->szSetting, "Group" ))
		OnRenameGroup( cws, hContact );
	else if ( !strcmp( cws->szSetting, "MyHandle" ))
		OnRenameContact( cws, hContact );
	else if ( !strcmp( cws->szSetting, "NotOnList" ))
		OnAddContactForever( cws, hContact );

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// OnIdleChanged - tracks idle start time for XEP-0012 support

int CJabberProto::OnIdleChanged( WPARAM wParam, LPARAM lParam )
{
	// don't report idle time, if user disabled
	if (lParam & IDF_PRIVACY) {
		jabberIdleStartTime = 0;
		return 0;
	}

	BOOL bIdle = lParam & IDF_ISIDLE;

	// second call, ignore it
	if (bIdle && jabberIdleStartTime)
		return 0;

	jabberIdleStartTime = bIdle ? time( 0 ) : 0;
	return 0;
}
