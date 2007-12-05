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
#include <m_hotkeys.h>

int InitSkinHotkeys(void);
int UninitSkinHotkeys(void);

#define DBMODULENAME "CoreSkinHotKeys"

typedef struct
{
	char	*pszService, *pszName;
	TCHAR	*ptszSection, *ptszDescription;
	LPARAM	lParam;
	WORD	DefHotKey, HotKey;
	ATOM	idHotkey;
}
	THotkeyItem;

static int sttCompareHotkeys(const THotkeyItem *p1, const THotkeyItem *p2);

static SortedList *lstHotkeys = NULL;
static HWND g_hwndHotkeyHost = NULL;

static LRESULT CALLBACK sttHotkeyHostWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

static char *sttHokeyVkToName(WORD vkKey);
static void sttHokeyToName(char *buf, int size, BYTE shift, BYTE key);
static void sttHotkeyEditCreate(HWND hwnd);
static void sttHotkeyEditDestroy(HWND hwnd);
static LRESULT CALLBACK sttHotkeyEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void sttRegisterHotkeys();
static void sttUnregisterHotkeys();

static int svcHotkeySubclass(WPARAM wParam, LPARAM lParam);
static int svcHotkeyUnsubclass(WPARAM wParam, LPARAM lParam);
static int svcHotkeyVKey2Name(WPARAM wParam, LPARAM lParam);
static int svcHotkeyHotkey2Name(WPARAM wParam, LPARAM lParam);
static int svcHotkeyRegister(WPARAM wParam, LPARAM lParam);

