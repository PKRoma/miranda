/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2007 Miranda ICQ/IM project,
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
#include "FontService.h"
#include "../database/dblists.h"

// *_w2 is working copy of list
// *_w3 is stores initial configuration
FontIdList   font_id_list_w2,   font_id_list_w3;
ColourIdList colour_id_list_w2, colour_id_list_w3;

#define UM_SETFONTGROUP		(WM_USER + 101)
#define TIMER_ID				11015

#define FSUI_COLORBOXWIDTH		50
#define FSUI_COLORBOXLEFT		5
#define FSUI_FONTFRAMEHORZ		5
#define FSUI_FONTFRAMEVERT		4
#define FSUI_FONTLEFT			(FSUI_COLORBOXLEFT+FSUI_COLORBOXWIDTH+5)

extern void UpdateFontSettings(TFontID *font_id, TFontSettings *fontsettings);
extern void UpdateColourSettings(TColourID *colour_id, COLORREF *colour);

void DestroyList( SortedList* list )
{
	int i;
	for ( i = 0; i < list->realCount; i++ )
		mir_free( list->items[i] );
	List_Destroy( list );
}

static void sttCopyList( SortedList* s, SortedList* d, size_t itemSize )
{
	int i;

	d->increment = s->increment;
	d->sortFunc  = s->sortFunc;

	DestroyList( d );

	for ( i = 0; i < s->realCount; i++ ) {
		void* item = mir_alloc( itemSize );
		memcpy( item, s->items[i], itemSize );
		List_Insert( d, item, i );
}	}

void WriteLine(HANDLE fhand, char *line)
{
	DWORD wrote;
	strcat(line, "\r\n");
	WriteFile(fhand, line, strlen(line), &wrote, 0);
}

BOOL ExportSettings(HWND hwndDlg, TCHAR *filename, FontIdList* flist, ColourIdList* clist)
{
	int i;
	char header[512], buff[1024], abuff[1024];

	HANDLE fhand = CreateFile(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if(fhand == INVALID_HANDLE_VALUE) {
		MessageBox(hwndDlg, filename, _T("Failed to create file"), MB_ICONWARNING | MB_OK);
		return FALSE;
	}

	header[0] = 0;

	strcpy(buff, "SETTINGS:\r\n");
	WriteLine(fhand, buff);

	for ( i = 0; i < flist->count; i++  ) {
		TFontID* F = flist->items[i];

		strcpy(buff, "[");
		strcat(buff, F->dbSettingsGroup);
		strcat(buff, "]");
		if ( strcmp( buff, header ) != 0) {
			strcpy(header, buff);
			WriteLine(fhand, buff);
		}

		if ( F->flags & FIDF_APPENDNAME )
			wsprintfA( buff, "%sName", F->prefix );
		else
			wsprintfA( buff, "%s", F->prefix );

		strcat(buff, "=s");
		#if defined( _UNICODE )
			WideCharToMultiByte(code_page, 0, F->value.szFace, -1, abuff, 1024, 0, 0);
			abuff[1023]=0;
			strcat( buff, abuff );
		#else
			strcat( buff, F->value.szFace );
		#endif
		WriteLine(fhand, buff);
		
		wsprintfA(buff, "%sSize=b", F->prefix);
		if ( F->flags & FIDF_SAVEACTUALHEIGHT ) {
			HDC hdc;
			SIZE size;
			HFONT hFont, hOldFont;
			LOGFONT lf;
			CreateFromFontSettings( &F->value, &lf, F->flags);
			hFont = CreateFontIndirect(&lf);

			hdc = GetDC(hwndDlg);
			hOldFont = (HFONT)SelectObject(hdc, hFont);
			GetTextExtentPoint32(hdc, _T("_W"), 2, &size);
			ReleaseDC(hwndDlg, hdc);
			SelectObject(hdc, hOldFont);
			DeleteObject(hFont);

			strcat(buff, _itoa((BYTE)size.cy, abuff, 10));
		}
		else if(F->flags & FIDF_SAVEPOINTSIZE) {
			HDC hdc = GetDC(hwndDlg);
			strcat(buff, _itoa((BYTE)-MulDiv(F->value.size, 72, GetDeviceCaps(hdc, LOGPIXELSY)), abuff, 10));
			ReleaseDC(hwndDlg, hdc);
		}
		else strcat(buff, _itoa((BYTE)F->value.size, abuff, 10));

		WriteLine(fhand, buff);

		wsprintfA(buff, "%sSty=b%d", F->prefix, (BYTE)F->value.style);
		WriteLine(fhand, buff);
		wsprintfA(buff, "%sSet=b%d", F->prefix, (BYTE)F->value.charset);
		WriteLine(fhand, buff);
		wsprintfA(buff, "%sCol=d%d", F->prefix, (DWORD)F->value.colour);
		WriteLine(fhand, buff);
		if(F->flags & FIDF_NOAS) {
			wsprintfA(buff, "%sAs=w%d", F->prefix, (WORD)0x00FF);
			WriteLine(fhand, buff);
		}
		wsprintfA(buff, "%sFlags=w%d", F->prefix, (WORD)F->flags);
		WriteLine(fhand, buff);
	}

	header[0] = 0;
	for ( i=0; i < clist->count; i++ ) {
		TColourID* C = clist->items[i];

		strcpy(buff, "[");
		strcat(buff, C->dbSettingsGroup);
		strcat(buff, "]");
		if(strcmp(buff, header) != 0) {
			strcpy(header, buff);
			WriteLine(fhand, buff);
		}
		wsprintfA(buff, "%s=d%d", C->setting, (DWORD)C->value );
		WriteLine(fhand, buff);
	}

	CloseHandle(fhand);
	return TRUE;
}

#define INTM_RELOADOPTIONS   (WM_USER+21)

void OptionsChanged()
{
	HWND hWnd = FindWindowEx( cli.hwndContactList, 0, CLISTCONTROL_CLASS, 0);
	if(hWnd) SendMessage(hWnd, INTM_RELOADOPTIONS, 0, 0);
	NotifyEventHooks(hFontReloadEvent, 0, 0);
	NotifyEventHooks(hColourReloadEvent, 0, 0);
}

TOOLINFO ti;
int x, y;

UINT_PTR CALLBACK CFHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam) {
	switch(uiMsg) {
		case WM_INITDIALOG:
			TranslateDialogDefault(hdlg);
			return 0;
	}

	return 0;
}

