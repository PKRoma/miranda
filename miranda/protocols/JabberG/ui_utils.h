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

#pragma warning(disable:4355)

/////////////////////////////////////////////////////////////////////////////////////////
// Callbacks

struct CCallbackImp
{
public:
	__inline CCallbackImp(): m_object(NULL), m_func(NULL) {}

	__inline CCallbackImp(const CCallbackImp &other): m_object(other.m_object), m_func(other.m_func) {}
	__inline CCallbackImp &operator=(const CCallbackImp &other) { m_object = other.m_object; m_func = other.m_func; return *this; }

	__inline bool operator==(const CCallbackImp &other) const { return (m_object == other.m_object) && (m_func == other.m_func); }
	__inline bool operator!=(const CCallbackImp &other) const { return (m_object != other.m_object) || (m_func != other.m_func); }

	__inline operator bool() const { return m_object && m_func; }

	__inline bool CheckObject(void *object) const { return (object == m_object) ? true : false; }

protected:
	template<typename TClass, typename TArgument>
	__inline CCallbackImp(TClass *object, void (__cdecl TClass::*func)(TArgument *argument)): m_object(object), m_func(*(TFnCallback *)(void *)&func) {}

	__inline void Invoke(void *argument) const { if (m_func && m_object) m_func(m_object, argument); }

private:
	typedef void (__cdecl *TFnCallback)(void *object, void *argument);

	void *m_object;
	TFnCallback m_func;
};

template<typename TArgument>
struct CCallback: public CCallbackImp
{
public:
	__inline CCallback() {}

	template<typename TClass>
	__inline CCallback(TClass *object, void (__cdecl TClass::*func)(TArgument *argument)): CCallbackImp(object, func) {}

	__inline void operator()(TArgument *argument) const { Invoke((void *)argument); }
};

template<typename TClass, typename TArgument>
__inline CCallback<TArgument> Callback(TClass *object, void (__cdecl TClass::*func)(TArgument *argument))
	{ return CCallback<TArgument>(object, func); }

/////////////////////////////////////////////////////////////////////////////////////////
// CDbLink

class CDlgBase;

class CDbLink
{
	char *m_szModule;
	char *m_szSetting;
	BYTE m_type;

	DWORD m_iDefault;
	TCHAR *m_szDefault;

	DBVARIANT dbv;

public:
	CDbLink(const char *szModule, const char *szSetting, BYTE type, DWORD iValue);
	CDbLink(const char *szModule, const char *szSetting, BYTE type, TCHAR *szValue);
	~CDbLink();

	__inline BYTE GetDataType() { return m_type; }

	DWORD LoadInt();
	void SaveInt(DWORD value);

	TCHAR *LoadText();
	void SaveText(TCHAR *value);
};

/////////////////////////////////////////////////////////////////////////////////////////
// CCtrlBase

class CCtrlBase
{
	friend class CDlgBase;

public:
	CCtrlBase(CDlgBase *wnd, int idCtrl );
	virtual ~CCtrlBase() { Unsubclass(); }

	__inline HWND GetHwnd() const { return m_hwnd; }
	__inline CDlgBase *GetParent() { return m_parentWnd; }

	void Enable( int bIsEnable = true );
	__inline void Disable() { Enable( false ); }
	BOOL Enabled( void ) const;

	LRESULT SendMsg( UINT Msg, WPARAM wParam, LPARAM lParam );

	void SetText(const TCHAR *text);
	void SetTextA(const char *text);
	void SetInt(int value);

	TCHAR *GetText();
	char *GetTextA();

	TCHAR *GetText(TCHAR *buf, int size);
	char *GetTextA(char *buf, int size);

	int GetInt();

	virtual BOOL OnCommand(HWND hwndCtrl, WORD idCtrl, WORD idCode) { return FALSE; }
	virtual BOOL OnNotify(int idCtrl, NMHDR *pnmh) { return FALSE; }

	virtual BOOL OnMeasureItem(MEASUREITEMSTRUCT *param) { return FALSE; }
	virtual BOOL OnDrawItem(DRAWITEMSTRUCT *param) { return FALSE; }
	virtual BOOL OnDeleteItem(DELETEITEMSTRUCT *param) { return FALSE; }

	virtual void OnInit();
	virtual void OnDestroy();

