/*
 * astyle --force-indent=tab=4 --brackets=linux --indent-switches
 *		  --pad=oper --one-line=keep-blocks  --unpad=paren
 *
 * Miranda IM: the free IM client for Microsoft* Windows*
 *
 * Copyright 2000-2009 Miranda ICQ/IM project,
 * all portions of this codebase are copyrighted to the people
 * listed in contributors.txt.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * you should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * part of tabSRMM messaging plugin for Miranda.
 *
 * (C) 2005-2009 by silvercircle _at_ gmail _dot_ com and contributors
 *
 * $Id$
 *
 * Plugin configuration variables and functions. Implemented as a class
 * though there will always be only a single instance.
 *
 */

#include "commonheaders.h"
extern PLUGININFOEX pluginInfo;

CGlobals 	PluginConfig;
CGlobals*	pConfig = &PluginConfig;

int 		Chat_PreShutdown();
int 		Chat_ModulesLoaded(WPARAM wp, LPARAM lp);
int			AvatarChanged(WPARAM wParam, LPARAM lParam);
int			MyAvatarChanged(WPARAM wParam, LPARAM lParam);
int 		IcoLibIconsChanged(WPARAM wParam, LPARAM lParam);
int 		FontServiceFontsChanged(WPARAM wParam, LPARAM lParam);
int 		SmileyAddOptionsChanged(WPARAM wParam, LPARAM lParam);
int 		IEViewOptionsChanged(WPARAM wParam, LPARAM lParam);
void 		RegisterFontServiceFonts();
int 		ModPlus_PreShutdown(WPARAM wparam, LPARAM lparam);
int 		ModPlus_Init(WPARAM wparam, LPARAM lparam);
INT_PTR 	CALLBACK HotkeyHandlerDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

HANDLE		CGlobals::m_event_ModulesLoaded = 0, CGlobals::m_event_PrebuildMenu = 0, CGlobals::m_event_SettingChanged = 0;
HANDLE		CGlobals::m_event_ContactDeleted = 0, CGlobals::m_event_Dispatch = 0, CGlobals::m_event_EventAdded = 0;
HANDLE		CGlobals::m_event_IconsChanged = 0, CGlobals::m_event_TypingEvent = 0, CGlobals::m_event_ProtoAck = 0;
HANDLE		CGlobals::m_event_PreShutdown = 0, CGlobals::m_event_OkToExit = 0;
HANDLE		CGlobals::m_event_IcoLibChanged = 0, CGlobals::m_event_AvatarChanged = 0, CGlobals::m_event_MyAvatarChanged = 0, CGlobals::m_event_FontsChanged = 0;
HANDLE		CGlobals::m_event_SmileyAdd = 0, CGlobals::m_event_IEView = 0, CGlobals::m_event_FoldersChanged = 0;
HANDLE 		CGlobals::m_event_ME_MC_SUBCONTACTSCHANGED = 0, CGlobals::m_event_ME_MC_FORCESEND = 0, CGlobals::m_event_ME_MC_UNFORCESEND = 0;

TCCache*	CGlobals::m_cCache = 0;
size_t		CGlobals::m_cCacheSize = 0, CGlobals::m_cCacheSizeAlloced = 0;

extern 		HANDLE 	hHookButtonPressedEvt;
extern		HANDLE 	hHookToolBarLoadedEvt;

#if defined(_UNICODE)
#if defined(_WIN64)
	static char szCurrentVersion[30];
	static char *szVersionUrl = "http://download.miranda.or.at/tabsrmm/3/version.txt";
	static char *szUpdateUrl = "http://download.miranda.or.at/tabsrmm/3/tabsrmm_x64.zip";
	static char *szFLVersionUrl = "http://addons.miranda-im.org/details.php?action=viewfile&id=3699";
	static char *szFLUpdateurl = "http://addons.miranda-im.org/feed.php?dlfile=3699";
#else
	static char szCurrentVersion[30];
	static char *szVersionUrl = "http://download.miranda.or.at/tabsrmm/3/version.txt";
	static char *szUpdateUrl = "http://download.miranda.or.at/tabsrmm/3/tabsrmmW.zip";
	static char *szFLVersionUrl = "http://addons.miranda-im.org/details.php?action=viewfile&id=3699";
	static char *szFLUpdateurl = "http://addons.miranda-im.org/feed.php?dlfile=3699";
#endif
#else
	static char szCurrentVersion[30];
	static char *szVersionUrl = "http://download.miranda.or.at/tabsrmm/3/version.txt";
	static char *szUpdateUrl = "http://download.miranda.or.at/tabsrmm/3/tabsrmm.zip";
	static char *szFLVersionUrl = "http://addons.miranda-im.org/details.php?action=viewfile&id=3698";
	static char *szFLUpdateurl = "http://addons.miranda-im.org/feed.php?dlfile=3698";
