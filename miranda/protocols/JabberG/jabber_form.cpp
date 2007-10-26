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

File name      : $Source: /cvsroot/miranda/miranda/protocols/JabberG/jabber_form.cpp,v $
Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#include "jabber.h"
#include "resource.h"
#include "jabber_caps.h"


static BOOL CALLBACK JabberFormMultiLineWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg ) {
	//case WM_GETDLGCODE:
	//	return DLGC_WANTARROWS|DLGC_WANTCHARS|DLGC_HASSETSEL|DLGC_WANTALLKEYS;
	case WM_KEYDOWN:
		if ( wParam == VK_TAB ) {
			SetFocus( GetNextDlgTabItem( GetParent( GetParent( hwnd )), hwnd, GetKeyState( VK_SHIFT )<0?TRUE:FALSE ));
			return TRUE;
		};
		break;
	}
	return CallWindowProc(( WNDPROC ) GetWindowLong( hwnd, GWL_USERDATA ), hwnd, msg, wParam, lParam );
}

struct TJabberFormControlInfo
{
	TJabberFormControlType type;
	SIZE szBlock;
	POINT ptLabel, ptCtrl;
	HWND hLabel, hCtrl;
};
typedef LIST<TJabberFormControlInfo> TJabberFormControlList;

struct TJabberFormLayoutInfo
{
	int ctrlHeight;
	int offset, width, maxLabelWidth;
	int y_pos, y_spacing;
	int id;
	bool compact;
};

void JabberFormCenterContent(HWND hwndStatic)
{
	RECT rcWindow;
	int minX;
	GetWindowRect(hwndStatic,&rcWindow);
	minX=rcWindow.right;
	HWND oldChild=NULL;
	HWND hWndChild=GetWindow(hwndStatic,GW_CHILD);
	while (hWndChild!=oldChild && hWndChild!=NULL)
	{
	   DWORD style=GetWindowLong(hWndChild, GWL_STYLE);
	   RECT rc;
	   GetWindowRect(hWndChild,&rc);
	   if ((style&SS_RIGHT) && !(style&WS_TABSTOP))
	   {
		  TCHAR * text;
		  RECT calcRect=rc;
		  int len=GetWindowTextLength(hWndChild);
		  text=(TCHAR*)malloc(sizeof(TCHAR)*(len+1));
		  GetWindowText(hWndChild,text,len+1);
		  HDC hdc=GetDC(hWndChild);
		  HFONT hfntSave = (HFONT)SelectObject(hdc, (HFONT)SendMessage(hWndChild, WM_GETFONT, 0, 0));
		  DrawText(hdc,text,-1,&calcRect,DT_CALCRECT|DT_WORDBREAK);
		  minX=min(minX, rc.right-(calcRect.right-calcRect.left));
		  SelectObject(hdc, hfntSave);
		  ReleaseDC(hWndChild,hdc);
	   }
	   else
	   {
		  minX=min(minX,rc.left);
	   }
	   oldChild=hWndChild;
	   hWndChild=GetWindow(hWndChild,GW_HWNDNEXT);
	}
	if (minX>rcWindow.left+5)
	{
		int dx=(minX-rcWindow.left)/2;
		oldChild=NULL;
		hWndChild=GetWindow(hwndStatic,GW_CHILD);
		while (hWndChild!=oldChild && hWndChild!=NULL )
		{
			DWORD style=GetWindowLong(hWndChild, GWL_STYLE);
			RECT rc;
			GetWindowRect(hWndChild,&rc);
			if ((style&SS_RIGHT) && !(style&WS_TABSTOP))
				MoveWindow(hWndChild,rc.left-rcWindow.left,rc.top-rcWindow.top, rc.right-rc.left-dx, rc.bottom-rc.top, TRUE);
			else
				MoveWindow(hWndChild,rc.left-dx-rcWindow.left,rc.top-rcWindow.top, rc.right-rc.left, rc.bottom-rc.top, TRUE);
			oldChild=hWndChild;
			hWndChild=GetWindow(hWndChild,GW_HWNDNEXT);
		}
	}
}