	virtual void OnApply() {}
	virtual void OnReset() {}

	static int cmp(const CCtrlBase *c1, const CCtrlBase *c2)
	{
		if (c1->m_idCtrl < c2->m_idCtrl) return -1;
		if (c1->m_idCtrl > c2->m_idCtrl) return +1;
		return 0;
	}

protected:
	HWND m_hwnd;
	int m_idCtrl;
	CCtrlBase* m_next;
	CDlgBase* m_parentWnd;

	virtual LRESULT CustomWndProc(UINT msg, WPARAM wParam, LPARAM lParam);
	void Subclass();
	void Unsubclass();

private:
	WNDPROC m_wndproc;
	static LRESULT CALLBACK GlobalSubclassWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (CCtrlBase *ctrl = (CCtrlBase*)GetWindowLong(hwnd, GWL_USERDATA))
			if (ctrl)
				return ctrl->CustomWndProc(msg, wParam, lParam);

		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
};

/////////////////////////////////////////////////////////////////////////////////////////
// CCtrlButton

class CCtrlButton : public CCtrlBase
{
public:
	CCtrlButton( CDlgBase* dlg, int ctrlId );

	template<class TDlg>
	CCtrlButton( TDlg* dlg, int ctrlId, void (__cdecl TDlg::*pfnOnClick)(CCtrlButton *argument) );

	virtual BOOL OnCommand(HWND hwndCtrl, WORD idCtrl, WORD idCode);

	CCallback<CCtrlButton> OnClick;
};

template<class TDlg>
CCtrlButton::CCtrlButton( TDlg* dlg, int ctrlId, void (__cdecl TDlg::*pfnOnClick)(CCtrlButton *argument) ) :
	CCtrlBase( dlg, ctrlId ),
	OnClick( dlg, pfnOnClick )
{
}

class CCtrlMButton : public CCtrlButton
{
public:
	CCtrlMButton( CDlgBase* dlg, int ctrlId, HICON hIcon, const char* tooltip );
	CCtrlMButton( CDlgBase* dlg, int ctrlId, int iCoreIcon, const char* tooltip );

	template<class TDlg>
	CCtrlMButton( TDlg* dlg, int ctrlId, HICON hIcon, const char* tooltip, void (__cdecl TDlg::*pfnOnClick)(CCtrlButton *argument) );

	template<class TDlg>
	CCtrlMButton( TDlg* dlg, int ctrlId, int iCoreIcon, const char* tooltip, void (__cdecl TDlg::*pfnOnClick)(CCtrlButton *argument) );

	void MakeFlat();
	void MakePush();

	virtual void OnInit();

protected:
	HICON m_hIcon;
	const char* m_toolTip;
};

template<class TDlg>
CCtrlMButton::CCtrlMButton( TDlg* dlg, int ctrlId, HICON hIcon, const char* tooltip, void (__cdecl TDlg::*pfnOnClick)(CCtrlButton *argument) ) :
	CCtrlButton( dlg, ctrlId, pfnOnClick ),
	m_hIcon( hIcon ),
	m_toolTip( tooltip )
{
}

template<class TDlg>
CCtrlMButton::CCtrlMButton( TDlg* dlg, int ctrlId, int iCoreIcon, const char* tooltip, void (__cdecl TDlg::*pfnOnClick)(CCtrlButton *argument) ) :
	CCtrlButton( dlg, ctrlId, pfnOnClick ),
	m_hIcon( LoadSkinnedIcon(iCoreIcon) ),
	m_toolTip( tooltip )
{
}

class CCtrlHyperlink : public CCtrlBase
{
public:
	CCtrlHyperlink( CDlgBase* dlg, int ctrlId, const char* url );

	virtual BOOL OnCommand(HWND hwndCtrl, WORD idCtrl, WORD idCode);

protected:
	const char* m_url;
};

/////////////////////////////////////////////////////////////////////////////////////////
// CCtrlClc
class CCtrlClc: public CCtrlBase
{
public:
	CCtrlClc( CDlgBase* dlg, int ctrlId );

