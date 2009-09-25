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
 * the info area for both im and chat sessions
 */

#include "commonheaders.h"

TCHAR *xStatusDescr[] = {	_T("Angry"), _T("Duck"), _T("Tired"), _T("Party"), _T("Beer"), _T("Thinking"), _T("Eating"),
								_T("TV"), _T("Friends"), _T("Coffee"),	_T("Music"), _T("Business"), _T("Camera"), _T("Funny"),
								_T("Phone"), _T("Games"), _T("College"), _T("Shopping"), _T("Sick"), _T("Sleeping"),
								_T("Surfing"), _T("@Internet"), _T("Engineering"), _T("Typing"), _T("Eating... yummy"),
								_T("Having fun"), _T("Chit chatting"),	_T("Crashing"), _T("Going to toilet"), _T("<undef>"),
								_T("<undef>"), _T("<undef>")
							};


InfoPanelConfig CInfoPanel::m_ipConfig = {0};

void CInfoPanel::setActive(const int newActive)
{
	m_active = newActive ? true : false;
}

/**
 * Load height. Private panel height is indicated by 0xffff for the high word
 */
void CInfoPanel::loadHeight()
{
	BYTE bSync = M->GetByte("syncAllPanels", 0);

	m_height = M->GetDword(m_dat->hContact, "panelheight", -1);

	if(m_height == -1 || HIWORD(m_height) == 0) {
		if(!(m_dat->pContainer->dwPrivateFlags & CNT_GLOBALSETTINGS))
			m_height = m_dat->pContainer->panelHeight;
		else
			m_height = bSync ? m_defaultHeight : (m_isChat ? m_defaultMUCHeight : m_defaultHeight);
		m_fPrivateHeight = false;
	}
	else {
		m_fPrivateHeight = true;
		m_height &= 0x0000ffff;
	}

	if (m_height <= 0 || m_height > 120)
		m_height = 52;
}

/**
 * Save current panel height to the database
 *
 * @param fFlush bool: flush values to database (usually only requested by destructor)
 */
void CInfoPanel::saveHeight(bool fFlush)
{
	BYTE bSync = M->GetByte("syncAllPanels", 0);

	if (m_height < 110 && m_height >= MIN_PANELHEIGHT) {          // only save valid panel splitter positions
		if(!m_fPrivateHeight) {
			if(!m_isChat || bSync) {
				if(!(m_dat->pContainer->dwPrivateFlags & CNT_GLOBALSETTINGS))
					m_dat->pContainer->panelHeight = m_height;
				else {
					PluginConfig.m_panelHeight = m_height;
					m_defaultHeight = m_height;
					if(fFlush)
						M->WriteDword(SRMSGMOD_T, "panelheight", m_height);
				}
			}
			else if(m_isChat && !bSync) {
				if(!(m_dat->pContainer->dwPrivateFlags & CNT_GLOBALSETTINGS))
					m_dat->pContainer->panelHeight = m_height;
				else {
					PluginConfig.m_MUCpanelHeight = m_height;
					m_defaultMUCHeight = m_height;
					if(fFlush)
						M->WriteDword("Chat", "panelheight", m_height);
				}
			}
		}
		else
			M->WriteDword(m_dat->hContact, SRMSGMOD_T, "panelheight", MAKELONG(m_height, 0xffff));

	}
}

/**
 * Sets the new height of the panel and broadcasts it to all
 * open sessions
 *
 * @param newHeight  LONG: the new height.
 * @param fBroadcast bool: broadcast the new height to all open sessions, respect
 *                   container's private setting flag.
 */
void CInfoPanel::setHeight(LONG newHeight, bool fBroadcast)
{
	if(newHeight < MIN_PANELHEIGHT || newHeight > 100)
		return;

	m_height = newHeight;

	if(fBroadcast) {
		if(!m_fPrivateHeight) {
			if(m_dat->pContainer->dwPrivateFlags & CNT_GLOBALSETTINGS)
				M->BroadcastMessage(DM_SETINFOPANEL, (WPARAM)m_dat, (LPARAM)newHeight);
			else
				::BroadCastContainer(m_dat->pContainer, DM_SETINFOPANEL, (WPARAM)m_dat, (LPARAM)newHeight);
		}
		saveHeight();
	}
}

void CInfoPanel::Configure() const
{
	::ShowWindow(GetDlgItem(m_dat->hwnd, IDC_PANELSPLITTER), m_active ? SW_SHOW : SW_HIDE);
}

void CInfoPanel::showHideControls(const UINT showCmd) const
{
	if(m_isChat)
		::ShowWindow(GetDlgItem(m_dat->hwnd, IDC_PANELSPLITTER), showCmd);
}