#endif
	static char *szPrefix = "tabsrmm ";


void CGlobals::RegisterWithUpdater()
{
	Update 		upd = {0};

 	if (!ServiceExists(MS_UPDATE_REGISTER))
 		return;

 	upd.cbSize = sizeof(upd);
 	upd.szComponentName = pluginInfo.shortName;
 	upd.pbVersion = (BYTE *)/*CreateVersionStringPlugin*/CreateVersionString(pluginInfo.version, szCurrentVersion);
 	upd.cpbVersion = (int)(strlen((char *)upd.pbVersion));
	upd.szVersionURL = szFLVersionUrl;
	upd.szUpdateURL = szFLUpdateurl;
#if defined(_UNICODE)
	upd.pbVersionPrefix = (BYTE *)"<span class=\"fileNameHeader\">tabSRMM Unicode 2.0 ";
#else
	upd.pbVersionPrefix = (BYTE *)"<span class=\"fileNameHeader\">tabSRMM 2.0 ";
#endif
	upd.cpbVersionPrefix = (int)(strlen((char *)upd.pbVersionPrefix));

 	upd.szBetaUpdateURL = szUpdateUrl;
 	upd.szBetaVersionURL = szVersionUrl;
	upd.pbVersion = (unsigned char *)szCurrentVersion;
	upd.cpbVersion = lstrlenA(szCurrentVersion);
 	upd.pbBetaVersionPrefix = (BYTE *)szPrefix;
 	upd.cpbBetaVersionPrefix = (int)(strlen((char *)upd.pbBetaVersionPrefix));
 	upd.szBetaChangelogURL   ="http://miranda.radicaled.ru/public/tabsrmm/chglogeng.txt";

 	CallService(MS_UPDATE_REGISTER, 0, (LPARAM)&upd);
}
/**
 * reload system values. These are read ONCE and are not allowed to change
 * without a restart
 */
void CGlobals::reloadSystemStartup()
{
	HDC			hScrnDC;
	DBVARIANT 	dbv = {0};

	m_WinVerMajor = 					WinVerMajor();
	m_WinVerMinor = 					WinVerMinor();
	m_bIsXP = 							IsWinVerXPPlus();
	m_bIsVista = 						IsWinVerVistaPlus();
	m_bIsWin7 = 						IsWinVer7Plus();

	::LoadTSButtonModule();
	::RegisterTabCtrlClass();

	dwThreadID = 						GetCurrentThreadId();

	PluginConfig.g_hMenuContext = 		LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_TABCONTEXT));
	CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM)g_hMenuContext, 0);

	SkinAddNewSoundEx("RecvMsgActive", Translate("Messages"), Translate("Incoming (Focused Window)"));
	SkinAddNewSoundEx("RecvMsgInactive", Translate("Messages"), Translate("Incoming (Unfocused Window)"));
	SkinAddNewSoundEx("AlertMsg", Translate("Messages"), Translate("Incoming (New Session)"));
	SkinAddNewSoundEx("SendMsg", Translate("Messages"), Translate("Outgoing"));
	SkinAddNewSoundEx("SendError", Translate("Messages"), Translate("Error sending message"));

	hCurSplitNS = LoadCursor(NULL, IDC_SIZENS);
	hCurSplitWE = LoadCursor(NULL, IDC_SIZEWE);
	hCurHyperlinkHand = LoadCursor(NULL, IDC_HAND);
	if (hCurHyperlinkHand == NULL)
		hCurHyperlinkHand = LoadCursor(g_hInst, MAKEINTRESOURCE(IDC_HYPERLINKHAND));

	hScrnDC = GetDC(0);
	g_DPIscaleX = 						GetDeviceCaps(hScrnDC, LOGPIXELSX) / 96.0;
	g_DPIscaleY = 						GetDeviceCaps(hScrnDC, LOGPIXELSY) / 96.0;
	ReleaseDC(0, hScrnDC);

#if defined(_UNICODE)
	const	char *tszKeyName = "titleformatW";
#else
	const	char *tszKeyName = "titleformatW";
#endif

	if (M->GetTString(NULL, SRMSGMOD_T, tszKeyName, &dbv)) {
		M->WriteTString(NULL, SRMSGMOD_T, tszKeyName, _T("%n - %s"));
		_tcsncpy(szDefaultTitleFormat, _T("%n - %s"), safe_sizeof(szDefaultTitleFormat));
	} else {
		_tcsncpy(szDefaultTitleFormat, dbv.ptszVal, safe_sizeof(szDefaultTitleFormat));
		::DBFreeVariant(&dbv);
	}
	szDefaultTitleFormat[255] = 0;

	reloadSettings();
	reloadAdv();
	hookSystemEvents();
}

