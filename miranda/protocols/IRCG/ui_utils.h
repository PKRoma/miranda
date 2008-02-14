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

File name      : $URL: https://miranda.svn.sourceforge.net/svnroot/miranda/trunk/miranda/protocols/JabberG/jabber_ui_utils.h $
Revision       : $Revision: 7159 $
Last change on : $Date: 2008-01-24 19:38:24 +0300 (Чт, 24 янв 2008) $
Last change by : $Author: m_mluhov $

*/

#ifndef __jabber_ui_utils_h__
#define __jabber_ui_utils_h__

struct CCallbackImp
{
public:
	__inline CCallbackImp(): m_object(NULL), m_func(NULL) {}

	__inline CCallbackImp(const CCallbackImp &other): m_object(other.m_object), m_func(other.m_func) {}
	__inline CCallbackImp &operator=(const CCallbackImp &other) { m_object = other.m_object; m_func = other.m_func; return *this; }

	__inline bool operator==(const CCallbackImp &other) const { return (m_object == other.m_object) && (m_func == other.m_func); }
	__inline bool operator!=(const CCallbackImp &other) const { return (m_object != other.m_object) || (m_func != other.m_func); }

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
__inline CCallback<TArgument> JCallback(TClass *object, void (__cdecl TClass::*func)(TArgument *argument))
	{ return CCallback<TArgument>(object, func); }

class CDlgBase;

class CDbLink
{
private:
	char *m_szModule;
	char *m_szSetting;
	BYTE m_type;

	DWORD m_iDefault;
	TCHAR *m_szDefault;

	DBVARIANT dbv;

public:
	CDbLink(char *szModule, char *szSetting, BYTE type, DWORD iValue)
	{
		m_szModule = mir_strdup(szModule);
		m_szSetting = mir_strdup(szSetting);
		m_type = type;
		m_iDefault = iValue;
		m_szDefault = 0;
		dbv.type = DBVT_DELETED;
	}
	CDbLink(char *szModule, char *szSetting, BYTE type, TCHAR *szValue)
	{
		m_szModule = mir_strdup(szModule);
		m_szSetting = mir_strdup(szSetting);
		m_type = type;
		m_szDefault = mir_tstrdup(szValue);
		dbv.type = DBVT_DELETED;
	}
	~CDbLink()
	{
		mir_free(m_szModule);
		mir_free(m_szSetting);
		mir_free(m_szDefault);
		if (dbv.type != DBVT_DELETED) DBFreeVariant(&dbv);
	}

	__inline BYTE GetDataType() { return m_type; }

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

class CCtrlBase
{
public:
	CCtrlBase(CDlgBase *wnd = NULL, int idCtrl = 0): m_wnd(wnd), m_idCtrl(idCtrl) {}
	virtual ~CCtrlBase() {}

	__inline CDlgBase *GetParent() { return m_wnd; }

	virtual BOOL OnCommand(HWND hwndCtrl, WORD idCtrl, WORD idCode) { return FALSE; }
	virtual BOOL OnNotify(int idCtrl, NMHDR *pnmh) { return FALSE; }

	virtual BOOL OnMeasureItem(MEASUREITEMSTRUCT *param) { return FALSE; }
	virtual BOOL OnDrawItem(DRAWITEMSTRUCT *param) { return FALSE; }
	virtual BOOL OnDeleteItem(DELETEITEMSTRUCT *param) { return FALSE; }

	virtual void OnInit() {}
	virtual void OnApply() {}
	virtual void OnReset() {}
	virtual void OnDestroy() {}

	static int cmp(const CCtrlBase *c1, const CCtrlBase *c2)
	{
		if (c1->m_idCtrl < c2->m_idCtrl) return -1;
		if (c1->m_idCtrl > c2->m_idCtrl) return +1;
		return 0;
	}

protected:
	int m_idCtrl;
	CDlgBase *m_wnd;
};

class CCtrlData: public CCtrlBase
{
public:
	CCtrlData(): m_dbLink(NULL), m_wndproc(0) {}
	bool Initialize(CDlgBase *wnd, int idCtrl, CDbLink *dbLink = NULL, CCallback<CCtrlData> onChange = CCallback<CCtrlData>());