void CInfoPanel::showHide() const
{
	HBITMAP hbm = (m_active && PluginConfig.m_AvatarMode != 5) ? m_dat->hOwnPic : (m_dat->ace ? m_dat->ace->hbmPic : PluginConfig.g_hbmUnknown);
	BITMAP 	bm;
	HWND	hwndDlg = m_dat->hwnd;

	if(!m_isChat) {
		if(!m_active && m_dat->hwndPanelPic) {
			::DestroyWindow(m_dat->hwndPanelPic);
			m_dat->hwndPanelPic = NULL;
		}
		if(!m_active && m_dat->hwndPanelPicParent) {
			::DestroyWindow(m_dat->hwndPanelPicParent);
			m_dat->hwndPanelPicParent = NULL;
			//_DebugTraceA("panel pic parent destroyed");
		}
		//
		m_dat->iRealAvatarHeight = 0;
		::AdjustBottomAvatarDisplay(m_dat);
		::GetObject(hbm, sizeof(bm), &bm);
		::CalcDynamicAvatarSize(m_dat, &bm);
		showHideControls(SW_HIDE);

		if (m_active) {
			if(m_dat->hwndContactPic)	{
				::DestroyWindow(m_dat->hwndContactPic);
				m_dat->hwndContactPic=NULL;
			}
			::GetAvatarVisibility(hwndDlg, m_dat);
			Configure();
			InvalidateRect(hwndDlg, NULL, FALSE);
		}
		::ShowWindow(GetDlgItem(hwndDlg, IDC_PANELSPLITTER), m_active ? SW_SHOW : SW_HIDE);
		::SendMessage(hwndDlg, WM_SIZE, 0, 0);
		::InvalidateRect(GetDlgItem(hwndDlg, IDC_CONTACTPIC), NULL, TRUE);
		::SetAeroMargins(m_dat->pContainer);
		if(M->isAero())
			::InvalidateRect(GetParent(hwndDlg), NULL, FALSE);
		::DM_ScrollToBottom(m_dat, 0, 1);
	}
	else {
		::ShowWindow(GetDlgItem(hwndDlg, IDC_PANELSPLITTER), m_active ? SW_SHOW : SW_HIDE);

		if (m_active) {
			Configure();
			::InvalidateRect(hwndDlg, NULL, FALSE);
		}

		::SendMessage(hwndDlg, WM_SIZE, 0, 0);
		::SetAeroMargins(m_dat->pContainer);
		if(M->isAero())
			::InvalidateRect(GetParent(hwndDlg), NULL, FALSE);
		::DM_ScrollToBottom(m_dat, 0, 1);
	}
}

/**
 * Decide if info panel must be visible for this session. Uses container setting and,
 * if applicable, local (per contact) override.
 *
 * @return bool: panel is visible for this session
 */
bool CInfoPanel::getVisibility()
{
	if (m_dat->hContact == 0) {
		setActive(false);    // no info panel, if no hcontact
		return(false);
	}

	BYTE 	bDefault = (m_dat->pContainer->dwFlags & CNT_INFOPANEL) ? 1 : 0;
	BYTE 	bContact = M->GetByte(m_dat->hContact, "infopanel", 0);

	BYTE 	visible = (bContact == 0 ? bDefault : (bContact == (BYTE)-1 ? 0 : 1));
	setActive(visible);
	return(m_active);
}

/**
 * Render the info panel background.
 *
 * @param hdc	 HDC: target device context
 * @param rc     RECT&: target rectangle
 * @param item   CSkinItem *: The item to render in non-aero mode
 * @param fAero  bool: aero active
 */
void CInfoPanel::renderBG(const HDC hdc, RECT& rc, CSkinItem *item, bool fAero) const
{
	if(m_active) {

		rc.bottom = m_height + 2;
		if(fAero) {
			RECT	rcBlack = rc;
			rcBlack.bottom = m_height + 2; // + 30;
			::FillRect(hdc, &rcBlack, CSkin::m_BrushBack);
			rc.bottom -= 2;
			CSkin::ApplyAeroEffect(hdc, &rc, CSkin::AERO_EFFECT_AREA_INFOPANEL);
		}
		else {
			if(PluginConfig.m_WinVerMajor >= 5) {
				if(CSkin::m_skinEnabled) {
					rc.bottom -= 2;
					CSkin::SkinDrawBG(m_dat->hwnd, m_dat->pContainer->hwnd, m_dat->pContainer, &rc, hdc);
					item = &SkinItems[ID_EXTBKINFOPANELBG];
					if(item->IGNORED)
						item = &SkinItems[ID_EXTBKINFOPANEL];
					CSkin::DrawItem(hdc, &rc, item);
				} else {
					rc.bottom -= 2;
					::DrawAlpha(hdc, &rc, PluginConfig.m_ipBackgroundGradient, 100, PluginConfig.m_ipBackgroundGradientHigh, 0, 17,
							  31, 8, 0);
					rc.top = rc.bottom - 1;
					rc.left--; rc.right++;
					::DrawEdge(hdc, &rc, BDR_SUNKENOUTER, BF_RECT);
				}
			}
			else
				FillRect(hdc, &rc, GetSysColorBrush(COLOR_3DFACE));
		}
	}
}

/**
 * render the content of the info panel. The target area is derived from the
 * position of the invisible text fields.
 *
 * @param hdc HDC: target device context
 */
void CInfoPanel::renderContent(const HDC hdc)
{
	if(m_active) {
		if(!m_isChat) {
			RECT rc;

			rc = m_dat->rcNick;
			if(m_height >= DEGRADE_THRESHOLD) {
				rc.top -= 2;// rc.bottom += 6;
			}
			RenderIPNickname(hdc, rc);
			if(m_height >= DEGRADE_THRESHOLD) {
				rc = m_dat->rcUIN;
				RenderIPUIN(hdc, rc);
			}
			rc = m_dat->rcStatus;
			RenderIPStatus(hdc, rc);

			/*
			 * panel picture
			 */

			DRAWITEMSTRUCT dis = {0};

			dis.rcItem = m_dat->rcPic;
			dis.hDC = hdc;
			dis.hwndItem = m_dat->hwnd;
			if(::MsgWindowDrawHandler(0, (LPARAM)&dis, m_dat->hwnd, m_dat) == 0)
				::PostMessage(m_dat->hwnd, WM_SIZE, 0, 1);
		}
		else {
			RECT rc;
			rc = m_dat->rcNick;

			if(m_height >= DEGRADE_THRESHOLD)
				rc.top -= 2; rc.bottom -= 2;

			Chat_RenderIPNickname(hdc, rc);
			if(m_height >= DEGRADE_THRESHOLD) {
				rc = m_dat->rcUIN;
				Chat_RenderIPSecondLine(hdc, rc);
			}
		}
	}
}

