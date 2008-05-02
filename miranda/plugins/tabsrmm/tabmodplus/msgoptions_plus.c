#include <commonheaders.h>

extern HINSTANCE hinstance;
extern BOOL g_bIMGtagButton, g_bClientInStatusBar;
extern HIMAGELIST g_himlOptions, CreateStateImageList();

static BOOL bOptionsInit;

HWND g_opHdlg;

static struct LISTOPTIONSGROUP lvGroups[] = {
	0, _T("Message window tweaks (changing any of them requires a restart)"),
	0, _T("General tweaks"),
	0, NULL
};

static struct LISTOPTIONSITEM lvItems[] = {
	0, _T("Enable image tag button (*)"), 0, LOI_TYPE_SETTING, (UINT_PTR)"adv_IMGtagButton", 0,
	0, _T("Show client icon in status bar (fingerprint plugin required) (*)"), 0, LOI_TYPE_SETTING, (UINT_PTR)"adv_ClientIconInStatusBar", 0,
	0, _T("Enable typing sounds (*)"), 0, LOI_TYPE_SETTING, (UINT_PTR)"adv_soundontyping", 0,
	0, _T("Disable animated GIF avatars (*)"), 0, LOI_TYPE_SETTING, (UINT_PTR)"adv_DisableAniAvatars", 0,
	0, _T("Enable fix for nicklist scroll bar"), 0, LOI_TYPE_SETTING, (UINT_PTR)"adv_ScrollBarFix", 0,
	0, _T("Close current tab on send"), 0, LOI_TYPE_SETTING, (UINT_PTR)"adv_AutoClose_2", 0,
	0, _T("Enable icon pack version check (*)"), 0, LOI_TYPE_SETTING, (UINT_PTR)"adv_IconpackWarning", 0,
	0, _T("Disable error popups on sending failures"), 0, LOI_TYPE_SETTING, (UINT_PTR)"adv_noErrorPopups", 1,
	0, NULL, 0, 0, 0, 0
};