void JabberFormSetInstruction( HWND hwndForm, TCHAR *text )
{
	if (!text) text = _T("");

	int len = lstrlen(text);
	int fixedLen = len;
	for (int i = 1; i < len; ++i)
		if ((text[i] == _T('\n')) && (text[i] != _T('\r')))
			++fixedLen;
	char *fixedText = NULL;
	if (fixedLen != len) {
		TCHAR *fixedText = (TCHAR *)mir_alloc(sizeof(TCHAR) * (fixedLen+1));
		TCHAR *p = fixedText;
		for (int i = 0; i < len; ++i) {
			*p = text[i];
			if (i && (text[i] == _T('\n')) && (text[i] != _T('\r'))) {
				*p++ = _T('\r');
				*p = _T('\n');
			}
			++p;
		}
		*p = 0;
		text = fixedText;
	}

	SetDlgItemText( hwndForm, IDC_INSTRUCTION, text );

	RECT rcText;
	GetWindowRect(GetDlgItem(hwndForm, IDC_INSTRUCTION), &rcText);
	int oldWidth = rcText.right-rcText.left;
	int deltaHeight = -(rcText.bottom-rcText.top);

	SetRect(&rcText, 0, 0, rcText.right-rcText.left, 0);
	HDC hdcEdit = GetDC(GetDlgItem(hwndForm, IDC_INSTRUCTION));
	HFONT hfntSave = (HFONT)SelectObject(hdcEdit, (HFONT)SendDlgItemMessage(hwndForm, IDC_INSTRUCTION, WM_GETFONT, 0, 0));
	DrawTextEx(hdcEdit, text, lstrlen(text), &rcText,
		DT_CALCRECT|DT_EDITCONTROL|DT_TOP|DT_WORDBREAK, NULL);
	SelectObject(hdcEdit, hfntSave);
	ReleaseDC(GetDlgItem(hwndForm, IDC_INSTRUCTION), hdcEdit);

	RECT rcWindow; GetClientRect(hwndForm, &rcWindow);
	if (rcText.bottom-rcText.top > (rcWindow.bottom-rcWindow.top)/5) {
		HWND hwndEdit = GetDlgItem(hwndForm, IDC_INSTRUCTION);
		SetWindowLong(hwndEdit, GWL_STYLE, WS_VSCROLL | GetWindowLong(hwndEdit, GWL_STYLE));
		rcText.bottom = rcText.top + (rcWindow.bottom-rcWindow.top)/5;
	} else {
		HWND hwndEdit = GetDlgItem(hwndForm, IDC_INSTRUCTION);
		SetWindowLong(hwndEdit, GWL_STYLE, ~WS_VSCROLL & GetWindowLong(hwndEdit, GWL_STYLE));
	}
	deltaHeight += rcText.bottom-rcText.top;

	SetWindowPos(GetDlgItem(hwndForm, IDC_INSTRUCTION), 0, 0, 0,
		oldWidth,
		rcText.bottom-rcText.top,
		SWP_NOMOVE|SWP_NOZORDER);

	GetWindowRect(GetDlgItem(hwndForm, IDC_WHITERECT), &rcText);
	MapWindowPoints(NULL, hwndForm, (LPPOINT)&rcText, 2);
	rcText.bottom += deltaHeight;
	SetWindowPos(GetDlgItem(hwndForm, IDC_WHITERECT), 0, 0, 0,
		rcText.right-rcText.left,
		rcText.bottom-rcText.top,
		SWP_NOMOVE|SWP_NOZORDER);

	GetWindowRect(GetDlgItem(hwndForm, IDC_FRAME1), &rcText);
	MapWindowPoints(NULL, hwndForm, (LPPOINT)&rcText, 2);
	rcText.top += deltaHeight;
	SetWindowPos(GetDlgItem(hwndForm, IDC_FRAME1), 0,
		rcText.left,
		rcText.top,
		0, 0,
		SWP_NOSIZE|SWP_NOZORDER);

	GetWindowRect(GetDlgItem(hwndForm, IDC_FRAME), &rcText);
	MapWindowPoints(NULL, hwndForm, (LPPOINT)&rcText, 2);
	rcText.top += deltaHeight;
	SetWindowPos(GetDlgItem(hwndForm, IDC_FRAME), 0,
		rcText.left,
		rcText.top,
		rcText.right-rcText.left,
		rcText.bottom-rcText.top,
		SWP_NOZORDER);

	GetWindowRect(GetDlgItem(hwndForm, IDC_VSCROLL), &rcText);
	MapWindowPoints(NULL, hwndForm, (LPPOINT)&rcText, 2);
	rcText.top += deltaHeight;
	SetWindowPos(GetDlgItem(hwndForm, IDC_VSCROLL), 0,
		rcText.left,
		rcText.top,
		rcText.right-rcText.left,
		rcText.bottom-rcText.top,
		SWP_NOZORDER);

	if (fixedText) mir_free(fixedText);
}

static TJabberFormControlType JabberFormTypeNameToId(TCHAR *type)
{
	if ( !_tcscmp( type, _T("text-private")))
		return JFORM_CTYPE_TEXT_PRIVATE;
	if ( !_tcscmp( type, _T("text-multi")) || !_tcscmp( type, _T("jid-multi")))
		return JFORM_CTYPE_TEXT_MULTI;
	if ( !_tcscmp( type, _T("boolean")))
		return JFORM_CTYPE_BOOLEAN;
	if ( !_tcscmp( type, _T("list-single")))
		return JFORM_CTYPE_LIST_SINGLE;
	if ( !_tcscmp( type, _T("list-multi")))
		return JFORM_CTYPE_LIST_MULTI;
	if ( !_tcscmp( type, _T("fixed")))
		return JFORM_CTYPE_FIXED;
	if ( !_tcscmp( type, _T("hidden")))
		return JFORM_CTYPE_HIDDEN;
	// else
	return JFORM_CTYPE_TEXT_SINGLE;
}