void CGlobals::reloadSystemModulesChanged()
{
	BOOL				bIEView = FALSE;
	CLISTMENUITEM 		mi = { 0 };

	m_MathModAvail = 					ServiceExists(MATH_RTF_REPLACE_FORMULAE);

	/*
	 * smiley add
	 */
	if (ServiceExists(MS_SMILEYADD_REPLACESMILEYS)) {
		PluginConfig.g_SmileyAddAvail = 1;
		m_event_SmileyAdd = HookEvent(ME_SMILEYADD_OPTIONSCHANGED, ::SmileyAddOptionsChanged);
	}
	else
		m_event_SmileyAdd = 0;

	/*
	 * Flashavatars
	 */

	g_FlashAvatarAvail = (ServiceExists(MS_FAVATAR_GETINFO) ? 1 : 0);

	/*
	 * ieView
	 */

	bIEView = ServiceExists(MS_IEVIEW_WINDOW);
	if (bIEView) {
		BOOL bOldIEView = M->GetByte("ieview_installed", 0);
		if (bOldIEView != bIEView)
			M->WriteByte(SRMSGMOD_T, "default_ieview", 1);
		M->WriteByte(SRMSGMOD_T, "ieview_installed", 1);
		m_event_IEView = HookEvent(ME_IEVIEW_OPTIONSCHANGED, ::IEViewOptionsChanged);
	} else {
		M->WriteByte(SRMSGMOD_T, "ieview_installed", 0);
		m_event_IEView = 0;
	}

	g_iButtonsBarGap = 							M->GetByte("ButtonsBarGap", 1);
	m_hwndClist = 								(HWND)CallService(MS_CLUI_GETHWND, 0, 0);
	m_MathModAvail = 							(ServiceExists(MATH_RTF_REPLACE_FORMULAE) ? 1 : 0);
	if (m_MathModAvail) {
		char *szDelim = (char *)CallService(MATH_GET_STARTDELIMITER, 0, 0);
		if (szDelim) {
#if defined(_UNICODE)
			MultiByteToWideChar(CP_ACP, 0, szDelim, -1, PluginConfig.m_MathModStartDelimiter, safe_sizeof(PluginConfig.m_MathModStartDelimiter));
#else
			strncpy(PluginConfig.m_MathModStartDelimiter, szDelim, sizeof(PluginConfig.m_MathModStartDelimiter));
#endif
			CallService(MTH_FREE_MATH_BUFFER, 0, (LPARAM)szDelim);
		}
	}
	else
		PluginConfig.m_MathModStartDelimiter[0] = 0;

	g_MetaContactsAvail = (ServiceExists(MS_MC_GETDEFAULTCONTACT) ? 1 : 0);


	if(g_MetaContactsAvail) {
		mir_snprintf(szMetaName, 256, "%s", (char *)CallService(MS_MC_GETPROTOCOLNAME, 0, 0));
		bMetaEnabled = M->GetByte(0, szMetaName, "Enabled", 0);
	}
	else {
		szMetaName[0] = 0;
		bMetaEnabled = 0;
	}

	g_PopupAvail = (ServiceExists(MS_POPUP_ADDPOPUPEX) ? 1 : 0);

#if defined(_UNICODE)
	g_PopupWAvail = (ServiceExists(MS_POPUP_ADDPOPUPW) ? 1 : 0);
#endif

	mi.cbSize = sizeof(mi);
	mi.position = -2000090000;
	if (ServiceExists(MS_SKIN2_GETICONBYHANDLE)) {
		mi.flags = CMIF_ICONFROMICOLIB | CMIF_DEFAULT;
		mi.icolibItem = LoadSkinnedIconHandle(SKINICON_EVENT_MESSAGE);
	} else {
		mi.flags = CMIF_DEFAULT;
		mi.hIcon = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
	}
	mi.pszName = LPGEN("&Message");
	mi.pszService = MS_MSG_SENDMESSAGE;
	PluginConfig.m_hMenuItem = (HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM) & mi);
}

/**
 * reload plugin settings on startup and runtime. Most of these setttings can be
 * changed while plugin is running.
 */
