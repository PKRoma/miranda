/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2007 Miranda ICQ/IM project, 
all portions of this codebase are copyrighted to the people 
listed in contributors.txt.

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
#include "commonheaders.h"
#include <m_hotkeys.h>

int InitSkinHotKeys(void);

int hkCloseMiranda(WPARAM wParam,LPARAM lParam)
{
	CallService("CloseAction",0,0);
	return 0;
}

int hkRestoreStatus(WPARAM wParam,LPARAM lParam)
{
	int nStatus = DBGetContactSettingWord(NULL, "CList", "Status", ID_STATUS_OFFLINE);
	if (nStatus != ID_STATUS_OFFLINE)
		PostMessage(pcli->hwndContactList, WM_COMMAND, nStatus, 0);

	return 0;
}
#ifndef FORCE_HOTKEY_IN_MODERN


int InitSkinHotKeys(void)
{
	HOTKEYDESC shk;

	CreateServiceFunction("CLIST/HK/CloseMiranda",hkCloseMiranda);
	CreateServiceFunction("CLIST/HK/RestoreStatus",hkRestoreStatus);

	shk.cbSize=sizeof(shk);

	shk.pszDescription="Close Miranda";
	shk.pszName="CloseMiranda";
	shk.pszSection="Main";
	shk.pszService="CLIST/HK/CloseMiranda";
	shk.DefHotKey=0;
	CallService(MS_HOTKEY_REGISTER,0,(LPARAM)&shk);	

	shk.pszDescription="Restore last status";
	shk.pszName="RestoreLastStatus";
	shk.pszSection="Status";
	shk.pszService="CLIST/HK/RestoreStatus";
	shk.DefHotKey=0;
	CallService(MS_HOTKEY_REGISTER,0,(LPARAM)&shk);	

	return 0;
}

void UninitSkinHotKeys(void)
{
}
#else

int RegistersAllHotkey(HWND hwnd); 
int cliHotKeysUnregister(HWND hwnd);
static int ServiceSkinPlayHotKey(WPARAM wParam, LPARAM lParam);

static ATOM aHide = 0;
static ATOM aRead = 0;
static ATOM aSearch = 0;
static ATOM aOpts = 0;

typedef struct {
	char  *pszService,*tempFile,*m_cacheTName;
   TCHAR *section,*description;
	int   DefHotKey;
	ATOM  aAtom;

} HotKeyItem,*pHotKeyItem;
static HotKeyItem *HotKeyList=NULL;
static int HotKeyCount;
static HANDLE hPlayEvent=NULL;
pHotKeyItem GetHotKeyItemByAtom(ATOM a);

static void WordToModAndVk(WORD w,UINT *mod,UINT *vk)
{
	*mod=0;
	if(HIBYTE(w)&HOTKEYF_CONTROL) *mod|=MOD_CONTROL;
	if(HIBYTE(w)&HOTKEYF_SHIFT) *mod|=MOD_SHIFT;
	if(HIBYTE(w)&HOTKEYF_ALT) *mod|=MOD_ALT;
	if(HIBYTE(w)&HOTKEYF_EXT) *mod|=MOD_WIN;
	*vk=LOBYTE(w);
}

int hkHideShow(WPARAM wParam,LPARAM lParam)
{
	pcli->pfnShowHide(0,0);
	return 0;
}

int hkSearch(WPARAM wParam,LPARAM lParam)
{
	DBVARIANT dbv={0};
	if(!DBGetContactSettingString(NULL,"CList","SearchUrl",&dbv)) {
		CallService(MS_UTILS_OPENURL,DBGetContactSettingByte(NULL,"CList","HKSearchNewWnd",0),(LPARAM)dbv.pszVal);
		DBFreeVariant(&dbv);
	}
	return 0;
}

int hkRead(WPARAM wParam,LPARAM lParam)
{
	if(pcli->pfnEventsProcessTrayDoubleClick(0)==0) return TRUE;
	SetForegroundWindow(pcli->hwndContactList);
	SetFocus(pcli->hwndContactList);
	return 0;
}

int hkOpts(WPARAM wParam,LPARAM lParam)
{
	CallService("Options/OptionsCommand",0, 0);
	return 0;
}

