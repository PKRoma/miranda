/*
Miranda ICQ: the free icq client for MS Windows 
Copyright (C) 2000-2  Richard Hughes, Roland Rabien & Tristan Van de Vreede

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
#include "../../core/commonheaders.h"

static int hLoaded;

struct PluginDlgData {
	int bSortAscending;
	int iLastColumnSortIndex;
	HWND hwndList;
	HBITMAP hBmpSortUp,hBmpSortDown;
};

static int CALLBACK PluginSearchResultsCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) {
	struct PluginDlgData *dat=(struct PluginDlgData*)lParamSort;
    static LVFINDINFO lvf;
    static int nItem1, nItem2;
    static char buf1[MAX_PATH], buf2[MAX_PATH];
	int sortMultiplier;
	int sortCol;

	sortMultiplier=dat->bSortAscending?1:-1;
	sortCol=dat->iLastColumnSortIndex;
    lvf.flags = LVFI_PARAM;
    lvf.lParam = lParam1;
    nItem1 = ListView_FindItem(dat->hwndList,-1,&lvf);
    lvf.lParam = lParam2;
    nItem2 = ListView_FindItem(dat->hwndList,-1,&lvf);
    ListView_GetItemText(dat->hwndList,nItem1,sortCol,buf1,sizeof(buf1));
    ListView_GetItemText(dat->hwndList,nItem2,sortCol,buf2,sizeof(buf2));
	switch(sortCol) {
		case 3:
		{
			int i1,i2;
			char *s;
			s = buf1;
			while(s) {
				if (!isdigit(*s)) break;
				s++;
			}
			*s='\0';
			i1 = atoi(buf1);
			s = buf2;
			while(s) {
				if (!isdigit(*s)) break;
				s++;
			}
			*s='\0';
			i2 = atoi(buf2);
			if (i1>i2) return sortMultiplier;
			if (i1<i2) return sortMultiplier*-1;
			return 0;
		}
		default: return stricmp(buf1,buf2)*sortMultiplier;
	}
	return 0;
}

static PLUGININFO *CopyPluginInfo(PLUGININFO *piSrc)
{
	PLUGININFO *pi;
	if(piSrc==NULL) return NULL;
	pi=(PLUGININFO*)malloc(sizeof(PLUGININFO));
	*pi=*piSrc;
	if(pi->author) pi->author=_strdup(pi->author);
	if(pi->authorEmail) pi->authorEmail=_strdup(pi->authorEmail);
	if(pi->copyright) pi->copyright=_strdup(pi->copyright);
	if(pi->description) pi->description=_strdup(pi->description);
	if(pi->homepage) pi->homepage=_strdup(pi->homepage);
	if(pi->shortName) pi->shortName=_strdup(pi->shortName);
	return pi;
}

static void FreePluginInfo(PLUGININFO *pi)
{
	if(pi->author) free(pi->author);
	if(pi->authorEmail) free(pi->authorEmail);
	if(pi->copyright) free(pi->copyright);
	if(pi->description) free(pi->description);
	if(pi->homepage) free(pi->homepage);
	if(pi->shortName) free(pi->shortName);
	free(pi);
}

/* This goes thru the database module "PluginsDisabled"
and deletes any matching plugin names (without case check)
since the plugin loader now ignores case too */

struct PLUGINDI {
	char *szThis;
	char **szSettings;
	int dwCount;
};

int EnumDisableInfoForPlugin(const char *szSetting,LPARAM lParam)
{
	struct PLUGINDI *pdi=(struct PLUGINDI*)lParam;
	if (pdi && lParam) {
		if (!strcmpi(szSetting,pdi->szThis)) {
			pdi->szSettings=realloc(pdi->szSettings,(pdi->dwCount+1)*sizeof(char*));
			pdi->szSettings[pdi->dwCount++]=_strdup(szSetting);
		} //if
	} //if
	return 0;
}