void CGlobals::reloadSettings()
{
	m_ncm.cbSize = sizeof(NONCLIENTMETRICS);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &m_ncm, 0);

	m_GlobalContainerFlags = 			M->GetDword("containerflags", CNT_FLAGS_DEFAULT);
	m_GlobalContainerFlagsEx = 			M->GetDword("containerflagsEx", CNT_FLAGSEX_DEFAULT);

	if (!(m_GlobalContainerFlags & CNT_NEWCONTAINERFLAGS))
		m_GlobalContainerFlags = CNT_FLAGS_DEFAULT;
	m_GlobalContainerTrans = 			M->GetDword("containertrans", CNT_TRANS_DEFAULT);

	DWORD dwFlags = 					M->GetDword("mwflags", MWF_LOG_DEFAULT);

	m_SendOnShiftEnter = 				(int)M->GetByte("sendonshiftenter", 0);
	m_SendOnEnter = 					(int)M->GetByte(SRMSGSET_SENDONENTER, SRMSGDEFSET_SENDONENTER);
	m_SendOnDblEnter = 					(int)M->GetByte("SendOnDblEnter", 0);
	m_AutoLocaleSupport = 				(int)M->GetByte("al", 1);
	m_AutoSwitchTabs = 					(int)M->GetByte("autoswitchtabs", 1);
	m_CutContactNameTo = 				(int) DBGetContactSettingWord(NULL, SRMSGMOD_T, "cut_at", 15);
	m_CutContactNameOnTabs = 			(int)M->GetByte("cuttitle", 0);
	m_StatusOnTabs = 					(int)M->GetByte("tabstatus", 1);
	m_LogStatusChanges = 				(int)dwFlags & MWF_LOG_STATUSCHANGES;
	m_UseDividers = 					(int)M->GetByte("usedividers", 0);
	m_DividersUsePopupConfig = 			(int)M->GetByte("div_popupconfig", 0);
	m_MsgTimeout = 						(int)M->GetDword(SRMSGMOD, SRMSGSET_MSGTIMEOUT, SRMSGDEFSET_MSGTIMEOUT);

	if (m_MsgTimeout < SRMSGSET_MSGTIMEOUT_MIN)
		m_MsgTimeout = SRMSGSET_MSGTIMEOUT_MIN;

	m_EscapeCloses = 					(int)M->GetByte("escmode", 0);

	m_HideOnClose =						(int) M->GetByte("hideonclose", 0);
	m_AllowTab =						(int) M->GetByte("tabmode", 0);

	m_WarnOnClose = 					(int)M->GetByte("warnonexit", 0);
	m_AvatarMode = 						(int)M->GetByte("avatarmode", 0);

	if (m_AvatarMode == 1 || m_AvatarMode == 2)
		m_AvatarMode = 3;

	m_OwnAvatarMode = 					(int)M->GetByte("ownavatarmode", 0);
	m_FlashOnClist = 					(int)M->GetByte("flashcl", 0);
	m_TabAutoClose = 					(int)M->GetDword("tabautoclose", 0);
	m_AlwaysFullToolbarWidth = 			(int)M->GetByte("alwaysfulltoolbar", 1);
	m_LimitStaticAvatarHeight = 		(int)M->GetDword("avatarheight", 96);
	m_SendFormat = 						(int)M->GetByte("sendformat", 0);
	m_FormatWholeWordsOnly = 1;
	m_FixFutureTimestamps = 			(int)M->GetByte("do_fft", 1);
	m_RTLDefault = 						(int)M->GetByte("rtldefault", 0);
	m_TabAppearance = 					(int)M->GetDword("tabconfig", TCF_FLASHICON | TCF_SINGLEROWTABCONTROL);
	m_panelHeight = 					(DWORD)M->GetDword("panelheight", CInfoPanel::DEGRADE_THRESHOLD);
	m_MUCpanelHeight = 					M->GetDword("Chat", "panelheight", CInfoPanel::DEGRADE_THRESHOLD);
	m_IdleDetect = 						(int)M->GetByte("detectidle", 1);
	m_smcxicon = 16;
	m_smcyicon = 16;
	m_PasteAndSend = 					(int)M->GetByte("pasteandsend", 1);
	m_szNoStatus = 						const_cast<TCHAR *>(CTranslator::get(CTranslator::GEN_NO_STATUS));
	m_LangPackCP = 						ServiceExists(MS_LANGPACK_GETCODEPAGE) ? CallService(MS_LANGPACK_GETCODEPAGE, 0, 0) : CP_ACP;
	m_SmileyButtonOverride = 			(BYTE)M->GetByte("smbutton_override", 1);
	m_visualMessageSizeIndicator = 		M->GetByte("msgsizebar", 0);
	m_autoSplit = 						M->GetByte("autosplit", 0);
	m_FlashOnMTN = 						M->GetByte(SRMSGMOD, SRMSGSET_SHOWTYPINGWINFLASH, SRMSGDEFSET_SHOWTYPINGWINFLASH);
	if(m_MenuBar == 0) {
		m_MenuBar = ::LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_MENUBAR));
		CallService(MS_LANGPACK_TRANSLATEMENU, WPARAM(m_MenuBar), 0);
	}

	m_SendLaterAvail = 					M->GetByte("sendLaterAvail", 0);
	m_ipBackgroundGradient = 			M->GetDword(FONTMODULE, "ipfieldsbg", 0x62caff);
	m_ipBackgroundGradientHigh = 		M->GetDword(FONTMODULE, "ipfieldsbgHigh", 0xf0f0f0);
	m_tbBackgroundHigh = 				M->GetDword(FONTMODULE, "tbBgHigh", 0);
	m_tbBackgroundLow = 				M->GetDword(FONTMODULE, "tbBgLow", 0);
}

