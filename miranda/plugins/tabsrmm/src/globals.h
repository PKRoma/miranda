/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2009 Miranda ICQ/IM project,
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

$Id$

*/


class _Globals {
public:
	_Globals()
	{}

	~_Globals()
	{}

	void		BroadcastMessage(UINT msg, WPARAM wParam, LPARAM lParam);
	void		BroadcastMessageAsync(UINT msg, WPARAM wParam, LPARAM lParam);
	INT_PTR		AddWindow(HWND hWnd, HANDLE h);
	INT_PTR		RemoveWindow(HWND hWnd);

	HWND		FindWindow(HANDLE h) const;

	HWND        g_hwndHotkeyHandler;
	HICON       g_iconIn, g_iconOut, g_iconErr, g_iconContainer, g_iconStatus;
	HCURSOR     hCurSplitNS, hCurSplitWE, hCurHyperlinkHand;
	HBITMAP     g_hbmUnknown;
	int         g_MetaContactsAvail, g_SmileyAddAvail, g_WantIEView, g_PopupAvail, g_PopupWAvail, g_WantHPP;
	int         g_FlashAvatarAvail;
	HIMAGELIST  g_hImageList;
	HICON       g_IconMsgEvent, g_IconTypingEvent, g_IconFileEvent, g_IconUrlEvent, g_IconSend;
	HICON       g_IconFolder, g_IconChecked, g_IconUnchecked;
	HICON       g_closeGlyph, g_maxGlyph, g_minGlyph, g_pulldownGlyph;
	int         g_nrProtos;
	HMENU       g_hMenuContext, g_hMenuContainer, g_hMenuEncoding, g_hMenuTrayUnread;
	HMENU       g_hMenuFavorites, g_hMenuRecent, g_hMenuTrayContext;
	HICON       g_buttonBarIcons[NR_BUTTONBARICONS];
	HICON       g_sideBarIcons[NR_SIDEBARICONS];
	HANDLE		g_buttonBarIconHandles[23];
	int         iSendJobCurrent;
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
	int         m_SplitterSaveOnClose;
	int         m_MathModAvail;
	TCHAR       m_MathModStartDelimiter[40];
	int         m_UnreadInTray;
	int         m_TrayFlashes;
	int         m_TrayFlashState;
	BOOL        m_SuperQuiet;
	HANDLE      m_TipOwner;
	HANDLE      m_UserMenuItem;
	double		g_DPIscaleX;
	double		g_DPIscaleY;
	BOOL		g_NickListScrollBarFix;
	BOOL		m_HideOnClose;
	BOOL		g_bSoundOnTyping;
	BOOL		m_AllowTab;
	BOOL		m_AllowOfflineMultisend;
	BOOL		g_bDisableAniAvatars;
	HBITMAP     m_hbmMsgArea;
	BYTE		g_iButtonsBarGap;
	HANDLE		m_hDataPath;
	HANDLE		m_hSkinsPath;
	HANDLE		m_hAvatarsPath;
	char       	szDataPath[MAX_PATH+1];
	char       	szSkinsPath[MAX_PATH + 1];
	char       	szAvatarsPath[MAX_PATH + 1];
	BYTE        m_WinVerMajor;
	BYTE        m_WinVerMinor;
	BYTE        m_bIsXP;
	BYTE        m_SideBarEnabled;
	HWND        m_hwndClist;
	int         m_TabAppearance;
	int         m_VSApiEnabled;
	struct      myTabCtrl tabConfig;
	int         m_panelHeight;
	TCHAR       szDefaultTitleFormat[256];
	DWORD       m_GlobalContainerFlags;
	DWORD       m_GlobalContainerTrans;
	WINDOWPLACEMENT m_GlobalContainerWpos;
	HANDLE      hLastOpenedContact;
	int         m_IdleDetect;
	int         m_DoStatusMsg;
	int         m_smcxicon, m_smcyicon;
	DWORD       local_gmt_diff;
	int         m_PasteAndSend;
	TCHAR       *m_szNoStatus;
	HFONT       m_hFontWebdings;
	struct      infopanelconfig ipConfig;
	COLORREF    crDefault, crIncoming, crOutgoing, crOldIncoming, crOldOutgoing, crStatus;
	BOOL        bUnicodeBuild;
	BYTE        bClipBorder;
	DWORD       bRoundedCorner;
	BYTE        bAvatarBoderType;
	HFONT       hFontCaption;
	COLORREF    skinDefaultFontColor;
	DWORD       m_LangPackCP;
	BYTE        m_SmileyButtonOverride;
	char        g_SkinnedFrame_left, g_SkinnedFrame_right, g_SkinnedFrame_bottom, g_SkinnedFrame_caption;
	char        g_realSkinnedFrame_left, g_realSkinnedFrame_right, g_realSkinnedFrame_bottom, g_realSkinnedFrame_caption;
	HPEN        g_SkinLightShadowPen, g_SkinDarkShadowPen;
	NONCLIENTMETRICS ncm;
	HICON       m_AnimTrayIcons[4];
	BOOL        g_DisableScrollbars;
	BOOL        m_visualMessageSizeIndicator;
	BOOL        m_autoSplit;
	BOOL		m_FlashOnMTN;
	int         rtf_ctablesize;
	DWORD       dwThreadID;
	char        szMetaName[256];
	HBITMAP		hbmLogo;
	HANDLE 		m_hMessageWindowList;
};

class _Mim {
public:
	_Mim()
	{}
	~_Mim()
	{}

	DWORD GetDword(const HANDLE hContact, const char *szModule, const char *szSetting, DWORD uDefault) const;
	DWORD GetDword(const char *szModule, const char *szSetting, DWORD uDefault) const;
	DWORD GetDword(const char *szSetting, DWORD uDefault) const;
	DWORD GetDword(const HANDLE hContact, const char *szSetting, DWORD uDefault) const;

	int GetByte(const HANDLE hContact, const char *szModule, const char *szSetting, int uDefault) const;
	int GetByte(const char *szModule, const char *szSetting, int uDefault) const;
	int GetByte(const char *szSetting, int uDefault) const;
	int GetByte(const HANDLE hContact, const char *szSetting, int uDefault) const;

	INT_PTR WriteDword(const HANDLE hContact, const char *szModule, const char *szSetting, DWORD value) const;
	INT_PTR WriteDword(const char *szModule, const char *szSetting, DWORD value) const;

	INT_PTR WriteByte(const HANDLE hContact, const char *szModule, const char *szSetting, BYTE value) const;
	INT_PTR WriteByte(const char *szModule, const char *szSetting, BYTE value) const;
};

extern	_Globals	Globals;
extern	_Globals	*pGlobals;
extern	_Mim		Mim;
extern  _Mim		*pMim;

#define DPISCALEY(argY) ((int) ((double)(argY) * Globals.g_DPIscaleY))
#define DPISCALEX(argX) ((int) ((double)(argX) * Globals.g_DPIscaleX))

#define DPISCALEY_S(argY) ((int) ((double)(argY) * Globals.g_DPIscaleY))
#define DPISCALEX_S(argX) ((int) ((double)(argX) * Globals.g_DPIscaleX))