	void AddContact(HANDLE hContact);
	void AddGroup(HANDLE hGroup);
	void AutoRebuild();
	void DeleteItem(HANDLE hItem);
	void EditLabel(HANDLE hItem);
	void EndEditLabel(bool save);
	void EnsureVisible(HANDLE hItem, bool partialOk);
	void Expand(HANDLE hItem, DWORD flags);
	HANDLE FindContact(HANDLE hContact);
	HANDLE FindGroup(HANDLE hGroup);
	COLORREF GetBkColor();
	bool GetCheck(HANDLE hItem);
	int GetCount();
	HWND GetEditControl();
	DWORD GetExpand(HANDLE hItem);
	int GetExtraColumns();
	BYTE GetExtraImage(HANDLE hItem, int iColumn);
	HIMAGELIST GetExtraImageList();
	HFONT GetFont(int iFontId);
	HANDLE GetSelection();
	HANDLE HitTest(int x, int y, DWORD *hitTest);
	void SelectItem(HANDLE hItem);
	void SetBkBitmap(DWORD mode, HBITMAP hBitmap);
	void SetBkColor(COLORREF clBack);
	void SetCheck(HANDLE hItem, bool check);
	void SetExtraColumns(int iColumns);
	void SetExtraImage(HANDLE hItem, int iColumn, int iImage);
	void SetExtraImageList(HIMAGELIST hImgList);
	void SetFont(int iFontId, HANDLE hFont, bool bRedraw);
	void SetIndent(int iIndent);
	void SetItemText(HANDLE hItem, char *szText);
	void SetHideEmptyGroups(bool state);
	void SetGreyoutFlags(DWORD flags);
	bool GetHideOfflineRoot();
	void SetHideOfflineRoot(bool state);
	void SetUseGroups(bool state);
	void SetOfflineModes(DWORD modes);
	DWORD GetExStyle();
	void SetExStyle(DWORD exStyle);
	int GetLefrMargin();
	void SetLeftMargin(int iMargin);
	HANDLE AddInfoItem(CLCINFOITEM *cii);
	int GetItemType(HANDLE hItem);
	HANDLE GetNextItem(HANDLE hItem, DWORD flags);
	COLORREF GetTextColot(int iFontId);
	void SetTextColor(int iFontId, COLORREF clText);

	struct TEventInfo
	{
		CCtrlClc		*ctrl;
		NMCLISTCONTROL	*info;
	};

	CCallback<TEventInfo>	OnExpanded;
	CCallback<TEventInfo>	OnListRebuilt;
	CCallback<TEventInfo>	OnItemChecked;
	CCallback<TEventInfo>	OnDragging;
	CCallback<TEventInfo>	OnDropped;
	CCallback<TEventInfo>	OnListSizeChange;
	CCallback<TEventInfo>	OnOptionsChanged;
	CCallback<TEventInfo>	OnDragStop;
	CCallback<TEventInfo>	OnNewContact;
	CCallback<TEventInfo>	OnContactMoved;
	CCallback<TEventInfo>	OnCheckChanged;
	CCallback<TEventInfo>	OnClick;

protected:
	BOOL OnNotify(int idCtrl, NMHDR *pnmh);
};

/////////////////////////////////////////////////////////////////////////////////////////
// CCtrlData - data access controls base class

class CCtrlData : public CCtrlBase
{
public:
	CCtrlData( CDlgBase* dlg, int ctrlId );

	virtual ~CCtrlData()
	{
		if (m_dbLink) delete m_dbLink;
	}

	__inline bool IsChanged() const { return m_changed; }

	void CreateDbLink( const char* szModuleName, const char* szSetting, BYTE type, DWORD iValue );
	void CreateDbLink( const char* szModuleName, const char* szSetting, TCHAR* szValue );

	virtual void OnInit();

	// Events
	CCallback<CCtrlData> OnChange;

protected:
	CDbLink *m_dbLink;
	bool m_changed;

	void NotifyChange();

	__inline BYTE GetDataType() { return m_dbLink ? m_dbLink->GetDataType() : DBVT_DELETED; }
	__inline DWORD LoadInt() { return m_dbLink ? m_dbLink->LoadInt() : 0; }
	__inline void SaveInt(DWORD value) { if (m_dbLink) m_dbLink->SaveInt(value); }
	__inline TCHAR *LoadText() { return m_dbLink ? m_dbLink->LoadText() : _T(""); }
	__inline void SaveText(TCHAR *value) { if (m_dbLink) m_dbLink->SaveText(value); }
};

