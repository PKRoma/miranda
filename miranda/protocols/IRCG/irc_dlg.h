/*
IRC plugin for Miranda IM

Copyright (C) 2003-05 Jurgen Persson
Copyright (C) 2007-08 George Hazan

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
*/

#include "ui_utils.h"

/////////////////////////////////////////////////////////////////////////////////////////
// Dialogs

struct CMessageBoxDlg : public CProtoDlgBase<CIrcProto>
{
	DCCINFO* pdci;

	CMessageBoxDlg( CIrcProto* _pro, DCCINFO* _dci );

	void OnCommand_Yes(HWND hwndCtrl, WORD idCtrl, WORD idCode);
	void OnCommand_No(HWND hwndCtrl, WORD idCtrl, WORD idCode);

	virtual void OnInitDialog();
};

struct CWhoisDlg : public CProtoDlgBase<CIrcProto>
{
	CWhoisDlg( CIrcProto* _pro );

	void OnOk(HWND hwndCtrl, WORD idCtrl, WORD idCode);
	void OnGo(HWND hwndCtrl, WORD idCtrl, WORD idCode);
	void OnQuery(HWND hwndCtrl, WORD idCtrl, WORD idCode);
	void OnPing(HWND hwndCtrl, WORD idCtrl, WORD idCode);
	void OnUserInfo(HWND hwndCtrl, WORD idCtrl, WORD idCode);
	void OnTime(HWND hwndCtrl, WORD idCtrl, WORD idCode);
	void OnVersion(HWND hwndCtrl, WORD idCtrl, WORD idCode);

	virtual BOOL DlgProc(UINT msg, WPARAM wParam, LPARAM lParam);

	virtual void OnInitDialog();
	virtual void OnClose();
	virtual void OnDestroy();
};

struct CNickDlg : public CProtoDlgBase<CIrcProto>
{
	CNickDlg( CIrcProto* _pro );

	virtual BOOL DlgProc(UINT msg, WPARAM wParam, LPARAM lParam);

	virtual void OnInitDialog();
	virtual void OnDestroy();

	void OnCommand_Yes(HWND hwndCtrl, WORD idCtrl, WORD idCode);
	void OnCommand_No(HWND hwndCtrl, WORD idCtrl, WORD idCode);
};

struct CListDlg : public CProtoDlgBase<CIrcProto>
{
	CListDlg( CIrcProto* _pro );

	virtual BOOL DlgProc(UINT msg, WPARAM wParam, LPARAM lParam);

	virtual void OnInitDialog();
	virtual void OnDestroy();

	void OnJoin(HWND hwndCtrl, WORD idCtrl, WORD idCode);
};

struct CJoinDlg : public CProtoDlgBase<CIrcProto>
{
	CJoinDlg( CIrcProto* _pro );

	virtual BOOL DlgProc(UINT msg, WPARAM wParam, LPARAM lParam);

	virtual void OnInitDialog();
	virtual void OnDestroy();

	void OnCommand_Yes(HWND hwndCtrl, WORD idCtrl, WORD idCode);
	void OnCommand_No(HWND hwndCtrl, WORD idCtrl, WORD idCode);
};

struct CInitDlg : public CProtoDlgBase<CIrcProto>
{
	CInitDlg( CIrcProto* _pro );

	virtual BOOL DlgProc(UINT msg, WPARAM wParam, LPARAM lParam);

	virtual void OnInitDialog();
	virtual void OnDestroy();

	void OnCommand_Yes(HWND hwndCtrl, WORD idCtrl, WORD idCode);
	void OnCommand_No(HWND hwndCtrl, WORD idCtrl, WORD idCode);
};

struct CQuickDlg : public CProtoDlgBase<CIrcProto>
{
	CQuickDlg( CIrcProto* _pro );

	virtual BOOL DlgProc(UINT msg, WPARAM wParam, LPARAM lParam);

	virtual void OnInitDialog();
	virtual void OnDestroy();

	void OnCommand_Yes(HWND hwndCtrl, WORD idCtrl, WORD idCode);
	void OnServerCombo(HWND hwndCtrl, WORD idCtrl, WORD idCode);
};

struct CQuestionDlg : public CProtoDlgBase<CIrcProto>
{
	CQuestionDlg( CIrcProto* _pro, HWND parent );

	virtual BOOL DlgProc(UINT msg, WPARAM wParam, LPARAM lParam);

	virtual void OnInitDialog();
	virtual void OnClose();
	virtual void OnDestroy();

	void OnCommand_Yes(HWND hwndCtrl, WORD idCtrl, WORD idCode);
};

struct CManagerDlg : public CProtoDlgBase<CIrcProto>
{
	CManagerDlg( CIrcProto* _pro );

	virtual BOOL DlgProc(UINT msg, WPARAM wParam, LPARAM lParam);

	virtual void OnInitDialog();
	virtual void OnClose();
	virtual void OnDestroy();

	void OnAdd(HWND hwndCtrl, WORD idCtrl, WORD idCode);
	void OnApplyModes(HWND hwndCtrl, WORD idCtrl, WORD idCode);
	void OnApplyTopic(HWND hwndCtrl, WORD idCtrl, WORD idCode);
	void OnCommand_Yes(HWND hwndCtrl, WORD idCtrl, WORD idCode);
	void OnChange(HWND hwndCtrl, WORD idCtrl, WORD idCode);
	void OnCheck(HWND hwndCtrl, WORD idCtrl, WORD idCode);
	void OnCheck5(HWND hwndCtrl, WORD idCtrl, WORD idCode);
	void OnCheck6(HWND hwndCtrl, WORD idCtrl, WORD idCode);
	void OnEdit(HWND hwndCtrl, WORD idCtrl, WORD idCode);
	void OnList(HWND hwndCtrl, WORD idCtrl, WORD idCode);
	void OnRadio(HWND hwndCtrl, WORD idCtrl, WORD idCode);
	void OnRemove(HWND hwndCtrl, WORD idCtrl, WORD idCode);
};
