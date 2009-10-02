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

#ifndef __GLOBALS_H
#define __GLOBALS_H

#define C_INVALID_PROTO "<proto error>"
#define C_INVALID_ACCOUNT _T("<account error>")

struct TSplitterBroadCast {
	ContainerWindowData *pSrcContainer;
	_MessageWindowData  *pSrcDat;
	LONG				pos, pos_chat;
	LONG				off_chat, off_im;
	LPARAM				lParam;
	BYTE				bSync;
};

struct TSessionStats {
	enum {
		BYTES_RECEIVED 		= 1,
		BYTES_SENT 			= 2,
		FAILURE				= 3,
		UPDATE_WITH_LAST_RCV= 4,
		SET_LAST_RCV		= 5,
		INIT_TIMER			= 6,
	};

	time_t 			started;
	unsigned int 	iSent, iReceived, iSentBytes, iReceivedBytes;
	unsigned int	messageCount;
	unsigned int 	iFailures;
	unsigned int 	lastReceivedChars;
	BOOL 			bWritten;
};

class CContactCache {
public:
	CContactCache									() {}
	CContactCache									(const HANDLE hContact);
	~CContactCache									()
	{
		if(m_stats)
			delete m_stats;
	}
	const	WORD			getStatus				() const { return(m_wStatus); }
	const	WORD			getMetaStatus			() const { return(m_wMetaStatus); }
	const	WORD			getActiveStatus			() const { return(m_isMeta ? m_wMetaStatus : m_wStatus); }
	const	WORD			getOldStatus			() const { return(m_wOldStatus); }
	const	TCHAR*			getNick					() const { return(m_szNick); }
	const	HANDLE			getContact				() const { return(m_hContact); }
	const	HANDLE			getActiveContact		() const { return(m_isMeta ? (m_hSubContact ? m_hSubContact : m_hContact) : m_hContact); }
	const	DWORD			getIdleTS				() const { return(m_idleTS); }
	const	char*			getProto				() const { return(m_szProto); }
	const	char*			getMetaProto			() const { return(m_szMetaProto ? m_szMetaProto : C_INVALID_PROTO); }
	const	char*			getActiveProto			() const { return(m_isMeta ? (m_szMetaProto ? m_szMetaProto : m_szProto) : m_szProto); }
	bool					isMeta					() const { return(m_isMeta); }
	const TCHAR*			getRealAccount			() const { return(m_szAccount ? m_szAccount : C_INVALID_ACCOUNT); }
	const TCHAR*			getUIN					() const { return(m_szUIN); }

	void					updateStats				(int iType, size_t value = 0);
	const DWORD				getSessionStart			() const { return(m_stats->started); }
	const int				getSessionMsgCount		() const { return((int)m_stats->messageCount) ; }
	void					updateState				();
	bool					updateStatus			();
	void					updateNick				();
	void					updateMeta				();
	void					updateUIN				();

private:
	void					allocStats				();

private:
	HANDLE			m_hContact;
	HANDLE			m_hSubContact;
	WORD			m_wStatus, m_wMetaStatus;
	WORD			m_wOldStatus;
	char*			m_szProto, *m_szMetaProto;
	TCHAR*			m_szAccount;
	TCHAR			m_szNick[80], m_szUIN[80];
	DWORD			m_idleTS;
	bool			m_isMeta;
	bool			m_Valid;
	TSessionStats* 	m_stats;
};

typedef std::map<HANDLE, CContactCache *> ContactCache;
typedef std::map<HANDLE, CContactCache *>::iterator ContactCacheIterator;

typedef BOOL (WINAPI *pfnSetMenuInfo )( HMENU hmenu, LPCMENUINFO lpcmi );

class CGlobals
{
public:
	enum {
		H_MS_MSG_SENDMESSAGE 		= 0,
		H_MS_MSG_SENDMESSAGEW 		= 1,
		H_MS_MSG_FORWARDMESSAGE 	= 2,
		H_MS_MSG_GETWINDOWAPI 		= 3,
		H_MS_MSG_GETWINDOWCLASS 	= 4,
		H_MS_MSG_GETWINDOWDATA 		= 5,
		H_MS_MSG_READMESSAGE 		= 6,
		H_MS_MSG_TYPINGMESSAGE 		= 7,
		H_MS_MSG_MOD_MESSAGEDIALOGOPENED = 8,
		H_MS_TABMSG_SETUSERPREFS 	= 9,
		H_MS_TABMSG_TRAYSUPPORT 	= 10,
		H_MSG_MOD_GETWINDOWFLAGS 	= 11,
		SERVICE_LAST 				= 12
	};