/**
 * Render the nickname in the info panel.
 * This will also show the status message (if one is available)
 * The field will dynamically adjust itself to the available info panel space. If
 * the info panel is too small to show both nick and UIN fields, this field will show
 * the UIN _instead_ of the nickname (most people have the nickname in the title
 * bar anyway).
 *
 * @param hdc    HDC: target DC for drawing
 *
 * @param rcItem RECT &: target rectangle
 * @param dat    _MessageWindowData*: message window information structure
 */
void CInfoPanel::RenderIPNickname(const HDC hdc, RECT& rcItem)
{
	TCHAR 		*szStatusMsg = NULL;
	CSkinItem*	item = &SkinItems[ID_EXTBKINFOPANEL];
	TCHAR*		szTextToShow = 0;
	bool		fShowUin = false;
	COLORREF	clr = 0;

	if(m_height < DEGRADE_THRESHOLD) {
		szTextToShow = m_dat->uin;
		fShowUin = true;
	} else
		szTextToShow = m_dat->szNickname;

	szStatusMsg = m_dat->statusMsg;

	SetBkMode(hdc, TRANSPARENT);

	rcItem.left += 2;
	if (szTextToShow[0]) {
		HFONT hOldFont = 0;
		HICON xIcon = 0;

		xIcon = GetXStatusIcon(m_dat);

		if (xIcon) {
			DrawIconEx(hdc, rcItem.left, (rcItem.bottom + rcItem.top - PluginConfig.m_smcyicon) / 2, xIcon, PluginConfig.m_smcxicon, PluginConfig.m_smcyicon, 0, 0, DI_NORMAL | DI_COMPAT);
			DestroyIcon(xIcon);
			rcItem.left += 21;
		}

		if (m_ipConfig.isValid) {
			if(fShowUin) {
				hOldFont = (HFONT)SelectObject(hdc, m_ipConfig.hFonts[IPFONTID_UIN]);
				clr = m_ipConfig.clrs[IPFONTID_UIN];
			}
			else {
				hOldFont = (HFONT)SelectObject(hdc, m_ipConfig.hFonts[IPFONTID_NICK]);
				clr = m_ipConfig.clrs[IPFONTID_NICK];
			}
		}

		m_szNick.cx = m_szNick.cy = 0;

		if (szStatusMsg && szStatusMsg[0]) {
			SIZE sStatusMsg, sMask;
			DWORD dtFlags, dtFlagsNick;

			GetTextExtentPoint32(hdc, szTextToShow, lstrlen(szTextToShow), &m_szNick);
			GetTextExtentPoint32(hdc, _T("A"), 1, &sMask);
			GetTextExtentPoint32(hdc, szStatusMsg, lstrlen(szStatusMsg), &sStatusMsg);
			dtFlagsNick = DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_NOPREFIX;
			if ((m_szNick.cx + sStatusMsg.cx + 6) < (rcItem.right - rcItem.left) || (rcItem.bottom - rcItem.top) < (2 * sMask.cy))
				dtFlagsNick |= DT_VCENTER;
			CSkin::RenderText(hdc, m_dat->hThemeIP, szTextToShow, &rcItem, dtFlagsNick, CSkin::m_glowSize, clr);
			if (m_ipConfig.isValid) {
				SelectObject(hdc, m_ipConfig.hFonts[IPFONTID_STATUS]);
				clr = m_ipConfig.clrs[IPFONTID_STATUS];
			}
			rcItem.left += (m_szNick.cx + 10);

			if (!(dtFlagsNick & DT_VCENTER))
				//if(dis->rcItem.bottom - dis->rcItem.top >= 2 * sStatusMsg.cy)
				dtFlags = DT_WORDBREAK | DT_END_ELLIPSIS | DT_NOPREFIX;
			else
				dtFlags = DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX | DT_VCENTER;


			rcItem.right -= 3;
			if (rcItem.left + 30 < rcItem.right)
				CSkin::RenderText(hdc, m_dat->hThemeIP, szStatusMsg, &rcItem, dtFlags, CSkin::m_glowSize, clr);
		} else
			CSkin::RenderText(hdc, m_dat->hThemeIP, szTextToShow, &rcItem, DT_SINGLELINE | DT_VCENTER | DT_WORD_ELLIPSIS | DT_NOPREFIX, CSkin::m_glowSize, clr);

		if (hOldFont)
			SelectObject(hdc, hOldFont);
	}
}

/**
 * Draws the UIN field for the info panel.
 *
 * @param hdc    HDC: device context for drawing.
 * @param rcItem RECT &: target rectangle for drawing
 * @param dat    _MessageWindowData *: the message session structure
 */
void CInfoPanel::RenderIPUIN(const HDC hdc, RECT& rcItem)
{
	TCHAR		szBuf[256];
	BOOL 		config = m_ipConfig.isValid;
	HFONT 		hOldFont = 0;
	CSkinItem*	item = &SkinItems[ID_EXTBKINFOPANEL];
	TCHAR		*tszUin = m_dat->uin;
	COLORREF	clr = 0;

	SetBkMode(hdc, TRANSPARENT);

	rcItem.left += 2;
	if (config) {
		hOldFont = (HFONT)SelectObject(hdc, m_ipConfig.hFonts[IPFONTID_UIN]);
		clr = m_ipConfig.clrs[IPFONTID_UIN];
	}
	if (m_dat->uin[0]) {
		SIZE sUIN;
		if (m_dat->idle) {
			time_t diff = time(NULL) - m_dat->idle;
			int i_hrs = diff / 3600;
			int i_mins = (diff - i_hrs * 3600) / 60;
			mir_sntprintf(szBuf, safe_sizeof(szBuf), _T("%s    Idle: %dh,%02dm"), tszUin, i_hrs, i_mins);
			GetTextExtentPoint32(hdc, szBuf, lstrlen(szBuf), &sUIN);
			CSkin::RenderText(hdc, m_dat->hThemeIP, szBuf, &rcItem, DT_SINGLELINE | DT_VCENTER, CSkin::m_glowSize, clr);
		} else {
			GetTextExtentPoint32(hdc, tszUin, lstrlen(tszUin), &sUIN);
			CSkin::RenderText(hdc, m_dat->hThemeIP, tszUin, &rcItem, DT_SINGLELINE | DT_VCENTER, CSkin::m_glowSize, clr);
		}
	}
	if (hOldFont)
		SelectObject(hdc, hOldFont);
}

