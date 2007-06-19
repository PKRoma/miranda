////////////////////////////////////////////////////////////////////////////////
// Gadu-Gadu Plugin for Miranda IM
//
// Copyright (c) 2003-2006 Adam Strzelecki <ono+miranda@java.pl>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
////////////////////////////////////////////////////////////////////////////////

#include "gg.h"
#include <ocidl.h>
#include <olectl.h>

char strtokenid[255];
char strtokenval[255];
char *ggTokenid = NULL;
char *ggTokenval = NULL;
LPPICTURE tokenPicture = NULL;

int tokenwidth = 0;
int tokenheight = 0;

#define MAX_LOADSTRING 100
#define HIMETRIC_INCH 2540
#define MAP_LOGHIM_TO_PIX(x,ppli) ( ((ppli)*(x) + HIMETRIC_INCH/2) / HIMETRIC_INCH )

////////////////////////////////////////////////////////////////////////////////
// User Util Dlg Page : Data
BOOL CALLBACK gg_tokendlgproc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
		case WM_INITDIALOG:
			TranslateDialogDefault(hwndDlg);
			RECT rc; GetClientRect(GetDlgItem(hwndDlg, IDC_WHITERECT), &rc);
			InvalidateRect(hwndDlg, &rc, TRUE);
			return TRUE;

		case WM_CTLCOLORSTATIC:
			/*
			if((GetDlgItem(hwndDlg, IDC_WHITERECT) == (HWND)lParam) ||
				(GetDlgItem(hwndDlg, IDC_LOGO) == (HWND)lParam))
			{
				SetBkColor((HDC)wParam,RGB(255,255,255));
				return (BOOL)GetStockObject(WHITE_BRUSH);
			}
			*/
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:
				{
					GetDlgItemText(hwndDlg, IDC_TOKEN, strtokenval, sizeof(strtokenval));
					EndDialog(hwndDlg, IDOK);
					break;
				}
				case IDCANCEL:
					EndDialog(hwndDlg, IDCANCEL);
					break;
			}
			break;
		case WM_PAINT:
			{
				PAINTSTRUCT paintStruct;
				HDC hdc = BeginPaint(hwndDlg, &paintStruct);
				RECT rc; GetClientRect(GetDlgItem(hwndDlg, IDC_WHITERECT), &rc);
				FillRect(hdc, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));

				if (tokenPicture)
				{
					// get width and height of picture
					long hmWidth;
					long hmHeight;
					tokenPicture->get_Width(&hmWidth);
					tokenPicture->get_Height(&hmHeight);

					// convert himetric to pixels
					int nWidth = MulDiv(hmWidth, GetDeviceCaps(hdc, LOGPIXELSX), HIMETRIC_INCH);
					int nHeight = MulDiv(hmHeight, GetDeviceCaps(hdc, LOGPIXELSY), HIMETRIC_INCH);

					// display picture using IPicture::Render
					tokenPicture->Render(hdc,
						(rc.right - tokenwidth) / 2,
						(rc.bottom - rc.top - tokenheight) / 2,
						nWidth, nHeight, 0, hmHeight, hmWidth, -hmHeight, &rc);
				}
				EndPaint(hwndDlg, &paintStruct);
				return 0;
			}
			break;

		case WM_DESTROY:
			break;
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// Gets GG token
int gg_gettoken()
{
	struct gg_http *h = NULL;
	struct gg_token *t = NULL;

	// Zero tokens
	ggTokenid = NULL;
	ggTokenval = NULL;

	if(!(h = gg_token(0)) || gg_token_watch_fd(h) || h->state == GG_STATE_ERROR || h->state != GG_STATE_DONE)
	{
		char error[128];
		mir_snprintf(error, sizeof(error), Translate("Token retrieval failed because of error:\n\t%s"), http_error_string(h ? h->error : 0));
		MessageBox(
				NULL,
				error,
				GG_PROTOERROR,
				MB_OK | MB_ICONSTOP
		);
		gg_free_pubdir(h);
		return FALSE;
	}

	if (!(t = (struct gg_token *)h->data) || (!h->body))
	{
		char error[128];
		mir_snprintf(error, sizeof(error), Translate("Token retrieval failed because of error:\n\t%s"), http_error_string(h ? h->error : 0));
		MessageBox(
				NULL,
				error,
				GG_PROTOERROR,
				MB_OK | MB_ICONSTOP
		);
		gg_free_pubdir(h);
		return FALSE;
	}

	// Return token id
	strncpy(strtokenid, t->tokenid, sizeof(strtokenid));
	tokenwidth = t->width;
	tokenheight = t->height;
	// Load bitmap
	HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, h->body_size);
	// Lock memory for reading
	LPVOID pvData = GlobalLock(hGlobal);
	if (pvData == NULL)
	{
		GlobalUnlock(hGlobal);
		MessageBox(
				NULL,
				Translate("Could not lock memory for token."),
				GG_PROTOERROR,
				MB_OK | MB_ICONSTOP
		);
		gg_free_pubdir(h);
		return FALSE;
	}
	memcpy(pvData, h->body, h->body_size);
	GlobalUnlock(hGlobal);

	// Create IStream* from global memory
	LPSTREAM pstm = NULL;
	HRESULT hr = CreateStreamOnHGlobal(hGlobal,TRUE, &pstm);
	if (!(SUCCEEDED(hr)) || (pstm == NULL))
	{
		MessageBox(
				NULL,
				Translate("CreateStreamOnHGlobal() failed for token."),
				GG_PROTOERROR,
				MB_OK | MB_ICONSTOP
		);

		if (pstm != NULL) pstm->Release();
		gg_free_pubdir(h);
		return FALSE;
	}

	// Create IPicture from image file
	if (tokenPicture) tokenPicture->Release();
	hr = OleLoadPicture(pstm, h->body_size, FALSE, IID_IPicture,
						  (LPVOID *)&tokenPicture);

	// Free gg structs
	gg_free_pubdir(h);
	// Check results
	if (!(SUCCEEDED(hr)) || (tokenPicture == NULL))
	{
		pstm->Release();
		MessageBox(
				NULL,
				Translate("Could not load image (hr failure) for token."),
				GG_PROTOERROR,
				MB_OK | MB_ICONSTOP
		);
		return FALSE;
	}
	pstm->Release();

	// Load token dialog
	if(DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_TOKEN), NULL, gg_tokendlgproc, 0) == IDCANCEL)
	{
		return FALSE;
	}

	// Fillup patterns
	ggTokenid = strtokenid;
	ggTokenval = strtokenval;
	return TRUE;
}