/**
 * reload "advanced tweaks" that can be applied w/o a restart
 */
void CGlobals::reloadAdv()
{
	g_bDisableAniAvatars=				M->GetByte("adv_DisableAniAvatars", 0);
	g_bSoundOnTyping = 					M->GetByte("adv_soundontyping", 0);

	if(g_bSoundOnTyping && m_TypingSoundAdded == false) {
		SkinAddNewSoundEx("SoundOnTyping", Translate("Other"), Translate("TABSRMM: Typing"));
		m_TypingSoundAdded = true;
	}
	m_AllowOfflineMultisend =			M->GetByte("AllowOfflineMultisend", 0);
}

const HMENU CGlobals::getMenuBar()
{
	if(m_MenuBar == 0) {
		m_MenuBar = ::LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_MENUBAR));
		CallService(MS_LANGPACK_TRANSLATEMENU, WPARAM(m_MenuBar), 0);
	}
	return(m_MenuBar);
}

/**
 * hook core events. This runs in LoadModule()
 * only core events and services are guaranteed to exist at this time
 */
void CGlobals::hookSystemEvents()
{
	m_event_ModulesLoaded 	= 	HookEvent(ME_SYSTEM_MODULESLOADED, ModulesLoaded);
	m_event_IconsChanged 	= 	HookEvent(ME_SKIN_ICONSCHANGED, ::IconsChanged);
	m_event_TypingEvent 	= 	HookEvent(ME_PROTO_CONTACTISTYPING, CMimAPI::TypingMessage);
	m_event_ProtoAck 		= 	HookEvent(ME_PROTO_ACK, CMimAPI::ProtoAck);
	m_event_PreShutdown 	= 	HookEvent(ME_SYSTEM_PRESHUTDOWN, PreshutdownSendRecv);
	m_event_OkToExit 		= 	HookEvent(ME_SYSTEM_OKTOEXIT, OkToExit);

	m_event_PrebuildMenu 	= 	HookEvent(ME_CLIST_PREBUILDCONTACTMENU, CMimAPI::PrebuildContactMenu);

	m_event_IcoLibChanged 	= 	HookEvent(ME_SKIN2_ICONSCHANGED, ::IcoLibIconsChanged);
	m_event_AvatarChanged 	= 	HookEvent(ME_AV_AVATARCHANGED, ::AvatarChanged);
	m_event_MyAvatarChanged = 	HookEvent(ME_AV_MYAVATARCHANGED, ::MyAvatarChanged);
	m_event_FontsChanged 	= 	HookEvent(ME_FONT_RELOAD, ::FontServiceFontsChanged);
}

/**
 * hook events provided by plugins. This must be run in the ModulesLoaded handler
 */
void CGlobals::hookPluginEvents()
{

}

/**
 * second part of the startup initialisation. All plugins are now fully loaded
 */

