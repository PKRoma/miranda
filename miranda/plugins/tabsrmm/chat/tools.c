/*
Chat module plugin for Miranda IM

Copyright (C) 2003 Jörgen Persson

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
#include "../commonheaders.h"

extern HICON		hIcons[30];
extern FONTINFO		aFonts[OPTIONS_FONTCOUNT];
extern HMENU		g_hMenu;
extern HANDLE		hBuildMenuEvent ;
extern HANDLE		hSendEvent;
extern SESSION_INFO g_TabSession;
extern MYGLOBALS	myGlobals;
extern NEN_OPTIONS  nen_options;

static void Chat_PlaySound(const char *szSound, HWND hWnd, struct MessageWindowData *dat)
{
    BOOL fPlay = TRUE;

    if (nen_options.iNoSounds)
        return;

    if (dat) {
        DWORD dwFlags = dat->pContainer->dwFlags;
        fPlay = dwFlags & CNT_NOSOUND ? FALSE : TRUE;
    }

    if (fPlay)
        SkinPlaySound(szSound);
}

int GetRichTextLength(HWND hwnd)
{
	GETTEXTLENGTHEX gtl;

	gtl.flags = GTL_PRECISE;
	gtl.codepage = CP_ACP ;
	return (int) SendMessage(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
}

TCHAR* RemoveFormatting(const TCHAR* pszWord)
{
	static TCHAR szTemp[10000];
	int i = 0;
	int j = 0;

	if ( pszWord == 0 || lstrlen(pszWord) == 0 )
		return NULL;

	while(j < 9999 && i <= lstrlen( pszWord )) {
		if (pszWord[i] == '%') {
			switch ( pszWord[i+1] ) {
			case '%':
				szTemp[j] = '%';
				j++;
				i++; i++;
				break;
			case 'b':
			case 'u':
			case 'i':
			case 'B':
			case 'U':
			case 'I':
			case 'r':
			case 'C':
			case 'F':
				i++; i++;
				break;

			case 'c':
			case 'f':
				i += 4;
				break;

			default:
				szTemp[j] = pszWord[i];
				j++;
				i++;
				break;
		}	}
		else {
			szTemp[j] = pszWord[i];
			j++;
			i++;
	}	}

	return (TCHAR*) &szTemp;
}

static void __stdcall ShowRoomFromPopup(void * pi)
{
	SESSION_INFO* si = (SESSION_INFO*) pi;
	ShowRoom(si, WINDOW_VISIBLE, TRUE);
}

static int CALLBACK PopupDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message) {
	case WM_COMMAND:
		if (HIWORD(wParam) == STN_CLICKED) {
			SESSION_INFO* si = (SESSION_INFO*)CallService(MS_POPUP_GETPLUGINDATA, (WPARAM)hWnd,(LPARAM)0);;

			CallFunctionAsync(ShowRoomFromPopup, si);

			PUDeletePopUp(hWnd);
			return TRUE;
		}
		break;
	case WM_CONTEXTMENU:
		{
			SESSION_INFO* si = (SESSION_INFO*)CallService(MS_POPUP_GETPLUGINDATA, (WPARAM)hWnd,(LPARAM)0);
			if (si->hContact)
				if (CallService(MS_CLIST_GETEVENT, (WPARAM)si->hContact, (LPARAM)0))
					CallService(MS_CLIST_REMOVEEVENT, (WPARAM)si->hContact, (LPARAM)szChatIconString);

			if (si->hWnd && KillTimer(si->hWnd, TIMERID_FLASHWND))
				FlashWindow(si->hWnd, FALSE);

			PUDeletePopUp( hWnd );
		}
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

static int ShowPopup (HANDLE hContact, SESSION_INFO* si, HICON hIcon,  char* pszProtoName,  TCHAR* pszRoomName, COLORREF crBkg, const TCHAR* fmt, ...)
{
	POPUPDATAT pd = {0};
	va_list marker;
	static TCHAR szBuf[4*1024];

	if (!fmt || lstrlen(fmt) == 0 || lstrlen(fmt) > 2000)
		return 0;

	va_start(marker, fmt);
	_vstprintf(szBuf, fmt, marker);
	va_end(marker);

	pd.lchContact = hContact;

	if ( hIcon )
		pd.lchIcon = hIcon ;
	else
		pd.lchIcon = LoadIconEx(IDI_CHANMGR, "window", 0, 0 );

	mir_sntprintf(pd.lptzContactName, MAX_CONTACTNAME-1, _T(TCHAR_STR_PARAM) _T(" - %s"), 
		pszProtoName, CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, GCDNF_TCHAR ));
	lstrcpyn( pd.lptzText, TranslateTS(szBuf), MAX_SECONDLINE-1);
	pd.iSeconds = g_Settings.iPopupTimeout;

	if (g_Settings.iPopupStyle == 2) {
		pd.colorBack = 0;
		pd.colorText = 0;
	}
	else if (g_Settings.iPopupStyle == 3) {
		pd.colorBack = g_Settings.crPUBkgColour;
		pd.colorText = g_Settings.crPUTextColour;
	}
	else {
		pd.colorBack = g_Settings.crLogBackground;
		pd.colorText = crBkg;
	}

	pd.PluginWindowProc = PopupDlgProc;
	pd.PluginData = si;
	return PUAddPopUpT(&pd);
}

static BOOL DoTrayIcon(SESSION_INFO* si, GCEVENT * gce)
{
	int iEvent = gce->pDest->iType;

	if ( si && (iEvent & si->iLogTrayFlags) ) {
		switch ( iEvent ) {
		case GC_EVENT_MESSAGE|GC_EVENT_HIGHLIGHT :
		case GC_EVENT_ACTION|GC_EVENT_HIGHLIGHT :
			CList_AddEvent(si->hContact, myGlobals.g_IconMsgEvent, szChatIconString, 0, TranslateT("%s wants your attention in %s"), gce->ptszNick, si->ptszName);
			break;
		case GC_EVENT_MESSAGE :
			CList_AddEvent(si->hContact, hIcons[ICON_MESSAGE], szChatIconString, CLEF_ONLYAFEW, TranslateT("%s speaks in %s"), gce->ptszNick, si->ptszName);
			break;
		case GC_EVENT_ACTION:
			CList_AddEvent(si->hContact, hIcons[ICON_ACTION], szChatIconString, CLEF_ONLYAFEW, TranslateT("%s speaks in %s"), gce->ptszNick, si->ptszName);
			break;
		case GC_EVENT_JOIN:
			CList_AddEvent(si->hContact, hIcons[ICON_JOIN], szChatIconString, CLEF_ONLYAFEW, TranslateT("%s has joined %s"), gce->ptszNick, si->ptszName);
			break;
		case GC_EVENT_PART:
			CList_AddEvent(si->hContact, hIcons[ICON_PART], szChatIconString, CLEF_ONLYAFEW, TranslateT("%s has left %s"), gce->ptszNick, si->ptszName);
			break;
		case GC_EVENT_QUIT:
			CList_AddEvent(si->hContact, hIcons[ICON_QUIT], szChatIconString, CLEF_ONLYAFEW, TranslateT("%s has disconnected"), gce->ptszNick);
			break;
		case GC_EVENT_NICK:
			CList_AddEvent(si->hContact, hIcons[ICON_NICK], szChatIconString, CLEF_ONLYAFEW, TranslateT("%s is now known as %s"), gce->ptszNick, gce->pszText);
			break;
		case GC_EVENT_KICK:
			CList_AddEvent(si->hContact, hIcons[ICON_KICK], szChatIconString, CLEF_ONLYAFEW, TranslateT("%s kicked %s from %s"), gce->pszStatus, gce->ptszNick, si->ptszName);
			break;
		case GC_EVENT_NOTICE:
			CList_AddEvent(si->hContact, hIcons[ICON_NOTICE], szChatIconString, CLEF_ONLYAFEW, TranslateT("Notice from %s"), gce->ptszNick);
			break;
		case GC_EVENT_TOPIC:
			CList_AddEvent(si->hContact, hIcons[ICON_TOPIC], szChatIconString, CLEF_ONLYAFEW, TranslateT("Topic change in %s"), si->ptszName);
			break;
		case GC_EVENT_INFORMATION:
			CList_AddEvent(si->hContact, hIcons[ICON_INFO], szChatIconString, CLEF_ONLYAFEW, TranslateT("Information in %s"), si->ptszName);
			break;
		case GC_EVENT_ADDSTATUS:
			CList_AddEvent(si->hContact, hIcons[ICON_ADDSTATUS], szChatIconString, CLEF_ONLYAFEW, TranslateT("%s enables \'%s\' status for %s in %s"), gce->pszText, gce->pszStatus, gce->ptszNick, si->ptszName);
			break;
		case GC_EVENT_REMOVESTATUS:
			CList_AddEvent(si->hContact, hIcons[ICON_REMSTATUS], szChatIconString, CLEF_ONLYAFEW, TranslateT("%s disables \'%s\' status for %s in %s"), gce->pszText, gce->pszStatus, gce->ptszNick, si->ptszName);
			break;
	}	}

	return TRUE;
}

static BOOL DoPopup(SESSION_INFO* si, GCEVENT* gce, struct MessageWindowData* dat)
{
	int iEvent = gce->pDest->iType;
	struct ContainerWindowData *pContainer = dat ? dat->pContainer : NULL;
	char *szProto = dat ? dat->szProto : si->pszModule;

	if (si && (iEvent & si->iLogPopupFlags))
	{

		if (nen_options.iDisable || (dat == 0 && g_Settings.SkipWhenNoWindow))                          // no popups at all. Period
			return 0;
		/*
		* check the status mode against the status mask
		*/

		if (nen_options.bSimpleMode) {
			switch(nen_options.bSimpleMode) {
			case 1:
				goto passed;
			case 3:
				if (dat == 0)            // window not open
					goto passed;
				else
					return 0;
			case 2:
				if (dat == 0)
					goto passed;
				if (pContainer != NULL) {
					if (IsIconic(pContainer->hwnd) || GetForegroundWindow() != pContainer->hwnd)
						goto passed;
				}
				return 0;
			default:
				return 0;
			}
		}

		if (nen_options.dwStatusMask != -1) {
			DWORD dwStatus = 0;
			if (szProto != NULL) {
				dwStatus = (DWORD)CallProtoService(szProto, PS_GETSTATUS, 0, 0);
				if (!(dwStatus == 0 || dwStatus <= ID_STATUS_OFFLINE || ((1<<(dwStatus - ID_STATUS_ONLINE)) & nen_options.dwStatusMask)))              // should never happen, but...
					return 0;
			}
		}
		if (dat && pContainer != 0) {                // message window is open, need to check the container config if we want to see a popup nonetheless
			if (nen_options.bWindowCheck)                   // no popups at all for open windows... no exceptions
				return 0;
			if (pContainer->dwFlags & CNT_DONTREPORT && (IsIconic(pContainer->hwnd) || pContainer->bInTray))        // in tray counts as "minimised"
				goto passed;
			if (pContainer->dwFlags & CNT_DONTREPORTUNFOCUSED) {
				if (!IsIconic(pContainer->hwnd) && GetForegroundWindow() != pContainer->hwnd && GetActiveWindow() != pContainer->hwnd)
					goto passed;
			}
			if (pContainer->dwFlags & CNT_ALWAYSREPORTINACTIVE) {
				if (pContainer->hwndActive == si->hWnd)
					return 0;
				else
					goto passed;
			}
			return 0;
		}
