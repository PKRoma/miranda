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

struct CJabberCallbackImp
{
public:
	__forceinline CJabberCallbackImp(): m_object(NULL), m_func(NULL) {}

	__forceinline CJabberCallbackImp(const CJabberCallbackImp &other): m_object(other.m_object), m_func(other.m_func) {}
	__forceinline CJabberCallbackImp &operator=(const CJabberCallbackImp &other) { m_object = other.m_object; m_func = other.m_func; return *this; }

	__forceinline bool operator==(const CJabberCallbackImp &other) const { return (m_object == other.m_object) && (m_func == other.m_func); }
	__forceinline bool operator!=(const CJabberCallbackImp &other) const { return (m_object != other.m_object) || (m_func != other.m_func); }

	__forceinline bool CheckObject(void *object) const { return (object == m_object) ? true : false; }

protected:
	template<typename TClass, typename TArgument>
	__forceinline CJabberCallbackImp(TClass *object, void (__cdecl TClass::*func)(TArgument *argument)): m_object(object), m_func(*(TFnCallback *)(void *)&func) {}

	__forceinline void Invoke(void *argument) const { if (m_func && m_object) m_func(m_object, argument); }

private:
	typedef void (__cdecl *TFnCallback)(void *object, void *argument);

	void *m_object;
	TFnCallback m_func;
};

template<typename TArgument>
struct CJabberCallback: public CJabberCallbackImp
{
public:
	__forceinline CJabberCallback() {}

	template<typename TClass>
	__forceinline CJabberCallback(TClass *object, void (__cdecl TClass::*func)(TArgument *argument)): CJabberCallbackImp(object, func) {}

	__forceinline void operator()(TArgument *argument) const { Invoke((void *)argument); }
};

template<typename TClass, typename TArgument>
__forceinline CJabberCallback<TArgument> JCallback(TClass *object, void (__cdecl TClass::*func)(TArgument *argument))
	{ return CJabberCallback<TArgument>(object, func); }

class CJabberDlgBase;

class CJabberDbLink
{
private:
	char *m_szModule;
	char *m_szSetting;
	BYTE m_type;

	DWORD m_iDefault;
	TCHAR *m_szDefault;

	DBVARIANT dbv;

public:
	CJabberDbLink(char *szModule, char *szSetting, BYTE type, DWORD iValue)
	{
		m_szModule = mir_strdup(szModule);
		m_szSetting = mir_strdup(szSetting);
		m_type = type;
		m_iDefault = iValue;
		m_szDefault = 0;
		dbv.type = DBVT_DELETED;
	}
	CJabberDbLink(char *szModule, char *szSetting, BYTE type, TCHAR *szValue)
	{
		m_szModule = mir_strdup(szModule);
		m_szSetting = mir_strdup(szSetting);
		m_type = type;
		m_szDefault = mir_tstrdup(szValue);
		dbv.type = DBVT_DELETED;
	}
	~CJabberDbLink()
	{
		mir_free(m_szModule);
		mir_free(m_szSetting);
		mir_free(m_szDefault);
		if (dbv.type != DBVT_DELETED) DBFreeVariant(&dbv);
	}

	__forceinline BYTE GetDataType() { return m_type; }

	DWORD LoadInt()
	{
		switch (m_type)
		{
			case DBVT_BYTE:		return DBGetContactSettingByte(NULL, m_szModule, m_szSetting, m_iDefault);
			case DBVT_WORD:		return DBGetContactSettingWord(NULL, m_szModule, m_szSetting, m_iDefault);
			case DBVT_DWORD:	return DBGetContactSettingDword(NULL, m_szModule, m_szSetting, m_iDefault);
			default:			return m_iDefault;
		}
	}
	void SaveInt(DWORD value)
	{
		switch (m_type)
		{
			case DBVT_BYTE:		DBWriteContactSettingByte(NULL, m_szModule, m_szSetting, (BYTE)value); break;
			case DBVT_WORD:		DBWriteContactSettingWord(NULL, m_szModule, m_szSetting, (WORD)value); break;
			case DBVT_DWORD:	DBWriteContactSettingDword(NULL, m_szModule, m_szSetting, value); break;
		}
	}
	TCHAR *LoadText()
	{
		if (dbv.type != DBVT_DELETED) DBFreeVariant(&dbv);
		if (!DBGetContactSettingTString(NULL, m_szModule, m_szSetting, &dbv))
		{
			if (dbv.type == DBVT_TCHAR)
				return dbv.ptszVal;
			return m_szDefault;
		}

		dbv.type = DBVT_DELETED;
		return m_szDefault;
	}
	void SaveText(TCHAR *value)
	{
		DBWriteContactSettingTString(NULL, m_szModule, m_szSetting, value);
	}
};