int CGlobals::ModulesLoaded(WPARAM wParam, LPARAM lParam)
{
	int 				i;
	MENUITEMINFOA 		mii = {0};
	HMENU 				submenu;
	CLISTMENUITEM 		mi = { 0 };

	::UnhookEvent(m_event_ModulesLoaded);

	M->configureCustomFolders();

	for (i = 0; i < NR_BUTTONBARICONS; i++)
		PluginConfig.g_buttonBarIcons[i] = 0;
	::LoadIconTheme();
	::CreateImageList(TRUE);

	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_BITMAP;
	mii.hbmpItem = HBMMENU_CALLBACK;
	submenu = GetSubMenu(PluginConfig.g_hMenuContext, 7);
	for (i = 0; i <= 8; i++)
		SetMenuItemInfoA(submenu, (UINT_PTR)i, TRUE, &mii);

	PluginConfig.reloadSystemModulesChanged();

	::BuildContainerMenu();

	::CB_InitDefaultButtons();
	::ModPlus_Init(wParam, lParam);
	::NotifyEventHooks(hHookToolBarLoadedEvt, (WPARAM)0, (LPARAM)0);
	//

	if (M->GetByte("avatarmode", -1) == -1)
		M->WriteByte(SRMSGMOD_T, "avatarmode", 2);

	PluginConfig.g_hwndHotkeyHandler = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_HOTKEYSLAVE), 0, HotkeyHandlerDlgProc);

	::CreateTrayMenus(TRUE);
	if (nen_options.bTraySupport)
		::CreateSystrayIcon(TRUE);

	mi.cbSize = sizeof(mi);
	mi.position = -500050005;
	mi.hIcon = PluginConfig.g_iconContainer;
	mi.pszContactOwner = NULL;
	mi.pszName = LPGEN("&Messaging settings...");
	mi.pszService = MS_TABMSG_SETUSERPREFS;
	PluginConfig.m_UserMenuItem = (HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM) & mi);

	::PreTranslateDates();
	PluginConfig.m_hFontWebdings = CreateFontA(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SYMBOL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE | DEFAULT_PITCH, "Wingdings");

	RestoreUnreadMessageAlerts();

	RegisterWithUpdater();

	::RegisterFontServiceFonts();
	::CacheLogFonts();
	::Chat_ModulesLoaded(wParam, lParam);
	if(PluginConfig.g_PopupWAvail||PluginConfig.g_PopupAvail)
		TN_ModuleInit();

	m_event_SettingChanged 	= HookEvent(ME_DB_CONTACT_SETTINGCHANGED, DBSettingChanged);
	m_event_ContactDeleted 	= HookEvent(ME_DB_CONTACT_DELETED, DBContactDeleted);

	m_event_Dispatch 		= HookEvent(ME_DB_EVENT_ADDED, CMimAPI::DispatchNewEvent);
	m_event_EventAdded 		= HookEvent(ME_DB_EVENT_ADDED, CMimAPI::MessageEventAdded);
	if(PluginConfig.g_MetaContactsAvail) {
		m_event_ME_MC_SUBCONTACTSCHANGED = HookEvent(ME_MC_SUBCONTACTSCHANGED, MetaContactEvent);
		m_event_ME_MC_FORCESEND			 = HookEvent(ME_MC_FORCESEND, MetaContactEvent);
		m_event_ME_MC_UNFORCESEND		 = HookEvent(ME_MC_UNFORCESEND, MetaContactEvent);
	}
	return 0;
}

/**
 * watches various important database settings and reacts accordingly
 * needed to catch status, nickname and other changes in order to update open message
 * sessions.
 */

int CGlobals::DBSettingChanged(WPARAM wParam, LPARAM lParam)
{
	DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING *) lParam;
	const char 	*szProto = NULL;
	const char  *setting = cws->szSetting;
	HWND		hwnd = 0;
	CContactCache* c = 0;
	bool		fChanged = false;

	if(wParam)
		hwnd = M->FindWindow((HANDLE)wParam);

	if (hwnd == 0 && wParam != 0) {     // we are not interested in this event if there is no open message window/tab
		if(!strcmp(setting, "Status") || !strcmp(setting, "MyHandle") || !strcmp(setting, "Nick")) {
			c = CGlobals::getContactCache((HANDLE)wParam);
			if(c) {
				fChanged = c->updateStatus();
				c->updateNick();
			}
		}
		return(0);
	}

	if (wParam == 0 && strstr("Nick,yahoo_id", setting)) {
		M->BroadcastMessage(DM_OWNNICKCHANGED, 0, (LPARAM)cws->szModule);
		return(0);
	}

	if(wParam) {
		c = CGlobals::getContactCache((HANDLE)wParam);
		if(c)
			szProto = c->getProto();
	}

	if (lstrcmpA(cws->szModule, "CList") && (szProto == NULL || lstrcmpA(cws->szModule, szProto)))
		return(0);

	if (PluginConfig.g_MetaContactsAvail && !lstrcmpA(cws->szModule, PluginConfig.szMetaName)) {
		if(wParam != 0 && !lstrcmpA(setting, "Nick"))      // filter out this setting to avoid infinite loops while trying to obtain the most online contact
			return(0);
		if(wParam == 0 && !lstrcmpA(setting, "Enabled")) { 		// catch the disabled meta contacts
			PluginConfig.bMetaEnabled = M->GetByte(0, PluginConfig.szMetaName, "Enabled", 0);
			cacheUpdateMetaChanged();
		}
	}

	if (hwnd) {
		if(c) {
			fChanged = c->updateStatus();
			c->updateNick();
		}
		if (strstr("IdleTS,XStatusId,display_uid", setting)) {
			if (!strcmp(setting, "XStatusId"))
				PostMessage(hwnd, DM_UPDATESTATUSMSG, 0, 0);
		}
		else if (lstrlenA(setting) > 6 && lstrlenA(setting) < 9 && !strncmp(setting, "Status", 6)) {
			fChanged = true;
			if(c) {
				c->updateMeta();
				c->updateUIN();
			}
		}
		else if (!strcmp(setting, "MirVer")) {
			PostMessage(hwnd, DM_CLIENTCHANGED, 0, 0);
			if(PluginConfig.g_bClientInStatusBar)
				ChangeClientIconInStatusBar(wParam,0);
		}
		else if (strstr("StatusMsg,StatusDescr,XStatusMsg,XStatusName,YMsg", setting))
			PostMessage(hwnd, DM_UPDATESTATUSMSG, 0, 0);
		if(fChanged)
			PostMessage(hwnd, DM_UPDATETITLE, 0, 1);
	}
	return(0);
}