typedef struct
{
	int font_id;
	int colour_id;
} FSUIListItemData;

static BOOL sttFsuiBindColourIdToFonts(HWND hwndList, const TCHAR *name, const TCHAR *backgroundGroup, const TCHAR *backgroundName, int colourId)
{
	int i;
	BOOL res = FALSE;
	for (i = SendMessage(hwndList, LB_GETCOUNT, 0, 0); i--; )
	{
		FSUIListItemData *itemData = (FSUIListItemData *)SendMessage(hwndList, LB_GETITEMDATA, i, 0);

		if (itemData && (itemData->font_id >= 0) && name  && !_tcscmp(font_id_list_w2.items[itemData->font_id]->name, name))
		{
			FSUIListItemData *itemData = (FSUIListItemData *)SendMessage(hwndList, LB_GETITEMDATA, i, 0);
			if (itemData)
			{
				itemData->colour_id = colourId;
				res = TRUE;
			}
		}

		if (itemData && (itemData->font_id >= 0) && backgroundGroup && backgroundName &&
				!_tcscmp(font_id_list_w2.items[itemData->font_id]->backgroundGroup, backgroundGroup) &&
				!_tcscmp(font_id_list_w2.items[itemData->font_id]->backgroundName, backgroundName))
		{
			FSUIListItemData *itemData = (FSUIListItemData *)SendMessage(hwndList, LB_GETITEMDATA, i, 0);
			if (itemData)
			{
				itemData->colour_id = colourId;
				res = TRUE;
			}
		}
	}
	return res;
}

static HTREEITEM sttFindNamedTreeItemAt(HWND hwndTree, HTREEITEM hItem, const TCHAR *name)
{
	TVITEM tvi = {0};
	TCHAR str[MAX_PATH];

	if (hItem)
		tvi.hItem = TreeView_GetChild(hwndTree, hItem);
	else
		tvi.hItem = TreeView_GetRoot(hwndTree);

	if (!name)
		return tvi.hItem;

	tvi.mask = TVIF_TEXT;
	tvi.pszText = str;
	tvi.cchTextMax = MAX_PATH;

	while (tvi.hItem)
	{
		TreeView_GetItem(hwndTree, &tvi);

		if (!lstrcmp(tvi.pszText, name))
			return tvi.hItem;

		tvi.hItem = TreeView_GetNextSibling(hwndTree, tvi.hItem);
	}
	return NULL;
}

static BOOL sttGetTreeNodeText( HWND hwndTree, HTREEITEM hItem, char* szBuf, int cbLen )
{
	int codepage = CallService( MS_LANGPACK_GETCODEPAGE, 0, 0 );
	char *bufPtr = szBuf;
	while (hItem)
	{
		TCHAR tmpBuf[128];
		TVITEM tvi;
		tvi.hItem = hItem;
		tvi.pszText = tmpBuf;
		tvi.cchTextMax = 128;
		tvi.mask = TVIF_HANDLE|TVIF_TEXT;
		TreeView_GetItem(hwndTree, &tvi);
#ifdef _UNICODE
		bufPtr += WideCharToMultiByte( codepage, 0, tvi.pszText, lstrlen(tvi.pszText), bufPtr, cbLen-(bufPtr-szBuf), NULL, NULL );
#else
		strncpy( bufPtr, tvi.pszText, cbLen-(bufPtr-szBuf) );
		bufPtr += lstrlen(tvi.pszText);
#endif
		if ((bufPtr - szBuf) >= cbLen) break;
		*bufPtr++ = '@';

		hItem = TreeView_GetParent(hwndTree, hItem);
	}
	szBuf[min(bufPtr-szBuf, cbLen-1)] = 0;
	return TRUE;
}