static int sttModulesLoaded(WPARAM wParam, LPARAM lParam);
static int sttOptionsInit(WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK sttOptionsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

///////////////////////////////////////////////////////////////////////////////
// Hotkey manager
int InitSkinHotkeys(void)
{
	WNDCLASSEX wcl = {0};
	wcl.cbSize = sizeof(wcl);
	wcl.lpfnWndProc = sttHotkeyHostWndProc;
	wcl.style = 0;
	wcl.cbClsExtra = 0;
	wcl.cbWndExtra = 0;
	wcl.hInstance = GetModuleHandle(NULL);
	wcl.hIcon = NULL;
	wcl.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcl.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
	wcl.lpszMenuName = NULL;
	wcl.lpszClassName = _T("MirandaHotkeyHostWnd");
	wcl.hIconSm = NULL;
	RegisterClassEx(&wcl);

	g_hwndHotkeyHost = CreateWindow(_T("MirandaHotkeyHostWnd"), NULL, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_DESKTOP, NULL, GetModuleHandle(NULL), NULL);
	SetWindowPos(g_hwndHotkeyHost, 0, 0, 0, 0, 0, SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_DEFERERASE|SWP_NOSENDCHANGING|SWP_HIDEWINDOW);

	lstHotkeys = List_Create(0, 5);
	lstHotkeys->sortFunc = sttCompareHotkeys;

	CreateServiceFunction(MS_HOTKEY_SUBCLASS, svcHotkeySubclass);
	CreateServiceFunction(MS_HOTKEY_UNSUBCLASS, svcHotkeyUnsubclass);
	CreateServiceFunction(MS_HOTKEY_VKEY2NAME, svcHotkeyVKey2Name);
	CreateServiceFunction(MS_HOTKEY_HOTKEY2NAME, svcHotkeyHotkey2Name);
	CreateServiceFunction(MS_HOTKEY_REGISTER, svcHotkeyRegister);

	HookEvent(ME_SYSTEM_MODULESLOADED, sttModulesLoaded);
	{
		DWORD key;
		if (( key = DBGetContactSettingDword( NULL, "Clist", "HKEnShowHide", 0 ))) {
			DBWriteContactSettingDword( NULL, DBMODULENAME, "ShowHide", key );
			DBDeleteContactSetting( NULL, "Clist", "HKEnShowHide" );
		}
		if (( key = DBGetContactSettingDword( NULL, "Clist", "HKEnReadMsg", 0 ))) {
			DBWriteContactSettingDword( NULL, DBMODULENAME, "ReadMessage", key );
			DBDeleteContactSetting( NULL, "Clist", "HKEnReadMsg" );
		}
		if (( key = DBGetContactSettingDword( NULL, "Clist", "HKEnNetSearch", 0 ))) {
			DBWriteContactSettingDword( NULL, DBMODULENAME, "SearchInWeb", key );
			DBDeleteContactSetting( NULL, "Clist", "HKEnNetSearch" );
		}
		if (( key = DBGetContactSettingDword( NULL, "Clist", "HKEnShowOptions", 0 ))) {
			DBWriteContactSettingDword( NULL, DBMODULENAME, "ShowOptions", key );
			DBDeleteContactSetting( NULL, "Clist", "HKEnShowOptions" );
	}	}

	return 0;
}

int UninitSkinHotkeys(void)
{
	int i;
	sttUnregisterHotkeys();
	DestroyWindow(g_hwndHotkeyHost);
	for ( i = 0; i < lstHotkeys->realCount; i++ ) {
		THotkeyItem *item = (THotkeyItem *)lstHotkeys->items[i];
		mir_free(item->pszName);
		mir_free(item->pszService);
		mir_free(item->ptszDescription);
		mir_free(item->ptszSection);
		mir_free(item);
	}
	List_Destroy(lstHotkeys);
	return 0;
}

static int sttModulesLoaded(WPARAM wParam, LPARAM lParam)
{
	HookEvent(ME_OPT_INITIALISE, sttOptionsInit);
	return 0;
}

static void sttWordToModAndVk(WORD w, BYTE *mod, BYTE *vk)
{
	*mod = 0;
	if (HIBYTE(w) & HOTKEYF_CONTROL) *mod |= MOD_CONTROL;
	if (HIBYTE(w) & HOTKEYF_SHIFT)   *mod |= MOD_SHIFT;
	if (HIBYTE(w) & HOTKEYF_ALT)     *mod |= MOD_ALT;
	if (HIBYTE(w) & HOTKEYF_EXT)     *mod |= MOD_WIN;
	*vk = LOBYTE(w);
}

static LRESULT CALLBACK sttHotkeyHostWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_HOTKEY ) {
		int i;
		for (i = 0; i < lstHotkeys->realCount; i++) {
			THotkeyItem *item = (THotkeyItem *)lstHotkeys->items[i];
			if (wParam == item->idHotkey) {
				CallService(item->pszService, 0, item->lParam);
				break;
		}	}

		return TRUE;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

static int svcHotkeySubclass(WPARAM wParam, LPARAM lParam)
{
	sttHotkeyEditCreate((HWND)wParam);
	return 0;
}

static int svcHotkeyUnsubclass(WPARAM wParam, LPARAM lParam)
{
	sttHotkeyEditDestroy((HWND)wParam);
	return 0;
}

static int svcHotkeyVKey2Name(WPARAM wParam, LPARAM lParam)
{
	return (int)sttHokeyVkToName(wParam);
}

static int svcHotkeyHotkey2Name(WPARAM wParam, LPARAM lParam)
{
	sttHokeyToName((char *)lParam, HIWORD(wParam), HIBYTE(LOWORD(wParam)), LOBYTE(LOWORD(wParam)));
	return 0;
}

static int svcHotkeyRegister(WPARAM wParam, LPARAM lParam)
{
	char buf[256];

	HOTKEYDESC *desc = (HOTKEYDESC *)lParam;
	THotkeyItem *item = mir_alloc(sizeof(THotkeyItem));

	item->pszName = mir_strdup(desc->pszName);
	item->pszService = mir_strdup(desc->pszService);
	item->ptszDescription = mir_a2t(desc->pszDescription);
	item->ptszSection = mir_a2t(desc->pszSection);
	item->DefHotKey = desc->DefHotKey;
	item->HotKey = DBGetContactSettingWord(NULL, DBMODULENAME, item->pszName, item->DefHotKey);
	item->lParam = desc->lParam;

	mir_snprintf(buf, SIZEOF(buf), "mir_hotkey_%s", item->pszName);
	item->idHotkey = GlobalAddAtomA(buf);
	if ( !DBGetContactSettingByte(NULL, DBMODULENAME "Off", item->pszName, 0)) {
		BYTE mod, vk;
		sttWordToModAndVk(item->HotKey, &mod, &vk);
		RegisterHotKey(g_hwndHotkeyHost, item->idHotkey, mod, vk);
	}

	List_InsertPtr(lstHotkeys, item);
	return item->idHotkey;
}

static int sttCompareHotkeys(const THotkeyItem *p1, const THotkeyItem *p2)
{
	int res;
	if ( res = lstrcmp( p1->ptszSection, p2->ptszSection ))
		return res;
	return lstrcmp( p1->ptszDescription, p2->ptszDescription );
}

static void sttRegisterHotkeys()
{
	int i;
	for ( i = 0; i < lstHotkeys->realCount; i++ ) {
		THotkeyItem *item = (THotkeyItem *)lstHotkeys->items[i];
		UnregisterHotKey(g_hwndHotkeyHost, item->idHotkey);
		if ( !DBGetContactSettingByte(NULL, DBMODULENAME "Off", item->pszName, 0 )) {
			BYTE mod, vk;
			sttWordToModAndVk(item->HotKey, &mod, &vk);
			if (!vk) continue;
			RegisterHotKey(g_hwndHotkeyHost, item->idHotkey, mod, vk);
}	}	}

static void sttUnregisterHotkeys()
{
	int i;
	for (i = 0; i < lstHotkeys->realCount; i++)
		UnregisterHotKey(g_hwndHotkeyHost, ((THotkeyItem *)lstHotkeys->items[i])->idHotkey);
}

///////////////////////////////////////////////////////////////////////////////
// Hotkey control
typedef struct
{
	WNDPROC oldWndProc;
	BYTE shift;
	BYTE key;
}
	THotkeyBoxData;

static char *sttHokeyVkToName(WORD vkKey)
{
	switch (vkKey) {
	case VK_LBUTTON        	: // 0x01
		return LPGEN("Left Click");
	case VK_RBUTTON        	: // 0x02
		return LPGEN("Right Click");
	case VK_CANCEL         	: // 0x03
		return LPGEN("Ctrl+Break");
	case VK_MBUTTON        	: // 0x04    /* NOT contiguous with L & RBUTTON */
		return LPGEN("Wheel Click");
	case VK_XBUTTON1       	: // 0x05    /* NOT contiguous with L & RBUTTON */
		return LPGEN("Back Click");
	case VK_XBUTTON2       	: // 0x06    /* NOT contiguous with L & RBUTTON */
		return LPGEN("Forward Click");
	case VK_BACK           	: // 0x08
		return LPGEN("Backspace");
	case VK_TAB            	: // 0x09
		return LPGEN("Tab");
	case VK_CLEAR          	: // 0x0C
		return LPGEN("Clear");
	case VK_RETURN         	: // 0x0D
		return LPGEN("Enter");
	case VK_PAUSE          	: // 0x13
		return LPGEN("Pause");
	case VK_CAPITAL        	: // 0x14
		return LPGEN("Caps Lock");
	case VK_ESCAPE         	: // 0x1B
		return LPGEN("Esc");
	case VK_SPACE          	: // 0x20
		return LPGEN("Space");
	case VK_PRIOR          	: // 0x21
		return LPGEN("Page Up");
	case VK_NEXT           	: // 0x22
		return LPGEN("Page Down");
	case VK_END            	: // 0x23
			return LPGEN("End");
	case VK_HOME           	: // 0x24
		return LPGEN("Home");
	case VK_LEFT           	: // 0x25
		return LPGEN("Left");
	case VK_UP             	: // 0x26
		return LPGEN("Up");
	case VK_RIGHT          	: // 0x27
		return LPGEN("Right");
	case VK_DOWN           	: // 0x28
		return LPGEN("Down");
	case VK_SELECT         	: // 0x29
		return LPGEN("Select");
	case VK_PRINT          	: // 0x2A
		return LPGEN("Print");
	case VK_EXECUTE        	: // 0x2B
		return LPGEN("Execute");
	case VK_SNAPSHOT       	: // 0x2C
		return LPGEN("Print Screen");
	case VK_INSERT         	: // 0x2D
		return LPGEN("Ins");
	case VK_DELETE         	: // 0x2E
		return LPGEN("Del");
	case VK_HELP           	: // 0x2F
		return LPGEN("Help");
	case VK_SLEEP          	: // 0x5F
		return LPGEN("Sleep");
	case VK_NUMPAD0        	: // 0x60
		return LPGEN("Num 0");
	case VK_NUMPAD1        	: // 0x61
		return LPGEN("Num 1");
	case VK_NUMPAD2        	: // 0x62
		return LPGEN("Num 2");
	case VK_NUMPAD3        	: // 0x63
		return LPGEN("Num 3");
	case VK_NUMPAD4        	: // 0x64
		return LPGEN("Num 4");
	case VK_NUMPAD5        	: // 0x65
		return LPGEN("Num 5");
	case VK_NUMPAD6        	: // 0x66
		return LPGEN("Num 6");
	case VK_NUMPAD7        	: // 0x67
		return LPGEN("Num 7");
	case VK_NUMPAD8        	: // 0x68
		return LPGEN("Num 8");
	case VK_NUMPAD9        	: // 0x69
		return LPGEN("Num 9");
	case VK_MULTIPLY       	: // 0x6A
		return LPGEN("Num *");
	case VK_ADD            	: // 0x6B
		return LPGEN("Num +");
	case VK_SEPARATOR      	: // 0x6C
		return LPGEN("Separator");
	case VK_SUBTRACT       	: // 0x6D
		return LPGEN("Num -");
	case VK_DECIMAL        	: // 0x6E
		return LPGEN("Num .");
	case VK_DIVIDE         	: // 0x6F
		return LPGEN("Num /");
	case VK_F1             	: // 0x70
		return LPGEN("F1");
	case VK_F2             	: // 0x71
		return LPGEN("F2");
	case VK_F3             	: // 0x72
		return LPGEN("F3");
	case VK_F4             	: // 0x73
		return LPGEN("F4");
	case VK_F5             	: // 0x74
		return LPGEN("F5");
	case VK_F6             	: // 0x75
		return LPGEN("F6");
	case VK_F7             	: // 0x76
		return LPGEN("F7");
	case VK_F8             	: // 0x77
		return LPGEN("F8");
	case VK_F9             	: // 0x78
		return LPGEN("F9");
	case VK_F10            	: // 0x79
		return LPGEN("F10");
	case VK_F11            	: // 0x7A
		return LPGEN("F11");
	case VK_F12            	: // 0x7B
		return LPGEN("F12");
	case VK_F13            	: // 0x7C
		return LPGEN("F13");
	case VK_F14            	: // 0x7D
		return LPGEN("F14");
	case VK_F15            	: // 0x7E
		return LPGEN("F15");
	case VK_F16            	: // 0x7F
		return LPGEN("F16");
	case VK_F17            	: // 0x80
		return LPGEN("F17");
	case VK_F18            	: // 0x81
		return LPGEN("F18");
	case VK_F19            	: // 0x82
		return LPGEN("F19");
	case VK_F20            	: // 0x83
		return LPGEN("F20");
	case VK_F21            	: // 0x84
		return LPGEN("F21");
	case VK_F22            	: // 0x85
		return LPGEN("F22");
	case VK_F23            	: // 0x86
		return LPGEN("F23");
	case VK_F24            	: // 0x87
		return LPGEN("F24");
	case VK_NUMLOCK        	: // 0x90
		return LPGEN("Num Lock");
	case VK_SCROLL         	: // 0x91
		return LPGEN("Scroll Lock");
	case VK_BROWSER_BACK        : // 0xA6
		return LPGEN("Back");
	case VK_BROWSER_FORWARD     : // 0xA7
		return LPGEN("Forward");
	case VK_BROWSER_REFRESH     : // 0xA8
		return LPGEN("Refresh");
	case VK_BROWSER_STOP        : // 0xA9
		return LPGEN("Stop");
	case VK_BROWSER_SEARCH      : // 0xAA
		return LPGEN("Search");
	case VK_BROWSER_FAVORITES   : // 0xAB
		return LPGEN("Favorites");
	case VK_BROWSER_HOME        : // 0xAC
		return LPGEN("Home");
	case VK_VOLUME_MUTE         : // 0xAD
		return LPGEN("Mute");
	case VK_VOLUME_DOWN         : // 0xAE
		return LPGEN("Vol Down");
	case VK_VOLUME_UP           : // 0xAF
		return LPGEN("Vol Up");
	case VK_MEDIA_NEXT_TRACK    : // 0xB0
		return LPGEN("Prev Track");
	case VK_MEDIA_PREV_TRACK    : // 0xB1
		return LPGEN("Next Track");
	case VK_MEDIA_STOP          : // 0xB2
		return LPGEN("Stop");
	case VK_MEDIA_PLAY_PAUSE    : // 0xB3
			return LPGEN("Play/Pause");
	case VK_LAUNCH_MAIL         : // 0xB4
		return LPGEN("Main");
	case VK_LAUNCH_MEDIA_SELECT : // 0xB5
		return LPGEN("Media");
	case VK_LAUNCH_APP1         : // 0xB6
		return LPGEN("App1");
	case VK_LAUNCH_APP2         : // 0xB7
		return LPGEN("App2");
	}
	if ((vkKey >= 'A') && (vkKey <= 'Z'))
	{
		static char letters[] = "A\0" "B\0" "C\0" "D\0" "E\0" "F\0" "G\0" "H\0" "I\0" "J\0" "K\0" "L\0" "M\0" "N\0" "O\0" "P\0" "Q\0" "R\0" "S\0" "T\0" "U\0" "V\0" "W\0" "X\0" "Y\0" "Z\0";
		return letters + (vkKey - 'A') * 2;
	}
	if ((vkKey >= '0') && (vkKey <= '9'))
	{
		static char digits[] = "0\0" "1\0" "2\0" "3\0" "4\0" "5\0" "6\0" "7\0" "8\0" "9\0";
		return digits + (vkKey - '0') * 2;
	}
	return LPGEN("");
}

static void sttHokeyToName(char *buf, int size, BYTE shift, BYTE key)
{
	mir_snprintf(buf, size, "%s%s%s%s%s",
		(shift & HOTKEYF_CONTROL)	? "Ctrl + "		: "",
		(shift & HOTKEYF_ALT)		? "Alt + "		: "",
		(shift & HOTKEYF_SHIFT)		? "Shift + "	: "",
		(shift & HOTKEYF_EXT)		? "Win + "		: "",
		sttHokeyVkToName(key));
}

static LRESULT CALLBACK sttHotkeyEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	THotkeyBoxData *data = (THotkeyBoxData *)GetWindowLong(hwnd, GWL_USERDATA);
	BOOL bKeyDown = FALSE;
	if (!data) return 0;

	switch (msg) {
		case HKM_GETHOTKEY:
			return data->key ? MAKEWORD(data->key, data->shift) : 0;

		case HKM_SETHOTKEY:
		{
			char buf[256] = {0};
			data->key = (BYTE)LOWORD(wParam);
			data->shift = (BYTE)HIWORD(wParam);
			sttHokeyToName(buf, SIZEOF(buf), data->shift, data->key);
			SetWindowTextA(hwnd, buf);
			return 0;
		}

		case WM_GETDLGCODE:
			return DLGC_WANTALLKEYS;

		case WM_KILLFOCUS:
			break;

		case WM_CHAR:
		case WM_SYSCHAR:
		case WM_PASTE:
			return TRUE;

		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			bKeyDown = TRUE;

		case WM_KEYUP:
		case WM_SYSKEYUP:
		{
			char buf[256] = {0};

			BYTE shift = 0;
			char *name = sttHokeyVkToName(wParam);
			BYTE key = (*name && bKeyDown) ? wParam : 0;

			if (GetAsyncKeyState(VK_CONTROL)) shift |= HOTKEYF_CONTROL;
			if (GetAsyncKeyState(VK_MENU)) shift |= HOTKEYF_ALT;
			if (GetAsyncKeyState(VK_SHIFT)) shift |= HOTKEYF_SHIFT;
			if (GetAsyncKeyState(VK_LWIN) || GetAsyncKeyState(VK_RWIN)) shift |= HOTKEYF_EXT;

			if (bKeyDown || !data->key) {
				data->shift = shift;
				data->key = key;
			}

			sttHokeyToName(buf, SIZEOF(buf), data->shift, data->key);
			SetWindowTextA(hwnd, buf);

			if (bKeyDown && data->key)
				SendMessage(GetParent(hwnd), WM_COMMAND, MAKELONG(GetWindowLong(hwnd, GWL_ID), 0), (LPARAM)hwnd);

			return TRUE;
		}

		case WM_DESTROY:
		{
			WNDPROC saveOldWndProc = data->oldWndProc;
			SetWindowLong(hwnd, GWL_WNDPROC, (ULONG_PTR)data->oldWndProc);
			SetWindowLong(hwnd, GWL_USERDATA, 0);
			mir_free(data);
			return CallWindowProc(saveOldWndProc, hwnd, msg, wParam, lParam);
	}	}

	return CallWindowProc(data->oldWndProc, hwnd, msg, wParam, lParam);
}

