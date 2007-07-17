/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2006-7 Boris Krasnovskiy.
Copyright (c) 2003-5 George Hazan.
Copyright (c) 2002-3 Richard Hughes (original version).

Miranda IM: the free icq client for MS Windows
Copyright (C) 2000-2002 Richard Hughes, Roland Rabien & Tristan Van de Vreede

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

#include "msn_global.h"

int  Lists_NameToCode( const char *name )
{
	if ( name[2] )
		return 0;

	switch( *( PWORD )name )  {
		case 'LA': return LIST_AL;
		case 'LB': return LIST_BL;
		case 'LR': return LIST_RL;
		case 'LF': return LIST_FL;
		case 'LP': return LIST_PL;
	}

	return 0;
}

void  Lists_Wipe( void )
{
	for ( HANDLE hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
		hContact != NULL; 
		hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM )hContact, 0 )) 
	{
		if (MSN_IsMyContact( hContact ))
			MSN_DeleteSetting(hContact, "AccList");
	}
}

bool  Lists_IsInList( int list, HANDLE hContact )
{
	int i = MSN_GetDword(hContact, "AccList", 0);

	if ( list != -1 && i != 0)
		if (( i & list ) != list )
			i = 0;
	return i != 0;
}

int  Lists_GetMask( HANDLE hContact )
{
	return MSN_GetDword(hContact, "AccList", 0);
}

int  Lists_Add( int list, HANDLE hContact )
{
	int i = MSN_GetDword(hContact, "AccList", 0) | list;
	MSN_SetDword(hContact, "AccList", i);
	return i;
}

void  Lists_Remove( int list, HANDLE hContact )
{
	int i = MSN_GetDword(hContact, "AccList", 0) & ~list;
	if (i == 0) MSN_DeleteSetting(hContact, "AccList");
	else MSN_SetDword(hContact, "AccList", i);
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN Server List Manager dialog procedure

static void ResetListOptions(HWND hwndList)
{
	int i;

	SendMessage(hwndList,CLM_SETBKBITMAP,0,(LPARAM)(HBITMAP)NULL);
	SendMessage(hwndList,CLM_SETBKCOLOR,GetSysColor(COLOR_WINDOW),0);
	SendMessage(hwndList,CLM_SETGREYOUTFLAGS,0,0);
	SendMessage(hwndList,CLM_SETLEFTMARGIN,2,0);
	SendMessage(hwndList,CLM_SETINDENT,10,0);
	for(i=0;i<=FONTID_MAX;i++)
		SendMessage(hwndList,CLM_SETTEXTCOLOR,i,GetSysColor(COLOR_WINDOWTEXT));
	SetWindowLong(hwndList,GWL_STYLE,GetWindowLong(hwndList,GWL_STYLE)|CLS_SHOWHIDDEN);
}

static void SetAllContactIcons( HWND hwndList )
{
	for ( HANDLE hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
		hContact != NULL; 
		hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM )hContact, 0 )) 
	{
		HANDLE hItem = ( HANDLE )SendMessage( hwndList, CLM_FINDCONTACT, ( WPARAM )hContact, 0 );
		if ( hItem == NULL ) continue;

		if ( !MSN_IsMyContact( hContact )) {
			SendMessage( hwndList, CLM_DELETEITEM, ( WPARAM )hItem, 0 );
			continue;
		}

		DWORD dwMask = Lists_GetMask( hContact );
		if ( dwMask == 0 ) {
			SendMessage( hwndList, CLM_DELETEITEM, ( WPARAM )hItem, 0 );
			continue;
		}

		if ( SendMessage( hwndList, CLM_GETEXTRAIMAGE, ( WPARAM )hItem, MAKELPARAM(0,0)) == 0xFF )
			SendMessage( hwndList, CLM_SETEXTRAIMAGE,( WPARAM )hItem, MAKELPARAM(0,( dwMask & LIST_FL )?1:0));
		if ( SendMessage( hwndList, CLM_GETEXTRAIMAGE, ( WPARAM )hItem, MAKELPARAM(1,0)) == 0xFF )
			SendMessage( hwndList, CLM_SETEXTRAIMAGE,( WPARAM )hItem, MAKELPARAM(1,( dwMask & LIST_AL )?2:0));
		if ( SendMessage( hwndList, CLM_GETEXTRAIMAGE, ( WPARAM )hItem, MAKELPARAM(2,0)) == 0xFF )
			SendMessage( hwndList, CLM_SETEXTRAIMAGE,( WPARAM )hItem, MAKELPARAM(2,( dwMask & LIST_BL )?3:0));
		if ( SendMessage( hwndList, CLM_GETEXTRAIMAGE, ( WPARAM )hItem, MAKELPARAM(3,0)) == 0xFF )
			SendMessage( hwndList, CLM_SETEXTRAIMAGE,( WPARAM )hItem, MAKELPARAM(3,( dwMask & LIST_RL )?4:0));
	}
}

