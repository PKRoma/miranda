/*

Object UI extensions
Copyright (C) 2008  Victor Pavlychko, George Hazan

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

File name      : $URL: https://miranda.svn.sourceforge.net/svnroot/miranda/trunk/miranda/protocols/JabberG/jabber_ui_utils.cpp $
Revision       : $Revision: 7159 $
Last change on : $Date: 2008-01-24 19:38:24 +0300 (Чт, 24 янв 2008) $
Last change by : $Author: m_mluhov $

*/

#include "irc.h"
#include "ui_utils.h"

extern HINSTANCE hInst;

bool CCtrlData::Initialize(CDlgBase *wnd, int idCtrl, CDbLink *dbLink, CCallback<CCtrlData> onChange)
{
	if (m_wnd) return false;

	m_wnd = wnd;
	m_idCtrl = idCtrl;
	m_dbLink = dbLink;
	OnChange = onChange;
	if (m_wnd) m_wnd->AddControl(this);
	return true;
}

void CCtrlData::OnInit()
{
	m_hwnd = (m_idCtrl && m_wnd && m_wnd->GetHwnd()) ? GetDlgItem(m_wnd->GetHwnd(), m_idCtrl) : NULL;
	m_changed = false;
}

void CCtrlData::NotifyChange()
{
	if (!m_wnd || m_wnd->IsInitialized()) m_changed = true;
	if (m_wnd) m_wnd->OnChange(this);
	OnChange(this);
}


CDlgBase::CDlgBase(int idDialog, HWND hwndParent) :
	m_controls(1, CCtrlBase::cmp),
	m_autocontrols(1, CCtrlBase::cmp)
{
	m_idDialog = idDialog;
	m_hwndParent = hwndParent;
	m_hwnd = NULL;
	m_isModal = false;
	m_initialized = false;
}

CDlgBase::~CDlgBase()
{
	// remove handlers
	m_controls.destroy();

	// destroy automatic handlers
	for (int i = 0; i < m_controls.getCount(); ++i)
		delete m_autocontrols[i];
	m_autocontrols.destroy();

	if (m_hwnd) DestroyWindow(m_hwnd);
}

void CDlgBase::Show()
{
	ShowWindow(CreateDialogParam(hInst, MAKEINTRESOURCE(m_idDialog), m_hwndParent, GlobalDlgProc, (LPARAM)(CDlgBase *)this), SW_SHOW);
}

int CDlgBase::DoModal()
{
	m_isModal = true;
	return DialogBoxParam(hInst, MAKEINTRESOURCE(m_idDialog), m_hwndParent, GlobalDlgProc, (LPARAM)(CDlgBase *)this);
}

int CDlgBase::Resizer(UTILRESIZECONTROL *urc)
{
	return 0;
}

