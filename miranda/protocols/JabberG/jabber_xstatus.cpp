/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-08  George Hazan
Copyright ( C ) 2007     Maxim Mluhov

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
#include "jabber_caps.h"

#include <m_genmenu.h>
#include <m_icolib.h>
#include <m_fontservice.h>

#include "sdk/m_cluiframes.h"

#include "sdk/m_proto_listeningto.h"
#include "sdk/m_skin_eng.h"

///////////////////////////////////////////////////////////////////////////////
// Simple dialog with timer and ok/cancel buttons
class CJabberDlgPepBase: public CJabberDlgBase
{
	typedef CJabberDlgBase CSuper;
public:
	CJabberDlgPepBase(CJabberProto *proto, CPepService *pepService, int id);

protected:
	CPepService *m_pepService;

	CCtrlButton m_btnOk;
	CCtrlButton m_btnCancel;

	void OnInitDialog();
	int Resizer(UTILRESIZECONTROL *urc);
	virtual BOOL DlgProc(UINT msg, WPARAM wParam, LPARAM lParam);

	void StopTimer();

private:
	int m_time;
};

CJabberDlgPepBase::CJabberDlgPepBase(CJabberProto *proto, CPepService *pepService, int id):
	CJabberDlgBase(proto, id, NULL),
	m_btnOk(this, IDOK),
	m_btnCancel(this, IDCANCEL)
{
}

void CJabberDlgPepBase::OnInitDialog()
{
	CSuper::OnInitDialog();

	m_time = 5;
	SetTimer(m_hwnd, 1, 1000, NULL);

	TCHAR buf[128];
	mir_sntprintf(buf, SIZEOF(buf), _T("%s (%d)"), TranslateT("OK"), m_time);
	m_btnOk.SetText(buf);
}

int CJabberDlgPepBase::Resizer(UTILRESIZECONTROL *urc)
{
	switch (urc->wId)
	{
	case IDOK:
	case IDCANCEL:
		return RD_ANCHORX_RIGHT|RD_ANCHORY_BOTTOM;
	}

	return CSuper::Resizer(urc);
}

BOOL CJabberDlgPepBase::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_TIMER:
		if (wParam = 1)
		{
			TCHAR buf[128];
			mir_sntprintf(buf, SIZEOF(buf), _T("%s (%d)"), TranslateT("OK"), --m_time);
			m_btnOk.SetText(buf);

			if (m_time < 0)
			{
				KillTimer(m_hwnd, 1);
				UIEmulateBtnClick(m_hwnd, IDOK);
			}

			return TRUE;
		}

		break;
	}

	return CSuper::DlgProc(msg, wParam, lParam);
}

void CJabberDlgPepBase::StopTimer()
{
	KillTimer(m_hwnd, 1);
	m_btnOk.SetText(TranslateT("OK"));
}

///////////////////////////////////////////////////////////////////////////////
// Simple PEP status
class CJabberDlgPepSimple: public CJabberDlgPepBase
{
	typedef CJabberDlgPepBase CSuper;
public:
	CJabberDlgPepSimple(CJabberProto *proto, CPepService *pepService, TCHAR *title);
	~CJabberDlgPepSimple();

	bool OkClicked() { return m_bOkClicked; }
	void AddStatusMode(LPARAM id, char *name, HICON hIcon, TCHAR *title, bool subitem = false);
	void SetActiveStatus(LPARAM id, TCHAR *text);
	LPARAM GetStatusMode();
	TCHAR *GetStatusText();

protected:
	CCtrlCombo m_cbModes;
	CCtrlEdit m_txtDescription;

	void OnInitDialog();
	int Resizer(UTILRESIZECONTROL *urc);

	UI_MESSAGE_MAP(CJabberDlgPepSimple, CSuper);
		UI_MESSAGE(WM_MEASUREITEM, OnWmMeasureItem);
		UI_MESSAGE(WM_DRAWITEM, OnWmDrawItem);
		UI_MESSAGE(WM_GETMINMAXINFO, OnWmGetMinMaxInfo);
	UI_MESSAGE_MAP_END();

	BOOL OnWmMeasureItem(UINT msg, WPARAM wParam, LPARAM lParam);
	BOOL OnWmDrawItem(UINT msg, WPARAM wParam, LPARAM lParam);
	BOOL OnWmGetMinMaxInfo(UINT msg, WPARAM wParam, LPARAM lParam);

private:
	struct CStatusMode
	{
		LPARAM m_id;
		char *m_name;
		HICON m_hIcon;
		TCHAR *m_title;
		bool m_subitem;

		CStatusMode(LPARAM id, char *name, HICON hIcon, TCHAR *title, bool subitem): m_id(id), m_name(name), m_hIcon(hIcon), m_title(title), m_subitem(subitem) {}
	};

	OBJLIST<CStatusMode> m_modes;
	TCHAR *m_text;
	TCHAR *m_title;
	int m_time;
	int m_prevSelected;
	int m_selected;
	bool m_bOkClicked;

	LPARAM m_active;
	TCHAR *m_activeText;

	void btnOk_OnClick(CCtrlButton *btn);
	void global_OnChange(CCtrlData *);
	void cbModes_OnChange(CCtrlData *);
};

CJabberDlgPepSimple::CJabberDlgPepSimple(CJabberProto *proto, CPepService *pepService, TCHAR *title):
	CJabberDlgPepBase(proto, pepService, IDD_PEP_SIMPLE),
	m_cbModes(this, IDC_CB_MODES),
	m_txtDescription(this, IDC_TXT_DESCRIPTION),
	m_modes(10),
	m_text(NULL),
	m_selected(0),
	m_prevSelected(-1),
	m_bOkClicked(false),
	m_title(title)
{
	m_btnOk.OnClick = Callback(this, &CJabberDlgPepSimple::btnOk_OnClick);
	m_cbModes.OnChange = Callback(this, &CJabberDlgPepSimple::cbModes_OnChange);
	m_cbModes.OnDropdown = 
	m_txtDescription.OnChange = Callback(this, &CJabberDlgPepSimple::global_OnChange);

	m_modes.insert(new CStatusMode(-1, "<none>", LoadSkinnedIcon(SKINICON_OTHER_SMALLDOT), TranslateT("None"), false));
}

CJabberDlgPepSimple::~CJabberDlgPepSimple()
{
	mir_free(m_text);
}

void CJabberDlgPepSimple::AddStatusMode(LPARAM id, char *name, HICON hIcon, TCHAR *title, bool subitem)
{
	m_modes.insert(new CStatusMode(id, name, hIcon, title, subitem));
}

void CJabberDlgPepSimple::SetActiveStatus(LPARAM id, TCHAR *text)
{
	m_active = id;
	m_activeText = text;
}

LPARAM CJabberDlgPepSimple::GetStatusMode()
{
	return m_modes[m_selected].m_id;
}

TCHAR *CJabberDlgPepSimple::GetStatusText()
{
	return m_text;
}

void CJabberDlgPepSimple::OnInitDialog()
{
	CSuper::OnInitDialog();

	SendMessage(m_hwnd, WM_SETICON, ICON_BIG, (LPARAM)m_proto->LoadIconEx("main"));
	SetWindowText(m_hwnd, m_title);

	m_txtDescription.Enable(false);
	for (int i = 0; i < m_modes.getCount(); ++i)
	{
		int idx = m_cbModes.AddString(m_modes[i].m_title, i);
		if ((m_modes[i].m_id == m_active) || !idx)
		{
			m_cbModes.SetCurSel(idx);
			if (idx) m_txtDescription.Enable();
		}
	}
	if (m_activeText) m_txtDescription.SetText(m_activeText);
}

int CJabberDlgPepSimple::Resizer(UTILRESIZECONTROL *urc)
{
	switch (urc->wId)
	{
	case IDC_CB_MODES:
		return RD_ANCHORX_WIDTH|RD_ANCHORY_TOP;
	case IDC_TXT_DESCRIPTION:
		return RD_ANCHORX_WIDTH|RD_ANCHORY_HEIGHT;
	}

	return CSuper::Resizer(urc);
}

void CJabberDlgPepSimple::btnOk_OnClick(CCtrlButton *btn)
{
	m_text = m_txtDescription.GetText();
	m_selected = m_cbModes.GetCurSel();
	m_bOkClicked = true;
}

void CJabberDlgPepSimple::global_OnChange(CCtrlData *)
{
	StopTimer();
}

void CJabberDlgPepSimple::cbModes_OnChange(CCtrlData *)
{
	StopTimer();

	if (m_prevSelected == m_cbModes.GetCurSel())
		return;

	char szSetting[128];

	if ((m_prevSelected >= 0) && (m_modes[m_cbModes.GetItemData(m_prevSelected)].m_id >= 0))
	{
		TCHAR *txt = m_txtDescription.GetText();
		mir_snprintf(szSetting, SIZEOF(szSetting), "PepMsg_%s", m_modes[m_cbModes.GetItemData(m_prevSelected)].m_name);
		m_proto->JSetStringT(NULL, szSetting, txt);
		mir_free(txt);
	}

	m_prevSelected = m_cbModes.GetCurSel();
	if ((m_prevSelected >= 0) && (m_modes[m_cbModes.GetItemData(m_prevSelected)].m_id >= 0))
	{
		mir_snprintf(szSetting, SIZEOF(szSetting), "PepMsg_%s", m_modes[m_cbModes.GetItemData(m_prevSelected)].m_name);

		DBVARIANT dbv;
		if (!m_proto->JGetStringT(NULL, szSetting, &dbv))
		{
			m_txtDescription.SetText(dbv.ptszVal);
			JFreeVariant(&dbv);
		} else
		{
			m_txtDescription.SetTextA("");
		}
		m_txtDescription.Enable(true);
	} else
	{
		m_txtDescription.SetTextA("");
		m_txtDescription.Enable(false);
	}
}

BOOL CJabberDlgPepSimple::OnWmMeasureItem(UINT msg, WPARAM wParam, LPARAM lParam)
{
	LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT)lParam;
	if (lpmis->CtlID != IDC_CB_MODES)
		return FALSE;

	TEXTMETRIC tm = {0};
	HDC hdc = GetDC(m_cbModes.GetHwnd());
	GetTextMetrics(hdc, &tm);
	ReleaseDC(m_cbModes.GetHwnd(), hdc);

	lpmis->itemHeight = max(tm.tmHeight, 18);
	if (lpmis->itemHeight < 18) lpmis->itemHeight = 18;

	return TRUE;
}