class CJabberCtrlBase
{
public:
	CJabberCtrlBase(CJabberDlgBase *wnd = NULL, int idCtrl = 0): m_wnd(wnd), m_idCtrl(idCtrl) {}
	virtual ~CJabberCtrlBase() {}

	inline CJabberDlgBase *GetParent() { return m_wnd; }

	virtual BOOL OnCommand(HWND hwndCtrl, WORD idCtrl, WORD idCode) { return FALSE; }
	virtual BOOL OnNotify(int idCtrl, NMHDR *pnmh) { return FALSE; }

	virtual BOOL OnMesaureItem(MEASUREITEMSTRUCT *param) { return FALSE; }
	virtual BOOL OnDrawItem(DRAWITEMSTRUCT *param) { return FALSE; }
	virtual BOOL OnDeleteItem(DELETEITEMSTRUCT *param) { return FALSE; }

	virtual void OnInit() {}
	virtual void OnApply() {}
	virtual void OnReset() {}
	virtual void OnDestroy() {}

	static int cmp(const CJabberCtrlBase *c1, const CJabberCtrlBase *c2)
	{
		if (c1->m_idCtrl < c2->m_idCtrl) return -1;
		if (c1->m_idCtrl > c2->m_idCtrl) return +1;
		return 0;
	}

protected:
	int m_idCtrl;
	CJabberDlgBase *m_wnd;
};

class CJabberCtrlData: public CJabberCtrlBase
{
public:
	CJabberCtrlData(): m_dbLink(NULL), m_wndproc(0) {}
	bool Initialize(CJabberDlgBase *wnd, int idCtrl, CJabberDbLink *dbLink = NULL, CJabberCallback<CJabberCtrlData> onChange = CJabberCallback<CJabberCtrlData>());

	virtual ~CJabberCtrlData()
	{
		Unsubclass();
		if (m_dbLink) delete m_dbLink;
	}

	inline HWND GetHwnd() const { return m_hwnd; }

	bool IsChanged() const { return m_changed; }

	void Enable(bool enable=true) { EnableWindow(m_hwnd, enable ? TRUE : FALSE); }
	void Disable() { Enable(false); }

	void SetText(TCHAR *text) { SetWindowText(m_hwnd, text); }
	void SetInt(int value)
	{
		TCHAR buf[32] = {0};
		mir_sntprintf(buf, SIZEOF(buf), _T("%d"), value);
		SetWindowText(m_hwnd, buf);
	}

	TCHAR *GetText()
	{
		int length = GetWindowTextLength(m_hwnd) + 1;
		TCHAR *result = (TCHAR *)mir_alloc(length * sizeof(TCHAR));
		GetWindowText(m_hwnd, result, length);
		return result;
	}
	char *GetTextA()
	{
#ifdef UNICODE
		int length = GetWindowTextLength(m_hwnd) + 1;
		char *result = (char *)mir_alloc(length * sizeof(char));
		GetWindowTextA(m_hwnd, result, length);
		return result;
#else
		return GetText();
#endif
	}

	TCHAR *GetText(TCHAR *buf, int size)
	{
		GetWindowText(m_hwnd, buf, size);
		buf[size-1] = 0;
		return buf;
	}
	char *GetTextA(char *buf, int size)
	{
#ifdef UNICODE
		GetWindowTextA(m_hwnd, buf, size);
		buf[size-1] = 0;
		return buf;
#else
		return GetText(buf, size);
#endif
	}

	int GetInt()
	{
		int length = GetWindowTextLength(m_hwnd) + 1;
		TCHAR *result = (TCHAR *)_alloca(length * sizeof(TCHAR));
		GetWindowText(m_hwnd, result, length);
		return _ttoi(result);
	}

	virtual void OnInit();
	virtual void OnDestroy() { Unsubclass(); m_hwnd = 0; }

	// Events
	CJabberCallback<CJabberCtrlData> OnChange;

protected:
	HWND m_hwnd;
	CJabberDbLink *m_dbLink;
	bool m_changed;

	void NotifyChange();