	virtual ~CCtrlData()
	{
		Unsubclass();
		if (m_dbLink) delete m_dbLink;
	}

	__inline HWND GetHwnd() const { return m_hwnd; }

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
	CCallback<CCtrlData> OnChange;

protected:
	HWND m_hwnd;
	CDbLink *m_dbLink;
	bool m_changed;

	void NotifyChange();

	__inline BYTE GetDataType() { return m_dbLink ? m_dbLink->GetDataType() : DBVT_DELETED; }
	__inline DWORD LoadInt() { return m_dbLink ? m_dbLink->LoadInt() : 0; }
	__inline void SaveInt(DWORD value) { if (m_dbLink) m_dbLink->SaveInt(value); }
	__inline TCHAR *LoadText() { return m_dbLink ? m_dbLink->LoadText() : _T(""); }
	__inline void SaveText(TCHAR *value) { if (m_dbLink) m_dbLink->SaveText(value); }

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
		if (CCtrlData *ctrl = (CCtrlData *)GetWindowLong(hwnd, GWL_USERDATA))
			if (ctrl) return ctrl->CustomWndProc(msg, wParam, lParam);
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
};

class CCtrlCheck: public CCtrlData
{
public:
	CCtrlCheck() {}
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

	int GetState() { return SendMessage(m_hwnd, BM_GETCHECK, 0, 0); }
	void SetState(int state) { SendMessage(m_hwnd, BM_SETCHECK, state, 0); }
};

class CCtrlEdit: public CCtrlData
{
public:
	CCtrlEdit() {}
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

class CCtrlListBox: public CCtrlData
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

class CCtrlCombo: public CCtrlData
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
	CCallback<CCtrlCombo>	OnCloseup;
	CCallback<CCtrlCombo>	OnDropdown;
};

template<typename TDlg>
class CCtrlCustom: public CCtrlBase
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
		if (m_wnd && m_pfnOnCommand) {
			m_wnd->m_lresult = 0;
			(((TDlg *)m_wnd)->*m_pfnOnCommand)(hwndCtrl, idCtrl, idCode);
			return m_wnd->m_lresult;
		}
		return FALSE;
	}
	virtual BOOL OnNotify(int idCtrl, NMHDR *pnmh)
	{
		if (m_wnd && m_pfnOnNotify) {
			m_wnd->m_lresult = 0;
			(((TDlg *)m_wnd)->*m_pfnOnNotify)(idCtrl, pnmh);
			return m_wnd->m_lresult;
		}
		return FALSE;
	}

	virtual BOOL OnMeasureItem(MEASUREITEMSTRUCT *param)
	{
		if (m_wnd && m_pfnOnMeasureItem) {
			m_wnd->m_lresult = 0;
			(((TDlg *)m_wnd)->*m_pfnOnMeasureItem)(param);
			return m_wnd->m_lresult;
		}
		return FALSE;
	}
	virtual BOOL OnDrawItem(DRAWITEMSTRUCT *param)
	{
		if (m_wnd && m_pfnOnDrawItem) {
			m_wnd->m_lresult = 0;
			(((TDlg *)m_wnd)->*m_pfnOnDrawItem)(param);
			return m_wnd->m_lresult;
		}
		return FALSE;
	}
	virtual BOOL OnDeleteItem(DELETEITEMSTRUCT *param)
	{
		if (m_wnd && m_pfnOnDeleteItem) {
			m_wnd->m_lresult = 0;
			(((TDlg *)m_wnd)->*m_pfnOnDeleteItem)(param);
			return m_wnd->m_lresult;
		}
		return FALSE;
	}
};

/////////////////////////////////////////////////////////////////////////////////////////
// Base dialog class

class CDlgBase
{
	friend class CCtrlData;
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
	__inline void AddControl(CCtrlBase *ctrl) { m_controls.insert(ctrl); }
	__inline void ManageControl(CCtrlBase *ctrl) { AddControl(ctrl); m_autocontrols.insert(ctrl); }