int hkAllOffline(WPARAM wParam,LPARAM lParam)
{
	PROTOCOLDESCRIPTOR** ppProtoDesc;
	int nProtoCount;
	int nProto;


	CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM)&nProtoCount, (LPARAM)&ppProtoDesc);
	for (nProto = 0; nProto < nProtoCount; nProto++)
	{
		if (ppProtoDesc[nProto]->type == PROTOTYPE_PROTOCOL)
			CallProtoService(ppProtoDesc[nProto]->szName, PS_SETSTATUS, ID_STATUS_OFFLINE, 0);
	}
	return 0;
}


int cliHotKeysRegister(HWND hwnd)
{
	//	UINT mod,vk;
	SKINHOTKEYDESCEX  shk;

	InitSkinHotKeys();

	CreateServiceFunction("CLIST/HK/SHOWHIDE",hkHideShow);
	CreateServiceFunction("CLIST/HK/Opts",hkOpts);
	CreateServiceFunction("CLIST/HK/Search",hkSearch);
	CreateServiceFunction("CLIST/HK/Read",hkRead);
	CreateServiceFunction("CLIST/HK/CloseMiranda",hkCloseMiranda);

	CreateServiceFunction("CLIST/HK/RestoreStatus",hkRestoreStatus);
	CreateServiceFunction("CLIST/HK/AllOffline",hkAllOffline);

	shk.cbSize=sizeof(shk);
	shk.pszDescription="Show Hide Contact List";
	shk.pszName="ShowHide";
	shk.pszSection="Main";
	shk.pszService="CLIST/HK/SHOWHIDE";
	shk.DefHotKey=833;
	CallService(MS_SKIN_ADDHOTKEY,0,(LPARAM)&shk);

	shk.pszDescription="Read Message";
	shk.pszName="ReadMessage";
	shk.pszSection="Main";
	shk.pszService="CLIST/HK/Read";
	shk.DefHotKey=841;
	CallService(MS_SKIN_ADDHOTKEY,0,(LPARAM)&shk);

	shk.pszDescription="Search in site";
	shk.pszName="SearchInWeb";
	shk.pszSection="Main";
	shk.pszService="CLIST/HK/Search";
	shk.DefHotKey=846;
	CallService(MS_SKIN_ADDHOTKEY,0,(LPARAM)&shk);	

	shk.pszDescription="Open Options Page";
	shk.pszName="ShowOptions";
	shk.pszSection="Main";
	shk.pszService="CLIST/HK/Opts";
	shk.DefHotKey=847;
	CallService(MS_SKIN_ADDHOTKEY,0,(LPARAM)&shk);	

	shk.pszDescription="Open Find User Dialog";
	shk.pszName="FindUsers";
	shk.pszSection="Main";
	shk.pszService="FindAdd/FindAddCommand";
	shk.DefHotKey=838;
	CallService(MS_SKIN_ADDHOTKEY,0,(LPARAM)&shk);	

	shk.pszDescription="Close Miranda";
	shk.pszName="CloseMiranda";
	shk.pszSection="Main";
	shk.pszService="CLIST/HK/CloseMiranda";
	shk.DefHotKey=0;
	CallService(MS_SKIN_ADDHOTKEY,0,(LPARAM)&shk);	

	shk.pszDescription="Restore last status";
	shk.pszName="RestoreLastStatus";
	shk.pszSection="Status";
	shk.pszService="CLIST/HK/RestoreStatus";
	shk.DefHotKey=0;
	CallService(MS_SKIN_ADDHOTKEY,0,(LPARAM)&shk);	

	shk.pszDescription="Set All Offline";
	shk.pszName="AllOffline";
	shk.pszSection="Status";
	shk.pszService="CLIST/HK/AllOffline";
	shk.DefHotKey=0;
	CallService(MS_SKIN_ADDHOTKEY,0,(LPARAM)&shk);	

	RegistersAllHotkey(hwnd);
	return 0;
}

int RegistersAllHotkey(HWND hwnd)
{
	UINT mod,vk;
	int i;
	pHotKeyItem phi;

	cliHotKeysUnregister(hwnd);
	for (i=0;i<HotKeyCount;i++)
	{
		phi=&HotKeyList[i];
		if(DBGetContactSettingByte(NULL, "SkinHotKeysOff", phi->m_cacheTName, 0)==0) {
			if(!phi->aAtom) phi->aAtom=GlobalAddAtomA(phi->m_cacheTName);
			WordToModAndVk((WORD)DBGetContactSettingWord(NULL,"SkinHotKeys",phi->m_cacheTName,0),&mod,&vk);
			if (vk!=0) RegisterHotKey(hwnd,phi->aAtom,mod,vk); 
		}
	}
	return 0;
}

