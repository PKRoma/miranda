/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2004 Miranda ICQ/IM project, 
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

extern int g_protocount;
extern struct protoPicCacheEntry *g_MyAvatars;
extern int GetImageFormat(char *filename);

#define DM_AVATARCHANGED (WM_USER + 20)
#define DM_MYAVATARCHANGED (WM_USER + 21)

typedef struct 
{
	HANDLE hContact;
	char proto[64];
    HANDLE hHook;
    HANDLE hHookMy;
	HFONT hFont;   // font
	COLORREF borderColor;
	COLORREF bkgColor;
	COLORREF avatarBorderColor;
	int avatarRoundCornerRadius;
	char noAvatarText[128];
	BOOL respectHidden;
	BOOL showingFlash;
	BOOL resizeIfSmaller;

} ACCData;


void ResizeFlash(HWND hwnd, ACCData* data)
{
	if ((data->hContact != NULL || data->proto[0] != '\0')
		&& ServiceExists(MS_FAVATAR_RESIZE))
	{
		RECT rc;
		GetClientRect(hwnd, &rc);

		if (data->borderColor != -1 || data->avatarBorderColor != -1)
		{
			rc.left ++;
			rc.right -= 2;
			rc.top ++;
			rc.bottom -= 2;
		}

		FLASHAVATAR fa = {0}; 
        fa.hContact = data->hContact;
		fa.cProto = data->proto;
		fa.hParentWindow = hwnd;
        fa.id = 1675;
		CallService(MS_FAVATAR_RESIZE, (WPARAM)&fa, (LPARAM)&rc);
		CallService(MS_FAVATAR_SETPOS, (WPARAM)&fa, (LPARAM)&rc);
	}
}

void SetBkgFlash(HWND hwnd, ACCData* data)
{
	if ((data->hContact != NULL || data->proto[0] != '\0')
		&& ServiceExists(MS_FAVATAR_SETBKCOLOR))
	{
		FLASHAVATAR fa = {0}; 
        fa.hContact = data->hContact;
		fa.cProto = data->proto;
		fa.hParentWindow = hwnd;
        fa.id = 1675;

		if (data->bkgColor != -1)
			CallService(MS_FAVATAR_SETBKCOLOR, (WPARAM)&fa, (LPARAM)data->bkgColor);
		else
			CallService(MS_FAVATAR_SETBKCOLOR, (WPARAM)&fa, (LPARAM)RGB(255,255,255));
	}
}

void DestroyFlash(HWND hwnd, ACCData* data)
{
	if (!data->showingFlash)
		return;

	if ((data->hContact != NULL || data->proto[0] != '\0')
		&& ServiceExists(MS_FAVATAR_DESTROY))
	{
		FLASHAVATAR fa = {0}; 
        fa.hContact = data->hContact;
		fa.cProto = data->proto;
		fa.hParentWindow = hwnd;
        fa.id = 1675;
		CallService(MS_FAVATAR_DESTROY, (WPARAM)&fa, 0);
	}

	data->showingFlash = FALSE;
}

void StartFlash(HWND hwnd, ACCData* data)
{
	if (data->showingFlash)
		return;

	if (!ServiceExists(MS_FAVATAR_MAKE))
		return;

	int format;
	if (data->hContact != NULL)
	{
		format = DBGetContactSettingWord(data->hContact, "ContactPhoto", "Format", 0);
	}
	else if (data->proto[0] != '\0')
	{
		protoPicCacheEntry *ace = NULL;
		for(int i = 0; i < g_protocount; i++) 
		{
			if (!strcmp(data->proto, g_MyAvatars[i].szProtoname))
			{
				ace = &g_MyAvatars[i];
				break;
			}
		}

		if (ace != NULL && ace->szFilename != NULL)
			format = GetImageFormat(ace->szFilename);
		else 
			format = 0;
	}
	else
		return;

	if (format != PA_FORMAT_XML && format != PA_FORMAT_SWF)
		return;

	FLASHAVATAR fa = {0}; 
    fa.hContact = data->hContact;
	fa.cProto = data->proto;
	fa.hParentWindow = hwnd;
    fa.id = 1675;
	CallService(MS_FAVATAR_MAKE, (WPARAM)&fa, 0);

	if (fa.hWindow == NULL) 
		return;

	data->showingFlash = TRUE;
	ResizeFlash(hwnd, data);
	SetBkgFlash(hwnd, data);
}

