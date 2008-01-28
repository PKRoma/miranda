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

#include <m_addcontact.h>
#include <m_file.h>
#include <m_genmenu.h>
#include <m_icolib.h>

#include "jabber_list.h"
#include "jabber_iq.h"
#include "jabber_caps.h"
#include "jabber_disco.h"

#include "sdk/m_proto_listeningto.h"
#include "sdk/m_modernopt.h"

#include "resource.h"

#pragma warning(disable:4355)

static int compareTransports( const TCHAR* p1, const TCHAR* p2 )
{	return _tcsicmp( p1, p2 );
}

static int compareListItems( const JABBER_LIST_ITEM* p1, const JABBER_LIST_ITEM* p2 )
{
	if ( p1->list != p2->list )
		return p1->list - p2->list;

	// for bookmarks, temporary contacts & groupchat members
	// resource must be used in the comparison
	if (( p1->list == LIST_ROSTER && ( p1->bUseResource == TRUE || p2->bUseResource == TRUE ))
		|| ( p1->list == LIST_BOOKMARK ) || ( p1->list == LIST_VCARD_TEMP ))
		return lstrcmpi( p1->jid, p2->jid );

	TCHAR szp1[ JABBER_MAX_JID_LEN ], szp2[ JABBER_MAX_JID_LEN ];
	JabberStripJid( p1->jid, szp1, SIZEOF( szp1 ));
	JabberStripJid( p2->jid, szp2, SIZEOF( szp2 ));
	return lstrcmpi( szp1, szp2 );
}

CJabberProto::CJabberProto( const char* aProtoName ) :
	jabberTransports( 50, compareTransports ),
	roster( 50, compareListItems ),
	m_iqManager( this ),
	m_adhocManager( this ),
	m_clientCapsManager( this ),
	m_privacyListManager( this )
{
	InitializeCriticalSection( &modeMsgMutex );

	InitializeCriticalSection( &csLists );

	szProtoName = mir_strdup( aProtoName );
	szModuleName = mir_strdup( szProtoName );
	_strlwr( szModuleName );
	szModuleName[0] = toupper( szModuleName[0] );
	JabberLog( "Setting protocol/module name to '%s/%s'", szProtoName, szModuleName );

	// Protocol services and events...
	heventNudge = JCreateHookableEvent( JE_NUDGE );
	heventRawXMLIn = JCreateHookableEvent( JE_RAWXMLIN );
	heventRawXMLOut = JCreateHookableEvent( JE_RAWXMLOUT );
	heventXStatusIconChanged = JCreateHookableEvent( JE_CUSTOMSTATUS_EXTRAICON_CHANGED );
	heventXStatusChanged = JCreateHookableEvent( JE_CUSTOMSTATUS_CHANGED );

	JCreateService( PS_GETNAME, &CJabberProto::JabberGetName );
	JCreateService( PS_GETAVATARINFO, &CJabberProto::JabberGetAvatarInfo );
	JCreateService( PS_GETSTATUS, &CJabberProto::JabberGetStatus );
	JCreateService( PS_SET_LISTENINGTO, &CJabberProto::JabberSetListeningTo );

	JCreateService( JS_GETCUSTOMSTATUSICON, &CJabberProto::JabberGetXStatusIcon );
	JCreateService( JS_GETXSTATUS, &CJabberProto::JabberGetXStatus );
	JCreateService( JS_SETXSTATUS, &CJabberProto::JabberSetXStatus );

	JCreateService( JS_SENDXML, &CJabberProto::ServiceSendXML );
	JCreateService( PS_GETMYAVATAR, &CJabberProto::JabberGetAvatar );
	JCreateService( PS_GETAVATARCAPS, &CJabberProto::JabberGetAvatarCaps );
	JCreateService( PS_SETMYAVATAR, &CJabberProto::JabberSetAvatar );

	JCreateService( JS_GETADVANCEDSTATUSICON, &CJabberProto::JGetAdvancedStatusIcon );
	JCreateService( JS_DB_GETEVENTTEXT_CHATSTATES, &CJabberProto::JabberGetEventTextChatStates );

	// XEP-0224 support (Attention/Nudge)
	JCreateService( JS_SEND_NUDGE, &CJabberProto::JabberSendNudge );

	// service to get from protocol chat buddy info
	JCreateService( MS_GC_PROTO_GETTOOLTIPTEXT, &CJabberProto::JabberGCGetToolTipText );

	// XMPP URI parser service for "File Association Manager" plugin
	JCreateService( JS_PARSE_XMPP_URI, &CJabberProto::JabberServiceParseXmppURI );

	JHookEvent( ME_CLIST_PREBUILDSTATUSMENU, &CJabberProto::OnBuildPrivacyMenu );
	JHookEvent( ME_DB_CONTACT_DELETED, &CJabberProto::OnContactDeleted );
	JHookEvent( ME_DB_CONTACT_SETTINGCHANGED, &CJabberProto::OnDbSettingChanged );
	JHookEvent( ME_IDLE_CHANGED, &CJabberProto::OnIdleChanged );
	JHookEvent( ME_CLIST_PREBUILDCONTACTMENU, &CJabberProto::OnPrebuildContactMenu );
	JHookEvent( ME_MODERNOPT_INITIALIZE, &CJabberProto::OnModernOptInit );
	JHookEvent( ME_OPT_INITIALISE, &CJabberProto::OnOptionsInit );
	JHookEvent( ME_SYSTEM_PRESHUTDOWN, &CJabberProto::OnPreShutdown );
	JHookEvent( ME_SKIN2_ICONSCHANGED, &CJabberProto::OnReloadIcons );

	m_iqManager.FillPermanentHandlers();
	m_iqManager.Start();
	m_adhocManager.FillDefaultNodes();
	m_clientCapsManager.AddDefaultCaps();

	JabberIconsInit();
	JabberMenuInit();
	JabberWsInit();
	JabberXStatusInit();
	JabberIqInit();
	JabberSerialInit();
	JabberConsoleInit();
	InitCustomFolders();
}

