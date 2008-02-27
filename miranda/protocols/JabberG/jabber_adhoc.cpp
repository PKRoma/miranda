/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-08  George Hazan
Copyright ( C ) 2007     Artem Shpynov

Module implements an XMPP protocol extension for reporting and executing ad-hoc, 
human-oriented commands according to XEP-0050: Ad-Hoc Commands
http://www.xmpp.org/extensions/xep-0050.html

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
#include <CommCtrl.h>
#include "jabber_iq.h"
#include "m_clui.h"
#include "jabber_caps.h"


#define ShowDlgItem( a, b, c )	 ShowWindow( GetDlgItem( a, b ), c )
#define EnableDlgItem( a, b, c ) EnableWindow( GetDlgItem( a, b ), c )

enum 
{
	JAHM_COMMANDLISTRESULT = WM_USER+1,
	JAHM_PROCESSRESULT
};

//Declarators
static HWND sttGetWindowFromIq( XmlNode *iqNode );
static BOOL CALLBACK sttDeleteChildWindowsProc( HWND hwnd, LPARAM lParam );

static BOOL CALLBACK JabberAdHoc_CommandDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam );

//implementations

// convert iqID to dialog hwnd
HWND CJabberProto::GetWindowFromIq( XmlNode *iqNode )
{
	TCHAR * id=JabberXmlGetAttrValue(iqNode, "id");
	if (_tcslen(id)>4)	return (HWND)_tcstol(id+4,NULL,10);
	return m_hwndCommandWindow;

}
// Callback to clear form content
static BOOL CALLBACK sttDeleteChildWindowsProc( HWND hwnd, LPARAM lParam )
{
	DestroyWindow( hwnd );
	return TRUE;
}

static void sttEnableControls( HWND hwndDlg, BOOL bEnable, int * controlsID )
{
	int i=0;
	while ( controlsID[i]!=0 ) 
		EnableDlgItem( hwndDlg, controlsID[i++], bEnable );
}

static void sttShowControls( HWND hwndDlg, BOOL bShow, int * controlsID )
{
	int i=0;
	while ( controlsID[i]!=0 )
		ShowDlgItem( hwndDlg, controlsID[i++], (bShow) ? SW_SHOW : SW_HIDE );
}

static void JabberAdHoc_RefreshFrameScroll(HWND hwndDlg, JabberAdHocData * dat)
{
	HWND hFrame = GetDlgItem( hwndDlg, IDC_FRAME );
	HWND hwndScroll = GetDlgItem( hwndDlg, IDC_VSCROLL );
	RECT rc;
	RECT rcScrollRc;
	GetClientRect( hFrame, &rc );
	GetClientRect( hFrame, &dat->frameRect );
	GetWindowRect( hwndScroll, &rcScrollRc );
	dat->frameRect.right-=(rcScrollRc.right-rcScrollRc.left);
	dat->frameHeight = rc.bottom-rc.top;
	if ( dat->frameHeight < dat->CurrentHeight) {
		ShowWindow( hwndScroll, SW_SHOW );
		EnableWindow( hwndScroll, TRUE );
	}
	else ShowWindow( hwndScroll, SW_HIDE );

	SetScrollRange( hwndScroll, SB_CTL, 0, dat->CurrentHeight-dat->frameHeight, FALSE );

}

//////////////////////////////////////////////////////////////////////////
// Iq handlers
// Forwards to dialog window procedure

void CJabberProto::OnIqResult_ListOfCommands( XmlNode *iqNode, void *userdata )
{
	SendMessage( GetWindowFromIq( iqNode ), JAHM_COMMANDLISTRESULT, 0, (LPARAM)iqNode );
}

void CJabberProto::OnIqResult_CommandExecution( XmlNode *iqNode, void *userdata )
{
	SendMessage( GetWindowFromIq( iqNode ), JAHM_PROCESSRESULT, (WPARAM)JabberXmlCopyNode( iqNode ), 0 );
}