/////////////////////////////////////////////////////////////////////////////////////////
// CCtrlCheck

class CCtrlCheck : public CCtrlData
{
public:
	CCtrlCheck( CDlgBase* dlg, int ctrlId );
	virtual BOOL OnCommand(HWND hwndCtrl, WORD idCtrl, WORD idCode) { NotifyChange(); return TRUE; }
	virtual void OnInit()
	{
		CCtrlData::OnInit();
		OnReset();
	}
	virtual void OnApply()
	{
		SaveInt(GetState());
	}
	virtual void OnReset()
	{
		SetState(LoadInt());
	}

	int GetState();
	void SetState(int state);
};

/////////////////////////////////////////////////////////////////////////////////////////
// CCtrlEdit

class CCtrlEdit : public CCtrlData
{
public:
	CCtrlEdit( CDlgBase* dlg, int ctrlId );
	virtual BOOL OnCommand(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{
		if (idCode == EN_CHANGE)
			NotifyChange();
		return TRUE;
	}
	virtual void OnInit()
	{
		CCtrlData::OnInit();
		OnReset();
	}
	virtual void OnApply()
	{
		if (GetDataType() == DBVT_TCHAR)
		{
			int len = GetWindowTextLength(m_hwnd) + 1;
			TCHAR *buf = (TCHAR *)_alloca(sizeof(TCHAR) * len);
			GetWindowText(m_hwnd, buf, len);
			SaveText(buf);
		} 
		else if (GetDataType() != DBVT_DELETED)
		{
			SaveInt(GetInt());
		}
	}
	virtual void OnReset()
	{
		if (GetDataType() == DBVT_TCHAR)
			SetText(LoadText());
		else if (GetDataType() != DBVT_DELETED)
			SetInt(LoadInt());
	}
};

/////////////////////////////////////////////////////////////////////////////////////////
// CCtrlListBox

class CCtrlListBox : public CCtrlBase
{
public:
	CCtrlListBox( CDlgBase* dlg, int ctrlId );

	int    AddString(TCHAR *text, LPARAM data=0);
	void   DeleteString(int index);
	int    FindString(TCHAR *str, int index = -1, bool exact = false);
	int    GetCount();
	int    GetCurSel();
	LPARAM GetItemData(int index);
	TCHAR* GetItemText(int index);
	TCHAR* GetItemText(int index, TCHAR *buf, int size);
	bool   GetSel(int index);
	int    GetSelCount();
	int*   GetSelItems(int *items, int count);
	int*   GetSelItems();
	int    InsertString(TCHAR *text, int pos, LPARAM data=0);
	void   ResetContent();
	int    SelectString(TCHAR *str);
	int    SetCurSel(int index);
	void   SetItemData(int index, LPARAM data);
	void   SetSel(int index, bool sel=true);

	// Events
	CCallback<CCtrlListBox>	OnDblClick;
	CCallback<CCtrlListBox>	OnSelCancel;
	CCallback<CCtrlListBox>	OnSelChange;

protected:
	BOOL OnCommand(HWND hwndCtrl, WORD idCtrl, WORD idCode);
};

/////////////////////////////////////////////////////////////////////////////////////////
// CCtrlCombo

class CCtrlCombo : public CCtrlData
{
public:
	CCtrlCombo( CDlgBase* dlg, int ctrlId );