void JabberFormLayoutSingleControl(TJabberFormControlInfo *item, TJabberFormLayoutInfo *layout_info, TCHAR *labelStr, TCHAR *valueStr)
{
	RECT rcLabel = {0}, rcCtrl = {0};
	if (item->hLabel)
	{
		SetRect(&rcLabel, 0, 0, layout_info->width, 0);
		HDC hdc = GetDC( item->hLabel );
		HFONT hfntSave = (HFONT)SelectObject(hdc, (HFONT)SendMessage(item->hLabel, WM_GETFONT, 0, 0));
		DrawText( hdc, labelStr, -1, &rcLabel, DT_CALCRECT|DT_WORDBREAK );
		SelectObject(hdc, hfntSave);
		ReleaseDC(item->hLabel, hdc);
	}

	int indent = layout_info->compact ? 10 : 20;

	if ((layout_info->compact && (item->type != JFORM_CTYPE_BOOLEAN) && (item->type != JFORM_CTYPE_FIXED))||
		(rcLabel.right >= layout_info->maxLabelWidth) ||
		(rcLabel.bottom > layout_info->ctrlHeight) ||
		(item->type == JFORM_CTYPE_LIST_MULTI) ||
		(item->type == JFORM_CTYPE_TEXT_MULTI))
	{
		int height = layout_info->ctrlHeight;
		if ((item->type == JFORM_CTYPE_LIST_MULTI) || (item->type == JFORM_CTYPE_TEXT_MULTI)) height *= 3;
		SetRect(&rcCtrl, indent, rcLabel.bottom, layout_info->width, rcLabel.bottom + height);
	} else
	if (item->type == JFORM_CTYPE_BOOLEAN)
	{
		SetRect(&rcCtrl, 0, 0, layout_info->width-20, 0);
		HDC hdc = GetDC( item->hCtrl );
		HFONT hfntSave = (HFONT)SelectObject(hdc, (HFONT)SendMessage(item->hCtrl, WM_GETFONT, 0, 0));
		DrawText( hdc, labelStr, -1, &rcCtrl, DT_CALCRECT|DT_RIGHT|DT_WORDBREAK );
		SelectObject(hdc, hfntSave);
		ReleaseDC(item->hCtrl, hdc);
		rcCtrl.right += 20;
	} else
	if (item->type == JFORM_CTYPE_FIXED)
	{
		SetRect(&rcCtrl, 0, 0, layout_info->width, 0);
		HDC hdc = GetDC( item->hCtrl );
		HFONT hfntSave = (HFONT)SelectObject(hdc, (HFONT)SendMessage(item->hCtrl, WM_GETFONT, 0, 0));
		DrawText( hdc, valueStr, -1, &rcCtrl, DT_CALCRECT|DT_EDITCONTROL|DT_WORDBREAK );
		SelectObject(hdc, hfntSave);
		ReleaseDC(item->hCtrl, hdc);
	} else
	{
		SetRect(&rcCtrl, rcLabel.right+5, 0, layout_info->width, layout_info->ctrlHeight);
		rcLabel.bottom = rcCtrl.bottom;
	}

	if (item->hLabel)
		SetWindowPos(item->hLabel, 0,
		0, 0, rcLabel.right-rcLabel.left, rcLabel.bottom-rcLabel.top,
		SWP_NOZORDER|SWP_NOMOVE);
	if (item->hCtrl)
		SetWindowPos(item->hCtrl, 0,
		0, 0, rcCtrl.right-rcCtrl.left, rcCtrl.bottom-rcCtrl.top,
		SWP_NOZORDER|SWP_NOMOVE);

	item->ptLabel.x = rcLabel.left;
	item->ptLabel.y = rcLabel.top;
	item->ptCtrl.x = rcCtrl.left;
	item->ptCtrl.y = rcCtrl.top;
	item->szBlock.cx = layout_info->width;
	item->szBlock.cy = max(rcLabel.bottom, rcCtrl.bottom);
}

#define JabberFormCreateLabel()	\
	CreateWindow( _T("static"), labelStr, WS_CHILD|WS_VISIBLE|SS_CENTERIMAGE, \
		0, 0, 0, 0, hwndStatic, ( HMENU ) IDC_STATIC, hInst, NULL )