int CJabberProto::AdHoc_RequestListOfCommands( TCHAR * szResponder, HWND hwndDlg )
{
	int iqId = (int)hwndDlg;
	XmlNodeIq iq( "get", iqId, szResponder );
	XmlNode* query = iq.addChild( "query" );
	query->addAttr( "xmlns", JABBER_FEAT_DISCO_ITEMS );
	query->addAttr( "node", JABBER_FEAT_COMMANDS );
	IqAdd( iqId, IQ_PROC_DISCOCOMMANDS, &CJabberProto::OnIqResult_ListOfCommands );
	m_ThreadInfo->send( iq );
	return iqId;
}

int CJabberProto::AdHoc_ExecuteCommand( HWND hwndDlg, TCHAR * jid, JabberAdHocData* dat )
{
	for ( int i = 1; i <= dat->CommandsNode->numChild; i++ )
		if ( IsDlgButtonChecked( GetDlgItem( hwndDlg, IDC_FRAME ), i )) {
			XmlNode *itemNode = JabberXmlGetNthChild( dat->CommandsNode, "item", i );
			if ( itemNode ) {
				TCHAR * node = JabberXmlGetAttrValue( itemNode, "node" );
				if ( node ) {
					TCHAR *jid2 = JabberXmlGetAttrValue( itemNode, "jid" );

					int iqId = (int)hwndDlg;
					XmlNodeIq iq( "set", iqId, jid2 );
					XmlNode* query = iq.addChild( "command" );
					query->addAttr( "xmlns", JABBER_FEAT_COMMANDS );
					query->addAttr( "node", node );
					query->addAttr( "action", _T("execute"));
					IqAdd( iqId, IQ_PROC_EXECCOMMANDS, &CJabberProto::OnIqResult_CommandExecution );
					m_ThreadInfo->send( iq );

					EnableDlgItem( hwndDlg, IDC_SUBMIT, FALSE );
					SetDlgItemText( hwndDlg, IDC_SUBMIT, TranslateT( "OK" ) );
		}	}	}
	if ( dat->CommandsNode ) delete dat->CommandsNode;
	dat->CommandsNode = NULL;
	return TRUE;
}

//Messages handlers
int CJabberProto::AdHoc_OnJAHMCommandListResult( HWND hwndDlg, XmlNode * iqNode, JabberAdHocData* dat )
{
	int nodeIdx = 0;
	TCHAR * type = JabberXmlGetAttrValue( iqNode, "type" );
	if ( !type || !_tcscmp( type, _T( "error" ) ) ) {
		// error ocured here
		TCHAR buff[255];
		TCHAR * code		= NULL;
		TCHAR * description = NULL;
		
		XmlNode* errorNode = JabberXmlGetChild( iqNode, "error" );
		if ( errorNode ) {
			code = JabberXmlGetAttrValue( errorNode, "code" );
			description = errorNode->text;
		}
		_sntprintf( buff, SIZEOF(buff), TranslateT( "Error %s %s" ), (code) ? code : _T(""), (description) ? description : _T("") );	
		JabberFormSetInstruction( hwndDlg, buff );
	} 
	else if ( !_tcscmp( type, _T("result") ) ) {	
		BOOL validResponse = FALSE;
		EnumChildWindows( GetDlgItem( hwndDlg, IDC_FRAME ), sttDeleteChildWindowsProc, 0 );
		dat->CurrentHeight = 0;
		dat->curPos = 0;
		SetScrollPos( GetDlgItem( hwndDlg, IDC_VSCROLL ), SB_CTL, 0, FALSE );
		XmlNode * queryNode = JabberXmlGetChild( iqNode, "query" );
		if ( queryNode ) {
			TCHAR * xmlns = JabberXmlGetAttrValue( queryNode, "xmlns" );
			TCHAR * node  = JabberXmlGetAttrValue( queryNode, "node" );
			if ( xmlns && node
					&& !_tcscmp( xmlns, _T( JABBER_FEAT_DISCO_ITEMS ) )
					&& !_tcscmp( node,  _T( JABBER_FEAT_COMMANDS ) ) )
				validResponse = TRUE;
		}
		if ( queryNode && queryNode->numChild > 0 && validResponse ) {
			if ( dat->CommandsNode ) delete dat->CommandsNode;
			dat->CommandsNode = JabberXmlCopyNode( queryNode);

			nodeIdx = 1;
			int ypos = 20;
			int maxChild = queryNode->numChild;
			for (nodeIdx = 1; nodeIdx <= maxChild; nodeIdx++ ) {
				XmlNode *itemNode = JabberXmlGetNthChild( queryNode, "item", nodeIdx );
				if (itemNode) {
					TCHAR *name = JabberXmlGetAttrValue( itemNode, "name" );
					if (!name) name = JabberXmlGetAttrValue( itemNode, "node" );
					ypos = AdHoc_AddCommandRadio( GetDlgItem( hwndDlg,IDC_FRAME ), TranslateTS(name), nodeIdx, ypos, (nodeIdx==1) ? 1 : 0);
					dat->CurrentHeight = ypos;
				}
				else break;
		}	}

		if (nodeIdx>1) {
			JabberFormSetInstruction( hwndDlg, TranslateT("Select Command") );				
			ShowDlgItem( hwndDlg, IDC_FRAME, SW_SHOW);
			ShowDlgItem( hwndDlg, IDC_VSCROLL, SW_SHOW);
			EnableDlgItem( hwndDlg, IDC_SUBMIT, TRUE);
		} else {
			JabberFormSetInstruction(hwndDlg, TranslateT("Not supported") );
	}	}

	JabberAdHoc_RefreshFrameScroll( hwndDlg, dat );
	return (TRUE);
}