static void sttHotkeyEditCreate(HWND hwnd)
{
	THotkeyBoxData *data = (THotkeyBoxData *)mir_alloc(sizeof(THotkeyBoxData));
	SetWindowLong(hwnd, GWL_USERDATA, (ULONG_PTR)data);
	data->oldWndProc = (WNDPROC)SetWindowLong(hwnd, GWL_WNDPROC, (ULONG_PTR)sttHotkeyEditProc);
}

static void sttHotkeyEditDestroy(HWND hwnd)
{
	THotkeyBoxData *data = (THotkeyBoxData *)GetWindowLong(hwnd, GWL_USERDATA);
	SetWindowLong(hwnd, GWL_WNDPROC, (ULONG_PTR)data->oldWndProc);
	SetWindowLong(hwnd, GWL_USERDATA, 0);
	if (data) mir_free(data);
}

///////////////////////////////////////////////////////////////////////////////
// Options
static int sttOptionsInit(WPARAM wParam, LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp = {0};
	odp.cbSize = sizeof(odp);
	odp.hInstance = GetModuleHandle(NULL);
	odp.flags = ODPF_BOLDGROUPS | ODPF_TCHAR;
	odp.position = -180000000;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_HOTKEYS);
	odp.ptszTitle = TranslateT("Hotkeys");
	odp.ptszGroup = TranslateT("Customize");
	odp.pfnDlgProc = sttOptionsDlgProc;
	CallService(MS_OPT_ADDPAGE, wParam, (LPARAM)&odp);
	return 0;
}