TJabberFormControlInfo *JabberFormAppendControl(HWND hwndStatic, TJabberFormLayoutInfo *layout_info, TJabberFormControlType type, TCHAR *labelStr, TCHAR *valueStr)
{
	TJabberFormControlList *controls = (TJabberFormControlList *)GetWindowLong(hwndStatic, GWL_USERDATA);
	if (!controls)
	{
		controls = new TJabberFormControlList(5);
		SetWindowLong(hwndStatic, GWL_USERDATA, (LONG)controls);
	}

	TJabberFormControlInfo *item = (TJabberFormControlInfo *)mir_alloc(sizeof(TJabberFormControlInfo));
	item->type = type;
	item->hLabel = item->hCtrl = NULL;

	switch (type)
	{
		case JFORM_CTYPE_TEXT_PRIVATE:
		{
			item->hLabel = JabberFormCreateLabel();
			item->hCtrl = CreateWindowEx( WS_EX_CLIENTEDGE, _T("edit"), valueStr,
				WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_LEFT|ES_AUTOHSCROLL|ES_PASSWORD,
				0, 0, 0, 0,
				hwndStatic, ( HMENU ) layout_info->id, hInst, NULL );
			++layout_info->id;
			break;
		}
		case JFORM_CTYPE_TEXT_MULTI:
		{
			item->hLabel = JabberFormCreateLabel();
			item->hCtrl = CreateWindowEx( WS_EX_CLIENTEDGE, _T("edit"), valueStr,
				WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_VSCROLL|ES_LEFT|ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN,
				0, 0, 0, 0,
				hwndStatic, ( HMENU ) layout_info->id, hInst, NULL );
			WNDPROC oldWndProc = ( WNDPROC ) SetWindowLong( item->hCtrl, GWL_WNDPROC, ( LPARAM )JabberFormMultiLineWndProc );
			SetWindowLong( item->hCtrl, GWL_USERDATA, ( LONG ) oldWndProc );
			++layout_info->id;
			break;
		}
		case JFORM_CTYPE_BOOLEAN:
		{
			item->hCtrl = CreateWindowEx( 0, _T("button"), labelStr,
				WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX|BS_MULTILINE,
				0, 0, 0, 0,
				hwndStatic, ( HMENU ) layout_info->id, hInst, NULL );
			if ( valueStr && !_tcscmp( valueStr, _T("1")))
				SendMessage( item->hCtrl, BM_SETCHECK, 1, 0 );
			++layout_info->id;
			break;
		}
		case JFORM_CTYPE_LIST_SINGLE:
		{
			item->hLabel = JabberFormCreateLabel();
			item->hCtrl = CreateWindowExA( WS_EX_CLIENTEDGE, "combobox", NULL,
				WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST,
				0, 0, 0, 0,
				hwndStatic, ( HMENU ) layout_info->id, hInst, NULL );
			++layout_info->id;
			break;
		}
		case JFORM_CTYPE_LIST_MULTI:
		{
			item->hLabel = JabberFormCreateLabel();
			item->hCtrl = CreateWindowExA( WS_EX_CLIENTEDGE, "listbox",
				NULL, WS_CHILD|WS_VISIBLE|WS_TABSTOP|LBS_MULTIPLESEL,
				0, 0, 0, 0,
				hwndStatic, ( HMENU ) layout_info->id, hInst, NULL );
			++layout_info->id;
			break;
		}
		case JFORM_CTYPE_FIXED:
		{
			item->hCtrl = CreateWindow( _T("edit"), valueStr,
				WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_READONLY,
				0, 0, 0, 0,
				hwndStatic, ( HMENU ) IDC_STATIC, hInst, NULL );
			break;
		}
		case JFORM_CTYPE_HIDDEN:
		{
			break;
		}
		case JFORM_CTYPE_TEXT_SINGLE:
		{
			item->hLabel = labelStr ? (JabberFormCreateLabel()) : NULL;
			item->hCtrl = CreateWindowEx( WS_EX_CLIENTEDGE, _T("edit"), valueStr,
				WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_LEFT|ES_AUTOHSCROLL,
				0, 0, 0, 0,
				hwndStatic, ( HMENU ) layout_info->id, hInst, NULL );
			++layout_info->id;
			break;
		}
	}

	HFONT hFont = ( HFONT ) SendMessage( GetParent(hwndStatic), WM_GETFONT, 0, 0 );
	if (item->hLabel) SendMessage( item->hLabel, WM_SETFONT, ( WPARAM ) hFont, 0 );
	if (item->hCtrl) SendMessage( item->hCtrl, WM_SETFONT, ( WPARAM ) hFont, 0 );

	JabberFormLayoutSingleControl(item, layout_info, labelStr, valueStr);

	controls->insert(item);
	return item;
}

void JabberFormAddListItem(TJabberFormControlInfo *item, TCHAR *text, bool selected)
{
	DWORD dwIndex;
	switch (item->type)
	{
	case JFORM_CTYPE_LIST_MULTI:
		dwIndex = SendMessage(item->hCtrl, LB_ADDSTRING, 0, (LPARAM)text);
		if (selected) SendMessage(item->hCtrl, LB_SETSEL, TRUE, dwIndex);
		break;
	case JFORM_CTYPE_LIST_SINGLE:
		dwIndex = SendMessage(item->hCtrl, CB_ADDSTRING, 0, (LPARAM)text);
		if (selected) SendMessage(item->hCtrl, CB_SETCURSEL, dwIndex, 0);
		break;
	}
}

