/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2003 Miranda ICQ/IM project, 
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
#include "../../core/commonheaders.h"

struct SoundItem {
	char *name,*section,*description,*tempFile;
};
static struct SoundItem *soundList=NULL;
static int soundCount;
static HANDLE hPlayEvent=NULL;

static int ServiceSkinAddNewSound(WPARAM wParam,LPARAM lParam)
{
	SKINSOUNDDESCEX *ssd=(SKINSOUNDDESCEX*)lParam;

    if (ssd->cbSize!=sizeof(SKINSOUNDDESC) && ssd->cbSize!=sizeof(SKINSOUNDDESCEX))
        return 0;
	soundList=(struct SoundItem*)realloc(soundList,sizeof(struct SoundItem)*(soundCount+1));
	soundList[soundCount].name=_strdup(ssd->pszName);
	soundList[soundCount].description=_strdup(ssd->pszDescription);
	soundList[soundCount].section=_strdup( ssd->cbSize==sizeof(SKINSOUNDDESCEX) ? ssd->pszSection : Translate("Other") );
	soundList[soundCount].tempFile=NULL;
    if (ssd->pszDefaultFile) {
        DBVARIANT dbv;

        if (DBGetContactSetting(NULL, "SkinSounds", soundList[soundCount].name, &dbv)) {
            DBWriteContactSettingString(NULL, "SkinSounds", soundList[soundCount].name, ssd->pszDefaultFile);
        }
        else DBFreeVariant(&dbv);
    }
	soundCount++;
	return 0;
}

static int SkinPlaySoundDefault(WPARAM wParam, LPARAM lParam)
{
	char * pszFile = (char *) lParam;
	if ( pszFile && (DBGetContactSettingByte(NULL,"Skin","UseSound",1) || (int)wParam==1)) {
		PlaySound(pszFile, NULL, SND_ASYNC | SND_FILENAME | SND_NOWAIT);
	}
	return 0;
}

static int ServiceSkinPlaySound(WPARAM wParam, LPARAM lParam)
{
	char * pszSoundName = (char *)lParam;
	int playSoundFlags = SND_ASYNC | SND_FILENAME | SND_NOWAIT;
	int j;

	for (j=0; j<soundCount; j++) {
		if (pszSoundName&&strcmp(soundList[j].name, pszSoundName)== 0) {
			if (DBGetContactSettingByte(NULL, "SkinSoundsOff", pszSoundName, 0)==0) {
				DBVARIANT dbv;

				if (DBGetContactSetting(NULL, "SkinSounds", pszSoundName, &dbv)==0) {
                    char szFull[MAX_PATH];
                    
                    CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)dbv.pszVal, (LPARAM)szFull);
					NotifyEventHooks(hPlayEvent, 0, (LPARAM)szFull);
					DBFreeVariant(&dbv);
				}
			}
			return 0;
		}
	}
	return 1;
}

static HTREEITEM FindNamedTreeItemAtRoot(HWND hwndTree,const char *name)
{
	TVITEM tvi;
	char str[128];

	tvi.mask=TVIF_TEXT;
	tvi.pszText=str;
	tvi.cchTextMax=sizeof(str);
	tvi.hItem=TreeView_GetRoot(hwndTree);
	while(tvi.hItem!=NULL) {
		TreeView_GetItem(hwndTree,&tvi);
		if(!strcmpi(str,name)) return tvi.hItem;
		tvi.hItem=TreeView_GetNextSibling(hwndTree,tvi.hItem);
	}
	return NULL;
}