passed:
		switch (iEvent) {
		case GC_EVENT_MESSAGE|GC_EVENT_HIGHLIGHT :
			ShowPopup(si->hContact, si, LoadSkinnedIcon(SKINICON_EVENT_MESSAGE), si->pszModule, si->ptszName, aFonts[16].color, TranslateT("%s says: %s"), gce->ptszNick, RemoveFormatting( gce->ptszText ));
			break;
		case GC_EVENT_ACTION|GC_EVENT_HIGHLIGHT :
			ShowPopup(si->hContact, si, LoadSkinnedIcon(SKINICON_EVENT_MESSAGE), si->pszModule, si->ptszName, aFonts[16].color, _T("%s %s"), gce->ptszNick, RemoveFormatting(gce->ptszText));
			break;
		case GC_EVENT_MESSAGE :
			ShowPopup(si->hContact, si, hIcons[ICON_MESSAGE], si->pszModule, si->ptszName, aFonts[9].color, TranslateT("%s says: %s"), gce->ptszNick, RemoveFormatting( gce->ptszText));
			break;
		case GC_EVENT_ACTION:
			ShowPopup(si->hContact, si, hIcons[ICON_ACTION], si->pszModule, si->ptszName, aFonts[15].color, _T("%s %s"), gce->ptszNick, RemoveFormatting(gce->ptszText));
			break;
		case GC_EVENT_JOIN:
			ShowPopup(si->hContact, si, hIcons[ICON_JOIN], si->pszModule, si->ptszName, aFonts[3].color, TranslateT("%s has joined"), gce->ptszNick);
			break;
		case GC_EVENT_PART:
			if (!gce->pszText)
				ShowPopup(si->hContact, si, hIcons[ICON_PART], si->pszModule, si->ptszName, aFonts[4].color, TranslateT("%s has left"), gce->ptszNick);
			else
				ShowPopup(si->hContact, si, hIcons[ICON_PART], si->pszModule, si->ptszName, aFonts[4].color, TranslateT("%s has left (%s)"), gce->ptszNick, RemoveFormatting(gce->ptszText));
				break;
		case GC_EVENT_QUIT:
			if (!gce->pszText)
				ShowPopup(si->hContact, si, hIcons[ICON_QUIT], si->pszModule, si->ptszName, aFonts[5].color, TranslateT("%s has disconnected"), gce->ptszNick);
			else
				ShowPopup(si->hContact, si, hIcons[ICON_QUIT], si->pszModule, si->ptszName, aFonts[5].color, TranslateT("%s has disconnected (%s)"), gce->ptszNick,RemoveFormatting(gce->ptszText));
				break;
		case GC_EVENT_NICK:
			ShowPopup(si->hContact, si, hIcons[ICON_NICK], si->pszModule, si->ptszName, aFonts[7].color, TranslateT("%s is now known as %s"), gce->ptszNick, gce->ptszText);
			break;
		case GC_EVENT_KICK:
			if (!gce->pszText)
				ShowPopup(si->hContact, si, hIcons[ICON_KICK], si->pszModule, si->ptszName, aFonts[6].color, TranslateT("%s kicked %s"), (char *)gce->pszStatus, gce->ptszNick);
			else
				ShowPopup(si->hContact, si, hIcons[ICON_KICK], si->pszModule, si->ptszName, aFonts[6].color, TranslateT("%s kicked %s (%s)"), (char *)gce->pszStatus, gce->ptszNick, RemoveFormatting(gce->ptszText));
			break;
		case GC_EVENT_NOTICE:
			ShowPopup(si->hContact, si, hIcons[ICON_NOTICE], si->pszModule, si->ptszName, aFonts[8].color, TranslateT("Notice from %s: %s"), gce->ptszNick, RemoveFormatting(gce->ptszText));
			break;
		case GC_EVENT_TOPIC:
			if (!gce->ptszNick)
				ShowPopup(si->hContact, si, hIcons[ICON_TOPIC], si->pszModule, si->ptszName, aFonts[11].color, TranslateT("The topic is \'%s\'"), RemoveFormatting(gce->ptszText));
			else
				ShowPopup(si->hContact, si, hIcons[ICON_TOPIC], si->pszModule, si->ptszName, aFonts[11].color, TranslateT("The topic is \'%s\' (set by %s)"), RemoveFormatting(gce->ptszText), gce->ptszNick);
			break;
		case GC_EVENT_INFORMATION:
			ShowPopup(si->hContact, si, hIcons[ICON_INFO], si->pszModule, si->ptszName, aFonts[12].color, _T("%s"), RemoveFormatting(gce->ptszText));
			break;
		case GC_EVENT_ADDSTATUS:
			ShowPopup(si->hContact, si, hIcons[ICON_ADDSTATUS], si->pszModule, si->ptszName, aFonts[13].color, TranslateT("%s enables \'%s\' status for %s"), gce->ptszText, (char *)gce->pszStatus, gce->ptszNick);
			break;
		case GC_EVENT_REMOVESTATUS:
			ShowPopup(si->hContact, si, hIcons[ICON_REMSTATUS], si->pszModule, si->ptszName, aFonts[14].color, TranslateT("%s disables \'%s\' status for %s"), gce->ptszText, (char *)gce->pszStatus, gce->ptszNick);
			break;
	}	}

	return TRUE;
}