void JabberFormLayoutControls(HWND hwndStatic, TJabberFormLayoutInfo *layout_info, int *formHeight)
{
	TJabberFormControlList *controls = (TJabberFormControlList *)GetWindowLong(hwndStatic, GWL_USERDATA);
	if (!controls) return;

	for (int i = 0; i < controls->getCount(); ++i)
	{
		if ((*controls)[i]->hLabel)
			SetWindowPos((*controls)[i]->hLabel, 0,
			layout_info->offset+(*controls)[i]->ptLabel.x, layout_info->y_pos+(*controls)[i]->ptLabel.y, 0, 0,
			SWP_NOZORDER|SWP_NOSIZE);
		if ((*controls)[i]->hCtrl)
			SetWindowPos((*controls)[i]->hCtrl, 0,
			layout_info->offset+(*controls)[i]->ptCtrl.x, layout_info->y_pos+(*controls)[i]->ptCtrl.y, 0, 0,
			SWP_NOZORDER|SWP_NOSIZE);

		layout_info->y_pos += (*controls)[i]->szBlock.cy;
		layout_info->y_pos += layout_info->y_spacing;
	}

	*formHeight = layout_info->y_pos + (layout_info->compact ? 0 : 9);
}

HJFORMLAYOUT JabberFormCreateLayout(HWND hwndStatic)
{
	RECT frameRect;
	GetClientRect( hwndStatic, &frameRect );

	TJabberFormLayoutInfo *layout_info = (TJabberFormLayoutInfo *)mir_alloc(sizeof(TJabberFormLayoutInfo));
	layout_info->compact = false;
	layout_info->ctrlHeight = 20;
	layout_info->id = 0;
	layout_info->width = frameRect.right - frameRect.left - 20 - 10;
	layout_info->y_spacing = 5;
	layout_info->maxLabelWidth = layout_info->width*2/5;
	layout_info->offset = 10;
	layout_info->y_pos = 14;
	return layout_info;
}

void JabberFormCreateUI( HWND hwndStatic, XmlNode *xNode, int *formHeight, BOOL bCompact )
{
	JabberFormDestroyUI(hwndStatic);

	XmlNode *n, *v, *o, *vs;

	int i, j, k;
	TCHAR* label, *typeName, *labelStr, *valueStr, *varStr, *str, *p, *valueText;
	RECT frameRect;

	if ( xNode==NULL || xNode->name==NULL || strcmp( xNode->name, "x" ) || hwndStatic==NULL ) return;

	GetClientRect( hwndStatic, &frameRect );

	TJabberFormLayoutInfo layout_info;
	layout_info.compact = bCompact ? true : false;
	layout_info.ctrlHeight = 20;
	layout_info.id = 0;
	layout_info.width = frameRect.right - frameRect.left - 20;
	if (!bCompact) layout_info.width -= 10;
	layout_info.y_spacing = bCompact ? 1 : 5;
	layout_info.maxLabelWidth = layout_info.width*2/5;
	layout_info.offset = 10;
	layout_info.y_pos = bCompact ? 0 : 14;
	for ( i=0; i<xNode->numChild; i++ ) {
		n = xNode->child[i];
		if ( n->name ) {
			if ( !strcmp( n->name, "field" )) {
				varStr=JabberXmlGetAttrValue( n, "var" );
				if (( typeName=JabberXmlGetAttrValue( n, "type" )) != NULL ) {
 					if (( label=JabberXmlGetAttrValue( n, "label" )) != NULL )
						labelStr = mir_tstrdup( label );
					else
						labelStr = mir_tstrdup( varStr );

					TJabberFormControlType type = JabberFormTypeNameToId(typeName);

					if (( v=JabberXmlGetChild( n, "value" )) != NULL )
					{
						valueText = v->text;
						if (type != JFORM_CTYPE_TEXT_MULTI)
						{
							valueStr = mir_tstrdup( valueText );
						} else
						{
							int size = 1;
							for ( j=0; j<n->numChild; j++ ) {
								v = n->child[j];
								if ( v->name && !strcmp( v->name, "value" ) && v->text )
									size += _tcslen( v->text ) + 2;
							}
							valueStr = ( TCHAR* )mir_alloc( sizeof(TCHAR)*size );
							valueStr[0] = '\0';
							for ( j=0; j<n->numChild; j++ ) {
								v = n->child[j];
								if ( v->name && !strcmp( v->name, "value" ) && v->text ) {
									if ( valueStr[0] )	_tcscat( valueStr, _T("\r\n"));
									_tcscat( valueStr, v->text );
							}	}
						}
					} else
					{
						valueText = valueStr = NULL;
					}

					TJabberFormControlInfo *item = JabberFormAppendControl(hwndStatic, &layout_info, type, labelStr, valueStr);

					if (type == JFORM_CTYPE_LIST_SINGLE)
					{
						for ( j=0; j<n->numChild; j++ ) {
							o = n->child[j];
							if ( o->name && !strcmp( o->name, "option" )) {
								if (( v=JabberXmlGetChild( o, "value" ))!=NULL && v->text ) {
									if (( str=JabberXmlGetAttrValue( o, "label" )) == NULL )
										str = v->text;
									if (( p = mir_tstrdup( str )) != NULL ) {
										bool selected = false;
										if ( valueText!=NULL && !_tcscmp( valueText, v->text ))
											selected = true;
										JabberFormAddListItem(item, p, selected);
										mir_free( p );
						}	}	}	}
					} else
					if (type == JFORM_CTYPE_LIST_MULTI)
					{
						for ( j=0; j<n->numChild; j++ ) {
							o = n->child[j];
							if ( o->name && !strcmp( o->name, "option" )) {
								if (( v = JabberXmlGetChild( o, "value" ))!=NULL && v->text ) {
									if (( str=JabberXmlGetAttrValue( o, "label" )) == NULL )
										str = v->text;
									if (( p = mir_tstrdup( str )) != NULL ) {
										bool selected = false;
										for ( k=0; k<n->numChild; k++ ) {
											vs = n->child[k];
											if ( vs->name && !strcmp( vs->name, "value" ) && vs->text && !_tcscmp( vs->text, v->text ))
											{
												selected = true;
												break;
											}
										}
										JabberFormAddListItem( item, p, selected );
										mir_free( p );
	}	}	}	}	}	}	}	}	}

	JabberFormLayoutControls(hwndStatic, &layout_info, formHeight);
}

