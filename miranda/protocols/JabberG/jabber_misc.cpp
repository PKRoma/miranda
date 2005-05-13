/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005     George Hazan

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

*/

#include "jabber.h"
#include "jabber_list.h"

///////////////////////////////////////////////////////////////////////////////
// JabberAddContactToRoster() - adds a contact to the roster

void JabberAddContactToRoster( const char* jid, const char* nick, const char* grpName )
{
	if ( grpName != NULL )
		JabberSend( jabberThreadInfo->s, 
			"<iq type='set'><query xmlns='jabber:iq:roster'><item name='%s' jid='%s'><group>%s</group></item></query></iq>", 
			UTF8(nick), UTF8(jid), UTF8(grpName));
	else
		JabberSend( jabberThreadInfo->s, 
			"<iq type='set'><query xmlns='jabber:iq:roster'><item name='%s' jid='%s'/></query></iq>", 
			UTF8(nick), UTF8(jid));
}

///////////////////////////////////////////////////////////////////////////////
// JabberChatDllError() - missing CHAT.DLL 

void JabberChatDllError()
{
	MessageBox( NULL, 
		JTranslate( "CHAT plugin is required for conferences. Install it before chatting" ), 
		JTranslate( "Jabber Error Message" ), MB_OK|MB_SETFOREGROUND );
}

///////////////////////////////////////////////////////////////////////////////
// JabberContactListCreateGroup()

static void JabberContactListCreateClistGroup( char* groupName )
{
	char str[33], newName[128];
	int i;
	DBVARIANT dbv;
	char* name;

	for ( i=0;;i++ ) {
		itoa( i, str, 10 );
		if ( DBGetContactSetting( NULL, "CListGroups", str, &dbv ))
			break;
		name = dbv.pszVal;
		if ( name[0]!='\0' && !strcmp( name+1, groupName )) {
			// Already exist, no need to create
			JFreeVariant( &dbv );
			return;
		}
		JFreeVariant( &dbv );
	}

	// Create new group with id = i ( str is the text representation of i )
	newName[0] = 1 | GROUPF_EXPANDED;
	strncpy( newName+1, groupName, sizeof( newName )-1 );
	newName[sizeof( newName )-1] = '\0';
	DBWriteContactSettingString( NULL, "CListGroups", str, newName );
	JCallService( MS_CLUI_GROUPADDED, i+1, 0 );
}

void JabberContactListCreateGroup( char* groupName )
{
	char name[128];
	char* p;

	if ( groupName==NULL || groupName[0]=='\0' || groupName[0]=='\\' ) return;

	strncpy( name, groupName, sizeof( name ));
	name[sizeof( name )-1] = '\0';
	for ( p=name; *p!='\0'; p++ ) {
		if ( *p == '\\' ) {
			*p = '\0';
			JabberContactListCreateClistGroup( name );
			*p = '\\';
		}
	}
	JabberContactListCreateClistGroup( name );
}

///////////////////////////////////////////////////////////////////////////////
// JabberDBAddAuthRequest()

void JabberDBAddAuthRequest( char* jid, char* nick )
{
	char* s, *p, *q;
	DBEVENTINFO dbei = {0};
	PBYTE pCurBlob;
	HANDLE hContact;

	// strip resource if present
	s = _strdup( jid );
	if (( p=strchr( s, '@' )) != NULL ) {
		if (( q=strchr( p, '/' )) != NULL )
			*q = '\0';
	}
	
	if (( hContact=JabberHContactFromJID( jid )) == NULL ) {
		hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_ADD, 0, 0 );
		JCallService( MS_PROTO_ADDTOCONTACT, ( WPARAM ) hContact, ( LPARAM )jabberProtoName );
		JSetString( hContact, "jid", s );
	}
	else DBDeleteContactSetting( hContact, jabberProtoName, "Hidden" );

	JSetString( hContact, "Nick", nick );

	//blob is: uin( DWORD ), hContact( HANDLE ), nick( ASCIIZ ), first( ASCIIZ ), last( ASCIIZ ), email( ASCIIZ ), reason( ASCIIZ )
	//blob is: 0( DWORD ), hContact( HANDLE ), nick( ASCIIZ ), ""( ASCIIZ ), ""( ASCIIZ ), email( ASCIIZ ), ""( ASCIIZ )
	dbei.cbSize = sizeof( DBEVENTINFO );
	dbei.szModule = jabberProtoName;
	dbei.timestamp = ( DWORD )time( NULL );
	dbei.flags = 0;
	dbei.eventType = EVENTTYPE_AUTHREQUEST;
	dbei.cbBlob = sizeof( DWORD )+ sizeof( HANDLE ) + strlen( nick ) + strlen( jid ) + 5;
	pCurBlob = dbei.pBlob = ( PBYTE ) malloc( dbei.cbBlob );
	*(( PDWORD ) pCurBlob ) = 0; pCurBlob += sizeof( DWORD );
	*(( PHANDLE ) pCurBlob ) = hContact; pCurBlob += sizeof( HANDLE );
	strcpy(( char* )pCurBlob, nick ); pCurBlob += strlen( nick )+1;
	*pCurBlob = '\0'; pCurBlob++;		//firstName
	*pCurBlob = '\0'; pCurBlob++;		//lastName
	strcpy(( char* )pCurBlob, jid ); pCurBlob += strlen( jid )+1;
	*pCurBlob = '\0';					//reason

	JCallService( MS_DB_EVENT_ADD, ( WPARAM ) ( HANDLE ) NULL, ( LPARAM )&dbei );
	JabberLog( "Setup DBAUTHREQUEST with nick='%s' jid='%s'", nick, jid );
}