BOOL ScreenToClient(HWND hWnd, LPRECT lpRect)
{
	BOOL ret;

	POINT pt;

	pt.x = lpRect->left;
	pt.y = lpRect->top;

	ret = ScreenToClient(hWnd, &pt);

	if (!ret) return ret;

	lpRect->left = pt.x;
	lpRect->top = pt.y;


	pt.x = lpRect->right;
	pt.y = lpRect->bottom;

	ret = ScreenToClient(hWnd, &pt);

	lpRect->right = pt.x;
	lpRect->bottom = pt.y;

	return ret;
}

static void Invalidate(HWND hwnd)
{
	ACCData* data =  (ACCData *) GetWindowLong(hwnd, 0);
	if (data->bkgColor == -1)
	{
		HWND parent = GetParent(hwnd);
		RECT rc;
		GetWindowRect(hwnd, &rc);
		ScreenToClient(parent, &rc); 
		InvalidateRect(parent, &rc, TRUE);
	}
	InvalidateRect(hwnd, NULL, TRUE);
}

static void NotifyAvatarChange(HWND hwnd)
{
	PSHNOTIFY pshn = {0};
	pshn.hdr.idFrom = GetDlgCtrlID(hwnd);
	pshn.hdr.hwndFrom = hwnd;
	pshn.hdr.code = NM_AVATAR_CHANGED;
	pshn.lParam = 0;
	SendMessage(GetParent(hwnd), WM_NOTIFY, 0, (LPARAM) &pshn);
}