CJabberProto::~CJabberProto()
{
	JabberWsUninit();
	JabberIqUninit();
	JabberXStatusUninit();
	JabberSerialUninit();
	JabberConsoleUninit();
	JabberSslUninit();
	JabberMenuUninit();

	DestroyHookableEvent( heventNudge );
	DestroyHookableEvent( heventRawXMLIn );
	DestroyHookableEvent( heventRawXMLOut );
	DestroyHookableEvent( heventXStatusIconChanged );
	DestroyHookableEvent( heventXStatusChanged );
	if ( hInitChat )
		DestroyHookableEvent( hInitChat );

	JabberListWipe();
	DeleteCriticalSection( &csLists );

	mir_free( hIconLibItems );

	DeleteCriticalSection( &modeMsgMutex );
	mir_free( modeMsgs.szOnline );
	mir_free( modeMsgs.szAway );
	mir_free( modeMsgs.szNa );
	mir_free( modeMsgs.szDnd );
	mir_free( modeMsgs.szFreechat );

	mir_free( streamId );
	mir_free( szModuleName );
	mir_free( szProtoName );
	mir_free( tszUserName );

	if ( jabberVcardPhotoFileName ) {
		DeleteFileA( jabberVcardPhotoFileName );
		mir_free( jabberVcardPhotoFileName );
	}

	for ( int i=0; i < jabberTransports.getCount(); i++ )
		free( jabberTransports[i] );
	jabberTransports.destroy();
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberAddToList - adds a contact to the contact list

HANDLE CJabberProto::AddToListByJID( const TCHAR* newJid, DWORD flags )
{
	HANDLE hContact;
	TCHAR* jid, *nick;

	JabberLog( "AddToListByJID jid = " TCHAR_STR_PARAM, newJid );

	if (( hContact=JabberHContactFromJID( newJid )) == NULL ) {
		// not already there: add
		jid = mir_tstrdup( newJid );
		JabberLog( "Add new jid to contact jid = " TCHAR_STR_PARAM, jid );
		hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_ADD, 0, 0 );
		JCallService( MS_PROTO_ADDTOCONTACT, ( WPARAM ) hContact, ( LPARAM )szProtoName );
		JSetStringT( hContact, "jid", jid );
		if (( nick=JabberNickFromJID( newJid )) == NULL )
			nick = mir_tstrdup( newJid );
//		JSetStringT( hContact, "Nick", nick );
		mir_free( nick );
		mir_free( jid );

		// Note that by removing or disable the "NotOnList" will trigger
		// the plugin to add a particular contact to the roster list.
		// See DBSettingChanged hook at the bottom part of this source file.
		// But the add module will delete "NotOnList". So we will not do it here.
		// Also because we need "MyHandle" and "Group" info, which are set after
		// PS_ADDTOLIST is called but before the add dialog issue deletion of
		// "NotOnList".
		// If temporary add, "NotOnList" won't be deleted, and that's expected.
		DBWriteContactSettingByte( hContact, "CList", "NotOnList", 1 );
		if ( flags & PALF_TEMPORARY )
			DBWriteContactSettingByte( hContact, "CList", "Hidden", 1 );
	}
	else {
		// already exist
		// Set up a dummy "NotOnList" when adding permanently only
		if ( !( flags & PALF_TEMPORARY ))
			DBWriteContactSettingByte( hContact, "CList", "NotOnList", 1 );
	}

	if (hContact && newJid)
		JabberDBCheckIsTransportedContact( newJid, hContact );
	return hContact;
}

HANDLE CJabberProto::AddToList( int flags, PROTOSEARCHRESULT* psr )
{
	if ( psr->cbSize != sizeof( JABBER_SEARCH_RESULT ))
		return NULL;

	JABBER_SEARCH_RESULT* jsr = ( JABBER_SEARCH_RESULT* )psr;
	return AddToListByJID( jsr->jid, flags );
}