	// register handlers for different controls
	template<typename TDlg>
	__inline void SetControlHandler(int idCtrl,
			void (TDlg::*pfnOnCommand)(HWND hwndCtrl, WORD idCtrl, WORD idCode))
			{ ManageControl(new CCtrlCustom<TDlg>((TDlg *)this, idCtrl, pfnOnCommand, NULL)); }

	template<typename TDlg>
	__inline void SetControlHandler(int idCtrl,
			void (TDlg::*pfnOnNotify)(int idCtrl, NMHDR *pnmh))
			{ ManageControl(new CCtrlCustom<TDlg>((TDlg *)this, idCtrl, NULL, pfnOnNotify)); }

	template<typename TDlg>
	__inline void SetControlHandler(int idCtrl,
			void (TDlg::*pfnOnCommand)(HWND hwndCtrl, WORD idCtrl, WORD idCode),
			void (TDlg::*pfnOnNotify)(int idCtrl, NMHDR *pnmh),
			void (TDlg::*pfnOnMeasureItem)(MEASUREITEMSTRUCT *param) = NULL,
			void (TDlg::*pfnOnDrawItem)(DRAWITEMSTRUCT *param) = NULL,
			void (TDlg::*pfnOnDeleteItem)(DELETEITEMSTRUCT *param) = NULL)
			{ ManageControl(new CCtrlCustom<TDlg>((TDlg *)this, idCtrl, pfnOnCommand, pfnOnNotify, pfnOnMeasureItem, pfnOnDrawItem, pfnOnDeleteItem)); }

private:
	LIST<CCtrlBase> m_controls;
	LIST<CCtrlBase> m_autocontrols; // this controls will be automatically destroyed

	__inline void NotifyControls(void (CCtrlBase::*fn)())
	{
		for (int i = 0; i < m_controls.getCount(); ++i)
			(m_controls[i]->*fn)();
	}

	__inline CCtrlBase *FindControl(int idCtrl)
	{
		CCtrlBase search(NULL, idCtrl);
		return m_controls.find(&search);
	}

	static BOOL CALLBACK GlobalDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static int GlobalDlgResizer(HWND hwnd, LPARAM lParam, UTILRESIZECONTROL *urc);
};

template<typename TProto>
class CProtoDlgBase : public CDlgBase
{
public:
	__inline CProtoDlgBase<TProto>(TProto *proto, int idDialog, HWND parent ) :
		CDlgBase( idDialog, parent ),
		m_proto( proto )
	{
	}

	__inline TProto *GetProto() { return m_proto; }

protected:
	TProto* m_proto;
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

template<typename TProto>
void UISetupMButtons(TProto *proto, HWND hwnd, TMButtonInfo *buttons, int count)
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

int UIEmulateBtnClick(HWND hwndDlg, UINT idcButton);
void UIShowControls(HWND hwndDlg, int *idList, int nCmdShow);

struct TCtrlInfo
{
	CCtrlData	*ctrl;
	int				idCtrl;
	BYTE			dbType;
	char			*szSetting;

	TCHAR			*szValue;
	DWORD			iValue;
};

template<typename TProto>
void UISetupControls(CProtoDlgBase<TProto>* wnd, TCtrlInfo *controls, int count)
{
	for (int i = 0; i < count; ++i)
	{
		CDbLink *dbLink = NULL;
		if (controls[i].dbType == DBVT_TCHAR)
			dbLink = new CDbLink(wnd->GetProto()->m_szProtoName, controls[i].szSetting, controls[i].dbType, controls[i].szValue);
		else if (controls[i].dbType != DBVT_DELETED)
			dbLink = new CDbLink(wnd->GetProto()->m_szProtoName, controls[i].szSetting, controls[i].dbType, controls[i].iValue);

		controls[i].ctrl->Initialize(wnd, controls[i].idCtrl, dbLink );
	}
}

#endif // __jabber_ui_utils_h__
