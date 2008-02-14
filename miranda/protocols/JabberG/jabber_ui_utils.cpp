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

#include "jabber.h"

bool CJabberCtrlData::Initialize(CJabberDlgBase *wnd, int idCtrl, CJabberDbLink *dbLink, CJabberCallback<CJabberCtrlData> onChange)
{
	if (m_wnd) return false;

	m_wnd = wnd;
	m_idCtrl = idCtrl;
	m_dbLink = dbLink;
	OnChange = onChange;
	if (m_wnd) m_wnd->AddControl(this);
	return true;
}

void CJabberCtrlData::OnInit()
{
	m_hwnd = (m_idCtrl && m_wnd && m_wnd->GetHwnd()) ? GetDlgItem(m_wnd->GetHwnd(), m_idCtrl) : NULL;
	m_changed = false;
}

void CJabberCtrlData::NotifyChange()
{
	if (!m_wnd || m_wnd->IsInitialized()) m_changed = true;
	if (m_wnd) m_wnd->OnChange(this);
	OnChange(this);
}


CJabberDlgBase::CJabberDlgBase(CJabberProto *proto, int idDialog, HWND hwndParent): m_controls(1, CJabberCtrlBase::cmp), m_autocontrols(1, CJabberCtrlBase::cmp)
{
	m_proto = proto;
	m_idDialog = idDialog;
	m_hwndParent = hwndParent;
	m_hwnd = NULL;
	m_isModal = false;
	m_initialized = false;
}

CJabberDlgBase::~CJabberDlgBase()
{
	// remove handlers
	m_controls.destroy();

	// destroy automatic handlers
	for (int i = 0; i < m_controls.getCount(); ++i)
		delete m_autocontrols[i];
	m_autocontrols.destroy();

	if (m_hwnd) DestroyWindow(m_hwnd);
}

void CJabberDlgBase::Show()
{
	ShowWindow(CreateDialogParam(hInst, MAKEINTRESOURCE(m_idDialog), m_hwndParent, GlobalDlgProc, (LPARAM)(CJabberDlgBase *)this), SW_SHOW);
}

int CJabberDlgBase::DoModal()
{
	m_isModal = true;
	return DialogBoxParam(hInst, MAKEINTRESOURCE(m_idDialog), m_hwndParent, GlobalDlgProc, (LPARAM)(CJabberDlgBase *)this);
}

int CJabberDlgBase::Resizer(UTILRESIZECONTROL *urc)
{
	return 0;
}

BOOL CJabberDlgBase::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			m_initialized = false;
			TranslateDialogDefault(m_hwnd);
			NotifyControls(&CJabberCtrlBase::OnInit);
			OnInitDialog();
			m_initialized = true;
			return TRUE;
		}

		case WM_MEASUREITEM:
		{
			MEASUREITEMSTRUCT *param = (MEASUREITEMSTRUCT *)lParam;
			if (param && param->CtlID)
				if (CJabberCtrlBase *ctrl = FindControl(param->CtlID))
					return ctrl->OnMesaureItem(param);
			return FALSE;
		}

		case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *param = (DRAWITEMSTRUCT *)lParam;
			if (param && param->CtlID)
				if (CJabberCtrlBase *ctrl = FindControl(param->CtlID))
					return ctrl->OnDrawItem(param);
			return FALSE;
		}

		case WM_DELETEITEM:
		{
			DELETEITEMSTRUCT *param = (DELETEITEMSTRUCT *)lParam;
			if (param && param->CtlID)
				if (CJabberCtrlBase *ctrl = FindControl(param->CtlID))
					return ctrl->OnDeleteItem(param);
			return FALSE;
		}

		case WM_COMMAND:
		{
			HWND hwndCtrl = (HWND)lParam;
			WORD idCtrl = LOWORD(wParam);
			WORD idCode = HIWORD(wParam);
			if (CJabberCtrlBase *ctrl = FindControl(idCtrl))
				return ctrl->OnCommand(hwndCtrl, idCtrl, idCode);
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
					NotifyControls(&CJabberCtrlBase::OnApply);
					OnApply();
				}
				else if (pnmh->code == PSN_RESET)
				{
					NotifyControls(&CJabberCtrlBase::OnReset);
					OnReset();
				}
			}

			if (CJabberCtrlBase *ctrl = FindControl(idCtrl))
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
			if (!OnClose())
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
			NotifyControls(&CJabberCtrlBase::OnDestroy);

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