void CInfoPanel::RenderIPStatus(const HDC hdc, RECT& rcItem)
{
	char		*szProto = m_dat->bIsMeta ? m_dat->szMetaProto : m_dat->szProto;
	SIZE		sProto = {0}, sStatus = {0}, sTime = {0};
	DWORD		oldPanelStatusCX = m_dat->panelStatusCX;
	RECT		rc;
	HFONT		hOldFont = 0;
	BOOL		config = m_ipConfig.isValid;
	CSkinItem 	*item = &SkinItems[ID_EXTBKINFOPANEL];
	TCHAR   	*szFinalProto = NULL;
	TCHAR 		szResult[80];
	int 		base_hour;
	TCHAR 		symbolic_time[3];
	COLORREF	clr = 0;

	szResult[0] = 0;

	if (m_dat->szStatus[0])
		GetTextExtentPoint32(hdc, m_dat->szStatus, lstrlen(m_dat->szStatus), &sStatus);

	/*
	 * figure out final account name
	 */
	if(m_dat->bIsMeta) {
		PROTOACCOUNT *acc = (PROTOACCOUNT *)CallService(MS_PROTO_GETACCOUNT, (WPARAM)0,
														(LPARAM)(m_dat->bIsMeta ? m_dat->szMetaProto : m_dat->szProto));
		if(acc && acc->tszAccountName)
			szFinalProto = acc->tszAccountName;
	}
	else
		szFinalProto = m_dat->szAccount;

	if (szFinalProto) {
		if (config)
			SelectObject(hdc, m_ipConfig.hFonts[IPFONTID_PROTO]);
		GetTextExtentPoint32(hdc, szFinalProto, lstrlen(szFinalProto), &sProto);
	}

	if (m_dat->timezone != -1) {
		DBTIMETOSTRINGT dbtts;
		time_t 			final_time;
		time_t 			now = time(NULL);

		final_time = now - m_dat->timediff;
		dbtts.szDest = szResult;
		dbtts.cbDest = 70;
		dbtts.szFormat = _T("t");
		CallService(MS_DB_TIME_TIMESTAMPTOSTRINGT, final_time, (LPARAM) &dbtts);
		GetTextExtentPoint32(hdc, szResult, lstrlen(szResult), &sTime);
	}

	m_dat->panelStatusCX = 3 + sStatus.cx + sProto.cx + 14 + (m_dat->hClientIcon ? 20 : 0) + sTime.cx + 13;;

	if(m_dat->panelStatusCX != oldPanelStatusCX) {
		SendMessage(m_dat->hwnd, WM_SIZE, 0, 0);
		//CSkin::MapClientToParent(GetDlgItem(m_dat->hwnd, IDC_PANELSTATUS), m_dat->hwnd, rcItem);
		rcItem = m_dat->rcStatus;
	}

	SetBkMode(hdc, TRANSPARENT);
	rc = rcItem;
	rc.left += 2;
	rc.right -=3;

	if(szResult[0]) {
		HFONT oldFont = (HFONT)SelectObject(hdc, PluginConfig.m_hFontWebdings);
		base_hour = _ttoi(szResult);
		base_hour = base_hour > 11 ? base_hour - 12 : base_hour;
		symbolic_time[0] = (TCHAR)(0xB7 + base_hour);
		symbolic_time[1] = 0;
		CSkin::RenderText(hdc, m_dat->hThemeIP, symbolic_time, &rcItem, DT_SINGLELINE | DT_VCENTER, CSkin::m_glowSize, 0);
		if (config) {
			oldFont = (HFONT)SelectObject(hdc, m_ipConfig.hFonts[IPFONTID_TIME]);
			clr = m_ipConfig.clrs[IPFONTID_TIME];
		}
		rcItem.left += 16;
		CSkin::RenderText(hdc, m_dat->hThemeIP, szResult, &rcItem, DT_SINGLELINE | DT_VCENTER, CSkin::m_glowSize, clr);
		SelectObject(hdc, oldFont);
		rc.left += (sTime.cx + 20);
	}

	if (config)
		hOldFont = (HFONT)SelectObject(hdc, m_ipConfig.hFonts[IPFONTID_STATUS]);

	if (m_dat->szStatus[0]) {
		if (config) {
			SelectObject(hdc, m_ipConfig.hFonts[IPFONTID_STATUS]);
			clr = m_ipConfig.clrs[IPFONTID_STATUS];
		}
		CSkin::RenderText(hdc, m_dat->hThemeIP, m_dat->szStatus, &rc, DT_SINGLELINE | DT_VCENTER, CSkin::m_glowSize, clr);
	}
	if (szFinalProto) {
		rc.left = rc.right - sProto.cx - 3 - (m_dat->hClientIcon ? 20 : 0);
		if (config) {
			SelectObject(hdc, m_ipConfig.hFonts[IPFONTID_PROTO]);
			clr = m_ipConfig.clrs[IPFONTID_PROTO];
		} else
			clr = ::GetSysColor(COLOR_HOTLIGHT);
		CSkin::RenderText(hdc, m_dat->hThemeIP, szFinalProto, &rc, DT_SINGLELINE | DT_VCENTER, CSkin::m_glowSize, clr);
	}

	if (m_dat->hClientIcon)
		DrawIconEx(hdc, rc.right - 19, (rc.bottom + rc.top - 16) / 2, m_dat->hClientIcon, 16, 16, 0, 0, DI_NORMAL);

	if (config && hOldFont)
		SelectObject(hdc, hOldFont);
}

