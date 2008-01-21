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

#ifndef M_PROTOINT_H__
#define M_PROTOINT_H__ 1

#ifndef __cplusplus
typedef struct tagPROTO_INTERFACE_VTBL
{
	HANDLE ( *AddToList )( struct tagPROTO_INTERFACE*, int flags, PROTOSEARCHRESULT* psr );
	HANDLE ( *AddToListByEvent )( struct tagPROTO_INTERFACE*, int flags, int iContact, HANDLE hDbEvent );

	int    ( *Authorize )( struct tagPROTO_INTERFACE*, HANDLE hContact );
	int    ( *AuthDeny )( struct tagPROTO_INTERFACE*, HANDLE hContact, const char* szReason );
	int    ( *AuthRecv )( struct tagPROTO_INTERFACE*, HANDLE hContact, PROTORECVEVENT* );
	int    ( *AuthRequest )( struct tagPROTO_INTERFACE*, HANDLE hContact, const char* szMessage );

	HANDLE ( *ChangeInfo )( struct tagPROTO_INTERFACE*, int iInfoType, void* pInfoData );

	int    ( *FileAllow )( struct tagPROTO_INTERFACE*, HANDLE hContact, HANDLE hTransfer, const char* szPath );
	int    ( *FileCancel )( struct tagPROTO_INTERFACE*, HANDLE hContact, HANDLE hTransfer );
	int    ( *FileDeny )( struct tagPROTO_INTERFACE*, HANDLE hContact, HANDLE hTransfer, const char* szReason );
	int    ( *FileResume )( struct tagPROTO_INTERFACE*, HANDLE hTransfer, int* action, const char** szFilename );

	DWORD  ( *GetCaps )( struct tagPROTO_INTERFACE*, int type );
	HICON  ( *GetIcon )( struct tagPROTO_INTERFACE*, int iconIndex );
	int    ( *GetInfo )( struct tagPROTO_INTERFACE*, HANDLE hContact, int infoType );

	HANDLE ( *SearchBasic )( struct tagPROTO_INTERFACE*, const char* id );
	HANDLE ( *SearchByEmail )( struct tagPROTO_INTERFACE*, const char* email );
	HANDLE ( *SearchByName )( struct tagPROTO_INTERFACE*, const char* nick, const char* firstName, const char* lastName );
	HWND   ( *SearchAdvanced )( struct tagPROTO_INTERFACE*, HWND owner );
	HWND   ( *CreateExtendedSearchUI )( struct tagPROTO_INTERFACE*, HWND owner );

	int    ( *RecvContacts )( struct tagPROTO_INTERFACE*, HANDLE hContact, PROTORECVEVENT* );
	int    ( *RecvFile )( struct tagPROTO_INTERFACE*, HANDLE hContact, PROTORECVFILE* );
	int    ( *RecvMsg )( struct tagPROTO_INTERFACE*, HANDLE hContact, PROTORECVEVENT* );
	int    ( *RecvUrl )( struct tagPROTO_INTERFACE*, HANDLE hContact, PROTORECVEVENT* );

	int    ( *SendContacts )( struct tagPROTO_INTERFACE*, HANDLE hContact, int flags, int nContacts, HANDLE* hContactsList );
	int    ( *SendFile )( struct tagPROTO_INTERFACE*, HANDLE hContact, const char* szDescription, char** ppszFiles );
	int    ( *SendMsg )( struct tagPROTO_INTERFACE*, HANDLE hContact, int flags, const char* msg );
	int    ( *SendUrl )( struct tagPROTO_INTERFACE*, HANDLE hContact, int flags, const char* url );

	int    ( *SetApparentMode )( struct tagPROTO_INTERFACE*, HANDLE hContact, int mode );
	int    ( *SetStatus )( struct tagPROTO_INTERFACE*, int iNewStatus );

	int    ( *GetAwayMsg )( struct tagPROTO_INTERFACE*, HANDLE hContact );
	int    ( *RecvAwayMsg )( struct tagPROTO_INTERFACE*, HANDLE hContact, int mode, PROTORECVEVENT* evt );
	int    ( *SendAwayMsg )( struct tagPROTO_INTERFACE*, HANDLE hContact, HANDLE hProcess, const char* msg );
	int    ( *SetAwayMsg )( struct tagPROTO_INTERFACE*, int iStatus, const char* msg );

	int    ( *UserIsTyping )( struct tagPROTO_INTERFACE*, HANDLE hContact, int type );
}
	PROTO_INTERFACE_VTBL;
