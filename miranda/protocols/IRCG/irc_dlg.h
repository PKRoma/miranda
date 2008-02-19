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
	virtual int Resizer(UTILRESIZECONTROL *urc);

	CCtrlButton  m_Join;
	void __cdecl OnJoin( CCtrlButton* );

	void UpdateList( void );
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
	void __cdecl OnChangeList( CCtrlListBox* );
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

/////////////////////////////////////////////////////////////////////////////////////////
// options

//---- the first property page: Account -------------------------------------------------

struct CConnectPrefsDlg : public CProtoDlgBase<CIrcProto>
{
	CCtrlCombo   m_serverCombo;
	CCtrlEdit    m_server, m_port, m_port2, m_pass;
	CCtrlMButton m_add, m_edit, m_del;
	CCtrlEdit    m_nick, m_nick2, m_name, m_userID;

	CCtrlCheck   m_ident, m_identTimer;
	CCtrlEdit    m_identSystem, m_identPort;

	CCtrlCheck   m_retry;
	CCtrlEdit    m_retryCount, m_retryWait;

	CCtrlCheck   m_forceVisible, m_rejoinOnKick, m_rejoinChannels, m_disableError,
		          m_address, m_useServer, m_showServer, m_keepAlive, m_autoJoin,
					 m_oldStyle, m_onlineNotif, m_channelAway, m_enableServer;
	CCtrlEdit    m_onlineTimer, m_limit, m_spin1, m_spin2, m_ssl;

	CConnectPrefsDlg( CIrcProto* _pro );

	static CDlgBase* Create( void* param ) { return new CConnectPrefsDlg(( CIrcProto* )param ); }

	virtual void OnInitDialog();
	virtual void OnApply();
	virtual void OnDestroy();

	void __cdecl OnServerCombo( CCtrlData* );
	void __cdecl OnAddServer( CCtrlButton* );
	void __cdecl OnDeleteServer( CCtrlButton* );
	void __cdecl OnEditServer( CCtrlButton* );
	void __cdecl OnStartup( CCtrlData* );
	void __cdecl OnIdent( CCtrlData* );
	void __cdecl OnUseServer( CCtrlData* );
	void __cdecl OnOnlineNotif( CCtrlData* );
	void __cdecl OnChannelAway( CCtrlData* );
	void __cdecl OnRetry( CCtrlData* );
};

struct CAddServerDlg : public CProtoDlgBase<CIrcProto>
{
	CConnectPrefsDlg* m_owner;
	CCtrlButton m_OK;

	CAddServerDlg( CIrcProto* _pro, CConnectPrefsDlg* _owner );

	virtual void OnInitDialog();
	virtual void OnClose();

	void __cdecl OnOk( CCtrlButton* );
};

struct CEditServerDlg : public CProtoDlgBase<CIrcProto>
{
	CCtrlButton m_OK;
	CConnectPrefsDlg* m_owner;

	CEditServerDlg( CIrcProto* _pro, CConnectPrefsDlg* _owner );

	virtual void OnInitDialog();
	virtual void OnClose();

	void __cdecl OnOk( CCtrlButton* );
};

//---- the second property page: DCC/CTCP -----------------------------------------------

struct CCtcpPrefsDlg : public CProtoDlgBase<CIrcProto>
{
	CCtrlCombo m_combo;
	CCtrlCheck m_slow, m_fast, m_disc, m_passive, m_sendNotice, m_enableIP, m_fromServer;
	CCtrlEdit m_ip;
	CCtrlCheck m_radio1, m_radio2, m_radio3;

	CCtcpPrefsDlg( CIrcProto* _pro );

	static CDlgBase* Create( void* param ) { return new CCtcpPrefsDlg(( CIrcProto* )param ); }

	virtual void OnInitDialog();
	virtual void OnApply();

	void __cdecl OnClicked( CCtrlData* );
};

//---- the third property page: Other ---------------------------------------------------

struct COtherPrefsDlg : public CProtoDlgBase<CIrcProto>
{
	CCtrlButton  m_url;
	CCtrlMButton m_add, m_delete;
	CCtrlCombo   m_performCombo, m_codepage;
	CCtrlEdit    m_pertormEdit, m_quitMessage, m_alias;
	CCtrlCheck   m_perform, m_scripting, m_autodetect;

	COtherPrefsDlg( CIrcProto* _pro );

	static CDlgBase* Create( void* param ) { return new COtherPrefsDlg(( CIrcProto* )param ); }

	virtual void OnInitDialog();
	virtual void OnApply();
	virtual void OnDestroy();

	void __cdecl OnUrl( CCtrlButton* );
	void __cdecl OnPerformCombo( CCtrlData* );
	void __cdecl OnCodePage( CCtrlData* );
	void __cdecl OnPerformEdit( CCtrlData* );
	void __cdecl OnPerform( CCtrlData* );
	void __cdecl OnAdd( CCtrlButton* );
	void __cdecl OnDelete( CCtrlButton* );

	void addPerformComboValue( int idx, const char* szValueName );
};

//---- the fourth property page: Ignore -------------------------------------------------

struct CIgnorePrefsDlg : public CProtoDlgBase<CIrcProto>
{
	CCtrlMButton m_add, m_edit, m_del;
	CCtrlCheck m_enable, m_ignoreChat, m_ignoreFile, m_ignoreChannel, m_ignoreUnknown;
	CCtrlBase m_list;  // TODO - convert it to CCtrlListView
	
	CIgnorePrefsDlg( CIrcProto* _pro );

	static CDlgBase* Create( void* param ) { return new CIgnorePrefsDlg(( CIrcProto* )param ); }

	virtual void OnInitDialog();
	virtual void OnDestroy();
	virtual void OnApply();

	virtual BOOL DlgProc(UINT msg, WPARAM wParam, LPARAM lParam);

	void __cdecl OnEnableIgnore( CCtrlData* );
	void __cdecl OnIgnoreChat( CCtrlData* );
	void __cdecl OnAdd( CCtrlButton* );
	void __cdecl OnEdit( CCtrlButton* );
	void __cdecl OnDelete( CCtrlButton* );

	void FixButtons( void );
	void RebuildList( void );
	void UpdateList( void );
};

struct CAddIgnoreDlg : public CProtoDlgBase<CIrcProto>
{
	CCtrlButton m_Ok;
	CIgnorePrefsDlg* m_owner;

	TCHAR szOldMask[500];

	CAddIgnoreDlg( CIrcProto* _pro, const TCHAR* mask, CIgnorePrefsDlg* parent );

	virtual void OnInitDialog();
	virtual void OnClose();

	void __cdecl OnOk( CCtrlButton* );
};