/**
 * Draws the Nickname field (first line) in a MUC window
 *
 * @param hdc    HDC: device context for drawing.
 * @param rcItem RECT &: target rectangle for drawing
 */
void CInfoPanel::Chat_RenderIPNickname(const HDC hdc, RECT& rcItem)
{
	SESSION_INFO	*si = reinterpret_cast<SESSION_INFO *>(m_dat->si);

	HFONT 		hOldFont = 0;

	if(si == 0)
		return;

	::SetBkMode(hdc, TRANSPARENT);
	m_szNick.cx = m_szNick.cy = 0;

	if(m_height < DEGRADE_THRESHOLD) {
		TCHAR	tszText[256];

		mir_sntprintf(tszText, 256, CTranslator::get(CTranslator::GEN_MUC_TOPIC_IS), si->ptszTopic ? si->ptszTopic :
					  CTranslator::get(CTranslator::GEN_MUC_NO_TOPIC));

		hOldFont = (HFONT)::SelectObject(hdc, m_ipConfig.hFonts[IPFONTID_UIN]);
		CSkin::RenderText(hdc, m_dat->hThemeIP, tszText, &rcItem, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX | DT_VCENTER,
						  CSkin::m_glowSize, m_ipConfig.clrs[IPFONTID_UIN]);
	} else {

		hOldFont = (HFONT)::SelectObject(hdc, m_ipConfig.hFonts[IPFONTID_NICK]);
		::GetTextExtentPoint32(hdc, m_dat->szNickname, lstrlen(m_dat->szNickname), &m_szNick);
		CSkin::RenderText(hdc, m_dat->hThemeIP, m_dat->szNickname, &rcItem, DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER,
						  CSkin::m_glowSize, m_ipConfig.clrs[IPFONTID_NICK]);
		rcItem.left += (m_szNick.cx + 4);

		::SelectObject(hdc, m_ipConfig.hFonts[IPFONTID_STATUS]);
		if(si->ptszStatusbarText) {
			TCHAR *pTmp = _tcschr(si->ptszStatusbarText, ']');
			pTmp += 2;
			TCHAR tszTemp[30];
			if(si->ptszStatusbarText[0] == '[' && pTmp > si->ptszStatusbarText && ((pTmp - si->ptszStatusbarText) < (size_t)30)) {
				mir_sntprintf(tszTemp, pTmp - si->ptszStatusbarText, _T("%s"), si->ptszStatusbarText);
				CSkin::RenderText(hdc, m_dat->hThemeIP, tszTemp, &rcItem, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX | DT_VCENTER,
								  CSkin::m_glowSize, m_ipConfig.clrs[IPFONTID_STATUS]);
			}
		}
	}
	if (hOldFont)
		SelectObject(hdc, hOldFont);
}

void CInfoPanel::Chat_RenderIPSecondLine(const HDC hdc, RECT& rcItem)
{
	HFONT 	hOldFont = 0;
	SIZE	szTitle;
	TCHAR	szPrefix[100];
	COLORREF clr = 0;

	SESSION_INFO	*si = reinterpret_cast<SESSION_INFO *>(m_dat->si);

	if(si == 0)
		return;

	hOldFont = (HFONT)::SelectObject(hdc, m_ipConfig.hFonts[IPFONTID_UIN]);
	clr = m_ipConfig.clrs[IPFONTID_UIN];

	const TCHAR *szTopicTitle = CTranslator::get(CTranslator::GEN_MUC_TOPIC_IS);
	mir_sntprintf(szPrefix, 100, szTopicTitle, _T(""));
	::GetTextExtentPoint32(hdc, szPrefix, lstrlen(szPrefix), &szTitle);
	rcItem.right -= 3;
	CSkin::RenderText(hdc, m_dat->hThemeIP, szPrefix, &rcItem, DT_SINGLELINE | DT_NOPREFIX | DT_TOP, CSkin::m_glowSize, clr);
	rcItem.left += (szTitle.cx + 4);

	if(si->ptszTopic && lstrlen(si->ptszTopic) > 1)
		CSkin::RenderText(hdc, m_dat->hThemeIP, si->ptszTopic, &rcItem, DT_WORDBREAK | DT_END_ELLIPSIS | DT_NOPREFIX | DT_TOP, CSkin::m_glowSize, clr);
	else
		CSkin::RenderText(hdc, m_dat->hThemeIP, CTranslator::get(CTranslator::GEN_MUC_NO_TOPIC), &rcItem, DT_TOP| DT_SINGLELINE | DT_NOPREFIX, CSkin::m_glowSize, clr);

	if(hOldFont)
		::SelectObject(hdc, hOldFont);
}

void CInfoPanel::Invalidate() const
{
	RECT	rc;

	::GetClientRect(m_dat->hwnd, &rc);
	rc.bottom = m_height;
	::InvalidateRect(m_dat->hwnd, &rc, FALSE);
}

void CInfoPanel::trackMouse(POINT& pt) const
{
	if(!m_active)
		return;

	POINT ptMouse = pt;
	ScreenToClient(m_dat->hwnd, &ptMouse);

	if (PtInRect(&m_dat->rcStatus, ptMouse)) {
		if (!(m_dat->dwFlagsEx & MWF_SHOW_AWAYMSGTIMER)) {
			if (m_dat->hClientIcon && pt.x >= m_dat->rcStatus.right - 20)
				SetTimer(m_dat->hwnd, TIMERID_AWAYMSG + 2, 500, 0);
			else
				SetTimer(m_dat->hwnd, TIMERID_AWAYMSG, 1000, 0);
			m_dat->dwFlagsEx |= MWF_SHOW_AWAYMSGTIMER;
		}
		return;
	} else if (PtInRect(&m_dat->rcNick, ptMouse)) {
		if (!(m_dat->dwFlagsEx & MWF_SHOW_AWAYMSGTIMER)) {
			SetTimer(m_dat->hwnd, TIMERID_AWAYMSG + 1, 1000, 0);
			m_dat->dwFlagsEx |= MWF_SHOW_AWAYMSGTIMER;
		}
		return;
	} else if (IsWindowVisible(m_dat->hwndTip)) {
		if (!PtInRect(&m_dat->rcStatus, ptMouse))
			SendMessage(m_dat->hwndTip, TTM_TRACKACTIVATE, FALSE, 0);
	}
}

