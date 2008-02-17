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

	CCtrlButton  m_Ok;
	void __cdecl OnOk( CCtrlButton* );

	virtual void OnInitDialog();
};

struct CCoolIrcDlg : public CProtoDlgBase<CIrcProto>
{
	CCoolIrcDlg( CIrcProto* _pro, int dlgId, HWND parent = NULL );

	virtual BOOL DlgProc(UINT msg, WPARAM wParam, LPARAM lParam);

	virtual void OnInitDialog();
	virtual void OnDestroy();
};

struct CWhoisDlg : public CCoolIrcDlg
{
	CWhoisDlg( CIrcProto* _pro );

	CCtrlCombo  m_InfoNick;
	CCtrlEdit   m_Reply;
	CCtrlBase   m_Caption, m_AwayTime;
	CCtrlBase   m_InfoName, m_InfoId, m_InfoAddress, m_InfoChannels, m_InfoAuth,
		         m_InfoServer, m_InfoAway2, m_InfoOther;
	CCtrlButton m_Ping, m_Version, m_Time, m_userInfo, m_Refresh, m_Query;

	void ShowMessage( const CIrcMessage* );
	void ShowMessageNoUser( const CIrcMessage* );

	void __cdecl OnGo( CCtrlButton* );
	void __cdecl OnQuery( CCtrlButton* );
	void __cdecl OnPing( CCtrlButton* );
	void __cdecl OnUserInfo( CCtrlButton* );
	void __cdecl OnTime( CCtrlButton* );
	void __cdecl OnVersion( CCtrlButton* );

	virtual void OnInitDialog();
	virtual void OnClose();
	virtual void OnDestroy();
};

struct CNickDlg : public CCoolIrcDlg
{
	CNickDlg( CIrcProto* _pro );

	CCtrlCombo   m_Enick;
	CCtrlButton  m_Ok;

	virtual void OnInitDialog();
	virtual void OnDestroy();

	void __cdecl OnOk( CCtrlButton* );
};

struct CListDlg : public CProtoDlgBase<CIrcProto>
{
	CListDlg( CIrcProto* _pro );

	virtual BOOL DlgProc(UINT msg, WPARAM wParam, LPARAM lParam);

	virtual void OnInitDialog();
	virtual void OnDestroy();

	CCtrlButton  m_Join;
	void __cdecl OnJoin( CCtrlButton* );
};

struct CJoinDlg : public CCoolIrcDlg
{
	CJoinDlg( CIrcProto* _pro );

	virtual void OnInitDialog();
	virtual void OnDestroy();

	CCtrlButton  m_Ok;
	void __cdecl OnOk( CCtrlButton* );
};

struct CInitDlg : public CCoolIrcDlg
{
	CInitDlg( CIrcProto* _pro );

	CCtrlButton  m_Ok;
	void __cdecl OnOk( CCtrlButton* );
};

struct CQuickDlg : public CCoolIrcDlg
{
	CQuickDlg( CIrcProto* _pro );

	virtual void OnInitDialog();
	virtual void OnDestroy();

	CCtrlCombo m_serverCombo;
	void __cdecl OnServerCombo( CCtrlData* );

	CCtrlButton  m_Ok;
	void __cdecl OnOk( CCtrlButton* );
};

struct CManagerDlg : public CCoolIrcDlg
{
	CManagerDlg( CIrcProto* _pro );

	CCtrlCheck   m_check1, m_check2, m_check3, m_check4, m_check5, m_check6, m_check7, m_check8, m_check9;
	CCtrlEdit    m_key, m_limit;
	CCtrlCombo   m_topic;
	CCtrlCheck   m_radio1, m_radio2, m_radio3;
	CCtrlMButton m_add, m_edit, m_remove, m_applyTopic, m_applyModes;
	CCtrlListBox m_list;

	virtual void OnInitDialog();
	virtual void OnClose();
	virtual void OnDestroy();

	void __cdecl OnCheck( CCtrlData* );
	void __cdecl OnCheck5( CCtrlData* );
	void __cdecl OnCheck6( CCtrlData* );
	void __cdecl OnRadio( CCtrlData* );

	void __cdecl OnAdd( CCtrlButton* );
	void __cdecl OnEdit( CCtrlButton* );
	void __cdecl OnRemove( CCtrlButton* );

	void __cdecl OnListDblClick( CCtrlListBox* );
	void __cdecl OnChangeList( CCtrlData* );
	void __cdecl OnChangeModes( CCtrlData* );
	void __cdecl OnChangeTopic( CCtrlData* );

	void __cdecl OnApplyModes( CCtrlButton* );
	void __cdecl OnApplyTopic( CCtrlButton* );

	void ApplyQuestion();
	void CloseQuestion();
	void InitManager( int mode, const TCHAR* window );
};

struct CQuestionDlg : public CCoolIrcDlg
{
	CQuestionDlg( CIrcProto* _pro, CManagerDlg* owner = NULL );

	virtual void OnClose();

	CCtrlButton  m_Ok;
	void __cdecl OnOk( CCtrlButton* );

	void Activate();

private:
	CManagerDlg* m_owner;
};