BOOL CALLBACK CJabberDlgBase::GlobalDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CJabberDlgBase *wnd = NULL;
	if (msg == WM_INITDIALOG)
	{
		SetWindowLong(hwnd, GWL_USERDATA, lParam);
		wnd = (CJabberDlgBase *)lParam;
		wnd->m_hwnd = hwnd;
	} else
	{
		wnd = (CJabberDlgBase *)GetWindowLong(hwnd, GWL_USERDATA);
	}

	if (!wnd) return FALSE;

	wnd->m_msg.hwnd = hwnd;
	wnd->m_msg.message = msg;
	wnd->m_msg.wParam = wParam;
	wnd->m_msg.lParam = lParam;
	return wnd->DlgProc(msg, wParam, lParam);
}

int CJabberDlgBase::GlobalDlgResizer(HWND hwnd, LPARAM lParam, UTILRESIZECONTROL *urc)
{
	CJabberDlgBase *wnd = (CJabberDlgBase *)GetWindowLong(hwnd, GWL_USERDATA);
	if (!wnd) return 0;

	return wnd->Resizer(urc);
}

/////////////////////////////////////////////////////////////////
// Misc utilities
void JabberUISetupMButtons(CJabberProto *proto, HWND hwnd, TMButtonInfo *buttons, int count)
{
	for (int i = 0; i < count; ++i)
	{
		SendDlgItemMessage(hwnd, buttons[i].idCtrl, BM_SETIMAGE, IMAGE_ICON,
			(LPARAM)(buttons[i].szIcon ?
				proto->LoadIconEx(buttons[i].szIcon) :
				LoadSkinnedIcon(buttons[i].iCoreIcon)));
		SendDlgItemMessage(hwnd, buttons[i].idCtrl, BUTTONSETASFLATBTN, 0, 0);
		SendDlgItemMessage(hwnd, buttons[i].idCtrl, BUTTONADDTOOLTIP, (WPARAM)TranslateTS(buttons[i].szTitle), BATF_TCHAR);
		if (buttons[i].bIsPush)
			SendDlgItemMessage(hwnd, buttons[i].idCtrl, BUTTONSETASPUSHBTN, 0, 0);
	}
}

int JabberUIEmulateBtnClick(HWND hwndDlg, UINT idcButton)
{
	if (IsWindowEnabled(GetDlgItem(hwndDlg, idcButton)))
		PostMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(idcButton, BN_CLICKED), (LPARAM)GetDlgItem(hwndDlg, idcButton));
	return 0;
}

void JabberUIShowControls(HWND hwndDlg, int *idList, int nCmdShow)
{
	for (; *idList; ++idList)
		ShowWindow(GetDlgItem(hwndDlg, *idList), nCmdShow);
}


void JabberUISetupControls(CJabberDlgBase *wnd, TJabberCtrlInfo *controls, int count)
{
	for (int i = 0; i < count; ++i)
	{
		CJabberDbLink *dbLink = NULL;
		if (controls[i].dbType == DBVT_TCHAR)
			dbLink = new CJabberDbLink(wnd->GetProto()->m_szProtoName, controls[i].szSetting, controls[i].dbType, controls[i].szValue);
		else if (controls[i].dbType != DBVT_DELETED)
			dbLink = new CJabberDbLink(wnd->GetProto()->m_szProtoName, controls[i].szSetting, controls[i].dbType, controls[i].iValue);

		controls[i].ctrl->Initialize(wnd, controls[i].idCtrl, dbLink );
	}
}