void JabberFormDestroyUI(HWND hwndStatic)
{
	TJabberFormControlList *controls = (TJabberFormControlList *)GetWindowLong(hwndStatic, GWL_USERDATA);
	if (controls)
	{
		controls->destroy();
		delete controls;
		SetWindowLong(hwndStatic, GWL_USERDATA, 0);
	}
}

XmlNode* JabberFormGetData( HWND hwndStatic, XmlNode* xNode )
{
	HWND hFrame, hCtrl;
	XmlNode *n, *v, *o, *x;
	int id, j, k, len;
	TCHAR *varName, *type, *fieldStr, *str, *str2, *p, *q, *labelText;

	if ( xNode==NULL || xNode->name==NULL || strcmp( xNode->name, "x" ) || hwndStatic==NULL )
		return NULL;

	hFrame = hwndStatic;
	id = 0;
	x = new XmlNode( "x" ); x->addAttr( "xmlns", JABBER_FEAT_DATA_FORMS ); x->addAttr( "type", "submit" );
	for ( int i=0; i<xNode->numChild; i++ ) {
		n = xNode->child[i];
		fieldStr = NULL;
		if ( lstrcmpA( n->name, "field" ))
			continue;

		if (( varName=JabberXmlGetAttrValue( n, "var" )) == NULL || ( type=JabberXmlGetAttrValue( n, "type" )) == NULL )
			continue;

		hCtrl = GetDlgItem( hFrame, id );
		XmlNode* field = x->addChild( "field" ); field->addAttr( "var", varName );

		if ( !_tcscmp( type, _T("text-multi")) || !_tcscmp( type, _T("jid-multi"))) {
			len = GetWindowTextLength( GetDlgItem( hFrame, id ));
			str = ( TCHAR* )mir_alloc( sizeof(TCHAR)*( len+1 ));
			GetDlgItemText( hFrame, id, str, len+1 );
			p = str;
			while ( p != NULL ) {
				if (( q = _tcsstr( p, _T("\r\n"))) != NULL )
					*q = '\0';
				field->addChild( "value", p );
				p = q ? q+2 : NULL;
			}
			mir_free( str );
			id++;
		}
		else if ( !_tcscmp( type, _T("boolean"))) {
			TCHAR buf[ 10 ];
			_itot( IsDlgButtonChecked( hFrame, id ) == BST_CHECKED ? 1 : 0, buf, 10 );
			field->addChild( "value", buf );
			id++;
		}
		else if ( !_tcscmp( type, _T("list-single"))) {
			len = GetWindowTextLength( GetDlgItem( hFrame, id ));
			str = ( TCHAR* )mir_alloc( sizeof( TCHAR )*( len+1 ));
			GetDlgItemText( hFrame, id, str, len+1 );
			for ( j=0; j < n->numChild; j++ ) {
				o = n->child[j];
				if ( o && o->name && !strcmp( o->name, "option" )) {
					if (( v=JabberXmlGetChild( o, "value" ))!=NULL && v->text ) {
						if (( str2=JabberXmlGetAttrValue( o, "label" )) == NULL )
							str2 = v->text;
						if ( !lstrcmp( str2, str ))
							break;
			}	}	}

			if ( j < n->numChild )
				field->addChild( "value", v->text );

			mir_free( str );
			id++;
		}
		else if ( !_tcscmp( type, _T("list-multi"))) {
			int count = SendMessage( hCtrl, LB_GETCOUNT, 0, 0 );
			for ( j=0; j<count; j++ ) {
				if ( SendMessage( hCtrl, LB_GETSEL, j, 0 ) > 0 ) {
					// an entry is selected
					len = SendMessage( hCtrl, LB_GETTEXTLEN, j, 0 );
					if (( str = ( TCHAR* )mir_alloc(( len+1 )*sizeof( TCHAR ))) != NULL ) {
						SendMessage( hCtrl, LB_GETTEXT, j, ( LPARAM )str );
						for ( k=0; k < n->numChild; k++ ) {
							o = n->child[k];
							if ( o && o->name && !strcmp( o->name, "option" )) {
								if (( v=JabberXmlGetChild( o, "value" ))!=NULL && v->text ) {
									if (( labelText=JabberXmlGetAttrValue( o, "label" )) == NULL )
										labelText = v->text;

									if ( !lstrcmp( labelText, str ))
										field->addChild( "value", v->text );
						}	}	}
						mir_free( str );
			}	}	}
			id++;
		}
		else if ( !_tcscmp( type, _T("fixed")) || !_tcscmp( type, _T("hidden"))) {
			v = JabberXmlGetChild( n, "value" );
			if ( v != NULL && v->text != NULL )
				field->addChild( "value", v->text );
		}
		else { // everything else is considered "text-single" or "text-private"
			len = GetWindowTextLength( GetDlgItem( hFrame, id ));
			str = ( TCHAR* )mir_alloc( sizeof(TCHAR)*( len+1 ));
			GetDlgItemText( hFrame, id, str, len+1 );
			field->addChild( "value", str );
			mir_free( str );
			id++;
	}	}

	return x;
}