int cliHotKeysUnregister(HWND hwnd)
{
	//	UINT mod,vk;
	int i;
	pHotKeyItem phi;

	for (i=0;i<HotKeyCount;i++)
	{
		phi=&HotKeyList[i];
		if (phi->aAtom)
		{
			UnregisterHotKey(hwnd, phi->aAtom);
			GlobalDeleteAtom(phi->aAtom);
			phi->aAtom=(ATOM)NULL;
		}
	}
	return 0;
}


int cliHotKeysProcess(HWND hwnd,WPARAM wParam,LPARAM lParam)
{
	return TRUE;
} 

int cliHotkeysProcessMessage(WPARAM wParam,LPARAM lParam)
{
	MSG *msg=(MSG*)wParam;
	switch(msg->message) {
		case WM_CREATE:
			cliHotKeysRegister(msg->hwnd);
			return 0;
		case WM_HOTKEY:
			//*((LRESULT*)lParam)=HotKeysProcess(msg->hwnd,msg->wParam,msg->lParam);
			{
				pHotKeyItem phi;
				phi=GetHotKeyItemByAtom((ATOM)msg->wParam);
				*((LRESULT*)lParam)=ServiceSkinPlayHotKey(0,(LPARAM)phi->m_cacheTName);
			}
			return TRUE;
		case WM_DESTROY:
			cliHotKeysUnregister(msg->hwnd);
			return 0;
	}

	return FALSE;
}

////////////////////NEWWWWWWWWWW

pHotKeyItem GetHotKeyItemByAtom(ATOM a)
{
	int i;
	if (HotKeyCount==0) return NULL;
	for (i=0;i<HotKeyCount;i++)
	{
		if (HotKeyList[i].aAtom==a) return (&HotKeyList[i]);
	}
	return NULL;
}
pHotKeyItem GetHotKeyItemByName(char *name)
{
	int i;
	if (HotKeyCount==0) return NULL;
	for (i=0;i<HotKeyCount;i++)
	{
		if ((name)&&!mir_strcmp(HotKeyList[i].m_cacheTName,name)) return (&HotKeyList[i]);
	}
	return NULL;
}

static int ServiceSkinAddNewHotKey(WPARAM wParam,LPARAM lParam)
{
	SKINHOTKEYDESCEX *ssd=(SKINHOTKEYDESCEX*)lParam;

	if (ssd->cbSize!=sizeof(SKINHOTKEYDESCEX))
		return 0;
	HotKeyList=(HotKeyItem*)realloc(HotKeyList,sizeof(HotKeyItem)*(HotKeyCount+1));

	HotKeyList[HotKeyCount].m_cacheTName = mir_strdup(ssd->pszName);
	HotKeyList[HotKeyCount].description = (TCHAR*)CallService(MS_LANGPACK_PCHARTOTCHAR,0,(LPARAM)ssd->pszDescription);
	HotKeyList[HotKeyCount].section = (TCHAR*)CallService(MS_LANGPACK_PCHARTOTCHAR,0,(LPARAM)( ssd->cbSize==sizeof(SKINHOTKEYDESCEX) ? ssd->pszSection : "Other" ));
	HotKeyList[HotKeyCount].pszService=mir_strdup(ssd->pszService);
	HotKeyList[HotKeyCount].DefHotKey=ssd->DefHotKey;
	HotKeyList[HotKeyCount].aAtom=0;
	HotKeyList[HotKeyCount].tempFile=NULL;

	if (ssd->DefHotKey) {
		DBVARIANT dbv={0};

		if (DBGetContactSetting(NULL, "SkinHotKeys", HotKeyList[HotKeyCount].m_cacheTName, &dbv)) {
			DBWriteContactSettingWord(NULL, "SkinHotKeys", HotKeyList[HotKeyCount].m_cacheTName, (WORD)ssd->DefHotKey);
		}
		else DBFreeVariant(&dbv);
	}

	HotKeyCount++;
	if (pcli->hwndContactList)  RegistersAllHotkey(pcli->hwndContactList);
	return 0;
}

static int SkinPlayHotKeyDefault(WPARAM wParam, LPARAM lParam)
{
	char * pszFile = (char *) lParam;
	if ( pszFile ) {
		//		PlayHotKey(pszFile, NULL, SND_ASYNC | SND_FILENAME | SND_NOWAIT);
	}
	return 0;
}