static void SaveListItem( HANDLE hContact, const char* szEmail, int list, int iPrevValue, int iNewValue )
{
	if ( iPrevValue == iNewValue )
		return;

	if ( iNewValue == 0 )
		list += LIST_REMOVE;
	MSN_AddUser( hContact, szEmail, list );
}

static void SaveSettings( HWND hwndList )
{
	for ( HANDLE hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
		hContact != NULL; 
		hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM )hContact, 0 )) 
	{
		if (!MSN_IsMyContact( hContact )) continue;

		HANDLE hItem = ( HANDLE )SendMessage( hwndList, CLM_FINDCONTACT, ( WPARAM )hContact, 0 );
		if ( hItem == NULL ) continue;

		char szEmail[ MSN_MAX_EMAIL_LEN ];
		if ( MSN_GetStaticString( "e-mail", hContact, szEmail, sizeof( szEmail ))) continue;

		DWORD dwMask = Lists_GetMask( hContact );
		if ( dwMask == 0 ) continue;

		SaveListItem( hContact, szEmail, LIST_FL, ( dwMask & LIST_FL ) != 0, SendMessage( hwndList, CLM_GETEXTRAIMAGE, ( WPARAM )hItem, MAKELPARAM(0,0)));
		SaveListItem( hContact, szEmail, LIST_AL, ( dwMask & LIST_AL ) != 0, SendMessage( hwndList, CLM_GETEXTRAIMAGE, ( WPARAM )hItem, MAKELPARAM(1,0)));
		SaveListItem( hContact, szEmail, LIST_BL, ( dwMask & LIST_BL ) != 0, SendMessage( hwndList, CLM_GETEXTRAIMAGE, ( WPARAM )hItem, MAKELPARAM(2,0)));
	}
}

