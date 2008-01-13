/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2008 Miranda ICQ/IM project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "commonheaders.h"

#include "m_protoint.h"

static HANDLE fnAddToList( int flags, PROTOSEARCHRESULT* psr )
{	return 0;
}

static HANDLE fnAddToListByEvent( int flags, int iContact, HANDLE hDbEvent )
{	return 0;
}

static int fnAuthorize( HANDLE hContact )
{	return 0;
}

static int fnAuthDeny( HANDLE hContact )
{	return 0;
}

static int fnAuthRecv( PROTORECVEVENT* evt )
{	return 0;
}

static int fnAuthRequest( const char* szMessage )
{	return 0;
}

static HANDLE fnChangeInfo( int iInfoType, void* pInfoData )
{	return 0;
}

static int fnFileAllow( HANDLE hTransfer, const char* szPath )
{	return 0;
}

static int fnFileCancel( HANDLE hTransfer )
{	return 0;
}

static int fnFileDeny( HANDLE hTransfer, const char* szReason )
{	return 0;
}

static int fnFileResume( HANDLE hTransfer, int* action, const char** szFilename )
{	return 0;
}

static int fnGetCaps( int type )
{	return 0;
}

static HICON fnGetIcon( int iconIndex )
{	return 0;
}

static int fnGetInfo( int infoType )
{	return 0;
}

static HANDLE fnSearchBasic( const char* id )
{	return 0;
}

static HANDLE fnSearchByEmail( const char* email )
{	return 0;
}

static HANDLE fnSearchByName( const char* nick, const char* firstName, const char* lastName )
{	return 0;
}

static HWND fnSearchAdvanced( HWND owner )
{	return 0;
}

static HWND fnCreateExtendedSearchUI( HWND owner )
{	return 0;
}

static int fnRecvContacts( PROTORECVEVENT* evt )
{	return 0;
}

static int fnRecvFile( PROTORECVFILE* evt )
{	return 0;
}

static int fnRecvMessage( PROTORECVEVENT* evt )
{	return 0;
}

static int fnRecvUrl( PROTORECVEVENT* evt )
{	return 0;
}

static int fnSendContacts( int flags, int nContacts, HANDLE* hContactsList )
{	return 0;
}

static int fnSendFile( const char* szDescription, char** ppszFiles )
{	return 0;
}

static int fnSendMessage( int flags, const char* msg )
{	return 0;
}

static int fnSendUrl( int flags, const char* url )
{	return 0;
}

static int fnSetApparentMode( int mode )
{	return 0;
}

static int fnSetStatus( int iNewStatus )
{	return 0;
}

static int fnGetAwayMsg( void )
{	return 0;
}

static int fnRecvAwayMsg( PROTORECVEVENT* evt )
{	return 0;
}

static int fnSendAwayMsg( HANDLE hContact, const char* msg )
{	return 0;
}

static int fnSetAwayMsg( int iStatus, const char* msg )
{	return 0;
}

static int fnUserIsTyping( HANDLE hContact, int type )
{	return 0;
}

// creates the default protocol container for compatibility with the old plugins

PROTO_INTERFACE* AddDefaultAccount( const char* szProtoName )
{
	PROTO_INTERFACE* ppi = ( PROTO_INTERFACE* )mir_alloc( sizeof( PROTO_INTERFACE ));
	if ( ppi != NULL ) {
		memset( ppi, 0, sizeof( PROTO_INTERFACE ));
		ppi->iVersion = 1;
		ppi->bOldProto = TRUE;
		ppi->szPhysName = mir_strdup( szProtoName );
		ppi->szProtoName = mir_strdup( szProtoName );
		ppi->tszUserName = mir_a2t( szProtoName );

		ppi->AddToList              = fnAddToList;
		ppi->AddToListByEvent       = fnAddToListByEvent;
		ppi->Authorize              = fnAuthorize;
		ppi->AuthDeny               = fnAuthDeny;
		ppi->AuthRecv               = fnAuthRecv;
		ppi->AuthRequest            = fnAuthRequest;
		ppi->ChangeInfo             = fnChangeInfo;
		ppi->FileAllow              = fnFileAllow;
		ppi->FileCancel             = fnFileCancel;
		ppi->FileDeny               = fnFileDeny;
		ppi->FileResume             = fnFileResume;
		ppi->GetCaps                = fnGetCaps;
		ppi->GetIcon                = fnGetIcon;
		ppi->GetInfo                = fnGetInfo;
		ppi->SearchBasic            = fnSearchBasic;
		ppi->SearchByEmail          = fnSearchByEmail;
		ppi->SearchByName           = fnSearchByName;
		ppi->SearchAdvanced         = fnSearchAdvanced;
		ppi->CreateExtendedSearchUI = fnCreateExtendedSearchUI;
		ppi->RecvContacts           = fnRecvContacts;
		ppi->RecvFile               = fnRecvFile;
		ppi->RecvMessage            = fnRecvMessage;
		ppi->RecvUrl                = fnRecvUrl;
		ppi->SendContacts           = fnSendContacts;
		ppi->SendFile               = fnSendFile;
		ppi->SendMessage            = fnSendMessage;
		ppi->SendUrl                = fnSendUrl;
		ppi->SetApparentMode        = fnSetApparentMode;
		ppi->SetStatus              = fnSetStatus;
		ppi->GetAwayMsg             = fnGetAwayMsg;
		ppi->RecvAwayMsg            = fnRecvAwayMsg;
		ppi->SendAwayMsg            = fnSendAwayMsg;
		ppi->SetAwayMsg             = fnSetAwayMsg;
		ppi->UserIsTyping           = fnUserIsTyping;
	}
	return ppi;
}