HANDLE __cdecl CJabberProto::AddToListByEvent( int flags, int iContact, HANDLE hDbEvent )
{
	DBEVENTINFO dbei;
	HANDLE hContact;
	char* nick, *firstName, *lastName, *jid;

	JabberLog( "AddToListByEvent" );
	ZeroMemory( &dbei, sizeof( dbei ));
	dbei.cbSize = sizeof( dbei );
	if (( dbei.cbBlob = JCallService( MS_DB_EVENT_GETBLOBSIZE, ( WPARAM )hDbEvent, 0 )) == ( DWORD )( -1 ))
		return NULL;
	if (( dbei.pBlob=( PBYTE ) alloca( dbei.cbBlob )) == NULL )
		return NULL;
	if ( JCallService( MS_DB_EVENT_GET, ( WPARAM )hDbEvent, ( LPARAM )&dbei ))
		return NULL;
	if ( strcmp( dbei.szModule, szProtoName ))
		return NULL;

/*
	// EVENTTYPE_CONTACTS is when adding from when we receive contact list ( not used in Jabber )
	// EVENTTYPE_ADDED is when adding from when we receive "You are added" ( also not used in Jabber )
	// Jabber will only handle the case of EVENTTYPE_AUTHREQUEST
	// EVENTTYPE_AUTHREQUEST is when adding from the authorization request dialog
*/

	if ( dbei.eventType != EVENTTYPE_AUTHREQUEST )
		return NULL;

	nick = ( char* )( dbei.pBlob + sizeof( DWORD )+ sizeof( HANDLE ));
	firstName = nick + strlen( nick ) + 1;
	lastName = firstName + strlen( firstName ) + 1;
	jid = lastName + strlen( lastName ) + 1;

	TCHAR* newJid = mir_a2t( jid );
	hContact = ( HANDLE ) AddToListByJID( newJid, flags );
	mir_free( newJid );
	return hContact;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberAuthAllow - processes the successful authorization

int CJabberProto::Authorize( HANDLE hContact )
{
	DBEVENTINFO dbei;
	char* nick, *firstName, *lastName, *jid;

	if ( !jabberOnline )
		return 1;

	memset( &dbei, sizeof( dbei ), 0 );
	dbei.cbSize = sizeof( dbei );
	if (( dbei.cbBlob=JCallService( MS_DB_EVENT_GETBLOBSIZE, ( WPARAM )hContact, 0 )) == ( DWORD )( -1 ))
		return 1;
	if (( dbei.pBlob=( PBYTE )alloca( dbei.cbBlob )) == NULL )
		return 1;
	if ( JCallService( MS_DB_EVENT_GET, ( WPARAM )hContact, ( LPARAM )&dbei ))
		return 1;
	if ( dbei.eventType != EVENTTYPE_AUTHREQUEST )
		return 1;
	if ( strcmp( dbei.szModule, szProtoName ))
		return 1;

	nick = ( char* )( dbei.pBlob + sizeof( DWORD )+ sizeof( HANDLE ));
	firstName = nick + strlen( nick ) + 1;
	lastName = firstName + strlen( firstName ) + 1;
	jid = lastName + strlen( lastName ) + 1;

	JabberLog( "Send 'authorization allowed' to " TCHAR_STR_PARAM, jid );

	XmlNode presence( "presence" ); presence.addAttr( "to", jid ); presence.addAttr( "type", "subscribed" );
	jabberThreadInfo->send( presence );

	TCHAR* newJid = mir_a2t( jid );

	// Automatically add this user to my roster if option is enabled
	if ( JGetByte( "AutoAdd", TRUE ) == TRUE ) {
		HANDLE hContact;
		JABBER_LIST_ITEM *item;

		if (( item = JabberListGetItemPtr( LIST_ROSTER, newJid )) == NULL || ( item->subscription != SUB_BOTH && item->subscription != SUB_TO )) {
			JabberLog( "Try adding contact automatically jid = " TCHAR_STR_PARAM, jid );
			if (( hContact=AddToListByJID( newJid, 0 )) != NULL ) {
				// Trigger actual add by removing the "NotOnList" added by AddToListByJID()
				// See AddToListByJID() and JabberDbSettingChanged().
				DBDeleteContactSetting( hContact, "CList", "NotOnList" );
	}	}	}

	mir_free( newJid );
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberAuthDeny - handles the unsuccessful authorization

int CJabberProto::AuthDeny( HANDLE hContact, const char* szReason )
{
	DBEVENTINFO dbei;
	char* nick, *firstName, *lastName, *jid;

	if ( !jabberOnline )
		return 1;

	JabberLog( "Entering AuthDeny" );
	memset( &dbei, sizeof( dbei ), 0 );
	dbei.cbSize = sizeof( dbei );
	if (( dbei.cbBlob=JCallService( MS_DB_EVENT_GETBLOBSIZE, ( WPARAM )hContact, 0 )) == ( DWORD )( -1 ))
		return 1;
	if (( dbei.pBlob=( PBYTE ) mir_alloc( dbei.cbBlob )) == NULL )
		return 1;
	if ( JCallService( MS_DB_EVENT_GET, ( WPARAM )hContact, ( LPARAM )&dbei )) {
		mir_free( dbei.pBlob );
		return 1;
	}
	if ( dbei.eventType != EVENTTYPE_AUTHREQUEST ) {
		mir_free( dbei.pBlob );
		return 1;
	}
	if ( strcmp( dbei.szModule, szProtoName )) {
		mir_free( dbei.pBlob );
		return 1;
	}

	nick = ( char* )( dbei.pBlob + sizeof( DWORD )+ sizeof( HANDLE ));
	firstName = nick + strlen( nick ) + 1;
	lastName = firstName + strlen( firstName ) + 1;
	jid = lastName + strlen( lastName ) + 1;

	JabberLog( "Send 'authorization denied' to " TCHAR_STR_PARAM, jid );

	XmlNode presence( "presence" ); presence.addAttr( "to", jid ); presence.addAttr( "type", "unsubscribed" );
	jabberThreadInfo->send( presence );

	mir_free( dbei.pBlob );
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// PSS_ADDED

int __cdecl CJabberProto::AuthRecv( HANDLE hContact, PROTORECVEVENT* evt )
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// PSS_AUTHREQUEST

int __cdecl CJabberProto::AuthRequest( HANDLE hContact, const char* szMessage )
{	
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// ChangeInfo 

HANDLE __cdecl CJabberProto::ChangeInfo( int iInfoType, void* pInfoData )
{
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberFileAllow - starts a file transfer

int __cdecl CJabberProto::FileAllow( HANDLE hContact, HANDLE hTransfer, const char* szPath )
{
	if ( !jabberOnline )
		return 0;

	filetransfer* ft = ( filetransfer* )hTransfer;
	ft->std.workingDir = mir_strdup( szPath );
	int len = strlen( ft->std.workingDir )-1;
	if ( ft->std.workingDir[len] == '/' || ft->std.workingDir[len] == '\\' )
		ft->std.workingDir[len] = 0;

	switch ( ft->type ) {
	case FT_OOB:
		mir_forkthread(( pThreadFunc )::JabberFileReceiveThread, ft );
		break;
	case FT_BYTESTREAM:
		JabberFtAcceptSiRequest( ft );
		break;
	case FT_IBB:
		JabberFtAcceptIbbRequest( ft );
		break;
	}
	return int( hTransfer );
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberFileCancel - cancels a file transfer

int __cdecl CJabberProto::FileCancel( HANDLE hContact, HANDLE hTransfer )
{
	filetransfer* ft = ( filetransfer* )hTransfer;
	HANDLE hEvent;

	JabberLog( "Invoking FileCancel()" );
	if ( ft->type == FT_OOB ) {
		if ( ft->s ) {
			JabberLog( "FT canceled" );
			JabberLog( "Closing ft->s = %d", ft->s );
			ft->state = FT_ERROR;
			Netlib_CloseHandle( ft->s );
			ft->s = NULL;
			if ( ft->hFileEvent != NULL ) {
				hEvent = ft->hFileEvent;
				ft->hFileEvent = NULL;
				SetEvent( hEvent );
			}
			JabberLog( "ft->s is now NULL, ft->state is now FT_ERROR" );
		}
	}
	else JabberFtCancel( ft );
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberFileDeny - denies a file transfer

int __cdecl CJabberProto::FileDeny( HANDLE hContact, HANDLE hTransfer, const char* szReason )
{
	if ( !jabberOnline )
		return 1;

	filetransfer* ft = ( filetransfer* )hTransfer;

	XmlNodeIq iq( "error", ft->iqId, ft->jid );

	switch ( ft->type ) {
	case FT_OOB:
		{	XmlNode* e = iq.addChild( "error", _T("File transfer refused"));
			e->addAttr( "code", 406 );
			jabberThreadInfo->send( iq );
		}
		break;
	case FT_BYTESTREAM:
	case FT_IBB:
		{	XmlNode* e = iq.addChild( "error", _T("File transfer refused"));
			e->addAttr( "code", 403 ); e->addAttr( "type", "cancel" );
			XmlNode* f = e->addChild( "forbidden" ); f->addAttr( "xmlns", "urn:ietf:params:xml:ns:xmpp-stanzas" );
			XmlNode* t = f->addChild( "text", "File transfer refused" ); t->addAttr( "xmlns", "urn:ietf:params:xml:ns:xmpp-stanzas" );
			jabberThreadInfo->send( iq );
		}
		break;
	}
	delete ft;
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberFileResume - processes file renaming etc

int __cdecl CJabberProto::FileResume( HANDLE hTransfer, int* action, const char** szFilename )
{
	filetransfer* ft = ( filetransfer* )hTransfer;
	if ( !jabberConnected || ft == NULL )
		return 1;

	if ( *action == FILERESUME_RENAME ) {
		if ( ft->wszFileName != NULL ) {
			mir_free( ft->wszFileName );
			ft->wszFileName = NULL;
		}

		replaceStr( ft->std.currentFile, *szFilename );
	}

	SetEvent( ft->hWaitEvent );
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// GetCaps - return protocol capabilities bits

DWORD __cdecl CJabberProto::GetCaps( int type )
{
	switch( type ) {
	case PFLAGNUM_1:
		return PF1_IM | PF1_AUTHREQ | PF1_CHAT | PF1_SERVERCLIST | PF1_MODEMSG|PF1_BASICSEARCH | PF1_EXTSEARCH | PF1_FILE;
	case PFLAGNUM_2:
		return PF2_ONLINE | PF2_INVISIBLE | PF2_SHORTAWAY | PF2_LONGAWAY | PF2_HEAVYDND | PF2_FREECHAT;
	case PFLAGNUM_3:
		return PF2_ONLINE | PF2_SHORTAWAY | PF2_LONGAWAY | PF2_HEAVYDND | PF2_FREECHAT;
	case PFLAGNUM_4:
		return PF4_FORCEAUTH | PF4_NOCUSTOMAUTH | PF4_SUPPORTTYPING | PF4_AVATARS | PF4_IMSENDUTF;
	case PFLAG_UNIQUEIDTEXT:
		return ( DWORD ) JTranslate( "JID" );
	case PFLAG_UNIQUEIDSETTING:
		return ( DWORD ) "jid";
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// GetIcon - loads an icon for the contact list

HICON __cdecl CJabberProto::GetIcon( int iconIndex )
{
	if (( iconIndex & 0xffff ) == PLI_PROTOCOL )
		return CopyIcon( LoadIconEx( "main" ));

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////
// GetInfo - retrieves a contact info

int __cdecl CJabberProto::GetInfo( HANDLE hContact, int infoType )
{
	if ( !jabberOnline )
		return 1;

	int result = 1;
	DBVARIANT dbv;
	if ( JGetByte( hContact, "ChatRoom" , 0) )
		return 1;

	if ( !JGetStringT( hContact, "jid", &dbv )) {
		if ( jabberThreadInfo ) {
			TCHAR jid[ 256 ];
			JabberGetClientJID( dbv.ptszVal, jid, SIZEOF( jid ));

			XmlNodeIq iq( m_iqManager.AddHandler( &CJabberProto::JabberIqResultEntityTime, JABBER_IQ_TYPE_GET, jid, JABBER_IQ_PARSE_HCONTACT ));
			XmlNode* pReq = iq.addChild( "time" );
			pReq->addAttr( "xmlns", JABBER_FEAT_ENTITY_TIME );
			jabberThreadInfo->send( iq );

			// XEP-0012, last logoff time
			XmlNodeIq iq2( m_iqManager.AddHandler( &CJabberProto::JabberIqResultLastActivity, JABBER_IQ_TYPE_GET, dbv.ptszVal, JABBER_IQ_PARSE_FROM ));
			iq2.addQuery( JABBER_FEAT_LAST_ACTIVITY );
			jabberThreadInfo->send( iq2 );

			JABBER_LIST_ITEM *item = NULL;

			if (( item = JabberListGetItemPtr( LIST_VCARD_TEMP, dbv.ptszVal )) == NULL)
				item = JabberListGetItemPtr( LIST_ROSTER, dbv.ptszVal );

			if ( !item ) {
				TCHAR szBareJid[ 1024 ];
				_tcsncpy( szBareJid, dbv.ptszVal, 1023 );
				TCHAR* pDelimiter = _tcschr( szBareJid, _T('/') );
				if ( pDelimiter ) {
					*pDelimiter = 0;
					pDelimiter++;
					if ( !*pDelimiter )
						pDelimiter = NULL;
				}
				JABBER_LIST_ITEM *tmpItem = NULL;
				if ( pDelimiter && ( tmpItem  = JabberListGetItemPtr( LIST_CHATROOM, szBareJid ))) {
					JABBER_RESOURCE_STATUS *him = NULL;
					for ( int i=0; i < tmpItem->resourceCount; i++ ) {
						JABBER_RESOURCE_STATUS& p = tmpItem->resource[i];
						if ( !lstrcmp( p.resourceName, pDelimiter )) him = &p;
					}
					if ( him ) {
						item = JabberListAdd( LIST_VCARD_TEMP, dbv.ptszVal );
						JabberListAddResource( LIST_VCARD_TEMP, dbv.ptszVal, him->status, him->statusMessage, him->priority );
					}
				}
				else
					item = JabberListAdd( LIST_VCARD_TEMP, dbv.ptszVal );
			}

			if ( item ) {
				if ( item->resource ) {
					for ( int i = 0; i < item->resourceCount; i++ ) {
						TCHAR szp1[ JABBER_MAX_JID_LEN ];
						JabberStripJid( dbv.ptszVal, szp1, SIZEOF( szp1 ));
						mir_sntprintf( jid, 256, _T("%s/%s"), szp1, item->resource[i].resourceName );

						XmlNodeIq iq3( m_iqManager.AddHandler( &CJabberProto::JabberIqResultLastActivity, JABBER_IQ_TYPE_GET, jid, JABBER_IQ_PARSE_FROM ));
						iq3.addQuery( JABBER_FEAT_LAST_ACTIVITY );
						jabberThreadInfo->send( iq3 );

						if ( !item->resource[i].dwVersionRequestTime ) {
							XmlNodeIq iq4( m_iqManager.AddHandler( &CJabberProto::JabberProcessIqResultVersion, JABBER_IQ_TYPE_GET, jid, JABBER_IQ_PARSE_FROM | JABBER_IQ_PARSE_HCONTACT | JABBER_IQ_PARSE_CHILD_TAG_NODE ));
							XmlNode* query = iq4.addQuery( JABBER_FEAT_VERSION );
							jabberThreadInfo->send( iq4 );
					}	}
				}
				else if ( !item->itemResource.dwVersionRequestTime ) {
					XmlNodeIq iq4( m_iqManager.AddHandler( &CJabberProto::JabberProcessIqResultVersion, JABBER_IQ_TYPE_GET, item->jid, JABBER_IQ_PARSE_FROM | JABBER_IQ_PARSE_HCONTACT | JABBER_IQ_PARSE_CHILD_TAG_NODE ));
					XmlNode* query = iq4.addQuery( JABBER_FEAT_VERSION );
					jabberThreadInfo->send( iq4 );
		}	}	}

		JabberSendGetVcard( dbv.ptszVal );
		JFreeVariant( &dbv );
		result = 0;
	}

	return result;
}

////////////////////////////////////////////////////////////////////////////////////////
// SearchBasic - searches the contact by JID

struct JABBER_SEARCH_BASIC
{	int hSearch;
	CJabberProto* ppro;
	char jid[128];
};

static void __cdecl JabberBasicSearchThread( JABBER_SEARCH_BASIC *jsb )
{
	SleepEx( 100, TRUE );

	JABBER_SEARCH_RESULT jsr = { 0 };
	jsr.hdr.cbSize = sizeof( JABBER_SEARCH_RESULT );
	jsr.hdr.nick = "";
	jsr.hdr.firstName = "";
	jsr.hdr.lastName = "";
	jsr.hdr.email = jsb->jid;

	TCHAR* jid = mir_a2t(jsb->jid);
	_tcsncpy( jsr.jid, jid, SIZEOF( jsr.jid ));
	mir_free( jid );

	jsr.jid[SIZEOF( jsr.jid )-1] = '\0';
	jsb->ppro->JSendBroadcast( NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, ( HANDLE ) jsb->hSearch, ( LPARAM )&jsr );
	jsb->ppro->JSendBroadcast( NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, ( HANDLE ) jsb->hSearch, 0 );
	mir_free( jsb );
}

HANDLE __cdecl CJabberProto::SearchBasic( const char* szJid )
{
	JabberLog( "JabberBasicSearch called with lParam = '%s'", szJid );

	JABBER_SEARCH_BASIC *jsb;
	if ( !jabberOnline || ( jsb=( JABBER_SEARCH_BASIC * ) mir_alloc( sizeof( JABBER_SEARCH_BASIC )) )==NULL )
		return 0;

	if ( strchr( szJid, '@' ) == NULL ) {
		const char* p = strstr( szJid, jabberThreadInfo->server );
		if ( !p ) {
			char szServer[ 100 ];
			if ( JGetStaticString( "LoginServer", NULL, szServer, sizeof szServer ))
				strcpy( szServer, "jabber.org" );

			mir_snprintf( jsb->jid, SIZEOF(jsb->jid), "%s@%s", szJid, szServer );
		}
		else strncpy( jsb->jid, szJid, SIZEOF(jsb->jid));
	}
	else strncpy( jsb->jid, szJid, SIZEOF(jsb->jid));

	JabberLog( "Adding '%s' without validation", jsb->jid );
	jsb->hSearch = JabberSerialNext();
	jsb->ppro = this;
	mir_forkthread(( pThreadFunc )JabberBasicSearchThread, jsb );
	return ( HANDLE )jsb->hSearch;
}

////////////////////////////////////////////////////////////////////////////////////////
// SearchByEmail - searches the contact by its e-mail

HANDLE __cdecl CJabberProto::SearchByEmail( const char* email )
{
	if ( !jabberOnline ) return 0;
	if ( email == NULL ) return 0;

	char szServerName[100];
	if ( JGetStaticString( "Jud", NULL, szServerName, sizeof szServerName ))
		strcpy( szServerName, "users.jabber.org" );

	int iqId = JabberSerialNext();
	JabberIqAdd( iqId, IQ_PROC_GETSEARCH, &CJabberProto::JabberIqResultSetSearch );

	XmlNodeIq iq( "set", iqId, szServerName );
	XmlNode* query = iq.addQuery( "jabber:iq:search" );
	query->addChild( "email", email );
	jabberThreadInfo->send( iq );
	return ( HANDLE )iqId;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberSearchByName - searches the contact by its first or last name, or by a nickname

HANDLE __cdecl CJabberProto::SearchByName( const char* nick, const char* firstName, const char* lastName )
{
	if ( !jabberOnline )
		return NULL;

	BOOL bIsExtFormat = JGetByte( "ExtendedSearch", TRUE );

	char szServerName[100];
	if ( JGetStaticString( "Jud", NULL, szServerName, sizeof szServerName ))
		strcpy( szServerName, "users.jabber.org" );

	int iqId = JabberSerialNext();
	XmlNodeIq iq( "set", iqId, szServerName );
	XmlNode* query = iq.addChild( "query" ), *field, *x;
	query->addAttr( "xmlns", "jabber:iq:search" );

	if ( bIsExtFormat ) {
		JabberIqAdd( iqId, IQ_PROC_GETSEARCH, &CJabberProto::JabberIqResultExtSearch );

		TCHAR *szXmlLang = JabberGetXmlLang();
		if ( szXmlLang ) {
			iq.addAttr( "xml:lang", szXmlLang );
			mir_free( szXmlLang );
		}
		x = query->addChild( "x" ); x->addAttr( "xmlns", JABBER_FEAT_DATA_FORMS ); x->addAttr( "type", "submit" );
	}
	else JabberIqAdd( iqId, IQ_PROC_GETSEARCH, &CJabberProto::JabberIqResultSetSearch );

	if ( nick[0] != '\0' ) {
		if ( bIsExtFormat ) {
			field = x->addChild( "field" ); field->addAttr( "var", "user" );
			field->addChild( "value", nick );
		}
		else query->addChild( "nick", nick );
	}

	if ( firstName[0] != '\0' ) {
		if ( bIsExtFormat ) {
			field = x->addChild( "field" ); field->addAttr( "var", "fn" );
			field->addChild( "value", firstName );
		}
		else query->addChild( "first", firstName );
	}

	if ( lastName[0] != '\0' ) {
		if ( bIsExtFormat ) {
			field = x->addChild( "field" ); field->addAttr( "var", "given" );
			field->addChild( "value", lastName );
		}
		else query->addChild( "last", lastName );
	}

	jabberThreadInfo->send( iq );
	return ( HANDLE )iqId;
}

////////////////////////////////////////////////////////////////////////////////////////
// RecvContacts

int __cdecl CJabberProto::RecvContacts( HANDLE hContact, PROTORECVEVENT* )
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// RecvFile

int __cdecl CJabberProto::RecvFile( HANDLE hContact, PROTORECVFILE* evt )
{
	CCSDATA ccs = { hContact, PSR_FILE, 0, ( LPARAM )evt };
	return JCallService( MS_PROTO_RECVFILE, 0, ( LPARAM )&ccs );
}

////////////////////////////////////////////////////////////////////////////////////////
// RecvMsg

int __cdecl CJabberProto::RecvMsg( HANDLE hContact, PROTORECVEVENT* evt )
{
	CCSDATA ccs = { hContact, PSR_MESSAGE, 0, ( LPARAM )evt };
	return JCallService( MS_PROTO_RECVMSG, 0, ( LPARAM )&ccs );
}

////////////////////////////////////////////////////////////////////////////////////////
// RecvUrl

int __cdecl CJabberProto::RecvUrl( HANDLE hContact, PROTORECVEVENT* )
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// SendContacts

int __cdecl CJabberProto::SendContacts( HANDLE hContact, int flags, int nContacts, HANDLE* hContactsList )
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// SendFile - sends a file

int __cdecl CJabberProto::SendFile( HANDLE hContact, const char* szDescription, char** ppszFiles )
{
	if ( !jabberOnline ) return 0;

	if ( JGetWord( hContact, "Status", ID_STATUS_OFFLINE ) == ID_STATUS_OFFLINE )
		return 0;

	DBVARIANT dbv;
	if ( JGetStringT( hContact, "jid", &dbv ))
		return 0;

	int i, j;
	struct _stat statbuf;
	JABBER_LIST_ITEM* item = JabberListGetItemPtr( LIST_ROSTER, dbv.ptszVal );
	if ( item == NULL ) {
		JFreeVariant( &dbv );
		return 0;
	}

	// Check if another file transfer session request is pending ( waiting for disco result )
	if ( item->ft != NULL ) {
		JFreeVariant( &dbv );
		return 0;
	}

	JabberCapsBits jcb = JabberGetResourceCapabilites( item->jid, TRUE );

	// fix for very smart clients, like gajim
	if ( !JGetByte( "BsDirect", FALSE ) && !JGetByte( "BsProxyManual", FALSE ) ) {
		// disable bytestreams
		jcb &= ~JABBER_CAPS_BYTESTREAMS;
	}

	// if only JABBER_CAPS_SI_FT feature set (without BS or IBB), disable JABBER_CAPS_SI_FT
	if (( jcb & (JABBER_CAPS_SI_FT | JABBER_CAPS_IBB | JABBER_CAPS_BYTESTREAMS)) == JABBER_CAPS_SI_FT)
		jcb &= ~JABBER_CAPS_SI_FT;

	if (
		// can't get caps
		( jcb & JABBER_RESOURCE_CAPS_ERROR )
		// caps not already received
		|| ( jcb == JABBER_RESOURCE_CAPS_NONE )
		// XEP-0096 and OOB not supported?
		|| !(jcb & ( JABBER_CAPS_SI_FT | JABBER_CAPS_OOB ) )
		) {
		JFreeVariant( &dbv );
		return 0;
	}

	filetransfer* ft = new filetransfer(this);
	ft->std.hContact = hContact;
	while( ppszFiles[ ft->std.totalFiles ] != NULL )
		ft->std.totalFiles++;

	ft->std.files = ( char** ) mir_alloc( sizeof( char* )* ft->std.totalFiles );
	ft->fileSize = ( long* ) mir_alloc( sizeof( long ) * ft->std.totalFiles );
	for( i=j=0; i < ft->std.totalFiles; i++ ) {
		if ( _stat( ppszFiles[i], &statbuf ))
			JabberLog( "'%s' is an invalid filename", ppszFiles[i] );
		else {
			ft->std.files[j] = mir_strdup( ppszFiles[i] );
			ft->fileSize[j] = statbuf.st_size;
			j++;
			ft->std.totalBytes += statbuf.st_size;
	}	}

	ft->std.currentFile = mir_strdup( ppszFiles[0] );
	ft->szDescription = mir_strdup( szDescription );
	ft->jid = mir_tstrdup( dbv.ptszVal );
	JFreeVariant( &dbv );

	if ( jcb & JABBER_CAPS_SI_FT )
		JabberFtInitiate( item->jid, ft );
	else if ( jcb & JABBER_CAPS_OOB )
		mir_forkthread(( pThreadFunc )::JabberFileServerThread, ft );

	return ( int )( HANDLE ) ft;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberSendMessage - sends a message

struct JabberSendMessageAckThreadParam
{
	HANDLE hContact;
	CJabberProto* ppro;
};

static void __cdecl JabberSendMessageAckThread( JabberSendMessageAckThreadParam* params )
{
	SleepEx( 10, TRUE );
	params->ppro->JabberLog( "Broadcast ACK" );
	params->ppro->JSendBroadcast( params->hContact, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, ( HANDLE ) 1, 0 );
	params->ppro->JabberLog( "Returning from thread" );
	delete params;
}

static char PGP_PROLOG[] = "-----BEGIN PGP MESSAGE-----\r\n\r\n";
static char PGP_EPILOG[] = "\r\n-----END PGP MESSAGE-----\r\n";

int __cdecl CJabberProto::SendMsg( HANDLE hContact, int flags, const char* pszSrc )
{
	int id;

	DBVARIANT dbv;
	if ( !jabberOnline || JGetStringT( hContact, "jid", &dbv )) {
		JSendBroadcast( hContact, ACKTYPE_MESSAGE, ACKRESULT_FAILED, ( HANDLE ) 1, 0 );
		return 0;
	}

	char *msg;
	int  isEncrypted;

	if ( !strncmp( pszSrc, PGP_PROLOG, strlen( PGP_PROLOG ))) {
		const char* szEnd = strstr( pszSrc, PGP_EPILOG );
		char* tempstring = ( char* )alloca( strlen( pszSrc ) + 1 );
		int nStrippedLength = strlen(pszSrc) - strlen(PGP_PROLOG) - (szEnd ? strlen(szEnd) : 0);
		strncpy( tempstring, pszSrc + strlen(PGP_PROLOG), nStrippedLength );
		tempstring[ nStrippedLength ] = 0;
		pszSrc = tempstring;
		isEncrypted = 1;
		flags &= ~PREF_UNICODE;
	}
	else isEncrypted = 0;

	if ( flags & PREF_UTF )
		msg = JabberUrlEncode( pszSrc );
	else if ( flags & PREF_UNICODE )
		msg = JabberTextEncodeW(( wchar_t* )&pszSrc[ strlen( pszSrc )+1 ] );
	else
		msg = JabberTextEncode( pszSrc );

	int nSentMsgId = 0;

	if ( msg != NULL ) {
		char msgType[ 16 ];
		if ( JabberListExist( LIST_CHATROOM, dbv.ptszVal ) && _tcschr( dbv.ptszVal, '/' )==NULL )
			strcpy( msgType, "groupchat" );
		else
			strcpy( msgType, "chat" );

		XmlNode m( "message" ); m.addAttr( "type", msgType );
		if ( !isEncrypted ) {
			XmlNode* body = m.addChild( "body" );
			body->sendText = msg;
		}
		else {
			m.addChild( "body", "[This message is encrypted.]" );
			XmlNode* x = m.addChild( "x" ); x->addAttr( "xmlns", "jabber:x:encrypted" );
			x->sendText = msg;
		}

		TCHAR szClientJid[ 256 ];
		JabberGetClientJID( dbv.ptszVal, szClientJid, SIZEOF( szClientJid ));

		JABBER_RESOURCE_STATUS *r = JabberResourceInfoFromJID( szClientJid );
		if ( r )
			r->bMessageSessionActive = TRUE;

		JabberCapsBits jcb = JabberGetResourceCapabilites( szClientJid, TRUE );

		if ( jcb & JABBER_RESOURCE_CAPS_ERROR )
			jcb = JABBER_RESOURCE_CAPS_NONE;

		if ( jcb & JABBER_CAPS_CHATSTATES )
			m.addChild( "active" )->addAttr( "xmlns", _T(JABBER_FEAT_CHATSTATES));

		if (
			// if message delivery check disabled by entity caps manager
			( jcb & JABBER_CAPS_MESSAGE_EVENTS_NO_DELIVERY ) ||
			// if client knows nothing about delivery
			!( jcb & ( JABBER_CAPS_MESSAGE_EVENTS | JABBER_CAPS_MESSAGE_RECEIPTS )) ||
			// if message sent to groupchat
			!strcmp( msgType, "groupchat" ) ||
			// if message delivery check disabled in settings
			!JGetByte( "MsgAck", FALSE ) || !JGetByte( hContact, "MsgAck", TRUE )) {
			if ( !strcmp( msgType, "groupchat" ))
				m.addAttr( "to", dbv.ptszVal );
			else {
				id = JabberSerialNext();
				m.addAttr( "to", szClientJid ); m.addAttrID( id );
			}
			jabberThreadInfo->send( m );

			JabberSendMessageAckThreadParam* params = new JabberSendMessageAckThreadParam;
			params->hContact = hContact;
			params->ppro = this;
			mir_forkthread(( pThreadFunc )JabberSendMessageAckThread, params );

			nSentMsgId = 1;
		}
		else {
			id = JabberSerialNext();
			m.addAttr( "to", szClientJid ); m.addAttrID( id );

			// message receipts XEP priority
			if ( jcb & JABBER_CAPS_MESSAGE_RECEIPTS ) {
				XmlNode* receiptRequest = m.addChild( "request" );
				receiptRequest->addAttr( "xmlns", JABBER_FEAT_MESSAGE_RECEIPTS );
			}
			else if ( jcb & JABBER_CAPS_MESSAGE_EVENTS ) {
				XmlNode* x = m.addChild( "x" ); x->addAttr( "xmlns", JABBER_FEAT_MESSAGE_EVENTS );
				x->addChild( "delivered" );
				x->addChild( "offline" );
			}
			else
				id = 1;

			jabberThreadInfo->send( m );
			nSentMsgId = id;
	}	}

	JFreeVariant( &dbv );
	return nSentMsgId;
}

////////////////////////////////////////////////////////////////////////////////////////
// SendUrl

int __cdecl CJabberProto::SendUrl( HANDLE hContact, int flags, const char* url )
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberSetApparentMode - sets the visibility status

int __cdecl CJabberProto::SetApparentMode( HANDLE hContact, int mode )
{
	if ( mode != 0 && mode != ID_STATUS_ONLINE && mode != ID_STATUS_OFFLINE )
		return 1;
	
	int oldMode = JGetWord( hContact, "ApparentMode", 0 );
	if ( mode == oldMode )
		return 1;

	JSetWord( hContact, "ApparentMode", ( WORD )mode );
	if ( !jabberOnline )
		return 0;

	DBVARIANT dbv;
	if ( !JGetStringT( hContact, "jid", &dbv )) {
		TCHAR* jid = dbv.ptszVal;
		switch ( mode ) {
		case ID_STATUS_ONLINE:
			if ( iStatus == ID_STATUS_INVISIBLE || oldMode == ID_STATUS_OFFLINE ) {
				XmlNode p( "presence" ); p.addAttr( "to", jid );
				jabberThreadInfo->send( p );
			}
			break;
		case ID_STATUS_OFFLINE:
			if ( iStatus != ID_STATUS_INVISIBLE || oldMode == ID_STATUS_ONLINE )
				JabberSendPresenceTo( ID_STATUS_INVISIBLE, jid, NULL );
			break;
		case 0:
			if ( oldMode == ID_STATUS_ONLINE && iStatus == ID_STATUS_INVISIBLE )
				JabberSendPresenceTo( ID_STATUS_INVISIBLE, jid, NULL );
			else if ( oldMode == ID_STATUS_OFFLINE && iStatus != ID_STATUS_INVISIBLE )
				JabberSendPresenceTo( iStatus, jid, NULL );
			break;
		}
		JFreeVariant( &dbv );
	}

	// TODO: update the zebra list ( jabber:iq:privacy )

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberSetStatus - sets the protocol status

int __cdecl CJabberProto::SetStatus( int iNewStatus )
{
	JabberLog( "PS_SETSTATUS( %d )", iNewStatus );
	int desiredStatus = iNewStatus;
	iDesiredStatus = desiredStatus;

 	if ( desiredStatus == ID_STATUS_OFFLINE ) {
		if ( jabberThreadInfo ) {
			jabberThreadInfo->send( "</stream:stream>" );
			jabberThreadInfo->close();
			jabberThreadInfo = NULL;
			if ( jabberConnected ) {
				jabberConnected = jabberOnline = FALSE;
				JabberRebuildStatusMenu();
			}
		}

		int oldStatus = iStatus;
		iStatus = iDesiredStatus = ID_STATUS_OFFLINE;
		JSendBroadcast( NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, ( HANDLE ) oldStatus, iStatus );
	}
	else if ( !jabberConnected && !( iStatus >= ID_STATUS_CONNECTING && iStatus < ID_STATUS_CONNECTING + MAX_CONNECT_RETRIES )) {
		if ( jabberConnected )
			return 0;

		ThreadData* thread = new ThreadData( this, JABBER_SESSION_NORMAL );
		iDesiredStatus = desiredStatus;
		int oldStatus = iStatus;
		iStatus = ID_STATUS_CONNECTING;
		JSendBroadcast( NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, ( HANDLE ) oldStatus, iStatus );
		thread->hThread = ( HANDLE ) mir_forkthread(( pThreadFunc )JabberServerThread, thread );
	}
	else JabberSetServerStatus( desiredStatus );

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberGetAwayMsg - returns a contact's away message

struct JabberGetAwayMsgThreadParams
{
	CJabberProto* ppro;
	HANDLE hContact;
};

static void __cdecl JabberGetAwayMsgThread( JabberGetAwayMsgThreadParams* params )
{
	DBVARIANT dbv;
	JABBER_LIST_ITEM *item;
	JABBER_RESOURCE_STATUS *r;
	int i, len, msgCount;
	HANDLE hContact = params->hContact;
	CJabberProto* ppro = params->ppro;
	delete params;

	if ( !ppro->JGetStringT( hContact, "jid", &dbv )) {
		if (( item = ppro->JabberListGetItemPtr( LIST_ROSTER, dbv.ptszVal )) != NULL ) {
			JFreeVariant( &dbv );
			if ( item->resourceCount > 0 ) {
				ppro->JabberLog( "resourceCount > 0" );
				r = item->resource;
				len = msgCount = 0;
				for ( i=0; i<item->resourceCount; i++ ) {
					if ( r[i].statusMessage ) {
						msgCount++;
						len += ( _tcslen( r[i].resourceName ) + _tcslen( r[i].statusMessage ) + 8 );
				}	}

				TCHAR* str = ( TCHAR* )alloca( sizeof( TCHAR )*( len+1 ));
				str[0] = str[len] = '\0';
				for ( i=0; i < item->resourceCount; i++ ) {
					if ( r[i].statusMessage ) {
						if ( str[0] != '\0' ) _tcscat( str, _T("\r\n" ));
						if ( msgCount > 1 ) {
							_tcscat( str, _T("( "));
							_tcscat( str, r[i].resourceName );
							_tcscat( str, _T(" ): "));
						}
						_tcscat( str, r[i].statusMessage );
				}	}

				char* msg = mir_t2a(str);
				char* msg2 = JabberUnixToDos(msg);
				ppro->JSendBroadcast( hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, ( HANDLE ) 1, ( LPARAM )msg2 );
				mir_free(msg);
				mir_free(msg2);
				return;
			}

			if ( item->itemResource.statusMessage != NULL ) {
				char* msg = mir_t2a(item->itemResource.statusMessage);
				ppro->JSendBroadcast( hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, ( HANDLE ) 1, ( LPARAM )msg );
				mir_free(msg);
				return;
			}
		}
		else JFreeVariant( &dbv );
	}

	ppro->JSendBroadcast( hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, ( HANDLE ) 1, ( LPARAM )"" );
}

int __cdecl CJabberProto::GetAwayMsg( HANDLE hContact )
{
	JabberLog( "GetAwayMsg called, hContact=%08X", hContact );

	JabberGetAwayMsgThreadParams* params = new JabberGetAwayMsgThreadParams;
	params->hContact = hContact;
	params->ppro = this;
	mir_forkthread(( pThreadFunc )JabberGetAwayMsgThread, params );
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// PSR_AWAYMSG

int __cdecl CJabberProto::RecvAwayMsg( HANDLE hContact, int statusMode, PROTORECVEVENT* evt )
{	
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// PSS_AWAYMSG

int __cdecl CJabberProto::SendAwayMsg( HANDLE hContact, HANDLE hProcess, const char* msg )
{	
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberSetAwayMsg - sets the away status message

int __cdecl CJabberProto::SetAwayMsg( int status, const char* msg )
{
	JabberLog( "SetAwayMsg called, wParam=%d lParam=%s", status, msg );

	EnterCriticalSection( &modeMsgMutex );

	char **szMsg;

	switch ( status ) {
	case ID_STATUS_ONLINE:
		szMsg = &modeMsgs.szOnline;
		break;
	case ID_STATUS_AWAY:
	case ID_STATUS_ONTHEPHONE:
	case ID_STATUS_OUTTOLUNCH:
		szMsg = &modeMsgs.szAway;
		status = ID_STATUS_AWAY;
		break;
	case ID_STATUS_NA:
		szMsg = &modeMsgs.szNa;
		break;
	case ID_STATUS_DND:
	case ID_STATUS_OCCUPIED:
		szMsg = &modeMsgs.szDnd;
		status = ID_STATUS_DND;
		break;
	case ID_STATUS_FREECHAT:
		szMsg = &modeMsgs.szFreechat;
		break;
	default:
		LeaveCriticalSection( &modeMsgMutex );
		return 1;
	}

	char* newModeMsg = mir_strdup( msg );

	if (( *szMsg == NULL && newModeMsg == NULL ) ||
		( *szMsg != NULL && newModeMsg != NULL && !strcmp( *szMsg, newModeMsg )) ) {
		// Message is the same, no update needed
		if ( newModeMsg != NULL )
			mir_free( newModeMsg );
	}
	else {
		// Update with the new mode message
		if ( *szMsg != NULL )
			mir_free( *szMsg );
		*szMsg = newModeMsg;
		// Send a presence update if needed
		if ( status == iStatus ) {
			JabberSendPresence( iStatus, true );
	}	}

	LeaveCriticalSection( &modeMsgMutex );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberUserIsTyping - sends a UTN notification

int __cdecl CJabberProto::UserIsTyping( HANDLE hContact, int type )
{
	if ( !jabberOnline ) return 0;

	DBVARIANT dbv;
	if ( JGetStringT( hContact, "jid", &dbv )) return 0;

	JABBER_LIST_ITEM *item;
	if (( item = JabberListGetItemPtr( LIST_ROSTER, dbv.ptszVal )) != NULL ) {
		TCHAR szClientJid[ 256 ];
		JabberGetClientJID( dbv.ptszVal, szClientJid, SIZEOF( szClientJid ));

		JabberCapsBits jcb = JabberGetResourceCapabilites( szClientJid, TRUE );

		if ( jcb & JABBER_RESOURCE_CAPS_ERROR )
			jcb = JABBER_RESOURCE_CAPS_NONE;

		XmlNode m( "message" ); m.addAttr( "to", szClientJid );

		if ( jcb & JABBER_CAPS_CHATSTATES ) {
			m.addAttr( "type", "chat" );
			m.addAttrID( JabberSerialNext());
			switch ( type ){
			case PROTOTYPE_SELFTYPING_OFF:
				m.addChild( "paused" )->addAttr( "xmlns", _T(JABBER_FEAT_CHATSTATES));
				jabberThreadInfo->send( m );
				break;
			case PROTOTYPE_SELFTYPING_ON:
				m.addChild( "composing" )->addAttr( "xmlns", _T(JABBER_FEAT_CHATSTATES));
				jabberThreadInfo->send( m );
				break;
			}
		}
		else if ( jcb & JABBER_CAPS_MESSAGE_EVENTS ) {
			XmlNode* x = m.addChild( "x" ); x->addAttr( "xmlns", JABBER_FEAT_MESSAGE_EVENTS );
			if ( item->messageEventIdStr != NULL )
				x->addChild( "id", item->messageEventIdStr );

			switch ( type ){
			case PROTOTYPE_SELFTYPING_OFF:
				jabberThreadInfo->send( m );
				break;
			case PROTOTYPE_SELFTYPING_ON:
				x->addChild( "composing" );
				jabberThreadInfo->send( m );
				break;
	}	}	}

	JFreeVariant( &dbv );
	return 0;
}