void DeletePluginDisableInfo(char *filename)
{	
	DBCONTACTENUMSETTINGS cns;
	struct PLUGINDI pdi;

	cns.pfnEnumProc=EnumDisableInfoForPlugin;
	cns.lParam=(LPARAM)&pdi;
	cns.szModule="PluginDisable";
	cns.ofsSettings=0;

	memset(&pdi,0,sizeof(pdi));
	pdi.szThis=filename;

	CallService(MS_DB_CONTACT_ENUMSETTINGS,0,(LPARAM)&cns);
	if (pdi.szSettings) {
		int i;
		for (i=0;i<pdi.dwCount;i++) {
			DBDeleteContactSetting(NULL,"PluginDisable",pdi.szSettings[i]);
			free(pdi.szSettings[i]);
		} //for
		free(pdi.szSettings);
	} //if
	return;
}

/* This goes thru the database to find a plugin name (without case check)
returns info about if the DB has information if the plugin
should be disabled or not. */

int EnumDisabledPluginSettings(const char *szSetting,LPARAM lParam)
{
	char *sz=*((char**)lParam);
	if (sz && !strcmpi(szSetting,sz)) {
		*((char**)lParam)=NULL;
		return 1;
	}
	return 0;
}

static int IsPluginDisabled(char *sz)
{
	DBCONTACTENUMSETTINGS cns;
	cns.pfnEnumProc=EnumDisabledPluginSettings;
	cns.lParam=(LPARAM)&sz;
	cns.szModule="PluginDisable";
	cns.ofsSettings=0;
	CallService(MS_DB_CONTACT_ENUMSETTINGS,0,(LPARAM)&cns);
	return (int)sz;
}

static void SetDetailsFields(HWND hwndDlg,int iItem)
{
	char str[256],szPluginFile[MAX_PATH];
	PLUGININFO *pluginInfo;
	LVITEM lvi;

	lvi.iImage = 0;
	if(iItem==-1) {pluginInfo=NULL; szPluginFile[0]='\0';}
	else {
		
		lvi.iItem=iItem;
		lvi.iSubItem=0;
		lvi.mask=LVIF_PARAM|LVIF_TEXT|LVIF_IMAGE;
		lvi.pszText=szPluginFile;
		lvi.cchTextMax=sizeof(szPluginFile);
		if(!ListView_GetItem(GetDlgItem(hwndDlg,IDC_PLUGINLIST),&lvi)) {
			szPluginFile[0]='\0';
			pluginInfo=NULL;
		}
		else pluginInfo=(PLUGININFO*)lvi.lParam;
	}
	ShowWindow(GetDlgItem(hwndDlg,IDC_ABOUTGROUP),pluginInfo?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg,IDC_VERSION),pluginInfo?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg,IDC_DESCRIPTION),pluginInfo?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg,IDC_AUTHOR),pluginInfo?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg,IDC_COPYRIGHT),pluginInfo?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg,IDC_HOMEPAGE),pluginInfo?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg,IDC_AVERSION),pluginInfo?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg,IDC_GETMORE),pluginInfo?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg,IDC_ADESCRIPTION),pluginInfo?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg,IDC_AAUTHOR),pluginInfo?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg,IDC_ACOPYRIGHT),pluginInfo?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg,IDC_AHOMEPAGE),pluginInfo?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg,IDC_PLUGINENABLE),pluginInfo?SW_SHOW:SW_HIDE);
	if(pluginInfo==NULL) {
		SetDlgItemText(hwndDlg,IDC_VERSION,"");
		SetDlgItemText(hwndDlg,IDC_DESCRIPTION,"");
		SetDlgItemText(hwndDlg,IDC_AUTHOR,"");
		SetDlgItemText(hwndDlg,IDC_COPYRIGHT,"");
		SetDlgItemText(hwndDlg,IDC_HOMEPAGE,"");
		return;
	}
	_snprintf(str,sizeof(str),Translate("About %s"),pluginInfo->shortName?Translate(pluginInfo->shortName):szPluginFile);
	SetDlgItemText(hwndDlg,IDC_ABOUTGROUP,str);
	_snprintf(str,sizeof(str),"%d.%d.%d.%d",(pluginInfo->version>>24)&0xFF,(pluginInfo->version>>16)&0xFF,(pluginInfo->version>>8)&0xFF,pluginInfo->version&0xFF);
	SetDlgItemText(hwndDlg,IDC_VERSION,str);
	if(pluginInfo->description==NULL) SetDlgItemText(hwndDlg,IDC_DESCRIPTION,"");
	else SetDlgItemText(hwndDlg,IDC_DESCRIPTION,Translate(pluginInfo->description));
	if(pluginInfo->author==NULL) {
		if(pluginInfo->authorEmail==NULL)	SetDlgItemText(hwndDlg,IDC_AUTHOR,"");
		else SetDlgItemText(hwndDlg,IDC_AUTHOR,pluginInfo->authorEmail);
	}
	else {
		if(pluginInfo->authorEmail==NULL)	SetDlgItemText(hwndDlg,IDC_AUTHOR,pluginInfo->author);
		else {
			wsprintf(str,"%s (%s)",pluginInfo->author,pluginInfo->authorEmail);
			SetDlgItemText(hwndDlg,IDC_AUTHOR,str);
		}
	}
	if(pluginInfo->copyright==NULL) SetDlgItemText(hwndDlg,IDC_COPYRIGHT,"");
	else SetDlgItemText(hwndDlg,IDC_COPYRIGHT,pluginInfo->copyright);
	if(pluginInfo->homepage==NULL) SetDlgItemText(hwndDlg,IDC_HOMEPAGE,"");
	else SetDlgItemText(hwndDlg,IDC_HOMEPAGE,pluginInfo->homepage);
	SetDlgItemText(hwndDlg,IDC_PLUGINENABLE,lvi.iImage==0?Translate("Enable Plugin"):Translate("Disable Plugin"));
}