BOOL CALLBACK DlgProcMsnServLists(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch ( msg ) {
	case WM_INITDIALOG:
		TranslateDialogDefault( hwndDlg );
		{	
			HIMAGELIST hIml = ImageList_Create(
				GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
				ILC_MASK | (IsWinVerXPPlus() ? ILC_COLOR32 : ILC_COLOR16 ), 5, 5 );
			
			HICON hIcon = LoadSkinnedIcon(SKINICON_OTHER_SMALLDOT);
			ImageList_AddIcon( hIml, hIcon );
			CallService( MS_SKIN2_RELEASEICON, ( WPARAM )hIcon, 0 );
			
			hIcon =  LoadIconEx( "list_fl" );
			ImageList_AddIcon( hIml, hIcon );
			SendDlgItemMessage( hwndDlg, IDC_ICON_FL, STM_SETICON, ( WPARAM )hIcon, 0 );

			hIcon =  LoadIconEx( "list_al" );
			ImageList_AddIcon( hIml, hIcon );
			SendDlgItemMessage( hwndDlg, IDC_ICON_AL, STM_SETICON, ( WPARAM )hIcon, 0 );
			
			hIcon =  LoadIconEx( "list_bl" );
			ImageList_AddIcon( hIml, hIcon );
			SendDlgItemMessage( hwndDlg, IDC_ICON_BL, STM_SETICON, ( WPARAM )hIcon, 0 );

			hIcon =  LoadIconEx( "list_rl" );
			ImageList_AddIcon( hIml, hIcon );
			SendDlgItemMessage( hwndDlg, IDC_ICON_RL, STM_SETICON, ( WPARAM )hIcon, 0 );

			SendDlgItemMessage( hwndDlg, IDC_LIST, CLM_SETEXTRAIMAGELIST, 0, (LPARAM)hIml );
		}
		SendDlgItemMessage(hwndDlg,IDC_LIST,CLM_SETEXTRACOLUMNS,4,0);

		return TRUE;

	case WM_SETFOCUS:
		SetFocus(GetDlgItem(hwndDlg,IDC_LIST));
		break;

	case WM_COMMAND:
		break;

	case WM_NOTIFY:
	{
		LPNMHDR nmc = (LPNMHDR)lParam;
		if ( nmc->idFrom == 0 && nmc->code == PSN_APPLY ) {
			SaveSettings(GetDlgItem(hwndDlg,IDC_LIST));
			break;
		}

		if ( nmc->idFrom != IDC_LIST )
			break;

		switch ( nmc->code) {
		case CLN_NEWCONTACT:
		case CLN_LISTREBUILT:
			SetAllContactIcons(nmc->hwndFrom);
			//fall through
		case CLN_OPTIONSCHANGED:
			ResetListOptions(nmc->hwndFrom);
			break;

		case NM_CLICK:
			HANDLE hItem;
			NMCLISTCONTROL *nm=(NMCLISTCONTROL*)lParam;
			DWORD hitFlags;
			int iImage;
			int itemType;

			// Make sure we have an extra column, also we can't change RL list
			if ( nm->iColumn == -1 || nm->iColumn == 3 )
				break;

			// Find clicked item
			hItem = (HANDLE)SendDlgItemMessage(hwndDlg, IDC_LIST, CLM_HITTEST, (WPARAM)&hitFlags, MAKELPARAM(nm->pt.x,nm->pt.y));
			// Nothing was clicked
			if ( hItem == NULL )
				break;

			// It was not our extended icon
			if ( !( hitFlags & CLCHT_ONITEMEXTRA ))
				break;

			// Get image in clicked column (0=none, 1=FL, 2=AL, 3=BL, 4=RL)
			iImage = SendDlgItemMessage(hwndDlg, IDC_LIST, CLM_GETEXTRAIMAGE, (WPARAM)hItem, MAKELPARAM(nm->iColumn, 0));
			if ( iImage == 0 )
				iImage = nm->iColumn + 1;
			else
				iImage = 0;

			// Get item type (contact, group, etc...)
			itemType = SendDlgItemMessage(hwndDlg, IDC_LIST, CLM_GETITEMTYPE, (WPARAM)hItem, 0);
			if ( itemType == CLCIT_CONTACT ) { // A contact
				SendDlgItemMessage(hwndDlg, IDC_LIST, CLM_SETEXTRAIMAGE, (WPARAM)hItem, MAKELPARAM(nm->iColumn, iImage));
				if (iImage && SendDlgItemMessage(hwndDlg,IDC_LIST,CLM_GETEXTRAIMAGE,(WPARAM)hItem,MAKELPARAM( 3-nm->iColumn, 0)) != 0xFF )
					if ( nm->iColumn == 1 || nm->iColumn == 2 )
						SendDlgItemMessage(hwndDlg, IDC_LIST, CLM_SETEXTRAIMAGE, (WPARAM)hItem, MAKELPARAM( 3-nm->iColumn, 0));
			}

			// Activate Apply button
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		}
		break;
	}

	case WM_DESTROY:
		HIMAGELIST hIml=(HIMAGELIST)SendDlgItemMessage(hwndDlg,IDC_LIST,CLM_GETEXTRAIMAGELIST,0,0);
		ImageList_Destroy(hIml);
		ReleaseIconEx( "list_fl" );
		ReleaseIconEx( "list_al" );
		ReleaseIconEx( "list_bl" );
		ReleaseIconEx( "list_rl" );
		break;
	}

	return FALSE;
}