static void sttFsuiCreateSettingsTreeNode(HWND hwndTree, const TCHAR *groupName)
{
	TCHAR itemName[1024];
	TCHAR* sectionName;
	int sectionLevel = 0;

	HTREEITEM hSection = NULL;
	lstrcpy(itemName, groupName);
	sectionName = itemName;

	while (sectionName) {
		// allow multi-level tree
		TCHAR* pItemName = sectionName;
		HTREEITEM hItem;

		if (sectionName = _tcschr(sectionName, '/')) {
			// one level deeper
			*sectionName = 0;
			sectionName++;
		}

		pItemName = TranslateTS( pItemName );

		hItem = sttFindNamedTreeItemAt(hwndTree, hSection, pItemName);
		if (!sectionName || !hItem) {
			if (!hItem) {
				TVINSERTSTRUCT tvis = {0};
				char paramName[MAX_PATH];

				tvis.hParent = hSection;
				tvis.hInsertAfter = TVI_LAST;//TVI_SORT;
				tvis.item.mask = TVIF_TEXT|TVIF_PARAM;
				tvis.item.pszText = pItemName;
				tvis.item.lParam = sectionName ? 0 : (LPARAM)mir_tstrdup(groupName);

				hItem = TreeView_InsertItem(hwndTree, &tvis);
				sttGetTreeNodeText( hwndTree, hItem, paramName, sizeof(paramName) );

				ZeroMemory(&tvis.item, sizeof(tvis.item));
				tvis.item.hItem = hItem;
				tvis.item.mask = TVIF_HANDLE|TVIF_STATE;
				tvis.item.state = tvis.item.stateMask = DBGetContactSettingByte(NULL, "FontServiceUI", paramName, TVIS_EXPANDED );
				TreeView_SetItem(hwndTree, &tvis.item);
			}
		}

		sectionLevel++;

		hSection = hItem;
	}
}

static void sttSaveCollapseState( HWND hwndTree )
{
	HTREEITEM hti;
	int codepage = CallService( MS_LANGPACK_GETCODEPAGE, 0, 0 );
	TVITEM tvi;

	hti = TreeView_GetRoot( hwndTree );
	while( hti != NULL ) {
		HTREEITEM ht;
		char paramName[MAX_PATH];

		tvi.mask = TVIF_STATE | TVIF_HANDLE | TVIF_CHILDREN | TVIF_PARAM;
		tvi.hItem = hti;
		tvi.stateMask = (DWORD)-1;
		TreeView_GetItem( hwndTree, &tvi );

		if( tvi.cChildren > 0 ) {
			sttGetTreeNodeText( hwndTree, hti, paramName, sizeof(paramName) );
			if ( tvi.state & TVIS_EXPANDED )
				DBWriteContactSettingByte(NULL, "FontServiceUI", paramName, TVIS_EXPANDED );
			else
				DBWriteContactSettingByte(NULL, "FontServiceUI", paramName, 0 );
		}

		ht = TreeView_GetChild( hwndTree, hti );
		if( ht == NULL ) {
			ht = TreeView_GetNextSibling( hwndTree, hti );
			if( ht == NULL ) {
				ht = TreeView_GetParent( hwndTree, hti );
				if( ht == NULL ) break;
				ht = TreeView_GetNextSibling( hwndTree, ht );
		}	}

		hti = ht;
}	}