static void __cdecl LoadPluginInfosThread(HWND hwndDlg)
{
	LVITEM lvi;
	char szPluginFilename[MAX_PATH];
	HWND hwndList = GetDlgItem(hwndDlg, IDC_PLUGINLIST);
	char szMirandaPluginsPath[MAX_PATH], szPluginPath[MAX_PATH];
	char *str2;
	PLUGININFO* (*MirandaPluginInfo)(DWORD);
	PLUGININFO *pluginInfo;
	HINSTANCE hInstPlugin;
	DWORD mirandaVersion;
	int pluginCount;
	int loadedOk;
	
	HANDLE hFile;
	int nTotalSizePluginEnabled = 0;
	int nTotalSizePluginAll = 0;
	int nTotalPluginEnabled = 0;
	int nFileSize;
	char str[MAX_PATH];


	mirandaVersion=(DWORD)CallService(MS_SYSTEM_GETVERSION,0,0);
	GetModuleFileName(GetModuleHandle(NULL),szMirandaPluginsPath,sizeof(szMirandaPluginsPath));
	str2=strrchr(szMirandaPluginsPath,'\\');
	if(str2!=NULL) *str2=0;
	lstrcat(szMirandaPluginsPath,"\\Plugins\\");

	lvi.mask=LVIF_PARAM;
	lvi.iSubItem=0;
	pluginCount=ListView_GetItemCount(hwndList);
	for(lvi.iItem=0;lvi.iItem<pluginCount;lvi.iItem++) {
		ListView_GetItemText(hwndList,lvi.iItem,0,szPluginFilename,sizeof(szPluginFilename));
		loadedOk=1;
		hInstPlugin=GetModuleHandle(szPluginFilename);
		if(hInstPlugin!=NULL) {
			if((MirandaPluginInfo=(PLUGININFO* (*)(DWORD))GetProcAddress(hInstPlugin,"MirandaPluginInfo"))==NULL
			   || (pluginInfo=CopyPluginInfo(MirandaPluginInfo(mirandaVersion)))==NULL)
				loadedOk=0;
		}
		else {
			lstrcpy(szPluginPath,szMirandaPluginsPath);
			lstrcat(szPluginPath,szPluginFilename);
			if((hInstPlugin=LoadLibrary(szPluginPath))==NULL
			   || (MirandaPluginInfo=(PLUGININFO* (*)(DWORD))GetProcAddress(hInstPlugin,"MirandaPluginInfo"))==NULL
			   || ((pluginInfo=CopyPluginInfo(MirandaPluginInfo(mirandaVersion)))==NULL)) {
				if(hInstPlugin) FreeLibrary(hInstPlugin);
				loadedOk=0;
			}
		}
		if(!IsWindow(hwndList)) break;
		if(!loadedOk) {
			if(ListView_GetItemState(hwndList,lvi.iItem,LVIS_SELECTED))
				if(lvi.iItem+1<pluginCount) ListView_SetItemState(hwndList,lvi.iItem+1,LVIS_SELECTED,LVIS_SELECTED);
			ListView_DeleteItem(hwndList,lvi.iItem);
			pluginCount--;
			lvi.iItem--;
			continue;
		}
		ListView_SetItemText(hwndList,lvi.iItem,0,pluginInfo->shortName);

		// Collect file information
		lstrcpy(szPluginPath, szMirandaPluginsPath);
		lstrcat(szPluginPath, szPluginFilename);
		// Set version
		wsprintf(str, "%d.%d.%d.%d", (pluginInfo->version>>24)&0xFF, (pluginInfo->version>>16)&0xFF, (pluginInfo->version>>8)&0xFF, pluginInfo->version&0xFF);
		ListView_SetItemText(hwndList, lvi.iItem, 1, str);
		// Set filename
		ListView_SetItemText(hwndList, lvi.iItem, 2, szPluginFilename);
		// Get filesize
		wsprintf(str, "");
		hFile=CreateFile(szPluginPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile != INVALID_HANDLE_VALUE) {
			nFileSize = GetFileSize(hFile, NULL);
			wsprintf(str, "%dk", nFileSize/1024, str2);
			CloseHandle(hFile);
		}
		// Set filesize
		ListView_SetItemText(hwndList, lvi.iItem, 3, str);

		lvi.lParam=(LPARAM)pluginInfo;
		ListView_SetItem(hwndList,&lvi);
		
		nTotalSizePluginAll += nFileSize;

		if (IsPluginDisabled(szPluginFilename)) {
			nTotalSizePluginEnabled+=nFileSize;
			nTotalPluginEnabled++;
		}
		if(ListView_GetItemState(hwndList,lvi.iItem,LVIS_SELECTED)) SetDetailsFields(hwndDlg,lvi.iItem);
	}

	{
		char str[255];
		char str3[255];
		GetDlgItemText(hwndDlg, IDC_PLUGINSTATIC1, str, sizeof(str));
		wsprintf(str3, Translate(": active %d/%d, size %dk/%dk"), nTotalPluginEnabled, pluginCount, nTotalSizePluginEnabled/1024, nTotalSizePluginAll/1024);
		lstrcat(str, str3);
		SetDlgItemText(hwndDlg, IDC_PLUGINSTATIC1, str);
	}
	hLoaded = 1;
}

