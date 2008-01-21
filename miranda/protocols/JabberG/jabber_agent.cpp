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
#include <commctrl.h>
#include "resource.h"
#include "jabber_iq.h"
#include "jabber_caps.h"

static BOOL CALLBACK JabberAgentsDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam );
static BOOL CALLBACK JabberAgentRegInputDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam );
static BOOL CALLBACK JabberAgentRegDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam );
static BOOL CALLBACK JabberAgentManualRegDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam );

struct TJabberRegWndData
{
	CJabberProto* ppro;
	int curPos;
	int formHeight, frameHeight;
	RECT frameRect;
};

/*
int JabberMenuHandleAgents( WPARAM wParam, LPARAM lParam, CJabberProto* ppro )
{
	if ( IsWindow( ppro->hwndJabberAgents ))
		SetForegroundWindow( ppro->hwndJabberAgents );
	else
		CreateDialogParam( hInst, MAKEINTRESOURCE( IDD_AGENTS ), NULL, JabberAgentsDlgProc, ( LPARAM )ppro );

	return 0;
}
*/

struct JabberAgentRegInputDlgProcParam {
	CJabberProto* ppro;
	TCHAR* m_jid;
};

void CJabberProto::JabberRegisterAgent( HWND hwndDlg, TCHAR* jid )
{
	JabberAgentRegInputDlgProcParam param;
	param.ppro = this;
	param.m_jid = jid;
	CreateDialogParam( hInst, MAKEINTRESOURCE( IDD_FORM ),
		hwndDlg, JabberAgentRegInputDlgProc, ( LPARAM )&param );
}