BOOL CJabberDlgPepSimple::OnWmDrawItem(UINT msg, WPARAM wParam, LPARAM lParam)
{
	LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
	if (lpdis->CtlID != IDC_CB_MODES)
		return FALSE;

	if (lpdis->itemData == -1) return FALSE;

	CStatusMode *mode = &m_modes[lpdis->itemData];

	TEXTMETRIC tm = {0};
	GetTextMetrics(lpdis->hDC, &tm);

	SetBkMode(lpdis->hDC, TRANSPARENT);
	if (lpdis->itemState & ODS_SELECTED)
	{
		SetTextColor(lpdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
		FillRect(lpdis->hDC, &lpdis->rcItem, GetSysColorBrush(COLOR_HIGHLIGHT));
	} else
	{
		SetTextColor(lpdis->hDC, GetSysColor(COLOR_WINDOWTEXT));
		FillRect(lpdis->hDC, &lpdis->rcItem, GetSysColorBrush(COLOR_WINDOW));
	}

	if (!mode->m_subitem || (lpdis->itemState & ODS_COMBOBOXEDIT))
	{
		TCHAR text[128];
		if (mode->m_subitem)
		{
			for (int i = lpdis->itemData; i >= 0; --i)
				if (!m_modes[i].m_subitem)
				{
					mir_sntprintf(text, SIZEOF(text), _T("%s [%s]"), m_modes[i].m_title, mode->m_title);
					break;
				}
		} else
		{
			lstrcpyn(text, mode->m_title, SIZEOF(text));
		}

		DrawIconEx(lpdis->hDC, lpdis->rcItem.left+2, (lpdis->rcItem.top+lpdis->rcItem.bottom-16)/2, mode->m_hIcon, 16, 16, 0, NULL, DI_NORMAL);
		TextOut(lpdis->hDC, lpdis->rcItem.left + 23, (lpdis->rcItem.top+lpdis->rcItem.bottom-tm.tmHeight)/2, text, lstrlen(text));
	} else
	{
		TCHAR text[128];
		mir_sntprintf(text, SIZEOF(text), _T("...%s"), mode->m_title);
		TextOut(lpdis->hDC, lpdis->rcItem.left + 30, (lpdis->rcItem.top+lpdis->rcItem.bottom-tm.tmHeight)/2, text, lstrlen(text));
	}

	return TRUE;
}

BOOL CJabberDlgPepSimple::OnWmGetMinMaxInfo(UINT msg, WPARAM wParam, LPARAM lParam)
{
	LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
	lpmmi->ptMinTrackSize.x = 200;
	lpmmi->ptMinTrackSize.y = 200;
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// CPepService base class
CPepService::CPepService(CJabberProto *proto, char *name, TCHAR *node):
	m_proto(proto),
	m_name(name),
	m_node(node)
{
}

CPepService::~CPepService()
{
}

void CPepService::Publish()
{
	XmlNodeIq iq( _T("set"), m_proto->SerialNext());
	CreateData( 
		iq << XCHILDNS( _T("pubsub"), _T(JABBER_FEAT_PUBSUB))
			<< XCHILD( _T("publish")) << XATTR( _T("node"), m_node )
				<< XCHILD( _T("item")) << XATTR( _T("id"), _T("current")));
	m_proto->m_ThreadInfo->send( iq );
}

void CPepService::Retract()
{
	m_proto->m_ThreadInfo->send(
		XmlNodeIq( _T("set"), m_proto->SerialNext())
			<< XCHILDNS( _T("pubsub"), _T(JABBER_FEAT_PUBSUB))
				<< XCHILD( _T("retract")) << XATTR( _T("node"), m_node) << XATTRI( _T("notify"), 1 )
					<< XCHILD( _T("item")) << XATTR( _T("id"), _T("current")));
}

///////////////////////////////////////////////////////////////////////////////
// CPepGuiService base class
CPepGuiService::CPepGuiService(CJabberProto *proto, char *name, TCHAR *node):
	CPepService(proto, name, node),
	m_hMenuItem(NULL),
	m_bGuiOpen(false),
	m_hIcolibItem(NULL),
	m_szText(NULL)
{
}

CPepGuiService::~CPepGuiService()
{
	if (m_hMenuService)
	{
		DestroyServiceFunction(m_hMenuService);
		m_hMenuService = NULL;
	}

	if (m_szText) mir_free(m_szText);
}

void CPepGuiService::InitGui()
{
	char szService[128];
	mir_snprintf(szService, SIZEOF(szService), "%s/AdvStatusSet/%s", m_proto->m_szModuleName, m_name);

	int (__cdecl CPepGuiService::*serviceProc)(WPARAM, LPARAM);
	serviceProc = &CPepGuiService::OnMenuItemClick;
	m_hMenuService = CreateServiceFunctionObj(szService, (MIRANDASERVICEOBJ)*(void **)&serviceProc, this);

	RebuildMenu();
}

void CPepGuiService::RebuildMenu()
{
	if (!m_proto->m_bJabberOnline || !m_proto->m_bPepSupported)
		return;

	char szService[128];
	mir_snprintf(szService, SIZEOF(szService), "%s/AdvStatusSet/%s", m_proto->m_szModuleName, m_name);

	CLISTMENUITEM mi = { 0 };
	mi.cbSize = sizeof(mi);
	mi.ptszPopupName = m_proto->m_tszUserName;
	mi.pszService = szService;
	mi.position = 10000;
	mi.flags = CMIF_TCHAR|CMIF_ICONFROMICOLIB;

	mi.icolibItem = m_hIcolibItem;
	mi.ptszName = m_szText ? m_szText : _T("<advanced status slot>");
	m_hMenuItem = (HANDLE)CallService(MS_CLIST_ADDSTATUSMENUITEM, 0, (LPARAM)&mi);
}

bool CPepGuiService::LaunchSetGui()
{
	if (m_bGuiOpen) return false;

	m_bGuiOpen = true;
	ShowSetDialog();
	m_bGuiOpen = false;

	return true;
}

void CPepGuiService::UpdateMenuItem(HANDLE hIcolibIcon, TCHAR *text)
{
	m_hIcolibItem = hIcolibIcon;
	if (m_szText) mir_free(m_szText);
	m_szText = text ? mir_tstrdup(text) : NULL;

	if (!m_hMenuItem) return;

	CLISTMENUITEM mi = {0};
	mi.cbSize = sizeof(mi);
	mi.flags = CMIF_TCHAR|CMIF_ICONFROMICOLIB|CMIM_ICON|CMIM_NAME;
	mi.icolibItem = m_hIcolibItem;
	mi.ptszName = m_szText ? m_szText : _T("<advanced status slot>");
	CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)m_hMenuItem, (LPARAM)&mi);
}