static void TogglePluginCheckmark(HWND hwndDlg,int iItem)
{
	LVITEM lvi;
	HWND hwndList=GetDlgItem(hwndDlg,IDC_PLUGINLIST);

	lvi.mask=LVIF_IMAGE|LVIF_PARAM;
	lvi.iItem=iItem;
	lvi.iSubItem=0;
	ListView_GetItem(hwndList,&lvi);
	SetDlgItemText(hwndDlg,IDC_PLUGINENABLE,lvi.iImage?Translate("Enable Plugin"):Translate("Disable Plugin"));
	ShowWindow(GetDlgItem(hwndDlg,IDC_RESTARTREQD),SW_SHOW);
	lvi.mask=LVIF_IMAGE;
	lvi.iImage^=1;
	ListView_SetItem(hwndList,&lvi);
	if(lvi.iImage) {
		PLUGININFO *thisInfo,*plugInfo;

		thisInfo=(PLUGININFO*)lvi.lParam;
		if(thisInfo && thisInfo->replacesDefaultModule) {
			for(lvi.iItem=ListView_GetItemCount(hwndList)-1;lvi.iItem>=0;lvi.iItem--) {
				if(lvi.iItem==iItem) continue;
				lvi.mask=LVIF_PARAM;
				ListView_GetItem(hwndList,&lvi);
				plugInfo=(PLUGININFO*)lvi.lParam;
				if(plugInfo && plugInfo->replacesDefaultModule==thisInfo->replacesDefaultModule) {
					lvi.mask=LVIF_IMAGE;
					lvi.iImage=0;
					ListView_SetItem(hwndList,&lvi);
				}
			}
		}
	}
	ShowWindow(GetDlgItem(hwndDlg,IDC_RESTARTREQD),TRUE);
	SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
}