	CGlobals()
	{
		//::ZeroMemory(this, sizeof(CGlobals));
		m_ContactCache.clear();
	}

	~CGlobals()
	{
		if(m_MenuBar)
			::DestroyMenu(m_MenuBar);

		ContactCacheIterator it;

		while(m_ContactCache.size()) {
			it = m_ContactCache.begin();
			delete ((*it).second);
			m_ContactCache.erase(it);
		}
	}
	void		reloadAdv();
	void		reloadSystemStartup();
	void		reloadSystemModulesChanged();
	void		reloadSettings();

	void		hookSystemEvents();
	void		hookPluginEvents();

	const HMENU getMenuBar();
	CContactCache*						getContactCache(const HANDLE hContact);

	HWND        g_hwndHotkeyHandler;
	HICON       g_iconIn, g_iconOut, g_iconErr, g_iconContainer, g_iconStatus;
	HICON		g_iconOverlayDisabled, g_iconOverlayEnabled;
	HCURSOR     hCurSplitNS, hCurSplitWE, hCurHyperlinkHand;
	HBITMAP     g_hbmUnknown;
	int         g_MetaContactsAvail, g_SmileyAddAvail, g_WantIEView, g_PopupAvail, g_PopupWAvail, g_WantHPP;
	int         g_FlashAvatarAvail;
	HIMAGELIST  g_hImageList;
	HICON       g_IconMsgEvent, g_IconTypingEvent, g_IconFileEvent, g_IconSend;
	HICON       g_IconFolder, g_IconChecked, g_IconUnchecked;
	HMENU       g_hMenuContext, g_hMenuContainer, g_hMenuEncoding, g_hMenuTrayUnread;
	HMENU       g_hMenuFavorites, g_hMenuRecent, g_hMenuTrayContext;
	HICON       g_buttonBarIcons[NR_BUTTONBARICONS];
	HICON       g_sideBarIcons[NR_SIDEBARICONS];
	HANDLE		g_buttonBarIconHandles[23];
	// dynamic options, need reload when options change
	int         m_SendOnShiftEnter;
	int         m_SendOnEnter;
	int         m_SendOnDblEnter;
	int         m_AutoLocaleSupport;
	int         m_AutoSwitchTabs;
	int         m_CutContactNameOnTabs;
	int         m_CutContactNameTo;
	int         m_StatusOnTabs;
	int         m_LogStatusChanges;
	int         m_UseDividers;
	int         m_DividersUsePopupConfig;
	int         m_MsgTimeout;
	int         m_EscapeCloses;
	int         m_WarnOnClose;
	int         m_AvatarMode, m_OwnAvatarMode;
	int         m_FlashOnClist;
	int         m_TabAutoClose;
	int         m_AlwaysFullToolbarWidth;
	int         m_LimitStaticAvatarHeight;
	int         m_SendFormat;
	int         m_FormatWholeWordsOnly;
	int         m_FixFutureTimestamps;
	int         m_RTLDefault;
	int         m_MathModAvail;
	TCHAR       m_MathModStartDelimiter[40];
	int         m_UnreadInTray;
	int         m_TrayFlashes;
	int         m_TrayFlashState;
	BOOL        m_SuperQuiet;
	HANDLE      m_UserMenuItem;
	double		g_DPIscaleX;
	double		g_DPIscaleY;
	BOOL		g_NickListScrollBarFix;
	BOOL		m_HideOnClose;
	BOOL		g_bSoundOnTyping;
	BOOL		m_AllowTab;
	BYTE		m_AllowOfflineMultisend;
	BOOL		g_bDisableAniAvatars;
	HBITMAP     m_hbmMsgArea;
	BYTE		g_iButtonsBarGap;
	BYTE        m_WinVerMajor;
	BYTE        m_WinVerMinor;
	bool		m_bIsXP, m_bIsVista, m_bIsWin7;
	HWND        m_hwndClist;
	int         m_TabAppearance;
	struct      myTabCtrl tabConfig;
	int         m_panelHeight, m_MUCpanelHeight;
	TCHAR       szDefaultTitleFormat[256];
	DWORD       m_GlobalContainerFlags, m_GlobalContainerFlagsEx;
	DWORD       m_GlobalContainerTrans;
	WINDOWPLACEMENT m_GlobalContainerWpos;
	HANDLE      hLastOpenedContact;
	int         m_IdleDetect;
	int         m_smcxicon, m_smcyicon;
	DWORD       local_gmt_diff;
	int         m_PasteAndSend;
	TCHAR       *m_szNoStatus;
	HFONT       m_hFontWebdings;
	COLORREF    crDefault, crIncoming, crOutgoing, crOldIncoming, crOldOutgoing, crStatus;
	BOOL        bUnicodeBuild;
	HFONT       hFontCaption;
	DWORD       m_LangPackCP;
	BYTE        m_SmileyButtonOverride;
	NONCLIENTMETRICS m_ncm;
	HICON       m_AnimTrayIcons[4];
	BOOL        m_visualMessageSizeIndicator;
	BOOL        m_autoSplit;
	BOOL		m_FlashOnMTN;
	int         rtf_ctablesize;
	DWORD       dwThreadID;
	char        szMetaName[256];
	HANDLE 		m_hMessageWindowList, hUserPrefsWindowList;
	bool		m_chat_enabled;
	HMENU		m_MenuBar;
	COLORREF	m_ipBackgroundGradient;
	COLORREF	m_ipBackgroundGradientHigh;
	COLORREF	m_tbBackgroundHigh, m_tbBackgroundLow;
	BOOL		m_SendLaterAvail;
	BYTE		g_bClientInStatusBar;
	HANDLE		hSvc[SERVICE_LAST];
	HANDLE		m_event_MsgWin, m_event_MsgPopup;
	HANDLE		m_hMenuItem;
	TSplitterBroadCast lastSPlitterPos;
	static HANDLE		m_event_FoldersChanged;

private:
	ContactCache			m_ContactCache;

private:
	bool				m_TypingSoundAdded;
	static HANDLE		m_event_ModulesLoaded, m_event_PrebuildMenu, m_event_SettingChanged;
	static HANDLE		m_event_ContactDeleted, m_event_Dispatch, m_event_EventAdded;
	static HANDLE		m_event_IconsChanged, m_event_TypingEvent, m_event_ProtoAck, m_event_PreShutdown, m_event_OkToExit;
	static HANDLE		m_event_IcoLibChanged, m_event_AvatarChanged, m_event_MyAvatarChanged, m_event_FontsChanged;
	static HANDLE		m_event_SmileyAdd, m_event_IEView;
	static HANDLE 		m_event_ME_MC_SUBCONTACTSCHANGED, m_event_ME_MC_FORCESEND, m_event_ME_MC_UNFORCESEND;

private:
	static	int		ModulesLoaded(WPARAM wParam, LPARAM lParam);
	static	int 	DBSettingChanged(WPARAM wParam, LPARAM lParam);
	static  int 	DBContactDeleted(WPARAM wParam, LPARAM lParam);
	static	int 	PreshutdownSendRecv(WPARAM wParam, LPARAM lParam);
	static 	int		MetaContactEvent(WPARAM wParam, LPARAM lParam);
	static	int 	OkToExit(WPARAM wParam, LPARAM lParam);
	static  void 	RestoreUnreadMessageAlerts(void);
	static  void	RegisterWithUpdater();
};

extern	CGlobals	PluginConfig;
extern	CGlobals	*pConfig;

#define DPISCALEY(argY) ((int) ((double)(argY) * PluginConfig.g_DPIscaleY))
#define DPISCALEX(argX) ((int) ((double)(argX) * PluginConfig.g_DPIscaleX))

#define DPISCALEY_S(argY) ((int) ((double)(argY) * PluginConfig.g_DPIscaleY))
#define DPISCALEX_S(argX) ((int) ((double)(argX) * PluginConfig.g_DPIscaleX))

#endif /* __GLOBALS_H */