#endif

typedef struct tagPROTO_INTERFACE
{
	#ifndef __cplusplus
		PROTO_INTERFACE_VTBL* vtbl;
	#endif

	int    iStatus,
          iDesiredStatus,
          iXStatus,
			 iVersion;
	BOOL	 bOldProto;
	TCHAR* tszUserName;
	char*  szPhysName;
	char*  szProtoName;
	char*  szModuleName;

	DWORD  reserved[ 40 ];

	#ifdef __cplusplus
	virtual	HANDLE CDECL AddToList( int flags, PROTOSEARCHRESULT* psr );
	virtual	HANDLE CDECL AddToListByEvent( int flags, int iContact, HANDLE hDbEvent );

	virtual	int    CDECL Authorize( HANDLE hContact );
	virtual	int    CDECL AuthDeny( HANDLE hContact, const char* szReason );
	virtual	int    CDECL AuthRecv( HANDLE hContact, PROTORECVEVENT* );
	virtual	int    CDECL AuthRequest( HANDLE hContact, const char* szMessage );

	virtual	HANDLE CDECL ChangeInfo( int iInfoType, void* pInfoData );

	virtual	int    CDECL FileAllow( HANDLE hContact, HANDLE hTransfer, const char* szPath );
	virtual	int    CDECL FileCancel( HANDLE hContact, HANDLE hTransfer );
	virtual	int    CDECL FileDeny( HANDLE hContact, HANDLE hTransfer, const char* szReason );
	virtual	int    CDECL FileResume( HANDLE hTransfer, int* action, const char** szFilename );

	virtual	DWORD  CDECL GetCaps( int type );
	virtual	HICON  CDECL GetIcon( int iconIndex );
	virtual	int    CDECL GetInfo( HANDLE hContact, int infoType );

	virtual	HANDLE CDECL SearchBasic( const char* id );
	virtual	HANDLE CDECL SearchByEmail( const char* email );
	virtual	HANDLE CDECL SearchByName( const char* nick, const char* firstName, const char* lastName );
	virtual	HWND   CDECL SearchAdvanced( HWND owner );
	virtual	HWND   CDECL CreateExtendedSearchUI( HWND owner );

	virtual	int    CDECL RecvContacts( HANDLE hContact, PROTORECVEVENT* );
	virtual	int    CDECL RecvFile( HANDLE hContact, PROTORECVFILE* );
	virtual	int    CDECL RecvMsg( HANDLE hContact, PROTORECVEVENT* );
	virtual	int    CDECL RecvUrl( HANDLE hContact, PROTORECVEVENT* );

	virtual	int    CDECL SendContacts( HANDLE hContact, int flags, int nContacts, HANDLE* hContactsList );
	virtual	int    CDECL SendFile( HANDLE hContact, const char* szDescription, char** ppszFiles );
	virtual	int    CDECL SendMsg( HANDLE hContact, int flags, const char* msg );
	virtual	int    CDECL SendUrl( HANDLE hContact, int flags, const char* url );

	virtual	int    CDECL SetApparentMode( HANDLE hContact, int mode );
	virtual	int    CDECL SetStatus( int iNewStatus );

	virtual	int    CDECL GetAwayMsg( HANDLE hContact );
	virtual	int    CDECL RecvAwayMsg( HANDLE hContact, int mode, PROTORECVEVENT* evt );
	virtual	int    CDECL SendAwayMsg( HANDLE hContact, HANDLE hProcess, const char* msg );
	virtual	int    CDECL SetAwayMsg( int iStatus, const char* msg );

	virtual	int    CDECL UserIsTyping( HANDLE hContact, int type );
	#endif
}
	PROTO_INTERFACE;

#endif // M_PROTOINT_H__