	virtual BOOL OnCommand(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{
		switch (idCode)
		{
			case CBN_CLOSEUP:  OnCloseup(this);  break;
			case CBN_DROPDOWN: OnDropdown(this); break;

			case CBN_EDITCHANGE:
			case CBN_EDITUPDATE:
			case CBN_SELCHANGE:
			case CBN_SELENDOK:
				NotifyChange();
				break;
		}
		return TRUE;
	}

	virtual void OnInit()
	{
		CCtrlData::OnInit();
		OnReset();
	}
	virtual void OnApply()
	{
		if (GetDataType() == DBVT_TCHAR)
		{
			int len = GetWindowTextLength(m_hwnd) + 1;
			TCHAR *buf = (TCHAR *)_alloca(sizeof(TCHAR) * len);
			GetWindowText(m_hwnd, buf, len);
			SaveText(buf);
		}
		else if (GetDataType() != DBVT_DELETED)
		{
			SaveInt(GetInt());
		}
	}
	virtual void OnReset()
	{
		if (GetDataType() == DBVT_TCHAR)
			SetText(LoadText());
		else if (GetDataType() != DBVT_DELETED)
			SetInt(LoadInt());
	}

	// Control interface
	int    AddString(TCHAR *text, LPARAM data = 0 );
	int    AddStringA(char *text, LPARAM data = 0 );
	void   DeleteString(int index);
	int    FindString(TCHAR *str, int index = -1, bool exact = false);
	int    FindStringA(char *str, int index = -1, bool exact = false);
	int    GetCount();
	int    GetCurSel();
	bool   GetDroppedState();
	LPARAM GetItemData(int index);
	TCHAR* GetItemText(int index);
	TCHAR* GetItemText(int index, TCHAR *buf, int size);
	int    InsertString(TCHAR *text, int pos, LPARAM data=0);
	void   ResetContent();
	int    SelectString(TCHAR *str);
	int    SetCurSel(int index);
	void   SetItemData(int index, LPARAM data);
	void   ShowDropdown(bool show = true);

	// Events
	CCallback<CCtrlCombo>	OnCloseup;
	CCallback<CCtrlCombo>	OnDropdown;
};

/////////////////////////////////////////////////////////////////////////////////////////
// CCtrlCustom

template<typename TDlg>
class CCtrlCustom : public CCtrlBase
{
private:
	void (TDlg::*m_pfnOnCommand)(HWND hwndCtrl, WORD idCtrl, WORD idCode);
	void (TDlg::*m_pfnOnNotify)(int idCtrl, NMHDR *pnmh);
	void (TDlg::*m_pfnOnMeasureItem)(MEASUREITEMSTRUCT *param);
	void (TDlg::*m_pfnOnDrawItem)(DRAWITEMSTRUCT *param);
	void (TDlg::*m_pfnOnDeleteItem)(DELETEITEMSTRUCT *param);

public:
	CCtrlCustom(TDlg *wnd, int idCtrl,
		void (TDlg::*pfnOnCommand)(HWND hwndCtrl, WORD idCtrl, WORD idCode),
		void (TDlg::*pfnOnNotify)(int idCtrl, NMHDR *pnmh),
		void (TDlg::*pfnOnMeasureItem)(MEASUREITEMSTRUCT *param) = NULL,
		void (TDlg::*pfnOnDrawItem)(DRAWITEMSTRUCT *param) = NULL,
		void (TDlg::*pfnOnDeleteItem)(DELETEITEMSTRUCT *param) = NULL): CCtrlBase(wnd, idCtrl)
	{
		m_pfnOnCommand		= pfnOnCommand;
		m_pfnOnNotify		= pfnOnNotify;
		m_pfnOnMeasureItem	= pfnOnMeasureItem;
		m_pfnOnDrawItem		= pfnOnDrawItem;
		m_pfnOnDeleteItem	= pfnOnDeleteItem;
	}

	virtual BOOL OnCommand(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{
		if (m_parentWnd && m_pfnOnCommand) {
			m_parentWnd->m_lresult = 0;
			(((TDlg *)m_parentWnd)->*m_pfnOnCommand)(hwndCtrl, idCtrl, idCode);
			return m_parentWnd->m_lresult;
		}
		return FALSE;
	}
	virtual BOOL OnNotify(int idCtrl, NMHDR *pnmh)
	{
		if (m_parentWnd && m_pfnOnNotify) {
			m_parentWnd->m_lresult = 0;
			(((TDlg *)m_parentWnd)->*m_pfnOnNotify)(idCtrl, pnmh);
			return m_parentWnd->m_lresult;
		}
		return FALSE;
	}

	virtual BOOL OnMeasureItem(MEASUREITEMSTRUCT *param)
	{
		if (m_parentWnd && m_pfnOnMeasureItem) {
			m_parentWnd->m_lresult = 0;
			(((TDlg *)m_parentWnd)->*m_pfnOnMeasureItem)(param);
			return m_parentWnd->m_lresult;
		}
		return FALSE;
	}
	virtual BOOL OnDrawItem(DRAWITEMSTRUCT *param)
	{
		if (m_parentWnd && m_pfnOnDrawItem) {
			m_parentWnd->m_lresult = 0;
			(((TDlg *)m_parentWnd)->*m_pfnOnDrawItem)(param);
			return m_parentWnd->m_lresult;
		}
		return FALSE;
	}
	virtual BOOL OnDeleteItem(DELETEITEMSTRUCT *param)
	{
		if (m_parentWnd && m_pfnOnDeleteItem) {
			m_parentWnd->m_lresult = 0;
			(((TDlg *)m_parentWnd)->*m_pfnOnDeleteItem)(param);
			return m_parentWnd->m_lresult;
		}
		return FALSE;
	}
};

/////////////////////////////////////////////////////////////////////////////////////////
// CDlgBase - base dialog class

class CDlgBase
{
	friend class CCtrlBase;
	friend class CCtrlData;

public:
	CDlgBase(int idDialog, HWND hwndParent);
	virtual ~CDlgBase();