static BOOL CALLBACK JabberAgentRegInputDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	static XmlNode *agentRegIqNode;

	int id, ypos, i;
	TCHAR *from, *str, *str2;

	TJabberRegWndData *dat = (TJabberRegWndData *)GetWindowLong(hwndDlg, GWL_USERDATA);

	switch ( msg ) {
	case WM_INITDIALOG:
	{
		JabberAgentRegInputDlgProcParam* param = (JabberAgentRegInputDlgProcParam*)lParam;
		SetWindowLong(hwndDlg, GWL_USERDATA, ( LPARAM )param->ppro );

		EnableWindow( GetParent( hwndDlg ), FALSE );
		TranslateDialogDefault( hwndDlg );
		agentRegIqNode = NULL;
		param->ppro->hwndAgentRegInput = hwndDlg;
		SetWindowText( hwndDlg, TranslateT( "Jabber Agent Registration" ));
		SetDlgItemText( hwndDlg, IDC_SUBMIT, TranslateT( "Register" ));
		SetDlgItemText( hwndDlg, IDC_FRAME_TEXT, TranslateT( "Please wait..." ));

		{	int iqId = param->ppro->JabberSerialNext();
			param->ppro->JabberIqAdd( iqId, IQ_PROC_GETREGISTER, &CJabberProto::JabberIqResultGetRegister );
			XmlNodeIq iq( "get", iqId, param->m_jid );
			XmlNode* query = iq.addQuery( JABBER_FEAT_REGISTER );
			param->ppro->jabberThreadInfo->send( iq );
		}

		// Enable WS_EX_CONTROLPARENT on IDC_FRAME ( so tab stop goes through all its children )
		LONG frameExStyle = GetWindowLong( GetDlgItem( hwndDlg, IDC_FRAME ), GWL_EXSTYLE );
		frameExStyle |= WS_EX_CONTROLPARENT;
		SetWindowLong( GetDlgItem( hwndDlg, IDC_FRAME ), GWL_EXSTYLE, frameExStyle );

		return TRUE;
	}
	case WM_CTLCOLORSTATIC:
		if ((GetWindowLong((HWND)lParam, GWL_ID) == IDC_WHITERECT) ||
			(GetWindowLong((HWND)lParam, GWL_ID) == IDC_INSTRUCTION) ||
			(GetWindowLong((HWND)lParam, GWL_ID) == IDC_TITLE))
		{
			//MessageBeep(MB_ICONSTOP);
			return (BOOL)GetStockObject(WHITE_BRUSH);
		} else
		{
			return NULL;
		}
	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDC_SUBMIT:
		{
			XmlNode *queryNode, *xNode, *n;

			if ( agentRegIqNode == NULL ) return TRUE;
			if (( from=JabberXmlGetAttrValue( agentRegIqNode, "from" )) == NULL ) return TRUE;
			if (( queryNode=JabberXmlGetChild( agentRegIqNode, "query" )) == NULL ) return TRUE;
			HWND hFrame = GetDlgItem( hwndDlg, IDC_FRAME );

			str = ( TCHAR* )alloca( sizeof(TCHAR) * 128 );
			str2 = ( TCHAR* )alloca( sizeof(TCHAR) * 128 );
			id = 0;

			int iqId = dat->ppro->JabberSerialNext();
			dat->ppro->JabberIqAdd( iqId, IQ_PROC_SETREGISTER, &CJabberProto::JabberIqResultSetRegister );

			XmlNodeIq iq( "set", iqId, from );
			XmlNode* query = iq.addQuery( JABBER_FEAT_REGISTER );

			if (( xNode=JabberXmlGetChild( queryNode, "x" )) != NULL ) {
				// use new jabber:x:data form
				query->addChild( JabberFormGetData( hFrame, xNode ));
			}
			else {
				// use old registration information form
				for ( i=0; i<queryNode->numChild; i++ ) {
					n = queryNode->child[i];
					if ( n->name ) {
						if ( !strcmp( n->name, "key" )) {
							// field that must be passed along with the registration
							if ( n->text )
								query->addChild( n->name, n->text );
							else
								query->addChild( n->name );
						}
						else if ( !strcmp( n->name, "registered" ) || !strcmp( n->name, "instructions" )) {
							// do nothing, we will skip these
						}
						else {
							GetDlgItemText( hFrame, id, str2, 128 );
							query->addChild( n->name, str2 );
							id++;
			}	}	}	}

			dat->ppro->jabberThreadInfo->send( iq );
			DialogBoxParam( hInst, MAKEINTRESOURCE( IDD_OPT_REGISTER ), hwndDlg, JabberAgentRegDlgProc, (LPARAM)dat->ppro );
			// Fall through to IDCANCEL
		}
		case IDCANCEL:
		case IDCLOSE:
			if ( agentRegIqNode )
				delete agentRegIqNode;
			DestroyWindow( hwndDlg );
			return TRUE;
		}
		break;
	case WM_JABBER_REGINPUT_ACTIVATE:
		if ( wParam == 1 ) { // success
			// lParam = <iq/> node from agent JID as a result of "get jabber:iq:register"
			HWND hFrame = GetDlgItem( hwndDlg, IDC_FRAME );
			HFONT hFont = ( HFONT ) SendMessage( hFrame, WM_GETFONT, 0, 0 );
			ShowWindow( GetDlgItem( hwndDlg, IDC_FRAME_TEXT ), SW_HIDE );

			XmlNode *queryNode, *xNode, *n;
			if (( agentRegIqNode=( XmlNode * ) lParam ) == NULL ) return TRUE;
			if (( queryNode=JabberXmlGetChild( agentRegIqNode, "query" )) == NULL ) return TRUE;
			id = 0;
			ypos = 14;

			RECT rect;
			dat = (TJabberRegWndData *)mir_alloc(sizeof(TJabberRegWndData));
			dat->curPos = 0;
			GetClientRect( GetDlgItem( hwndDlg, IDC_FRAME ), &( dat->frameRect ));
			GetClientRect( GetDlgItem( hwndDlg, IDC_VSCROLL ), &rect );
			dat->frameRect.right -= ( rect.right - rect.left );
			GetClientRect( GetDlgItem( hwndDlg, IDC_FRAME ), &rect );
			dat->frameHeight = rect.bottom - rect.top;
			SetWindowLong(hwndDlg, GWL_USERDATA, (LONG)dat);

			if (( xNode=JabberXmlGetChild( queryNode, "x" )) != NULL ) {
				// use new jabber:x:data form
				if (( n=JabberXmlGetChild( xNode, "instructions" ))!=NULL && n->text!=NULL )
					JabberFormSetInstruction( hwndDlg, n->text );

				JabberFormCreateUI( hFrame, xNode, &dat->formHeight /*dummy*/ );
			}
			else {
				// use old registration information form
				HJFORMLAYOUT layout_info = JabberFormCreateLayout(hFrame);
				for ( i=0; i<queryNode->numChild; i++ ) {
					n = queryNode->child[i];
					if ( n->name ) {
						if ( !strcmp( n->name, "instructions" )) {
							JabberFormSetInstruction( hwndDlg, n->text );
						}
						else if ( !strcmp( n->name, "key" ) || !strcmp( n->name, "registered" )) {
							// do nothing
						}
						else if ( !strcmp( n->name, "password" )) {
							TCHAR *name = mir_a2t(n->name);
							JabberFormAppendControl(hFrame, layout_info, JFORM_CTYPE_TEXT_PRIVATE, name, n->text);
							mir_free(name);
						}
						else {	// everything else is a normal text field
							TCHAR *name = mir_a2t(n->name);
							JabberFormAppendControl(hFrame, layout_info, JFORM_CTYPE_TEXT_SINGLE, name, n->text);
							mir_free(name);
				}	}	}
				JabberFormLayoutControls(hFrame, layout_info, &dat->formHeight);
				mir_free(layout_info);
			}

			if ( dat->formHeight > dat->frameHeight ) {
				HWND hwndScroll;

				hwndScroll = GetDlgItem( hwndDlg, IDC_VSCROLL );
				EnableWindow( hwndScroll, TRUE );
				SetScrollRange( hwndScroll, SB_CTL, 0, dat->formHeight - dat->frameHeight, FALSE );
				dat->curPos = 0;
			}

			EnableWindow( GetDlgItem( hwndDlg, IDC_SUBMIT ), TRUE );
		}
		else if ( wParam == 0 ) {
			// lParam = error message
			SetDlgItemText( hwndDlg, IDC_FRAME_TEXT, ( LPCTSTR ) lParam );
		}
		return TRUE;
	case WM_VSCROLL:
		{
			int pos;

			if ( dat != NULL ) {
				pos = dat->curPos;
				switch ( LOWORD( wParam )) {
				case SB_LINEDOWN:
					pos += 10;
					break;
				case SB_LINEUP:
					pos -= 10;
					break;
				case SB_PAGEDOWN:
					pos += ( dat->frameHeight - 10 );
					break;
				case SB_PAGEUP:
					pos -= ( dat->frameHeight - 10 );
					break;
				case SB_THUMBTRACK:
					pos = HIWORD( wParam );
					break;
				}
				if ( pos > ( dat->formHeight - dat->frameHeight ))
					pos = dat->formHeight - dat->frameHeight;
				if ( pos < 0 )
					pos = 0;
				if ( dat->curPos != pos ) {
					ScrollWindow( GetDlgItem( hwndDlg, IDC_FRAME ), 0, dat->curPos - pos, NULL, &( dat->frameRect ));
					SetScrollPos( GetDlgItem( hwndDlg, IDC_VSCROLL ), SB_CTL, pos, TRUE );
					dat->curPos = pos;
				}
			}
		}
		break;
	case WM_DESTROY:
		JabberFormDestroyUI(GetDlgItem(hwndDlg, IDC_FRAME));
		dat->ppro->hwndAgentRegInput = NULL;
		EnableWindow( GetParent( hwndDlg ), TRUE );
		SetActiveWindow( GetParent( hwndDlg ));
		if (dat) mir_free(dat);
		break;
	}

	return FALSE;
}