typedef struct 
{
	struct MessageWindowData *dat;
	SESSION_INFO* si;
	const char* sound;
	int   iEvent;
	HICON hNotifyIcon;
	BOOL  bActiveTab, bHighlight, bInactive, bMustFlash, bMustAutoswitch;
}
	FLASH_PARAMS;

static void DoFlashAndSoundThread( FLASH_PARAMS* p )
{
	if (p->sound) {
		if ( !lstrcmpA( p->sound, "ChatHighlight"))
			SkinPlaySound( p->sound );
		else
			Chat_PlaySound( p->sound, p->si->hWnd, p->dat);
	}

	if (p->si->hWnd && p->dat) {
		HWND hwndTab = GetParent(p->si->hWnd);
		BOOL bForcedIcon = ( p->hNotifyIcon == hIcons[ICON_HIGHLIGHT] || p->hNotifyIcon == hIcons[ICON_MESSAGE] );

		//if (IsIconic(dat->pContainer->hwnd) || 1) { //dat->pContainer->hwndActive != si->hWnd) {
		if ((p->iEvent & p->si->iLogTrayFlags) || bForcedIcon) { //dat->pContainer->hwndActive != si->hWnd) {
			if (!p->bActiveTab) {
				if (p->hNotifyIcon == hIcons[ICON_HIGHLIGHT])
					p->dat->iFlashIcon = p->hNotifyIcon;
				else {
					if (p->dat->iFlashIcon != hIcons[ICON_HIGHLIGHT] && p->dat->iFlashIcon != hIcons[ICON_MESSAGE])
						p->dat->iFlashIcon = p->hNotifyIcon;
				}
				if (p->bMustFlash) {
					SetTimer(p->si->hWnd, TIMERID_FLASHWND, TIMEOUT_FLASHWND, NULL);
					p->dat->mayFlashTab = TRUE;
		}	}	}

		// autoswitch tab..
		if (p->bMustAutoswitch) {
			if ((p->dat->pContainer->bInTray || IsIconic(p->dat->pContainer->hwnd)) && !IsZoomed(p->dat->pContainer->hwnd) && myGlobals.m_AutoSwitchTabs && p->dat->pContainer->hwndActive != p->si->hWnd) {
				int iItem = GetTabIndexFromHWND(hwndTab, p->si->hWnd);
				if (iItem >= 0) {
					TabCtrl_SetCurSel(hwndTab, iItem);
					ShowWindow(p->dat->pContainer->hwndActive, SW_HIDE);
					p->dat->pContainer->hwndActive = p->si->hWnd;
					SendMessage(p->dat->pContainer->hwnd, DM_UPDATETITLE, (WPARAM)p->dat->hContact, 0);
					p->dat->pContainer->dwFlags |= CNT_DEFERREDTABSELECT;
		}	}	}

		/*
		* flash window if it is not focused
		*/
		if (p->bMustFlash && p->bInactive)
			if (!(p->dat->pContainer->dwFlags & CNT_NOFLASH))
				FlashContainer(p->dat->pContainer, 1, 0);

		if (p->hNotifyIcon && p->bInactive && ((p->iEvent & p->si->iLogTrayFlags) || bForcedIcon)) {
			HICON hIcon;

			if (p->bMustFlash)
				p->dat->hTabIcon = p->hNotifyIcon;
			else if (p->dat->iFlashIcon) { //if (!bActiveTab) {
				TCITEM item = {0};

				p->dat->hTabIcon = p->dat->iFlashIcon;
				item.mask = TCIF_IMAGE;
				item.iImage = 0;
				TabCtrl_SetItem(GetParent(p->si->hWnd), p->dat->iTabID, &item);
			}
			hIcon = (HICON)SendMessage(p->dat->pContainer->hwnd, WM_GETICON, ICON_BIG, 0);
			if (p->hNotifyIcon == hIcons[ICON_HIGHLIGHT] || (hIcon != hIcons[ICON_MESSAGE] && hIcon != hIcons[ICON_HIGHLIGHT])) {
				SendMessage(p->dat->pContainer->hwnd, DM_SETICON, ICON_BIG, (LPARAM)p->hNotifyIcon);
				p->dat->pContainer->dwFlags |= CNT_NEED_UPDATETITLE;
	}	}	}

	if (p->bMustFlash)
		UpdateTrayMenu(p->dat, p->si->wStatus, p->si->pszModule, p->dat ? p->dat->szStatus : NULL, p->si->hContact, p->bHighlight ? 2 : 1);

	free( p );
}