typedef struct {
	XmlNode *xNode;
	TCHAR defTitle[128];	// Default title if no <title/> in xNode
	RECT frameRect;		// Clipping region of the frame to scroll
	int frameHeight;	// Height of the frame ( can be eliminated, redundant to frameRect )
	int formHeight;		// Actual height of the form
	int curPos;			// Current scroll position
	JABBER_FORM_SUBMIT_FUNC pfnSubmit;
	void *userdata;
} JABBER_FORM_INFO;

static BOOL CALLBACK JabberFormDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	JABBER_FORM_INFO *jfi;

	switch ( msg ) {
	case WM_INITDIALOG:
		{
			XmlNode *n;
			LONG frameExStyle;

			// lParam is ( JABBER_FORM_INFO * )
			TranslateDialogDefault( hwndDlg );
			ShowWindow( GetDlgItem( hwndDlg, IDC_FRAME_TEXT ), SW_HIDE );
			jfi = ( JABBER_FORM_INFO * ) lParam;
			if ( jfi != NULL ) {
				// Set dialog title
				if ( jfi->xNode!=NULL && ( n=JabberXmlGetChild( jfi->xNode, "title" ))!=NULL && n->text!=NULL )
					SetWindowText( hwndDlg, n->text );
				else if ( jfi->defTitle != NULL )
					SetWindowText( hwndDlg, TranslateTS( jfi->defTitle ));
				// Set instruction field
				if ( jfi->xNode!=NULL && ( n=JabberXmlGetChild( jfi->xNode, "instructions" ))!=NULL && n->text!=NULL )
					JabberFormSetInstruction( hwndDlg, n->text );
				else
				{
					if ( jfi->xNode!=NULL && ( n=JabberXmlGetChild( jfi->xNode, "title" ))!=NULL && n->text!=NULL )
						JabberFormSetInstruction( hwndDlg, n->text );
					else if ( jfi->defTitle != NULL )
						JabberFormSetInstruction( hwndDlg, TranslateTS( jfi->defTitle ));
				}

				// Create form
				if ( jfi->xNode != NULL ) {
					RECT rect;

					GetClientRect( GetDlgItem( hwndDlg, IDC_FRAME ), &( jfi->frameRect ));
					GetClientRect( GetDlgItem( hwndDlg, IDC_VSCROLL ), &rect );
					jfi->frameRect.right -= ( rect.right - rect.left );
					GetClientRect( GetDlgItem( hwndDlg, IDC_FRAME ), &rect );
					jfi->frameHeight = rect.bottom - rect.top;
					JabberFormCreateUI( GetDlgItem( hwndDlg, IDC_FRAME ), jfi->xNode, &( jfi->formHeight ));
				}
			}

			if ( jfi->formHeight > jfi->frameHeight ) {
				HWND hwndScroll;

				hwndScroll = GetDlgItem( hwndDlg, IDC_VSCROLL );
				EnableWindow( hwndScroll, TRUE );
				SetScrollRange( hwndScroll, SB_CTL, 0, jfi->formHeight - jfi->frameHeight, FALSE );
				jfi->curPos = 0;
			}

			// Enable WS_EX_CONTROLPARENT on IDC_FRAME ( so tab stop goes through all its children )
			frameExStyle = GetWindowLong( GetDlgItem( hwndDlg, IDC_FRAME ), GWL_EXSTYLE );
			frameExStyle |= WS_EX_CONTROLPARENT;
			SetWindowLong( GetDlgItem( hwndDlg, IDC_FRAME ), GWL_EXSTYLE, frameExStyle );

			SetWindowLong( hwndDlg, GWL_USERDATA, ( LONG ) jfi );
			if ( jfi->pfnSubmit != NULL )
				EnableWindow( GetDlgItem( hwndDlg, IDC_SUBMIT ), TRUE );
		}
		return TRUE;
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
	case WM_MOUSEWHEEL:
		{
			int zDelta = GET_WHEEL_DELTA_WPARAM( wParam );
			if ( zDelta ) {
				int nScrollLines=0;
				SystemParametersInfo( SPI_GETWHEELSCROLLLINES, 0, (void*)&nScrollLines, 0 );
				for (int i = 0; i < ( nScrollLines + 1 ) / 2; i++ )
					SendMessage( hwndDlg, WM_VSCROLL, ( zDelta < 0 ) ? SB_LINEDOWN : SB_LINEUP, 0 );
			}
		}
		break;
	case WM_VSCROLL:
		{
			int pos;

			jfi = ( JABBER_FORM_INFO * ) GetWindowLong( hwndDlg, GWL_USERDATA );
			if ( jfi != NULL ) {
				pos = jfi->curPos;
				switch ( LOWORD( wParam )) {
				case SB_LINEDOWN:
					pos += 15;
					break;
				case SB_LINEUP:
					pos -= 15;
					break;
				case SB_PAGEDOWN:
					pos += ( jfi->frameHeight - 10 );
					break;
				case SB_PAGEUP:
					pos -= ( jfi->frameHeight - 10 );
					break;
				case SB_THUMBTRACK:
					pos = HIWORD( wParam );
					break;
				}
				if ( pos > ( jfi->formHeight - jfi->frameHeight ))
					pos = jfi->formHeight - jfi->frameHeight;
				if ( pos < 0 )
					pos = 0;
				if ( jfi->curPos != pos ) {
					ScrollWindow( GetDlgItem( hwndDlg, IDC_FRAME ), 0, jfi->curPos - pos, NULL, &( jfi->frameRect ));
					SetScrollPos( GetDlgItem( hwndDlg, IDC_VSCROLL ), SB_CTL, pos, TRUE );
					jfi->curPos = pos;
				}
			}
		}
		break;
	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDC_SUBMIT:
			jfi = ( JABBER_FORM_INFO * ) GetWindowLong( hwndDlg, GWL_USERDATA );
			if ( jfi != NULL ) {
				XmlNode* n = JabberFormGetData( GetDlgItem( hwndDlg, IDC_FRAME ), jfi->xNode );
				( jfi->pfnSubmit )( n, jfi->userdata );
			}
			// fall through
		case IDCLOSE:
			DestroyWindow( hwndDlg );
			return TRUE;
		}
		break;
	case WM_CLOSE:
		DestroyWindow( hwndDlg );
		break;
	case WM_DESTROY:
		jfi = ( JABBER_FORM_INFO * ) GetWindowLong( hwndDlg, GWL_USERDATA );
		if ( jfi != NULL ) {
			if ( jfi->xNode != NULL )
				delete jfi->xNode;
			mir_free( jfi );
		}
		break;
	}

	return FALSE;
}

static VOID CALLBACK JabberFormCreateDialogApcProc( DWORD param )
{
	CreateDialogParam( hInst, MAKEINTRESOURCE( IDD_FORM ), NULL, JabberFormDlgProc, ( LPARAM )param );
}

void JabberFormCreateDialog( XmlNode *xNode, TCHAR* defTitle, JABBER_FORM_SUBMIT_FUNC pfnSubmit, void *userdata )
{
	JABBER_FORM_INFO *jfi;

	jfi = ( JABBER_FORM_INFO * ) mir_alloc( sizeof( JABBER_FORM_INFO ));
	memset( jfi, 0, sizeof( JABBER_FORM_INFO ));
	jfi->xNode = JabberXmlCopyNode( xNode );
	if ( defTitle )
		_tcsncpy( jfi->defTitle, defTitle, SIZEOF( jfi->defTitle ));
	jfi->pfnSubmit = pfnSubmit;
	jfi->userdata = userdata;

	if ( GetCurrentThreadId() != jabberMainThreadId )
		QueueUserAPC( JabberFormCreateDialogApcProc, hMainThread, ( DWORD )jfi );
	else
		JabberFormCreateDialogApcProc(( DWORD )jfi );
}
