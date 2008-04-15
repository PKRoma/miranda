/*
  Name: NewEventNotify - Plugin for Miranda ICQ
  File: neweventnotify.h - Main Header File
  Version: 0.0.4
  Description: Notifies you when you receive a message
  Author: icebreaker, <icebreaker@newmail.net>
  Date: 18.07.02 13:59 / Update: 16.09.02 17:45
  Copyright: (C) 2002 Starzinger Michael

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

$Id$

Event popups for tabSRMM - most of the code taken from NewEventNotify (see copyright above)

  Code modified and adapted for tabSRMM by Nightwish (silvercircle@gmail.com)
  Additional code (popup merging, options) by Prezes
  
*/

#ifndef _NEN_H_
#define _NEN_H_

//#include "m_popup.h"
//#include "m_popupw.h"

#define MODULE "tabSRMM_NEN"

int tabSRMM_ShowPopup(WPARAM wParam, LPARAM lParam, WORD eventType, int windowOpen, struct ContainerWindowData *pContainer, HWND hwndChild, char *szProto, struct MessageWindowData *dat);

#define DEFAULT_COLBACK RGB(255,255,128)
#define DEFAULT_COLTEXT RGB(0,0,0)
#define DEFAULT_MASKNOTIFY (MASK_MESSAGE|MASK_URL|MASK_FILE|MASK_OTHER)
#define DEFAULT_MASKACTL (MASK_OPEN|MASK_DISMISS)
#define DEFAULT_MASKACTR (MASK_DISMISS)
#define DEFAULT_DELAY -1

#define MASK_MESSAGE    0x0001
#define MASK_URL        0x0002
#define MASK_FILE       0x0004
#define MASK_OTHER      0x0008

#define MASK_DISMISS    0x0001
#define MASK_OPEN       0x0002
#define MASK_REMOVE     0x0004

#define PU_REMOVE_ON_FOCUS 1
#define PU_REMOVE_ON_TYPE 2
#define PU_REMOVE_ON_SEND 4

#define SETTING_LIFETIME_MIN		1
#define SETTING_LIFETIME_MAX		60
#define SETTING_LIFETIME_DEFAULT	4

//Entrys in the database, don't translate
#define OPT_PREVIEW "Preview"
#define OPT_COLDEFAULT_MESSAGE "DefaultColorMsg"
#define OPT_COLBACK_MESSAGE "ColorBackMsg"
#define OPT_COLTEXT_MESSAGE "ColorTextMsg"
#define OPT_COLDEFAULT_URL "DefaultColorUrl"
#define OPT_COLBACK_URL "ColorBackUrl"
#define OPT_COLTEXT_URL "ColorTextUrl"
#define OPT_COLDEFAULT_FILE "DefaultColorFile"
#define OPT_COLBACK_FILE "ColorBackFile"
#define OPT_COLTEXT_FILE "ColorTextFile"
#define OPT_COLDEFAULT_OTHERS "DefaultColorOthers"
#define OPT_COLBACK_OTHERS "ColorBackOthers"
#define OPT_COLTEXT_OTHERS "ColorTextOthers"
#define OPT_MASKNOTIFY "Notify"
#define OPT_MASKACTL "ActionLeft"
#define OPT_MASKACTR "ActionRight"
#define OPT_MASKACTTE "ActionTimeExpires"
#define OPT_MERGEPOPUP "MergePopup"
#define OPT_DELAY_MESSAGE "DelayMessage"
#define OPT_DELAY_URL "DelayUrl"
#define OPT_DELAY_FILE "DelayFile"
#define OPT_DELAY_OTHERS "DelayOthers"
#define OPT_SHOW_DATE "ShowDate"
#define OPT_SHOW_TIME "ShowTime"
#define OPT_SHOW_HEADERS "ShowHeaders"
#define OPT_NUMBER_MSG "NumberMsg"
#define OPT_SHOW_ON "ShowOldOrNew"
#define OPT_NORSS "NoRSSAnnounces"
#define OPT_DISABLE "Disabled"
#define OPT_WINDOWCHECK "WindowCheck"
#define OPT_LIMITPREVIEW "LimitPreview"
#define OPT_MINIMIZEANIMATED "Animated"
#define OPT_ANNOUNCEMETHOD "method"
#define OPT_FLOATER "floater"
#define OPT_FLOATERINWIN "floater_win"
#define OPT_FLOATERONLYMIN "floater_onlymin"
#define OPT_REMOVEMASK "removemask"
#define OPT_SIMPLEOPT "simplemode"