BOOL DoSoundsFlashPopupTrayStuff(SESSION_INFO* si, GCEVENT * gce, BOOL bHighlight, int bManyFix)
{
	FLASH_PARAMS* params;
	struct MessageWindowData *dat = 0;
	HWND hwndContainer = 0;

	if (!gce || si == NULL || gce->bIsMe || si->iType == GCW_SERVER)
		return FALSE;

	params = (FLASH_PARAMS*)calloc( 1, sizeof(FLASH_PARAMS));
	params->si = si;
	params->bInactive = TRUE;
	params->bActiveTab = params->bMustFlash = params->bMustAutoswitch = FALSE;

	if (si->hWnd) {
		params->dat = dat = (struct MessageWindowData *)GetWindowLong(si->hWnd, GWL_USERDATA);
		if (dat) {
			hwndContainer = dat->pContainer->hwnd;
			params->bInactive = hwndContainer != GetForegroundWindow();
			params->bActiveTab = (dat->pContainer->hwndActive == si->hWnd);
		}
	}
	params->iEvent = gce->pDest->iType;

	if ( bHighlight ) {
		gce->pDest->iType |= GC_EVENT_HIGHLIGHT;
		if (params->bInactive || !g_Settings.SoundsFocus)
			params->sound = "ChatHighlight";
		if (DBGetContactSettingByte(si->hContact, "CList", "Hidden", 0) != 0)
			DBDeleteContactSetting(si->hContact, "CList", "Hidden");
		if (params->bInactive)
			DoTrayIcon(si, gce);
		if (dat || !g_Settings.SkipWhenNoWindow)
			DoPopup(si, gce, dat);
		if (params->bInactive && g_TabSession.hWnd)
			SendMessage(g_TabSession.hWnd, GC_SETMESSAGEHIGHLIGHT, 0, (LPARAM) si);
		if (g_Settings.FlashWindowHightlight && params->bInactive)
			params->bMustFlash = TRUE;
		params->bMustAutoswitch = TRUE;
		params->hNotifyIcon = hIcons[ICON_HIGHLIGHT];
	}
	else {
		// do blinking icons in tray
		if (params->bInactive || !g_Settings.TrayIconInactiveOnly)
			DoTrayIcon(si, gce);

		// stupid thing to not create multiple popups for a QUIT event for instance
		if (bManyFix == 0) {
			// do popups
			if (dat || !g_Settings.SkipWhenNoWindow)
				DoPopup(si, gce, dat);

			// do sounds and flashing
			switch (params->iEvent) {
			case GC_EVENT_JOIN:
				if (params->bInactive || !g_Settings.SoundsFocus) {
					params->sound = "ChatJoin";
					params->hNotifyIcon = hIcons[ICON_JOIN];
				}
				break;
			case GC_EVENT_PART:
				if (params->bInactive || !g_Settings.SoundsFocus) {
					params->sound = "ChatPart";
					params->hNotifyIcon = hIcons[ICON_PART];
				}
				break;
			case GC_EVENT_QUIT:
				if (params->bInactive || !g_Settings.SoundsFocus) {
					params->sound = "ChatQuit";
					params->hNotifyIcon = hIcons[ICON_QUIT];
				}
				break;
			case GC_EVENT_ADDSTATUS:
			case GC_EVENT_REMOVESTATUS:
				if (params->bInactive || !g_Settings.SoundsFocus) {
					params->sound = "ChatMode";
					params->hNotifyIcon = hIcons[params->iEvent == GC_EVENT_ADDSTATUS ? ICON_ADDSTATUS : ICON_REMSTATUS];
				}
				break;
			case GC_EVENT_KICK:
				if (params->bInactive || !g_Settings.SoundsFocus) {
					params->sound = "ChatKick";
					params->hNotifyIcon = hIcons[ICON_KICK];
				}
				break;
			case GC_EVENT_MESSAGE:
				if (params->bInactive || !g_Settings.SoundsFocus)
					params->sound = "ChatMessage";
				if (params->bInactive && !(si->wState&STATE_TALK)) {
					si->wState |= STATE_TALK;
					DBWriteContactSettingWord(si->hContact, si->pszModule,"ApparentMode",(LPARAM)(WORD) 40071);
				}
				break;
			case GC_EVENT_ACTION:
				if (params->bInactive || !g_Settings.SoundsFocus) {
					params->sound = "ChatAction";
					params->hNotifyIcon = hIcons[ICON_ACTION];
				}
				break;
			case GC_EVENT_NICK:
				if (params->bInactive || !g_Settings.SoundsFocus) {
					params->sound = "ChatNick";
					params->hNotifyIcon = hIcons[ICON_NICK];
				}
				break;
			case GC_EVENT_NOTICE:
				if (params->bInactive || !g_Settings.SoundsFocus) {
					params->sound = "ChatNotice";
					params->hNotifyIcon = hIcons[ICON_NOTICE];
				}
				break;
			case GC_EVENT_TOPIC:
				if (params->bInactive || !g_Settings.SoundsFocus) {
					params->sound = "ChatTopic";
					params->hNotifyIcon = hIcons[ICON_TOPIC];
				}
				break;
		}	}
		else {
			switch(params->iEvent) {
			case GC_EVENT_JOIN:
				params->hNotifyIcon = hIcons[ICON_JOIN];
				break;
			case GC_EVENT_PART:
				params->hNotifyIcon = hIcons[ICON_PART];
				break;
			case GC_EVENT_QUIT:
				params->hNotifyIcon = hIcons[ICON_QUIT];
				break;
			case GC_EVENT_KICK:
				params->hNotifyIcon = hIcons[ICON_KICK];
				break;
			case GC_EVENT_ACTION:
				params->hNotifyIcon = hIcons[ICON_ACTION];
				break;
			case GC_EVENT_NICK:
				params->hNotifyIcon = hIcons[ICON_NICK];
				break;
			case GC_EVENT_NOTICE:
				params->hNotifyIcon = hIcons[ICON_NOTICE];
				break;
			case GC_EVENT_TOPIC:
				params->hNotifyIcon = hIcons[ICON_TOPIC];
				break;
			case GC_EVENT_ADDSTATUS:
				params->hNotifyIcon = hIcons[ICON_ADDSTATUS];
				break;
			case GC_EVENT_REMOVESTATUS:
				params->hNotifyIcon = hIcons[ICON_REMSTATUS];
				break;
		}	}

		if (params->iEvent == GC_EVENT_MESSAGE) {
   		params->bMustAutoswitch = TRUE;
			if (g_Settings.FlashWindow)
      		params->bMustFlash = TRUE;
			params->hNotifyIcon = hIcons[ICON_MESSAGE];
	}	}

	mir_forkthread( DoFlashAndSoundThread, params );
	return TRUE;
}