static int ServiceSkinPlayHotKey(WPARAM wParam, LPARAM lParam)
{
	char *pszName=(char *)lParam;

	pHotKeyItem phi = GetHotKeyItemByName(pszName);
	if (phi==NULL) return 1;

	if (DBGetContactSettingByte(NULL, "SkinHotKeysOff", phi->m_cacheTName, 0)==0) {
		//				DBVARIANT dbv={0};

		//				if (DBGetContactSetting(NULL, "SkinHotKeys", pszHotKeyName, &dbv)==0) {
		//                    char szFull[MAX_PATH];

		if (phi->pszService) CallService(phi->pszService, 0,0);
		return 0;
		//DBFreeVariant(&dbv);
		//				}
	}

	return 1;
}

static HTREEITEM FindNamedTreeItemAtRoot(HWND hwndTree,const TCHAR *name)
{
	TVITEM tvi;
	TCHAR str[128]={0};

	tvi.mask=TVIF_TEXT;
	tvi.pszText=str;
	tvi.cchTextMax=SIZEOF(str);
	tvi.hItem=TreeView_GetRoot(hwndTree);
	while(tvi.hItem!=NULL) {
		TreeView_GetItem(hwndTree,&tvi);
		if(!lstrcmpi(str,name)) 
			return tvi.hItem;

		tvi.hItem=TreeView_GetNextSibling(hwndTree,tvi.hItem);
	}
	return NULL;
}