static LRESULT CALLBACK ACCWndProc(HWND hwnd, UINT msg,  WPARAM wParam, LPARAM lParam) {
	ACCData* data =  (ACCData *) GetWindowLong(hwnd, 0);
	switch(msg) 
	{
		case WM_NCCREATE:
		{
			SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) | BS_OWNERDRAW);
			SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);

			data = (ACCData*) mir_alloc(sizeof(ACCData));
			if (data == NULL) 
				return FALSE;
			SetWindowLong(hwnd, 0, (LONG)data);

			ZeroMemory(data, sizeof(ACCData));
            data->hHook = HookEventMessage(ME_AV_AVATARCHANGED, hwnd, DM_AVATARCHANGED);
            data->hHookMy = HookEventMessage(ME_AV_MYAVATARCHANGED, hwnd, DM_MYAVATARCHANGED);
			data->hFont = (HFONT) GetStockObject(DEFAULT_GUI_FONT);
			data->borderColor = -1;
			data->bkgColor = -1;
			data->avatarBorderColor = -1;
			data->respectHidden = TRUE;
			data->showingFlash = FALSE;
			data->resizeIfSmaller = TRUE;

			return TRUE;
		}
        case WM_NCDESTROY:
        {
			DestroyFlash(hwnd, data);
			if (data) 
			{
                UnhookEvent(data->hHook);
                UnhookEvent(data->hHookMy);
				mir_free(data);
			}
			SetWindowLong(hwnd, 0, (LONG)NULL);
			break;
		}
		case WM_SETFONT:
		{
			data->hFont = (HFONT)wParam;
			Invalidate(hwnd);
			break;
		}
		case AVATAR_SETCONTACT:
		{
			DestroyFlash(hwnd, data);

			data->hContact = (HANDLE) lParam;
			lstrcpynA(data->proto, (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)data->hContact, 0), sizeof(data->proto));

			StartFlash(hwnd, data);

			NotifyAvatarChange(hwnd);
			Invalidate(hwnd);
			return TRUE;
		}
		case AVATAR_SETPROTOCOL:
		{
			DestroyFlash(hwnd, data);

			data->hContact = NULL;
			if (lParam == NULL)
				data->proto[0] = '\0';
			else
				lstrcpynA(data->proto, (char *) lParam, sizeof(data->proto));

			StartFlash(hwnd, data);

			NotifyAvatarChange(hwnd);
			Invalidate(hwnd);
			return TRUE;
		}
		case AVATAR_SETBKGCOLOR:
		{
			data->bkgColor = (COLORREF) lParam;
			if (data->showingFlash)
				SetBkgFlash(hwnd, data);
			NotifyAvatarChange(hwnd);
			Invalidate(hwnd);
			return TRUE;
		}
		case AVATAR_SETBORDERCOLOR:
		{
			data->borderColor = (COLORREF) lParam;
			if (data->showingFlash)
				ResizeFlash(hwnd, data);
			NotifyAvatarChange(hwnd);
			Invalidate(hwnd);
			return TRUE;
		}
		case AVATAR_SETAVATARBORDERCOLOR:
		{
			data->avatarBorderColor = (COLORREF) lParam;
			if (data->showingFlash)
				ResizeFlash(hwnd, data);
			NotifyAvatarChange(hwnd);
			Invalidate(hwnd);
			return TRUE;
		}
		case AVATAR_SETAVATARROUNDCORNERRADIUS:
		{
			data->avatarRoundCornerRadius = (int) lParam;
			NotifyAvatarChange(hwnd);
			Invalidate(hwnd);
			return TRUE;
		}
		case AVATAR_SETNOAVATARTEXT:
		{
			lstrcpynA(data->noAvatarText, Translate((char*) lParam), sizeof(data->noAvatarText));
			Invalidate(hwnd);
			return TRUE;
		}
		case AVATAR_RESPECTHIDDEN:
		{
			data->respectHidden = (BOOL) lParam;
			NotifyAvatarChange(hwnd);
			Invalidate(hwnd);
			return TRUE;
		}
		case AVATAR_SETRESIZEIFSMALLER:
		{
			data->resizeIfSmaller = (BOOL) lParam;
			NotifyAvatarChange(hwnd);
			Invalidate(hwnd);
			return TRUE;
		}
		case AVATAR_GETUSEDSPACE:
		{
			int *width = (int *)wParam;
			int *height = (int *)lParam;

			if (data->hContact == NULL && data->proto[0] == '\0')
			{
				*width = 0;
				*height = 0;
				return TRUE;
			}

			RECT rc;
			GetClientRect(hwnd, &rc);

			// Get avatar
			if (data->showingFlash && ServiceExists(MS_FAVATAR_GETINFO))
			{
				FLASHAVATAR fa = {0}; 
                fa.hContact = data->hContact;
				fa.cProto = data->proto;
				fa.hParentWindow = hwnd;
                fa.id = 1675;
				CallService(MS_FAVATAR_GETINFO, (WPARAM)&fa, 0);
				if (fa.hWindow != NULL)
				{
					*width = rc.right - rc.left;
					*height = rc.bottom - rc.top;
					return TRUE;
				}
			}

			avatarCacheEntry *ace;
			if (data->hContact == NULL)
				ace = (avatarCacheEntry *) CallService(MS_AV_GETMYAVATAR, 0, (LPARAM) data->proto);
			else
				ace = (avatarCacheEntry *) CallService(MS_AV_GETAVATARBITMAP, 0, (LPARAM) data->hContact);

			if (ace == NULL || ace->bmHeight == 0 || ace->bmWidth == 0 
				|| (data->respectHidden && (ace->dwFlags & AVS_HIDEONCLIST)))
			{
				*width = 0;
				*height = 0;
				return TRUE;
			}

			// Get its size
			int targetWidth = rc.right - rc.left;
			int targetHeight = rc.bottom - rc.top;

			if (!data->resizeIfSmaller && ace->bmHeight <= targetHeight && ace->bmWidth <= targetWidth)
			{
				*height = ace->bmHeight;
				*width = ace->bmWidth;
			} 
			else if (ace->bmHeight > ace->bmWidth)
			{
				float dScale = targetHeight / (float)ace->bmHeight;
				*height = targetHeight;
				*width = (int) (ace->bmWidth * dScale);
			}
			else 
			{
				float dScale = targetWidth / (float)ace->bmWidth;
				*height = (int) (ace->bmHeight * dScale);
				*width = targetWidth;
			}

			return TRUE;
		}
        case DM_AVATARCHANGED:
		{
			if (data->hContact == (HANDLE) wParam)
			{
				DestroyFlash(hwnd, data);
				StartFlash(hwnd, data);

				NotifyAvatarChange(hwnd);
				Invalidate(hwnd);
			}
            break;
		}
        case DM_MYAVATARCHANGED:
		{
			if (data->hContact == NULL && strcmp(data->proto, (char*) wParam) == 0)
			{
				DestroyFlash(hwnd, data);
				StartFlash(hwnd, data);

				NotifyAvatarChange(hwnd);
				Invalidate(hwnd);
			}
            break;
		}
		case WM_NCPAINT:
		case WM_PAINT:
		{
			if (data->hContact == NULL && data->proto[0] == '\0')
				break;

			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			if (hdc == NULL) 
				break;

			int oldBkMode = SetBkMode(hdc, TRANSPARENT);
			SetStretchBltMode(hdc, HALFTONE);

			RECT rc;
			GetClientRect(hwnd, &rc);

			// Draw background
			if (data->bkgColor != -1)
			{
				HBRUSH hbrush = CreateSolidBrush(data->bkgColor);
				FillRect(hdc, &rc, hbrush);
				DeleteObject(hbrush);
			}

			// If has a flash avatar, don't draw it
			if (data->showingFlash && ServiceExists(MS_FAVATAR_GETINFO))
			{
				FLASHAVATAR fa = {0}; 
                fa.hContact = data->hContact;
				fa.cProto = data->proto;
				fa.hParentWindow = hwnd;
                fa.id = 1675;
				CallService(MS_FAVATAR_GETINFO, (WPARAM)&fa, 0);
				if (fa.hWindow != NULL)
				{
					// Draw control border
					if (data->borderColor != -1 || data->avatarBorderColor != -1)
					{
						HBRUSH hbrush = CreateSolidBrush(data->borderColor != -1 ? data->borderColor : data->avatarBorderColor);
						FrameRect(hdc, &rc, hbrush);
						DeleteObject(hbrush);
					}
					return TRUE;
				}
			}

			// Draw avatar
            AVATARDRAWREQUEST avdrq = {0};
            avdrq.cbSize = sizeof(avdrq);
			avdrq.rcDraw = rc;
            avdrq.hContact = data->hContact;
			avdrq.szProto = data->proto;
            avdrq.hTargetDC = hdc;
            avdrq.dwFlags = AVDRQ_HIDEBORDERONTRANSPARENCY
				| (data->respectHidden ? AVDRQ_RESPECTHIDDEN : 0) 
				| (data->hContact != NULL ? 0 : AVDRQ_OWNPIC)
				| (data->avatarBorderColor == -1 ? 0 : AVDRQ_DRAWBORDER)
				| (data->avatarRoundCornerRadius <= 0 ? 0 : AVDRQ_ROUNDEDCORNER)
				| (data->resizeIfSmaller ? 0 : AVDRQ_DONTRESIZEIFSMALLER);
            avdrq.clrBorder = data->avatarBorderColor;
            avdrq.radius = data->avatarRoundCornerRadius;
			if (!CallService(MS_AV_DRAWAVATAR, 0, (LPARAM)&avdrq)) 
			{
				HGDIOBJ oldFont = SelectObject(hdc, data->hFont);

				// Get text rectangle
				RECT tr = avdrq.rcDraw;
				tr.top += 10;
				tr.bottom -= 10;
				tr.left += 10;
				tr.right -= 10;

				// Calc text size
				RECT tr_ret = tr;
				DrawTextA(hdc, data->noAvatarText, -1, &tr_ret, 
						DT_WORDBREAK | DT_NOPREFIX | DT_CENTER | DT_CALCRECT);
				
				// Calc needed size
				tr.top += ((tr.bottom - tr.top) - (tr_ret.bottom - tr_ret.top)) / 2;
				tr.bottom = tr.top + (tr_ret.bottom - tr_ret.top);
				DrawTextA(hdc, data->noAvatarText, -1, &tr, 
						DT_WORDBREAK | DT_NOPREFIX | DT_CENTER);

				SelectObject(hdc, oldFont);
			}

			// Draw control border
			if (data->borderColor != -1)
			{
				HBRUSH hbrush = CreateSolidBrush(data->borderColor);
				FrameRect(hdc, &rc, hbrush);
				DeleteObject(hbrush);
			}

			SetBkMode(hdc, oldBkMode);

			EndPaint(hwnd, &ps);
			return TRUE;
		}
		case WM_ERASEBKGND:
		{
			HDC hdc = (HDC) wParam;
			RECT rc;
			GetClientRect(hwnd, &rc);

			// Draw background
			if (data->bkgColor != -1)
			{
				HBRUSH hbrush = CreateSolidBrush(data->bkgColor);
				FillRect(hdc, &rc, hbrush);
				DeleteObject(hbrush);
			}

			// Draw control border
			if (data->borderColor != -1)
			{
				HBRUSH hbrush = CreateSolidBrush(data->borderColor);
				FrameRect(hdc, &rc, hbrush);
				DeleteObject(hbrush);
			}

			return TRUE;
		}
		case WM_SIZE:
		{
			if (data->showingFlash)
				ResizeFlash(hwnd, data);
			InvalidateRect(hwnd, NULL, TRUE);
			break;
		}
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}


int LoadACC() 
{
	WNDCLASSEX wc = {0};
	wc.cbSize         = sizeof(wc);
	wc.lpszClassName  = AVATAR_CONTROL_CLASS;
	wc.lpfnWndProc    = ACCWndProc;
	wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wc.cbWndExtra     = sizeof(ACCData*);
	wc.hbrBackground  = 0;
	wc.style          = CS_GLOBALCLASS;
	RegisterClassEx(&wc);
	return 0;
}