int Chat_GetColorIndex(const char* pszModule, COLORREF cr)
{
	MODULEINFO * pMod = MM_FindModule(pszModule);
	int i = 0;

	if (!pMod || pMod->nColorCount == 0)
		return -1;

	for (i = 0; i < pMod->nColorCount; i++)
		if (pMod->crColors[i] == cr)
			return i;

	return -1;
}

// obscure function that is used to make sure that any of the colors
// passed by the protocol is used as fore- or background color
// in the messagebox. THis is to vvercome limitations in the richedit
// that I do not know currently how to fix

void CheckColorsInModule(const char* pszModule)
{
	MODULEINFO * pMod = MM_FindModule( pszModule );
	int i = 0;
	COLORREF crFG;
	COLORREF crBG = (COLORREF)DBGetContactSettingDword(NULL, FONTMODULE, "inputbg", GetSysColor(COLOR_WINDOW));

	LoadLogfont(MSGFONTID_MESSAGEAREA, NULL, &crFG, FONTMODULE);

	if ( !pMod )
		return;

	for (i = 0; i < pMod->nColorCount; i++) {
		if (pMod->crColors[i] == crFG || pMod->crColors[i] == crBG) {
			if (pMod->crColors[i] == RGB(255,255,255))
				pMod->crColors[i]--;
			else
				pMod->crColors[i]++;
}	}	}

TCHAR* my_strstri( const TCHAR* s1, const TCHAR* s2)
{
	int i,j,k;
	for(i=0;s1[i];i++)
		for(j=i,k=0; tolower(s1[j]) == tolower(s2[k]);j++,k++)
			if (!s2[k+1])
				return (TCHAR*)(s1+i);

	return NULL;
}

BOOL IsHighlighted(SESSION_INFO* si, const TCHAR* pszText)
{
	if ( g_Settings.HighlightEnabled && g_Settings.pszHighlightWords && pszText && si->pMe ) {
		TCHAR* p1 = g_Settings.pszHighlightWords;
		TCHAR* p2 = NULL;
		const TCHAR* p3 = pszText;
		static TCHAR szWord1[1000];
		static TCHAR szWord2[1000];
		static TCHAR szTrimString[] = _T(":,.!?;\'>)");

		// compare word for word
		while (*p1 != '\0') {
			// find the next/first word in the highlight word string
			// skip 'spaces' be4 the word
			while(*p1 == ' ' && *p1 != '\0')
				p1 += 1;

			//find the end of the word
			p2 = _tcschr(p1, ' ');
			if (!p2)
				p2 = _tcschr(p1, '\0');
			if (p1 == p2)
				return FALSE;

			// copy the word into szWord1
			lstrcpyn(szWord1, p1, p2-p1>998?999:p2-p1+1);
			p1 = p2;

			// replace %m with the users nickname
			p2 = _tcschr(szWord1, '%');
			if (p2 && p2[1] == 'm') {
				TCHAR szTemp[50];

				p2[1] = 's';
				lstrcpyn(szTemp, szWord1, 999);
				mir_sntprintf(szWord1, SIZEOF(szWord1), szTemp, si->pMe->pszNick);
			}

			// time to get the next/first word in the incoming text string
			while(*p3 != '\0')
			{
				// skip 'spaces' be4 the word
				while(*p3 == ' ' && *p3 != '\0')
					p3 += 1;

				//find the end of the word
				p2 = _tcschr(p3, ' ');
				if (!p2)
					p2 = _tcschr(p3, '\0');


				if (p3 != p2) {
					// eliminate ending character if needed
					if (p2-p3 > 1 && _tcschr(szTrimString, p2[-1]))
						p2 -= 1;

					// copy the word into szWord2 and remove formatting
					lstrcpyn(szWord2, p3, p2-p3>998?999:p2-p3+1);

					// reset the pointer if it was touched because of an ending character
					if (*p2 != '\0' && *p2 != ' ')
						p2 += 1;
					p3 = p2;

					CharLower(szWord1);
					CharLower(szWord2);

					// compare the words, using wildcards
					if (WCCmp(szWord1, RemoveFormatting(szWord2)))
						return TRUE;
			} 	}

			p3 = pszText;
	}	}

	return FALSE;
}

