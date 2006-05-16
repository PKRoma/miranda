/*
Change ICQ Details plugin for Miranda IM

Copyright © 2001-2005 Richard Hughes, Martin Öberg

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


#include <windows.h>
#include <commctrl.h>
#include <newpluginapi.h>
#include <m_database.h>
#include <m_userinfo.h>
#include <m_langpack.h>
#include <m_protosvc.h>
#include "changeinfo.h"
#include "resource.h"

HWND hwndList;
HFONT hListFont;
int iEditItem=-1;
static int editTopIndex;
static HANDLE hAckHook=NULL;
HANDLE hUpload[5];

#define DM_PROTOACK  (WM_USER+10)
static void PaintItemSetting(HDC hdc,RECT *rc,int i,UINT itemState)
{
	char *text;
	int alloced=0;
	char str[80];

	if(setting[i].value==0 && !(setting[i].displayType&LIF_ZEROISVALID)) {
		SetTextColor(hdc,GetSysColor(COLOR_GRAYTEXT));
		text=Translate("<empty>");
	}
	else {
		switch(setting[i].displayType&LIM_TYPE) {
			case LI_STRING:
			case LI_LONGSTRING:
				text=BinaryToEscapes((char*)setting[i].value);
				alloced=1;
				break;
			case LI_NUMBER:
				text=str;
				wsprintf(text,"%d",setting[i].value);
				break;
			case LI_LIST:
				if(setting[i].dbType==DBVT_ASCIIZ) {
					text=(char*)setting[i].value;
				}
				else {
					int j;
					text=Translate("Unknown value");
					for(j=0;j<setting[i].listCount;j++)
						if(((ListTypeDataItem*)setting[i].pList)[j].id==setting[i].value) {
							text=((ListTypeDataItem*)setting[i].pList)[j].szValue;
							break;
						}
				}
				break;
		}
	}
	if(setting[i].displayType&LIF_PASSWORD) {
		if(setting[i].changed) {
			int i;
			for(i=0;text[i];i++) text[i]='*';
		}
		else {
			if(alloced) {free(text); alloced=0;}
			text="*****************";
		}
	}
	if((setting[i].displayType&LIM_TYPE)==LI_LIST && (itemState&CDIS_SELECTED || iEditItem==i)) {
		RECT rcBtn;
		rcBtn=*rc;
		rcBtn.left=rcBtn.right-rc->bottom+rc->top;
		InflateRect(&rcBtn,-2,-2);
		rc->right=rcBtn.left;
		DrawFrameControl(hdc,&rcBtn,DFC_SCROLL,iEditItem==i?DFCS_SCROLLDOWN|DFCS_PUSHED:DFCS_SCROLLDOWN);
	}
	DrawText(hdc,text,-1,rc,DT_END_ELLIPSIS|DT_LEFT|DT_NOCLIP|DT_NOPREFIX|DT_SINGLELINE|DT_VCENTER);
	if(alloced) free(text);
}

BOOL CALLBACK ChangeInfoDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) {
		case WM_INITDIALOG:
			TranslateDialogDefault(hwndDlg);
			hwndList=GetDlgItem(hwndDlg,IDC_LIST);
			LoadSettingsFromDb(0);
			ListView_SetExtendedListViewStyle(hwndList,LVS_EX_FULLROWSELECT);
			iEditItem=-1;
			{	HFONT hFont;
				LOGFONT lf;
				hListFont=(HFONT)SendMessage(hwndList,WM_GETFONT,0,0);
				GetObject(hListFont,sizeof(lf),&lf);
				lf.lfHeight-=6;
				hFont=CreateFontIndirect(&lf);
				SendMessage(hwndList,WM_SETFONT,(WPARAM)hFont,0);
			}
			{	LV_COLUMN lvc={0};
				RECT rc;

				GetClientRect(hwndList,&rc);
				rc.right-=GetSystemMetrics(SM_CXVSCROLL);
				lvc.mask = LVCF_WIDTH;
				lvc.cx = rc.right/3;
				ListView_InsertColumn(hwndList, 0, &lvc);
				lvc.cx = rc.right-lvc.cx;
				ListView_InsertColumn(hwndList, 1, &lvc);
			}
			{	LV_ITEM lvi={0};
				lvi.mask = LVIF_PARAM;
				for(lvi.iItem=0;lvi.iItem<settingCount;lvi.iItem++) {
					lvi.lParam=lvi.iItem;
					ListView_InsertItem(hwndList, &lvi);
				}
			}
			SendMessage(GetParent(hwndDlg),PSM_CHANGED,0,0);
			return TRUE;
		case WM_NOTIFY:
			switch (((LPNMHDR)lParam)->idFrom) {
				case 0:
					switch (((LPNMHDR)lParam)->code) {
						case PSN_INFOCHANGED:
							LoadSettingsFromDb(1);
							break;
						case PSN_KILLACTIVE:
							EndStringEdit(1);
							EndListEdit(1);
							break;
						case PSN_APPLY:
							if(ChangesMade()) {
								if(IDYES!=MessageBox(hwndDlg,Translate("You've made some changes to your ICQ info but it has not been saved to the server. Are you sure you want to close this dialog?"),Translate("ICQ Info Not Saved"),MB_YESNOCANCEL)) {
									SetWindowLong(hwndDlg,DWL_MSGRESULT,PSNRET_INVALID_NOCHANGEPAGE);
									return TRUE;
								}
							}
							break;
					}
					break;
				case IDC_LIST:
					switch (((LPNMHDR)lParam)->code) {
						case LVN_GETDISPINFO:
							if(iEditItem!=-1) {
								if(editTopIndex!=ListView_GetTopIndex(hwndList)) {
									EndStringEdit(1);
									EndListEdit(1);
								}
							}
							break;
							
						case NM_CUSTOMDRAW:
						{
							LPNMLVCUSTOMDRAW cd=(LPNMLVCUSTOMDRAW)lParam;

							switch(cd->nmcd.dwDrawStage)
							{

								case CDDS_PREPAINT:
									SetWindowLong(hwndDlg,DWL_MSGRESULT,CDRF_NOTIFYSUBITEMDRAW);
									return TRUE;

								case CDDS_ITEMPREPAINT:
								{

									RECT rc;

									ListView_GetItemRect(hwndList, cd->nmcd.dwItemSpec, &rc, LVIR_BOUNDS);

									if (GetWindowLong(hwndList, GWL_STYLE) & WS_DISABLED)
									{	// ??
										SetTextColor(cd->nmcd.hdc, cd->clrText);
										FillRect(cd->nmcd.hdc, &rc, GetSysColorBrush(COLOR_3DFACE));
									}
									else if ((cd->nmcd.uItemState & CDIS_SELECTED || iEditItem == (int)cd->nmcd.dwItemSpec)
										      && setting[cd->nmcd.lItemlParam].displayType != LI_DIVIDER)
									{	// Selected item
										SetTextColor(cd->nmcd.hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
										FillRect(cd->nmcd.hdc, &rc, GetSysColorBrush(COLOR_HIGHLIGHT));
									}
									else
									{
										// Unselected item
										SetTextColor(cd->nmcd.hdc, GetSysColor(COLOR_WINDOWTEXT));
										FillRect(cd->nmcd.hdc, &rc, GetSysColorBrush(COLOR_WINDOW));
									}

									if (setting[cd->nmcd.lItemlParam].displayType == LI_DIVIDER)
									{
										RECT rcLine;
										SIZE textSize;
										char *szText = Translate(setting[cd->nmcd.lItemlParam].szDescription);
										HFONT hoFont;

										hoFont = (HFONT)SelectObject(cd->nmcd.hdc, hListFont);
										SetTextColor(cd->nmcd.hdc, GetSysColor(COLOR_3DSHADOW));
										ListView_GetItemRect(hwndList, cd->nmcd.dwItemSpec, &rc, LVIR_BOUNDS);
										DrawText(cd->nmcd.hdc, szText, -1, &rc, DT_CENTER|DT_NOCLIP|DT_NOPREFIX|DT_SINGLELINE|DT_VCENTER);
										GetTextExtentPoint32(cd->nmcd.hdc, szText, lstrlen(szText), &textSize);
										rcLine.top = (rc.top + rc.bottom)/2-1;
										rcLine.bottom = rcLine.top+2;
										rcLine.left = rc.left + 3;
										rcLine.right = (rc.left+rc.right-textSize.cx)/2-3;
										DrawEdge(cd->nmcd.hdc, &rcLine, BDR_SUNKENOUTER, BF_RECT);
										rcLine.left = (rc.left + rc.right + textSize.cx)/2 + 3;
										rcLine.right = rc.right-3;
										DrawEdge(cd->nmcd.hdc, &rcLine, BDR_SUNKENOUTER, BF_RECT);
										SelectObject(cd->nmcd.hdc, hoFont);
										SetWindowLong(hwndDlg, DWL_MSGRESULT, CDRF_SKIPDEFAULT);
									}
									else
									{
										SetWindowLong(hwndDlg, DWL_MSGRESULT, CDRF_NOTIFYSUBITEMDRAW|CDRF_NOTIFYPOSTPAINT);
									}

									return TRUE;

								}

								case CDDS_SUBITEM|CDDS_ITEMPREPAINT:
								{	RECT rc;
									HFONT hoFont;
									hoFont=(HFONT)SelectObject(cd->nmcd.hdc,hListFont);
									ListView_GetSubItemRect(hwndList,cd->nmcd.dwItemSpec,cd->iSubItem,LVIR_BOUNDS,&rc);
									if(cd->iSubItem==0)	{
										RECT rc2;
										ListView_GetSubItemRect(hwndList,cd->nmcd.dwItemSpec,1,LVIR_BOUNDS,&rc2);
										rc.right=rc2.left;
										rc.left+=2;
										DrawText(cd->nmcd.hdc,Translate(setting[cd->nmcd.lItemlParam].szDescription),-1,&rc,DT_END_ELLIPSIS|DT_LEFT|DT_NOCLIP|DT_NOPREFIX|DT_SINGLELINE|DT_VCENTER);
									}
									else PaintItemSetting(cd->nmcd.hdc,&rc,cd->nmcd.lItemlParam,cd->nmcd.uItemState);
									SelectObject(cd->nmcd.hdc,hoFont);
									SetWindowLong(hwndDlg,DWL_MSGRESULT,CDRF_SKIPDEFAULT);
									return TRUE;
								}
								case CDDS_ITEMPOSTPAINT:
								{	RECT rc;
									ListView_GetItemRect(hwndList,cd->nmcd.dwItemSpec,&rc,LVIR_BOUNDS);
									if(cd->nmcd.uItemState&CDIS_FOCUS) {
										HDC hdc2;
										hdc2=GetDC(hwndList);	//I don't know what the listview's done to its DC, but I can't figure out how to undo it
										DrawFocusRect(hdc2,&rc);
										ReleaseDC(hwndList,hdc2);
									}
									break;
								}
							}
							break;
						}
						case NM_CLICK:
						{	LPNMLISTVIEW nm=(LPNMLISTVIEW)lParam;
							LV_ITEM lvi;
							RECT rc;

							EndStringEdit(1);
							EndListEdit(1);
							if(nm->iSubItem!=1) break;
							lvi.mask=LVIF_PARAM|LVIF_STATE;
							lvi.stateMask=0xFFFFFFFF;
							lvi.iItem=nm->iItem; lvi.iSubItem=nm->iSubItem;
							ListView_GetItem(hwndList,&lvi);
							if(!(lvi.state&LVIS_SELECTED)) break;
							ListView_EnsureVisible(hwndList,lvi.iItem,FALSE);
							ListView_GetSubItemRect(hwndList,lvi.iItem,lvi.iSubItem,LVIR_BOUNDS,&rc);
							editTopIndex=ListView_GetTopIndex(hwndList);
							switch(setting[lvi.lParam].displayType&LIM_TYPE) {
								case LI_STRING:
								case LI_LONGSTRING:
								case LI_NUMBER:
									BeginStringEdit(nm->iItem,&rc,lvi.lParam,0);
									break;
								case LI_LIST:
									BeginListEdit(nm->iItem,&rc,lvi.lParam,0);
									break;
							}
							break;
						}
						case LVN_KEYDOWN:
						{	LPNMLVKEYDOWN nm=(LPNMLVKEYDOWN)lParam;
							LV_ITEM lvi;
							RECT rc;

							EndStringEdit(1);
							EndListEdit(1);
							if(nm->wVKey==VK_SPACE || nm->wVKey==VK_RETURN) nm->wVKey=0;
							if(nm->wVKey && (nm->wVKey<'0' || (nm->wVKey>'9' && nm->wVKey<'A') || (nm->wVKey>'Z' && nm->wVKey<VK_NUMPAD0) || nm->wVKey>=VK_F1))
								break;
							lvi.mask=LVIF_PARAM|LVIF_STATE;
							lvi.stateMask=0xFFFFFFFF;
							lvi.iItem=ListView_GetNextItem(hwndList,-1,LVNI_ALL|LVNI_SELECTED);
							if(lvi.iItem==-1) break;
							lvi.iSubItem=1;
							ListView_GetItem(hwndList,&lvi);
							ListView_EnsureVisible(hwndList,lvi.iItem,FALSE);
							ListView_GetSubItemRect(hwndList,lvi.iItem,lvi.iSubItem,LVIR_BOUNDS,&rc);
							editTopIndex=ListView_GetTopIndex(hwndList);
							switch(setting[lvi.lParam].displayType&LIM_TYPE) {
								case LI_STRING:
								case LI_LONGSTRING:
								case LI_NUMBER:
									BeginStringEdit(lvi.iItem,&rc,lvi.lParam,nm->wVKey);
									break;
								case LI_LIST:
									BeginListEdit(lvi.iItem,&rc,lvi.lParam,nm->wVKey);
									break;
							}
							SetWindowLong(hwndDlg,DWL_MSGRESULT,TRUE);
							return TRUE;
						}
						case NM_KILLFOCUS:
							if(!IsStringEditWindow(GetFocus())) EndStringEdit(1);
							if(!IsListEditWindow(GetFocus())) EndListEdit(1);
							break;
					}
					break;
			}
			break;
		case WM_KILLFOCUS:
			EndStringEdit(1);
			EndListEdit(1);
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDCANCEL:
					SendMessage(GetParent(hwndDlg),msg,wParam,lParam);
					break;
				case IDC_SAVE:
					if(!SaveSettingsToDb(hwndDlg)) break;
					EnableWindow(GetDlgItem(hwndDlg,IDC_SAVE),FALSE);
					EnableWindow(GetDlgItem(hwndDlg,IDC_LIST),FALSE);
					SetDlgItemText(hwndDlg,IDC_UPLOADING,Translate("Upload in progress..."));
					EnableWindow(GetDlgItem(hwndDlg,IDC_UPLOADING),TRUE);
					ShowWindow(GetDlgItem(hwndDlg,IDC_UPLOADING),SW_SHOW);
					hAckHook=HookEventMessage(ME_PROTO_ACK,hwndDlg,DM_PROTOACK);
					if(!UploadSettings(hwndDlg)) {
						EnableWindow(GetDlgItem(hwndDlg,IDC_SAVE),TRUE);
						EnableWindow(GetDlgItem(hwndDlg,IDC_LIST),TRUE);
						ShowWindow(GetDlgItem(hwndDlg,IDC_UPLOADING),SW_HIDE);
						UnhookEvent(hAckHook); hAckHook=NULL;
					}
					break;
			}
			break;
		case DM_PROTOACK:
		{	ACKDATA *ack=(ACKDATA*)lParam;
			int i,done;
			char str[80];
			if(ack->type!=ACKTYPE_SETINFO) break;
			if(ack->result!=ACKRESULT_SUCCESS) break;   //not great error detection
			for(i=0;i<sizeof(hUpload)/sizeof(hUpload[0]);i++)
				if(hUpload[i] && ack->hProcess==hUpload[i]) break;
			if(i==sizeof(hUpload)/sizeof(hUpload[0])) break;
			hUpload[i]=NULL;
			for(done=0,i=0;i<sizeof(hUpload)/sizeof(hUpload[0]);i++)
				done+=hUpload[i]==NULL;
			wsprintf(str,"%s%d%%",Translate("Upload in progress..."),100*done/(sizeof(hUpload)/sizeof(hUpload[0])));
			SetDlgItemText(hwndDlg,IDC_UPLOADING,str);
			if(done<sizeof(hUpload)/sizeof(hUpload[0])) break;
			ClearChangeFlags();
			UnhookEvent(hAckHook); hAckHook=NULL;
			EnableWindow(GetDlgItem(hwndDlg,IDC_LIST),TRUE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_UPLOADING),FALSE);
			SetDlgItemText(hwndDlg,IDC_UPLOADING,Translate("Upload complete"));
			SendMessage(GetParent(hwndDlg),PSM_FORCECHANGED,0,0);
			break;
		}
		case WM_DESTROY:
			if(hAckHook) {
				UnhookEvent(hAckHook);
				hAckHook=NULL;
			}
			{	HFONT hFont;
				hFont=(HFONT)SendMessage(hwndList,WM_GETFONT,0,0);
				DeleteObject(hFont);
			}
			FreeStoredDbSettings();
			break;
	}
	return FALSE;
}