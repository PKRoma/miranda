/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-08  George Hazan

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
#include "jabber_caps.h"

/////////////////////////////////////////////////////////////////////////////////////////
// Agent registration progress dialog

class CAgentRegProgressDlg : public CJabberDlgBase
{
	CCtrlButton m_ok;

public:  
	CAgentRegProgressDlg( CJabberProto* _ppro, HWND _owner ) :
		CJabberDlgBase( _ppro, IDD_OPT_REGISTER, _owner, false ),
		m_ok( this, IDOK )
	{
		m_ok.OnClick = Callback( this, &CAgentRegProgressDlg::OnOk );
	}

	virtual void OnInitDialog()
	{
		m_proto->m_hwndRegProgress = m_hwnd;
		SetWindowTextA( m_hwnd, "Jabber Agent Registration" );
		TranslateDialogDefault( m_hwnd );
	}

	virtual BOOL DlgProc( UINT msg, WPARAM wParam, LPARAM lParam )
	{
		if ( msg == WM_JABBER_REGDLG_UPDATE ) {
			if (( TCHAR* )lParam == NULL )
				SetDlgItemText( m_hwnd, IDC_REG_STATUS, TranslateT( "No message" ));
			else
				SetDlgItemText( m_hwnd, IDC_REG_STATUS, ( TCHAR* )lParam );
			if ( wParam >= 0 )
				SendMessage( GetDlgItem( m_hwnd, IDC_PROGRESS_REG ), PBM_SETPOS, wParam, 0 );
			if ( wParam >= 100 )
				m_ok.SetText( TranslateT( "OK" ));
		}

		return CJabberDlgBase::DlgProc( msg, wParam, lParam );
	}

	void OnOk( CCtrlButton* )
	{
		m_proto->m_hwndRegProgress = NULL;
		EndDialog( m_hwnd, 0 );
	}
};

/////////////////////////////////////////////////////////////////////////////////////////
// Transport registration form

class CAgentRegDlg : public CJabberDlgBase
{
	int m_curPos;
	int m_formHeight, m_frameHeight;
	RECT m_frameRect;
	HXML m_agentRegIqNode;
	TCHAR* m_jid;

	CCtrlButton m_submit;

public:
	CAgentRegDlg( CJabberProto* _ppro, TCHAR* _jid ) :
		CJabberDlgBase( _ppro, IDD_FORM, NULL, false ),
		m_submit( this, IDC_SUBMIT ),
		m_jid( _jid )
	{
		m_submit.OnClick = Callback( this, &CAgentRegDlg::OnSubmit );
	}

	virtual void OnInitDialog()
	{
		EnableWindow( GetParent( m_hwnd ), FALSE );
		m_proto->m_hwndAgentRegInput = m_hwnd;
		SetWindowText( m_hwnd, TranslateT( "Jabber Agent Registration" ));
		SetDlgItemText( m_hwnd, IDC_SUBMIT, TranslateT( "Register" ));
		SetDlgItemText( m_hwnd, IDC_FRAME_TEXT, TranslateT( "Please wait..." ));

		int iqId = m_proto->SerialNext();
		m_proto->IqAdd( iqId, IQ_PROC_GETREGISTER, &CJabberProto::OnIqResultGetRegister );
		XmlNodeIq iq( "get", iqId, m_jid );
		iq.addQuery( _T(JABBER_FEAT_REGISTER));
		m_proto->m_ThreadInfo->send( iq );

		// Enable WS_EX_CONTROLPARENT on IDC_FRAME ( so tab stop goes through all its children )
		LONG frameExStyle = GetWindowLong( GetDlgItem( m_hwnd, IDC_FRAME ), GWL_EXSTYLE );
		frameExStyle |= WS_EX_CONTROLPARENT;
		SetWindowLong( GetDlgItem( m_hwnd, IDC_FRAME ), GWL_EXSTYLE, frameExStyle );
	}