void CInfoPanel::showTip(UINT ctrlId, const LPARAM lParam) const
{
	if (m_dat->hwndTip) {
		RECT 	rc;
		bool	fMapped = false;
		POINT	pt;
		TCHAR 	szTitle[256];
		HWND	hwndDlg = m_dat->hwnd;

		if (ctrlId == 0) {
			rc = m_dat->rcStatus;
		}
		else if(ctrlId == IDC_PANELNICK) {
			rc = m_dat->rcNick;
		}
		else if(ctrlId == IDC_PANELSTATUS + 1) {
			rc = m_dat->rcStatus;
		}
		else {
			fMapped = true;
			::GetWindowRect(GetDlgItem(hwndDlg, ctrlId), &rc);
		}

		if(!fMapped) {
			pt.x = rc.left;
			pt.y = rc.bottom;
			::ClientToScreen(m_dat->hwnd, &pt);
			rc.left = pt.x;
			rc.bottom = pt.y;
		}
		::SendMessage(m_dat->hwndTip, TTM_TRACKPOSITION, 0, (LPARAM)MAKELONG(rc.left, rc.bottom));
		if (lParam)
			m_dat->ti.lpszText = reinterpret_cast<TCHAR *>(lParam);
		else {
			if (lstrlen(m_dat->statusMsg) > 0)
				m_dat->ti.lpszText = m_dat->statusMsg;
			else
				m_dat->ti.lpszText = PluginConfig.m_szNoStatus;
		}
		::SendMessage(m_dat->hwndTip, TTM_UPDATETIPTEXT, 0, (LPARAM)&m_dat->ti);
		::SendMessage(m_dat->hwndTip, TTM_SETMAXTIPWIDTH, 0, 350);

		switch (ctrlId) {
			case IDC_PANELNICK: {
				DBVARIANT dbv;

				if (!M->GetTString(m_dat->bIsMeta ? m_dat->hSubContact : m_dat->hContact, m_dat->bIsMeta ? m_dat->szMetaProto : m_dat->szProto, "XStatusName", &dbv)) {
					if (lstrlen(dbv.ptszVal) > 1) {
						_sntprintf(szTitle, safe_sizeof(szTitle), CTranslator::get(CTranslator::GEN_IP_TIP_XSTATUS), m_dat->szNickname, dbv.ptszVal);
						szTitle[safe_sizeof(szTitle) - 1] = 0;
						::DBFreeVariant(&dbv);
						break;
					}
					::DBFreeVariant(&dbv);
				}
				if (m_dat->xStatus > 0 && m_dat->xStatus <= 32) {
					_sntprintf(szTitle, safe_sizeof(szTitle), CTranslator::get(CTranslator::GEN_IP_TIP_XSTATUS), m_dat->szNickname, xStatusDescr[m_dat->xStatus - 1]);
					szTitle[safe_sizeof(szTitle) - 1] = 0;
				} else
					return;
				break;
			}
			case IDC_PANELSTATUS + 1: {
				_sntprintf(szTitle, safe_sizeof(szTitle), CTranslator::get(CTranslator::GEN_IP_TIP_CLIENT), m_dat->szNickname);
				szTitle[safe_sizeof(szTitle) - 1] = 0;
				break;
			}
			case IDC_PANELSTATUS: {
				mir_sntprintf(szTitle, safe_sizeof(szTitle), CTranslator::get(CTranslator::GEN_IP_TIP_STATUSMSG), m_dat->szNickname, m_dat->szStatus);
				break;
			}
			default:
				_sntprintf(szTitle, safe_sizeof(szTitle), CTranslator::get(CTranslator::GEN_IP_TIP_TITLE));
		}
		::SendMessage(m_dat->hwndTip, TTM_SETTITLE, 1, (LPARAM)szTitle);
		::SendMessage(m_dat->hwndTip, TTM_TRACKACTIVATE, TRUE, (LPARAM)&m_dat->ti);
	}
}

/**
 * Stub for the dialog procedure. Just handles INITDIALOG and sets
 * our userdata. Real processing is done by ConfigDlgProc()
 *
 * @params Like a normal dialog procedure
 */

INT_PTR CALLBACK CInfoPanel::ConfigDlgProcStub(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CInfoPanel *infoPanel = reinterpret_cast<CInfoPanel *>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));

	if(infoPanel)
		return(infoPanel->ConfigDlgProc(hwnd, msg, wParam, lParam));

	switch(msg) {
		case WM_INITDIALOG: {
			::SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
			infoPanel = reinterpret_cast<CInfoPanel *>(lParam);
			return(infoPanel->ConfigDlgProc(hwnd, msg, wParam, lParam));
		}
		default:
			break;
	}
	return(FALSE);
}