/**
 * event fired when a contact has been deleted. Make sure to close its message session
 */

int CGlobals::DBContactDeleted(WPARAM wParam, LPARAM lParam)
{
	HWND hwnd;

	if (hwnd = M->FindWindow((HANDLE) wParam)) {
		struct _MessageWindowData *dat = (struct _MessageWindowData *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

		if (dat)
			dat->bWasDeleted = 1;				// indicate a deleted contact. The WM_CLOSE handler will "fast close" the session and skip housekeeping.
		SendMessage(hwnd, WM_CLOSE, 0, 1);
	}
	return 0;
}

/**
 * Handle events from metacontacts protocol. Basically, just update
 * our contact cache and, if a message window exists, tell it to update
 * relevant information.
 */
int CGlobals::MetaContactEvent(WPARAM wParam, LPARAM lParam)
{
	if(wParam) {
		CContactCache *c = CGlobals::getContactCache((HANDLE)wParam);
		if(c)
			c->updateMeta();

		HWND	hwnd = M->FindWindow((HANDLE)wParam);
		if(hwnd && c) {
			c->updateUIN();								// only do this for open windows, not needed normally
			::PostMessage(hwnd, DM_UPDATETITLE, 0, 0);
		}
	}
	return(0);
}

int CGlobals::PreshutdownSendRecv(WPARAM wParam, LPARAM lParam)
{
	HANDLE 	hContact;
	int		i;

	if (PluginConfig.m_chat_enabled)
		::Chat_PreShutdown();

	::TN_ModuleDeInit();

	while(pFirstContainer){
		//MaD: fix for correct closing hidden contacts
		if (PluginConfig.m_HideOnClose)
			PluginConfig.m_HideOnClose = FALSE;
		//
		::SendMessage(pFirstContainer->hwnd, WM_CLOSE, 0, 1);
	}

	hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact) {
		M->WriteDword(hContact, SRMSGMOD_T, "messagecount", 0);
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}

	for(i = 0; i < SERVICE_LAST; i++) {
		if(PluginConfig.hSvc[i])
			DestroyServiceFunction(PluginConfig.hSvc[i]);
	}

	::SI_DeinitStatusIcons();
	::CB_DeInitCustomButtons();
	/*
	 * the event API
	 */

	DestroyHookableEvent(PluginConfig.m_event_MsgWin);
	DestroyHookableEvent(PluginConfig.m_event_MsgPopup);

	::NEN_WriteOptions(&nen_options);
	::DestroyWindow(PluginConfig.g_hwndHotkeyHandler);

	::UnregisterClass(_T("TSStatusBarClass"), g_hInst);
	::UnregisterClass(_T("SideBarClass"), g_hInst);
	::UnregisterClassA("TSTabCtrlClass", g_hInst);
	return 0;
}