	virtual void OnDestroy()
	{
		JabberFormDestroyUI(GetDlgItem(m_hwnd, IDC_FRAME));
		m_proto->m_hwndAgentRegInput = NULL;
		EnableWindow( GetParent( m_hwnd ), TRUE );
		SetActiveWindow( GetParent( m_hwnd ));
	}

	virtual BOOL DlgProc( UINT msg, WPARAM wParam, LPARAM lParam )
	{
		switch( msg ) {
		case WM_CTLCOLORSTATIC:
			switch( GetDlgCtrlID(( HWND )lParam )) {
			case IDC_WHITERECT: case IDC_INSTRUCTION: case IDC_TITLE:
				return (BOOL)GetStockObject(WHITE_BRUSH);
			default:
				return NULL;
			}

		case WM_JABBER_REGINPUT_ACTIVATE:
			if ( wParam == 1 ) { // success
				// lParam = <iq/> node from agent JID as a result of "get jabber:iq:register"
				HWND hFrame = GetDlgItem( m_hwnd, IDC_FRAME );
				HFONT hFont = ( HFONT ) SendMessage( hFrame, WM_GETFONT, 0, 0 );
				ShowWindow( GetDlgItem( m_hwnd, IDC_FRAME_TEXT ), SW_HIDE );

				HXML queryNode, xNode;
				if (( m_agentRegIqNode = ( HANDLE )lParam ) == NULL ) return TRUE;
				if (( queryNode = xmlGetChild( m_agentRegIqNode , "query" )) == NULL ) return TRUE;
				int id = 0, ypos = 14;

				RECT rect;
				
				m_curPos = 0;
				GetClientRect( GetDlgItem( m_hwnd, IDC_FRAME ), &( m_frameRect ));
				GetClientRect( GetDlgItem( m_hwnd, IDC_VSCROLL ), &rect );
				m_frameRect.right -= ( rect.right - rect.left );
				GetClientRect( GetDlgItem( m_hwnd, IDC_FRAME ), &rect );
				m_frameHeight = rect.bottom - rect.top;

				if (( xNode=xmlGetChild( queryNode , "x" )) != NULL ) {
					// use new jabber:x:data form
					HXML n = xmlGetChild( xNode , "instructions" );
					if ( n != NULL && xmlGetText( n )!=NULL )
						JabberFormSetInstruction( m_hwnd, xmlGetText( n ) );

					JabberFormCreateUI( hFrame, xNode, &m_formHeight /*dummy*/ );
				}
				else {
					// use old registration information form
					HJFORMLAYOUT layout_info = JabberFormCreateLayout(hFrame);
					for ( int i=0; ; i++ ) {
						HXML n = xmlGetChild( queryNode ,i);
						if ( !n )
							break;

						if ( xmlGetName( n ) ) {
							if ( !lstrcmp( xmlGetName( n ), _T("instructions"))) {
								JabberFormSetInstruction( m_hwnd, xmlGetText( n ) );
							}
							else if ( !lstrcmp( xmlGetName( n ), _T("key")) || !lstrcmp( xmlGetName( n ), _T("registered"))) {
								// do nothing
							}
							else if ( !lstrcmp( xmlGetName( n ), _T("password")))
								JabberFormAppendControl(hFrame, layout_info, JFORM_CTYPE_TEXT_PRIVATE, xmlGetName( n ), xmlGetText( n ));
							else 	// everything else is a normal text field
								JabberFormAppendControl(hFrame, layout_info, JFORM_CTYPE_TEXT_SINGLE, xmlGetName( n ), xmlGetText( n ));
					}	}
					JabberFormLayoutControls(hFrame, layout_info, &m_formHeight);
					mir_free(layout_info);
				}

				if ( m_formHeight > m_frameHeight ) {
					HWND hwndScroll;

					hwndScroll = GetDlgItem( m_hwnd, IDC_VSCROLL );
					EnableWindow( hwndScroll, TRUE );
					SetScrollRange( hwndScroll, SB_CTL, 0, m_formHeight - m_frameHeight, FALSE );
					m_curPos = 0;
				}

				EnableWindow( GetDlgItem( m_hwnd, IDC_SUBMIT ), TRUE );
			}
			else if ( wParam == 0 ) {
				// lParam = error message
				SetDlgItemText( m_hwnd, IDC_FRAME_TEXT, ( LPCTSTR ) lParam );
			}
			return TRUE;

		case WM_VSCROLL:
			int pos = m_curPos;
			switch ( LOWORD( wParam )) {
				case SB_LINEDOWN:   pos += 10;   break;
				case SB_LINEUP:     pos -= 10;   break;
				case SB_PAGEDOWN:   pos += ( m_frameHeight - 10 );  break;
				case SB_PAGEUP:     pos -= ( m_frameHeight - 10 );  break;
				case SB_THUMBTRACK: pos = HIWORD( wParam );            break;
			}
			if ( pos > ( m_formHeight - m_frameHeight ))
				pos = m_formHeight - m_frameHeight;
			if ( pos < 0 )
				pos = 0;
			if ( m_curPos != pos ) {
				ScrollWindow( GetDlgItem( m_hwnd, IDC_FRAME ), 0, m_curPos - pos, NULL, &( m_frameRect ));
				SetScrollPos( GetDlgItem( m_hwnd, IDC_VSCROLL ), SB_CTL, pos, TRUE );
				m_curPos = pos;
		}	}

		return CJabberDlgBase::DlgProc( msg, wParam, lParam );
	}