int CJabberProto::AdHoc_OnJAHMProcessResult(HWND hwndDlg, XmlNode *workNode, JabberAdHocData* dat)
{
	EnumChildWindows( GetDlgItem( hwndDlg, IDC_FRAME ), sttDeleteChildWindowsProc, 0 );
	dat->CurrentHeight = 0;
	dat->curPos = 0;
	SetScrollPos( GetDlgItem( hwndDlg, IDC_VSCROLL ), SB_CTL, 0, FALSE );

	if ( dat->AdHocNode ) delete dat->AdHocNode;
	dat->AdHocNode = workNode;
	
	if ( dat->AdHocNode == NULL ) return TRUE;

	TCHAR *type;
	if (( type = JabberXmlGetAttrValue( workNode, "type" )) == NULL ) return TRUE;
	if ( !lstrcmp( type, _T("result") ) ) { 
		// wParam = <iq/> node from responder as a result of command execution
		int ypos;
		int id;

		XmlNode *commandNode, *xNode, *n;
		if ( ( commandNode=JabberXmlGetChild( dat->AdHocNode, "command" ) ) == NULL ) return TRUE;
		id = 0;
		ypos = 14;
		if (( xNode = JabberXmlGetChild( commandNode, "x" )) != NULL ) {
			// use jabber:x:data form
			HWND hFrame = GetDlgItem( hwndDlg, IDC_FRAME );
			HFONT hFont = ( HFONT ) SendMessage( hFrame, WM_GETFONT, 0, 0 );
			ShowWindow( GetDlgItem( hwndDlg, IDC_FRAME_TEXT ), SW_HIDE );
			if (( n=JabberXmlGetChild( xNode, "instructions" ))!=NULL && n->text!=NULL )
				JabberFormSetInstruction( hwndDlg, n->text );
			else
				JabberFormSetInstruction( hwndDlg, NULL );
			JabberFormCreateUI( hFrame, xNode, &dat->CurrentHeight );
			ShowDlgItem(  hwndDlg, IDC_FRAME , SW_SHOW);
		} 
		else {
			//NO X FORM
			HWND hFrame = GetDlgItem( hwndDlg, IDC_FRAME );

			int toHide[]={ IDC_FRAME_TEXT, IDC_FRAME, IDC_VSCROLL,   0}; 
			sttShowControls(hwndDlg, FALSE, toHide );

			TCHAR * noteText=NULL;
			XmlNode * noteNode=JabberXmlGetChild(commandNode, "note");
			if (noteNode) noteText=noteNode->text;
			JabberFormSetInstruction(hwndDlg, noteText?noteText:_T(""));

		}
		//check actions
		XmlNode * actionsNode=JabberXmlGetChild(commandNode,"actions");
		if (actionsNode!=NULL) {
			ShowDlgItem( hwndDlg, IDC_PREV, (JabberXmlGetChild(actionsNode,"prev")!=NULL) ? SW_SHOW : SW_HIDE);
			ShowDlgItem( hwndDlg, IDC_NEXT, (JabberXmlGetChild(actionsNode,"next")!=NULL) ? SW_SHOW : SW_HIDE);
			ShowDlgItem( hwndDlg, IDC_COMPLETE, (JabberXmlGetChild(actionsNode,"complete")!=NULL) ? SW_SHOW : SW_HIDE);
			ShowDlgItem( hwndDlg, IDC_SUBMIT, SW_HIDE);
			
			int toEnable[]={ IDC_PREV, IDC_NEXT, IDC_COMPLETE,   0}; 
			sttEnableControls( hwndDlg, TRUE, toEnable );
		} else 	{
			int toHide[]={ IDC_PREV, IDC_NEXT, IDC_COMPLETE,   0}; 
 			sttShowControls(hwndDlg, FALSE, toHide );

			ShowDlgItem(hwndDlg,IDC_SUBMIT,	SW_SHOW);
			EnableDlgItem(hwndDlg,IDC_SUBMIT, TRUE);
		}

		TCHAR * status=JabberXmlGetAttrValue(commandNode,"status");
		if (!status || _tcscmp(status,_T("executing"))) {
			ShowDlgItem( hwndDlg, IDC_SUBMIT, SW_HIDE);
			SetWindowText(GetDlgItem(hwndDlg,IDCANCEL), TranslateT("Done"));
	} 	} 
	else if ( !lstrcmp( type, _T("error"))) {	
		// error occurred here
		int toHide[]={ IDC_FRAME, IDC_FRAME_TEXT, IDC_VSCROLL,
						IDC_PREV, IDC_NEXT, IDC_COMPLETE, IDC_SUBMIT,  0}; 
		
		sttShowControls(hwndDlg, FALSE, toHide );
		
		TCHAR * code=NULL;
		TCHAR * description=NULL;
		TCHAR buff[255];
		XmlNode* errorNode=JabberXmlGetChild(workNode,"error");

		if ( errorNode ) {
			code=JabberXmlGetAttrValue(errorNode,"code");
			description=errorNode->text;
		}
		_sntprintf(buff,SIZEOF(buff),TranslateT("Error %s %s"),code ? code : _T(""),description?description:_T(""));	
		JabberFormSetInstruction(hwndDlg,buff);
	}
	JabberAdHoc_RefreshFrameScroll( hwndDlg, dat );
	return TRUE;
}