BOOL CDlgBase::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			m_initialized = false;
			TranslateDialogDefault(m_hwnd);
			NotifyControls(&CCtrlBase::OnInit);
			OnInitDialog();
			m_initialized = true;
			return TRUE;
		}

		case WM_MEASUREITEM:
		{
			MEASUREITEMSTRUCT *param = (MEASUREITEMSTRUCT *)lParam;
			if (param && param->CtlID)
				if (CCtrlBase *ctrl = FindControl(param->CtlID))
					return ctrl->OnMeasureItem(param);
			return FALSE;
		}

		case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *param = (DRAWITEMSTRUCT *)lParam;
			if (param && param->CtlID)
				if (CCtrlBase *ctrl = FindControl(param->CtlID))
					return ctrl->OnDrawItem(param);
			return FALSE;
		}

		case WM_DELETEITEM:
		{
			DELETEITEMSTRUCT *param = (DELETEITEMSTRUCT *)lParam;
			if (param && param->CtlID)
				if (CCtrlBase *ctrl = FindControl(param->CtlID))
					return ctrl->OnDeleteItem(param);
			return FALSE;
		}

		case WM_COMMAND:
		{
			HWND hwndCtrl = (HWND)lParam;
			WORD idCtrl = LOWORD(wParam);
			WORD idCode = HIWORD(wParam);
			if (CCtrlBase *ctrl = FindControl(idCtrl))
				return ctrl->OnCommand(hwndCtrl, idCtrl, idCode);

			if ( idCode == CBN_SELCHANGE || idCode == EN_CHANGE )
				SendMessage( GetParent( m_hwnd ), PSM_CHANGED, 0, 0 );
			return FALSE;
		}

		case WM_NOTIFY:
		{
			int idCtrl = wParam;
			NMHDR *pnmh = (NMHDR *)lParam;

			if (pnmh->idFrom == 0)
			{
				if (pnmh->code == PSN_APPLY)
				{
					NotifyControls(&CCtrlBase::OnApply);
					OnApply();
				}
				else if (pnmh->code == PSN_RESET)
				{
					NotifyControls(&CCtrlBase::OnReset);
					OnReset();
				}
			}

			if (CCtrlBase *ctrl = FindControl(idCtrl))
				return ctrl->OnNotify(idCtrl, pnmh);
			return FALSE;
		}

		case WM_SIZE:
		{
			UTILRESIZEDIALOG urd;
			urd.cbSize = sizeof(urd);
			urd.hwndDlg = m_hwnd;
			urd.hInstance = hInst;
			urd.lpTemplate = MAKEINTRESOURCEA(m_idDialog);
			urd.lParam = 0;
			urd.pfnResizer = GlobalDlgResizer;
			CallService(MS_UTILS_RESIZEDIALOG, 0, (LPARAM)&urd);
			return TRUE;
		}

		case WM_CLOSE:
		{
			m_lresult = FALSE;
			OnClose();
			if ( !m_lresult )
			{
				if (m_isModal)
					EndDialog(m_hwnd, 0);
				else
					DestroyWindow(m_hwnd);
			}
			return TRUE;
		}

		case WM_DESTROY:
		{
			OnDestroy();
			NotifyControls(&CCtrlBase::OnDestroy);

			SetWindowLong(m_hwnd, GWL_USERDATA, 0);
			m_hwnd = NULL;
			if (m_isModal)
			{
				m_isModal = false;
			} else
			{	// modeless dialogs MUST be allocated with 'new'
				delete this;
			}
			return TRUE;
		}
	}

	return FALSE;
}

BOOL CALLBACK CDlgBase::GlobalDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CDlgBase *wnd = NULL;
	if (msg == WM_INITDIALOG)
	{
		SetWindowLong(hwnd, GWL_USERDATA, lParam);
		wnd = (CDlgBase *)lParam;
		wnd->m_hwnd = hwnd;
	} else
	{
		wnd = (CDlgBase *)GetWindowLong(hwnd, GWL_USERDATA);
	}

	if (!wnd) return FALSE;

	wnd->m_msg.hwnd = hwnd;
	wnd->m_msg.message = msg;
	wnd->m_msg.wParam = wParam;
	wnd->m_msg.lParam = lParam;
	return wnd->DlgProc(msg, wParam, lParam);
}

int CDlgBase::GlobalDlgResizer(HWND hwnd, LPARAM lParam, UTILRESIZECONTROL *urc)
{
	CDlgBase *wnd = (CDlgBase *)GetWindowLong(hwnd, GWL_USERDATA);
	if (!wnd) return 0;

	return wnd->Resizer(urc);
}

/////////////////////////////////////////////////////////////////
// Misc utilities

int UIEmulateBtnClick(HWND hwndDlg, UINT idcButton)
{
	if (IsWindowEnabled(GetDlgItem(hwndDlg, idcButton)))
		PostMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(idcButton, BN_CLICKED), (LPARAM)GetDlgItem(hwndDlg, idcButton));
	return 0;
}

void UIShowControls(HWND hwndDlg, int *idList, int nCmdShow)
{
	for (; *idList; ++idList)
		ShowWindow(GetDlgItem(hwndDlg, *idList), nCmdShow);
}
