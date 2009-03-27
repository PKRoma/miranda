/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2009 Miranda ICQ/IM project,
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

INT_PTR Proto_CallContactService(WPARAM wParam,LPARAM lParam);

static int MyCallProtoService( const char *szModule, const char *szService, WPARAM wParam, LPARAM lParam )
{
	char str[MAXMODULELABELLENGTH];
	mir_snprintf( str, sizeof(str), "%s%s", szModule, szService );
	return CallService(str,wParam,lParam);
}

struct DEFAULT_PROTO_INTERFACE : public PROTO_INTERFACE
{
	HANDLE __cdecl AddToList( int flags, PROTOSEARCHRESULT* psr )
	{	return ( HANDLE )MyCallProtoService( m_szModuleName, PS_ADDTOLIST, flags, (LPARAM)psr );
	}

	HANDLE __cdecl AddToListByEvent( int flags, int iContact, HANDLE hDbEvent )
	{	return ( HANDLE )MyCallProtoService( m_szModuleName, PS_ADDTOLISTBYEVENT, MAKELONG(flags, iContact), (LPARAM)hDbEvent );
	}

	int __cdecl Authorize( HANDLE hContact )
	{	return MyCallProtoService( m_szModuleName, PS_AUTHALLOW, (WPARAM)hContact, 0 );
	}

	int __cdecl AuthDeny( HANDLE hContact, const char* szReason )
	{	return MyCallProtoService( m_szModuleName, PS_AUTHDENY, (WPARAM)hContact, (LPARAM)szReason );
	}

	int __cdecl AuthRecv( HANDLE hContact, PROTORECVEVENT* evt )
	{	CCSDATA ccs = { hContact, PSR_AUTH, 0, (LPARAM)evt };
		return Proto_CallContactService( 0, (LPARAM)&ccs );
	}

	int __cdecl AuthRequest( HANDLE hContact, const char* szMessage )
	{	CCSDATA ccs = { hContact, PSS_AUTHREQUEST, 0, (LPARAM)szMessage };
		return Proto_CallContactService( 0, (LPARAM)&ccs );
	}

	HANDLE __cdecl ChangeInfo( int iInfoType, void* pInfoData )
	{	return ( HANDLE )MyCallProtoService( m_szModuleName, PS_CHANGEINFO, iInfoType, ( LPARAM )pInfoData );
	}

	int __cdecl FileAllow( HANDLE hContact, HANDLE hTransfer, const char* szPath )
	{	CCSDATA ccs = { hContact, PSS_FILEALLOW, (WPARAM)hTransfer, (LPARAM)szPath };
		return Proto_CallContactService( 0, (LPARAM)&ccs );
	}

	int __cdecl FileCancel( HANDLE hContact, HANDLE hTransfer )
	{	CCSDATA ccs = { hContact, PSS_FILECANCEL, (WPARAM)hTransfer, 0 };
		return Proto_CallContactService( 0, (LPARAM)&ccs );
	}

	int __cdecl FileDeny( HANDLE hContact, HANDLE hTransfer, const char* szReason )
	{	CCSDATA ccs = { hContact, PSS_FILEDENY, (WPARAM)hTransfer, (LPARAM)szReason };
		return Proto_CallContactService( 0, (LPARAM)&ccs );
	}

	int __cdecl FileResume( HANDLE hTransfer, int* action, const char** szFilename )
	{	PROTOFILERESUME pfr = { *action, *szFilename };
		int res = MyCallProtoService( m_szModuleName, PS_FILERESUME, ( WPARAM )hTransfer, ( LPARAM )&pfr );
		*action = pfr.action; *szFilename = pfr.szFilename;
		return res;
	}

	DWORD __cdecl GetCaps( int type, HANDLE hContact )
	{	return MyCallProtoService( m_szModuleName, PS_GETCAPS, type, (LPARAM)hContact );
	}

	HICON __cdecl GetIcon( int iconIndex )
	{	return ( HICON )MyCallProtoService( m_szModuleName, PS_LOADICON, iconIndex, 0 );
	}

	int __cdecl GetInfo( HANDLE hContact, int flags )
	{	CCSDATA ccs = { hContact, PSS_GETINFO, flags, 0 };
		return Proto_CallContactService( 0, (LPARAM)&ccs );
	}

	HANDLE __cdecl SearchBasic( const char* id )
	{	return ( HANDLE )MyCallProtoService( m_szModuleName, PS_BASICSEARCH, 0, ( LPARAM )id );
	}

	HANDLE __cdecl SearchByEmail( const char* email )
	{	return ( HANDLE )MyCallProtoService( m_szModuleName, PS_SEARCHBYEMAIL, 0, ( LPARAM )email );
	}

	HANDLE __cdecl SearchByName( const char* nick, const char* firstName, const char* lastName )
	{	PROTOSEARCHBYNAME psn;
		psn.pszNick = ( char* )nick;
		psn.pszFirstName = ( char* )firstName;
		psn.pszLastName = ( char* )lastName;
		return ( HANDLE )MyCallProtoService( m_szModuleName, PS_SEARCHBYNAME, 0, ( LPARAM )&psn );
	}

	HWND __cdecl SearchAdvanced( HWND owner )
	{	return ( HWND )MyCallProtoService( m_szModuleName, PS_SEARCHBYADVANCED, 0, ( LPARAM )owner );
	}

