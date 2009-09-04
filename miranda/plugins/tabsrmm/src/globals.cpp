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

CGlobals PluginConfig;
CGlobals *pConfig = &PluginConfig;

void CGlobals::Reload()
{
	DWORD dwFlags = M->GetDword("mwflags", MWF_LOG_DEFAULT);

	m_SendOnShiftEnter = (int)M->GetByte("sendonshiftenter", 0);
	m_SendOnEnter = (int)M->GetByte(SRMSGSET_SENDONENTER, SRMSGDEFSET_SENDONENTER);
	m_SendOnDblEnter = (int)M->GetByte("SendOnDblEnter", 0);
	m_AutoLocaleSupport = (int)M->GetByte("al", 1);
	m_AutoSwitchTabs = (int)M->GetByte("autoswitchtabs", 1);
	m_CutContactNameTo = (int) DBGetContactSettingWord(NULL, SRMSGMOD_T, "cut_at", 15);
	m_CutContactNameOnTabs = (int)M->GetByte("cuttitle", 0);
	m_StatusOnTabs = (int)M->GetByte("tabstatus", 1);
	m_LogStatusChanges = (int)dwFlags&MWF_LOG_STATUSCHANGES;//DBGetContactSettingByte(NULL, SRMSGMOD_T, "logstatus", 0);
	m_UseDividers = (int)M->GetByte("usedividers", 0);
	m_DividersUsePopupConfig = (int)M->GetByte("div_popupconfig", 0);
	m_MsgTimeout = (int)M->GetDword(SRMSGMOD, SRMSGSET_MSGTIMEOUT, SRMSGDEFSET_MSGTIMEOUT);

	if (m_MsgTimeout < SRMSGSET_MSGTIMEOUT_MIN)
		m_MsgTimeout = SRMSGSET_MSGTIMEOUT_MIN;

	m_EscapeCloses = (int)M->GetByte("escmode", 0);

	m_HideOnClose =(int) M->GetByte("hideonclose", 0);
	m_AllowTab =(int) M->GetByte("tabmode", 0);
	m_AllowOfflineMultisend =(int) M->GetByte("AllowOfflineMultisend", 0);

	m_WarnOnClose = (int)M->GetByte("warnonexit", 0);
	m_AvatarMode = (int)M->GetByte("avatarmode", 0);

	if (m_AvatarMode == 1 || m_AvatarMode == 2)
		m_AvatarMode = 3;

	m_OwnAvatarMode = (int)M->GetByte("ownavatarmode", 0);
	m_FlashOnClist = (int)M->GetByte("flashcl", 0);
	m_TabAutoClose = (int)M->GetDword("tabautoclose", 0);
	m_AlwaysFullToolbarWidth = (int)M->GetByte("alwaysfulltoolbar", 1);
	m_LimitStaticAvatarHeight = (int)M->GetDword("avatarheight", 96);
	m_SendFormat = (int)M->GetByte("sendformat", 0);
	m_FormatWholeWordsOnly = 1;
	m_FixFutureTimestamps = (int)M->GetByte("do_fft", 1);
	m_RTLDefault = (int)M->GetByte("rtldefault", 0);
	m_SplitterSaveOnClose = (int)M->GetByte("splitsavemode", 1);
	m_MathModAvail = ServiceExists(MATH_RTF_REPLACE_FORMULAE);
	m_WinVerMajor = WinVerMajor();
	m_WinVerMinor = WinVerMinor();
	m_bIsXP = IsWinVerXPPlus();
	m_bIsVista = IsWinVerVistaPlus();
	m_bIsWin7 = IsWinVer7Plus();
	m_TabAppearance = (int)M->GetDword("tabconfig", TCF_FLASHICON | TCF_SINGLEROWTABCONTROL);
	m_panelHeight = (DWORD)M->GetDword("panelheight", 51);
	m_MUCpanelHeight = M->GetDword("Chat", "panelheight", 30);
	m_IdleDetect = (int)M->GetByte("detectidle", 1);
	m_smcxicon = GetSystemMetrics(SM_CXSMICON);
	m_smcyicon = GetSystemMetrics(SM_CYSMICON);
	m_PasteAndSend = (int)M->GetByte("pasteandsend", 1);
	m_szNoStatus = const_cast<TCHAR *>(CTranslator::get(CTranslator::GEN_NO_STATUS));
	m_LangPackCP = ServiceExists(MS_LANGPACK_GETCODEPAGE) ? CallService(MS_LANGPACK_GETCODEPAGE, 0, 0) : CP_ACP;
	m_SmileyButtonOverride = (BYTE)M->GetByte("smbutton_override", 1);
	m_visualMessageSizeIndicator = M->GetByte("msgsizebar", 0);
	m_autoSplit = M->GetByte("autosplit", 0);
	m_FlashOnMTN = M->GetByte(SRMSGMOD, SRMSGSET_SHOWTYPINGWINFLASH, SRMSGDEFSET_SHOWTYPINGWINFLASH);
	if(m_MenuBar == 0)
		m_MenuBar = ::LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_MENUBAR));

	m_ncm.cbSize = sizeof(NONCLIENTMETRICS);
	m_ipBackgroundGradient = M->GetDword(FONTMODULE, "ipfieldsbg", GetSysColor(COLOR_3DSHADOW));
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &m_ncm, 0);
}

const HMENU CGlobals::getMenuBar()
{
	if(m_MenuBar == 0)
		m_MenuBar = ::LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_MENUBAR));
	return(m_MenuBar);
}