	void OnSubmit( CCtrlButton* )
	{
		HXML queryNode, xNode;
		const TCHAR *from;

		if ( m_agentRegIqNode == NULL ) return;
		if (( from = xmlGetAttrValue( m_agentRegIqNode, _T("from"))) == NULL ) return;
		if (( queryNode = xmlGetChild( m_agentRegIqNode ,  "query" )) == NULL ) return;
		HWND hFrame = GetDlgItem( m_hwnd, IDC_FRAME );

		TCHAR *str = ( TCHAR* )alloca( sizeof(TCHAR) * 128 );
		TCHAR *str2 = ( TCHAR* )alloca( sizeof(TCHAR) * 128 );
		int id = 0;

		int iqId = m_proto->SerialNext();
		m_proto->IqAdd( iqId, IQ_PROC_SETREGISTER, &CJabberProto::OnIqResultSetRegister );

		XmlNodeIq iq( "set", iqId, from );
		HXML query = iq.addQuery( _T(JABBER_FEAT_REGISTER));

		if (( xNode = xmlGetChild( queryNode , "x" )) != NULL ) {
			// use new jabber:x:data form
			xmlAddChild( query, JabberFormGetData( hFrame, xNode ));
		}
		else {
			// use old registration information form
			for ( int i=0; ; i++ ) {
				HXML n = xmlGetChild( queryNode ,i);
				if ( !n )
					break;

				if ( xmlGetName( n ) ) {
					if ( !lstrcmp( xmlGetName( n ), _T("key"))) {
						// field that must be passed along with the registration
						if ( xmlGetText( n ) )
							xmlAddChild( query, xmlGetName( n ), xmlGetText( n ) );
						else
							xmlAddChild( query, xmlGetName( n ) );
					}
					else if ( !lstrcmp( xmlGetName( n ), _T("registered")) || !lstrcmp( xmlGetName( n ), _T("instructions"))) {
						// do nothing, we will skip these
					}
					else {
						GetDlgItemText( hFrame, id, str2, 128 );
						xmlAddChild( query, xmlGetName( n ), str2 );
						id++;
		}	}	}	}

		m_proto->m_ThreadInfo->send( iq );

		CAgentRegProgressDlg( m_proto, m_hwnd ).DoModal();

		Close();
	}
};

void CJabberProto::RegisterAgent( HWND hwndDlg, TCHAR* jid )
{
	( new CAgentRegDlg( this, jid ))->Show();
}