int CJabberProto::AdHoc_SubmitCommandForm(HWND hwndDlg, JabberAdHocData * dat, char * action)
{
	XmlNode * commandNode = JabberXmlGetChild( dat->AdHocNode, "command" );
	XmlNode * xNode		  = JabberXmlGetChild( commandNode, "x" );
	XmlNode * dataNode    = JabberFormGetData( GetDlgItem( hwndDlg, IDC_FRAME ), xNode);

	int iqId = (int)hwndDlg;
	XmlNodeIq iq( "set", iqId, JabberXmlGetAttrValue(dat->AdHocNode, "from"));
	XmlNode* command = iq.addChild( "command" );			
	command->addAttr( "xmlns", JABBER_FEAT_COMMANDS );
	
	TCHAR * sessionId = JabberXmlGetAttrValue(commandNode, "sessionid");
	if ( sessionId ) 
		command->addAttr( "sessionid", sessionId );
	
	TCHAR * node = JabberXmlGetAttrValue(commandNode, "node");
	if ( node ) 
		command->addAttr( "node", node );
	
	if ( action ) 
		command->addAttr( "action", action );
	
	command->addChild( dataNode );
	IqAdd( iqId, IQ_PROC_EXECCOMMANDS, &CJabberProto::OnIqResult_CommandExecution );
	m_ThreadInfo->send( iq );
	
	JabberFormSetInstruction(hwndDlg,TranslateT("In progress. Please Wait..."));
	
	int toDisable[]={IDC_SUBMIT, IDC_PREV, IDC_NEXT, IDC_COMPLETE, 0};
	sttEnableControls( hwndDlg, FALSE, toDisable);
	
	return TRUE;
}