	HWND __cdecl CreateExtendedSearchUI( HWND owner )
	{	return ( HWND )MyCallProtoService( m_szModuleName, PS_CREATEADVSEARCHUI, 0, ( LPARAM )owner );
	}

	int __cdecl RecvContacts( HANDLE hContact, PROTORECVEVENT* evt )
	{	CCSDATA ccs = { hContact, PSR_CONTACTS, 0, (LPARAM)evt };
		return Proto_CallContactService( 0, (LPARAM)&ccs );
	}

	int __cdecl RecvFile( HANDLE hContact, PROTORECVFILE* evt )
	{	CCSDATA ccs = { hContact, PSR_FILE, 0, (LPARAM)evt };
		return Proto_CallContactService( 0, (LPARAM)&ccs );
	}

	int __cdecl RecvMsg( HANDLE hContact, PROTORECVEVENT* evt )
	{	CCSDATA ccs = { hContact, PSR_MESSAGE, 0, (LPARAM)evt };
		return Proto_CallContactService( 0, (LPARAM)&ccs );
	}

	int __cdecl RecvUrl( HANDLE hContact, PROTORECVEVENT* evt )
	{	CCSDATA ccs = { hContact, PSR_URL, 0, (LPARAM)evt };
		return Proto_CallContactService( 0, (LPARAM)&ccs );
	}

	int __cdecl SendContacts( HANDLE hContact, int flags, int nContacts, HANDLE* hContactsList )
	{	CCSDATA ccs = { hContact, PSS_CONTACTS, MAKEWPARAM(flags,nContacts), (LPARAM)hContactsList };
		return Proto_CallContactService( 0, (LPARAM)&ccs );
	}

	int __cdecl SendFile( HANDLE hContact, const char* szDescription, char** ppszFiles )
	{	CCSDATA ccs = { hContact, PSS_FILE, (WPARAM)szDescription, (LPARAM)ppszFiles };
		return Proto_CallContactService( 0, (LPARAM)&ccs );
	}

	int __cdecl SendMsg( HANDLE hContact, int flags, const char* msg )
	{	CCSDATA ccs = { hContact, PSS_MESSAGE, flags, (LPARAM)msg };
		return Proto_CallContactService( 0, (LPARAM)&ccs );
	}

	int __cdecl SendUrl( HANDLE hContact, int flags, const char* url )
	{	CCSDATA ccs = { hContact, PSS_URL, flags, (LPARAM)url };
		return Proto_CallContactService( 0, (LPARAM)&ccs );
	}

	int __cdecl SetApparentMode( HANDLE hContact, int mode )
	{	CCSDATA ccs = { hContact, PSS_SETAPPARENTMODE, mode, 0 };
		return Proto_CallContactService( 0, (LPARAM)&ccs );
	}

	int __cdecl SetStatus( int iNewStatus )
	{	return MyCallProtoService( m_szModuleName, PS_SETSTATUS, iNewStatus, 0 );
	}

	int __cdecl GetAwayMsg( HANDLE hContact )
	{	CCSDATA ccs = { hContact, PSS_GETAWAYMSG, 0, 0 };
		return Proto_CallContactService( 0, (LPARAM)&ccs );
	}

	int __cdecl RecvAwayMsg( HANDLE hContact, int statusMode, PROTORECVEVENT* evt )
	{	CCSDATA ccs = { hContact, PSR_AWAYMSG, statusMode, (LPARAM)evt };
		return Proto_CallContactService( 0, (LPARAM)&ccs );
	}

	int __cdecl SendAwayMsg( HANDLE hContact, HANDLE hProcess, const char* msg )
	{	CCSDATA ccs = { hContact, PSS_AWAYMSG, (WPARAM)hProcess, (LPARAM)msg };
		return Proto_CallContactService( 0, (LPARAM)&ccs );
	}

	int __cdecl SetAwayMsg( int iStatus, const char* msg )
	{	return MyCallProtoService( m_szModuleName, PS_SETAWAYMSG, iStatus, ( LPARAM )msg );
	}

	int __cdecl UserIsTyping( HANDLE hContact, int type )
	{	CCSDATA ccs = { hContact, PSS_USERISTYPING, (WPARAM)hContact, type };
		return Proto_CallContactService( 0, (LPARAM)&ccs );
	}

	int __cdecl OnEvent( PROTOEVENTTYPE, WPARAM, LPARAM )
	{
		return 0;
	}
};

// creates the default protocol container for compatibility with the old plugins

PROTO_INTERFACE* AddDefaultAccount( const char* szProtoName )
{
	PROTO_INTERFACE* ppi = new DEFAULT_PROTO_INTERFACE;
	if ( ppi != NULL ) {
		ppi->m_iVersion = 1;
		ppi->m_szModuleName = mir_strdup( szProtoName );
		ppi->m_szProtoName = mir_strdup( szProtoName );
		ppi->m_tszUserName = mir_a2t( szProtoName );
	}
	return ppi;
}

int FreeDefaultAccount( PROTO_INTERFACE* ppi )
{
	mir_free( ppi->m_szModuleName );
	mir_free( ppi->m_szProtoName );
	mir_free( ppi->m_tszUserName );
	delete ppi;
	return 0;
}
