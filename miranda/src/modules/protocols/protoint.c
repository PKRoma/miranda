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

int Proto_CallContactService(WPARAM wParam,LPARAM lParam);

static int MyCallProtoService( const char *szModule, const char *szService, WPARAM wParam, LPARAM lParam )
{
	char str[MAXMODULELABELLENGTH];
	mir_snprintf( str, sizeof(str), "%s%s", szModule, szService );
	return CallService(str,wParam,lParam);
}

static HANDLE fnAddToList( PROTO_INTERFACE* ppi, int flags, PROTOSEARCHRESULT* psr )
{	return ( HANDLE )MyCallProtoService( ppi->m_szModuleName, PS_ADDTOLIST, flags, (LPARAM)psr );
}

static HANDLE fnAddToListByEvent( PROTO_INTERFACE* ppi, int flags, int iContact, HANDLE hDbEvent )
{	return ( HANDLE )MyCallProtoService( ppi->m_szModuleName, PS_ADDTOLISTBYEVENT, MAKELONG(flags, iContact), (LPARAM)hDbEvent );
}

static int fnAuthorize( PROTO_INTERFACE* ppi, HANDLE hContact )
{	return MyCallProtoService( ppi->m_szModuleName, PS_AUTHALLOW, (WPARAM)hContact, 0 );
}

static int fnAuthDeny( PROTO_INTERFACE* ppi, HANDLE hContact, const char* szReason )
{	return MyCallProtoService( ppi->m_szModuleName, PS_AUTHDENY, (WPARAM)hContact, (LPARAM)szReason );
}