static BOOL CALLBACK JabberAgentRegDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	CJabberProto* ppro = (CJabberProto*)GetWindowLong( hwndDlg, GWL_USERDATA );
	switch ( msg ) {
	case WM_INITDIALOG:
		ppro = (CJabberProto*)lParam;
		SetWindowLong( hwndDlg, GWL_USERDATA, ( LONG )ppro );

		ppro->hwndRegProgress = hwndDlg;
		SetWindowTextA( hwndDlg, "Jabber Agent Registration" );
		TranslateDialogDefault( hwndDlg );
		ShowWindow( GetDlgItem( hwndDlg, IDOK ), SW_HIDE );
		ShowWindow( GetDlgItem( hwndDlg, IDCANCEL ), SW_HIDE );
		ShowWindow( GetDlgItem( hwndDlg, IDC_PROGRESS_REG ), SW_SHOW );
		ShowWindow( GetDlgItem( hwndDlg, IDCANCEL2 ), SW_SHOW );
		return TRUE;
	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDCANCEL2:
		case IDOK2:
			ppro->hwndRegProgress = NULL;
			EndDialog( hwndDlg, 0 );
			return TRUE;
		}
		break;
	case WM_JABBER_REGDLG_UPDATE:	// wParam=progress ( 0-100 ), lparam=status string
		if (( TCHAR* )lParam == NULL )
			SetDlgItemText( hwndDlg, IDC_REG_STATUS, TranslateT( "No message" ));
		else
			SetDlgItemText( hwndDlg, IDC_REG_STATUS, ( TCHAR* )lParam );
		if ( wParam >= 0 )
			SendMessage( GetDlgItem( hwndDlg, IDC_PROGRESS_REG ), PBM_SETPOS, wParam, 0 );
		if ( wParam >= 100 ) {
			ShowWindow( GetDlgItem( hwndDlg, IDCANCEL2 ), SW_HIDE );
			ShowWindow( GetDlgItem( hwndDlg, IDOK2 ), SW_SHOW );
			SetFocus( GetDlgItem( hwndDlg, IDOK2 ));
		}
		else
			SetFocus( GetDlgItem( hwndDlg, IDCANCEL2 ));
		return TRUE;
	}

	return FALSE;
}
