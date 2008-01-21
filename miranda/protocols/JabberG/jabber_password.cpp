/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-07  George Hazan

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

File name      : $URL$
Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#include "jabber.h"
#include "jabber_iq.h"
#include "resource.h"
#include "jabber_caps.h"

static BOOL CALLBACK JabberChangePasswordDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam );

int JabberMenuHandleChangePassword( WPARAM wParam, LPARAM lParam, CJabberProto* ppro )
{
	if ( IsWindow( ppro->hwndJabberChangePassword ))
		SetForegroundWindow( ppro->hwndJabberChangePassword );
	else
		ppro->hwndJabberChangePassword = CreateDialogParam( hInst, MAKEINTRESOURCE( IDD_CHANGEPASSWORD ), NULL, JabberChangePasswordDlgProc, ( LPARAM )ppro );

	return 0;
}

static BOOL CALLBACK JabberChangePasswordDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	CJabberProto* ppro = (CJabberProto*)GetWindowLong( hwndDlg, GWL_USERDATA );
	switch ( msg ) {
	case WM_INITDIALOG:
		ppro = (CJabberProto*)lParam;
		SetWindowLong( hwndDlg, GWL_USERDATA, ( LONG )lParam );

		SendMessage( hwndDlg, WM_SETICON, ICON_BIG, ( LPARAM )ppro->LoadIconEx( "key" ));
		TranslateDialogDefault( hwndDlg );
		if ( ppro->jabberOnline && ppro->jabberThreadInfo!=NULL ) {
			TCHAR text[128];
			mir_sntprintf( text, SIZEOF( text ), _T("%s %s@") _T(TCHAR_STR_PARAM), TranslateT( "Set New Password for" ), ppro->jabberThreadInfo->username, ppro->jabberThreadInfo->server );
			SetWindowText( hwndDlg, text );
		}
		return TRUE;
	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDOK:
			if ( ppro->jabberOnline && ppro->jabberThreadInfo!=NULL ) {
				char newPasswd[128], text[128];
				GetDlgItemTextA( hwndDlg, IDC_NEWPASSWD, newPasswd, SIZEOF( newPasswd ));
				GetDlgItemTextA( hwndDlg, IDC_NEWPASSWD2, text, SIZEOF( text ));
				if ( strcmp( newPasswd, text )) {
					MessageBox( hwndDlg, TranslateT( "New password does not match." ), TranslateT( "Change Password" ), MB_OK|MB_ICONSTOP|MB_SETFOREGROUND );
					break;
				}
				GetDlgItemTextA( hwndDlg, IDC_OLDPASSWD, text, SIZEOF( text ));
				if ( strcmp( text, ppro->jabberThreadInfo->password )) {
					MessageBox( hwndDlg, TranslateT( "Current password is incorrect." ), TranslateT( "Change Password" ), MB_OK|MB_ICONSTOP|MB_SETFOREGROUND );
					break;
				}
				strncpy( ppro->jabberThreadInfo->newPassword, newPasswd, SIZEOF( ppro->jabberThreadInfo->newPassword ));

				int iqId = ppro->JabberSerialNext();
				ppro->JabberIqAdd( iqId, IQ_PROC_NONE, &CJabberProto::JabberIqResultSetPassword );

				XmlNodeIq iq( "set", iqId, ppro->jabberThreadInfo->server );
				XmlNode* q = iq.addQuery( JABBER_FEAT_REGISTER );
				q->addChild( "username", ppro->jabberThreadInfo->username );
				q->addChild( "password", newPasswd );
				ppro->jabberThreadInfo->send( iq );
			}
			DestroyWindow( hwndDlg );
			break;
		case IDCANCEL:
			DestroyWindow( hwndDlg );
			break;
		}
		break;
	case WM_CLOSE:
		DestroyWindow( hwndDlg );
		break;
	case WM_DESTROY:
		ppro->hwndJabberChangePassword = NULL;
		break;
	}

	return FALSE;
}