static LRESULT CALLBACK sttOptionsDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static BOOL initialized = FALSE;

	switch (msg) {
	case WM_INITDIALOG:
		{
			int i, nItems=0, iGroupId=-1;
			LVCOLUMN lvc;
			RECT rc;

			initialized = FALSE;

			TranslateDialogDefault(hwndDlg);

			sttHotkeyEditCreate(GetDlgItem(hwndDlg, IDC_HOTKEY));
			
			ListView_SetExtendedListViewStyle(GetDlgItem(hwndDlg, IDC_LV_HOTKEYS), LVS_EX_CHECKBOXES|LVS_EX_FULLROWSELECT);

			GetClientRect(GetDlgItem(hwndDlg, IDC_LV_HOTKEYS), &rc);
			lvc.mask = LVCF_WIDTH;
			lvc.cx = (rc.right - GetSystemMetrics(SM_CXHTHUMB) - 1)/2;
			ListView_InsertColumn(GetDlgItem(hwndDlg, IDC_LV_HOTKEYS), 0, &lvc);
			ListView_InsertColumn(GetDlgItem(hwndDlg, IDC_LV_HOTKEYS), 1, &lvc);

			if ( IsWinVerXPPlus() )
				ListView_EnableGroupView(GetDlgItem(hwndDlg, IDC_LV_HOTKEYS), TRUE);

			for (i = 0; i < lstHotkeys->realCount; i++) {
				LVITEM lvi = {0};
				THotkeyItem *item = (THotkeyItem *)lstHotkeys->items[i];
				char buf[256];

				if ( !i || lstrcmp(item->ptszSection, ((THotkeyItem *)lstHotkeys->items[i-1])->ptszSection )) {
					#ifdef _UNICODE
					if (IsWinVerXPPlus()) {
						LVGROUP lvg;
						lvg.cbSize = sizeof(lvg);
						lvg.mask = LVGF_HEADER|LVGF_GROUPID;
						lvg.iGroupId = ++iGroupId;
						lvg.pszHeader = TranslateTS(item->ptszSection);
						lvg.cchHeader = lstrlen(lvg.pszHeader);
						ListView_InsertGroup(GetDlgItem(hwndDlg, IDC_LV_HOTKEYS), -1, &lvg);
					} 
					else
					#endif
					{
						lvi.mask = LVIF_TEXT;
						lvi.iItem = nItems++;
						lvi.iSubItem = 0;
						lvi.pszText = TranslateTS(item->ptszSection);
						ListView_InsertItem(GetDlgItem(hwndDlg, IDC_LV_HOTKEYS), &lvi);
				}	}

				lvi.mask = LVIF_TEXT|LVIF_GROUPID|LVIF_PARAM;
				lvi.iItem = nItems++;
				lvi.iSubItem = 0;
				lvi.iGroupId = iGroupId;
				lvi.pszText = TranslateTS(item->ptszDescription);
				lvi.lParam = (LPARAM)item;
				ListView_InsertItem(GetDlgItem(hwndDlg, IDC_LV_HOTKEYS), &lvi);

				ListView_SetCheckState(GetDlgItem(hwndDlg, IDC_LV_HOTKEYS), lvi.iItem, 
					DBGetContactSettingByte(NULL, DBMODULENAME "Off", item->pszName, 0) ? 0 : 1);

				lvi.mask = LVIF_TEXT;
				lvi.iSubItem = 1;
				sttHokeyToName(buf, SIZEOF(buf), HIBYTE(item->HotKey), LOBYTE(item->HotKey));
				lvi.pszText = mir_a2t(buf);
				ListView_SetItem(GetDlgItem(hwndDlg, IDC_LV_HOTKEYS), &lvi);
				mir_free(lvi.pszText);
		}	}
		initialized = TRUE;
		return TRUE;

	case WM_DESTROY:
		sttRegisterHotkeys();
		break;

	case WM_COMMAND:
		if (( LOWORD( wParam ) == IDC_HOTKEY) && (( HIWORD( wParam ) == EN_KILLFOCUS) || (HIWORD(wParam) == 0 ))) {
			LVITEM lvi;
			THotkeyItem *item;
			WORD wHotkey = (WORD)SendDlgItemMessage(hwndDlg, IDC_HOTKEY, HKM_GETHOTKEY, 0, 0);

			ShowWindow(GetDlgItem(hwndDlg, IDC_HOTKEY), SW_HIDE);
			if ( !wHotkey || (wHotkey == VK_ESCAPE) || (HIWORD(wParam) != 0 ))
				break;

			lvi.mask = LVIF_PARAM;
			lvi.iItem = ListView_GetNextItem(GetDlgItem(hwndDlg, IDC_LV_HOTKEYS), -1, LVNI_SELECTED);
			if (lvi.iItem >= 0) {
				ListView_GetItem(GetDlgItem(hwndDlg, IDC_LV_HOTKEYS), &lvi);
				if (item = (THotkeyItem *)lvi.lParam) {
					char buf[256];
					item->HotKey = wHotkey;

					lvi.mask = LVIF_TEXT;
					lvi.iSubItem = 1;
					sttHokeyToName(buf, SIZEOF(buf), HIBYTE(item->HotKey), LOBYTE(item->HotKey));
					lvi.pszText = mir_a2t(buf);
					ListView_SetItem(GetDlgItem(hwndDlg, IDC_LV_HOTKEYS), &lvi);
					mir_free(lvi.pszText);

					SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
		}	}	}
		break;

	case WM_NOTIFY:
		{
			LPNMHDR lpnmhdr = (LPNMHDR)lParam;
			switch (lpnmhdr->idFrom) {
			case 0:
				{
					int i, count;

					if (( lpnmhdr->code != PSN_APPLY) && (lpnmhdr->code != PSN_RESET ))
						break;

					count = ListView_GetItemCount(GetDlgItem(hwndDlg, IDC_LV_HOTKEYS));
					for (i = 0; i < count; i++) {
						LVITEM lvi;
						THotkeyItem *item;
						lvi.mask = LVIF_PARAM;
						lvi.iItem = i;
						ListView_GetItem(GetDlgItem(hwndDlg, IDC_LV_HOTKEYS), &lvi);
						if (( item = (THotkeyItem *)lvi.lParam ) == 0 )
							continue;

						if ( lpnmhdr->code == PSN_APPLY ) {
							int state = ListView_GetCheckState(GetDlgItem(hwndDlg, IDC_LV_HOTKEYS), i) ? 0 : 1;

							if (DBGetContactSettingWord(NULL, DBMODULENAME, item->pszName, item->DefHotKey) != item->HotKey)
								DBWriteContactSettingWord(NULL, DBMODULENAME, item->pszName, item->HotKey);

							if (DBGetContactSettingWord(NULL, DBMODULENAME "Off", item->pszName, 0) != state)
								DBWriteContactSettingByte(NULL, DBMODULENAME "Off", item->pszName, state);
						}
						else if (lpnmhdr->code == PSN_RESET)
							item->HotKey = DBGetContactSettingWord(NULL, DBMODULENAME, item->pszName, item->DefHotKey);
					}
					break;
				}
			case IDC_LV_HOTKEYS:
				switch (lpnmhdr->code) {
				case LVN_KEYDOWN:
					{
						LPNMLVKEYDOWN param = (LPNMLVKEYDOWN)lParam;
						if (param->wVKey != VK_F2)
							break;
					}
				case LVN_ITEMACTIVATE:
					{
						LVITEM lvi;
						THotkeyItem *item;
						int iItem = ListView_GetNextItem(GetDlgItem(hwndDlg, IDC_LV_HOTKEYS), -1, LVNI_SELECTED);
						if (iItem < 0)
							break;

						lvi.mask = LVIF_PARAM;
						lvi.iItem = iItem;
						ListView_GetItem(GetDlgItem(hwndDlg, IDC_LV_HOTKEYS), &lvi);

						if (item = (THotkeyItem *)lvi.lParam) {
							RECT rc;	
							ListView_GetSubItemRect(GetDlgItem(hwndDlg, IDC_LV_HOTKEYS), iItem, 1, LVIR_BOUNDS, &rc);
							MapWindowPoints(GetDlgItem(hwndDlg, IDC_LV_HOTKEYS), hwndDlg, (LPPOINT)&rc, 2);
							SendDlgItemMessage(hwndDlg, IDC_HOTKEY, HKM_SETHOTKEY, MAKELONG(LOBYTE(item->HotKey), HIBYTE(item->HotKey)), 0);

							SetWindowPos(GetDlgItem(hwndDlg, IDC_LV_HOTKEYS), HWND_BOTTOM, 0, 0, 0, 0, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE);
							SetWindowPos(GetDlgItem(hwndDlg, IDC_HOTKEY), HWND_TOP, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, SWP_SHOWWINDOW);
							RedrawWindow(GetDlgItem(hwndDlg, IDC_HOTKEY), NULL, NULL, RDW_INVALIDATE);

							SetFocus(GetDlgItem(hwndDlg, IDC_HOTKEY));
							RedrawWindow(GetDlgItem(hwndDlg, IDC_HOTKEY), NULL, NULL, RDW_INVALIDATE);
						}
						break;
					}
				case LVN_ITEMCHANGED:
					{
						LPNMLISTVIEW param = (LPNMLISTVIEW)lParam;
						if (initialized && param->lParam && (param->uNewState>>12 != param->uOldState>>12))
							SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
						break;
					}
				case NM_CUSTOMDRAW:
					{
						NMLVCUSTOMDRAW *param = (NMLVCUSTOMDRAW *) lParam;
						switch (param->nmcd.dwDrawStage) {
						case CDDS_PREPAINT:
						case CDDS_ITEMPREPAINT:
							SetWindowLong( hwndDlg, DWL_MSGRESULT, CDRF_NOTIFYSUBITEMDRAW );
							return TRUE;

						case CDDS_SUBITEM|CDDS_ITEMPREPAINT:
							{
								TCHAR buf[256];
								LVITEM lvi = {0};
								lvi.mask = LVIF_TEXT|LVIF_PARAM;
								lvi.iItem = param->nmcd.dwItemSpec;
								lvi.pszText = buf;
								lvi.cchTextMax = SIZEOF(buf);
								ListView_GetItem(lpnmhdr->hwndFrom, &lvi);

								if (!lvi.lParam) {
									RECT rc;
									HFONT hfnt;

									ListView_GetSubItemRect(lpnmhdr->hwndFrom, param->nmcd.dwItemSpec, param->iSubItem, LVIR_BOUNDS, &rc);
									FillRect(param->nmcd.hdc, &rc, GetSysColorBrush(COLOR_WINDOW));

									if (param->iSubItem == 0) {
										rc.left += 3;
										hfnt = SelectObject(param->nmcd.hdc, (HFONT)SendMessage(GetParent(hwndDlg), PSM_GETBOLDFONT, 0, 0));
										DrawText(param->nmcd.hdc, buf, -1, &rc, DT_LEFT|DT_NOPREFIX|DT_SINGLELINE|DT_VCENTER);
										SelectObject(param->nmcd.hdc, hfnt);
									}

									SetWindowLong( hwndDlg, DWL_MSGRESULT, CDRF_SKIPDEFAULT );
									return TRUE;
								}
								break;
						}	}
						break;
					}
					break;
			}	}
			break;
	}	}

	return FALSE;
}