int CJabberProto::AdHoc_AddCommandRadio(HWND hFrame, TCHAR * labelStr, int id, int ypos, int value)
{
	int labelHeight;
	RECT strRect={0};

	int verticalStep=4;
	int ctrlMinHeight=18;
	HWND hCtrl=NULL;

	RECT rcFrame;
	GetClientRect(hFrame,&rcFrame);

	int ctrlOffset=20;
	int ctrlWidth=rcFrame.right-ctrlOffset;

	HDC hdc = GetDC( hFrame );
	labelHeight = max(ctrlMinHeight, DrawText( hdc , labelStr, -1, &strRect, DT_CALCRECT ));
	ctrlWidth=min( ctrlWidth, strRect.right-strRect.left+20 );
	ReleaseDC( hFrame, hdc );

	hCtrl = CreateWindowEx( 0, _T("button"), labelStr, WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTORADIOBUTTON, ctrlOffset, ypos, ctrlWidth, labelHeight, hFrame, ( HMENU ) id, hInst, NULL );
	SendMessage( hCtrl, WM_SETFONT, ( WPARAM ) SendMessage( GetParent(hFrame), WM_GETFONT, 0, 0 ), 0 );
	SendMessage( hCtrl, BM_SETCHECK, value, 0 );
	return (ypos + labelHeight + verticalStep);

}