	// general utilities
	void Show();
	int DoModal();

	__inline HWND GetHwnd() const { return m_hwnd; }
	__inline bool IsInitialized() const { return m_initialized; }
	__inline void Close() { SendMessage(m_hwnd, WM_CLOSE, 0, 0); }
	__inline const MSG *ActiveMessage() const { return &m_msg; }

	// global jabber events
	virtual void OnJabberOffline() {}
	virtual void OnJabberOnline() {}

	// dynamic creation support (mainly to avoid leaks in options)
	struct CreateParam
	{
		CDlgBase *(*create)(void *param);
		void *param;
	};
	static BOOL CALLBACK DynamicDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (msg == WM_INITDIALOG)
		{
			CreateParam *param = (CreateParam *)lParam;
			CDlgBase *wnd = param->create(param->param);
			SetWindowLong(hwnd, DWL_DLGPROC, (LONG)GlobalDlgProc);
			return GlobalDlgProc(hwnd, msg, wParam, (LPARAM)wnd);
		}

		return FALSE;
	}

	LRESULT m_lresult;
protected:
	HWND    m_hwnd;
	HWND    m_hwndParent;
	int     m_idDialog;
	MSG     m_msg;
	bool    m_isModal;
	bool    m_initialized;

	CCtrlBase* m_first;

	// override this handlers to provide custom functionality
	// general messages
	virtual void OnInitDialog() { }
	virtual void OnClose() { }
	virtual void OnDestroy() { }

	// miranda-related stuff
	virtual int Resizer(UTILRESIZECONTROL *urc);
	virtual void OnApply() {}
	virtual void OnReset() {}
	virtual void OnChange(CCtrlBase *ctrl) {}

	// main dialog procedure
	virtual BOOL DlgProc(UINT msg, WPARAM wParam, LPARAM lParam);

	// resister controls
	void AddControl(CCtrlBase *ctrl);

private:
	LIST<CCtrlBase> m_controls;

	void NotifyControls(void (CCtrlBase::*fn)());
	CCtrlBase *FindControl(int idCtrl);

	static BOOL CALLBACK GlobalDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static int GlobalDlgResizer(HWND hwnd, LPARAM lParam, UTILRESIZECONTROL *urc);
};

/////////////////////////////////////////////////////////////////////////////////////////
// CProtoDlgBase

template<typename TProto>
class CProtoDlgBase : public CDlgBase
{
public:
	__inline CProtoDlgBase<TProto>(TProto *proto, int idDialog, HWND parent ) :
		CDlgBase( idDialog, parent ),
		m_proto( proto )
	{
	}

	__inline void CreateLink( CCtrlData& ctrl, char *szSetting, BYTE type, DWORD iValue)
	{
		ctrl.CreateDbLink((( PROTO_INTERFACE* )m_proto)->m_szModuleName, szSetting, type, iValue );
	}
	__inline void CreateLink( CCtrlData& ctrl, const char *szSetting, TCHAR *szValue)
	{
		ctrl.CreateDbLink((( PROTO_INTERFACE* )m_proto)->m_szModuleName, szSetting, szValue );
	}

	__inline TProto *GetProto() { return m_proto; }

protected:
	TProto* m_proto;
};

/////////////////////////////////////////////////////////////////////////////////////////

int UIEmulateBtnClick(HWND hwndDlg, UINT idcButton);
void UIShowControls(HWND hwndDlg, int *idList, int nCmdShow);

#endif // __jabber_ui_utils_h__
