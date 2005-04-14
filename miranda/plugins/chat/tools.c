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
#include "chat.h"

extern HICON		hIcons[OPTIONS_ICONCOUNT];
extern BOOL			PopUpInstalled;
extern HINSTANCE	g_hInst;
extern FONTINFO		aFonts[OPTIONS_FONTCOUNT];

int GetRichTextLength(HWND hwnd)
{
	GETTEXTLENGTHEX gtl;

	gtl.flags = GTL_PRECISE;
	gtl.codepage = CP_ACP ;
	return (int) SendMessage(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
}
char * RemoveFormatting(char * pszWord)
{
	static char szTemp[10000];
	int i = 0;
	int j = 0;

	if(pszWord == 0 || lstrlen(pszWord) == 0)
		return NULL;

	while(j < 9999 && i <= lstrlen(pszWord))
	{
		if(pszWord[i] == '%')
		{
			switch (pszWord[i+1])
			{
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
				i += 11;
				break;

			default:
				szTemp[j] = pszWord[i];
				j++;
				i++;
				break;
			}

		}
		else
		{
			szTemp[j] = pszWord[i];
			j++;
			i++;
		}
	}

	return (char*)&szTemp;


}
static int CALLBACK PopupDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message) {
		case WM_COMMAND:
			if (HIWORD(wParam) == STN_CLICKED) 
			{ 
				HWND hWindow;
				BOOL bRedrawFlag = FALSE;


				hWindow = (HWND)CallService(MS_POPUP_GETPLUGINDATA, (WPARAM)hWnd,(LPARAM)0);

				if ((int)hWindow < 1)
					return FALSE;

				if(!IsWindowVisible(hWindow))
					bRedrawFlag = TRUE;
				if (IsIconic(hWindow))
					ShowWindow(hWindow, SW_NORMAL);
				ShowWindow(hWindow, SW_SHOW);
				SendMessage(hWindow, WM_SIZE, 0, 0);
				if(bRedrawFlag)
				{
//					SendMessage(hWindow, WM_SETREDRAW, TRUE, 0);
//					InvalidateRect(hWindow, NULL, TRUE);
					SendMessage(hWindow, GC_REDRAWLOG, 0, 0);
				}
				SetWindowPos(hWindow, 0, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE| SWP_NOSIZE | SWP_FRAMECHANGED);
				SetForegroundWindow(hWindow);

				PUDeletePopUp(hWnd);

				return TRUE;
			}
			break;
		case WM_CONTEXTMENU:
			PUDeletePopUp( hWnd );
			break;
		default:
			break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

static int ShowPopup (HANDLE hContact, HWND hWnd, HICON hIcon, char* pszProtoName, char * pszRoomName, COLORREF crBkg, BOOL bBroadcast, char * fmt, ...)
{
	POPUPDATAEX pd = {0};
	va_list marker;
	static char szBuf[4*1024];

	if (!fmt || lstrlen(fmt) < 1 || lstrlen(fmt) > 4000)
		return 0;

	va_start(marker, fmt);
	vsprintf(szBuf, fmt, marker);
	va_end(marker);

	pd.lchContact = hContact;	

	if (hIcon)
		pd.lchIcon = hIcon ;	
	else
		pd.lchIcon = LoadIcon(g_hInst,MAKEINTRESOURCE(IDI_CHANMGR));

	if(!bBroadcast)
		_snprintf(pd.lpzContactName, MAX_CONTACTNAME, "%s - %s", pszProtoName, (char*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME,(WPARAM)hContact,0));
	else
		_snprintf(pd.lpzContactName, MAX_CONTACTNAME, "%s", pszProtoName);

	lstrcpyn(pd.lpzText, Translate(szBuf), MAX_SECONDLINE);

	pd.iSeconds = g_LogOptions.iPopupTimeout;

	if (g_LogOptions.iPopupStyle == 2)
	{
		pd.colorBack = 0;									
		pd.colorText = 0;
	}
	else if (g_LogOptions.iPopupStyle == 3)
	{
		pd.colorBack = g_LogOptions.crPUBkgColour;									
		pd.colorText = g_LogOptions.crPUTextColour;
	}
	else 
	{
		pd.colorBack = g_LogOptions.crLogBackground;									
		pd.colorText = crBkg;

//		pd.colorBack = crBkg;									
//		pd.colorText = g_LogOptions.crPUTextColour;
	}

	pd.PluginWindowProc = PopupDlgProc;

	pd.PluginData = hWnd;

	return PUAddPopUpEx (&pd);

}



// remember to put all in here to Translate( ) template
BOOL DoPopupAndTrayIcon(HWND hWnd, int iEvent, CHATWINDOWDATA * dat, NEWEVENTLPARAM * nlu)
{
	static time_t time = 0;
	static int iOldEvent = 0;

	if(nlu->bIsMe || dat->iType == GCW_SERVER )
		return FALSE;

	if (iEvent&g_LogOptions.dwTrayIconFlags)
	{
		if(!g_LogOptions.TrayIconInactiveOnly || (GetActiveWindow() != hWnd || GetForegroundWindow() != hWnd))
		{
			switch (iEvent)
			{
			case GC_EVENT_MESSAGE|GC_EVENT_HIGHLIGHT :
			case GC_EVENT_ACTION|GC_EVENT_HIGHLIGHT :
				CList_AddEvent(hWnd, dat->hContact, LoadSkinnedIcon(SKINICON_EVENT_MESSAGE), "chaticon", 0, "%s wants your attention in %s", nlu->pszName, dat->pszName); 
				break;
			case GC_EVENT_MESSAGE :
				CList_AddEvent(hWnd, dat->hContact, hIcons[17], "chaticon", CLEF_ONLYAFEW, "%s speaks in %s", nlu->pszName, dat->pszName); 
				break;
			case GC_EVENT_ACTION:
				CList_AddEvent(hWnd, dat->hContact, hIcons[23], "chaticon", CLEF_ONLYAFEW, "%s speaks in %s", nlu->pszName, dat->pszName); 
				break;
			case GC_EVENT_JOIN:
				CList_AddEvent(hWnd, dat->hContact, hIcons[11], "chaticon", CLEF_ONLYAFEW, "%s has joined %s", nlu->pszName, dat->pszName); 
				break;
			case GC_EVENT_PART:
				CList_AddEvent(hWnd, dat->hContact, hIcons[12], "chaticon", CLEF_ONLYAFEW, "%s has left %s", nlu->pszName, dat->pszName); 
				break;
			case GC_EVENT_QUIT:
				CList_AddEvent(hWnd, dat->hContact, hIcons[13], "chaticon", CLEF_ONLYAFEW, "%s disconnected", nlu->pszName); 
				break;
			case GC_EVENT_NICK:
				CList_AddEvent(hWnd, dat->hContact, hIcons[15], "chaticon", CLEF_ONLYAFEW, "%s is now known as %s", nlu->pszName, nlu->pszText); 
				break;
			case GC_EVENT_KICK:
				CList_AddEvent(hWnd, dat->hContact, hIcons[14], "chaticon", CLEF_ONLYAFEW, "%s kicked %s from %s", nlu->pszStatus, nlu->pszName, dat->pszName); 
				break;
			case GC_EVENT_NOTICE:
				CList_AddEvent(hWnd, dat->hContact, hIcons[16], "chaticon", CLEF_ONLYAFEW, "Notice from %s", dat->pszName); 
				break;
			case GC_EVENT_TOPIC:
				CList_AddEvent(hWnd, dat->hContact, hIcons[19], "chaticon", CLEF_ONLYAFEW, "Topic change in %s", dat->pszName); 
				break;
			case GC_EVENT_INFORMATION:
				CList_AddEvent(hWnd, dat->hContact, hIcons[20], "chaticon", CLEF_ONLYAFEW, "Informative message in %s", dat->pszName); 
				break;
			case GC_EVENT_ADDSTATUS:
				CList_AddEvent(hWnd, dat->hContact, hIcons[21], "chaticon", CLEF_ONLYAFEW, "%s enables \'%s\' status for %s in %s", nlu->pszText, nlu->pszStatus, nlu->pszName, dat->pszName); 
				break;
			case GC_EVENT_REMOVESTATUS:
				CList_AddEvent(hWnd, dat->hContact, hIcons[22], "chaticon", CLEF_ONLYAFEW, "%s disables \'%s\' status for %s in %s", nlu->pszText, nlu->pszStatus, nlu->pszName, dat->pszName); 
				break;

			default:break;
			}
		}
	}
	if (PopUpInstalled && iEvent&g_LogOptions.dwPopupFlags)
	{
		if(!nlu->bBroadcasted || nlu->time != time || iEvent != iOldEvent)
		{

			if(!g_LogOptions.PopUpInactiveOnly || (GetActiveWindow() != hWnd || GetForegroundWindow() != hWnd))
			{
				switch (iEvent)
				{			
				case GC_EVENT_MESSAGE|GC_EVENT_HIGHLIGHT :
					ShowPopup(dat->hContact, hWnd, LoadSkinnedIcon(SKINICON_EVENT_MESSAGE), dat->pszModule, dat->pszName, aFonts[16].color, nlu->bBroadcasted, "%s says: %s", nlu->pszName, RemoveFormatting(nlu->pszText)); 
					break;
				case GC_EVENT_ACTION|GC_EVENT_HIGHLIGHT :
					ShowPopup(dat->hContact, hWnd, LoadSkinnedIcon(SKINICON_EVENT_MESSAGE), dat->pszModule, dat->pszName, aFonts[16].color, nlu->bBroadcasted, "%s %s", nlu->pszName, RemoveFormatting(nlu->pszText)); 
					break;
				case GC_EVENT_MESSAGE :
					ShowPopup(dat->hContact, hWnd, hIcons[17], dat->pszModule, dat->pszName, aFonts[9].color, nlu->bBroadcasted, "%s says: %s", nlu->pszName, RemoveFormatting(nlu->pszText)); 
					break;
				case GC_EVENT_ACTION:
					ShowPopup(dat->hContact, hWnd, hIcons[23], dat->pszModule, dat->pszName, aFonts[15].color, nlu->bBroadcasted, "%s %s", nlu->pszName, RemoveFormatting(nlu->pszText)); 
					break;
				case GC_EVENT_JOIN:
					ShowPopup(dat->hContact, hWnd, hIcons[11], dat->pszModule, dat->pszName, aFonts[3].color, nlu->bBroadcasted, "%s has joined", nlu->pszName); 
					break;
				case GC_EVENT_PART:
					if(!nlu->pszText)
						ShowPopup(dat->hContact, hWnd, hIcons[12], dat->pszModule, dat->pszName, aFonts[4].color, nlu->bBroadcasted, "%s has left", nlu->pszName); 
					else					
						ShowPopup(dat->hContact, hWnd, hIcons[12], dat->pszModule, dat->pszName, aFonts[4].color, nlu->bBroadcasted, "%s has left (%s)", nlu->pszName, RemoveFormatting(nlu->pszText)); 
						break;
				case GC_EVENT_QUIT:
					if(!nlu->pszText)
						ShowPopup(dat->hContact, hWnd, hIcons[13], dat->pszModule, dat->pszName, aFonts[5].color, nlu->bBroadcasted, "%s disconnected", nlu->pszName); 
					else
						ShowPopup(dat->hContact, hWnd, hIcons[13], dat->pszModule, dat->pszName, aFonts[5].color, nlu->bBroadcasted, "%s disconnected (%s)", nlu->pszName,RemoveFormatting(nlu->pszText)); 
						break;
				case GC_EVENT_NICK:
					ShowPopup(dat->hContact, hWnd, hIcons[15], dat->pszModule, dat->pszName, aFonts[7].color, nlu->bBroadcasted, "%s is now known as %s", nlu->pszName, nlu->pszText); 
					break;
				case GC_EVENT_KICK:
					if(!nlu->pszText)
						ShowPopup(dat->hContact, hWnd, hIcons[14], dat->pszModule, dat->pszName, aFonts[6].color, nlu->bBroadcasted, "%s kicked %s", nlu->pszStatus, nlu->pszName); 
					else
						ShowPopup(dat->hContact, hWnd, hIcons[14], dat->pszModule, dat->pszName, aFonts[6].color, nlu->bBroadcasted, "%s kicked %s (%s)", nlu->pszStatus, nlu->pszName, RemoveFormatting(nlu->pszText)); 
					break;
				case GC_EVENT_NOTICE:
					ShowPopup(dat->hContact, hWnd, hIcons[16], dat->pszModule, dat->pszName, aFonts[8].color, nlu->bBroadcasted, "Notice from %s: %s", nlu->pszName, RemoveFormatting(nlu->pszText)); 
					break;
				case GC_EVENT_TOPIC:
					if(!nlu->pszName)
						ShowPopup(dat->hContact, hWnd, hIcons[19], dat->pszModule, dat->pszName, aFonts[11].color, nlu->bBroadcasted, "Topic is \'%s\'", RemoveFormatting(nlu->pszText)); 
					else
						ShowPopup(dat->hContact, hWnd, hIcons[19], dat->pszModule, dat->pszName, aFonts[11].color, nlu->bBroadcasted, "Topic is \'%s\' (set by %s)", RemoveFormatting(nlu->pszText), nlu->pszName); 
					break;
				case GC_EVENT_INFORMATION:
					ShowPopup(dat->hContact, hWnd, hIcons[20], dat->pszModule, dat->pszName, aFonts[12].color, nlu->bBroadcasted, "%s", RemoveFormatting(nlu->pszText)); 
					break;
				case GC_EVENT_ADDSTATUS:
					ShowPopup(dat->hContact, hWnd, hIcons[21], dat->pszModule, dat->pszName, aFonts[13].color, nlu->bBroadcasted, "%s enables \'%s\' status for %s", nlu->pszText, nlu->pszStatus, nlu->pszName); 
					break;
				case GC_EVENT_REMOVESTATUS:
					ShowPopup(dat->hContact, hWnd, hIcons[22], dat->pszModule, dat->pszName, aFonts[14].color, nlu->bBroadcasted, "%s disables \'%s\' status for %s", nlu->pszText, nlu->pszStatus, nlu->pszName); 
					break;

				default:break;
				}
			}
		}
		time = nlu->time;
		iOldEvent = iEvent;

	}

	return FALSE;
}

int GetColorIndex(char * pszModule, COLORREF cr)
{
	MODULE * pMod = MM_FindModule(pszModule);
	int i = 0;

	if(!pMod || pMod->nColorCount == 0)
		return -1;

	for (i = 0; i < pMod->nColorCount; i++)
	{
		if (pMod->crColors[i] == cr)
			return i;
	}

	return -1;


}

// obscure function that is used to make sure that ny of hte colors
// passed by the protocol is used as fore- or background color
// in the messagebox. THis is to evercome limitations in the richedit
// that I do not know currently how to overcome
void CheckColorsInModule(char * pszModule)
{
	MODULE * pMod = MM_FindModule(pszModule);
	int i = 0;
	COLORREF crFG;
	COLORREF crBG = (COLORREF)DBGetContactSettingDword(NULL, "Chat", "ColorMessageBG", GetSysColor(COLOR_WINDOW));

	LoadMsgDlgFont(17, NULL, &crFG);

	if(!pMod)
		return;

	for (i = 0; i < pMod->nColorCount; i++)
	{
		if (pMod->crColors[i] == crFG || pMod->crColors[i] == crBG)
		{
			if(pMod->crColors[i] == RGB(255,255,255))
				pMod->crColors[i]--;
			else
				pMod->crColors[i]++;

		}
	}

	return;
}

char* my_strstri(char *s1, char *s2) 
{ 
	int i,j,k; 
	for(i=0;s1[i];i++) 
		for(j=i,k=0;tolower(s1[j])==tolower(s2[k]);j++,k++) 
			if(!s2[k+1]) 
				return (s1+i); 
	return NULL; 
} 