static BOOL CALLBACK DlgProcLogOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int i;
	LOGFONT lf;

	static HBRUSH hBkgColourBrush = 0;

	switch (msg) {
		case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);

			sttCopyList(( SortedList* )&font_id_list, ( SortedList* )&font_id_list_w2, sizeof( TFontID ));
			sttCopyList(( SortedList* )&font_id_list, ( SortedList* )&font_id_list_w3, sizeof( TFontID ));

			sttCopyList(( SortedList* )&colour_id_list, ( SortedList* )&colour_id_list_w2, sizeof( TColourID ));
			sttCopyList(( SortedList* )&colour_id_list, ( SortedList* )&colour_id_list_w3, sizeof( TColourID ));

			for ( i = 0; i < font_id_list_w2.count; i++ ) {
				TFontID* F = font_id_list_w2.items[i];
				// sync settings with database
				UpdateFontSettings( F, &F->value );
				sttFsuiCreateSettingsTreeNode(GetDlgItem(hwndDlg, IDC_FONTGROUP), F->group);
			}

			for ( i = 0; i < colour_id_list_w2.count; i++  ) {
				TColourID* C = colour_id_list_w2.items[i];

				// sync settings with database
				UpdateColourSettings( C, &C->value );
			}

			SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_SETDEFAULTCOLOUR, 0, (LPARAM)GetSysColor(COLOR_WINDOW));

			return TRUE;
		}

		case UM_SETFONTGROUP:
		{
			TCHAR *group_buff = NULL;
			TVITEM tvi = {0};
			tvi.hItem = TreeView_GetSelection(GetDlgItem(hwndDlg, IDC_FONTGROUP));
			tvi.mask = TVIF_HANDLE|TVIF_PARAM;
			TreeView_GetItem(GetDlgItem(hwndDlg, IDC_FONTGROUP), &tvi);
			group_buff = (TCHAR *)tvi.lParam;

			SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_RESETCONTENT, 0, 0);
			SendDlgItemMessage(hwndDlg, IDC_COLOURLIST, CB_RESETCONTENT, 0, 0);

			if (group_buff)
			{
				BOOL need_restart = FALSE;
				int fontId = 0, itemId;
				int first_font_index = -1;
				int colourId = 0;
				int first_colour_index = -1;

				SendDlgItemMessage(hwndDlg, IDC_FONTLIST, WM_SETREDRAW, FALSE, 0);

				for ( fontId = 0; fontId < font_id_list_w2.count; fontId++ ) {
					TFontID* F = font_id_list_w2.items[fontId];
					if ( _tcsncmp( F->group, group_buff, 64 ) == 0 ) {
						FSUIListItemData *itemData = mir_alloc(sizeof(FSUIListItemData));
						itemData->colour_id = -1;
						itemData->font_id = fontId;

						if ( first_font_index == -1 )
							first_font_index = fontId;

						itemId = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_ADDSTRING, (WPARAM)-1, (LPARAM)itemData);
						need_restart |= (F->flags & FIDF_NEEDRESTART);
				}	}

				ShowWindow( GetDlgItem(hwndDlg, IDC_STAT_RESTART), (need_restart ? SW_SHOW : SW_HIDE));

				if ( hBkgColourBrush ) {
					DeleteObject( hBkgColourBrush );
					hBkgColourBrush = 0;
				}

				for ( colourId = 0; colourId < colour_id_list_w2.count; colourId++ ) {
					TColourID* C = colour_id_list_w2.items[colourId];
					if ( _tcsncmp( C->group, group_buff, 64 ) == 0 ) {
						FSUIListItemData *itemData = NULL;
						if ( first_colour_index == -1 )
							first_colour_index = colourId;

						if (!sttFsuiBindColourIdToFonts(GetDlgItem(hwndDlg, IDC_FONTLIST), C->name, C->group, C->name, colourId))
						{
							itemData = mir_alloc(sizeof(FSUIListItemData));
							itemData->colour_id = colourId;
							itemData->font_id = -1;

							itemId = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_ADDSTRING, (WPARAM)-1, (LPARAM)itemData);
						}

						//itemId = SendDlgItemMessage(hwndDlg, IDC_COLOURLIST, CB_ADDSTRING, (WPARAM)-1, (LPARAM)TranslateTS( C->name ));
						//SendDlgItemMessage(hwndDlg, IDC_COLOURLIST, CB_SETITEMDATA, itemId, colourId);

						if ( _tcscmp( C->name, _T("Background") ) == 0 )
							hBkgColourBrush = CreateSolidBrush( C->value );
				}	}

				if ( !hBkgColourBrush )
					hBkgColourBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));

				SendDlgItemMessage(hwndDlg, IDC_FONTLIST, WM_SETREDRAW, TRUE, 0);
				UpdateWindow(GetDlgItem(hwndDlg, IDC_FONTLIST));

				SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_SETSEL, TRUE, 0);
				SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_FONTLIST, LBN_SELCHANGE), 0);
			} else
			{
				EnableWindow(GetDlgItem(hwndDlg, IDC_BKGCOLOUR), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_FONTCOLOUR), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_CHOOSEFONT), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_RESET), FALSE);
			}
			return TRUE;
		}

		case WM_MEASUREITEM:
		{
			MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT *)lParam;
			HFONT hFont = NULL, hoFont = NULL;
			SIZE fontSize;
			BOOL bIsFont = FALSE;
			FSUIListItemData *itemData = (FSUIListItemData *)mis->itemData;
			TCHAR *itemName = NULL;
			HDC hdc;

			if ((mis->CtlID != IDC_FONTLIST) || (mis->itemID == -1))
				break;

			if (!itemData) return FALSE;

			if (itemData->font_id >= 0)
			{
				int iItem = itemData->font_id;
				bIsFont = TRUE;
				CreateFromFontSettings(&font_id_list_w2.items[iItem]->value, &lf, font_id_list_w2.items[iItem]->flags);
				hFont = CreateFontIndirect(&lf);
				itemName = TranslateTS(font_id_list_w2.items[iItem]->name);
			}
			if (itemData->colour_id >= 0)
			{
				int iItem = itemData->colour_id;
				if ( !itemName )
					itemName = TranslateTS( colour_id_list_w2.items[iItem]->name );
			}
			
			hdc = GetDC(GetDlgItem(hwndDlg, mis->CtlID));
			if ( hFont )
				hoFont = (HFONT) SelectObject(hdc, hFont);
			else
				hoFont = (HFONT) SelectObject(hdc, (HFONT)SendDlgItemMessage(hwndDlg, mis->CtlID, WM_GETFONT, 0, 0));

			GetTextExtentPoint32(hdc, itemName, lstrlen(itemName), &fontSize);
			if (hoFont) SelectObject(hdc, hoFont);
			if (hFont) DeleteObject(hFont);
			ReleaseDC(GetDlgItem(hwndDlg, mis->CtlID), hdc);
			mis->itemWidth = fontSize.cx + 2*FSUI_FONTFRAMEHORZ + 4;
			mis->itemHeight = fontSize.cy + 2*FSUI_FONTFRAMEVERT + 4;
			return TRUE;
		}

		case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *) lParam;
			HBRUSH hBrush = NULL;
			HFONT hFont = NULL, hoFont = NULL;
			COLORREF clBack = (COLORREF)-1;
			COLORREF clText = GetSysColor(COLOR_WINDOWTEXT);
			BOOL bIsFont = FALSE;
			TCHAR *itemName = NULL;
			FSUIListItemData *itemData = (FSUIListItemData *)dis->itemData;

			if(dis->CtlID != IDC_FONTLIST)
				break;

			if (!itemData) return FALSE;

			if (itemData->font_id >= 0)
			{
				int iItem = itemData->font_id;
				bIsFont = TRUE;
				CreateFromFontSettings(&font_id_list_w2.items[iItem]->value, &lf, font_id_list_w2.items[iItem]->flags);
				hFont = CreateFontIndirect(&lf);
				itemName = TranslateTS(font_id_list_w2.items[iItem]->name);
				clText = font_id_list_w2.items[iItem]->value.colour;
			}
			if (itemData->colour_id >= 0)
			{
				int iItem = itemData->colour_id;
				if (bIsFont)
				{
					clBack = colour_id_list_w2.items[iItem]->value;
				} else
				{
					clText = colour_id_list_w2.items[iItem]->value;
					itemName = TranslateTS(colour_id_list_w2.items[iItem]->name);
				}
			}

			if (hFont)
			{
				hoFont = (HFONT) SelectObject(dis->hDC, hFont);
			} else
			{
				hoFont = (HFONT) SelectObject(dis->hDC, (HFONT)SendDlgItemMessage(hwndDlg, dis->CtlID, WM_GETFONT, 0, 0));
			}
			SetBkMode(dis->hDC, TRANSPARENT);

			if (dis->itemState & ODS_SELECTED)
			{
				SetTextColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
				FillRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_HIGHLIGHT));
			} else
			{
				SetTextColor(dis->hDC, bIsFont?clText:GetSysColor(COLOR_WINDOWTEXT));
				if (bIsFont && (clBack != (COLORREF)-1))
				{
					HBRUSH hbrTmp = CreateSolidBrush(clBack);
					FillRect(dis->hDC, &dis->rcItem, hbrTmp);
					DeleteObject(hbrTmp);
				} else
				{
					FillRect(dis->hDC, &dis->rcItem, bIsFont ? hBkgColourBrush : GetSysColorBrush(COLOR_WINDOW));
				}
			}
			
			if (bIsFont)
			{
				HBRUSH hbrBack;
				RECT rc;

				if (clBack != (COLORREF)-1)
				{
					hbrBack = CreateSolidBrush(clBack);
				} else
				{
					LOGBRUSH lb;
					GetObject(hBkgColourBrush, sizeof(lf), &lb);
					hbrBack = CreateBrushIndirect(&lb);
				}

				SetRect(&rc,
					dis->rcItem.left+FSUI_COLORBOXLEFT,
					dis->rcItem.top+FSUI_FONTFRAMEVERT,
					dis->rcItem.left+FSUI_COLORBOXLEFT+FSUI_COLORBOXWIDTH,
					dis->rcItem.bottom-FSUI_FONTFRAMEVERT);
				
				FillRect(dis->hDC, &rc, hbrBack);

				FrameRect(dis->hDC, &rc, GetSysColorBrush(COLOR_HIGHLIGHT));
				rc.left += 1;
				rc.top += 1;
				rc.right -= 1;
				rc.bottom -= 1;
				FrameRect(dis->hDC, &rc, GetSysColorBrush(COLOR_HIGHLIGHTTEXT));

				SetTextColor(dis->hDC, clText);
				DrawText(dis->hDC, _T("abc"), 3, &rc, DT_CENTER|DT_NOPREFIX|DT_SINGLELINE|DT_VCENTER|DT_WORD_ELLIPSIS);

				if (dis->itemState & ODS_SELECTED)
					SetTextColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
				rc = dis->rcItem;
				rc.left += FSUI_FONTLEFT;
				DrawText(dis->hDC, itemName, _tcslen(itemName), &rc, DT_LEFT|DT_NOPREFIX|DT_SINGLELINE|DT_VCENTER|DT_WORD_ELLIPSIS);
			} else
			{
				RECT rc;
				HBRUSH hbrTmp;
				SetRect(&rc,
					dis->rcItem.left+FSUI_COLORBOXLEFT,
					dis->rcItem.top+FSUI_FONTFRAMEVERT,
					dis->rcItem.left+FSUI_COLORBOXLEFT+FSUI_COLORBOXWIDTH,
					dis->rcItem.bottom-FSUI_FONTFRAMEVERT);
				
				hbrTmp = CreateSolidBrush(clText);
				FillRect(dis->hDC, &rc, hbrTmp);
				DeleteObject(hbrTmp);

				FrameRect(dis->hDC, &rc, GetSysColorBrush(COLOR_HIGHLIGHT));
				rc.left += 1;
				rc.top += 1;
				rc.right -= 1;
				rc.bottom -= 1;
				FrameRect(dis->hDC, &rc, GetSysColorBrush(COLOR_HIGHLIGHTTEXT));

				rc = dis->rcItem;
				rc.left += FSUI_FONTLEFT;
				DrawText(dis->hDC, itemName, _tcslen(itemName), &rc, DT_LEFT|DT_NOPREFIX|DT_SINGLELINE|DT_VCENTER|DT_WORD_ELLIPSIS);
			}
			if (hoFont) SelectObject(dis->hDC, hoFont);
			if (hFont) DeleteObject(hFont);
			return TRUE;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_FONTLIST:
				{
					if (HIWORD(wParam) == LBN_SELCHANGE)
					{
						int selCount, i;

						char bEnableFont = 1;
						char bEnableClText = 1;
						char bEnableClBack = 1;
						char bEnableReset = 1;

						COLORREF clBack = 0xffffffff;
						COLORREF clText = 0xffffffff;

						if (selCount = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETSELCOUNT, (WPARAM)0, (LPARAM)0))
						{
							int *selItems = (int *)mir_alloc(font_id_list_w2.count * sizeof(int));
							SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETSELITEMS, (WPARAM)selCount, (LPARAM)selItems);
							for (i = 0; i < selCount; ++i)
							{
								FSUIListItemData *itemData = (FSUIListItemData *)SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETITEMDATA, selItems[i], 0);
								if (IsBadReadPtr(itemData, sizeof(FSUIListItemData))) continue; // prevent possible problems with corrupted itemData

								if (bEnableClBack && (itemData->colour_id < 0))
									bEnableClBack = 0;
								if (bEnableFont && (itemData->font_id < 0))
									bEnableFont = 0;
								if (!bEnableFont || bEnableClText && (itemData->font_id < 0))
									bEnableClText = 0;
								if (bEnableReset && (itemData->font_id >= 0) && !(font_id_list_w2.items[itemData->font_id]->flags&FIDF_DEFAULTVALID))
									bEnableReset = 0;

								if (bEnableClBack && (itemData->colour_id >= 0) && (clBack == 0xffffffff))
									clBack = colour_id_list_w2.items[itemData->colour_id]->value;
								if (bEnableClText && (itemData->font_id >= 0) && (clText == 0xffffffff))
									clText = font_id_list_w2.items[itemData->font_id]->value.colour;
							}
							mir_free(selItems);
						} else
						{
							bEnableFont = 0;
							bEnableClText = 0;
							bEnableClBack = 0;
							bEnableReset = 0;
						}

						EnableWindow(GetDlgItem(hwndDlg, IDC_BKGCOLOUR), bEnableClBack);
						EnableWindow(GetDlgItem(hwndDlg, IDC_FONTCOLOUR), bEnableClText);
						EnableWindow(GetDlgItem(hwndDlg, IDC_CHOOSEFONT), bEnableFont);
						EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_RESET), bEnableReset);

						if (bEnableClBack) SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_SETCOLOUR, 0, clBack);
						if (bEnableClText) SendDlgItemMessage(hwndDlg, IDC_FONTCOLOUR, CPM_SETCOLOUR, 0, clText);

						return TRUE;
					} else
					if (HIWORD(wParam) != LBN_DBLCLK)
					{
						return TRUE;
					}
					//fall through
				}
				case IDC_CHOOSEFONT:
				{
					int selCount;
					if (selCount = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETSELCOUNT, 0, 0))
					{
						FSUIListItemData *itemData;
						CHOOSEFONT cf = { 0 };
						int i;
						int *selItems = (int *)mir_alloc(selCount * sizeof(int));
						SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETSELITEMS, (WPARAM)selCount, (LPARAM) selItems);
						itemData = (FSUIListItemData *)SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETITEMDATA, selItems[0], 0);
						if (itemData->font_id < 0)
						{
							mir_free(selItems);
							if (itemData->colour_id >= 0) 
							{
								SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, WM_LBUTTONUP, 0, 0);
							}
							return TRUE;
						}

						CreateFromFontSettings(&font_id_list_w2.items[itemData->font_id]->value, &lf, font_id_list_w2.items[itemData->font_id]->flags);
						CreateFontIndirect(&lf);

						cf.lStructSize = sizeof(cf);
						cf.hwndOwner = hwndDlg;
						cf.lpLogFont = &lf;
						if ( font_id_list_w2.items[itemData->font_id]->flags & FIDF_ALLOWEFFECTS )
						{
							cf.Flags = CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS | CF_EFFECTS | CF_ENABLETEMPLATE | CF_ENABLEHOOK;
							// use custom font dialog to disable colour selection
							cf.hInstance = GetModuleHandle(NULL);
							cf.lpTemplateName = MAKEINTRESOURCE(IDD_CUSTOM_FONT);
							cf.lpfnHook = CFHookProc;
						}
						else cf.Flags = CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;

						if (ChooseFont(&cf))
						{
							for (i = 0; i < selCount; ++i)
							{
								FSUIListItemData *itemData = (FSUIListItemData *)SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETITEMDATA, selItems[i], 0);
								if (itemData->font_id < 0) continue;
								font_id_list_w2.items[itemData->font_id]->value.size = (char)lf.lfHeight;
								font_id_list_w2.items[itemData->font_id]->value.style = (lf.lfWeight >= FW_BOLD ? DBFONTF_BOLD : 0) | (lf.lfItalic ? DBFONTF_ITALIC : 0) | (lf.lfUnderline ? DBFONTF_UNDERLINE : 0) | (lf.lfStrikeOut ? DBFONTF_STRIKEOUT : 0);
								font_id_list_w2.items[itemData->font_id]->value.charset = lf.lfCharSet;
								_tcscpy(font_id_list_w2.items[itemData->font_id]->value.szFace, lf.lfFaceName);
								{
									MEASUREITEMSTRUCT mis = { 0 };
									mis.CtlID = IDC_FONTLIST;
									mis.itemID = selItems[i];
									mis.itemData = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETITEMDATA, selItems[i], 0);
									SendMessage(hwndDlg, WM_MEASUREITEM, 0, (LPARAM) & mis);
									SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_SETITEMHEIGHT, selItems[i], mis.itemHeight);
								}
							}
							InvalidateRect(GetDlgItem(hwndDlg, IDC_FONTLIST), NULL, TRUE);
							EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_UNDO), TRUE);
							break;
						}

						mir_free(selItems);
					}
					return TRUE;
				}

				case IDC_FONTCOLOUR:
				{
					int selCount, i;
					if (selCount = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETSELCOUNT, 0, 0))
					{
						int *selItems = (int *)mir_alloc(selCount * sizeof(int));
						SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETSELITEMS, (WPARAM)selCount, (LPARAM) selItems);
						for (i = 0; i < selCount; i++)
						{
							FSUIListItemData *itemData = (FSUIListItemData *)SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETITEMDATA, selItems[i], 0);
							if (itemData->font_id < 0) continue;
							font_id_list_w2.items[itemData->font_id]->value.colour = SendDlgItemMessage(hwndDlg, IDC_FONTCOLOUR, CPM_GETCOLOUR, 0, 0);
						}
						mir_free(selItems);
						InvalidateRect(GetDlgItem(hwndDlg, IDC_FONTLIST), NULL, FALSE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_UNDO), TRUE);
					}
					break;
				}

				case IDC_BKGCOLOUR:
				{
					int selCount, i;
					if (selCount = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETSELCOUNT, 0, 0))
					{
						int *selItems = (int *)mir_alloc(selCount * sizeof(int));
						SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETSELITEMS, (WPARAM)selCount, (LPARAM) selItems);
						for (i = 0; i < selCount; i++)
						{
							FSUIListItemData *itemData = (FSUIListItemData *)SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETITEMDATA, selItems[i], 0);
							if (itemData->colour_id < 0) continue;
							colour_id_list_w2.items[itemData->colour_id]->value = SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_GETCOLOUR, 0, 0);
						}
						mir_free(selItems);
						InvalidateRect(GetDlgItem(hwndDlg, IDC_FONTLIST), NULL, FALSE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_UNDO), TRUE);
					}
					break;
				}

				case IDC_BTN_RESET:
				{
					int selCount;
					if (font_id_list_w2.count && (selCount = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETSELCOUNT, (WPARAM)0, (LPARAM)0)))
					{
						int *selItems = (int *)mir_alloc(font_id_list_w2.count * sizeof(int));
						SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETSELITEMS, (WPARAM)selCount, (LPARAM)selItems);
						for (i = 0; i < selCount; ++i)
						{
							FSUIListItemData *itemData = (FSUIListItemData *)SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETITEMDATA, selItems[i], 0);
							if (IsBadReadPtr(itemData, sizeof(FSUIListItemData))) continue; // prevent possible problems with corrupted itemData

							if((itemData->font_id >= 0) && (font_id_list_w2.items[itemData->font_id]->flags & FIDF_DEFAULTVALID)) {
								font_id_list_w2.items[itemData->font_id]->value = font_id_list_w2.items[itemData->font_id]->deffontsettings;
								{
									MEASUREITEMSTRUCT mis = { 0 };
									mis.CtlID = IDC_FONTLIST;
									mis.itemID = selItems[i];
									mis.itemData = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETITEMDATA, selItems[i], 0);
									SendMessage(hwndDlg, WM_MEASUREITEM, 0, (LPARAM) & mis);
									SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_SETITEMHEIGHT, selItems[i], mis.itemHeight);
								}
							}

							if (itemData->colour_id >= 0)
							{
								colour_id_list_w2.items[itemData->colour_id]->value = colour_id_list_w2.items[itemData->colour_id]->defcolour;
							}
						}
						mir_free(selItems);
						InvalidateRect(GetDlgItem(hwndDlg, IDC_FONTLIST), NULL, FALSE);
						SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_FONTLIST, LBN_SELCHANGE), 0);
						EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_UNDO), TRUE);
					}
					break;
				}

				case IDC_BTN_UNDO:
				{
					sttCopyList(( SortedList* )&font_id_list_w3,   ( SortedList* )&font_id_list_w2,   sizeof( TFontID ));
					sttCopyList(( SortedList* )&colour_id_list_w3, ( SortedList* )&colour_id_list_w2, sizeof( TColourID ));
					EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_UNDO), FALSE);
			
					SendMessage(hwndDlg, UM_SETFONTGROUP, 0, 0);
					break;
				}

				case IDC_BTN_EXPORT:
				{
					TCHAR fname_buff[MAX_PATH];
					OPENFILENAME ofn = {0};
					ofn.lStructSize = sizeof(ofn);
					ofn.lpstrFile = fname_buff;
					ofn.lpstrFile[0] = '\0';
					ofn.nMaxFile = MAX_PATH;
					ofn.hwndOwner = hwndDlg;
					ofn.Flags = OFN_NOREADONLYRETURN | OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT;
					ofn.lpstrFilter = _T("Configuration Files (*.ini)\0*.ini\0Text Files (*.txt)\0*.TXT\0All Files (*.*)\0*.*\0");
					ofn.nFilterIndex = 1;

					ofn.lpstrDefExt = _T("ini");

					if ( GetSaveFileName( &ofn ) == TRUE )
						if ( !ExportSettings( hwndDlg, ofn.lpstrFile, &font_id_list, &colour_id_list ))
							MessageBox(hwndDlg, _T("Error writing file"), _T("Error"), MB_ICONWARNING | MB_OK);

					return TRUE;
				}
			}
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;

		case WM_NOTIFY:
			if (((LPNMHDR) lParam)->idFrom == 0 && ((LPNMHDR) lParam)->code == PSN_APPLY ) {
				char str[32];

				sttCopyList(( SortedList* )&font_id_list,   ( SortedList* )&font_id_list_w3,   sizeof( TFontID ));
				sttCopyList(( SortedList* )&colour_id_list, ( SortedList* )&colour_id_list_w3, sizeof( TColourID ));

				EnableWindow( GetDlgItem(hwndDlg, IDC_BTN_UNDO), TRUE );

				sttCopyList(( SortedList* )&font_id_list_w2,   ( SortedList* )&font_id_list,   sizeof( TFontID ));
				sttCopyList(( SortedList* )&colour_id_list_w2, ( SortedList* )&colour_id_list, sizeof( TColourID ));

				for ( i=0; i < font_id_list_w2.count; i++ ) {
					TFontID* F = font_id_list_w2.items[i];

					if ( F->flags & FIDF_APPENDNAME )
						wsprintfA(str, "%sName", F->prefix);
					else
						wsprintfA(str, "%s", F->prefix);

					if ( DBWriteContactSettingTString( NULL, F->dbSettingsGroup, str, F->value.szFace )) {
						#if defined( _UNICODE )
							char buff[1024];
							WideCharToMultiByte(code_page, 0, F->value.szFace, -1, buff, 1024, 0, 0);
							DBWriteContactSettingString(NULL, F->dbSettingsGroup, str, buff);
						#endif
					}
					
					wsprintfA(str, "%sSize", F->prefix);
					if ( F->flags & FIDF_SAVEACTUALHEIGHT ) {
						HDC hdc;
						SIZE size;
						HFONT hFont, hOldFont;
						CreateFromFontSettings( &F->value, &lf, F->flags );
						hFont = CreateFontIndirect( &lf );
						hdc = GetDC(hwndDlg);
						hOldFont = (HFONT)SelectObject( hdc, hFont );
						GetTextExtentPoint32( hdc, _T("_W"), 2, &size);
						ReleaseDC(hwndDlg, hdc);
						SelectObject(hdc, hOldFont);
						DeleteObject(hFont);

						DBWriteContactSettingByte(NULL, F->dbSettingsGroup, str, (char)size.cy);
					}
					else if ( F->flags & FIDF_SAVEPOINTSIZE ) {
						HDC hdc = GetDC(hwndDlg);
						DBWriteContactSettingByte(NULL, F->dbSettingsGroup, str, (BYTE)-MulDiv(F->value.size, 72, GetDeviceCaps(hdc, LOGPIXELSY)));
						ReleaseDC(hwndDlg, hdc);
					}
					else DBWriteContactSettingByte(NULL, F->dbSettingsGroup, str, F->value.size);

					wsprintfA(str, "%sSty", F->prefix);
					DBWriteContactSettingByte(NULL, F->dbSettingsGroup, str, F->value.style);
					wsprintfA(str, "%sSet", F->prefix);
					DBWriteContactSettingByte(NULL, F->dbSettingsGroup, str, F->value.charset);
					wsprintfA(str, "%sCol", F->prefix);
					DBWriteContactSettingDword(NULL, F->dbSettingsGroup, str, F->value.colour);
					if ( F->flags & FIDF_NOAS ) {
						wsprintfA(str, "%sAs", F->prefix);
						DBWriteContactSettingWord(NULL, F->dbSettingsGroup, str, (WORD)0x00FF);
					}
					wsprintfA(str, "%sFlags", F->prefix);
					DBWriteContactSettingWord(NULL, F->dbSettingsGroup, str, (WORD)F->flags);
				}

				for ( i=0; i < colour_id_list_w2.count; i++ ) {
					TColourID* C = colour_id_list_w2.items[i];

					wsprintfA(str, "%s", C->setting);
					DBWriteContactSettingDword(NULL, C->dbSettingsGroup, str, C->value);
				}
			
				OptionsChanged();
				return TRUE;
			} else
			if (((LPNMHDR) lParam)->idFrom == IDC_FONTGROUP)
			{
				switch(((NMHDR*)lParam)->code) {
				case TVN_SELCHANGEDA: // !!!! This needs to be here - both !!
				case TVN_SELCHANGEDW:
					{
						SendMessage(hwndDlg, UM_SETFONTGROUP, 0, 0);
						break;
					}
				case TVN_DELETEITEMA: // no idea why both TVN_SELCHANGEDA/W should be there but let's keep this both too...
				case TVN_DELETEITEMW:
					{
						TCHAR *name = (TCHAR *)(((LPNMTREEVIEW)lParam)->itemOld.lParam);
						if (name) mir_free(name);
						break;
					}
				}				
			}
			break;

		case WM_DESTROY:
			KillTimer(hwndDlg, TIMER_ID);
			sttSaveCollapseState(GetDlgItem(hwndDlg, IDC_FONTGROUP));
			DeleteObject(hBkgColourBrush);
			DestroyList(( SortedList* )&font_id_list_w2 );
			DestroyList(( SortedList* )&font_id_list_w3 );
			DestroyList(( SortedList* )&colour_id_list_w2 );
			DestroyList(( SortedList* )&colour_id_list_w3 );
			break;
	}
	return FALSE;
}

int OptInit(WPARAM wParam, LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp = {0};

	odp.cbSize						= sizeof(odp);
	odp.cbSize						= OPTIONPAGE_OLD_SIZE2;
	odp.position					= -790000000;
	odp.hInstance					= GetModuleHandle(NULL);;
	odp.pszTemplate				= MAKEINTRESOURCEA(IDD_OPT_FONTS);
	odp.pszTitle					= "Fonts";
	odp.pszGroup					= "Customize";
	odp.flags						= ODPF_BOLDGROUPS;
	odp.nIDBottomSimpleControl	= 0;
	odp.pfnDlgProc					= DlgProcLogOptions;
	CallService( MS_OPT_ADDPAGE, wParam,( LPARAM )&odp );	
	return 0;
}