	__forceinline BYTE GetDataType() { return m_dbLink ? m_dbLink->GetDataType() : DBVT_DELETED; }
	__forceinline DWORD LoadInt() { return m_dbLink ? m_dbLink->LoadInt() : 0; }
	__forceinline void SaveInt(DWORD value) { if (m_dbLink) m_dbLink->SaveInt(value); }
	__forceinline TCHAR *LoadText() { return m_dbLink ? m_dbLink->LoadText() : _T(""); }
	__forceinline void SaveText(TCHAR *value) { if (m_dbLink) m_dbLink->SaveText(value); }

	virtual LRESULT CustomWndProc(UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (msg == WM_DESTROY) Unsubclass();
		return CallWindowProc(m_wndproc, m_hwnd, msg, wParam, lParam);
	}
	void Subclass()
	{
		SetWindowLong(m_hwnd, GWL_USERDATA, (LONG)this);
		m_wndproc = (WNDPROC)SetWindowLong(m_hwnd, GWL_WNDPROC, (LONG)GlobalSubclassWndProc);
	}
	void Unsubclass()
	{
		if (m_wndproc)
		{
			SetWindowLong(m_hwnd, GWL_WNDPROC, (LONG)m_wndproc);
			SetWindowLong(m_hwnd, GWL_USERDATA, (LONG)0);
			m_wndproc = 0;
		}
	}

private:
	WNDPROC m_wndproc;
	static LRESULT CALLBACK GlobalSubclassWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (CJabberCtrlData *ctrl = (CJabberCtrlData *)GetWindowLong(hwnd, GWL_USERDATA))
			if (ctrl) return ctrl->CustomWndProc(msg, wParam, lParam);
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
};