int CGlobals::OkToExit(WPARAM wParam, LPARAM lParam)
{
	::CreateSystrayIcon(0);
	::CreateTrayMenus(0);

	CMimAPI::m_shutDown = true;
	UnhookEvent(m_event_EventAdded);
	UnhookEvent(m_event_Dispatch);
	UnhookEvent(m_event_PrebuildMenu);
	UnhookEvent(m_event_SettingChanged);
	UnhookEvent(m_event_ContactDeleted);
	UnhookEvent(m_event_OkToExit);
	UnhookEvent(m_event_AvatarChanged);
	UnhookEvent(m_event_MyAvatarChanged);
	UnhookEvent(m_event_ProtoAck);
	UnhookEvent(m_event_TypingEvent);
	UnhookEvent(m_event_FontsChanged);
	UnhookEvent(m_event_IcoLibChanged);
	UnhookEvent(m_event_IconsChanged);

	if(m_event_SmileyAdd)
		UnhookEvent(m_event_SmileyAdd);

	if(m_event_IEView)
		UnhookEvent(m_event_IEView);

	if(m_event_FoldersChanged)
		UnhookEvent(m_event_FoldersChanged);

	if(m_event_ME_MC_FORCESEND) {
		UnhookEvent(m_event_ME_MC_FORCESEND);
		UnhookEvent(m_event_ME_MC_SUBCONTACTSCHANGED);
		UnhookEvent(m_event_ME_MC_UNFORCESEND);
	}
	::ModPlus_PreShutdown(wParam, lParam);
	::SendLater_ClearAll();
	return 0;
}

/**
 * used on startup to restore flashing tray icon if one or more messages are
 * still "unread"
 */

void CGlobals::RestoreUnreadMessageAlerts(void)
{
	CLISTEVENT 	cle = { 0 };
	DBEVENTINFO dbei = { 0 };
	TCHAR		toolTip[256];
	int 		windowAlreadyExists;
	int 		usingReadNext = 0;

	int autoPopup = M->GetByte(SRMSGMOD, SRMSGSET_AUTOPOPUP, SRMSGDEFSET_AUTOPOPUP);
	HANDLE hDbEvent, hContact;

	dbei.cbSize = sizeof(dbei);
	cle.cbSize = sizeof(cle);
	cle.hIcon = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
	cle.pszService = "SRMsg/ReadMessage";

	hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact) {

		if(M->GetDword(hContact, "SendLater", "count", 0))
		   SendLater_Add(hContact);

		hDbEvent = (HANDLE) CallService(MS_DB_EVENT_FINDFIRSTUNREAD, (WPARAM) hContact, 0);
		while (hDbEvent) {
			dbei.cbBlob = 0;
			CallService(MS_DB_EVENT_GET, (WPARAM) hDbEvent, (LPARAM) & dbei);
			if (!(dbei.flags & (DBEF_SENT | DBEF_READ)) && dbei.eventType == EVENTTYPE_MESSAGE) {
				windowAlreadyExists = M->FindWindow(hContact) != NULL;
				if (!usingReadNext && windowAlreadyExists)
					continue;

				cle.hContact = hContact;
				cle.hDbEvent = hDbEvent;
				mir_sntprintf(toolTip, safe_sizeof(toolTip), CTranslator::get(CTranslator::GEN_STRING_MESSAGEFROM),
							  (TCHAR *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) hContact, GCDNF_TCHAR));
				cle.ptszTooltip = toolTip;
				CallService(MS_CLIST_ADDEVENT, 0, (LPARAM) & cle);
			}
			hDbEvent = (HANDLE) CallService(MS_DB_EVENT_FINDNEXT, (WPARAM) hDbEvent, 0);
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
}

/**
 * retrieve contact cache entry for the given contact. It _never_ returns zero, for a hContact
 * 0, it retrieves a dummy object.
 * Non-existing cache entries are created on demand.
 *
 * @param 	hContact:			contact handle
 * @return	CContactCache*		pointer to the cache entry for this contact
 */
CContactCache* CGlobals::getContactCache(const HANDLE hContact)
{
	size_t i;

	for(i = 0; i < m_cCacheSize; i++) {
		if(m_cCache[i].hContact == hContact) {
			m_cCache[i].c->inc();
			return(m_cCache[i].c);
		}
	}

	CContactCache *c = new CContactCache(hContact);
	m_cCache[m_cCacheSize].hContact = hContact;
	m_cCache[m_cCacheSize++].c = c;
	if(m_cCacheSize == m_cCacheSizeAlloced) {
		m_cCacheSizeAlloced += 50;
		m_cCache = (TCCache *)realloc(m_cCache, m_cCacheSizeAlloced * sizeof(TCCache));
	}
	return(c);
}

/**
 * when the state of the meta contacts protocol changes from enabled to disabled
 * (or vice versa), this updates the contact cache
 *
 * it is ONLY called from the DBSettingChanged() event handler when the relevant
 * database value is touched.
 */
void CGlobals::cacheUpdateMetaChanged()
{
	size_t 			i;
	CContactCache* 	c;

	for(i = 0; i < m_cCacheSize; i++) {
		c = m_cCache[i].c;

	}
}