INT_PTR CALLBACK CInfoPanel::ConfigDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) {
		case WM_INITDIALOG: {
			TCHAR	tszTitle[100];

			mir_sntprintf(tszTitle, 100, CTranslator::getOpt(CTranslator::OPT_IPANEL_VISBILITY_TITLE),
						  m_isChat ? CTranslator::getOpt(CTranslator::OPT_IPANEL_VISIBILTY_CHAT) : CTranslator::getOpt(CTranslator::OPT_IPANEL_VISIBILTY_IM));
			::SetDlgItemText(hwnd, IDC_STATIC_VISIBILTY, tszTitle);

			mir_sntprintf(tszTitle, 100, m_isChat ? CTranslator::getOpt(CTranslator::OPT_IPANEL_SYNC_TITLE_IM) :
						  CTranslator::getOpt(CTranslator::OPT_IPANEL_SYNC_TITLE_MUC));

			::SetDlgItemText(hwnd, IDC_NOSYNC, tszTitle);

			::SendDlgItemMessage(hwnd, IDC_PANELVISIBILITY, CB_INSERTSTRING, -1, (LPARAM)CTranslator::getOpt(CTranslator::OPT_IPANEL_VIS_INHERIT));
			::SendDlgItemMessage(hwnd, IDC_PANELVISIBILITY, CB_INSERTSTRING, -1, (LPARAM)CTranslator::getOpt(CTranslator::OPT_IPANEL_VIS_OFF));
			::SendDlgItemMessage(hwnd, IDC_PANELVISIBILITY, CB_INSERTSTRING, -1, (LPARAM)CTranslator::getOpt(CTranslator::OPT_IPANEL_VIS_ON));

			BYTE v = M->GetByte(m_dat->hContact, "infopanel", 0);
			::SendDlgItemMessage(hwnd, IDC_PANELVISIBILITY, CB_SETCURSEL, (WPARAM)(v == 0 ? 0 : (v == (BYTE)-1 ? 1 : 2)), 0);

			::SendDlgItemMessage(hwnd, IDC_PANELSIZE, CB_INSERTSTRING, -1, (LPARAM)CTranslator::getOpt(CTranslator::OPT_IPANEL_SIZE_GLOBAL));
			::SendDlgItemMessage(hwnd, IDC_PANELSIZE, CB_INSERTSTRING, -1, (LPARAM)CTranslator::getOpt(CTranslator::OPT_IPANEL_SIZE_PRIVATE));

			::SendDlgItemMessage(hwnd, IDC_PANELSIZE, CB_SETCURSEL, (WPARAM)(m_fPrivateHeight ? 1 : 0), 0);

			::CheckDlgButton(hwnd, IDC_NOSYNC, M->GetByte("syncAllPanels", 0) ? BST_UNCHECKED : BST_CHECKED);

			::ShowWindow(GetDlgItem(hwnd, IDC_IPCONFIG_PRIVATECONTAINER), m_dat->pContainer->dwPrivateFlags & CNT_GLOBALSETTINGS ? SW_HIDE : SW_SHOW);

			if(!m_isChat) {
				v = M->GetByte(m_dat->hContact, SRMSGMOD_T, "hideavatar", -1);
				::SendDlgItemMessage(hwnd, IDC_PANELPICTUREVIS, CB_INSERTSTRING, -1, (LPARAM)CTranslator::getOpt(CTranslator::OPT_UPREFS_IPGLOBAL));
				::SendDlgItemMessage(hwnd, IDC_PANELPICTUREVIS, CB_INSERTSTRING, -1, (LPARAM)CTranslator::getOpt(CTranslator::OPT_UPREFS_AVON));
				::SendDlgItemMessage(hwnd, IDC_PANELPICTUREVIS, CB_INSERTSTRING, -1, (LPARAM)CTranslator::getOpt(CTranslator::OPT_UPREFS_AVOFF));
				::SendDlgItemMessage(hwnd, IDC_PANELPICTUREVIS, CB_SETCURSEL, (v == (BYTE)-1 ? 0 : (v == 1 ? 1 : 2)), 0);
			}
			else
				::EnableWindow(GetDlgItem(hwnd, IDC_PANELPICTUREVIS), FALSE);

			return(FALSE);
		}

		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORSTATIC:
		{
			HWND hwndChild = (HWND)lParam;
			UINT id = ::GetDlgCtrlID(hwndChild);

			if(m_configDlgFont == 0) {
				HFONT hFont = (HFONT)::SendDlgItemMessage(hwnd, IDC_IPCONFIG_TITLE, WM_GETFONT, 0, 0);
				LOGFONT lf = {0};

				::GetObject(hFont, sizeof(lf), &lf);
				lf.lfWeight = FW_BOLD;
				m_configDlgBoldFont = ::CreateFontIndirect(&lf);

				lf.lfHeight = (int)(lf.lfHeight * 1.2);
				m_configDlgFont = ::CreateFontIndirect(&lf);
				::SendDlgItemMessage(hwnd, IDC_IPCONFIG_TITLE, WM_SETFONT, (WPARAM)m_configDlgFont, FALSE);
			}

			if(hwndChild == ::GetDlgItem(hwnd, IDC_IPCONFIG_TITLE)) {
				::SetTextColor((HDC)wParam, RGB(60, 60, 150));
				::SendMessage(hwndChild, WM_SETFONT, (WPARAM)m_configDlgFont, FALSE);
			} else if(id == IDC_IPCONFIG_FOOTER || id == IDC_SIZE_TIP || id == IDC_IPCONFIG_PRIVATECONTAINER)
				::SetTextColor((HDC)wParam, RGB(160, 50, 50));
			else if(id == IDC_GROUP_SIZE || id == IDC_GROUP_SCOPE || id == IDC_GROUP_OTHER)
				::SendMessage(hwndChild, WM_SETFONT, (WPARAM)m_configDlgBoldFont, FALSE);

			::SetBkColor((HDC)wParam, ::GetSysColor(COLOR_WINDOW));
			return (INT_PTR)::GetSysColorBrush(COLOR_WINDOW);
		}

		case WM_COMMAND: {
			LONG	lOldHeight = m_height;

			switch(LOWORD(wParam)) {
				case IDC_PANELSIZE: {
					LRESULT iResult = ::SendDlgItemMessage(hwnd, IDC_PANELSIZE, CB_GETCURSEL, 0, 0);

					if(iResult == 0) {
						if(m_fPrivateHeight) {
							M->WriteDword(m_dat->hContact, SRMSGMOD_T, "panelheight", m_height);
							loadHeight();
						}
					}
					else if(iResult == 1) {
						M->WriteDword(m_dat->hContact, SRMSGMOD_T, "panelheight",
									  MAKELONG(M->GetDword(m_dat->hContact, "panelheight", m_height), 0xffff));
						loadHeight();
					}
					break;
				}

				case IDC_PANELPICTUREVIS: {
					BYTE	vOld = M->GetByte(m_dat->hContact, SRMSGMOD_T, "hideavatar", -1);
					LRESULT iResult = ::SendDlgItemMessage(hwnd, IDC_PANELPICTUREVIS, CB_GETCURSEL, 0, 0);

					BYTE vNew = (iResult == 0 ? (BYTE)-1 : (iResult == 1 ? 1 : 0));
					if(vNew != vOld) {
						if(vNew == (BYTE)-1)
							DBDeleteContactSetting(m_dat->hContact, SRMSGMOD_T, "hideavatar");
						else
							M->WriteByte(m_dat->hContact, SRMSGMOD_T, "hideavatar", vNew);
						m_dat->panelWidth = -1;
						::ShowPicture(m_dat, FALSE);
						::SendMessage(m_dat->hwnd, WM_SIZE, 0, 0);
						::DM_ScrollToBottom(m_dat, 0, 1);
					}
					break;
				}

				case IDC_PANELVISIBILITY: {
					BYTE	vOld = M->GetByte(m_dat->hContact, SRMSGMOD_T, "infopanel", 0);
					LRESULT iResult = ::SendDlgItemMessage(hwnd, IDC_PANELVISIBILITY, CB_GETCURSEL, 0, 0);

					BYTE vNew = (iResult == 0 ? 0 : (iResult == 1 ? (BYTE)-1 : 1));
					if(vNew != vOld) {
						M->WriteByte(m_dat->hContact, SRMSGMOD_T, "infopanel", vNew);
						getVisibility();
						showHide();
					}
					break;
				}

				case IDC_SIZECOMPACT:
					setHeight(MIN_PANELHEIGHT + 2, true);
					break;

				case IDC_SIZENORMAL:
					setHeight(DEGRADE_THRESHOLD, true);
					break;

				case IDC_SIZELARGE:
					setHeight(51, true);
					break;

				case IDC_NOSYNC:
					M->WriteByte(SRMSGMOD_T, "syncAllPanels", ::IsDlgButtonChecked(hwnd, IDC_NOSYNC) ? 0 : 1);
					if(!IsDlgButtonChecked(hwnd, IDC_NOSYNC)) {
						loadHeight();
						if(m_dat->pContainer->dwPrivateFlags & CNT_GLOBALSETTINGS)
							M->BroadcastMessage(DM_SETINFOPANEL, (WPARAM)m_dat, (LPARAM)m_defaultHeight);
						else
							::BroadCastContainer(m_dat->pContainer, DM_SETINFOPANEL, (WPARAM)m_dat, (LPARAM)m_defaultHeight);
					} else {
						if(m_dat->pContainer->dwPrivateFlags & CNT_GLOBALSETTINGS)
							M->BroadcastMessage(DM_SETINFOPANEL, (WPARAM)m_dat, 0);
						else
							::BroadCastContainer(m_dat->pContainer,DM_SETINFOPANEL, (WPARAM)m_dat, 0);
					}
					break;
			}
			if(m_height != lOldHeight) {
				::SendMessage(m_dat->hwnd, WM_SIZE, 0, 0);
				m_dat->panelWidth = -1;
				::SetAeroMargins(m_dat->pContainer);
				::RedrawWindow(m_dat->hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
				::RedrawWindow(GetParent(m_dat->hwnd), NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
			}
			break;
		}

		case WM_CLOSE:
			if(wParam == 1 && lParam == 1) {
				::DestroyWindow(hwnd);
			}
			break;

		case WM_DESTROY: {
			::DeleteObject(m_configDlgBoldFont);
			::DeleteObject(m_configDlgFont);

			m_configDlgBoldFont = m_configDlgFont = 0;
			::SetWindowLongPtr(hwnd, GWLP_USERDATA, 0L);
			break;
		}
	}
	return(FALSE);
}

int CInfoPanel::invokeConfigDialog(const POINT& pt)
{
	RECT 	rc;
	POINT	ptTest = pt;

	GetWindowRect(m_dat->hwnd, &rc);
	rc.bottom = rc.top + m_height;
	rc.right -= m_dat->panelWidth;

	if(!PtInRect(&rc, ptTest))
		return(0);

	if(m_hwndConfig == 0) {
		m_configDlgBoldFont = m_configDlgFont = 0;
		m_hwndConfig = ::CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_INFOPANEL), m_dat->pContainer->hwnd,
										   ConfigDlgProcStub, (LPARAM)this);
		if(m_hwndConfig) {
			RECT	rc, rcLog;
			POINT	pt;

			TranslateDialogDefault(m_hwndConfig);

			::GetClientRect(m_hwndConfig, &rc);
			::GetWindowRect(GetDlgItem(m_dat->hwnd, m_isChat ? IDC_CHAT_LOG : IDC_LOG), &rcLog);
			pt.x = rcLog.left;
			pt.y = rcLog.top;
			::ScreenToClient(m_dat->pContainer->hwnd, &pt);

			::SetWindowPos(m_hwndConfig, HWND_TOP, pt.x + 10, pt.y - (m_active ? 10 : 0), 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
			return(1);
		}
	}
	return(0);
}