#define DM_REBUILD_STREE (WM_USER+1)
#define DM_HIDEPANE      (WM_USER+2)
#define DM_SHOWPANE      (WM_USER+3)
BOOL CALLBACK DlgProcHotKeyOpts2(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND hwndTree = NULL;
	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		hwndTree = GetDlgItem(hwndDlg, IDC_HOTKEYTREE);
		SetWindowLong(hwndTree,GWL_STYLE,GetWindowLong(hwndTree,GWL_STYLE)|TVS_NOHSCROLL|TVS_CHECKBOXES);
		SendMessage(hwndDlg, DM_HIDEPANE, 0, 0);
		SendMessage(hwndDlg, DM_REBUILD_STREE, 0, 0);
		TreeView_SetItemState(hwndTree, 0, TVIS_SELECTED, TVIS_SELECTED);
		return TRUE;

	case DM_REBUILD_STREE:
		{
			TVINSERTSTRUCT tvis;
			int i;
			TreeView_SelectItem(hwndTree ,NULL);
			CLUI_ShowWindowMod(hwndTree,SW_HIDE);
			TreeView_DeleteAllItems(hwndTree);
			tvis.hParent=NULL;
			tvis.hInsertAfter=TVI_SORT;
			tvis.item.mask=TVIF_TEXT|TVIF_STATE|TVIF_PARAM;
			tvis.item.state=tvis.item.stateMask=TVIS_EXPANDED;
			for(i=0;i<HotKeyCount;i++) {
				tvis.item.stateMask=TVIS_EXPANDED;
				tvis.item.state=TVIS_EXPANDED;
				tvis.hParent=NULL;
				tvis.hParent=FindNamedTreeItemAtRoot(hwndTree,HotKeyList[i].section);
				if(tvis.hParent==NULL) {
					tvis.item.lParam=-1;
					tvis.item.pszText=HotKeyList[i].section;
					tvis.hParent = tvis.item.hItem = TreeView_InsertItem(hwndTree,&tvis);
					tvis.item.stateMask=TVIS_STATEIMAGEMASK;
					tvis.item.state=INDEXTOSTATEIMAGEMASK(0);
					TreeView_SetItem(hwndTree,&tvis.item);
				}
				tvis.item.stateMask=TVIS_STATEIMAGEMASK;
				tvis.item.state=INDEXTOSTATEIMAGEMASK(!DBGetContactSettingByte(NULL,"SkinHotKeysOff",HotKeyList[i].m_cacheTName,0)?2:1);
				tvis.item.lParam=i;
				tvis.item.pszText=HotKeyList[i].description;
				TreeView_InsertItem(hwndTree,&tvis);
			}
			{
				TVITEM tvi;
				tvi.hItem = TreeView_GetRoot(hwndTree);
				while(tvi.hItem!=NULL) {
					tvi.mask = TVIF_PARAM|TVIF_HANDLE|TVIF_STATE;
					TreeView_GetItem(hwndTree, &tvi);
					if (tvi.lParam==-1) {
						TreeView_SetItemState(hwndTree, tvi.hItem, INDEXTOSTATEIMAGEMASK(0), TVIS_STATEIMAGEMASK);
					}
					tvi.hItem=TreeView_GetNextSibling(hwndTree,tvi.hItem);
				}
			}
			CLUI_ShowWindowMod(hwndTree, SW_SHOW);
			break;
		}
	case DM_HIDEPANE:
		CLUI_ShowWindowMod(GetDlgItem(hwndDlg, IDC_SGROUP), SW_HIDE);
		CLUI_ShowWindowMod(GetDlgItem(hwndDlg, IDC_NAME), SW_HIDE);
		CLUI_ShowWindowMod(GetDlgItem(hwndDlg, IDC_NAMEVAL), SW_HIDE);
		CLUI_ShowWindowMod(GetDlgItem(hwndDlg, IDC_SLOC), SW_HIDE);
		CLUI_ShowWindowMod(GetDlgItem(hwndDlg, IDC_LOCATION), SW_HIDE);
		CLUI_ShowWindowMod(GetDlgItem(hwndDlg, IDC_CHANGE), SW_HIDE);
		CLUI_ShowWindowMod(GetDlgItem(hwndDlg, IDC_PREVIEW), SW_HIDE);
		CLUI_ShowWindowMod(GetDlgItem(hwndDlg, IDC_SETHOTKEY), SW_HIDE);
		CLUI_ShowWindowMod(GetDlgItem(hwndDlg, IDC_HKTITLE), SW_HIDE);


		CLUI_ShowWindowMod(GetDlgItem(hwndDlg, IDC_GETMORE), SW_HIDE);
		break;
	case DM_SHOWPANE:
		CLUI_ShowWindowMod(GetDlgItem(hwndDlg, IDC_SGROUP), SW_SHOW);
		CLUI_ShowWindowMod(GetDlgItem(hwndDlg, IDC_NAME), SW_SHOW);
		CLUI_ShowWindowMod(GetDlgItem(hwndDlg, IDC_NAMEVAL), SW_SHOW);
		//CLUI_ShowWindowMod(GetDlgItem(hwndDlg, IDC_SLOC), SW_SHOW);
		//CLUI_ShowWindowMod(GetDlgItem(hwndDlg, IDC_LOCATION), SW_SHOW);
		//CLUI_ShowWindowMod(GetDlgItem(hwndDlg, IDC_CHANGE), SW_SHOW);
		//CLUI_ShowWindowMod(GetDlgItem(hwndDlg, IDC_PREVIEW), SW_SHOW);
		//CLUI_ShowWindowMod(GetDlgItem(hwndDlg, IDC_GETMORE), SW_SHOW);
		CLUI_ShowWindowMod(GetDlgItem(hwndDlg, IDC_SETHOTKEY), SW_SHOW);
		CLUI_ShowWindowMod(GetDlgItem(hwndDlg, IDC_HKTITLE), SW_SHOW);
		break;
	case WM_COMMAND:
		if(LOWORD(wParam)==IDC_PREVIEW) {
			TVITEM tvi;
			HTREEITEM hti;

			ZeroMemory(&tvi,sizeof(tvi));
			ZeroMemory(&hti,sizeof(hti));
			hti=TreeView_GetSelection(hwndTree);
			if (hti==NULL) break;
			tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM|TVIF_TEXT;
			tvi.hItem = hti;
			if (TreeView_GetItem(hwndTree, &tvi)==FALSE) break;
			if (tvi.lParam==-1) break;
			if (HotKeyList[tvi.lParam].tempFile) 
				NotifyEventHooks(hPlayEvent, 0, (LPARAM)HotKeyList[tvi.lParam].tempFile);
			else {
				DBVARIANT dbv={0};
				if(!DBGetContactSettingString(NULL,"SkinHotKeys",HotKeyList[tvi.lParam].m_cacheTName,&dbv)) {
					char szPathFull[MAX_PATH];

					CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)dbv.pszVal, (LPARAM)szPathFull);
					NotifyEventHooks(hPlayEvent, 0, (LPARAM)szPathFull);
					DBFreeVariant(&dbv);
				}
			}
			break;
		}
		if(LOWORD(wParam)==IDC_CHANGE) {
			char str[MAX_PATH], strFull[MAX_PATH];
			OPENFILENAMEA ofn;
			TVITEM tvi;
			HTREEITEM hti;

			ZeroMemory(&tvi,sizeof(tvi));
			ZeroMemory(&hti,sizeof(hti));
			hti=TreeView_GetSelection(hwndTree);
			if (hti==NULL) break;
			tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM|TVIF_TEXT;
			tvi.hItem = hti;
			if (TreeView_GetItem(hwndTree, &tvi)==FALSE) break;
			if (tvi.lParam==-1) break;
			str[0] = 0;
			if (HotKeyList[tvi.lParam].tempFile) {
				_snprintf(strFull, SIZEOF(strFull), "%s", HotKeyList[tvi.lParam].tempFile);
			}
			else {
				if (DBGetContactSettingByte(NULL, "SkinHotKeysOff", HotKeyList[tvi.lParam].m_cacheTName, 0)==0) {
					DBVARIANT dbv={0};

					if (DBGetContactSetting(NULL, "SkinHotKeys", HotKeyList[tvi.lParam].m_cacheTName, &dbv)==0) {  
						DBFreeVariant(&dbv);
					}
				}
			}
			_snprintf(strFull, SIZEOF(strFull), "%s", HotKeyList[tvi.lParam].tempFile?HotKeyList[tvi.lParam].tempFile:"");
			CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)strFull, (LPARAM)str);
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
			ofn.hwndOwner = GetParent(hwndDlg);
			ofn.hInstance = NULL;
			ofn.lpstrFilter = "Wave Files (*.wav)\0*.WAV\0All Files (*)\0*\0";
			ofn.lpstrFile = str;
			ofn.Flags = OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
			ofn.nMaxFile = SIZEOF(str);
			ofn.nMaxFileTitle = MAX_PATH;
			ofn.lpstrDefExt = "wav";
			if(!GetOpenFileNameA(&ofn)) break;
			CallService(MS_UTILS_PATHTORELATIVE, (WPARAM)str, (LPARAM)strFull);
			HotKeyList[tvi.lParam].tempFile = _strdup(strFull);
			SetDlgItemTextA(hwndDlg, IDC_LOCATION, strFull);
		}
		if(LOWORD(wParam)==IDC_GETMORE) {
			CallService(MS_UTILS_OPENURL,1,(LPARAM)"http://www.miranda-im.org/download/index.php?action=display&id=5");
			break;
		}
		if(LOWORD(wParam)==IDC_SETHOTKEY) {

			//				char str[MAX_PATH], strFull[MAX_PATH];
			//				OPENFILENAME ofn;
			TVITEM tvi;
			HTREEITEM hti;

			ZeroMemory(&tvi,sizeof(tvi));
			ZeroMemory(&hti,sizeof(hti));
			hti=TreeView_GetSelection(hwndTree);
			if (hti==NULL) break;
			tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM|TVIF_TEXT;
			tvi.hItem = hti;
			if (TreeView_GetItem(hwndTree, &tvi)==FALSE) break;
			if (tvi.lParam==-1) break;
			HotKeyList[tvi.lParam].DefHotKey=(WORD)SendDlgItemMessage(hwndDlg,IDC_SETHOTKEY,HKM_GETHOTKEY,0,0);
			DBWriteContactSettingWord(NULL,"SkinHotKeys",HotKeyList[tvi.lParam].m_cacheTName,(WORD)SendDlgItemMessage(hwndDlg,IDC_SETHOTKEY,HKM_GETHOTKEY,0,0));

		}

		SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
		break;
	case WM_NOTIFY:
		switch(((LPNMHDR)lParam)->idFrom) {
		case 0:
			switch (((LPNMHDR)lParam)->code) {
			case PSN_APPLY:
				{	
					int i;
					for(i=0;i<HotKeyCount;i++) {
						if(HotKeyList[i].tempFile)
							DBWriteContactSettingWord(NULL,"SkinHotKeys",HotKeyList[i].m_cacheTName,(WORD)HotKeyList[i].DefHotKey);
					}
					{
						TVITEM tvi,tvic;
						tvi.hItem = TreeView_GetRoot(hwndTree);
						while(tvi.hItem!=NULL) {
							tvi.mask = TVIF_PARAM|TVIF_HANDLE|TVIF_STATE;
							TreeView_GetItem(hwndTree, &tvi);
							if (tvi.lParam==-1) {
								tvic.hItem = TreeView_GetChild(hwndTree, tvi.hItem);
								while(tvic.hItem!=NULL) {
									tvic.mask = TVIF_PARAM|TVIF_HANDLE|TVIF_STATE;
									TreeView_GetItem(hwndTree, &tvic);
									if(((tvic.state&TVIS_STATEIMAGEMASK)>>12==2)) {
										DBCONTACTGETSETTING cgs;
										cgs.szModule="SkinHotKeysOff";
										cgs.szSetting=HotKeyList[tvic.lParam].m_cacheTName;
										CallService(MS_DB_CONTACT_DELETESETTING,(WPARAM)(HANDLE)NULL,(LPARAM)&cgs);
									}
									else {
										DBWriteContactSettingByte(NULL,"SkinHotKeysOff",HotKeyList[tvic.lParam].m_cacheTName,1);
									}
									tvic.hItem=TreeView_GetNextSibling(hwndTree,tvic.hItem);
								}
							}
							tvi.hItem=TreeView_GetNextSibling(hwndTree,tvi.hItem);
						}
					}
					RegistersAllHotkey(pcli->hwndContactList);
					return TRUE;
				}
			}
			break;
		case IDC_HOTKEYTREE:
			switch(((NMHDR*)lParam)->code) {
			case TVN_SELCHANGEDA:
			case TVN_SELCHANGEDW:
				{
					NMTREEVIEW *pnmtv = (NMTREEVIEW*)lParam;
					TVITEM tvi = pnmtv->itemNew;

					if (tvi.lParam==-1) {
						SendMessage(hwndDlg, DM_HIDEPANE, 0, 0);
					}
					else {
						TCHAR buf[256];
						mir_sntprintf(buf, SIZEOF(buf), _T("%s: %s"), HotKeyList[tvi.lParam].section, HotKeyList[tvi.lParam].description);
						SetDlgItemText(hwndDlg, IDC_NAMEVAL, buf);
						SendDlgItemMessage(hwndDlg,IDC_SETHOTKEY,HKM_SETHOTKEY,DBGetContactSettingWord(NULL,"SkinHotKeys",HotKeyList[tvi.lParam].m_cacheTName,HotKeyList[tvi.lParam].DefHotKey ),0);

						SetDlgItemText(hwndDlg, IDC_LOCATION, TranslateT("<not specified>"));
						SendMessage(hwndDlg, DM_SHOWPANE, 0, 0);
					}
					break;
				}
			case NM_CLICK:
				{
					TVHITTESTINFO hti;
					hti.pt.x=(short)LOWORD(GetMessagePos());
					hti.pt.y=(short)HIWORD(GetMessagePos());
					ScreenToClient(((LPNMHDR)lParam)->hwndFrom,&hti.pt);
					if(TreeView_HitTest(((LPNMHDR)lParam)->hwndFrom,&hti)) {
						if (hti.flags&TVHT_ONITEM) {
							if(hti.flags&TVHT_ONITEMSTATEICON)
								SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
						}
					}
					break;
				}
			}
			break;
		}
		break;
	}
	return FALSE;
}

int InitSkinHotKeys(void)
{
	HotKeyList=NULL;
	HotKeyCount=0;
	CreateServiceFunction(MS_SKIN_ADDHOTKEY,ServiceSkinAddNewHotKey);
	CreateServiceFunction(MS_SKIN_PLAYHOTKEY,ServiceSkinPlayHotKey);
	//	hPlayEvent=CreateHookableEvent(ME_SKIN_PLAYINGHotKey);
	//	SetHookDefaultForHookableEvent(hPlayEvent, SkinPlayHotKeyDefault);
	return 0;
}

void UninitSkinHotKeys(void)
{
	int i;
    if (!HotKeyList) return;
	for(i=0;i<HotKeyCount;i++) {
		mir_free_and_nill(HotKeyList[i].m_cacheTName);
		mir_free_and_nill(HotKeyList[i].section);
		mir_free_and_nill(HotKeyList[i].description);
		mir_free_and_nill(HotKeyList[i].pszService);

		//		if (HotKeyList[i].tempFile) free(HotKeyList[i].tempFile);
	}
	if(HotKeyCount) free(HotKeyList);
}
#endif
