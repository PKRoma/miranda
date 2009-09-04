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

void CInfoPanel::loadHeight()
{
	if (!(m_dat->dwFlagsEx & MWF_SHOW_SPLITTEROVERRIDE))
		m_height = m_isChat ? m_defaultMUCHeight : m_defaultHeight;
	else
		m_height = M->GetDword(m_dat->hContact, "panelheight", m_isChat ? m_defaultMUCHeight : m_defaultHeight);

	if (m_height <= 0 || m_height > 120)
		m_height = 52;
}

/**
 * Save current panel height to the database
 */
void CInfoPanel::saveHeight()
{

	if (m_height < 110 && m_height >= MIN_PANELHEIGHT) {          // only save valid panel splitter positions
		if (m_dat->dwFlagsEx & MWF_SHOW_SPLITTEROVERRIDE)
			M->WriteDword(m_dat->hContact, SRMSGMOD_T, "panelheight", m_height);
		else {
			if(!m_isChat) {
				PluginConfig.m_panelHeight = m_height;
				M->WriteDword(SRMSGMOD_T, "panelheight", m_height);
			}
			else {
				PluginConfig.m_MUCpanelHeight = m_height;
				M->WriteDword("Chat", "panelheight", m_height);
			}
		}
	}
}

void CInfoPanel::Configure() const
{
	if(!m_isChat) {
		::ShowWindow(GetDlgItem(m_dat->hwnd, IDC_PANELSTATUS), SW_HIDE);
		::ShowWindow(GetDlgItem(m_dat->hwnd, IDC_PANELPIC), SW_HIDE);
	}

	::ShowWindow(GetDlgItem(m_dat->hwnd, IDC_PANELUIN), SW_HIDE);
	::ShowWindow(GetDlgItem(m_dat->hwnd, IDC_PANELNICK), SW_HIDE);
	::ShowWindow(GetDlgItem(m_dat->hwnd, IDC_PANELSPLITTER), m_active ? SW_SHOW : SW_HIDE);
}

void CInfoPanel::showHideControls(const UINT showCmd) const
{
	if(m_isChat)
		::ShowWindow(GetDlgItem(m_dat->hwnd, IDC_PANELSPLITTER), showCmd);
	else {
		::ShowWindow(GetDlgItem(m_dat->hwnd, IDC_PANELPIC), showCmd);
	}
}