static BOOL CALLBACK JabberAdHoc_CommandDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	JabberAdHocData* dat = ( JabberAdHocData* )GetWindowLong( hwndDlg, GWL_USERDATA );
	switch (msg)
	{
	case WM_INITDIALOG:
		{
			CJabberAdhocStartupParams* pStartupParams = (CJabberAdhocStartupParams *)lParam;
			dat=(JabberAdHocData *)mir_alloc(sizeof(JabberAdHocData));
			memset(dat,0,sizeof(JabberAdHocData));
			
			//hmmm, useless code? if (dat->ResponderJID) mir_free(dat->ResponderJID);
			dat->ResponderJID = mir_tstrdup(pStartupParams->m_szJid);
			dat->proto = pStartupParams->m_pProto;

			SetWindowLong(hwndDlg,GWL_USERDATA,(LPARAM)dat);
			SendMessage( hwndDlg, WM_SETICON, ICON_BIG, ( LPARAM )dat->proto->LoadIconEx( "adhoc" ));
			dat->proto->m_hwndCommandWindow = hwndDlg;
			TranslateDialogDefault( hwndDlg );

			//Firstly hide frame
			LONG frameExStyle = GetWindowLong( GetDlgItem( hwndDlg, IDC_FRAME ), GWL_EXSTYLE );
			frameExStyle |= WS_EX_CONTROLPARENT;

			SetWindowLong( GetDlgItem( hwndDlg, IDC_FRAME ), GWL_EXSTYLE, frameExStyle );
	
			int toHide[]={ IDC_FRAME, IDC_VSCROLL, IDC_PREV, IDC_NEXT, IDC_COMPLETE, IDC_FRAME_TEXT, 0}; 
			sttShowControls(hwndDlg, FALSE, toHide );
		
			int toShow[]={ IDC_INSTRUCTION, IDC_SUBMIT, IDCANCEL, 0}; 
			sttShowControls(hwndDlg, TRUE, toShow );

			EnableDlgItem(hwndDlg,IDC_VSCROLL,TRUE);
	
			SetWindowPos(GetDlgItem(hwndDlg,IDC_VSCROLL),HWND_BOTTOM,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE);

			SetDlgItemText(hwndDlg,IDC_SUBMIT, TranslateT("Execute"));
			JabberFormSetInstruction(hwndDlg,TranslateT("Requesting command list. Please wait..."));

			if ( !pStartupParams->m_szNode ) {
				dat->proto->AdHoc_RequestListOfCommands(pStartupParams->m_szJid, hwndDlg);

				TCHAR Caption[ 512 ];
				_sntprintf(Caption, SIZEOF(Caption), _T("%s %s"), TranslateT("Jabber Ad-Hoc commands at"), dat->ResponderJID );
				SetWindowText(hwndDlg, Caption);
			}
			else
			{
				int iqId = (int)hwndDlg;
				XmlNodeIq iq( "set", iqId, pStartupParams->m_szJid );
				XmlNode* query = iq.addChild( "command" );
				query->addAttr( "xmlns", JABBER_FEAT_COMMANDS );
				query->addAttr( "node", pStartupParams->m_szNode );
				query->addAttr( "action", _T("execute"));
				dat->proto->IqAdd( iqId, IQ_PROC_EXECCOMMANDS, &CJabberProto::OnIqResult_CommandExecution );
				dat->proto->m_ThreadInfo->send( iq );

				EnableDlgItem( hwndDlg, IDC_SUBMIT, FALSE );
				SetDlgItemText( hwndDlg, IDC_SUBMIT, TranslateT( "OK" ) );

				TCHAR Caption[ 512 ];
				_sntprintf(Caption, SIZEOF(Caption), _T("%s %s"), TranslateT("Sending Ad-Hoc command to"), dat->ResponderJID );
				SetWindowText(hwndDlg, Caption);
			}

			delete pStartupParams;

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
		{	
			switch ( LOWORD( wParam )) 
			{

			case IDC_PREV:
				return dat->proto->AdHoc_SubmitCommandForm(hwndDlg,dat,"prev");
			case IDC_NEXT:
				return dat->proto->AdHoc_SubmitCommandForm(hwndDlg,dat,"next");
			case IDC_COMPLETE:
				return dat->proto->AdHoc_SubmitCommandForm(hwndDlg,dat,"complete");
			case IDC_SUBMIT:
				if (!dat->AdHocNode && dat->CommandsNode && LOWORD( wParam )==IDC_SUBMIT) 
					return dat->proto->AdHoc_ExecuteCommand(hwndDlg,dat->ResponderJID, dat);
				else
					return dat->proto->AdHoc_SubmitCommandForm(hwndDlg,dat, NULL);
			case IDCLOSE:
			case IDCANCEL:
				if ( dat->AdHocNode )
					delete dat->AdHocNode;
				dat->AdHocNode=NULL;
				DestroyWindow( hwndDlg );
				return TRUE;
			}
			break;
		}
	case JAHM_COMMANDLISTRESULT:
		return dat->proto->AdHoc_OnJAHMCommandListResult(hwndDlg,(XmlNode *)lParam,dat);
	case JAHM_PROCESSRESULT:
		return dat->proto->AdHoc_OnJAHMProcessResult(hwndDlg,(XmlNode *) wParam,dat);

	case WM_MOUSEWHEEL:
		{
			int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
			if ( zDelta ) {
				int nScrollLines=0;
				SystemParametersInfo(SPI_GETWHEELSCROLLLINES,0,(void*)&nScrollLines,0);
				for (int i=0; i<(nScrollLines+1)/2; i++)
					SendMessage(hwndDlg,WM_VSCROLL, (zDelta<0)?SB_LINEDOWN:SB_LINEUP,0);
			}	}
		return TRUE;

	case WM_VSCROLL:
		{
			int pos;
			if ( dat != NULL ) {
				pos = dat->curPos;
				switch ( LOWORD( wParam ))
				{
				case SB_LINEDOWN:
					pos += 10;
					break;
				case SB_LINEUP:
					pos -= 10;
					break;
				case SB_PAGEDOWN:
					pos += ( dat->CurrentHeight - 10 );
					break;
				case SB_PAGEUP:
					pos -= ( dat->CurrentHeight - 10 );
					break;
				case SB_THUMBTRACK:
					pos = HIWORD( wParam );
					break;
				}
				if ( pos > ( dat->CurrentHeight - dat->frameHeight ))
					pos = dat->CurrentHeight - dat->frameHeight;
				if ( pos < 0 )
					pos = 0;
				if ( dat->curPos != pos ) {
					ScrollWindow( GetDlgItem( hwndDlg, IDC_FRAME ), 0, dat->curPos - pos, NULL ,  &( dat->frameRect ));
					SetScrollPos( GetDlgItem( hwndDlg, IDC_VSCROLL ), SB_CTL, pos, TRUE );
					RECT Invalid=dat->frameRect;
					if (dat->curPos - pos >0)
						Invalid.bottom=Invalid.top+(dat->curPos - pos);
					else
						Invalid.top=Invalid.bottom+(dat->curPos - pos);

					RedrawWindow(GetDlgItem( hwndDlg, IDC_FRAME ), NULL, NULL, RDW_UPDATENOW |RDW_ALLCHILDREN);
					dat->curPos = pos;
			}	}
			break;
		}
	case WM_DESTROY:
		{
			JabberFormDestroyUI(GetDlgItem(hwndDlg, IDC_FRAME));

			dat->proto->m_hwndCommandWindow = NULL;
			if (dat->AdHocNode) delete dat->AdHocNode;
			dat->AdHocNode=NULL;
			if (dat->ResponderJID) mir_free(dat->ResponderJID);
			dat->ResponderJID = NULL;
			if (dat->CommandsNode) delete dat->CommandsNode;
			dat->CommandsNode=NULL;
			mir_free(dat);
			dat=NULL;
			SetWindowLong(hwndDlg, GWL_USERDATA, 0);
			break;
		}
	}
	return FALSE;
}