#define DM_REBUILD_STREE (WM_USER+1)
#define DM_HIDEPANE      (WM_USER+2)
#define DM_SHOWPANE      (WM_USER+3)
BOOL CALLBACK DlgProcSoundOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HWND hwndTree = NULL;
	switch (msg)
	{
		case WM_INITDIALOG:
		{	
            TranslateDialogDefault(hwndDlg);
            hwndTree = GetDlgItem(hwndDlg, IDC_SOUNDTREE);
            SetWindowLong(hwndTree,GWL_STYLE,GetWindowLong(hwndTree,GWL_STYLE)|TVS_NOHSCROLL|TVS_CHECKBOXES);
            SendMessage(hwndDlg, DM_HIDEPANE, 0, 0);
            SendMessage(hwndDlg, DM_REBUILD_STREE, 0, 0);
            TreeView_SetItemState(hwndTree, 0, TVIS_SELECTED, TVIS_SELECTED);
			return TRUE;
		}
        case DM_REBUILD_STREE:
        {
            TVINSERTSTRUCT tvis;
            int i;

            TreeView_SelectItem(hwndTree ,NULL);
            ShowWindow(hwndTree,SW_HIDE);
            TreeView_DeleteAllItems(hwndTree);
			tvis.hParent=NULL;
			tvis.hInsertAfter=TVI_SORT;
			tvis.item.mask=TVIF_TEXT|TVIF_STATE|TVIF_PARAM;
			tvis.item.state=tvis.item.stateMask=TVIS_EXPANDED;
			for(i=0;i<soundCount;i++) {
                tvis.item.stateMask=TVIS_EXPANDED;
                tvis.item.state=TVIS_EXPANDED;
				tvis.hParent=NULL;
				tvis.hParent=FindNamedTreeItemAtRoot(hwndTree,soundList[i].section);
				if(tvis.hParent==NULL) {
                 	tvis.item.lParam=-1;
					tvis.item.pszText=soundList[i].section;
					tvis.hParent=TreeView_InsertItem(hwndTree,&tvis);
                    tvis.item.stateMask=TVIS_STATEIMAGEMASK;
                    tvis.item.state=INDEXTOSTATEIMAGEMASK(0);
                    TreeView_SetItem(hwndTree,&tvis.item);
				}
                tvis.item.stateMask=TVIS_STATEIMAGEMASK;
		        tvis.item.state=INDEXTOSTATEIMAGEMASK(!DBGetContactSettingByte(NULL,"SkinSoundsOff",soundList[i].name,0)?2:1);
				tvis.item.lParam=i;
				tvis.item.pszText=soundList[i].description;
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
            ShowWindow(hwndTree, SW_SHOW);
            break;
        }
        case DM_HIDEPANE:
            ShowWindow(GetDlgItem(hwndDlg, IDC_SGROUP), SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_NAME), SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_NAMEVAL), SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_SLOC), SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_LOCATION), SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_CHANGE), SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_PREVIEW), SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_GETMORE), SW_HIDE);
            break;
        case DM_SHOWPANE:
            ShowWindow(GetDlgItem(hwndDlg, IDC_SGROUP), SW_SHOW);
            ShowWindow(GetDlgItem(hwndDlg, IDC_NAME), SW_SHOW);
            ShowWindow(GetDlgItem(hwndDlg, IDC_NAMEVAL), SW_SHOW);
            ShowWindow(GetDlgItem(hwndDlg, IDC_SLOC), SW_SHOW);
            ShowWindow(GetDlgItem(hwndDlg, IDC_LOCATION), SW_SHOW);
            ShowWindow(GetDlgItem(hwndDlg, IDC_CHANGE), SW_SHOW);
            ShowWindow(GetDlgItem(hwndDlg, IDC_PREVIEW), SW_SHOW);
            ShowWindow(GetDlgItem(hwndDlg, IDC_GETMORE), SW_SHOW);
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
                if (soundList[tvi.lParam].tempFile) 
                    NotifyEventHooks(hPlayEvent, 1, (LPARAM)soundList[tvi.lParam].tempFile);
                else {
                    DBVARIANT dbv;
                    if(!DBGetContactSetting(NULL,"SkinSounds",soundList[tvi.lParam].name,&dbv)) {
                        char szPathFull[MAX_PATH];
                        
                        CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)dbv.pszVal, (LPARAM)szPathFull);
                        NotifyEventHooks(hPlayEvent, 1, (LPARAM)szPathFull);
                        DBFreeVariant(&dbv);
                    }
                }
				break;
			}
			if(LOWORD(wParam)==IDC_CHANGE) {
				char str[MAX_PATH], strFull[MAX_PATH];
				OPENFILENAME ofn;
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
                if (soundList[tvi.lParam].tempFile) {
                    _snprintf(strFull, sizeof(strFull), "%s", soundList[tvi.lParam].tempFile);
                }
                else {
                    if (DBGetContactSettingByte(NULL, "SkinSoundsOff", soundList[tvi.lParam].name, 0)==0) {
                        DBVARIANT dbv;

                        if (DBGetContactSetting(NULL, "SkinSounds", soundList[tvi.lParam].name, &dbv)==0) {                           
                            CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)dbv.pszVal, (LPARAM)str);
                            DBFreeVariant(&dbv);
                        }
                    }
                }
                _snprintf(strFull, sizeof(strFull), "%s", soundList[tvi.lParam].tempFile?soundList[tvi.lParam].tempFile:"");
                CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)strFull, (LPARAM)str);
				ZeroMemory(&ofn, sizeof(ofn));
				ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
				ofn.hwndOwner = GetParent(hwndDlg);
				ofn.hInstance = NULL;
				ofn.lpstrFilter = "Wave Files (*.wav)\0*.WAV\0All Files (*)\0*\0";
				ofn.lpstrFile = str;
				ofn.Flags = OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
				ofn.nMaxFile = sizeof(str);
				ofn.nMaxFileTitle = MAX_PATH;
				ofn.lpstrDefExt = "wav";
				if(!GetOpenFileName(&ofn)) break;
                CallService(MS_UTILS_PATHTORELATIVE, (WPARAM)str, (LPARAM)strFull);
                soundList[tvi.lParam].tempFile = _strdup(strFull);
                SetDlgItemText(hwndDlg, IDC_LOCATION, strFull);
			}
			if(LOWORD(wParam)==IDC_GETMORE) {
				CallService(MS_UTILS_OPENURL,1,(LPARAM)"http://www.miranda-im.org/download/index.php?action=display&id=5");
				break;
			}
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		case WM_NOTIFY:
			switch(((LPNMHDR)lParam)->idFrom) {
                case 0:
                    switch (((LPNMHDR)lParam)->code)
					{
                        case PSN_APPLY:
						{	
                            int i;
							for(i=0;i<soundCount;i++) {
								if(soundList[i].tempFile)
									DBWriteContactSettingString(NULL,"SkinSounds",soundList[i].name,soundList[i].tempFile);
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
                                                cgs.szModule="SkinSoundsOff";
                                                cgs.szSetting=soundList[tvic.lParam].name;
                                                CallService(MS_DB_CONTACT_DELETESETTING,(WPARAM)(HANDLE)NULL,(LPARAM)&cgs);
                                            }
                                            else {
                                                DBWriteContactSettingByte(NULL,"SkinSoundsOff",soundList[tvic.lParam].name,1);
                                            }
                                            tvic.hItem=TreeView_GetNextSibling(hwndTree,tvic.hItem);
                                        }
                                    }
                                    tvi.hItem=TreeView_GetNextSibling(hwndTree,tvi.hItem);
                                }
                            }
                            return TRUE;
                        }
                    }
                    break;
                case IDC_SOUNDTREE:
                    switch(((NMHDR*)lParam)->code) {
                        case TVN_SELCHANGED:
                        {
                            NMTREEVIEW *pnmtv = (NMTREEVIEW*)lParam;
                            TVITEM tvi = pnmtv->itemNew;

                            if (tvi.lParam==-1) {
                                SendMessage(hwndDlg, DM_HIDEPANE, 0, 0);
                            }
                            else {
                                char buf[256];
                                DBVARIANT dbv;
                                
                                _snprintf(buf, sizeof(buf), "%s: %s", soundList[tvi.lParam].section, soundList[tvi.lParam].description);
                                SetDlgItemText(hwndDlg, IDC_NAMEVAL, buf);
                                if (soundList[tvi.lParam].tempFile) 
                                    SetDlgItemText(hwndDlg, IDC_LOCATION, soundList[tvi.lParam].tempFile);
                                else if(!DBGetContactSetting(NULL,"SkinSounds",soundList[tvi.lParam].name,&dbv)) {
                                    SetDlgItemText(hwndDlg, IDC_LOCATION, dbv.pszVal);
                                    DBFreeVariant(&dbv);
                                }
                                else SetDlgItemText(hwndDlg, IDC_LOCATION, Translate("<not specified>"));
                                SendMessage(hwndDlg, DM_SHOWPANE, 0, 0);
                            }
                            break;
                        }
                        case TVN_KEYDOWN:
                        {
                            NMTVKEYDOWN* ptkd = (NMTVKEYDOWN*)lParam;
                            TVHITTESTINFO hti;

                            if (ptkd) {
                                if (ptkd->wVKey == 0x0020) {
                                    hti.pt.x=(short)LOWORD(GetMessagePos());
						            hti.pt.y=(short)HIWORD(GetMessagePos());
						            ScreenToClient(((LPNMHDR)lParam)->hwndFrom,&hti.pt);
                                    if(TreeView_HitTest(((LPNMHDR)lParam)->hwndFrom,&hti)) {
                                        if (hti.flags&TVHT_ONITEM) {
                                            if (TreeView_GetParent(hwndTree, hti.hItem)!=TreeView_GetRoot(hwndTree)) {
                                                // the stupid checkbox gets enabled here.
                                            }
                                            else {
                                                SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
                                            }
                                        }
                                    }
                                }
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
                                    if(hti.flags&TVHT_ONITEMSTATEICON) {
                                        if (TreeView_GetParent(hwndTree, hti.hItem)!=TreeView_GetRoot(hwndTree))
                                            SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
                                    }
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

int InitSkinSounds(void)
{
	soundList=NULL;
	soundCount=0;
	CreateServiceFunction(MS_SKIN_ADDNEWSOUND,ServiceSkinAddNewSound);
	CreateServiceFunction(MS_SKIN_PLAYSOUND,ServiceSkinPlaySound);
	hPlayEvent=CreateHookableEvent(ME_SKIN_PLAYINGSOUND);
	SetHookDefaultForHookableEvent(hPlayEvent, SkinPlaySoundDefault);
	return 0;
}

void UninitSkinSounds(void)
{
	int i;
	for(i=0;i<soundCount;i++) {
		free(soundList[i].name);
        free(soundList[i].section);
		free(soundList[i].description);
		if (soundList[i].tempFile) free(soundList[i].tempFile);
	}
	if(soundCount) free(soundList);
}
