/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2003 Miranda ICQ/IM project, 
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
#include "../../core/commonheaders.h"
#include "clist.h"

#define HOTKEY_SHOWHIDE 0x01
#define HOTKEY_READMSG  0x02
#define HOTKEY_NETSEARCH 0x04
#define HOTKEY_SHOWOPTIONS 0x08

int EventsProcessTrayDoubleClick(void);
int ShowHide(WPARAM wParam,LPARAM lParam);

static void WordToModAndVk(WORD w,UINT *mod,UINT *vk)
{
	*mod=0;
	if(HIBYTE(w)&HOTKEYF_CONTROL) *mod|=MOD_CONTROL;
	if(HIBYTE(w)&HOTKEYF_SHIFT) *mod|=MOD_SHIFT;
	if(HIBYTE(w)&HOTKEYF_ALT) *mod|=MOD_ALT;
	if(HIBYTE(w)&HOTKEYF_EXT) *mod|=MOD_WIN;
	*vk=LOBYTE(w);
}

int HotKeysRegister(HWND hwnd)
{
	UINT mod,vk;

	if(DBGetContactSettingByte(NULL,"CList","HKEnShowHide",0)) {
		WordToModAndVk((WORD)DBGetContactSettingWord(NULL,"CList","HKShowHide",0),&mod,&vk);
		RegisterHotKey(hwnd,HOTKEY_SHOWHIDE,mod,vk);
	}
	if(DBGetContactSettingByte(NULL,"CList","HKEnReadMsg",0)) {
		WordToModAndVk((WORD)DBGetContactSettingWord(NULL,"CList","HKReadMsg",0),&mod,&vk);
		RegisterHotKey(hwnd,HOTKEY_READMSG,mod,vk);
	}
	if(DBGetContactSettingByte(NULL,"CList","HKEnNetSearch",0)) {
		WordToModAndVk((WORD)DBGetContactSettingWord(NULL,"CList","HKNetSearch",0),&mod,&vk);
		RegisterHotKey(hwnd,HOTKEY_NETSEARCH,mod,vk);
	}
	if(DBGetContactSettingByte(NULL,"CList","HKEnShowOptions",0)) {
		WordToModAndVk((WORD)DBGetContactSettingWord(NULL,"CList","HKShowOptions",0),&mod,&vk);
		RegisterHotKey(hwnd,HOTKEY_SHOWOPTIONS,mod,vk);
	}
	return 0;
}

void HotKeysUnregister(HWND hwnd)
{
	UnregisterHotKey(hwnd,HOTKEY_SHOWHIDE);
	UnregisterHotKey(hwnd,HOTKEY_READMSG);
	UnregisterHotKey(hwnd,HOTKEY_NETSEARCH);
	UnregisterHotKey(hwnd,HOTKEY_SHOWOPTIONS);
}

int HotKeysProcess(HWND hwnd,WPARAM wParam,LPARAM lParam)
{
	switch((int)wParam)	{
		case HOTKEY_SHOWHIDE:
			ShowHide(0,0);
			break;
		case HOTKEY_NETSEARCH:
		{	DBVARIANT dbv;
			if(!DBGetContactSetting(NULL,"CList","SearchUrl",&dbv)) {
				CallService(MS_UTILS_OPENURL,DBGetContactSettingByte(NULL,"CList","HKSearchNewWnd",0),(LPARAM)dbv.pszVal);
				free(dbv.pszVal);
			}
			break;
		}
		case HOTKEY_READMSG:
		{
			if(EventsProcessTrayDoubleClick()==0) break;
			SetForegroundWindow(hwnd);
			SetFocus(hwnd);
			break;
		}
		case HOTKEY_SHOWOPTIONS:
		{
			CallService("Options/OptionsCommand",0, 0);
			break;
		}
	}
	return TRUE;
} 

int HotkeysProcessMessage(WPARAM wParam,LPARAM lParam)
{
	MSG *msg=(MSG*)wParam;
	switch(msg->message) {
		case WM_CREATE:
			HotKeysRegister(msg->hwnd);
			break;
		case WM_HOTKEY:
			*((LRESULT*)lParam)=HotKeysProcess(msg->hwnd,msg->wParam,msg->lParam);
			return TRUE;
		case WM_DESTROY:
			HotKeysUnregister(msg->hwnd);
			break;
	}
	
	return FALSE;
}