BOOL CALLBACK PlusOptionsProc(HWND hwndDlg,UINT msg,WPARAM wParam,LPARAM lParam)
{	
	switch(msg)	{

		case WM_INITDIALOG:	{
			TVINSERTSTRUCT tvi = {0};
			int i = 0;
			TranslateDialogDefault(hwndDlg);
			SetWindowLong(GetDlgItem(hwndDlg, IDC_PLUS_CHECKTREE), GWL_STYLE, GetWindowLong(GetDlgItem(hwndDlg, IDC_PLUS_CHECKTREE), GWL_STYLE) | (TVS_NOHSCROLL | TVS_CHECKBOXES));

			g_himlOptions = (HIMAGELIST)SendDlgItemMessage(hwndDlg, IDC_PLUS_CHECKTREE, TVM_SETIMAGELIST, TVSIL_STATE, (LPARAM)CreateStateImageList());
			if (g_himlOptions)
				ImageList_Destroy(g_himlOptions);

			/*
			* fill the list box, create groups first, then add items
			*/

			while (lvGroups[i].szName != NULL) {
				tvi.hParent = 0;
				tvi.hInsertAfter = TVI_LAST;
				tvi.item.mask = TVIF_TEXT | TVIF_STATE;
				tvi.item.pszText = TranslateTS(lvGroups[i].szName);
				tvi.item.stateMask = TVIS_STATEIMAGEMASK | TVIS_EXPANDED | TVIS_BOLD;
				tvi.item.state = INDEXTOSTATEIMAGEMASK(0) | TVIS_EXPANDED | TVIS_BOLD;
				lvGroups[i++].handle = (LRESULT)TreeView_InsertItem(GetDlgItem(hwndDlg, IDC_PLUS_CHECKTREE), &tvi);
			}

			i = 0;

			while (lvItems[i].szName != 0) {
				tvi.hParent = (HTREEITEM)lvGroups[lvItems[i].uGroup].handle;
				tvi.hInsertAfter = TVI_LAST;
				tvi.item.pszText = TranslateTS(lvItems[i].szName);
				tvi.item.mask = TVIF_TEXT | TVIF_STATE | TVIF_PARAM;
				tvi.item.lParam = i;
				tvi.item.stateMask = TVIS_STATEIMAGEMASK;
				//if (lvItems[i].uType == LOI_TYPE_FLAG)
				//	tvi.item.state = INDEXTOSTATEIMAGEMASK((dwFlags & (UINT)lvItems[i].lParam) ? 3 : 2);
				if (lvItems[i].uType == LOI_TYPE_SETTING)
					tvi.item.state = INDEXTOSTATEIMAGEMASK(DBGetContactSettingByte(NULL, SRMSGMOD_T, (char *)lvItems[i].lParam, lvItems[i].id) ? 3 : 2);  // NOTE: was 2 : 1 without state image mask
				lvItems[i].handle = (LRESULT)TreeView_InsertItem(GetDlgItem(hwndDlg, IDC_PLUS_CHECKTREE), &tvi);
				i++;
			}
			g_bIMGtagButton = DBGetContactSettingByte(NULL, SRMSGMOD_T, "adv_IMGtagButton",0 );
			g_bClientInStatusBar = DBGetContactSettingByte(NULL, SRMSGMOD_T, "adv_ClientIconInStatusBar", 0);

			return TRUE;
		}
		case WM_NOTIFY:
			switch (((LPNMHDR) lParam)->idFrom) {
				case IDC_PLUS_CHECKTREE:
					if (((LPNMHDR)lParam)->code == NM_CLICK || (((LPNMHDR)lParam)->code == TVN_KEYDOWN && ((LPNMTVKEYDOWN)lParam)->wVKey == VK_SPACE)) {
						TVHITTESTINFO hti;
						TVITEM item = {0};

						item.mask = TVIF_HANDLE | TVIF_STATE;
						item.stateMask = TVIS_STATEIMAGEMASK | TVIS_BOLD;
						hti.pt.x = (short)LOWORD(GetMessagePos());
						hti.pt.y = (short)HIWORD(GetMessagePos());
						ScreenToClient(((LPNMHDR)lParam)->hwndFrom, &hti.pt);
						if (TreeView_HitTest(((LPNMHDR)lParam)->hwndFrom, &hti) || ((LPNMHDR)lParam)->code == TVN_KEYDOWN) {
							if (((LPNMHDR)lParam)->code == TVN_KEYDOWN) {
								hti.flags |= TVHT_ONITEMSTATEICON;
								item.hItem = TreeView_GetSelection(((LPNMHDR)lParam)->hwndFrom);
							} else
								item.hItem = (HTREEITEM)hti.hItem;
							SendDlgItemMessageA(hwndDlg, IDC_PLUS_CHECKTREE, TVM_GETITEMA, 0, (LPARAM)&item);
							if (item.state & TVIS_BOLD && hti.flags & TVHT_ONITEMSTATEICON) {
								item.state = INDEXTOSTATEIMAGEMASK(0) | TVIS_BOLD;
								SendDlgItemMessageA(hwndDlg, IDC_PLUS_CHECKTREE, TVM_SETITEMA, 0, (LPARAM)&item);
							} else if (hti.flags & TVHT_ONITEMSTATEICON) {

								if (((item.state & TVIS_STATEIMAGEMASK) >> 12) == 3) {
									item.state = INDEXTOSTATEIMAGEMASK(1);
									SendDlgItemMessageA(hwndDlg, IDC_PLUS_CHECKTREE, TVM_SETITEMA, 0, (LPARAM)&item);
								}

								SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
							}
						}
					}
					break;
				default:
					switch (((LPNMHDR) lParam)->code) {
						case PSN_APPLY: {
							int i = 0;
							TVITEM item = {0};
							/*
							* scan the tree view and obtain the options...
							*/
							while (lvItems[i].szName != NULL) {
								item.mask = TVIF_HANDLE | TVIF_STATE;
								item.hItem = (HTREEITEM)lvItems[i].handle;
								item.stateMask = TVIS_STATEIMAGEMASK;

								SendDlgItemMessageA(hwndDlg, IDC_PLUS_CHECKTREE, TVM_GETITEMA, 0, (LPARAM)&item);
								//if (lvItems[i].uType == LOI_TYPE_FLAG)
								//	dwFlags |= (item.state >> 12) == 3/*2*/ ? lvItems[i].lParam : 0;
								if (lvItems[i].uType == LOI_TYPE_SETTING)
									DBWriteContactSettingByte(NULL, SRMSGMOD_T, (char *)lvItems[i].lParam, (BYTE)((item.state >> 12) == 3/*2*/ ? 1 : 0));  // NOTE: state image masks changed
								i++;
							}
							return TRUE;
						}
					}
					break;
				}
				break;
		case WM_COMMAND: {
			if(LOWORD(wParam) == IDC_PLUS_HELP) {
				CallService(MS_UTILS_OPENURL, 0, (LPARAM)"http://miranda.or.at/TabSRMM_AdvancedTweaks");
				break;
			}
			break;
		}
		case WM_CLOSE:
			EndDialog(hwndDlg,0);
			return 0;
		}
	return 0;
}
