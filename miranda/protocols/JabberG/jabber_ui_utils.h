/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-08  George Hazan
Copyright ( C ) 2007-08  Maxim Mluhov
Copyright ( C ) 2007-08  Victor Pavlychko

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or ( at your option ) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

File name      : $URL$
Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#ifndef __jabber_ui_utils_h__
#define __jabber_ui_utils_h__

struct CJabberCtrlBase
{
	int m_idCtrl;

	virtual ~CJabberCtrlBase() {}
	virtual BOOL OnCommand(HWND hwndCtrl, WORD idCtrl, WORD idCode) { return FALSE; }
	virtual BOOL OnNotify(int idCtrl, NMHDR *pnmh) { return FALSE; }
	virtual void OnApply() {}
	virtual void OnReset() {}

	static int cmp(const CJabberCtrlBase *c1, const CJabberCtrlBase *c2)
	{
		if (c1->m_idCtrl < c2->m_idCtrl) return -1;
		if (c1->m_idCtrl > c2->m_idCtrl) return +1;
		return 0;
	}
};

template<typename TDlg>
class CJabberCtrlCustom: public CJabberCtrlBase
{
private:
	TDlg *m_wnd;
	BOOL (TDlg::*m_pfnOnCommand)(HWND hwndCtrl, WORD idCtrl, WORD idCode);
	BOOL (TDlg::*m_pfnOnNotify)(int idCtrl, NMHDR *pnmh);

public:
	CJabberCtrlCustom(TDlg *wnd, int idCtrl,
		BOOL (TDlg::*pfnOnCommand)(HWND hwndCtrl, WORD idCtrl, WORD idCode),
		BOOL (TDlg::*pfnOnNotify)(int idCtrl, NMHDR *pnmh))
	{
		m_wnd = wnd;
		m_idCtrl = idCtrl;
		m_pfnOnCommand = pfnOnCommand;
		m_pfnOnNotify = pfnOnNotify;
	}
	virtual BOOL OnCommand(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{
		return (m_wnd && m_pfnOnCommand) ? ((m_wnd->*m_pfnOnCommand)(hwndCtrl, idCtrl, idCode)) : FALSE;
	}
	virtual BOOL OnNotify(int idCtrl, NMHDR *pnmh)
	{
		return (m_wnd && m_pfnOnNotify) ? ((m_wnd->*m_pfnOnNotify)(idCtrl, pnmh)) : FALSE;
	}
};

// Base dialog class
class CJabberDlgBase
{
public:
	CJabberDlgBase(CJabberProto *proto, int idDialog, HWND hwndParent);
	virtual ~CJabberDlgBase();

	// general utilities
	void Show();
	int DoModal();
	HWND GetHwnd() const { return m_hwnd; }
	void Close() { SendMessage(m_hwnd, WM_CLOSE, 0, 0); }

	// global jabber events
	virtual void OnJabberOffline() {}
	virtual void OnJabberOnline() {}

protected:
	CJabberProto	*m_proto;
	HWND			m_hwnd;
	HWND			m_hwndParent;
	int				m_idDialog;
	MSG				m_msg;
	bool			m_isModal;

	// override this handlers to provide custom functionality
	// general messages
	virtual bool OnInitDialog() { return false; }
	virtual bool OnClose() { return false; }
	virtual bool OnDestroy() { return false; }

	// miranda-related stuff
	virtual int Resizer(UTILRESIZECONTROL *urc);
	virtual void OnApply() {}
	virtual void OnReset() {}

	// main dialog procedure
	virtual BOOL DlgProc(UINT msg, WPARAM wParam, LPARAM lParam);

	// register handlers for different controls
	template<typename TDlg>
	void SetControlHandler(int idCtrl,
			BOOL (TDlg::*pfnOnCommand)(HWND hwndCtrl, WORD idCtrl, WORD idCode))
			{ m_controls.insert(new CJabberCtrlCustom<TDlg>((TDlg *)this, idCtrl, pfnOnCommand, NULL)); }

	template<typename TDlg>
	void SetControlHandler(int idCtrl,
			BOOL (TDlg::*pfnOnNotify)(int idCtrl, NMHDR *pnmh))
			{ m_controls.insert(new CJabberCtrlCustom<TDlg>((TDlg *)this, idCtrl, NULL, pfnOnNotify)); }

	template<typename TDlg>
	void SetControlHandler(int idCtrl,
			BOOL (TDlg::*pfnOnCommand)(HWND hwndCtrl, WORD idCtrl, WORD idCode),
			BOOL (TDlg::*pfnOnNotify)(int idCtrl, NMHDR *pnmh))
			{ m_controls.insert(new CJabberCtrlCustom<TDlg>((TDlg *)this, idCtrl, pfnOnCommand, pfnOnNotify)); }

private:
	LIST<CJabberCtrlBase> m_controls;

	static BOOL CALLBACK GlobalDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static int GlobalDlgResizer(HWND hwnd, LPARAM lParam, UTILRESIZECONTROL *urc);
};

// General utilities
struct TMButtonInfo
{
	int		idCtrl;
	TCHAR	*szTitle;
	char	*szIcon;
	int		iCoreIcon;
	bool	bIsPush;
};

void JabberUISetupMButtons(CJabberProto *proto, HWND hwnd, TMButtonInfo *buttons, int count);
int JabberUIEmulateBtnClick(HWND hwndDlg, UINT idcButton);
void JabberUIShowControls(HWND hwndDlg, int *idList, int nCmdShow);

#endif // __jabber_ui_utils_h__