BOOL LogToFile(SESSION_INFO* si, GCEVENT * gce)
{
	MODULEINFO * mi = NULL;
	TCHAR szBuffer[4096];
	TCHAR szLine[4096];
	TCHAR szTime[100];
	FILE *hFile = NULL;
	char szFile[MAX_PATH];
	char szName[MAX_PATH];
	char szFolder[MAX_PATH];
	char p = '\0', *pszSessionName;
	szBuffer[0] = '\0';

	if (!si || !gce)
		return FALSE;

	mi = MM_FindModule(si->pszModule);
	if ( !mi )
		return FALSE;

 	mir_snprintf(szName, MAX_PATH,"%s",mi->pszModDispName);
	ValidateFilename(szName);
	mir_snprintf(szFolder, MAX_PATH,"%s\\%s", g_Settings.pszLogDir, szName );

	if (!PathIsDirectoryA(szFolder))
		CreateDirectoryA(szFolder, NULL);

	pszSessionName = t2a( si->ptszID, 0 );
	mir_snprintf( szName, MAX_PATH,"%s.log", pszSessionName );
	ValidateFilename(szName);
	mir_free( pszSessionName );

	mir_snprintf(szFile, MAX_PATH,"%s\\%s", szFolder, szName );
	lstrcpyn(szTime, MakeTimeStamp(g_Settings.pszTimeStampLog, gce->time), 99);

	hFile = fopen(szFile,"at+");
	if (hFile)
	{
		TCHAR szTemp[512], szTemp2[512];
		TCHAR* pszNick = NULL;
		if ( gce->ptszNick ) {
			if ( g_Settings.LogLimitNames && lstrlen(gce->ptszNick) > 20 ) {
				lstrcpyn(szTemp2, gce->ptszNick, 20);
				lstrcpyn(szTemp2+20, _T("..."), 4);
			}
			else lstrcpyn(szTemp2, gce->ptszNick, 511);

			if (gce->pszUserInfo)
				mir_sntprintf(szTemp, SIZEOF(szTemp), _T("%s (%s)"), szTemp2, gce->pszUserInfo);
			else
				mir_sntprintf(szTemp, SIZEOF(szTemp), _T("%s"), szTemp2);
			pszNick = szTemp;
		}
		switch (gce->pDest->iType) {
		case GC_EVENT_MESSAGE:
		case GC_EVENT_MESSAGE|GC_EVENT_HIGHLIGHT:
			p = '*';
			mir_sntprintf(szBuffer, SIZEOF(szBuffer), _T("%s * %s"), gce->ptszNick, RemoveFormatting(gce->ptszText));
			break;
		case GC_EVENT_ACTION:
		case GC_EVENT_ACTION|GC_EVENT_HIGHLIGHT:
			p = '*';
			mir_sntprintf(szBuffer, SIZEOF(szBuffer), _T("%s %s"), gce->ptszNick, RemoveFormatting(gce->ptszText));
			break;
		case GC_EVENT_JOIN:
			p = '>';
			mir_sntprintf(szBuffer, SIZEOF(szBuffer), TranslateT("%s has joined"), (char *)pszNick);
			break;
		case GC_EVENT_PART:
			p = '<';
			if (!gce->pszText)
				mir_sntprintf(szBuffer, SIZEOF(szBuffer), TranslateT("%s has left"), (char *)pszNick);
			else
				mir_sntprintf(szBuffer, SIZEOF(szBuffer), TranslateT("%s has left (%s)"), (char *)pszNick, RemoveFormatting(gce->ptszText));
				break;
		case GC_EVENT_QUIT:
			p = '<';
			if (!gce->pszText)
				mir_sntprintf(szBuffer, SIZEOF(szBuffer), TranslateT("%s has disconnected"), (char *)pszNick);
			else
				mir_sntprintf(szBuffer, SIZEOF(szBuffer), TranslateT("%s has disconnected (%s)"), (char *)pszNick,RemoveFormatting(gce->ptszText));
				break;
		case GC_EVENT_NICK:
			p = '^';
			mir_sntprintf(szBuffer, SIZEOF(szBuffer), TranslateT("%s is now known as %s"), gce->ptszNick, gce->ptszText);
			break;
		case GC_EVENT_KICK:
			p = '~';
			if (!gce->pszText)
				mir_sntprintf(szBuffer, SIZEOF(szBuffer), TranslateT("%s kicked %s"), (char *)gce->pszStatus, gce->ptszNick);
			else
				mir_sntprintf(szBuffer, SIZEOF(szBuffer), TranslateT("%s kicked %s (%s)"), (char *)gce->pszStatus, gce->ptszNick, RemoveFormatting(gce->ptszText));
			break;
		case GC_EVENT_NOTICE:
			p = '¤';
			mir_sntprintf(szBuffer, SIZEOF(szBuffer), TranslateT("Notice from %s: %s"), gce->ptszNick, RemoveFormatting(gce->ptszText));
			break;
		case GC_EVENT_TOPIC:
			p = '#';
			if (!gce->pszNick)
				mir_sntprintf(szBuffer, SIZEOF(szBuffer), TranslateT("The topic is \'%s\'"), RemoveFormatting(gce->ptszText));
			else
				mir_sntprintf(szBuffer, SIZEOF(szBuffer), TranslateT("The topic is \'%s\' (set by %s)"), RemoveFormatting(gce->ptszText), gce->ptszNick);
			break;
		case GC_EVENT_INFORMATION:
			p = '!';
			mir_sntprintf(szBuffer, SIZEOF(szBuffer), _T("%s"), RemoveFormatting(gce->ptszText));
			break;
		case GC_EVENT_ADDSTATUS:
			p = '+';
			mir_sntprintf(szBuffer, SIZEOF(szBuffer), TranslateT("%s enables \'%s\' status for %s"), gce->ptszText, (char *)gce->pszStatus, gce->ptszNick);
			break;
		case GC_EVENT_REMOVESTATUS:
			p = '-';
			mir_sntprintf(szBuffer, SIZEOF(szBuffer), TranslateT("%s disables \'%s\' status for %s"), gce->ptszText, (char *)gce->pszStatus, gce->ptszNick);
			break;
		}
		if (p)
			mir_sntprintf(szLine, SIZEOF(szLine), TranslateT("%s %c %s\n"), szTime, p, szBuffer);
		else
			mir_sntprintf(szLine, SIZEOF(szLine), TranslateT("%s %s\n"), szTime, szBuffer);

		if ( szLine[0] ) {
			char* p = t2a( szLine, 0 );
			fputs(p, hFile);
			mir_free( p );

			if ( g_Settings.LoggingLimit > 0 ) {
				DWORD dwSize;
				DWORD trimlimit;

				fseek(hFile,0,SEEK_END);
				dwSize = ftell(hFile);
				rewind (hFile);
				trimlimit = g_Settings.LoggingLimit*1024+ 1024*10;
				if (dwSize > trimlimit) {
					BYTE * pBuffer = 0;
					BYTE * pBufferTemp = 0;
					int read = 0;

					pBuffer = (BYTE *)mir_alloc(g_Settings.LoggingLimit*1024+1);
					pBuffer[g_Settings.LoggingLimit*1024] = '\0';
					fseek(hFile,-g_Settings.LoggingLimit*1024,SEEK_END);
					read = fread(pBuffer, 1, g_Settings.LoggingLimit*1024, hFile);
					fclose(hFile);
					hFile = NULL;

					// trim to whole lines, should help with broken log files I hope.
					pBufferTemp = strchr(pBuffer, '\n');
					if ( pBufferTemp ) {
						pBufferTemp++;
						read = read - (pBufferTemp - pBuffer);
					}
					else pBufferTemp = pBuffer;

					if (read > 0) {
						hFile = fopen(szFile,"wt");
						if (hFile ) {
							fwrite(pBufferTemp, 1, read, hFile);
							fclose(hFile); hFile = NULL;
					}	}

					mir_free(pBuffer);
		}	}	}

		if (hFile)
			fclose(hFile); hFile = NULL;
		return TRUE;
	}

	return FALSE;
}

