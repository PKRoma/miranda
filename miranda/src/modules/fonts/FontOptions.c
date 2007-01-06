/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2006 Miranda ICQ/IM project, Scott Ellis (mail@scottellis.com.au)
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

FontIdList   font_id_list_w2,   font_id_list_w3;
ColourIdList colour_id_list_w2, colour_id_list_w3;

#define M_SETFONTGROUP		(WM_USER + 101)
#define TIMER_ID				11015

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
				if ( SendDlgItemMessage(hwndDlg, IDC_FONTGROUP, LB_FINDSTRINGEXACT, (WPARAM)-1, (WPARAM)F->group) == CB_ERR)
					SendDlgItemMessage(hwndDlg, IDC_FONTGROUP, LB_ADDSTRING, (WPARAM)-1, (WPARAM)F->group);
			}

			for ( i = 0; i < colour_id_list_w2.count; i++  ) {
				TColourID* C = colour_id_list_w2.items[i];

				// sync settings with database
				UpdateColourSettings( C, &C->value );
			}

			SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_SETDEFAULTCOLOUR, 0, (LPARAM)GetSysColor(COLOR_WINDOW));

			if ( font_id_list_w2.count )
				SendDlgItemMessage(hwndDlg, IDC_FONTGROUP, LB_SETCURSEL, 0, 0);
			SendMessage(hwndDlg, M_SETFONTGROUP, 0, 0);
			return TRUE;
		}
		case M_SETFONTGROUP:
		{
			int sel = SendDlgItemMessage(hwndDlg, IDC_FONTGROUP, LB_GETCURSEL, 0, 0);
			if ( sel != -1 ) {
				TCHAR group_buff[64];
				SendDlgItemMessage(hwndDlg, IDC_FONTGROUP, LB_GETTEXT, sel, (LPARAM)group_buff);

				SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_RESETCONTENT, 0, 0);
				SendDlgItemMessage(hwndDlg, IDC_COLOURLIST, CB_RESETCONTENT, 0, 0);
				{
					BOOL need_restart = FALSE;
					int fontId = 0, itemId;
					int first_font_index = -1;
					int colourId = 0;
					int first_colour_index = -1;

					for ( i = 0; i < font_id_list_w2.count; i++, fontId++ ) {
						TFontID* F = font_id_list_w2.items[i];
						if ( _tcsncmp( F->group, group_buff, 64 ) == 0 ) {
							if ( first_font_index == -1 )
								first_font_index = fontId;

							itemId = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_ADDSTRING, (WPARAM)-1, fontId);
							need_restart |= (F->flags & FIDF_NEEDRESTART);
					}	}

					ShowWindow( GetDlgItem(hwndDlg, IDC_STAT_RESTART), (need_restart ? SW_SHOW : SW_HIDE));

					if ( hBkgColourBrush ) {
						DeleteObject( hBkgColourBrush );
						hBkgColourBrush = 0;
					}

					for ( i = 0; i < colour_id_list_w2.count; i++, colourId++ ) {
						TColourID* C = colour_id_list_w2.items[i];
						if ( _tcsncmp( C->group, group_buff, 64 ) == 0 ) {
							if ( first_colour_index == -1 )
								first_colour_index = colourId;

							itemId = SendDlgItemMessage(hwndDlg, IDC_COLOURLIST, CB_ADDSTRING, (WPARAM)-1, (LPARAM)TranslateTS( C->name ));
							SendDlgItemMessage(hwndDlg, IDC_COLOURLIST, CB_SETITEMDATA, itemId, colourId);

							if ( _tcscmp( C->name, TranslateT( "Background" )) == 0 )
								hBkgColourBrush = CreateSolidBrush( C->value );
					}	}

					if ( !hBkgColourBrush )
						hBkgColourBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
					
					SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_SETSEL, TRUE, 0);
					SendDlgItemMessage(hwndDlg, IDC_FONTCOLOUR, CPM_SETCOLOUR, 0, (LPARAM)font_id_list_w2.items[first_font_index]->value.colour);
					SendDlgItemMessage(hwndDlg, IDC_FONTCOLOUR, CPM_SETDEFAULTCOLOUR, 0, (LPARAM)font_id_list_w2.items[first_font_index]->deffontsettings.colour);

					if ( font_id_list_w2.items[first_font_index]->flags & FIDF_DEFAULTVALID )
						EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_RESET), TRUE);
					else
						EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_RESET), FALSE);

					if ( first_colour_index == -1 ) {
						HWND hw = GetDlgItem(hwndDlg, IDC_COLOURLIST);
						EnableWindow(hw, FALSE);
						hw = GetDlgItem(hwndDlg, IDC_BKGCOLOUR);
						EnableWindow(hw, FALSE);

					}
					else {
						HWND hw = GetDlgItem(hwndDlg, IDC_COLOURLIST);
						EnableWindow(hw, TRUE);
						hw = GetDlgItem(hwndDlg, IDC_BKGCOLOUR);
						EnableWindow(hw, TRUE);

						SendDlgItemMessage(hwndDlg, IDC_COLOURLIST, CB_SETCURSEL, 0, 0);
						SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_SETCOLOUR, 0, (LPARAM)colour_id_list_w2.items[first_colour_index]->value );
						SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_SETDEFAULTCOLOUR, 0, colour_id_list_w2.items[first_colour_index]->defcolour );
			}	}	}
			return TRUE;
		}
		case WM_CTLCOLORLISTBOX:
			if((HWND)lParam == GetDlgItem(hwndDlg, IDC_FONTLIST))
				return (BOOL) hBkgColourBrush;
			break;

		case WM_MEASUREITEM:
		{
			MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT *) lParam;
			if(mis->CtlID == IDC_FONTLIST && font_id_list_w2.count) {
				HFONT hFont, hoFont;
				SIZE fontSize;
				int iItem = mis->itemData;
				//int iItem = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETITEMDATA, mis->itemID, 0);
				HDC hdc = GetDC(GetDlgItem(hwndDlg, mis->CtlID));
				CreateFromFontSettings( &font_id_list_w2.items[iItem]->value, &lf, font_id_list_w2.items[iItem]->flags);
				hFont = CreateFontIndirect(&lf);

				//hFont = CreateFontA(font_id_list2[iItem]->value.size, 0, 0, 0,
				//					font_id_list2[iItem]->value.style & DBFONTF_BOLD ? FW_BOLD : FW_NORMAL,
				//					font_id_list2[iItem]->value.style & DBFONTF_ITALIC ? 1 : 0, 0, 0, font_id_list2[iItem]->value.charset, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, font_id_list2[iItem]->value.szFace);
				hoFont = (HFONT) SelectObject(hdc, hFont);
				GetTextExtentPoint32(hdc, font_id_list_w2.items[iItem]->name, _tcslen(font_id_list_w2.items[iItem]->name), &fontSize);
				SelectObject(hdc, hoFont);
				ReleaseDC(GetDlgItem(hwndDlg, mis->CtlID), hdc);
				DeleteObject(hFont);
				mis->itemWidth = fontSize.cx;
				mis->itemHeight = fontSize.cy;
				return TRUE;
			}
			break;
		}
		case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *) lParam;
			if(dis->CtlID == IDC_FONTLIST && font_id_list_w2.count) {
				HBRUSH hBrush = NULL;
				HFONT hFont, hoFont;
				TCHAR* pszText;
				int i;
				int iItem = dis->itemData;
				CreateFromFontSettings(&font_id_list_w2.items[iItem]->value, &lf, font_id_list_w2.items[iItem]->flags);
				hFont = CreateFontIndirect(&lf);
				//hFont = CreateFontA(font_id_list2[iItem]->value.size, 0, 0, 0,
				//					font_id_list2[iItem]->value.style & DBFONTF_BOLD ? FW_BOLD : FW_NORMAL,
				//					font_id_list2[iItem]->value.style & DBFONTF_ITALIC ? 1 : 0, 0, 0, font_id_list2[iItem]->value.charset, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, font_id_list2[iItem]->value.szFace);
				hoFont = (HFONT) SelectObject(dis->hDC, hFont);
				SetBkMode(dis->hDC, TRANSPARENT);
				for ( i = 0; i < colour_id_list_w2.count; i++) {
					TColourID* C = colour_id_list_w2.items[i];
					if ( _tcsncmp( C->group, font_id_list_w2.items[iItem]->backgroundGroup, 64 ) == 0 ) {
						if ( _tcsncmp( C->name, font_id_list_w2.items[iItem]->backgroundName, 64 ) == 0 ) {
							hBrush = CreateSolidBrush( C->value );
							FillRect(dis->hDC, &dis->rcItem, hBrush);
							DeleteObject(hBrush);
						}
				}	}
				if (!hBrush) {
					FillRect(dis->hDC, &dis->rcItem, hBkgColourBrush);
				}
				if (dis->itemState & ODS_SELECTED)
					FrameRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_HIGHLIGHT));
				SetTextColor(dis->hDC, font_id_list_w2.items[iItem]->value.colour);
				pszText = TranslateTS(font_id_list_w2.items[iItem]->name);
				TextOut(dis->hDC, dis->rcItem.left, dis->rcItem.top, pszText, _tcslen(pszText));
				SelectObject(dis->hDC, hoFont);
				DeleteObject(hFont);
				return TRUE;
			}
			break;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_COLOURLIST:
					if (HIWORD(wParam) == LBN_SELCHANGE) {
						int sel = SendDlgItemMessage(hwndDlg, IDC_COLOURLIST, CB_GETCURSEL, 0, 0);
						if(sel != -1) {
							int i = SendDlgItemMessage(hwndDlg, IDC_COLOURLIST, CB_GETITEMDATA, sel, 0);
							SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_SETCOLOUR, 0, colour_id_list_w2.items[i]->value );
							SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_SETDEFAULTCOLOUR, 0, colour_id_list_w2.items[i]->defcolour );
					}	}

					if (HIWORD(wParam) != LBN_DBLCLK)
						return TRUE;
					//fall through
				case IDC_BKGCOLOUR:

					{
						int sel = SendDlgItemMessage(hwndDlg, IDC_COLOURLIST, CB_GETCURSEL, 0, 0);
						if(sel != -1) {
							int i = SendDlgItemMessage(hwndDlg, IDC_COLOURLIST, CB_GETITEMDATA, sel, 0);
							colour_id_list_w2.items[i]->value = SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_GETCOLOUR, 0, 0);
							if ( _tcscmp(colour_id_list_w2.items[i]->name, TranslateT( "Background" )) == 0) {
								DeleteObject(hBkgColourBrush);
								hBkgColourBrush = CreateSolidBrush(SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_GETCOLOUR, 0, 0));
					}	}	}
					InvalidateRect(GetDlgItem(hwndDlg, IDC_FONTLIST), NULL, TRUE);
					break;

				case IDC_FONTGROUP:
					if (HIWORD(wParam) == LBN_SELCHANGE) {
						SendMessage(hwndDlg, M_SETFONTGROUP, 0, 0);
					}
					return TRUE;
				case IDC_FONTLIST:
					if (HIWORD(wParam) == LBN_SELCHANGE) {
						if(font_id_list_w2.count) {
							int *selItems = (int *)mir_alloc(font_id_list_w2.count * sizeof(int));
							int sel, selCount, i;

							selCount = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETSELITEMS, (WPARAM)font_id_list_w2.count, (LPARAM) selItems);

							if (selCount > 1) {
								BOOL show_default = FALSE;
								SendDlgItemMessage(hwndDlg, IDC_FONTCOLOUR, CPM_SETCOLOUR, 0, GetSysColor(COLOR_3DFACE));
								SendDlgItemMessage(hwndDlg, IDC_FONTCOLOUR, CPM_SETDEFAULTCOLOUR, 0, GetSysColor(COLOR_WINDOWTEXT));

								for (sel = 0; sel < selCount; sel++) {
									i = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETITEMDATA, selItems[sel], 0);
									show_default |= (font_id_list_w2.items[i]->flags & FIDF_DEFAULTVALID);
								}
								if(show_default)
									EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_RESET), TRUE);
								else
									EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_RESET), FALSE);
							}
							else {
								int i = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETITEMDATA,
														   SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETCURSEL, 0, 0), 0);
								SendDlgItemMessage(hwndDlg, IDC_FONTCOLOUR, CPM_SETCOLOUR, 0, font_id_list_w2.items[i]->value.colour);
								SendDlgItemMessage(hwndDlg, IDC_FONTCOLOUR, CPM_SETDEFAULTCOLOUR, 0, (LPARAM)font_id_list_w2.items[i]->deffontsettings.colour);

								if(font_id_list_w2.items[i]->flags & FIDF_DEFAULTVALID) {
									EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_RESET), TRUE);
								} else
									EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_RESET), FALSE);
							}
							mir_free(selItems);
						}
						return TRUE;
					}
					if (HIWORD(wParam) != LBN_DBLCLK)
						return TRUE;
					//fall through
				case IDC_CHOOSEFONT:
				{
					if ( SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETCOUNT, 0, 0 )) {
						int sel = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETCURSEL, 0, 0);
						int i = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETITEMDATA, sel, 0);
						CHOOSEFONT cf = { 0 };

						CreateFromFontSettings(&font_id_list_w2.items[i]->value, &lf, font_id_list_w2.items[i]->flags);
						CreateFontIndirect(&lf);

						cf.lStructSize = sizeof(cf);
						cf.hwndOwner = hwndDlg;
						cf.lpLogFont = &lf;
						if ( font_id_list_w2.items[i]->flags & FIDF_ALLOWEFFECTS ) {
							cf.Flags = CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS | CF_EFFECTS | CF_ENABLETEMPLATE | CF_ENABLEHOOK;
							// use custom font dialog to disable colour selection
							cf.hInstance = GetModuleHandle(NULL);
							cf.lpTemplateName = MAKEINTRESOURCE(IDD_CUSTOM_FONT);
							cf.lpfnHook = CFHookProc;
						}
						else cf.Flags = CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;

						if (ChooseFont(&cf)) {
							int *selItems = (int *)mir_alloc(font_id_list_w2.count * sizeof(int));
							int sel, selCount;

							selCount = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETSELITEMS, (WPARAM)font_id_list_w2.count, (LPARAM) selItems);
							for (sel = 0; sel < selCount; sel++) {
								i = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETITEMDATA, selItems[sel], 0);
								font_id_list_w2.items[i]->value.size = (char)lf.lfHeight;
								font_id_list_w2.items[i]->value.style = (lf.lfWeight >= FW_BOLD ? DBFONTF_BOLD : 0) | (lf.lfItalic ? DBFONTF_ITALIC : 0) | (lf.lfUnderline ? DBFONTF_UNDERLINE : 0) | (lf.lfStrikeOut ? DBFONTF_STRIKEOUT : 0);
								font_id_list_w2.items[i]->value.charset = lf.lfCharSet;
								_tcscpy(font_id_list_w2.items[i]->value.szFace, lf.lfFaceName);
								{
									MEASUREITEMSTRUCT mis = { 0 };
									mis.CtlID = IDC_FONTLIST;
									mis.itemData = i;
									SendMessage(hwndDlg, WM_MEASUREITEM, 0, (LPARAM) & mis);
									SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_SETITEMHEIGHT, selItems[sel], mis.itemHeight);
							}	}

							mir_free(selItems);
							InvalidateRect(GetDlgItem(hwndDlg, IDC_FONTLIST), NULL, TRUE);
							break;
						}
					}
					return TRUE;
				}
				case IDC_FONTCOLOUR:
				{
					if(font_id_list_w2.count) {
						int *selItems = (int *)mir_alloc(font_id_list_w2.count * sizeof(int));
						int sel, selCount, i;

						selCount = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETSELITEMS, (WPARAM)font_id_list_w2.count, (LPARAM) selItems);
						for (sel = 0; sel < selCount; sel++) {
							i = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETITEMDATA, selItems[sel], 0);
							font_id_list_w2.items[i]->value.colour = SendDlgItemMessage(hwndDlg, IDC_FONTCOLOUR, CPM_GETCOLOUR, 0, 0);
						}
						mir_free(selItems);
						InvalidateRect(GetDlgItem(hwndDlg, IDC_FONTLIST), NULL, FALSE);
					}
					break;
				}
				case IDC_BTN_RESET:
				{
					if(font_id_list_w2.count) {
						int *selItems = (int *)mir_alloc(font_id_list_w2.count * sizeof(int));
						int sel, selCount, i;

						selCount = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETSELITEMS, (WPARAM)font_id_list_w2.count, (LPARAM) selItems);
						for (sel = 0; sel < selCount; sel++) {
							i = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETITEMDATA, selItems[sel], 0);
							if(font_id_list_w2.items[i]->flags & FIDF_DEFAULTVALID) {
								font_id_list_w2.items[i]->value = font_id_list_w2.items[i]->deffontsettings;
								{
									MEASUREITEMSTRUCT mis = { 0 };
									mis.CtlID = IDC_FONTLIST;
									mis.itemData = i;
									SendMessage(hwndDlg, WM_MEASUREITEM, 0, (LPARAM) & mis);
									SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_SETITEMHEIGHT, selItems[sel], mis.itemHeight);
								}
							}
						}
						if (selCount > 1) {
							SendDlgItemMessage(hwndDlg, IDC_FONTCOLOUR, CPM_SETCOLOUR, 0, GetSysColor(COLOR_3DFACE));
							SendDlgItemMessage(hwndDlg, IDC_FONTCOLOUR, CPM_SETDEFAULTCOLOUR, 0, GetSysColor(COLOR_WINDOWTEXT));
						}
						else {
							int i = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETITEMDATA,
													   SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETCURSEL, 0, 0), 0);
							SendDlgItemMessage(hwndDlg, IDC_FONTCOLOUR, CPM_SETCOLOUR, 0, font_id_list_w2.items[i]->value.colour);
						}
						mir_free(selItems);
						InvalidateRect(GetDlgItem(hwndDlg, IDC_FONTLIST), NULL, FALSE);
					}
					break;
				}
				case IDC_BTN_UNDO:
				{
					sttCopyList(( SortedList* )&font_id_list_w3,   ( SortedList* )&font_id_list_w2,   sizeof( TFontID ));
					sttCopyList(( SortedList* )&colour_id_list_w3, ( SortedList* )&colour_id_list_w2, sizeof( TColourID ));
					EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_UNDO), FALSE);
			
					SendDlgItemMessage(hwndDlg, IDC_FONTGROUP, LB_SETCURSEL, 0, 0);
					SendMessage(hwndDlg, M_SETFONTGROUP, 0, 0);
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
					ofn.lpstrFilter = _T("INI\0*.ini\0Text\0*.TXT\0All\0*.*\0");
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
			}
			break;

		case WM_DESTROY:
			KillTimer(hwndDlg, TIMER_ID);
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