typedef struct {
    BOOL bPreview;
    BOOL bDefaultColorMsg;
	BOOL bDefaultColorUrl;
	BOOL bDefaultColorFile;
	BOOL bDefaultColorOthers;
    COLORREF colBackMsg;
    COLORREF colTextMsg;
	COLORREF colBackUrl;
    COLORREF colTextUrl;
	COLORREF colBackFile;
    COLORREF colTextFile;
	COLORREF colBackOthers;
    COLORREF colTextOthers;
    UINT maskNotify;
    UINT maskActL;
    UINT maskActR;
	UINT maskActTE;
	int iDelayMsg;
	int iDelayUrl;
	int iDelayFile;
	int iDelayOthers;
	int iDelayDefault;
	BOOL bMergePopup;
	BOOL bShowDate;
	BOOL bShowTime;
	BOOL bShowHeaders;
	BYTE iNumberMsg;
	BOOL bShowON;
	BOOL bNoRSS;
    int  iDisable;
    int  dwStatusMask;
    BOOL bTraySupport;
    BOOL bTrayExist;
    BOOL bMinimizeToTray;
    int  iAutoRestore;
    BOOL iNoSounds;
    BOOL iNoAutoPopup;
    BOOL bWindowCheck;
    int  iLimitPreview;
    BOOL bAnimated;
    WORD wMaxRecent;
    WORD wMaxFavorites;
    int  iAnnounceMethod;
    BOOL floaterMode;
    BOOL bFloaterInWin;
    BOOL bFloaterOnlyMin;
	BYTE bSimpleMode;
    DWORD dwRemoveMask;
} NEN_OPTIONS;

#define FLOATER_ATTACHED 1
#define FLOATER_FREE 2
#define FLOATER_ALWAYS 4

typedef struct EVENT_DATA {
	HANDLE hEvent;
    char szText[MAX_SECONDLINE + 2];
    DWORD timestamp;
} EVENT_DATA;

typedef struct {
    UINT eventType;
    HANDLE hContact;
    NEN_OPTIONS *pluginOptions;
	POPUPDATAEX* pud;
	HWND hWnd;
	long iSeconds;
    char szHeader[256];
    int  nrMerged;
    EVENT_DATA *eventData;
    int  nrEventsAlloced;
    int  iActionTaken;
} PLUGIN_DATA;

typedef struct EVENT_DATAW {
	HANDLE hEvent;
    wchar_t szText[MAX_SECONDLINE + 2];
    DWORD timestamp;
} EVENT_DATAW;

typedef struct {
    UINT eventType;
    HANDLE hContact;
    NEN_OPTIONS *pluginOptions;
	POPUPDATAW* pud;
	HWND hWnd;
	long iSeconds;
    wchar_t szHeader[256];
    int  nrMerged;
    EVENT_DATAW *eventData;
    int  nrEventsAlloced;
    int  iActionTaken;
} PLUGIN_DATAW;

#define NR_MERGED 5

#define TIMER_TO_ACTION 50685

#define POPUP_COMMENT_MESSAGE "Message"
#define POPUP_COMMENT_URL "URL"
#define POPUP_COMMENT_FILE "File"
#define POPUP_COMMENT_CONTACTS "Contacts"
#define POPUP_COMMENT_ADDED "You were added!"
#define POPUP_COMMENT_AUTH "Requests your authorisation"
#define POPUP_COMMENT_WEBPAGER "ICQ Web pager"
#define POPUP_COMMENT_EMAILEXP "ICQ Email express"
#define POPUP_COMMENT_OTHER "Unknown Event"

#define MAX_DATASIZE	24
#define MAX_POPUPS 20

#endif