UINT CreateGCMenu(HWND hwndDlg, HMENU *hMenu, int iIndex, POINT pt, SESSION_INFO* si, TCHAR* pszUID, TCHAR* pszWordText)
{
	GCMENUITEMS gcmi = {0};
	int i;
	HMENU hSubMenu = 0;
    DWORD codepage = DBGetContactSettingDword(si->hContact, SRMSGMOD_T, "ANSIcodepage", 0);
    int pos;

	*hMenu = GetSubMenu(g_hMenu, iIndex);
	CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM) *hMenu, 0);
	gcmi.pszID = si->ptszID;
	gcmi.pszModule = si->pszModule;
	gcmi.pszUID = pszUID;

	if (iIndex == 1) {
		int i = GetRichTextLength(GetDlgItem(hwndDlg, IDC_CHAT_LOG));

		EnableMenuItem(*hMenu, ID_CLEARLOG, MF_ENABLED);
		EnableMenuItem(*hMenu, ID_COPYALL, MF_ENABLED);
		ModifyMenu(*hMenu, 4, MF_GRAYED|MF_BYPOSITION, 4, NULL);
		if (!i) {
			EnableMenuItem(*hMenu, ID_COPYALL, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(*hMenu, ID_CLEARLOG, MF_BYCOMMAND | MF_GRAYED);
			if (pszWordText && pszWordText[0])
				ModifyMenu(*hMenu, 4, MF_ENABLED|MF_BYPOSITION, 4, NULL);
		}

		if ( pszWordText && pszWordText[0] ) {
			TCHAR szMenuText[4096];
			mir_sntprintf( szMenuText, 4096, _T("Look up \'%s\':"), pszWordText );
			ModifyMenu( *hMenu, 4, MF_STRING|MF_BYPOSITION, 4, szMenuText );
		}
		else ModifyMenu( *hMenu, 4, MF_STRING|MF_GRAYED|MF_BYPOSITION, 4, TranslateT( "No word to look up" ));
		gcmi.Type = MENU_ON_LOG;
	}
	else if (iIndex == 0)
	{
		TCHAR szTemp[30], szTemp2[30];
		lstrcpyn(szTemp, TranslateT("&Message"), 24);
		if ( pszUID )
			mir_sntprintf( szTemp2, SIZEOF(szTemp2), _T("%s %s"), szTemp, pszUID);
		else
			lstrcpyn(szTemp2, szTemp, 24);

		if ( lstrlen(szTemp2) > 22 )
			lstrcpyn( szTemp2+22, _T("..."), 4 );
		ModifyMenu( *hMenu, ID_MESS, MF_STRING|MF_BYCOMMAND, ID_MESS, szTemp2 );
		gcmi.Type = MENU_ON_NICKLIST;
	}

	NotifyEventHooks(hBuildMenuEvent, 0, (WPARAM)&gcmi);

	if (gcmi.nItems > 0)
		AppendMenu(*hMenu, MF_SEPARATOR, 0, 0);

	for (i = 0; i < gcmi.nItems; i++) {
		TCHAR* ptszDescr = a2tf( gcmi.Item[i].pszDesc, si->dwFlags, 0 );
		DWORD dwState = gcmi.Item[i].bDisabled ? MF_GRAYED : 0;

		if ( gcmi.Item[i].uType == MENU_NEWPOPUP ) {
			hSubMenu = CreateMenu();
			AppendMenu( *hMenu, dwState|MF_POPUP, (UINT)hSubMenu, ptszDescr );
		}
		else if (gcmi.Item[i].uType == MENU_POPUPHMENU)
			AppendMenu( hSubMenu==0?*hMenu:hSubMenu, dwState|MF_POPUP, gcmi.Item[i].dwID, ptszDescr);
		else if (gcmi.Item[i].uType == MENU_POPUPITEM)
			AppendMenu( hSubMenu==0?*hMenu:hSubMenu, dwState|MF_STRING, gcmi.Item[i].dwID, ptszDescr);
		else if (gcmi.Item[i].uType == MENU_POPUPCHECK)
			AppendMenu( hSubMenu==0?*hMenu:hSubMenu, dwState|MF_CHECKED|MF_STRING, gcmi.Item[i].dwID, ptszDescr);
		else if (gcmi.Item[i].uType == MENU_POPUPSEPARATOR)
			AppendMenu( hSubMenu==0?*hMenu:hSubMenu, MF_SEPARATOR, 0, ptszDescr );
		else if (gcmi.Item[i].uType == MENU_SEPARATOR)
			AppendMenu( *hMenu, MF_SEPARATOR, 0, ptszDescr );
		else if (gcmi.Item[i].uType == MENU_HMENU)
			AppendMenu( *hMenu, dwState|MF_POPUP, gcmi.Item[i].dwID, ptszDescr);
		else if (gcmi.Item[i].uType == MENU_ITEM)
			AppendMenu( *hMenu, dwState|MF_STRING, gcmi.Item[i].dwID, ptszDescr );
		else if (gcmi.Item[i].uType == MENU_CHECK)
			AppendMenu( *hMenu, dwState|MF_CHECKED|MF_STRING, gcmi.Item[i].dwID, ptszDescr );

		mir_free( ptszDescr );
	}

    if(iIndex == 1 && si->iType != GCW_SERVER && !(si->dwFlags && GC_UNICODE)) {
        AppendMenu(*hMenu, MF_SEPARATOR, 0, 0);
        InsertMenu(myGlobals.g_hMenuEncoding, 1, MF_BYPOSITION | MF_STRING, (UINT_PTR)CP_UTF8, TranslateT("UTF-8"));
        pos = GetMenuItemCount(*hMenu);
        InsertMenu(*hMenu, pos, MF_BYPOSITION | MF_POPUP, (UINT_PTR) myGlobals.g_hMenuEncoding, TranslateT("Character Encoding"));
        for(i = 0; i < GetMenuItemCount(myGlobals.g_hMenuEncoding); i++)
            CheckMenuItem(myGlobals.g_hMenuEncoding, i, MF_BYPOSITION | MF_UNCHECKED);
        if(codepage == CP_ACP)
            CheckMenuItem(myGlobals.g_hMenuEncoding, 0, MF_BYPOSITION | MF_CHECKED);
        else
            CheckMenuItem(myGlobals.g_hMenuEncoding, codepage, MF_BYCOMMAND | MF_CHECKED);

    }

	return TrackPopupMenu(*hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwndDlg, NULL);
}

void DestroyGCMenu(HMENU *hMenu, int iIndex)
{
	MENUITEMINFO mi;
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_SUBMENU;
	while(GetMenuItemInfo(*hMenu, iIndex, TRUE, &mi))
	{
		if (mi.hSubMenu != NULL)
			DestroyMenu(mi.hSubMenu);
		RemoveMenu(*hMenu, iIndex, MF_BYPOSITION);
	}
}

BOOL DoEventHookAsync(HWND hwnd, const TCHAR* pszID, const char* pszModule, int iType, TCHAR* pszUID, TCHAR* pszText, DWORD dwItem)
{
	SESSION_INFO* si;
	GCHOOK* gch = (GCHOOK*)mir_alloc( sizeof( GCHOOK ));
	GCDEST* gcd = (GCDEST*)mir_alloc( sizeof( GCDEST ));

	memset( gch, 0, sizeof( GCHOOK ));
	memset( gcd, 0, sizeof( GCDEST ));

	replaceStrA( &gcd->pszModule, pszModule);
	#if defined( _UNICODE )
		if (( si = SM_FindSession(pszID, pszModule)) == NULL )
			return FALSE;

		if ( !( si->dwFlags & GC_UNICODE )) {
            DWORD dwCP = DBGetContactSettingDword(si->hContact, SRMSGMOD_T, "ANSIcodepage", 0);
			gcd->pszID = t2a( pszID, 0 );
			gch->pszUID = t2a( pszUID, 0 );
			gch->pszText = t2a( pszText, dwCP );
		}
		else {
	#endif
			replaceStr( &gcd->ptszID, pszID );
			replaceStr( &gch->ptszUID, pszUID);
			replaceStr( &gch->ptszText, pszText);
	#if defined( _UNICODE )
		}
	#endif
	gcd->iType = iType;
	gch->dwData = dwItem;
	gch->pDest = gcd;
	PostMessage(hwnd, GC_FIREHOOK, 0, (LPARAM) gch);
	return TRUE;
}