static BOOL CALLBACK DlgProcPluginsOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct PluginDlgData *dat;

	dat=(struct PluginDlgData*)GetWindowLong(hwndDlg,GWL_USERDATA);
	switch (msg)
	{
		case WM_INITDIALOG:
		{	LVCOLUMN lvc;
			LVITEM lvi;
			HANDLE hFind;
			char szMirandaPath[MAX_PATH],szSearchPath[MAX_PATH];
			WIN32_FIND_DATA fd;
			int i;

			{	HIMAGELIST hIml;
				hIml=ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),ILC_COLOR4|ILC_MASK,2,2);
				ImageList_AddIcon(hIml,LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_NOTICK)));
				ImageList_AddIcon(hIml,LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_TICK)));
				ListView_SetImageList(GetDlgItem(hwndDlg,IDC_PLUGINLIST),hIml,LVSIL_SMALL);
			}
			hLoaded = 0;
			dat=(struct PluginDlgData*)malloc(sizeof(struct PluginDlgData));
			SetWindowLong(hwndDlg,GWL_USERDATA,(LONG)dat);
			TranslateDialogDefault(hwndDlg);
			dat->hwndList=GetDlgItem(hwndDlg,IDC_PLUGINLIST);
			dat->iLastColumnSortIndex=1;
			dat->bSortAscending=1;
			dat->hBmpSortUp=(HBITMAP)LoadImage(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_SORTCOLUP),IMAGE_BITMAP,0,0,LR_LOADMAP3DCOLORS);
			dat->hBmpSortDown=(HBITMAP)LoadImage(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_SORTCOLDOWN),IMAGE_BITMAP,0,0,LR_LOADMAP3DCOLORS);
			ListView_SetExtendedListViewStyleEx(GetDlgItem(hwndDlg, IDC_PLUGINLIST), LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
			{
				RECT rc;
				GetClientRect(GetDlgItem(hwndDlg, IDC_PLUGINLIST), &rc);
				rc.right -= GetSystemMetrics(SM_CXVSCROLL);
				lvc.mask = LVCF_WIDTH|LVCF_TEXT;
				lvc.cx = 180;
				lvc.pszText = Translate("Function");
				ListView_InsertColumn(GetDlgItem(hwndDlg, IDC_PLUGINLIST), 0, &lvc);
				lvc.cx = 72;
				lvc.pszText = Translate("Version");	
				ListView_InsertColumn(GetDlgItem(hwndDlg, IDC_PLUGINLIST), 1, &lvc);
				lvc.cx = 115;
				lvc.pszText = Translate("Filename");
				ListView_InsertColumn(GetDlgItem(hwndDlg, IDC_PLUGINLIST), 2, &lvc);
				lvc.cx = rc.right-130-165-72;
				lvc.pszText = Translate("Filesize");
				ListView_InsertColumn(GetDlgItem(hwndDlg, IDC_PLUGINLIST), 3, &lvc);
			}
			lvi.iItem=0;
			lvi.mask=LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
			lvi.iSubItem=0;
			lvi.lParam=0;
			{	char *str2;
				GetModuleFileName(GetModuleHandle(NULL),szMirandaPath,sizeof(szMirandaPath));
				str2=strrchr(szMirandaPath,'\\');
				if(str2!=NULL) *str2=0;
			}
			lstrcpy(szSearchPath,szMirandaPath);
			lstrcat(szSearchPath,"\\Plugins\\*.dll");
			hFind=FindFirstFile(szSearchPath,&fd);
			if(hFind!=INVALID_HANDLE_VALUE) {
				do {
					lvi.pszText=fd.cFileName;
					lvi.iImage=IsPluginDisabled(fd.cFileName)?1:0;
					i=ListView_InsertItem(GetDlgItem(hwndDlg,IDC_PLUGINLIST),&lvi);
					lvi.iItem++;
				} while(FindNextFile(hFind,&fd));
				FindClose(hFind);
				ListView_SetItemState(GetDlgItem(hwndDlg,IDC_PLUGINLIST),0,LVIS_SELECTED,LVIS_SELECTED);
			}
			else SetDetailsFields(hwndDlg,-1);
			//forkthread(LoadPluginInfosThread,0,hwndDlg);
			LoadPluginInfosThread(hwndDlg);
			return TRUE;
		}
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDC_HOMEPAGE:
				{	char str[1024];
					GetDlgItemText(hwndDlg,IDC_HOMEPAGE,str,sizeof(str));
					CallService(MS_UTILS_OPENURL,1,(LPARAM)str);
					break;
				}
				case IDC_GETMORE:
				{	char szUrl[256];
					_snprintf(szUrl,sizeof(szUrl),"http://www.miranda-im.org/download/index.php");
					CallService(MS_UTILS_OPENURL,1,(LPARAM)szUrl);
					break;
				}
				case IDC_PLUGINENABLE:
				{
					LVITEM lvi;
					char szPluginFile[MAX_PATH];
					lvi.iItem=ListView_GetNextItem(GetDlgItem(hwndDlg,IDC_PLUGINLIST),-1,LVNI_SELECTED);
					lvi.iSubItem=0;
					lvi.mask=LVIF_PARAM|LVIF_TEXT;
					lvi.pszText=szPluginFile;
					lvi.cchTextMax=sizeof(szPluginFile);
					if(ListView_GetItem(GetDlgItem(hwndDlg,IDC_PLUGINLIST),&lvi)) {
						TogglePluginCheckmark(hwndDlg,lvi.iItem);
					}
					break;
				}
			}
			break;
		case WM_NOTIFY:
			switch(((NMHDR*)lParam)->idFrom) {
				case IDC_PLUGINLIST:

					switch(((NMHDR*)lParam)->code) {
						case LVN_ITEMCHANGED:
							SetDetailsFields(hwndDlg,ListView_GetNextItem(GetDlgItem(hwndDlg,IDC_PLUGINLIST),-1,LVNI_SELECTED));
							break;
						case NM_CLICK:
						{	LVHITTESTINFO hti;

							hti.pt=((NMLISTVIEW*)lParam)->ptAction;
							ListView_SubItemHitTest(GetDlgItem(hwndDlg,IDC_PLUGINLIST),&hti);
							if(hti.flags!=LVHT_ONITEMICON || hti.iSubItem) break;
							TogglePluginCheckmark(hwndDlg,hti.iItem);
							break;
						}
						case LVN_KEYDOWN:
							if(((NMLVKEYDOWN*)lParam)->wVKey==VK_SPACE)
								TogglePluginCheckmark(hwndDlg,ListView_GetNextItem(GetDlgItem(hwndDlg,IDC_PLUGINLIST),-1,LVNI_SELECTED));
							break;
						case LVN_COLUMNCLICK:
						{
							LPNMLISTVIEW nmlv=(LPNMLISTVIEW)lParam;
							HDITEM hdi;
							
							if (!hLoaded) break;
							hdi.mask=HDI_BITMAP|HDI_FORMAT;
							hdi.fmt=HDF_LEFT|HDF_STRING;
							Header_SetItem(ListView_GetHeader(GetDlgItem(hwndDlg,IDC_PLUGINLIST)),dat->iLastColumnSortIndex,&hdi);

							if(nmlv->iSubItem!=dat->iLastColumnSortIndex)
							{
								dat->bSortAscending=TRUE;
								dat->iLastColumnSortIndex=nmlv->iSubItem;
							}
							else dat->bSortAscending=!dat->bSortAscending;

							hdi.fmt=HDF_LEFT|HDF_BITMAP|HDF_STRING|HDF_BITMAP_ON_RIGHT;
							hdi.hbm=dat->bSortAscending?dat->hBmpSortDown:dat->hBmpSortUp;
							Header_SetItem(ListView_GetHeader(GetDlgItem(hwndDlg,IDC_PLUGINLIST)),dat->iLastColumnSortIndex,&hdi);

							ListView_SortItems(GetDlgItem(hwndDlg, IDC_PLUGINLIST), PluginSearchResultsCompareFunc, (LPARAM)dat);
							break;
						}
					}
					break;
				case 0:
					switch (((LPNMHDR)lParam)->code)
					{
						case PSN_APPLY:
						{	LVITEM lvi;
							LVITEM lvic;
							char filename[MAX_PATH];

							lvi.mask=LVIF_TEXT;
							lvi.iSubItem=2;
							lvi.pszText=filename;
							lvi.cchTextMax=sizeof(filename);
							lvic.mask=LVIF_IMAGE;
							lvic.iSubItem=0;
							for(lvi.iItem=ListView_GetItemCount(GetDlgItem(hwndDlg,IDC_PLUGINLIST))-1;lvi.iItem>=0;lvi.iItem--) {
								lvic.iItem=lvi.iItem;
								ListView_GetItem(GetDlgItem(hwndDlg,IDC_PLUGINLIST),&lvi);
								ListView_GetItem(GetDlgItem(hwndDlg,IDC_PLUGINLIST),&lvic);
								if(lvic.iImage) {
									DeletePluginDisableInfo(filename);
								}
								else {
									DBWriteContactSettingByte(NULL,"PluginDisable",filename,1);
								}
							}
							return TRUE;
						}

					}
					break;
			}
			break;
		case WM_DESTROY:
		{	LVITEM lvi;

			lvi.mask=LVIF_PARAM;
			lvi.iSubItem=0;
			for(lvi.iItem=ListView_GetItemCount(GetDlgItem(hwndDlg,IDC_PLUGINLIST))-1;lvi.iItem>=0;lvi.iItem--) {
				ListView_GetItem(GetDlgItem(hwndDlg,IDC_PLUGINLIST),&lvi);
				if(lvi.lParam) FreePluginInfo((PLUGININFO*)lvi.lParam);
			}
			DeleteObject(dat->hBmpSortDown);
			DeleteObject(dat->hBmpSortUp);
			free(dat);
			break;
		}
	}
	return FALSE;
}

static int InitPluginsOptions(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp;

	ZeroMemory(&odp,sizeof(odp));
	odp.cbSize=sizeof(odp);
	odp.position=1300000000;
	odp.hInstance=GetModuleHandle(NULL);
	odp.pszTemplate=MAKEINTRESOURCE(IDD_OPT_PLUGINS);
	odp.pszTitle=Translate("Plugins");
	odp.pfnDlgProc=DlgProcPluginsOpts;
	odp.flags=ODPF_BOLDGROUPS;
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);
	return 0;
}

int LoadPluginOptionsModule(void)
{
	HookEvent(ME_OPT_INITIALISE,InitPluginsOptions);
	return 0;
}