///////////////////////////////////////////////////////////////////////////////
// JabberDBCreateContact()

HANDLE JabberDBCreateContact( char* jid, char* nick, BOOL temporary, BOOL stripResource )
{
	HANDLE hContact;
	char* s, *p, *q;
	DBVARIANT dbv;
	int len;
	char* szProto;

	if ( jid==NULL || jid[0]=='\0' )
		return NULL;

	s = _strdup( jid );
	q = NULL;
	// strip resource if present
	if (( p=strchr( s, '@' )) != NULL )
		if (( q=strchr( p, '/' )) != NULL )
			*q = '\0';
	
	if ( !stripResource && q!=NULL )	// so that resource is not converted to lowercase
		*q = '/';
	len = strlen( s );

	// We can't use JabberHContactFromJID() here because of the stripResource option
	hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
	while ( hContact != NULL ) {
		szProto = ( char* )JCallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM ) hContact, 0 );
		if ( szProto!=NULL && !strcmp( jabberProtoName, szProto )) {
			if ( !DBGetContactSetting( hContact, jabberProtoName, "jid", &dbv )) {
				p = dbv.pszVal;
				if ( p && ( int )strlen( p )>=len && ( p[len]=='\0'||p[len]=='/' ) && !strncmp( p, s, len )) {
					JFreeVariant( &dbv );
					break;
				}
				JFreeVariant( &dbv );
		}	}
		hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM ) hContact, 0 );
	}

	//if (( hContact=JabberHContactFromJID( s )) == NULL ) {
	if ( hContact == NULL ) {
		hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_ADD, 0, 0 );
		JCallService( MS_PROTO_ADDTOCONTACT, ( WPARAM ) hContact, ( LPARAM )jabberProtoName );
		JSetString( hContact, "jid", s );
		if ( nick!=NULL && nick[0]!='\0' )
			JSetString( hContact, "Nick", nick );
		if ( temporary )
			DBWriteContactSettingByte( hContact, "CList", "NotOnList", 1 );
		JabberLog( "Create Jabber contact jid=%s, nick=%s", s, nick );
	}

	free( s );

	return hContact;
}

///////////////////////////////////////////////////////////////////////////////
// JabberForkThread()

struct FORK_ARG {
	HANDLE hEvent;
	void ( __cdecl *threadcode )( void* );
	void *arg;
};

static void __cdecl forkthread_r( struct FORK_ARG *fa )
{	
	void ( *callercode )( void* ) = fa->threadcode;
	void *arg = fa->arg;
	JCallService( MS_SYSTEM_THREAD_PUSH, 0, 0 );
	SetEvent( fa->hEvent );
	__try {
		callercode( arg );
	} __finally {
		JCallService( MS_SYSTEM_THREAD_POP, 0, 0 );
	} 
	return;
}

ULONG JabberForkThread( void ( __cdecl *threadcode )( void* ), unsigned long stacksize, void *arg )
{
	struct FORK_ARG fa;
	fa.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	fa.threadcode = threadcode;
	fa.arg = arg;

	ULONG rc = _beginthread(( JABBER_THREAD_FUNC )forkthread_r, stacksize, &fa );
	if (( unsigned long ) -1L != rc )
		WaitForSingleObject( fa.hEvent, INFINITE );

	CloseHandle( fa.hEvent );
	return rc;
}

///////////////////////////////////////////////////////////////////////////////
// JabberSetServerStatus()

void JabberSetServerStatus( int iNewStatus )
{
	// change status
	int oldStatus = jabberStatus;
	switch ( iNewStatus ) {
	case ID_STATUS_ONLINE:
	case ID_STATUS_NA:
	case ID_STATUS_FREECHAT:
	case ID_STATUS_INVISIBLE:
		jabberStatus = iNewStatus;
		break;
	case ID_STATUS_AWAY:
	case ID_STATUS_ONTHEPHONE:
	case ID_STATUS_OUTTOLUNCH:
		jabberStatus = ID_STATUS_AWAY;
		break;
	case ID_STATUS_DND:
	case ID_STATUS_OCCUPIED:
		jabberStatus = ID_STATUS_DND;
		break;
	default:
		return;
	}

	// send presence update
	JabberSendPresence( jabberStatus );
	ProtoBroadcastAck( jabberProtoName, NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, ( HANDLE ) oldStatus, jabberStatus );
}