BOOL DoEventHook(const TCHAR* pszID, const char* pszModule, int iType, const TCHAR* pszUID, const TCHAR* pszText, DWORD dwItem)
{
	SESSION_INFO* si;
	GCHOOK gch = {0};
	GCDEST gcd = {0};

	gcd.pszModule = (char*)pszModule;
	#if defined( _UNICODE )
		if (( si = SM_FindSession(pszID, pszModule)) == NULL )
			return FALSE;

		if ( !( si->dwFlags & GC_UNICODE )) {
            DWORD dwCP = DBGetContactSettingDword(si->hContact, SRMSGMOD_T, "ANSIcodepage", 0);
			gcd.pszID = t2a( pszID, 0 );
			gch.pszUID = t2a( pszUID, 0 );
			gch.pszText = t2a( pszText, dwCP );
		}
		else {
	#endif
			gcd.ptszID = mir_tstrdup( pszID );
			gch.ptszUID = mir_tstrdup( pszUID );
			gch.ptszText = mir_tstrdup( pszText );
	#if defined( _UNICODE )
		}
	#endif

	gcd.iType = iType;
	gch.dwData = dwItem;
	gch.pDest = &gcd;
	NotifyEventHooks(hSendEvent,0,(WPARAM)&gch);
	
	mir_free( gcd.pszID );
	mir_free( gch.ptszUID );
	mir_free( gch.ptszText );
	return TRUE;
}

BOOL IsEventSupported(int eventType)
{
	switch (eventType)
	{
		// Supported events
		case GC_EVENT_JOIN:
		case GC_EVENT_PART:
		case GC_EVENT_QUIT:
		case GC_EVENT_KICK:
		case GC_EVENT_NICK:
		case GC_EVENT_NOTICE:
		case GC_EVENT_MESSAGE:
		case GC_EVENT_TOPIC:
		case GC_EVENT_INFORMATION:
		case GC_EVENT_ACTION:
		case GC_EVENT_ADDSTATUS:
		case GC_EVENT_REMOVESTATUS:
		case GC_EVENT_CHUID:
		case GC_EVENT_CHANGESESSIONAME:
		case GC_EVENT_ADDGROUP:
		case GC_EVENT_SETITEMDATA:
		case GC_EVENT_GETITEMDATA:
		case GC_EVENT_SETSBTEXT:
		case GC_EVENT_ACK:
		case GC_EVENT_SENDMESSAGE:
		case GC_EVENT_SETSTATUSEX:
		case GC_EVENT_CONTROL:
		case GC_EVENT_SETCONTACTSTATUS:
			return TRUE;
	}

	// Other events
	return FALSE;
}

void ValidateFilename (char * filename)
{
	char *p1 = filename;
	char szForbidden[] = "\\/:*?\"<>|";
	while(*p1 != '\0')
	{
		if (strchr(szForbidden, *p1))
			*p1 = '_';
		p1 +=1;
}	}

TCHAR* a2tf( const TCHAR* str, int flags, DWORD cp )
{
	if ( str == NULL )
		return NULL;

	#if defined( _UNICODE )
		if ( flags & GC_UNICODE )
			return mir_tstrdup( str );
		else {
            int cbLen;
			TCHAR *result;

            if(cp == CP_UTF8)
                return(Utf8_Decode((char *)str));

            if(cp == 0)
                cp = myGlobals.m_LangPackCP; // CallService( MS_LANGPACK_GETCODEPAGE, 0, 0 );
			cbLen = MultiByteToWideChar( cp, 0, (char*)str, -1, 0, 0 );
			result = ( TCHAR* )mir_alloc( sizeof(TCHAR)*( cbLen+1 ));
			if ( result == NULL )
				return NULL;

			MultiByteToWideChar( cp, 0, (char*)str, -1, result, cbLen );
			result[ cbLen ] = 0;
			return result;
		}
	#else
		return mir_strdup( str );
	#endif
}

static char* u2a( const wchar_t* src, DWORD cp )
{
	int  cbLen;
	char *result;

    if(cp == 0)
        cp = myGlobals.m_LangPackCP;
    else if(cp == CP_UTF8)
        return(Utf8_Encode(src));

	cbLen = WideCharToMultiByte( cp, 0, src, -1, NULL, 0, NULL, NULL );
	result = ( char* )mir_alloc( cbLen+1 );
	if ( result == NULL )
		return NULL;

	WideCharToMultiByte( cp, 0, src, -1, result, cbLen, NULL, NULL );
	result[ cbLen ] = 0;
	return result;
}

char* t2a( const TCHAR* src, DWORD cp )
{
	#if defined( _UNICODE )
		return u2a( src, cp );
	#else
		return mir_strdup( src );
	#endif
}

TCHAR* replaceStr( TCHAR** dest, const TCHAR* src )
{
	mir_free( *dest );
	*dest = mir_tstrdup( src );
	return *dest;
}

char* replaceStrA( char** dest, const char* src )
{
	mir_free( *dest );
	*dest = mir_strdup( src );
	return *dest;
}

/*
 * set all filters and notification config for a session
 * uses per channel mask + filterbits, default config as backup
 */

void Chat_SetFilters(SESSION_INFO *si)
{
    DWORD dwFlags_default = 0, dwMask = 0, dwFlags_local = 0;
	int i;

    if(si == NULL)
        return;

    dwFlags_default = DBGetContactSettingDword(NULL, "Chat", "FilterFlags", 0x03E0);
    dwFlags_local = DBGetContactSettingDword(si->hContact, "Chat", "FilterFlags", 0x03E0);
    dwMask = DBGetContactSettingDword(si->hContact, "Chat", "FilterMask", 0);

    si->iLogFilterFlags = dwFlags_default;
    for(i = 0; i < 32; i++) {
        if(dwMask & (1 << i))
            si->iLogFilterFlags = (dwFlags_local & (1 << i) ? si->iLogFilterFlags | (1 << i) : si->iLogFilterFlags & ~(1 << i));
    }

    dwFlags_default = DBGetContactSettingDword(NULL, "Chat", "PopupFlags", 0x03E0);
    dwFlags_local = DBGetContactSettingDword(si->hContact, "Chat", "PopupFlags", 0x03E0);
    dwMask = DBGetContactSettingDword(si->hContact, "Chat", "PopupMask", 0);

    si->iLogPopupFlags = dwFlags_default;
    for(i = 0; i < 32; i++) {
        if(dwMask & (1 << i))
            si->iLogPopupFlags = (dwFlags_local & (1 << i) ? si->iLogPopupFlags | (1 << i) : si->iLogPopupFlags & ~(1 << i));
    }

    dwFlags_default = DBGetContactSettingDword(NULL, "Chat", "TrayIconFlags", 0x03E0);
    dwFlags_local = DBGetContactSettingDword(si->hContact, "Chat", "TrayIconFlags", 0x03E0);
    dwMask = DBGetContactSettingDword(si->hContact, "Chat", "TrayIconMask", 0);

    si->iLogTrayFlags = dwFlags_default;
    for(i = 0; i < 32; i++) {
        if(dwMask & (1 << i))
            si->iLogTrayFlags = (dwFlags_local & (1 << i) ? si->iLogTrayFlags | (1 << i) : si->iLogTrayFlags & ~(1 << i));
    }

    if(si->iLogFilterFlags == 0)
        si->bFilterEnabled = 0;
}



