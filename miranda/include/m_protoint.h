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

typedef struct tagPROTO_INTERFACE
{
	int    iStatus,
          iDesiredStatus,
          iXStatus;
	char*  szPhysName;
	TCHAR* tszUserName;

	DWORD  reserved[ 40 ];

	HANDLE ( *AddToList )( int flags, PROTOSEARCHRESULT* psr );
	HANDLE ( *AddToListByEvent )( int flags, int iContact, HANDLE hDbEvent );

	int    ( *Authorize )( HANDLE hContact );
	int    ( *AuthDeny )( HANDLE hContact );
	int    ( *AuthRecv )( PROTORECVEVENT* );
	int    ( *AuthRequest )( const char* szMessage );

	HANDLE ( *ChangeInfo )( int iInfoType, void* pInfoData );

	int    ( *FileAllow )( HANDLE hTransfer, const char* szPath );
	int    ( *FileCancel )( HANDLE hTransfer );
	int    ( *FileDeny )( HANDLE hTransfer, const char* szReason );
	int    ( *FileResume )( HANDLE hTransfer, int* action, const char** szFilename );

	DWORD  ( *GetCaps )( void );
	HICON  ( *GetIcon )( int iconIndex );
	int    ( *GetInfo )( int infoType );

	HANDLE ( *SearchBasic )( const char* id );
	HANDLE ( *SearchByEmail )( const char* email );
	HANDLE ( *SearchByName )( const char* nick, const char* firstName, const char* lastName );
	HWND   ( *SearchAdvanced )( HWND owner );
	HWND   ( *CreateExtendedSearchUI )( HWND owner );

	int    ( *RecvContacts )( PROTORECVEVENT* );
	int    ( *RecvFile )( PROTORECVFILE* );
	int    ( *RecvMessage )( PROTORECVEVENT* );
	int    ( *RecvUrl )( PROTORECVEVENT* );

	int    ( *SendContacts )( int flags, int nContacts, HANDLE* hContactsList );
	int    ( *SendFile )( const char* szDescription, char** ppszFiles );
	int    ( *SendMessage )( int flags, const char* msg );
	int    ( *SendUrl )( int flags, const char* url );

	int    ( *SetApparentMode )( int mode );
	int    ( *SetStatus )( int iNewStatus );

	int    ( *GetAwayMsg )( void );
	int    ( *RecvAwayMsg )( PROTORECVEVENT* );
	int    ( *SendAwayMsg )( HANDLE hContact, const char* msg );
	int    ( *SetAwayMsg )( int iStatus, const char* msg );

	int    ( *UserIsTyping )( HANDLE hContact, int type );
}
	PROTO_INTERFACE;

#endif // M_PROTOINT_H__