int __cdecl CJabberProto::ContactMenuRunCommands(WPARAM wParam, LPARAM lParam )
{
	HANDLE hContact;
	DBVARIANT dbv;
	int res = -1;
	JABBER_LIST_ITEM * item=NULL;
	
	if ((( hContact=( HANDLE ) wParam )!=NULL || (lParam!=0)) && m_bJabberOnline ) {
		if ( wParam && !JGetStringT( hContact, "jid", &dbv )) {
			TCHAR jid[ 512 ];
			int selected = 0;
			_tcsncpy(jid, dbv.ptszVal, SIZEOF(jid));

			ListLock();
			{
				item = ListGetItemPtr( LIST_ROSTER, jid);
				if (item)
				{
					if (item->resourceCount>1)
					{
						HMENU hMenu=CreatePopupMenu();
						for (int i=0; i<item->resourceCount; i++)
							AppendMenu(hMenu,MF_STRING,i+1, item->resource[i].resourceName);
						HWND hwndTemp=CreateWindowEx(WS_EX_TOOLWINDOW,_T("button"),_T("PopupMenuHost"),0,0,0,10,10,NULL,NULL,hInst,NULL);
						SetForegroundWindow(hwndTemp);
						POINT pt;
						GetCursorPos(&pt);
						RECT rc;
						selected=TrackPopupMenu(hMenu,TPM_RETURNCMD,pt.x,pt.y,0,hwndTemp,&rc);
						DestroyMenu(hMenu);
						DestroyWindow(hwndTemp);
					}
					else selected=1;

					if (selected>0) 
					{
						selected--;
						if (item->resource)
						{
							_tcsncat(jid,_T("/"),SIZEOF(jid));
							_tcsncat(jid,item->resource[selected].resourceName,SIZEOF(jid));
						}
						selected=1;
					}
				}
			}
			ListUnlock();

			if (!item || selected) {
				CJabberAdhocStartupParams* pStartupParams = new CJabberAdhocStartupParams( this, jid, NULL );
				CreateDialogParam( hInst, MAKEINTRESOURCE( IDD_FORM ), NULL, JabberAdHoc_CommandDlgProc, ( LPARAM )(pStartupParams) );
			}
			JFreeVariant( &dbv );
			
		}
		else if (lParam!=0)
			CreateDialogParam( hInst, MAKEINTRESOURCE( IDD_FORM ), NULL, JabberAdHoc_CommandDlgProc, lParam );
	}
	return res;
}

void CJabberProto::ContactMenuAdhocCommands( CJabberAdhocStartupParams* param )
{
	if( param )
		CreateDialogParam( hInst, MAKEINTRESOURCE( IDD_FORM ), NULL, JabberAdHoc_CommandDlgProc, (LPARAM)param );
}