static int fnAuthRecv( PROTO_INTERFACE* ppi, HANDLE hContact, PROTORECVEVENT* evt )
{	CCSDATA ccs = { hContact, PSS_ADDED, 0, (LPARAM)evt };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnAuthRequest( PROTO_INTERFACE* ppi, HANDLE hContact, const char* szMessage )
{	CCSDATA ccs = { hContact, PSS_AUTHREQUEST, 0, (LPARAM)szMessage };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static HANDLE fnChangeInfo( PROTO_INTERFACE* ppi, int iInfoType, void* pInfoData )
{	return ( HANDLE )MyCallProtoService( ppi->m_szModuleName, PS_CHANGEINFO, iInfoType, ( LPARAM )pInfoData );
}

static int fnFileAllow( PROTO_INTERFACE* ppi, HANDLE hContact, HANDLE hTransfer, const char* szPath )
{	CCSDATA ccs = { hContact, PSS_FILEALLOW, (WPARAM)hTransfer, (LPARAM)szPath };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnFileCancel( PROTO_INTERFACE* ppi, HANDLE hContact, HANDLE hTransfer )
{	CCSDATA ccs = { hContact, PSS_FILECANCEL, (WPARAM)hTransfer, 0 };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnFileDeny( PROTO_INTERFACE* ppi, HANDLE hContact, HANDLE hTransfer, const char* szReason )
{	CCSDATA ccs = { hContact, PSS_FILEDENY, (WPARAM)hTransfer, (LPARAM)szReason };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnFileResume( PROTO_INTERFACE* ppi, HANDLE hTransfer, int* action, const char** szFilename )
{	PROTOFILERESUME pfr = { *action, *szFilename };
	int res = MyCallProtoService( ppi->m_szModuleName, PS_FILERESUME, ( WPARAM )hTransfer, ( LPARAM )&pfr );
	*action = pfr.action; *szFilename = pfr.szFilename;
	return res;
}

static int fnGetCaps( PROTO_INTERFACE* ppi, int type )
{	return MyCallProtoService( ppi->m_szModuleName, PS_GETCAPS, type, 0 );
}

static HICON fnGetIcon( PROTO_INTERFACE* ppi, int iconIndex )
{	return ( HICON )MyCallProtoService( ppi->m_szModuleName, PS_LOADICON, iconIndex, 0 );
}

static int fnGetInfo( PROTO_INTERFACE* ppi, HANDLE hContact, int flags )
{	CCSDATA ccs = { hContact, PSS_GETINFO, flags, 0 };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static HANDLE fnSearchBasic( PROTO_INTERFACE* ppi, const char* id )
{	return ( HANDLE )MyCallProtoService( ppi->m_szModuleName, PS_BASICSEARCH, 0, ( LPARAM )id );
}

static HANDLE fnSearchByEmail( PROTO_INTERFACE* ppi, const char* email )
{	return ( HANDLE )MyCallProtoService( ppi->m_szModuleName, PS_SEARCHBYEMAIL, 0, ( LPARAM )email );
}

static HANDLE fnSearchByName( PROTO_INTERFACE* ppi, const char* nick, const char* firstName, const char* lastName )
{	PROTOSEARCHBYNAME psn;
	psn.pszNick = ( char* )nick;
	psn.pszFirstName = ( char* )firstName;
	psn.pszLastName = ( char* )lastName;
	return ( HANDLE )MyCallProtoService( ppi->m_szModuleName, PS_SEARCHBYNAME, 0, ( LPARAM )&psn );
}

static HWND fnSearchAdvanced( PROTO_INTERFACE* ppi, HWND owner )
{	return ( HWND )MyCallProtoService( ppi->m_szModuleName, PS_SEARCHBYADVANCED, 0, ( LPARAM )owner );
}

static HWND fnCreateExtendedSearchUI( PROTO_INTERFACE* ppi, HWND owner )
{	return ( HWND )MyCallProtoService( ppi->m_szModuleName, PS_CREATEADVSEARCHUI, 0, ( LPARAM )owner );
}

static int fnRecvContacts( PROTO_INTERFACE* ppi, HANDLE hContact, PROTORECVEVENT* evt )
{	CCSDATA ccs = { hContact, PSR_CONTACTS, 0, (LPARAM)evt };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnRecvFile( PROTO_INTERFACE* ppi, HANDLE hContact, PROTORECVFILE* evt )
{	CCSDATA ccs = { hContact, PSR_FILE, 0, (LPARAM)evt };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnRecvMessage( PROTO_INTERFACE* ppi, HANDLE hContact, PROTORECVEVENT* evt )
{	CCSDATA ccs = { hContact, PSR_MESSAGE, 0, (LPARAM)evt };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnRecvUrl( PROTO_INTERFACE* ppi, HANDLE hContact, PROTORECVEVENT* evt )
{	CCSDATA ccs = { hContact, PSR_URL, 0, (LPARAM)evt };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnSendContacts( PROTO_INTERFACE* ppi, HANDLE hContact, int flags, int nContacts, HANDLE* hContactsList )
{	CCSDATA ccs = { hContact, PSS_CONTACTS, MAKEWPARAM(flags,nContacts), (LPARAM)hContactsList };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnSendFile( PROTO_INTERFACE* ppi, HANDLE hContact, const char* szDescription, char** ppszFiles )
{	CCSDATA ccs = { hContact, PSS_FILE, (WPARAM)szDescription, (LPARAM)ppszFiles };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnSendMessage( PROTO_INTERFACE* ppi, HANDLE hContact, int flags, const char* msg )
{	CCSDATA ccs = { hContact, PSS_MESSAGE, flags, (LPARAM)msg };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnSendUrl( PROTO_INTERFACE* ppi, HANDLE hContact, int flags, const char* url )
{	CCSDATA ccs = { hContact, PSS_URL, flags, (LPARAM)url };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnSetApparentMode( PROTO_INTERFACE* ppi, HANDLE hContact, int mode )
{	CCSDATA ccs = { hContact, PSS_SETAPPARENTMODE, mode, 0 };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnSetStatus( PROTO_INTERFACE* ppi, int iNewStatus )
{	return MyCallProtoService( ppi->m_szModuleName, PS_SETSTATUS, iNewStatus, 0 );
}

static int fnGetAwayMsg( PROTO_INTERFACE* ppi, HANDLE hContact )
{	CCSDATA ccs = { hContact, PSS_GETAWAYMSG, 0, 0 };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnRecvAwayMsg( PROTO_INTERFACE* ppi, HANDLE hContact, int statusMode, PROTORECVEVENT* evt )
{	CCSDATA ccs = { hContact, PSR_AWAYMSG, statusMode, (LPARAM)evt };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnSendAwayMsg( PROTO_INTERFACE* ppi, HANDLE hContact, HANDLE hProcess, const char* msg )
{	CCSDATA ccs = { hContact, PSS_AWAYMSG, (WPARAM)hProcess, (LPARAM)msg };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnSetAwayMsg( PROTO_INTERFACE* ppi, int iStatus, const char* msg )
{	return MyCallProtoService( ppi->m_szModuleName, PS_SETAWAYMSG, iStatus, ( LPARAM )msg );
}

static int fnUserIsTyping( PROTO_INTERFACE* ppi, HANDLE hContact, int type )
{	CCSDATA ccs = { hContact, PSS_USERISTYPING, (WPARAM)hContact, type };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

// creates the default protocol container for compatibility with the old plugins

static PROTO_INTERFACE_VTBL defaultVtbl;

PROTO_INTERFACE* AddDefaultAccount( const char* szProtoName )
{
	PROTO_INTERFACE* ppi = ( PROTO_INTERFACE* )mir_calloc( sizeof( PROTO_INTERFACE ));
	if ( ppi != NULL ) {
		ppi->m_iVersion = 1;
		ppi->m_szModuleName = mir_strdup( szProtoName );
		ppi->m_szProtoName = mir_strdup( szProtoName );
		ppi->m_tszUserName = mir_a2t( szProtoName );

		ppi->vtbl = &defaultVtbl;
		ppi->vtbl->AddToList              = fnAddToList;
		ppi->vtbl->AddToListByEvent       = fnAddToListByEvent;
		ppi->vtbl->Authorize              = fnAuthorize;
		ppi->vtbl->AuthDeny               = fnAuthDeny;
		ppi->vtbl->AuthRecv               = fnAuthRecv;
		ppi->vtbl->AuthRequest            = fnAuthRequest;
		ppi->vtbl->ChangeInfo             = fnChangeInfo;
		ppi->vtbl->FileAllow              = fnFileAllow;
		ppi->vtbl->FileCancel             = fnFileCancel;
		ppi->vtbl->FileDeny               = fnFileDeny;
		ppi->vtbl->FileResume             = fnFileResume;
		ppi->vtbl->GetCaps                = fnGetCaps;
		ppi->vtbl->GetIcon                = fnGetIcon;
		ppi->vtbl->GetInfo                = fnGetInfo;
		ppi->vtbl->SearchBasic            = fnSearchBasic;
		ppi->vtbl->SearchByEmail          = fnSearchByEmail;
		ppi->vtbl->SearchByName           = fnSearchByName;
		ppi->vtbl->SearchAdvanced         = fnSearchAdvanced;
		ppi->vtbl->CreateExtendedSearchUI = fnCreateExtendedSearchUI;
		ppi->vtbl->RecvContacts           = fnRecvContacts;
		ppi->vtbl->RecvFile               = fnRecvFile;
		ppi->vtbl->RecvMsg                = fnRecvMessage;
		ppi->vtbl->RecvUrl                = fnRecvUrl;
		ppi->vtbl->SendContacts           = fnSendContacts;
		ppi->vtbl->SendFile               = fnSendFile;
		ppi->vtbl->SendMsg                = fnSendMessage;
		ppi->vtbl->SendUrl                = fnSendUrl;
		ppi->vtbl->SetApparentMode        = fnSetApparentMode;
		ppi->vtbl->SetStatus              = fnSetStatus;
		ppi->vtbl->GetAwayMsg             = fnGetAwayMsg;
		ppi->vtbl->RecvAwayMsg            = fnRecvAwayMsg;
		ppi->vtbl->SendAwayMsg            = fnSendAwayMsg;
		ppi->vtbl->SetAwayMsg             = fnSetAwayMsg;
		ppi->vtbl->UserIsTyping           = fnUserIsTyping;
	}
	return ppi;
}

int FreeDefaultAccount( PROTO_INTERFACE* ppi )
{
	mir_free( ppi->m_szModuleName );
	mir_free( ppi->m_szProtoName );
	mir_free( ppi->m_tszUserName );
	mir_free( ppi );
	return 0;
}