class CJabberCtrlCheck: public CJabberCtrlData
{
public:
	CJabberCtrlCheck() {}
	virtual BOOL OnCommand(HWND hwndCtrl, WORD idCtrl, WORD idCode) { NotifyChange(); return TRUE; }
	virtual void OnInit()
	{
		CJabberCtrlData::OnInit();
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

	int GetState() { return SendMessage(m_hwnd, BM_GETCHECK, 0, 0); }
	void SetState(int state) { SendMessage(m_hwnd, BM_SETCHECK, state, 0); }
};

class CJabberCtrlEdit: public CJabberCtrlData
{
public:
	CJabberCtrlEdit() {}
	virtual BOOL OnCommand(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{
		if (idCode == EN_CHANGE)
			NotifyChange();
		return TRUE;
	}
	virtual void OnInit()
	{
		CJabberCtrlData::OnInit();
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
		} else
		if (GetDataType() != DBVT_DELETED)
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

class CJabberCtrlListBox: public CJabberCtrlData
{
public:
	virtual void COMPILE_LOCK() = 0;

	int AddString(TCHAR *text, LPARAM data=0)
	{
		int iItem = SendMessage(m_hwnd, LB_ADDSTRING, 0, (LPARAM)text);
		SendMessage(m_hwnd, LB_SETITEMDATA, iItem, data);
		return iItem;
	}
	void DeleteString(int index) { SendMessage(m_hwnd, LB_DELETESTRING, index, 0); }
	int FindString(TCHAR *str, int index = -1, bool exact = false) { return SendMessage(m_hwnd, exact?LB_FINDSTRINGEXACT:LB_FINDSTRING, index, (LPARAM)str); }
	int GetCount() { return SendMessage(m_hwnd, LB_GETCOUNT, 0, 0); }
	int GetCurSel() { return SendMessage(m_hwnd, LB_GETCURSEL, 0, 0); }
	LPARAM GetItemData(int index) { return SendMessage(m_hwnd, LB_GETITEMDATA, index, 0); }
	TCHAR *GetItemText(int index)
	{
		TCHAR *result = (TCHAR *)mir_alloc(sizeof(TCHAR) * (SendMessage(m_hwnd, LB_GETTEXTLEN, index, 0) + 1));
		SendMessage(m_hwnd, LB_GETTEXT, index, (LPARAM)result);
		return result;
	}
	TCHAR *GetItemText(int index, TCHAR *buf, int size)
	{
		TCHAR *result = (TCHAR *)_alloca(sizeof(TCHAR) * (SendMessage(m_hwnd, LB_GETTEXTLEN, index, 0) + 1));
		SendMessage(m_hwnd, LB_GETTEXT, index, (LPARAM)result);
		lstrcpyn(buf, result, size);
		return buf;
	}
	bool GetSel(int index) { return SendMessage(m_hwnd, LB_GETSEL, index, 0) ? true : false; }
	int GetSelCount() { return SendMessage(m_hwnd, LB_GETSELCOUNT, 0, 0); }
	int *GetSelItems(int *items, int count)
	{
		SendMessage(m_hwnd, LB_GETSELITEMS, count, (LPARAM)items);
		return items;
	}
	int *GetSelItems()
	{
		int count = GetSelCount() + 1;
		int *result = (int *)mir_alloc(sizeof(int) * count);
		SendMessage(m_hwnd, LB_GETSELITEMS, count, (LPARAM)result);
		result[count-1] = -1;
		return result;
	}
	int InsertString(TCHAR *text, int pos, LPARAM data=0)
	{
		int iItem = SendMessage(m_hwnd, CB_INSERTSTRING, pos, (LPARAM)text);
		SendMessage(m_hwnd, CB_SETITEMDATA, iItem, data);
		return iItem;
	}
	void ResetContent() { SendMessage(m_hwnd, LB_RESETCONTENT, 0, 0); }
	int SelectString(TCHAR *str) { return SendMessage(m_hwnd, LB_SELECTSTRING, 0, (LPARAM)str); }
	int SetCurSel(int index) { return SendMessage(m_hwnd, LB_SETCURSEL, index, 0); }
	void SetItemData(int index, LPARAM data) { SendMessage(m_hwnd, LB_SETITEMDATA, index, data); }
	void SetSel(int index, bool sel=true) { SendMessage(m_hwnd, LB_SETSEL, sel ? TRUE : FALSE, index); }
};

class CJabberCtrlCombo: public CJabberCtrlData
{
public:
	virtual BOOL OnCommand(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{
		switch (idCode)
		{
			case CBN_CLOSEUP:		OnCloseup(this);	break;
			case CBN_DROPDOWN:		OnDropdown(this);	break;

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
		CJabberCtrlData::OnInit();
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
		} else
		if (GetDataType() != DBVT_DELETED)
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
	int AddString(TCHAR *text, LPARAM data=0)
	{
		int iItem = SendMessage(m_hwnd, CB_ADDSTRING, 0, (LPARAM)text);
		SendMessage(m_hwnd, CB_SETITEMDATA, iItem, data);
		return iItem;
	}
	void DeleteString(int index) { SendMessage(m_hwnd, CB_DELETESTRING, index, 0); }
	int FindString(TCHAR *str, int index = -1, bool exact = false) { return SendMessage(m_hwnd, exact?CB_FINDSTRINGEXACT:CB_FINDSTRING, index, (LPARAM)str); }
	int GetCount() { return SendMessage(m_hwnd, CB_GETCOUNT, 0, 0); }
	int GetCurSel() { return SendMessage(m_hwnd, CB_GETCURSEL, 0, 0); }
	bool GetDroppedState() { return SendMessage(m_hwnd, CB_GETDROPPEDSTATE, 0, 0) ? true : false; }
	LPARAM GetItemData(int index) { return SendMessage(m_hwnd, CB_GETITEMDATA, index, 0); }
	TCHAR *GetItemText(int index)
	{
		TCHAR *result = (TCHAR *)mir_alloc(sizeof(TCHAR) * (SendMessage(m_hwnd, CB_GETLBTEXTLEN, index, 0) + 1));
		SendMessage(m_hwnd, CB_GETLBTEXT, index, (LPARAM)result);
		return result;
	}
	TCHAR *GetItemText(int index, TCHAR *buf, int size)
	{
		TCHAR *result = (TCHAR *)_alloca(sizeof(TCHAR) * (SendMessage(m_hwnd, CB_GETLBTEXTLEN, index, 0) + 1));
		SendMessage(m_hwnd, CB_GETLBTEXT, index, (LPARAM)result);
		lstrcpyn(buf, result, size);
		return buf;
	}
	int InsertString(TCHAR *text, int pos, LPARAM data=0)
	{
		int iItem = SendMessage(m_hwnd, CB_INSERTSTRING, pos, (LPARAM)text);
		SendMessage(m_hwnd, CB_SETITEMDATA, iItem, data);
		return iItem;
	}
	void ResetContent() { SendMessage(m_hwnd, CB_RESETCONTENT, 0, 0); }
	int SelectString(TCHAR *str) { return SendMessage(m_hwnd, CB_SELECTSTRING, 0, (LPARAM)str); }
	int SetCurSel(int index) { return SendMessage(m_hwnd, CB_SETCURSEL, index, 0); }
	void SetItemData(int index, LPARAM data) { SendMessage(m_hwnd, CB_SETITEMDATA, index, data); }
	void ShowDropdown(bool show = true) { SendMessage(m_hwnd, CB_SHOWDROPDOWN, show ? TRUE : FALSE, 0); }

	// Events
	CJabberCallback<CJabberCtrlCombo>	OnCloseup;
	CJabberCallback<CJabberCtrlCombo>	OnDropdown;
};

template<typename TDlg>
class CJabberCtrlCustom: public CJabberCtrlBase
{
private:
	BOOL (TDlg::*m_pfnOnCommand)(HWND hwndCtrl, WORD idCtrl, WORD idCode);
	BOOL (TDlg::*m_pfnOnNotify)(int idCtrl, NMHDR *pnmh);
	BOOL (TDlg::*m_pfnOnMeasureItem)(MEASUREITEMSTRUCT *param);
	BOOL (TDlg::*m_pfnOnDrawItem)(DRAWITEMSTRUCT *param);
	BOOL (TDlg::*m_pfnOnDeleteItem)(DELETEITEMSTRUCT *param);

public:
	CJabberCtrlCustom(TDlg *wnd, int idCtrl,
		BOOL (TDlg::*pfnOnCommand)(HWND hwndCtrl, WORD idCtrl, WORD idCode),
		BOOL (TDlg::*pfnOnNotify)(int idCtrl, NMHDR *pnmh),
		BOOL (TDlg::*pfnOnMeasureItem)(MEASUREITEMSTRUCT *param) = NULL,
		BOOL (TDlg::*pfnOnDrawItem)(DRAWITEMSTRUCT *param) = NULL,
		BOOL (TDlg::*pfnOnDeleteItem)(DELETEITEMSTRUCT *param) = NULL): CJabberCtrlBase(wnd, idCtrl)
	{
		m_pfnOnCommand		= pfnOnCommand;
		m_pfnOnNotify		= pfnOnNotify;
		m_pfnOnMeasureItem	= pfnOnMeasureItem;
		m_pfnOnDrawItem		= pfnOnDrawItem;
		m_pfnOnDeleteItem	= pfnOnDeleteItem;
	}

	virtual BOOL OnCommand(HWND hwndCtrl, WORD idCtrl, WORD idCode)
	{
		return (m_wnd && m_pfnOnCommand) ? ((((TDlg *)m_wnd)->*m_pfnOnCommand)(hwndCtrl, idCtrl, idCode)) : FALSE;
	}
	virtual BOOL OnNotify(int idCtrl, NMHDR *pnmh)
	{
		return (m_wnd && m_pfnOnNotify) ? ((((TDlg *)m_wnd)->*m_pfnOnNotify)(idCtrl, pnmh)) : FALSE;
	}

	virtual BOOL OnMesaureItem(MEASUREITEMSTRUCT *param)
	{
		return (m_wnd && m_pfnOnMeasureItem) ? ((((TDlg *)m_wnd)->*m_pfnOnMeasureItem)(param)) : FALSE;
	}
	virtual BOOL OnDrawItem(DRAWITEMSTRUCT *param)
	{
		return (m_wnd && m_pfnOnDrawItem) ? ((((TDlg *)m_wnd)->*m_pfnOnDrawItem)(param)) : FALSE;
	}
	virtual BOOL OnDeleteItem(DELETEITEMSTRUCT *param)
	{
		return (m_wnd && m_pfnOnDeleteItem) ? ((((TDlg *)m_wnd)->*m_pfnOnDeleteItem)(param)) : FALSE;
	}
};

// Base dialog class
class CJabberDlgBase
{
	friend class CJabberCtrlData;

public:
	CJabberDlgBase(CJabberProto *proto, int idDialog, HWND hwndParent);
	virtual ~CJabberDlgBase();

	// general utilities
	void Show();
	int DoModal();
	CJabberProto *GetProto() { return m_proto; }
	inline HWND GetHwnd() const { return m_hwnd; }
	bool IsInitialized() const { return m_initialized; }
	inline void Close() { SendMessage(m_hwnd, WM_CLOSE, 0, 0); }
	inline const MSG *ActiveMessage() const { return &m_msg; }

	// global jabber events
	virtual void OnJabberOffline() {}
	virtual void OnJabberOnline() {}

	// dynamic creation support (mainly to avoid leaks in options)
	struct CreateParam
	{
		CJabberDlgBase *(*create)(void *param);
		void *param;
	};
	static BOOL CALLBACK DynamicDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (msg == WM_INITDIALOG)
		{
			CreateParam *param = (CreateParam *)lParam;
			CJabberDlgBase *wnd = param->create(param->param);
			SetWindowLong(hwnd, DWL_DLGPROC, (LONG)GlobalDlgProc);
			return GlobalDlgProc(hwnd, msg, wParam, (LPARAM)wnd);
		}

		return FALSE;
	}

protected:
	CJabberProto	*m_proto;
	HWND			m_hwnd;
	HWND			m_hwndParent;
	int				m_idDialog;
	MSG				m_msg;
	bool			m_isModal;
	bool			m_initialized;

	// override this handlers to provide custom functionality
	// general messages
	virtual bool OnInitDialog() { return false; }
	virtual bool OnClose() { return false; }
	virtual bool OnDestroy() { return false; }

	// miranda-related stuff
	virtual int Resizer(UTILRESIZECONTROL *urc);
	virtual void OnApply() {}
	virtual void OnReset() {}
	virtual void OnChange(CJabberCtrlBase *ctrl) {}

	// main dialog procedure
	virtual BOOL DlgProc(UINT msg, WPARAM wParam, LPARAM lParam);

	// resister controls
	inline void AddControl(CJabberCtrlBase *ctrl) { m_controls.insert(ctrl); }
	inline void ManageControl(CJabberCtrlBase *ctrl) { AddControl(ctrl); m_autocontrols.insert(ctrl); }

	// register handlers for different controls
	template<typename TDlg>
	inline void SetControlHandler(int idCtrl,
			BOOL (TDlg::*pfnOnCommand)(HWND hwndCtrl, WORD idCtrl, WORD idCode))
			{ ManageControl(new CJabberCtrlCustom<TDlg>((TDlg *)this, idCtrl, pfnOnCommand, NULL)); }

	template<typename TDlg>
	inline void SetControlHandler(int idCtrl,
			BOOL (TDlg::*pfnOnNotify)(int idCtrl, NMHDR *pnmh))
			{ ManageControl(new CJabberCtrlCustom<TDlg>((TDlg *)this, idCtrl, NULL, pfnOnNotify)); }

	template<typename TDlg>
	inline void SetControlHandler(int idCtrl,
			BOOL (TDlg::*pfnOnCommand)(HWND hwndCtrl, WORD idCtrl, WORD idCode),
			BOOL (TDlg::*pfnOnNotify)(int idCtrl, NMHDR *pnmh),
			BOOL (TDlg::*pfnOnMeasureItem)(MEASUREITEMSTRUCT *param) = NULL,
			BOOL (TDlg::*pfnOnDrawItem)(DRAWITEMSTRUCT *param) = NULL,
			BOOL (TDlg::*pfnOnDeleteItem)(DELETEITEMSTRUCT *param) = NULL)
			{ ManageControl(new CJabberCtrlCustom<TDlg>((TDlg *)this, idCtrl, pfnOnCommand, pfnOnNotify, pfnOnMeasureItem, pfnOnDrawItem, pfnOnDeleteItem)); }

private:
	LIST<CJabberCtrlBase> m_controls;
	LIST<CJabberCtrlBase> m_autocontrols; // this controls will be automatically destroyed

	inline void NotifyControls(void (CJabberCtrlBase::*fn)())
	{
		for (int i = 0; i < m_controls.getCount(); ++i)
			(m_controls[i]->*fn)();
	}

	inline CJabberCtrlBase *FindControl(int idCtrl)
	{
		CJabberCtrlBase search(NULL, idCtrl);
		return m_controls.find(&search);
	}

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

struct TJabberCtrlInfo
{
	CJabberCtrlData	*ctrl;
	int				idCtrl;
	BYTE			dbType;
	char			*szSetting;

	TCHAR			*szValue;
	DWORD			iValue;
};

void JabberUISetupControls(CJabberDlgBase *wnd, TJabberCtrlInfo *controls, int count);

#endif // __jabber_ui_utils_h__