void CInfoPanel::showHide() const
{
	HBITMAP hbm = (m_active && PluginConfig.m_AvatarMode != 5) ? m_dat->hOwnPic : (m_dat->ace ? m_dat->ace->hbmPic : PluginConfig.g_hbmUnknown);
	BITMAP 	bm;
	HWND	hwndDlg = m_dat->hwnd;

	if(!m_isChat) {
		if(!m_active && m_dat->hwndPanelPic) {
			::DestroyWindow(m_dat->hwndPanelPic);
			m_dat->hwndPanelPic=NULL;
		}
		//
		m_dat->iRealAvatarHeight = 0;
		::AdjustBottomAvatarDisplay(m_dat);
		::GetObject(hbm, sizeof(bm), &bm);
		::CalcDynamicAvatarSize(m_dat, &bm);
		showHideControls(SW_HIDE);

		//ShowMultipleControls(hwndDlg, infoPanelControls, 2, dat->dwFlagsEx & MWF_SHOW_INFOPANEL ? SW_SHOW : SW_HIDE);

		if (m_active) {
			//MAD
			if(m_dat->hwndContactPic)	{
				::DestroyWindow(m_dat->hwndContactPic);
				m_dat->hwndContactPic=NULL;
			}
			//
			::GetAvatarVisibility(hwndDlg, m_dat);
			Configure();
			InvalidateRect(hwndDlg, NULL, FALSE);
		}
		::ShowWindow(GetDlgItem(m_dat->hwnd, IDC_PANELSTATUS), SW_HIDE);
		::ShowWindow(GetDlgItem(m_dat->hwnd, IDC_PANELUIN), SW_HIDE);
		::ShowWindow(GetDlgItem(m_dat->hwnd, IDC_PANELNICK), SW_HIDE);
		::ShowWindow(GetDlgItem(m_dat->hwnd, IDC_PANELPIC), SW_HIDE);
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

		::ShowWindow(GetDlgItem(m_dat->hwnd, IDC_PANELUIN), SW_HIDE);
		::ShowWindow(GetDlgItem(m_dat->hwnd, IDC_PANELNICK), SW_HIDE);
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
			::FillRect(hdc, &rcBlack, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
			rc.bottom -= 2;
			CSkin::ApplyAeroEffect(hdc, &rc, CSkin::AERO_EFFECT_AREA_INFOPANEL);
			rc.top = rc.bottom - 1;
			rc.left--; rc.right++;
			//::DrawEdge(hdc, &rc, BDR_SUNKENOUTER, BF_RECT);
		}
		else {
			if(PluginConfig.m_WinVerMajor >= 5) {
				if(m_dat->pContainer->bSkinned)
					CSkin::SkinDrawBG(m_dat->hwnd, m_dat->pContainer->hwnd, m_dat->pContainer, &rc, hdc);
				rc.bottom -= 2;
				::DrawAlpha(hdc, &rc, PluginConfig.m_ipBackgroundGradient, 100, GetSysColor(COLOR_3DFACE), 1, 17,
						  31, 8, 0);
				rc.top = rc.bottom - 1;
				rc.left--; rc.right++;
				::DrawEdge(hdc, &rc, BDR_SUNKENOUTER, BF_RECT);
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

			CSkin::MapClientToParent(GetDlgItem(m_dat->hwnd, IDC_PANELNICK), m_dat->hwnd, rc);
			if(m_height >= DEGRADE_THRESHOLD) {
				rc.top -= 2; rc.bottom += 6;
			}
			RenderIPNickname(hdc, rc);
			if(m_height >= DEGRADE_THRESHOLD) {
				CSkin::MapClientToParent(GetDlgItem(m_dat->hwnd, IDC_PANELUIN), m_dat->hwnd, rc);
				RenderIPUIN(hdc, rc);
			}
			CSkin::MapClientToParent(GetDlgItem(m_dat->hwnd, IDC_PANELSTATUS), m_dat->hwnd, rc);
			RenderIPStatus(hdc, rc);

			/*
			 * panel picture
			 */

			DRAWITEMSTRUCT dis = {0};

			::ShowWindow(GetDlgItem(m_dat->hwnd, IDC_PANELPIC), SW_HIDE);
			CSkin::MapClientToParent(GetDlgItem(m_dat->hwnd, IDC_PANELPIC), m_dat->hwnd, dis.rcItem);
			dis.hDC = hdc;
			dis.hwndItem = GetDlgItem(m_dat->hwnd, IDC_PANELPIC);
			::MsgWindowDrawHandler(0, (LPARAM)&dis, m_dat->hwnd, m_dat);
		}
		else {
			RECT rc;
			CSkin::MapClientToParent(GetDlgItem(m_dat->hwnd, IDC_PANELNICK), m_dat->hwnd, rc);
			if(m_height >= DEGRADE_THRESHOLD) {
				rc.top -= 2; rc.bottom += 6;
			}
			Chat_RenderIPNickname(hdc, rc);
			if(m_height >= DEGRADE_THRESHOLD) {
				CSkin::MapClientToParent(GetDlgItem(m_dat->hwnd, IDC_PANELUIN), m_dat->hwnd, rc);
				rc.top -= 3;
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
				SetTextColor(hdc, m_ipConfig.clrs[IPFONTID_UIN]);
			}
			else {
				hOldFont = (HFONT)SelectObject(hdc, m_ipConfig.hFonts[IPFONTID_NICK]);
				SetTextColor(hdc, m_ipConfig.clrs[IPFONTID_NICK]);
			}
		}
		if (szStatusMsg && szStatusMsg[0]) {
			SIZE szNick, sStatusMsg, sMask;
			DWORD dtFlags, dtFlagsNick;

			GetTextExtentPoint32(hdc, szTextToShow, lstrlen(szTextToShow), &szNick);
			GetTextExtentPoint32(hdc, _T("A"), 1, &sMask);
			GetTextExtentPoint32(hdc, szStatusMsg, lstrlen(szStatusMsg), &sStatusMsg);
			dtFlagsNick = DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_NOPREFIX;
			if ((szNick.cx + sStatusMsg.cx + 6) < (rcItem.right - rcItem.left) || (rcItem.bottom - rcItem.top) < (2 * sMask.cy))
				dtFlagsNick |= DT_VCENTER;
			CSkin::RenderText(hdc, m_dat->hThemeIP, szTextToShow, &rcItem, dtFlagsNick, CSkin::m_glowSize);
			if (m_ipConfig.isValid) {
				SelectObject(hdc, m_ipConfig.hFonts[IPFONTID_STATUS]);
				SetTextColor(hdc, m_ipConfig.clrs[IPFONTID_STATUS]);
			}
			rcItem.left += (szNick.cx + 10);

			if (!(dtFlagsNick & DT_VCENTER))
				//if(dis->rcItem.bottom - dis->rcItem.top >= 2 * sStatusMsg.cy)
				dtFlags = DT_WORDBREAK | DT_END_ELLIPSIS | DT_NOPREFIX;
			else
				dtFlags = DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX | DT_VCENTER;


			rcItem.right -= 3;
			if (rcItem.left + 30 < rcItem.right)
				CSkin::RenderText(hdc, m_dat->hThemeIP, szStatusMsg, &rcItem, dtFlags, CSkin::m_glowSize);
		} else
			CSkin::RenderText(hdc, m_dat->hThemeIP, szTextToShow, &rcItem, DT_SINGLELINE | DT_VCENTER | DT_WORD_ELLIPSIS | DT_NOPREFIX, CSkin::m_glowSize);

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

	SetBkMode(hdc, TRANSPARENT);

	rcItem.left += 2;
	if (config) {
		hOldFont = (HFONT)SelectObject(hdc, m_ipConfig.hFonts[IPFONTID_UIN]);
		SetTextColor(hdc, m_ipConfig.clrs[IPFONTID_UIN]);
	}
	if (m_dat->uin[0]) {
		SIZE sUIN;
		if (m_dat->idle) {
			time_t diff = time(NULL) - m_dat->idle;
			int i_hrs = diff / 3600;
			int i_mins = (diff - i_hrs * 3600) / 60;
			mir_sntprintf(szBuf, safe_sizeof(szBuf), _T("%s    Idle: %dh,%02dm"), tszUin, i_hrs, i_mins);
			GetTextExtentPoint32(hdc, szBuf, lstrlen(szBuf), &sUIN);
			CSkin::RenderText(hdc, m_dat->hThemeIP, szBuf, &rcItem, DT_SINGLELINE | DT_VCENTER, CSkin::m_glowSize);
		} else {
			GetTextExtentPoint32(hdc, tszUin, lstrlen(tszUin), &sUIN);
			CSkin::RenderText(hdc, m_dat->hThemeIP, tszUin, &rcItem, DT_SINGLELINE | DT_VCENTER, CSkin::m_glowSize);
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
		CSkin::MapClientToParent(GetDlgItem(m_dat->hwnd, IDC_PANELSTATUS), m_dat->hwnd, rcItem);
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
		CSkin::RenderText(hdc, m_dat->hThemeIP, symbolic_time, &rcItem, DT_SINGLELINE | DT_VCENTER);
		if (config) {
			oldFont = (HFONT)SelectObject(hdc, m_ipConfig.hFonts[IPFONTID_TIME]);
			SetTextColor(hdc, m_ipConfig.clrs[IPFONTID_TIME]);
		}
		rcItem.left += 16;
		CSkin::RenderText(hdc, m_dat->hThemeIP, szResult, &rcItem, DT_SINGLELINE | DT_VCENTER, CSkin::m_glowSize);
		SelectObject(hdc, oldFont);
		rc.left += (sTime.cx + 20);
	}

	if (config)
		hOldFont = (HFONT)SelectObject(hdc, m_ipConfig.hFonts[IPFONTID_STATUS]);

	if (m_dat->szStatus[0]) {
		if (config) {
			SelectObject(hdc, m_ipConfig.hFonts[IPFONTID_STATUS]);
			SetTextColor(hdc, m_ipConfig.clrs[IPFONTID_STATUS]);
		}
		CSkin::RenderText(hdc, m_dat->hThemeIP, m_dat->szStatus, &rc, DT_SINGLELINE | DT_VCENTER, CSkin::m_glowSize);
	}
	if (szFinalProto) {
		rc.left = rc.right - sProto.cx - 3 - (m_dat->hClientIcon ? 20 : 0);
		if (config) {
			SelectObject(hdc, m_ipConfig.hFonts[IPFONTID_PROTO]);
			SetTextColor(hdc, m_ipConfig.clrs[IPFONTID_PROTO]);
		} else
			SetTextColor(hdc, GetSysColor(COLOR_HOTLIGHT));
		CSkin::RenderText(hdc, m_dat->hThemeIP, szFinalProto, &rc, DT_SINGLELINE | DT_VCENTER, CSkin::m_glowSize);
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

	const TCHAR *tszMode = 0;
	const TCHAR	*tszTopic = 0;
	size_t 		s = 0;
	HFONT 		hOldFont = 0;

	if(si == 0)
		return;

	if(si->ptszStatusbarText) {
		tstring sbarText(si->ptszStatusbarText);
		if((s = sbarText.find_first_of(' ')) != sbarText.npos) {
			sbarText[s] = 0;
			tszMode = sbarText.c_str();
			tszTopic = &sbarText[s + 1];
		}
	}

	::SetBkMode(hdc, TRANSPARENT);

	if(m_height < DEGRADE_THRESHOLD) {
		TCHAR	tszText[256];

		mir_sntprintf(tszText, 256, CTranslator::get(CTranslator::GEN_MUC_TOPIC_IS), tszTopic ? tszTopic :
					  CTranslator::get(CTranslator::GEN_MUC_NO_TOPIC));

		hOldFont = (HFONT)::SelectObject(hdc, m_ipConfig.hFonts[IPFONTID_UIN]);
		::SetTextColor(hdc, m_ipConfig.clrs[IPFONTID_UIN]);
		CSkin::RenderText(hdc, m_dat->hTheme, tszText, &rcItem, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, CSkin::m_glowSize);
	} else {
		SIZE	szNick;

		hOldFont = (HFONT)::SelectObject(hdc, m_ipConfig.hFonts[IPFONTID_NICK]);
		::SetTextColor(hdc, m_ipConfig.clrs[IPFONTID_NICK]);
		::GetTextExtentPoint32(hdc, m_dat->szNickname, lstrlen(m_dat->szNickname), &szNick);
		CSkin::RenderText(hdc, m_dat->hTheme, m_dat->szNickname, &rcItem, DT_SINGLELINE | DT_NOPREFIX | DT_TOP, CSkin::m_glowSize);
		rcItem.left += (szNick.cx + 4);
		::SelectObject(hdc, m_ipConfig.hFonts[IPFONTID_STATUS]);
		::SetTextColor(hdc, m_ipConfig.clrs[IPFONTID_STATUS]);
		if(tszMode)
			CSkin::RenderText(hdc, m_dat->hTheme, tszMode, &rcItem, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX | DT_TOP, CSkin::m_glowSize);
	}
	if (hOldFont)
		SelectObject(hdc, hOldFont);
}

void CInfoPanel::Chat_RenderIPSecondLine(const HDC hdc, RECT& rcItem)
{
	const 	TCHAR *tszMode = 0;
	const 	TCHAR	*tszTopic = 0;
	HFONT 	hOldFont = 0;
	size_t 	s;
	SIZE	szTitle;
	TCHAR	szPrefix[100];

	SESSION_INFO	*si = reinterpret_cast<SESSION_INFO *>(m_dat->si);

	if(si == 0)
		return;

	if(si->ptszStatusbarText) {
		tstring sbarText(si->ptszStatusbarText);
		if((s = sbarText.find_first_of(' ')) != sbarText.npos) {
			sbarText[s] = 0;
			tszMode = sbarText.c_str();
			tszTopic = &sbarText[s + 1];
		}
	}

	hOldFont = (HFONT)::SelectObject(hdc, m_ipConfig.hFonts[IPFONTID_UIN]);
	::SetTextColor(hdc, m_ipConfig.clrs[IPFONTID_UIN]);

	const TCHAR *szTopicTitle = CTranslator::get(CTranslator::GEN_MUC_TOPIC_IS);
	mir_sntprintf(szPrefix, 100, szTopicTitle, _T(""));
	::GetTextExtentPoint32(hdc, szPrefix, lstrlen(szPrefix), &szTitle);

	CSkin::RenderText(hdc, m_dat->hTheme, szPrefix, &rcItem, DT_SINGLELINE | DT_NOPREFIX | DT_TOP);
	rcItem.left += (szTitle.cx + 4);

	if(tszTopic && lstrlen(tszTopic) > 1)
		CSkin::RenderText(hdc, m_dat->hTheme, tszTopic, &rcItem, DT_WORDBREAK | DT_END_ELLIPSIS | DT_NOPREFIX | DT_TOP, CSkin::m_glowSize);
	else
		CSkin::RenderText(hdc, m_dat->hTheme, CTranslator::get(CTranslator::GEN_MUC_NO_TOPIC), &rcItem, DT_TOP| DT_SINGLELINE | DT_NOPREFIX, CSkin::m_glowSize);


	if(hOldFont)
		::SelectObject(hdc, hOldFont);
}

void CInfoPanel::Invalidate() const
{
	RECT	rc;

	GetClientRect(m_dat->hwnd, &rc);
	rc.bottom = m_height;
	::InvalidateRect(m_dat->hwnd, &rc, FALSE);
}

void CInfoPanel::trackMouse(POINT& pt) const
{
	if(!m_active)
		return;

	RECT rc, rcNick;;

	GetWindowRect(GetDlgItem(m_dat->hwnd, IDC_PANELSTATUS), &rc);
	GetWindowRect(GetDlgItem(m_dat->hwnd, IDC_PANELNICK), &rcNick);
	if (PtInRect(&rc, pt)) {
		if (!(m_dat->dwFlagsEx & MWF_SHOW_AWAYMSGTIMER)) {
			if (m_dat->hClientIcon && pt.x >= rc.right - 20)
				SetTimer(m_dat->hwnd, TIMERID_AWAYMSG + 2, 500, 0);
			else
				SetTimer(m_dat->hwnd, TIMERID_AWAYMSG, 1000, 0);
			m_dat->dwFlagsEx |= MWF_SHOW_AWAYMSGTIMER;
		}
		return;
	} else if (PtInRect(&rcNick, pt)) {
		if (!(m_dat->dwFlagsEx & MWF_SHOW_AWAYMSGTIMER)) {
			SetTimer(m_dat->hwnd, TIMERID_AWAYMSG + 1, 1000, 0);
			m_dat->dwFlagsEx |= MWF_SHOW_AWAYMSGTIMER;
		}
		return;
	} else if (IsWindowVisible(m_dat->hwndTip)) {
		GetWindowRect(GetDlgItem(m_dat->hwnd, IDC_PANELSTATUS), &rc);
		if (!PtInRect(&rc, pt))
			SendMessage(m_dat->hwndTip, TTM_TRACKACTIVATE, FALSE, 0);
	}
}

void CInfoPanel::showTip(UINT ctrlId, const LPARAM lParam) const
{
	if (m_dat->hwndTip) {
		RECT 	rc;
		TCHAR 	szTitle[256];
		HWND	hwndDlg = m_dat->hwnd;

		if (ctrlId == 0)
			ctrlId = IDC_PANELSTATUS;

		if (ctrlId == IDC_PANELSTATUS + 1)
			GetWindowRect(GetDlgItem(hwndDlg, IDC_PANELSTATUS), &rc);
		else
			GetWindowRect(GetDlgItem(hwndDlg, ctrlId), &rc);
		SendMessage(m_dat->hwndTip, TTM_TRACKPOSITION, 0, (LPARAM)MAKELONG(rc.left, rc.bottom));
		if (lParam)
			m_dat->ti.lpszText = reinterpret_cast<TCHAR *>(lParam);
		else {
			if (lstrlen(m_dat->statusMsg) > 0)
				m_dat->ti.lpszText = m_dat->statusMsg;
			else
				m_dat->ti.lpszText = PluginConfig.m_szNoStatus;
		}
		SendMessage(m_dat->hwndTip, TTM_UPDATETIPTEXT, 0, (LPARAM)&m_dat->ti);
		SendMessage(m_dat->hwndTip, TTM_SETMAXTIPWIDTH, 0, 350);

		switch (ctrlId) {
			case IDC_PANELNICK: {
				DBVARIANT dbv;

				if (!M->GetTString(m_dat->bIsMeta ? m_dat->hSubContact : m_dat->hContact, m_dat->bIsMeta ? m_dat->szMetaProto : m_dat->szProto, "XStatusName", &dbv)) {
					if (lstrlen(dbv.ptszVal) > 1) {
						_sntprintf(szTitle, safe_sizeof(szTitle), CTranslator::get(CTranslator::GEN_IP_TIP_XSTATUS), m_dat->szNickname, dbv.ptszVal);
						szTitle[safe_sizeof(szTitle) - 1] = 0;
						DBFreeVariant(&dbv);
						break;
					}
					DBFreeVariant(&dbv);
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
		SendMessage(m_dat->hwndTip, TTM_SETTITLE, 1, (LPARAM)szTitle);
		SendMessage(m_dat->hwndTip, TTM_TRACKACTIVATE, TRUE, (LPARAM)&m_dat->ti);
	}
}