int CPepGuiService::OnMenuItemClick(WPARAM, LPARAM)
{
	LaunchSetGui();
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// CPepMood

struct
{
	TCHAR	*szName;
	char* szTag;
} static g_arrMoods[] = 
{
	{ LPGENT("None"),			NULL					},
	{ LPGENT("Afraid"),			"afraid"		},
	{ LPGENT("Amazed"),			"amazed"		},
	{ LPGENT("Angry"),			"angry"		},
	{ LPGENT("Annoyed"),		"annoyed"		},
	{ LPGENT("Anxious"),		"anxious"		},
	{ LPGENT("Aroused"),		"aroused"		},
	{ LPGENT("Ashamed"),		"ashamed"		},
	{ LPGENT("Bored"),			"bored"		},
	{ LPGENT("Brave"),			"brave"		},
	{ LPGENT("Calm"),			"calm"		},
	{ LPGENT("Cold"),			"cold"		},
	{ LPGENT("Confused"),		"confused"	},
	{ LPGENT("Contented"),		"contented"	},
	{ LPGENT("Cranky"),			"cranky"		},
	{ LPGENT("Curious"),		"curious"		},
	{ LPGENT("Depressed"),		"depressed"	},
	{ LPGENT("Disappointed"),	"disappointed"},
	{ LPGENT("Disgusted"),		"disgusted"	},
	{ LPGENT("Distracted"),		"distracted"	},
	{ LPGENT("Embarrassed"),	"embarrassed"	},
	{ LPGENT("Excited"),		"excited"		},
	{ LPGENT("Flirtatious"),	"flirtatious"	},
	{ LPGENT("Frustrated"),		"frustrated"	},
	{ LPGENT("Grumpy"),			"grumpy"		},
	{ LPGENT("Guilty"),			"guilty"		},
	{ LPGENT("Happy"),			"happy"		},
	{ LPGENT("Hot"),			"hot"			},
	{ LPGENT("Humbled"),		"humbled"		},
	{ LPGENT("Humiliated"),		"humiliated"	},
	{ LPGENT("Hungry"),			"hungry"		},
	{ LPGENT("Hurt"),			"hurt"		},
	{ LPGENT("Impressed"),		"impressed"	},
	{ LPGENT("In awe"),			"in_awe"		},
	{ LPGENT("In love"),		"in_love"		},
	{ LPGENT("Indignant"),		"indignant"	},
	{ LPGENT("Interested"),		"interested"	},
	{ LPGENT("Intoxicated"),	"intoxicated"	},
	{ LPGENT("Invincible"),		"invincible"	},
	{ LPGENT("Jealous"),		"jealous"		},
	{ LPGENT("Lonely"),			"lonely"		},
	{ LPGENT("Mean"),			"mean"		},
	{ LPGENT("Moody"),			"moody"		},
	{ LPGENT("Nervous"),		"nervous"		},
	{ LPGENT("Neutral"),		"neutral"		},
	{ LPGENT("Offended"),		"offended"	},
	{ LPGENT("Playful"),		"playful"		},
	{ LPGENT("Proud"),			"proud"		},
	{ LPGENT("Relieved"),		"relieved"	},
	{ LPGENT("Remorseful"),		"remorseful"	},
	{ LPGENT("Restless"),		"restless"	},
	{ LPGENT("Sad"),			"sad"			},
	{ LPGENT("Sarcastic"),		"sarcastic"	},
	{ LPGENT("Serious"),		"serious"		},
	{ LPGENT("Shocked"),		"shocked"		},
	{ LPGENT("Shy"),			"shy"			},
	{ LPGENT("Sick"),			"sick"		},
	{ LPGENT("Sleepy"),			"sleepy"		},
	{ LPGENT("Stressed"),		"stressed"	},
	{ LPGENT("Surprised"),		"surprised"	},
	{ LPGENT("Thirsty"),		"thirsty"		},
	{ LPGENT("Worried"),		"worried"		},
};

CPepMood::CPepMood(CJabberProto *proto):
	CPepGuiService(proto, "Mood", _T(JABBER_FEAT_USER_MOOD)),
	m_icons(proto),
	m_text(NULL)
{
	UpdateMenuItem(LoadSkinnedIconHandle(SKINICON_OTHER_SMALLDOT), LPGENT("Set mood..."));
}

CPepMood::~CPepMood()
{
	if (m_text) mir_free(m_text);
}

void CPepMood::InitGui()
{
	CSuper::InitGui();

	char szFile[MAX_PATH];
	GetModuleFileNameA(hInst, szFile, MAX_PATH);
	if (char *p = strrchr(szFile, '\\'))
		strcpy( p+1, "..\\Icons\\jabber_xstatus.dll" );

	TCHAR szSection[100];

	mir_sntprintf(szSection, SIZEOF(szSection), _T("Status Icons/%s/Moods"), m_proto->m_tszUserName);
	for (int i = 1; i < SIZEOF(g_arrMoods); i++)
		m_icons.RegisterIcon( g_arrMoods[i].szTag, szFile, -(200+i), szSection, TranslateTS(g_arrMoods[i].szName));
}

void CPepMood::ProcessItems(const TCHAR *from, HXML itemsNode)
{
	HANDLE hContact = NULL;
	if (lstrcmp(from, m_proto->m_szJabberJID))
	{
		hContact = m_proto->HContactFromJID(from);
		if (!hContact) return;
	}

	if ( xmlGetChild( itemsNode, _T("retract")))
	{
		SetMood(hContact, NULL, NULL);
		return;
	}

	HXML itemNode = xmlGetChild( itemsNode, _T("item"));
	if (!itemNode) return;

	HXML moodNode = xmlGetChildByTag( itemNode, _T("mood"), _T("xmlns"), _T(JABBER_FEAT_USER_MOOD));
	if (!moodNode) return;

	LPCTSTR moodType = NULL, moodText = NULL;
	for ( int i=0; ; i++ )
	{
		HXML n = xmlGetChild( moodNode, i );
		if ( !n )
			break;

		if ( !_tcscmp( xmlGetName( n ), _T("text")))
			moodText = xmlGetText( n );
		else
			moodType = xmlGetName( n );
	}

	SetMood(hContact, moodType, moodText);
}

void CPepMood::CreateData( HXML n )
{
	HXML moodNode = n << XCHILDNS( _T("mood"), _T(JABBER_FEAT_USER_MOOD));
	moodNode << XCHILD( _A2T(g_arrMoods[m_mode].szTag));
	if ( m_text )
		moodNode << XCHILD( _T("text"), m_text );
}

void CPepMood::ResetExtraIcon(HANDLE hContact)
{
	char *szMood = m_proto->ReadAdvStatusA(hContact, ADVSTATUS_MOOD, "id");
	SetExtraIcon(hContact, szMood);
	mir_free(szMood);
}

void CPepMood::SetExtraIcon(HANDLE hContact, char *szMood)
{
	IconExtraColumn iec;
	iec.cbSize = sizeof(iec);
	iec.hImage = m_icons.GetClistHandle(szMood);
	iec.ColumnType = EXTRA_ICON_ADV1;
	CallService(MS_CLIST_EXTRA_SET_ICON, (WPARAM)hContact, (LPARAM)&iec);
}

void CPepMood::SetMood(HANDLE hContact, const TCHAR *szMood, const TCHAR *szText)
{
	int mood = -1;
	if (szMood)
	{
		char* p = mir_t2a( szMood );

		for (int i = 1; i < SIZEOF(g_arrMoods); ++i)
			if (!lstrcmpA(g_arrMoods[i].szTag, p ))
			{
				mood = i;
				break;
			}

		mir_free( p );

		if (mood < 0)
			return;
	}

	if (!hContact)
	{
		m_mode = mood;
		replaceStr(m_text, szText);

		HANDLE hIcon = (mood >= 0) ? m_icons.GetIcolibHandle(g_arrMoods[mood].szTag) : LoadSkinnedIconHandle(SKINICON_OTHER_SMALLDOT);
		TCHAR title[128];

		if (mood >= 0)
		{
			mir_sntprintf(title, SIZEOF(title), TranslateT("Mood: %s"), TranslateTS(g_arrMoods[mood].szName));
			m_proto->m_pInfoFrame->UpdateInfoItem("$/PEP/mood", m_icons.GetIcolibHandle(g_arrMoods[mood].szTag), TranslateTS(g_arrMoods[mood].szName));
		} else
		{
			lstrcpy(title, LPGENT("Set mood..."));
			m_proto->m_pInfoFrame->UpdateInfoItem("$/PEP/mood", LoadSkinnedIconHandle(SKINICON_OTHER_SMALLDOT), TranslateT("Set mood..."));
		}

		UpdateMenuItem(hIcon, title);
	} else
	{
		SetExtraIcon(hContact, mood < 0 ? NULL : g_arrMoods[mood].szTag);
	}

	if (szMood)
	{
		m_proto->JSetByte(hContact, DBSETTING_XSTATUSID, mood);
		m_proto->JSetStringT(hContact, DBSETTING_XSTATUSNAME, g_arrMoods[mood].szName);
		if (szText)
			m_proto->JSetStringT(hContact, DBSETTING_XSTATUSMSG, szText);
		else
			m_proto->JDeleteSetting(hContact, DBSETTING_XSTATUSMSG);

		m_proto->WriteAdvStatus(hContact, ADVSTATUS_MOOD, szMood, m_icons.GetIcolibName(g_arrMoods[mood].szTag), TranslateTS(g_arrMoods[mood].szName), szText);
	} else
	{
		m_proto->JDeleteSetting(hContact, DBSETTING_XSTATUSID);
		m_proto->JDeleteSetting(hContact, DBSETTING_XSTATUSNAME);
		m_proto->JDeleteSetting(hContact, DBSETTING_XSTATUSMSG);

		m_proto->ResetAdvStatus(hContact, ADVSTATUS_MOOD);
	}

	NotifyEventHooks(m_proto->m_hEventXStatusChanged, (WPARAM)hContact, 0);
}

void CPepMood::ShowSetDialog()
{
	CJabberDlgPepSimple dlg(m_proto, this, TranslateT("Set Mood"));
	for (int i = 1; i < SIZEOF(g_arrMoods); ++i)
		dlg.AddStatusMode(i, g_arrMoods[i].szTag, m_icons.GetIcon(g_arrMoods[i].szTag), TranslateTS(g_arrMoods[i].szName));
	dlg.SetActiveStatus(m_mode, m_text);
	dlg.DoModal();

	if (!dlg.OkClicked()) return;

	m_mode = dlg.GetStatusMode();
	if (m_mode >= 0)
	{
		replaceStr(m_text, dlg.GetStatusText());
		Publish();

		UpdateMenuItem(m_icons.GetIcolibHandle(g_arrMoods[m_mode].szTag), g_arrMoods[m_mode].szName);
		m_proto->m_pInfoFrame->UpdateInfoItem("$/PEP/mood", m_icons.GetIcolibHandle(g_arrMoods[m_mode].szTag), TranslateTS(g_arrMoods[m_mode].szName));
	} else
	{
		Retract();
		UpdateMenuItem(LoadSkinnedIconHandle(SKINICON_OTHER_SMALLDOT), LPGENT("Set mood..."));
		m_proto->m_pInfoFrame->UpdateInfoItem("$/PEP/mood", LoadSkinnedIconHandle(SKINICON_OTHER_SMALLDOT), TranslateT("Set mood..."));
	}
}

///////////////////////////////////////////////////////////////////////////////
// CPepActivity
#define ACTIVITY_ICON(section, item)	-(300 + (section) * 20)

struct
{
	char *szFirst;
	char *szSecond;
	TCHAR *szTitle;
	int iconid;
} g_arrActivities[] = 
{
	{ "doing_chores", NULL,       _T("Doing chores"),       ACTIVITY_ICON( 0,  0) },
	{ NULL, "buying_groceries",   _T("buying groceries"),   ACTIVITY_ICON( 0,  1) },
	{ NULL, "cleaning",           _T("cleaning"),           ACTIVITY_ICON( 0,  2) },
	{ NULL, "cooking",            _T("cooking"),            ACTIVITY_ICON( 0,  3) },
	{ NULL, "doing_maintenance",  _T("doing maintenance"),  ACTIVITY_ICON( 0,  4) },
	{ NULL, "doing_the_dishes",   _T("doing the dishes"),   ACTIVITY_ICON( 0,  5) },
	{ NULL, "doing_the_laundry",  _T("doing the laundry"),  ACTIVITY_ICON( 0,  6) },
	{ NULL, "gardening",          _T("gardening"),          ACTIVITY_ICON( 0,  7) },
	{ NULL, "running_an_errand",  _T("running an errand"),  ACTIVITY_ICON( 0,  8) },
	{ NULL, "walking_the_dog",    _T("walking the dog"),    ACTIVITY_ICON( 0,  9) },
	{ "drinking", NULL,           _T("Drinking"),           ACTIVITY_ICON( 1,  0) },
	{ NULL, "having_a_beer",      _T("having a beer"),      ACTIVITY_ICON( 1,  1) },
	{ NULL, "having_coffee",      _T("having coffee"),      ACTIVITY_ICON( 1,  2) },
	{ NULL, "having_tea",         _T("having tea"),         ACTIVITY_ICON( 1,  3) },
	{ "eating", NULL,             _T("Eating"),             ACTIVITY_ICON( 2,  0) },
	{ NULL, "having_a_snack",     _T("having a snack"),     ACTIVITY_ICON( 2,  1) },
	{ NULL, "having_breakfast",   _T("having breakfast"),   ACTIVITY_ICON( 2,  2) },
	{ NULL, "having_dinner",      _T("having dinner"),      ACTIVITY_ICON( 2,  3) },
	{ NULL, "having_lunch",       _T("having lunch"),       ACTIVITY_ICON( 2,  4) },
	{ "exercising", NULL,         _T("Exercising"),         ACTIVITY_ICON( 3,  0) },
	{ NULL, "cycling",            _T("cycling"),            ACTIVITY_ICON( 3,  1) },
	{ NULL, "hiking",             _T("hiking"),             ACTIVITY_ICON( 3,  2) },
	{ NULL, "jogging",            _T("jogging"),            ACTIVITY_ICON( 3,  3) },
	{ NULL, "playing_sports",     _T("playing sports"),     ACTIVITY_ICON( 3,  4) },
	{ NULL, "running",            _T("running"),            ACTIVITY_ICON( 3,  5) },
	{ NULL, "skiing",             _T("skiing"),             ACTIVITY_ICON( 3,  6) },
	{ NULL, "swimming",           _T("swimming"),           ACTIVITY_ICON( 3,  7) },
	{ NULL, "working_out",        _T("working out"),        ACTIVITY_ICON( 3,  8) },
	{ "grooming", NULL,           _T("Grooming"),           ACTIVITY_ICON( 4,  0) },
	{ NULL, "at_the_spa",         _T("at the spa"),         ACTIVITY_ICON( 4,  1) },
	{ NULL, "brushing_teeth",     _T("brushing teeth"),     ACTIVITY_ICON( 4,  2) },
	{ NULL, "getting_a_haircut",  _T("getting a haircut"),  ACTIVITY_ICON( 4,  3) },
	{ NULL, "shaving",            _T("shaving"),            ACTIVITY_ICON( 4,  4) },
	{ NULL, "taking_a_bath",      _T("taking a bath"),      ACTIVITY_ICON( 4,  5) },
	{ NULL, "taking_a_shower",    _T("taking a shower"),    ACTIVITY_ICON( 4,  6) },
	{ "having_appointment", NULL, _T("Having appointment"), ACTIVITY_ICON( 5,  0) },
	{ "inactive", NULL,           _T("Inactive"),           ACTIVITY_ICON( 6,  0) },
	{ NULL, "day_off",            _T("day off"),            ACTIVITY_ICON( 6,  1) },
	{ NULL, "hanging_out",        _T("hanging out"),        ACTIVITY_ICON( 6,  2) },
	{ NULL, "on_vacation",        _T("on vacation"),        ACTIVITY_ICON( 6,  3) },
	{ NULL, "scheduled_holiday",  _T("scheduled holiday"),  ACTIVITY_ICON( 6,  4) },
	{ NULL, "sleeping",           _T("sleeping"),           ACTIVITY_ICON( 6,  5) },
	{ "relaxing", NULL,           _T("Relaxing"),           ACTIVITY_ICON( 7,  0) },
	{ NULL, "gaming",             _T("gaming"),             ACTIVITY_ICON( 7,  1) },
	{ NULL, "going_out",          _T("going out"),          ACTIVITY_ICON( 7,  2) },
	{ NULL, "partying",           _T("partying"),           ACTIVITY_ICON( 7,  3) },
	{ NULL, "reading",            _T("reading"),            ACTIVITY_ICON( 7,  4) },
	{ NULL, "rehearsing",         _T("rehearsing"),         ACTIVITY_ICON( 7,  5) },
	{ NULL, "shopping",           _T("shopping"),           ACTIVITY_ICON( 7,  6) },
	{ NULL, "socializing",        _T("socializing"),        ACTIVITY_ICON( 7,  7) },
	{ NULL, "sunbathing",         _T("sunbathing"),         ACTIVITY_ICON( 7,  8) },
	{ NULL, "watching_tv",        _T("watching TV"),        ACTIVITY_ICON( 7,  9) },
	{ NULL, "watching_a_movie",   _T("watching a movie"),   ACTIVITY_ICON( 7, 10) },
	{ "talking", NULL,            _T("Talking"),            ACTIVITY_ICON( 8,  0) },
	{ NULL, "in_real_life",       _T("in real life"),       ACTIVITY_ICON( 8,  1) },
	{ NULL, "on_the_phone",       _T("on the phone"),       ACTIVITY_ICON( 8,  2) },
	{ NULL, "on_video_phone",     _T("on video phone"),     ACTIVITY_ICON( 8,  3) },
	{ "traveling", NULL,          _T("Traveling"),          ACTIVITY_ICON( 9,  0) },
	{ NULL, "commuting",          _T("commuting"),          ACTIVITY_ICON( 9,  1) },
	{ NULL, "cycling",            _T("cycling"),            ACTIVITY_ICON( 9,  2) },
	{ NULL, "driving",            _T("driving"),            ACTIVITY_ICON( 9,  3) },
	{ NULL, "in_a_car",           _T("in a car"),           ACTIVITY_ICON( 9,  4) },
	{ NULL, "on_a_bus",           _T("on a bus"),           ACTIVITY_ICON( 9,  5) },
	{ NULL, "on_a_plane",         _T("on a plane"),         ACTIVITY_ICON( 9,  6) },
	{ NULL, "on_a_train",         _T("on a train"),         ACTIVITY_ICON( 9,  7) },
	{ NULL, "on_a_trip",          _T("on a trip"),          ACTIVITY_ICON( 9,  8) },
	{ NULL, "walking",            _T("walking"),            ACTIVITY_ICON( 9,  9) },
	{ "working", NULL,            _T("Working"),            ACTIVITY_ICON(10,  0) },
	{ NULL, "coding",             _T("coding"),             ACTIVITY_ICON(10,  1) },
	{ NULL, "in_a_meeting",       _T("in a meeting"),       ACTIVITY_ICON(10,  2) },
	{ NULL, "studying",           _T("studying"),           ACTIVITY_ICON(10,  3) },
	{ NULL, "writing",            _T("writing"),            ACTIVITY_ICON(10,  4) },
	{ NULL, NULL, NULL } // the end, don't delete this
};

inline char *ActivityGetId(int id)
{
	return g_arrActivities[id].szSecond ? g_arrActivities[id].szSecond : g_arrActivities[id].szFirst;
}

// -1 if not found, otherwise activity number
static int ActivityCheck( LPCTSTR szFirstNode, LPCTSTR szSecondNode )
{
	if (!szFirstNode) return 0;

	char *s1 = mir_t2a( szFirstNode ), *s2 = mir_t2a( szSecondNode );

	int i = 0, nFirst = -1, nSecond = -1;
	while ( g_arrActivities[i].szFirst || g_arrActivities[i].szSecond ) {
		// check first node
		if ( g_arrActivities[i].szFirst && !strcmp( s1, g_arrActivities[i].szFirst )) {
			// first part found
			nFirst = i;
			if ( !s2 ) {
				nSecond = i;
				break;
			}
			i++; // move to next
			while ( g_arrActivities[i].szSecond ) {
				if ( !strcmp( g_arrActivities[i].szSecond, s2 )) {
					nSecond = i;
					break;
				}
				i++;
			}
			break;
		}
		i++;
	}

	mir_free( s1 );
	mir_free( s2 );

	if ( nSecond != -1 )
		return nSecond;
	
	return nFirst;
}

char *ActivityGetFirst(int id)
{
	if (id >= SIZEOF(g_arrActivities) - 1)
		return NULL;

	while (id >= 0)
	{
		if (g_arrActivities[id].szFirst)
			return g_arrActivities[id].szFirst;
		--id;
	}

	return NULL;
}

char *ActivityGetSecond(int id)
{
	return (id >= 0) ? g_arrActivities[id].szSecond : NULL;
}

TCHAR *ActivityGetFirstTitle(int id)
{
	if (id >= SIZEOF(g_arrActivities) - 1)
		return NULL;

	while (id >= 0)
	{
		if (g_arrActivities[id].szFirst)
			return g_arrActivities[id].szTitle;
		--id;
	}

	return NULL;
}

TCHAR *ActivityGetSecondTitle(int id)
{
	return ((id >= 0) && g_arrActivities[id].szSecond) ? g_arrActivities[id].szTitle : NULL;
}

void ActivityBuildTitle(int id, TCHAR *buf, int size)
{
	TCHAR *szFirst = ActivityGetFirstTitle(id);
	TCHAR *szSecond = ActivityGetSecondTitle(id);

	if (szFirst)
	{
		if (szSecond)
			mir_sntprintf(buf, size, _T("%s [%s]"), TranslateTS(szFirst), TranslateTS(szSecond));
		else
			lstrcpyn(buf, TranslateTS(szFirst), size);
	} else
		*buf = 0;
}

CPepActivity::CPepActivity(CJabberProto *proto):
	CPepGuiService(proto, "Activity", _T(JABBER_FEAT_USER_ACTIVITY)),
	m_icons(proto),
	m_text(NULL)
{
	UpdateMenuItem(LoadSkinnedIconHandle(SKINICON_OTHER_SMALLDOT), LPGENT("Set activity..."));
}

CPepActivity::~CPepActivity()
{
	if (m_text) mir_free(m_text);
}

void CPepActivity::InitGui()
{
	CSuper::InitGui();

	char szFile[MAX_PATH];
	GetModuleFileNameA(hInst, szFile, MAX_PATH);
	if (char *p = strrchr(szFile, '\\'))
		strcpy( p+1, "..\\Icons\\jabber_xstatus.dll" );

	TCHAR szSection[100];

	mir_sntprintf(szSection, SIZEOF(szSection), _T("Status Icons/%s/Activities"), m_proto->m_tszUserName);
	for (int i = 0; i < SIZEOF(g_arrActivities); i++)
		if (g_arrActivities[i].szFirst)
			m_icons.RegisterIcon(g_arrActivities[i].szFirst, szFile, g_arrActivities[i].iconid, szSection, TranslateTS(g_arrActivities[i].szTitle));
}

void CPepActivity::ProcessItems(const TCHAR *from, HXML itemsNode)
{
	HANDLE hContact = NULL;
	if (lstrcmp(from, m_proto->m_szJabberJID))
	{
		hContact = m_proto->HContactFromJID(from);
		if (!hContact) return;
	}

	if ( xmlGetChild( itemsNode, "retract"))
	{
		SetActivity(hContact, NULL, NULL, NULL);
		return;
	}

	HXML itemNode = xmlGetChild( itemsNode, "item");
	if (!itemNode) return;

	HXML actNode = xmlGetChildByTag( itemNode, "activity", "xmlns", _T(JABBER_FEAT_USER_ACTIVITY));
	if (!actNode) return;

	LPCTSTR szFirstNode = NULL, szSecondNode = NULL, szText = NULL;

	HXML textNode = xmlGetChild( actNode, "text");
	if (textNode && xmlGetText( textNode ))
		szText = xmlGetText( textNode );

	for ( int i = 0; ; i++ )
	{
		HXML n = xmlGetChild( actNode, i );
		if ( !n )
			break;

		if ( lstrcmp( xmlGetName( n ), _T("text")))
		{
			szFirstNode = xmlGetName( n );
			HXML secondNode = xmlGetChild( n, 0 );
			if (szFirstNode && secondNode && xmlGetName( secondNode ))
				szSecondNode = xmlGetName( secondNode );
			break;
		}
	}

	SetActivity(hContact, szFirstNode, szSecondNode, szText);
}

void CPepActivity::CreateData( HXML n )
{
	char *szFirstNode = ActivityGetFirst(m_mode);
	char *szSecondNode = ActivityGetSecond(m_mode);

	HXML activityNode = n << XCHILDNS( _T("activity"), _T(JABBER_FEAT_USER_ACTIVITY));
	HXML firstNode = activityNode << XCHILD( _A2T( szFirstNode ));
			
	if (firstNode && szSecondNode)
		firstNode << XCHILD( _A2T(szSecondNode));

	if (m_text)
		activityNode << XCHILD( _T("text"), m_text);
}

void CPepActivity::ResetExtraIcon(HANDLE hContact)
{
	char *szActivity = m_proto->ReadAdvStatusA(hContact, ADVSTATUS_ACTIVITY, "id");
	SetExtraIcon(hContact, szActivity);
	mir_free(szActivity);
}

void CPepActivity::SetExtraIcon(HANDLE hContact, char *szActivity)
{
	IconExtraColumn iec;
	iec.cbSize = sizeof(iec);
	iec.hImage = m_icons.GetClistHandle(szActivity);
	iec.ColumnType = EXTRA_ICON_ADV2;
	CallService(MS_CLIST_EXTRA_SET_ICON, (WPARAM)hContact, (LPARAM)&iec);
}

void CPepActivity::SetActivity(HANDLE hContact, LPCTSTR szFirst, LPCTSTR szSecond, LPCTSTR szText)
{
	int activity = -1;
	if (szFirst || szSecond)
	{
		activity = ActivityCheck(szFirst, szSecond);

		if (activity < 0)
			return;
	}

	TCHAR activityTitle[128];
	ActivityBuildTitle(activity, activityTitle, SIZEOF(activityTitle));

	if (!hContact)
	{
		m_mode = activity;
		replaceStr(m_text, szText);

		HANDLE hIcon = (activity >= 0) ? m_icons.GetIcolibHandle(ActivityGetFirst(activity)) : LoadSkinnedIconHandle(SKINICON_OTHER_SMALLDOT);
		TCHAR title[128];

		if (activity >= 0)
		{
			mir_sntprintf(title, SIZEOF(title), TranslateT("Activity: %s"), activityTitle);
			m_proto->m_pInfoFrame->UpdateInfoItem("$/PEP/activity", m_icons.GetIcolibHandle(ActivityGetFirst(activity)), activityTitle);
		} else
		{
			lstrcpy(title, LPGENT("Set activity..."));
			m_proto->m_pInfoFrame->UpdateInfoItem("$/PEP/activity", LoadSkinnedIconHandle(SKINICON_OTHER_SMALLDOT), TranslateT("Set activity..."));
		}

		UpdateMenuItem(hIcon, title);
	} else
	{
		SetExtraIcon(hContact, activity < 0 ? NULL : ActivityGetFirst(activity));
	}


	if (activity >= 0) {
		TCHAR* p = mir_a2t( ActivityGetId(activity));
		m_proto->WriteAdvStatus(hContact, ADVSTATUS_ACTIVITY, p, m_icons.GetIcolibName(ActivityGetFirst(activity)), activityTitle, szText);
		mir_free( p );
	}
	else
		m_proto->ResetAdvStatus(hContact, ADVSTATUS_ACTIVITY);
}

void CPepActivity::ShowSetDialog()
{
	CJabberDlgPepSimple dlg(m_proto, this, TranslateT("Set Activity"));
	for (int i = 0; i < SIZEOF(g_arrActivities); ++i)
		if (g_arrActivities[i].szFirst || g_arrActivities[i].szSecond)
			dlg.AddStatusMode(i, ActivityGetId(i), m_icons.GetIcon(ActivityGetFirst(i)), TranslateTS(g_arrActivities[i].szTitle), g_arrActivities[i].szSecond ? true : false);
	dlg.SetActiveStatus(m_mode, m_text);
	dlg.DoModal();

	if (!dlg.OkClicked()) return;

	m_mode = dlg.GetStatusMode();
	if (m_mode >= 0)
	{
		replaceStr(m_text, dlg.GetStatusText());
		Publish();

		UpdateMenuItem(m_icons.GetIcolibHandle(ActivityGetFirst(m_mode)), g_arrActivities[m_mode].szTitle);
		m_proto->m_pInfoFrame->UpdateInfoItem("$/PEP/activity", m_icons.GetIcolibHandle(ActivityGetFirst(m_mode)), TranslateTS(g_arrActivities[m_mode].szTitle));
	} else
	{
		Retract();
		UpdateMenuItem(LoadSkinnedIconHandle(SKINICON_OTHER_SMALLDOT), LPGENT("Set activity..."));
		m_proto->m_pInfoFrame->UpdateInfoItem("$/PEP/activity", LoadSkinnedIconHandle(SKINICON_OTHER_SMALLDOT), TranslateT("Set activity..."));
	}
}

///////////////////////////////////////////////////////////////////////////////
// icq api emulation

HICON CJabberProto::GetXStatusIcon(int bStatus, UINT flags)
{
	CPepMood *pepMood = (CPepMood *)m_pepServices.Find(_T(JABBER_FEAT_USER_MOOD));
	HICON icon = pepMood->m_icons.GetIcon(g_arrMoods[bStatus].szTag);
	return ( flags & LR_SHARED ) ? icon : CopyIcon( icon );
}

int CJabberProto::CListMW_ExtraIconsApply( WPARAM wParam, LPARAM lParam ) 
{
	if (m_bJabberOnline && m_bPepSupported && ServiceExists(MS_CLIST_EXTRA_SET_ICON)) 
	{
		char* szProto = ( char* )JCallService( MS_PROTO_GETCONTACTBASEPROTO, wParam, 0 );
		if ( szProto==NULL || strcmp( szProto, m_szModuleName ))
			return 0; // only apply icons to our contacts, do not mess others

		m_pepServices.ResetExtraIcon((HANDLE)wParam);
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberGetXStatus - gets the extended status info (mood)

int __cdecl CJabberProto::OnGetXStatus( WPARAM wParam, LPARAM lParam )
{
	if ( !m_bJabberOnline || !m_bPepSupported )
		return 0;

	if ( wParam ) *(( char** )wParam ) = DBSETTING_XSTATUSNAME;
	if ( lParam ) *(( char** )lParam ) = DBSETTING_XSTATUSMSG;
	return ((CPepMood *)m_pepServices.Find(_T(JABBER_FEAT_USER_MOOD)))->m_mode;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberGetXStatusIcon - Retrieves specified custom status icon
//wParam = (int)N  // custom status id, 0 = my current custom status
//lParam = flags   // use LR_SHARED for shared HICON
//return = HICON   // custom status icon (use DestroyIcon to release resources if not LR_SHARED)

int __cdecl CJabberProto::OnGetXStatusIcon( WPARAM wParam, LPARAM lParam )
{
	if ( !m_bJabberOnline )
		return 0;

	if ( !wParam )
		wParam = ((CPepMood *)m_pepServices.Find(_T(JABBER_FEAT_USER_MOOD)))->m_mode;

	if ( wParam < 1 || wParam >= SIZEOF(g_arrMoods) )
		return 0;

	if ( lParam & LR_SHARED )
		return (int)GetXStatusIcon( wParam, LR_SHARED );

	return (int)GetXStatusIcon( wParam, 0 );
}

/////////////////////////////////////////////////////////////////////////////////////////
// SendPepMood - sends mood

BOOL CJabberProto::SendPepTune( TCHAR* szArtist, TCHAR* szLength, TCHAR* szSource, TCHAR* szTitle, TCHAR* szTrack, TCHAR* szUri )
{
	if ( !m_bJabberOnline || !m_bPepSupported )
		return FALSE;

	XmlNodeIq iq( _T("set"), SerialNext() );
	HXML pubsubNode = iq << XCHILDNS( _T("pubsub"), _T(JABBER_FEAT_PUBSUB));

	if ( !szArtist && !szLength && !szSource && !szTitle && !szUri ) {
		pubsubNode << XCHILD( _T("retract")) << XATTR( _T("node"), _T(JABBER_FEAT_USER_TUNE)) << XATTRI( _T("notify"), 1 )
			<< XCHILD( _T("item")) << XATTR( _T("id"), _T("current"));
	}
	else {
		HXML tuneNode = pubsubNode << XCHILD( _T("publish")) << XATTR( _T("node"), _T(JABBER_FEAT_USER_TUNE))
			<< XCHILD( _T("item")) << XATTR( _T("id"), _T("current"))
				<< XCHILDNS( _T("tune"), _T(JABBER_FEAT_USER_TUNE));
		if ( szArtist ) tuneNode << XCHILD( _T("artist"), szArtist );
		if ( szLength ) tuneNode << XCHILD( _T("length"), szLength );
		if ( szSource ) tuneNode << XCHILD( _T("source"), szSource );
		if ( szTitle ) tuneNode << XCHILD( _T("title"), szTitle );
		if ( szTrack ) tuneNode << XCHILD( _T("track"), szTrack );
		if ( szUri ) tuneNode << XCHILD( _T("uri"), szUri );
	}
	m_ThreadInfo->send( iq );

	return TRUE;
}

void CJabberProto::SetContactTune( HANDLE hContact, LPCTSTR szArtist, LPCTSTR szLength, LPCTSTR szSource, LPCTSTR szTitle, LPCTSTR szTrack, LPCTSTR szUri )
{
	if ( !szArtist && !szTitle ) {
		JDeleteSetting( hContact, "ListeningTo" );
		ResetAdvStatus( hContact, ADVSTATUS_TUNE );
		return;
	}

	TCHAR *szListeningTo;
	if ( ServiceExists( MS_LISTENINGTO_GETPARSEDTEXT )) {
		LISTENINGTOINFO li;
		ZeroMemory( &li, sizeof( li ));
		li.cbSize = sizeof( li );
		li.dwFlags = LTI_TCHAR;
		li.ptszArtist = ( TCHAR* )szArtist;
		li.ptszLength = ( TCHAR* )szLength;
		li.ptszAlbum = ( TCHAR* )szSource;
		li.ptszTitle = ( TCHAR* )szTitle;
		li.ptszTrack = ( TCHAR* )szTrack;
		szListeningTo = (TCHAR *)CallService( MS_LISTENINGTO_GETPARSEDTEXT, (WPARAM)_T("%title% - %artist%"), (LPARAM)&li );
	}
	else {
		szListeningTo = (TCHAR *) mir_alloc( 2048 * sizeof( TCHAR ));
		mir_sntprintf( szListeningTo, 2047, _T("%s - %s"), szTitle ? szTitle : _T(""), szArtist ? szArtist : _T("") );
	}

	JSetStringT( hContact, "ListeningTo", szListeningTo );

	char tuneIcon[128];
	mir_snprintf(tuneIcon, SIZEOF(tuneIcon), "%s_%s", m_szModuleName, "main");
	WriteAdvStatus( hContact, ADVSTATUS_TUNE, _T("listening_to"), tuneIcon, TranslateT("Listening To"), szListeningTo );

	mir_free( szListeningTo );
}

TCHAR* a2tf( const TCHAR* str, BOOL unicode )
{
	if ( str == NULL )
		return NULL;

	#if defined( _UNICODE )
		return ( unicode ) ? mir_tstrdup( str ) : mir_a2t(( char* )str );
	#else
		return mir_strdup( str );
	#endif
}

void overrideStr( TCHAR*& dest, const TCHAR* src, BOOL unicode, const TCHAR* def = NULL )
{
	if ( dest != NULL )
	{
		mir_free( dest );
		dest = NULL;
	}

	if ( src != NULL )
		dest = a2tf( src, unicode );
	else if ( def != NULL )
		dest = mir_tstrdup( def );
}

int __cdecl CJabberProto::OnSetListeningTo( WPARAM wParam, LPARAM lParam )
{
	LISTENINGTOINFO *cm = (LISTENINGTOINFO *)lParam;
	if ( !cm || cm->cbSize != sizeof(LISTENINGTOINFO) ) {
		SendPepTune( NULL, NULL, NULL, NULL, NULL, NULL );
		JDeleteSetting( NULL, "ListeningTo" );
	}
	else {
		TCHAR *szArtist = NULL, *szLength = NULL, *szSource = NULL;
		TCHAR *szTitle = NULL, *szTrack = NULL;

		BOOL unicode = cm->dwFlags & LTI_UNICODE;

		overrideStr( szArtist, cm->ptszArtist, unicode );
		overrideStr( szSource, cm->ptszAlbum, unicode );
		overrideStr( szTitle, cm->ptszTitle, unicode );
		overrideStr( szTrack, cm->ptszTrack, unicode );
		overrideStr( szLength, cm->ptszLength, unicode );

		TCHAR szLengthInSec[ 32 ];
		szLengthInSec[ 0 ] = _T('\0');
		if ( szLength ) {
			unsigned int multiplier = 1, result = 0;
			for ( TCHAR *p = szLength; *p; p++ ) {
				if ( *p == _T(':')) multiplier *= 60;
			}
			if ( multiplier <= 3600 ) {
				TCHAR *szTmp = szLength;
				while ( szTmp[0] ) {
					result += ( _ttoi( szTmp ) * multiplier );
					multiplier /= 60;
					szTmp = _tcschr( szTmp, _T(':') );
					if ( !szTmp )
						break;
					szTmp++;
				}
			}
			mir_sntprintf( szLengthInSec, SIZEOF( szLengthInSec ), _T("%d"), result );
		}

		SendPepTune( szArtist, szLength ? szLengthInSec : NULL, szSource, szTitle, szTrack, NULL );
		SetContactTune( NULL, szArtist, szLength, szSource, szTitle, szTrack, NULL );
		
		mir_free( szArtist );
		mir_free( szLength );
		mir_free( szSource );
		mir_free( szTitle );
		mir_free( szTrack );
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// process InfoFrame clicks

void CJabberProto::InfoFrame_OnUserMood(CJabberInfoFrame_Event *evt)
{
	m_pepServices.Find(_T(JABBER_FEAT_USER_MOOD))->LaunchSetGui();
}

void CJabberProto::InfoFrame_OnUserActivity(CJabberInfoFrame_Event *evt)
{
	m_pepServices.Find(_T(JABBER_FEAT_USER_ACTIVITY))->LaunchSetGui();
}

/////////////////////////////////////////////////////////////////////////////////////////
// builds xstatus menu

void CJabberProto::XStatusInit()
{
	JHookEvent( ME_CLIST_EXTRA_IMAGE_APPLY,  &CJabberProto::CListMW_ExtraIconsApply );

	RegisterAdvStatusSlot( ADVSTATUS_MOOD );
	RegisterAdvStatusSlot( ADVSTATUS_TUNE );
	RegisterAdvStatusSlot( ADVSTATUS_ACTIVITY );
}

void CJabberProto::XStatusUninit()
{
	if ( m_hHookExtraIconsRebuild )
		UnhookEvent( m_hHookExtraIconsRebuild );

	if ( m_hHookExtraIconsApply )
		UnhookEvent( m_hHookExtraIconsApply );
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberSetXStatus - sets the extended status info (mood)

int __cdecl CJabberProto::OnSetXStatus( WPARAM wParam, LPARAM lParam )
{
	if ( !m_bPepSupported || !m_bJabberOnline)
		return 0;

	CPepMood *pepMood = (CPepMood *)m_pepServices.Find(_T(JABBER_FEAT_USER_MOOD));
	if (!wParam)
	{
		pepMood->m_mode = -1;
		pepMood->Publish();
		return 0;
	}

	if ((wParam > 0) && (wParam < SIZEOF(g_arrMoods)))
	{
		pepMood->m_mode = wParam;
		pepMood->LaunchSetGui();
		return wParam;
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Advanced status slots
// DB data format:
//     Contact / AdvStatus / Proto/Status/id = mode_id
//     Contact / AdvStatus / Proto/Status/icon = icon
//     Contact / AdvStatus / Proto/Status/title = title
//     Contact / AdvStatus / Proto/Status/text = title

void CJabberProto::RegisterAdvStatusSlot(const char *pszSlot)
{
	char szSetting[256];
	mir_snprintf(szSetting, SIZEOF(szSetting), "AdvStatus/%s/%s/id", m_szModuleName, pszSlot);
	CallService(MS_DB_SETSETTINGRESIDENT, TRUE, (LPARAM)szSetting);
	mir_snprintf(szSetting, SIZEOF(szSetting), "AdvStatus/%s/%s/icon", m_szModuleName, pszSlot);
	CallService(MS_DB_SETSETTINGRESIDENT, TRUE, (LPARAM)szSetting);
	mir_snprintf(szSetting, SIZEOF(szSetting), "AdvStatus/%s/%s/title", m_szModuleName, pszSlot);
	CallService(MS_DB_SETSETTINGRESIDENT, TRUE, (LPARAM)szSetting);
	mir_snprintf(szSetting, SIZEOF(szSetting), "AdvStatus/%s/%s/text", m_szModuleName, pszSlot);
	CallService(MS_DB_SETSETTINGRESIDENT, TRUE, (LPARAM)szSetting);
}

void CJabberProto::ResetAdvStatus(HANDLE hContact, const char *pszSlot)
{
	char szSetting[128];
	mir_snprintf(szSetting, SIZEOF(szSetting), "%s/%s/id", m_szModuleName, pszSlot);
	DBDeleteContactSetting(hContact, "AdvStatus", szSetting);
	mir_snprintf(szSetting, SIZEOF(szSetting), "%s/%s/icon", m_szModuleName, pszSlot);
	DBDeleteContactSetting(hContact, "AdvStatus", szSetting);
	mir_snprintf(szSetting, SIZEOF(szSetting), "%s/%s/title", m_szModuleName, pszSlot);
	DBDeleteContactSetting(hContact, "AdvStatus", szSetting);
	mir_snprintf(szSetting, SIZEOF(szSetting), "%s/%s/text", m_szModuleName, pszSlot);
	DBDeleteContactSetting(hContact, "AdvStatus", szSetting);
}

void CJabberProto::WriteAdvStatus(HANDLE hContact, const char *pszSlot, const TCHAR *pszMode, const char *pszIcon, const TCHAR *pszTitle, const TCHAR *pszText)
{
	char szSetting[128];

	mir_snprintf(szSetting, SIZEOF(szSetting), "%s/%s/id", m_szModuleName, pszSlot);
	DBWriteContactSettingTString(hContact, "AdvStatus", szSetting, pszMode);

	mir_snprintf(szSetting, SIZEOF(szSetting), "%s/%s/icon", m_szModuleName, pszSlot);
	DBWriteContactSettingString(hContact, "AdvStatus", szSetting, pszIcon);

	mir_snprintf(szSetting, SIZEOF(szSetting), "%s/%s/title", m_szModuleName, pszSlot);
	DBWriteContactSettingTString(hContact, "AdvStatus", szSetting, pszTitle);

	mir_snprintf(szSetting, SIZEOF(szSetting), "%s/%s/text", m_szModuleName, pszSlot);
	if (pszText)
		DBWriteContactSettingTString(hContact, "AdvStatus", szSetting, pszText);
	else
		DBDeleteContactSetting(hContact, "AdvStatus", szSetting);
}

char *CJabberProto::ReadAdvStatusA(HANDLE hContact, const char *pszSlot, const char *pszValue)
{
	char szSetting[128];
	DBVARIANT dbv = {0};

	mir_snprintf(szSetting, SIZEOF(szSetting), "%s/%s/%s", m_szModuleName, pszSlot, pszValue);
	if (DBGetContactSettingString(hContact, "AdvStatus", szSetting, &dbv))
		return NULL;

	char *res = mir_strdup(dbv.pszVal);
	DBFreeVariant(&dbv);
	return res;
}

TCHAR *CJabberProto::ReadAdvStatusT(HANDLE hContact, const char *pszSlot, const char *pszValue)
{
	char szSetting[128];
	DBVARIANT dbv = {0};

	mir_snprintf(szSetting, SIZEOF(szSetting), "%s/%s/%s", m_szModuleName, pszSlot, pszValue);
	if (DBGetContactSettingTString(hContact, "AdvStatus", szSetting, &dbv))
		return NULL;

	TCHAR *res = mir_tstrdup(dbv.ptszVal);
	DBFreeVariant(&dbv);
	return res;
}

/////////////////////////////////////////////////////////////////////////////////////////
// CJabberInfoFrame

class CJabberInfoFrameItem
{
public:
	char *m_pszName;
	HANDLE m_hIcolibIcon;
	TCHAR *m_pszText;
	LPARAM m_pUserData;
	bool m_bCompact;
	bool m_bShow;
	void (CJabberProto::*m_onEvent)(CJabberInfoFrame_Event *);
	RECT m_rcItem;
	int m_tooltipId;

public:
/*
	CJabberInfoFrameItem():
		m_pszName(NULL), m_hIcolibIcon(NULL), m_pszText(NULL)
	{
	}
*/
	CJabberInfoFrameItem(char *pszName, bool bCompact=false, LPARAM pUserData=0):
		m_pszName(NULL), m_hIcolibIcon(NULL), m_pszText(NULL), m_bShow(true), m_bCompact(bCompact), m_pUserData(pUserData), m_onEvent(NULL)
	{
		m_pszName = mir_strdup(pszName);
	}
	~CJabberInfoFrameItem()
	{
		mir_free(m_pszName);
		mir_free(m_pszText);
	}

	void SetInfo(HANDLE hIcolibIcon, TCHAR *pszText)
	{
		mir_free(m_pszText);
		m_pszText = pszText ? mir_tstrdup(pszText) : NULL;
		m_hIcolibIcon = hIcolibIcon;
	}

	static int cmp(const CJabberInfoFrameItem *p1, const CJabberInfoFrameItem *p2)
	{
		return lstrcmpA(p1->m_pszName, p2->m_pszName);
	}
};

CJabberInfoFrame::CJabberInfoFrame(CJabberProto *proto):
	m_pItems(3, CJabberInfoFrameItem::cmp), m_compact(false)
{
	m_proto = proto;
	m_hwnd = m_hwndToolTip = NULL;
	m_clickedItem = -1;
	m_hiddenItemCount = 0;
	m_bLocked = false;
	m_nextTooltipId = 0;
	m_hhkFontsChanged = 0;

	if (ServiceExists(MS_CLIST_FRAMES_ADDFRAME))
	{
		InitClass();

		CLISTFrame frame = {0};
		frame.cbSize = sizeof(frame);
		HWND hwndClist = (HWND)CallService(MS_CLUI_GETHWND, 0, 0);
		frame.hWnd = CreateWindowEx(0, _T("JabberInfoFrameClass"), NULL, WS_CHILD|WS_VISIBLE, 0, 0, 100, 100, hwndClist, NULL, hInst, this);
		int e = GetLastError();
		frame.hIcon = NULL;
		frame.align = alBottom;
		frame.height = 2 * SZ_FRAMEPADDING + GetSystemMetrics(SM_CYSMICON) + SZ_LINEPADDING; // compact height by default
		frame.Flags = F_VISIBLE|F_LOCKED|F_NOBORDER|F_TCHAR;
		frame.tname = mir_a2t(proto->m_szModuleName);
		frame.TBtname = proto->m_tszUserName;
		m_frameId = CallService(MS_CLIST_FRAMES_ADDFRAME, (WPARAM)&frame, 0);
		mir_free(frame.tname);

		m_hhkFontsChanged = HookEventMessage(ME_FONT_RELOAD, m_hwnd, WM_APP);
		ReloadFonts();

		m_hwndToolTip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
			WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			m_hwnd, NULL, hInst, NULL);
		SetWindowPos(m_hwndToolTip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

		CreateInfoItem("$", true);
		UpdateInfoItem("$", proto->GetIconHandle(IDI_JABBER), proto->m_tszUserName);

		CreateInfoItem("$/JID", true);
		UpdateInfoItem("$/JID", LoadSkinnedIconHandle(SKINICON_OTHER_USERDETAILS), _T("Offline"));
		SetInfoItemCallback("$/JID", &CJabberProto::InfoFrame_OnSetup);
	}
}

CJabberInfoFrame::~CJabberInfoFrame()
{
	if (!m_hwnd) return;

	if (m_hhkFontsChanged) UnhookEvent(m_hhkFontsChanged);
	CallService(MS_CLIST_FRAMES_REMOVEFRAME, (WPARAM)m_frameId, 0);
	SetWindowLong(m_hwnd, GWL_USERDATA, 0);
	DestroyWindow(m_hwnd);
	DestroyWindow(m_hwndToolTip);
	DeleteObject(m_hfntText);
	DeleteObject(m_hfntTitle);
	m_hwnd = NULL;
}

void CJabberInfoFrame::InitClass()
{
	static bool bClassRegistered = false;
	if (bClassRegistered) return;

	WNDCLASSEX wcx = {0};
	wcx.cbSize = sizeof(wcx);
	wcx.style = CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW;
	wcx.lpfnWndProc = GlobalWndProc;
	wcx.hInstance = hInst;
	wcx.lpszClassName = _T("JabberInfoFrameClass");
	wcx.hCursor = LoadCursor(NULL, IDC_ARROW);
	RegisterClassEx(&wcx);
	bClassRegistered = true;
}

LRESULT CALLBACK CJabberInfoFrame::GlobalWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CJabberInfoFrame *pFrame;

	if (msg == WM_CREATE)
	{
		CREATESTRUCT *pcs = (CREATESTRUCT *)lParam;
		pFrame = (CJabberInfoFrame *)pcs->lpCreateParams;
		if (pFrame) pFrame->m_hwnd = hwnd;
		SetWindowLong(hwnd, GWL_USERDATA, (LONG)pFrame);
	} else
	{
		pFrame = (CJabberInfoFrame *)GetWindowLong(hwnd, GWL_USERDATA);
	}

	return pFrame ? pFrame->WndProc(msg, wParam, lParam) : DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CJabberInfoFrame::WndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_APP:
		{
			ReloadFonts();
			return 0;
		}

		case WM_PAINT:
		{
			RECT rc; GetClientRect(m_hwnd, &rc);
			m_compact = rc.bottom < (2 * (GetSystemMetrics(SM_CYSMICON) + SZ_LINEPADDING) + SZ_LINESPACING + 2 * SZ_FRAMEPADDING);

			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(m_hwnd, &ps);
			m_compact ? PaintCompact(hdc) : PaintNormal(hdc);
			EndPaint(m_hwnd, &ps);
			return 0;
		}

		case WM_RBUTTONUP:
		{
			POINT pt = { LOWORD(lParam), HIWORD(lParam) };
			MapWindowPoints(m_hwnd, NULL, &pt, 1);
			HMENU hMenu = (HMENU)CallService(MS_CLIST_MENUBUILDFRAMECONTEXT, m_frameId, 0);
			int res = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, (HWND)CallService(MS_CLUI_GETHWND, 0, 0), NULL);
			CallService(MS_CLIST_MENUPROCESSCOMMAND, MAKEWPARAM(res, 0), m_frameId);
			return 0;
		}

		case WM_LBUTTONDOWN:
		{
			POINT pt = { LOWORD(lParam), HIWORD(lParam) };
			for (int i = 0; i < m_pItems.getCount(); ++i)
				if (m_pItems[i].m_onEvent && PtInRect(&m_pItems[i].m_rcItem, pt))
				{
					m_clickedItem = i;
					return 0;
				}

			return 0;
		}

		case WM_LBUTTONUP:
		{
			POINT pt = { LOWORD(lParam), HIWORD(lParam) };
			if ((m_clickedItem >= 0) && (m_clickedItem < m_pItems.getCount()) && m_pItems[m_clickedItem].m_onEvent && PtInRect(&m_pItems[m_clickedItem].m_rcItem, pt))
			{
				CJabberInfoFrame_Event evt;
				evt.m_event = CJabberInfoFrame_Event::CLICK;
				evt.m_pszName = m_pItems[m_clickedItem].m_pszName;
				evt.m_pUserData = m_pItems[m_clickedItem].m_pUserData;
				(m_proto->*m_pItems[m_clickedItem].m_onEvent)(&evt);
				return 0;
			}

			m_clickedItem = -1;

			return 0;
		}

		case WM_LBUTTONDBLCLK:
		{
			m_compact = !m_compact;
			UpdateSize();
			return 0;
		}
	}

	return DefWindowProc(m_hwnd, msg, wParam, lParam);
}

void CJabberInfoFrame::LockUpdates()
{
	m_bLocked = true;
}

void CJabberInfoFrame::Update()
{
	m_bLocked = false;
	UpdateSize();
}

void CJabberInfoFrame::ReloadFonts()
{
	LOGFONT lfFont;

	FontID fontid = {0};
	fontid.cbSize = sizeof(fontid);
	lstrcpyA(fontid.group, "Jabber");
	lstrcpyA(fontid.name, "Frame title");
	m_clTitle = CallService(MS_FONT_GET, (WPARAM)&fontid, (LPARAM)&lfFont);
	m_hfntTitle = CreateFontIndirect(&lfFont);
	lstrcpyA(fontid.name, "Frame text");
	m_clText = CallService(MS_FONT_GET, (WPARAM)&fontid, (LPARAM)&lfFont);
	m_hfntText = CreateFontIndirect(&lfFont);

	ColourID colourid = {0};
	colourid.cbSize = sizeof(colourid);
	lstrcpyA(colourid.group, "Jabber");
	lstrcpyA(colourid.name, "Background");
	m_clBack = CallService(MS_COLOUR_GET, (WPARAM)&colourid, 0);

	UpdateSize();
}

void CJabberInfoFrame::UpdateSize()
{
	if (!m_hwnd) return;
	if (m_bLocked) return;

	int line_count = m_compact ? 1 : (m_pItems.getCount() - m_hiddenItemCount);
	int height = 2 * SZ_FRAMEPADDING + line_count * (GetSystemMetrics(SM_CYSMICON) + SZ_LINEPADDING) + (line_count - 1) * SZ_LINESPACING;

	if (CallService(MS_CLIST_FRAMES_GETFRAMEOPTIONS, MAKEWPARAM(FO_FLAGS, m_frameId), 0) & F_VISIBLE)
	{
		if (!ServiceExists(MS_SKIN_DRAWGLYPH))
		{
			// crazy resizing for clist_nicer...
			CallService(MS_CLIST_FRAMES_SHFRAME, m_frameId, 0);
			CallService(MS_CLIST_FRAMES_SETFRAMEOPTIONS, MAKEWPARAM(FO_HEIGHT, m_frameId), height);
			CallService(MS_CLIST_FRAMES_SHFRAME, m_frameId, 0);
		} else
		{
			CallService(MS_CLIST_FRAMES_SETFRAMEOPTIONS, MAKEWPARAM(FO_HEIGHT, m_frameId), height);
			RedrawWindow(m_hwnd, NULL, NULL, RDW_INVALIDATE);
		}
	} else
	{
		CallService(MS_CLIST_FRAMES_SETFRAMEOPTIONS, MAKEWPARAM(FO_HEIGHT, m_frameId), height);
	}
}

void CJabberInfoFrame::RemoveTooltip(int id)
{
	TOOLINFO ti = {0};
	ti.cbSize = sizeof(TOOLINFO);

	ti.hwnd = m_hwnd;
	ti.uId = id;
	SendMessage(m_hwndToolTip, TTM_DELTOOLW, 0, (LPARAM)&ti);
}

void CJabberInfoFrame::SetToolTip(int id, RECT *rc, TCHAR *pszText)
{
	TOOLINFO ti = {0};
	ti.cbSize = sizeof(TOOLINFO);

	ti.hwnd = m_hwnd;
	ti.uId = id;
	SendMessage(m_hwndToolTip, TTM_DELTOOLW, 0, (LPARAM)&ti);

	ti.uFlags = TTF_SUBCLASS;
	ti.hwnd = m_hwnd;
	ti.uId = id;
	ti.hinst = hInst;
	ti.lpszText = pszText;
	ti.rect = *rc;
	SendMessage(m_hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&ti);
}

void CJabberInfoFrame::PaintSkinGlyph(HDC hdc, RECT *rc, char **glyphs, COLORREF fallback)
{
	if (ServiceExists(MS_SKIN_DRAWGLYPH))
	{
		SKINDRAWREQUEST rq = {0};
		rq.hDC = hdc;
		rq.rcDestRect = *rc;
		rq.rcClipRect = *rc;

		for ( ; *glyphs; ++glyphs)
		{
			strncpy(rq.szObjectID, *glyphs, sizeof(rq.szObjectID));
			if (!CallService(MS_SKIN_DRAWGLYPH, (WPARAM)&rq, 0))
				return;
		}
	}

	if (fallback != 0xFFFFFFFF)
	{
		HBRUSH hbr = CreateSolidBrush(fallback);
		FillRect(hdc, rc, hbr);
		DeleteObject(hbr);
	}
}

void CJabberInfoFrame::PaintCompact(HDC hdc)
{
	RECT rc; GetClientRect(m_hwnd, &rc);
	char *glyphs[] = { "Main,ID=ProtoInfo", "Main,ID=EventArea", "Main,ID=StatusBar", NULL };
	PaintSkinGlyph(hdc, &rc, glyphs, m_clBack);

	HFONT hfntSave = (HFONT)SelectObject(hdc, m_hfntTitle);
	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, m_clTitle);

	int cx_icon = GetSystemMetrics(SM_CXSMICON);
	int cy_icon = GetSystemMetrics(SM_CYSMICON);

	int cx = rc.right - cx_icon - SZ_FRAMEPADDING;
	for (int i = m_pItems.getCount(); i--; )
	{
		CJabberInfoFrameItem &item = m_pItems[i];

		SetRect(&item.m_rcItem, 0, 0, 0, 0);
		if (!item.m_bShow) continue;
		if (!item.m_bCompact) continue;

		int depth = 0;
		for (char *p = item.m_pszName; p = strchr(p+1, '/'); ++depth) ;

		if (depth == 0)
		{
			if (item.m_hIcolibIcon)
			{
				HICON hIcon = (HICON)CallService(MS_SKIN2_GETICONBYHANDLE, 0, (LPARAM)item.m_hIcolibIcon);
				if (hIcon)
				{
					DrawIconEx(hdc, SZ_FRAMEPADDING, (rc.bottom-cy_icon)/2, hIcon, cx_icon, cy_icon, 0, NULL, DI_NORMAL);
					CallService(MS_SKIN2_RELEASEICON, (WPARAM)hIcon, 0);
				}
			}

			RECT rcText; SetRect(&rcText, cx_icon + SZ_FRAMEPADDING + SZ_ICONSPACING, 0, rc.right - SZ_FRAMEPADDING, rc.bottom);
			DrawText(hdc, item.m_pszText, lstrlen(item.m_pszText), &rcText, DT_NOPREFIX|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);
		} else
		{
			if (item.m_hIcolibIcon)
			{
				HICON hIcon = (HICON)CallService(MS_SKIN2_GETICONBYHANDLE, 0, (LPARAM)item.m_hIcolibIcon);
				if (hIcon)
				{
					SetRect(&item.m_rcItem, cx, (rc.bottom-cy_icon)/2, cx+cx_icon, (rc.bottom-cy_icon)/2+cy_icon);
					DrawIconEx(hdc, cx, (rc.bottom-cy_icon)/2, hIcon, cx_icon, cy_icon, 0, NULL, DI_NORMAL);
					cx -= cx_icon;

					CallService(MS_SKIN2_RELEASEICON, (WPARAM)hIcon, 0);

					SetToolTip(item.m_tooltipId, &item.m_rcItem, item.m_pszText);
				}
			}
		}
	}

	SelectObject(hdc, hfntSave);
}

void CJabberInfoFrame::PaintNormal(HDC hdc)
{
	RECT rc; GetClientRect(m_hwnd, &rc);
	char *glyphs[] = { "Main,ID=ProtoInfo", "Main,ID=EventArea", "Main,ID=StatusBar", NULL };
	PaintSkinGlyph(hdc, &rc, glyphs, m_clBack);

	HFONT hfntSave = (HFONT)SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
	SetBkMode(hdc, TRANSPARENT);

	int cx_icon = GetSystemMetrics(SM_CXSMICON);
	int cy_icon = GetSystemMetrics(SM_CYSMICON);
	int line_height = cy_icon + SZ_LINEPADDING;
	int cy = SZ_FRAMEPADDING;

	for (int i = 0; i < m_pItems.getCount(); ++i)
	{
		CJabberInfoFrameItem &item = m_pItems[i];
		
		if (!item.m_bShow)
		{
			SetRect(&item.m_rcItem, 0, 0, 0, 0);
			continue;
		}

		int cx = SZ_FRAMEPADDING;
		int depth = 0;
		for (char *p = item.m_pszName; p = strchr(p+1, '/'); cx += cx_icon) ++depth;

		SetRect(&item.m_rcItem, cx, cy, rc.right - SZ_FRAMEPADDING, cy + line_height);

		if (item.m_hIcolibIcon)
		{
			HICON hIcon = (HICON)CallService(MS_SKIN2_GETICONBYHANDLE, 0, (LPARAM)item.m_hIcolibIcon);
			if (hIcon)
			{
				DrawIconEx(hdc, cx, cy + (line_height-cy_icon)/2, hIcon, cx_icon, cy_icon, 0, NULL, DI_NORMAL);
				cx += cx_icon + SZ_ICONSPACING;

				CallService(MS_SKIN2_RELEASEICON, (WPARAM)hIcon, 0);
			}
		}

		SelectObject(hdc, depth ? m_hfntText : m_hfntTitle);
		SetTextColor(hdc, depth ? m_clText : m_clTitle);

		RECT rcText; SetRect(&rcText, cx, cy, rc.right - SZ_FRAMEPADDING, cy + line_height);
		DrawText(hdc, item.m_pszText, lstrlen(item.m_pszText), &rcText, DT_NOPREFIX|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);

		RemoveTooltip(item.m_tooltipId);

		cy += line_height + SZ_LINESPACING;
	}

	SelectObject(hdc, hfntSave);
}

void CJabberInfoFrame::CreateInfoItem(char *pszName, bool bCompact, LPARAM pUserData)
{
	CJabberInfoFrameItem item(pszName);
	if (CJabberInfoFrameItem *pItem = m_pItems.find(&item))
		return;

	CJabberInfoFrameItem *newItem = new CJabberInfoFrameItem(pszName, bCompact, pUserData);
	newItem->m_tooltipId = m_nextTooltipId++;
	m_pItems.insert(newItem);
	UpdateSize();
}

void CJabberInfoFrame::SetInfoItemCallback(char *pszName, void (CJabberProto::*onEvent)(CJabberInfoFrame_Event *))
{
	CJabberInfoFrameItem item(pszName);
	if (CJabberInfoFrameItem *pItem = m_pItems.find(&item))
	{
		pItem->m_onEvent = onEvent;
	}
}

void CJabberInfoFrame::UpdateInfoItem(char *pszName, HANDLE hIcolibIcon, TCHAR *pszText)
{
	CJabberInfoFrameItem item(pszName);
	if (CJabberInfoFrameItem *pItem = m_pItems.find(&item))
		pItem->SetInfo(hIcolibIcon, pszText);
	if (m_hwnd)
		RedrawWindow(m_hwnd, NULL, NULL, RDW_INVALIDATE);
}

void CJabberInfoFrame::ShowInfoItem(char *pszName, bool bShow)
{
	bool bUpdate = false;
	int length = strlen(pszName);
	for (int i = 0; i < m_pItems.getCount(); ++i)
		if ((m_pItems[i].m_bShow != bShow) && !strncmp(m_pItems[i].m_pszName, pszName, length))
		{
			m_pItems[i].m_bShow = bShow;
			m_hiddenItemCount += bShow ? -1 : 1;
			bUpdate = true;
		}

	if (bUpdate)
		UpdateSize();
}

void CJabberInfoFrame::RemoveInfoItem(char *pszName)
{
	bool bUpdate = false;
	int length = strlen(pszName);
	for (int i = 0; i < m_pItems.getCount(); ++i)
		if (!strncmp(m_pItems[i].m_pszName, pszName, length))
		{
			if (!m_pItems[i].m_bShow) --m_hiddenItemCount;
			RemoveTooltip(m_pItems[i].m_tooltipId);
			m_pItems.remove(i);
			bUpdate = true;
			--i;
		}

	if (bUpdate)
		UpdateSize();
}
