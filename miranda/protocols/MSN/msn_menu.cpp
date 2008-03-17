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

extern LIST<void> arServices;

HANDLE msnBlockMenuItem = NULL;
HANDLE msnMenuItems[ 1 ];
HANDLE menuItemsAll[ 6 ] = { 0 };

/////////////////////////////////////////////////////////////////////////////////////////
// Block command callback function

static int MsnBlockCommand( WPARAM wParam, LPARAM lParam )
{
	if ( msnLoggedIn ) {
		char tEmail[ MSN_MAX_EMAIL_LEN ];
		MSN_GetStaticString( "e-mail", (HANDLE)wParam, tEmail, sizeof( tEmail ));

		MSN_SetWord(( HANDLE )wParam, "ApparentMode", Lists_IsInList( LIST_BL, tEmail ) ? 0 : ID_STATUS_OFFLINE );
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnGotoInbox - goes to the Inbox folder at the Hotmail.com
void MsnInvokeMyURL( bool ismail, char* url );
static int MsnGotoInbox( WPARAM, LPARAM )
{
	MsnInvokeMyURL( true, NULL );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnEditProfile - goes to the Profile section at the Hotmail.com

static int MsnEditProfile( WPARAM, LPARAM )
{
	MsnInvokeMyURL( false, NULL );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnInviteCommand - invite command callback function

static int MsnInviteCommand( WPARAM wParam, LPARAM lParam )
{
	ThreadData* tActiveThreads[ 64 ];
	int tThreads = MSN_GetActiveThreads( tActiveThreads ), tChosenThread;

	switch( tThreads ) {
	case 0:
		MessageBox(NULL, TranslateT("No active chat session is found."), TranslateT("MSN Chat"), MB_OK|MB_ICONINFORMATION);
		return 0;

	case 1:
		tChosenThread = 0;
		break;

	default:
		HMENU tMenu = ::CreatePopupMenu();

		for ( int i=0; i < tThreads; i++ ) 
		{
			if (IsChatHandle(tActiveThreads[i]->mJoinedContacts[0])) 
			{
				char sessionName[ 255 ];
				mir_snprintf( sessionName, sizeof( sessionName ), "%s %s%s",
					msnProtocolName, MSN_Translate( "Chat #" ), tActiveThreads[i]->mChatID );
				::AppendMenuA( tMenu, MF_STRING, ( UINT_PTR )( i+1 ), sessionName );
			}
			else ::AppendMenu( tMenu, MF_STRING, ( UINT_PTR )( i+1 ), MSN_GetContactNameT( *tActiveThreads[i]->mJoinedContacts ));
		}

		HWND tWindow = CreateWindow(_T("EDIT"),_T(""),0,1,1,1,1,NULL,NULL,hInst,NULL);

		POINT pt;
		::GetCursorPos ( &pt );
		tChosenThread = ::TrackPopupMenu( tMenu, TPM_NONOTIFY | TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, tWindow, NULL );
		::DestroyMenu( tMenu );
		::DestroyWindow( tWindow );
		if ( !tChosenThread )
			return 0;

		tChosenThread--;
	}

	char tEmail[ MSN_MAX_EMAIL_LEN ];
	if ( MSN_IsMeByContact(( HANDLE )wParam, tEmail )) return 0;
	if ( *tEmail ) 
	{
		for ( int j=0; j < tActiveThreads[ tChosenThread ]->mJoinedCount; j++ ) {
			// if the user is already in the chat session
			if ( tActiveThreads[ tChosenThread ]->mJoinedContacts[j] == ( HANDLE )wParam ) {
				MessageBox(NULL, TranslateT("User is already in the chat session."), TranslateT("MSN Chat"), MB_OK|MB_ICONINFORMATION);
				return 0;
		}	}

		tActiveThreads[ tChosenThread ]->sendPacket( "CAL", tEmail );

		if ( msnHaveChatDll )
			MSN_ChatStart(tActiveThreads[ tChosenThread ]);
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnRebuildContactMenu - gray or ungray the block menus according to contact's status

int MsnRebuildContactMenu( WPARAM wParam, LPARAM lParam )
{
	char szEmail[ MSN_MAX_EMAIL_LEN ];
	if ( !MSN_GetStaticString( "e-mail", ( HANDLE )wParam, szEmail, sizeof( szEmail ))) {
		CLISTMENUITEM clmi = { 0 };
		clmi.cbSize = sizeof( clmi );
		clmi.pszName = (char*)(Lists_IsInList( LIST_BL, szEmail ) ? "&Unblock" : "&Block");
		clmi.flags = CMIM_NAME;
		MSN_CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )msnBlockMenuItem, ( LPARAM )&clmi );
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnSendNetMeeting - Netmeeting callback function

static int MsnSendNetMeeting( WPARAM wParam, LPARAM lParam )
{
	if ( !msnLoggedIn ) return 0;

	HANDLE hContact = HANDLE(wParam);

	if ( MSN_IsMeByContact( hContact )) return 0;

	ThreadData* thread = MSN_GetThreadByContact( hContact );

	if ( thread == NULL ) {
		MessageBox( NULL, TranslateT( "You must be talking to start Netmeeting" ), _T("MSN Protocol"), MB_OK | MB_ICONERROR );
		return 0;
	}

	char msg[ 1024 ];

	mir_snprintf( msg, sizeof( msg ),
		"Content-Type: text/x-msmsgsinvite; charset=UTF-8\r\n\r\n"
		"Application-Name: NetMeeting\r\n"
		"Application-GUID: {44BBA842-CC51-11CF-AAFA-00AA00B6015C}\r\n"
		"Session-Protocol: SM1\r\n"
		"Invitation-Command: INVITE\r\n"
		"Invitation-Cookie: %i\r\n"
		"Session-ID: {1A879604-D1B8-11D7-9066-0003FF431510}\r\n\r\n",
		rand() << 16 | rand());

	thread->sendMessage( 'N', NULL, 1, msg, MSG_DISABLE_HDR );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	SetNicknameCommand - sets nick name

static INT_PTR CALLBACK DlgProcSetNickname(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch ( msg )
	{
		case WM_INITDIALOG:
		{
			TranslateDialogDefault( hwndDlg );
			SendMessage( hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIconEx( "main" ));
			SendMessage( GetDlgItem( hwndDlg, IDC_NICKNAME ), EM_LIMITTEXT, 129, 0 );

			DBVARIANT dbv;
			if ( !MSN_GetStringT( "Nick", NULL, &dbv )) {
				SetDlgItemText( hwndDlg, IDC_NICKNAME, dbv.ptszVal );
				MSN_FreeVariant( &dbv );
			}
			return TRUE;
		}
		case WM_COMMAND:
			switch(wParam)
			{
				case IDOK:
					if ( msnLoggedIn ) {
						TCHAR str[ 130 ];
						GetDlgItemText( hwndDlg, IDC_NICKNAME, str, SIZEOF( str ));
						MSN_SendNicknameT( str );
					}

				case IDCANCEL:
 					DestroyWindow( hwndDlg );
					break;
			}
			break;

		case WM_CLOSE:
			DestroyWindow( hwndDlg );
			break;

		case WM_DESTROY:
			ReleaseIconEx( "main" );
			break;
	}
	return FALSE;
}

static int SetNicknameUI( WPARAM wParam, LPARAM lParam )
{
	HWND hwndSetNickname = CreateDialog(hInst, MAKEINTRESOURCE( IDD_SETNICKNAME ), NULL, DlgProcSetNickname );

	SetForegroundWindow( hwndSetNickname );
	SetFocus( hwndSetNickname );
 	ShowWindow( hwndSetNickname, SW_SHOW );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnViewProfile - view a contact's profile at http://members.msn.com

static const char sttUrlPrefix[] = "http://members.msn.com/";

static int MsnViewProfile( WPARAM wParam, LPARAM lParam )
{
	char tUrl[ MSN_MAX_EMAIL_LEN + sizeof(sttUrlPrefix) ];
	strcpy( tUrl, sttUrlPrefix );

	if ( !MSN_GetStaticString( "e-mail", ( HANDLE )wParam, tUrl + sizeof(sttUrlPrefix) - 1, MSN_MAX_EMAIL_LEN ))
		MSN_CallService( MS_UTILS_OPENURL, 1, ( LPARAM )tUrl );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnViewServiceStatus - display MSN services status

static int MsnViewServiceStatus( WPARAM wParam, LPARAM lParam )
{
	MSN_CallService( MS_UTILS_OPENURL, 1, ( LPARAM )"http://messenger.msn.com/Status.aspx" );
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////
// Menus initialization

void MsnInitMenus( void )
{
	char servicefunction[ 100 ];
	strcpy( servicefunction, msnProtocolName );
	char* tDest = servicefunction + strlen( servicefunction );

	CLISTMENUITEM mi = { 0 };
	mi.cbSize = sizeof( mi );
	mi.pszService = servicefunction;
	mi.pszPopupName = msnProtocolName;

	strcpy( tDest, MS_SET_NICKNAME_UI );
	arServices.insert( CreateServiceFunction( servicefunction, SetNicknameUI ));
	mi.flags = CMIF_ICONFROMICOLIB;
	mi.popupPosition = 500085000;
	mi.position = 2000060000;
	mi.icolibItem = GetIconHandle( IDI_MSN );
	mi.pszName = LPGEN("Set &Nickname");
	msnMenuItems[ 0 ] = ( HANDLE )MSN_CallService( MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM)&mi );

	strcpy( tDest, MS_GOTO_INBOX );
	arServices.insert( CreateServiceFunction( servicefunction, MsnGotoInbox ));
	mi.position = 2000060001;
	mi.icolibItem = GetIconHandle( IDI_INBOX );
	mi.pszName = LPGEN("Display Hotmail &Inbox");
	menuItemsAll[ 0 ] = ( HANDLE )MSN_CallService( MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM)&mi );

	strcpy( tDest, MS_EDIT_PROFILE );
	arServices.insert( CreateServiceFunction( servicefunction, MsnEditProfile ));
	mi.position = 2000060002;
	mi.icolibItem = GetIconHandle( IDI_PROFILE );
	mi.pszName = LPGEN("My MSN &Space");
	menuItemsAll[ 1 ] = ( HANDLE )MSN_CallService( MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM)&mi );

	strcpy( tDest, MS_VIEW_STATUS );
	arServices.insert( CreateServiceFunction( servicefunction, MsnViewServiceStatus ));
	mi.position = 2000060003;
	mi.icolibItem = GetIconHandle( IDI_SERVICES );
	mi.pszName = LPGEN("View MSN Services &Status");
	menuItemsAll[ 2 ] = ( HANDLE )MSN_CallService( MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM)&mi );


	//////////////////////////////////////////////////////////////////////////////////////
	// Contact menu initialization

	strcpy( tDest, MSN_BLOCK );
	arServices.insert( CreateServiceFunction( servicefunction, MsnBlockCommand ));
	mi.position = -500050000;
	mi.icolibItem = GetIconHandle( IDI_MSNBLOCK );
	mi.pszContactOwner = msnProtocolName;
	mi.pszName = LPGEN("&Block");
	msnBlockMenuItem = ( HANDLE )MSN_CallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	strcpy( tDest, MSN_NETMEETING );
	arServices.insert( CreateServiceFunction( servicefunction, MsnSendNetMeeting ));
	mi.position = -500050002;
	mi.icolibItem = GetIconHandle( IDI_NETMEETING );
	mi.pszName = LPGEN("&Start Netmeeting");
	menuItemsAll[ 3 ] = ( HANDLE )MSN_CallService( MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi );

	strcpy( tDest, MSN_VIEW_PROFILE );
	arServices.insert( CreateServiceFunction( servicefunction, MsnViewProfile ));
	mi.position = -500050003;
	mi.icolibItem = GetIconHandle( IDI_PROFILE );
	mi.pszName = LPGEN("&View Profile");
	menuItemsAll[ 4 ] = ( HANDLE )MSN_CallService( MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi );

	strcpy( tDest, MSN_INVITE );
	arServices.insert( CreateServiceFunction( servicefunction, MsnInviteCommand ));
	mi.flags = CMIF_ICONFROMICOLIB | CMIF_NOTOFFLINE;
	mi.position = -500050001;
	mi.icolibItem = GetIconHandle( IDI_INVITE );
	mi.pszName = LPGEN("&Invite to chat");
	menuItemsAll[ 5 ] = ( HANDLE )MSN_CallService( MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi );

	MSN_EnableMenuItems( false );
}

void  MSN_EnableMenuItems( bool parEnable )
{
	CLISTMENUITEM clmi = { 0 };
	clmi.cbSize = sizeof( clmi );
	clmi.flags = CMIM_FLAGS;
	if ( !parEnable )
		clmi.flags |= CMIF_GRAYED;

	for ( unsigned i=0; i < SIZEOF(msnMenuItems); i++ )
		if ( msnMenuItems[i] != NULL )
			MSN_CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )msnMenuItems[i], ( LPARAM )&clmi );

	MSN_CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )msnBlockMenuItem, ( LPARAM )&clmi );
}
