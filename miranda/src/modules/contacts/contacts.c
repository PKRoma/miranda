/*
Miranda IM

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

extern HWND hwndContactTree;

#define NAMEORDERCOUNT 8
static char *nameOrderDescr[NAMEORDERCOUNT]={"My custom name (not moveable)","Nick","FirstName","E-mail","LastName","Username","FirstName LastName","'(Unknown Contact)' (not moveable)"};
BYTE nameOrder[NAMEORDERCOUNT];

static int GetDatabaseString( CONTACTINFO *ci, const char* setting, DBVARIANT* dbv )
{
	if ( ci->dwFlag & CNF_UNICODE )
		return DBGetContactSettingWString(ci->hContact,ci->szProto,setting,dbv);

	return DBGetContactSetting(ci->hContact,ci->szProto,setting,dbv);
}

static int GetContactInfo(WPARAM wParam, LPARAM lParam) {
	DBVARIANT dbv;
	CONTACTINFO *ci = (CONTACTINFO*)lParam;

	if (ci==NULL) return 1;
	if (ci->szProto==NULL) ci->szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)ci->hContact,0);
	if (ci->szProto==NULL) return 1;
	ci->type = 0;
	switch(ci->dwFlag & 0x7F) {
		case CNF_FIRSTNAME:
			if (!GetDatabaseString(ci,"FirstName",&dbv)) {
				ci->type = CNFT_ASCIIZ;
				ci->pszVal = dbv.ptszVal;
				return 0;
			}
			break;
		case CNF_LASTNAME:
			if (!GetDatabaseString(ci,"LastName",&dbv)) {
				ci->type = CNFT_ASCIIZ;
				ci->pszVal = dbv.ptszVal;
				return 0;
			}
			break;
		case CNF_NICK:
			if (!GetDatabaseString(ci,"Nick",&dbv)) {
				ci->type = CNFT_ASCIIZ;
				ci->pszVal = dbv.ptszVal;
				return 0;
			}
			break;
		case CNF_EMAIL:
			if (!GetDatabaseString(ci,"e-mail",&dbv)) {
				ci->type = CNFT_ASCIIZ;
				ci->pszVal = dbv.ptszVal;
				return 0;
			}
			break;
		case CNF_CITY:
			if (!GetDatabaseString(ci,"City",&dbv)) {
				ci->type = CNFT_ASCIIZ;
				ci->pszVal = dbv.ptszVal;
				return 0;
			}
			break;
		case CNF_STATE:
			if (!GetDatabaseString(ci,"State",&dbv)) {
				ci->type = CNFT_ASCIIZ;
				ci->pszVal = dbv.ptszVal;
				return 0;
			}
			break;
		case CNF_COUNTRY:
		{   int i,countryCount;
			struct CountryListEntry *countries;
			if (!GetDatabaseString(ci,"Country",&dbv)) {
				CallService(MS_UTILS_GETCOUNTRYLIST,(WPARAM)&countryCount,(LPARAM)&countries);
				for(i=0;i<countryCount;i++) {
					if(countries[i].id!=dbv.wVal) continue;
					ci->type = CNFT_ASCIIZ;
					ci->pszVal = ( TCHAR* )_strdup(countries[i].szName);
					DBFreeVariant(&dbv);
					return 0;
				}
				DBFreeVariant(&dbv);
			}
			break;
		}
		case CNF_PHONE:
			if (!GetDatabaseString(ci,"Phone",&dbv)) {
				ci->type = CNFT_ASCIIZ;
				ci->pszVal = dbv.ptszVal;
				return 0;
			}
			break;
		case CNF_HOMEPAGE:
			if (!GetDatabaseString(ci,"Homepage",&dbv)) {
				ci->type = CNFT_ASCIIZ;
				ci->pszVal = dbv.ptszVal;
				return 0;
			}
			break;
		case CNF_AGE:
			if (!GetDatabaseString(ci,"Age",&dbv)) {
				ci->type = CNFT_WORD;
				ci->wVal = dbv.wVal;
				DBFreeVariant(&dbv);
				return 0;
			}
			break;
		case CNF_GENDER:
		{
			BYTE i;
			if (i=DBGetContactSettingByte(ci->hContact,ci->szProto,"Gender",0)) {
				ci->type = CNFT_BYTE;
				ci->bVal = i;
				return 0;
			}
			break;
		}
		case CNF_FIRSTLAST:
		{
			if(!GetDatabaseString(ci,"FirstName",&dbv)) {
				char *firstName=_strdup(dbv.pszVal);
				if(!GetDatabaseString(ci,"LastName",&dbv)) {
					char *buffer = (char*)malloc(strlen(firstName)+strlen(dbv.pszVal)+2);
					ci->type = CNFT_ASCIIZ;
					mir_snprintf(buffer,strlen(firstName)+strlen(dbv.pszVal)+2,"%s %s",firstName,dbv.pszVal);
					free(dbv.pszVal);
					free(firstName);
					ci->pszVal = ( TCHAR* )_strdup(buffer);
					free(buffer);
					return 0;
				}
			}
			break;
		}
		case CNF_UNIQUEID:
		{
			char *uid = (char*)CallProtoService(ci->szProto,PS_GETCAPS,PFLAG_UNIQUEIDSETTING,0);
			if ((int)uid!=CALLSERVICE_NOTFOUND&&uid) {
				if (!GetDatabaseString(ci,uid,&dbv)) {
					if (dbv.type==DBVT_BYTE) {
						ci->type = CNFT_BYTE;
						ci->bVal = dbv.bVal;
						return 0;
					}
					if (dbv.type==DBVT_WORD) {
						ci->type = CNFT_WORD;
						ci->wVal = dbv.wVal;
						return 0;
					}
					if (dbv.type==DBVT_DWORD) {
						ci->type = CNFT_DWORD;
						ci->dVal = dbv.dVal;
						return 0;
					}
					if (dbv.type==DBVT_ASCIIZ) {
						ci->type = CNFT_ASCIIZ;
						ci->pszVal = dbv.ptszVal;
						return 0;
					}
				}
			}
			break;
		}
		case CNF_DISPLAYNC:
		case CNF_DISPLAY:
		{
			int i;
			for(i=0;i<NAMEORDERCOUNT;i++) {
				switch(nameOrder[i])  {
					case 0: // custom name
					{
						// make sure we aren't in CNF_DISPLAYNC mode
						// don't get custom name for NULL contact
						char* saveProto = ci->szProto; ci->szProto = "CList";
						if (ci->hContact!=NULL && (ci->dwFlag&0x7F)==CNF_DISPLAY && !GetDatabaseString(ci,"MyHandle",&dbv)) {
							ci->type = CNFT_ASCIIZ;
							ci->pszVal = dbv.ptszVal;
							return 0;
						}
						ci->szProto = saveProto;
						break;
					}
					case 1: // nick
					{
						if (!GetDatabaseString(ci,"Nick",&dbv)) {
							ci->type = CNFT_ASCIIZ;
							ci->pszVal = dbv.ptszVal;
							return 0;
						}
						break;
					}
					case 2: // First Name
					{
						if (!GetDatabaseString(ci,"FirstName",&dbv)) {
							ci->type = CNFT_ASCIIZ;
							ci->pszVal = dbv.ptszVal;
							return 0;
						}
						break;
					}
					case 3: // E-mail
					{
						if (!GetDatabaseString(ci,"e-mail",&dbv)) {
							ci->type = CNFT_ASCIIZ;
							ci->pszVal = dbv.ptszVal;
							return 0;
						}
						break;
					}
					case 4: // Last Name
					{
						if (!GetDatabaseString(ci,"LastName",&dbv)) {
							ci->type = CNFT_ASCIIZ;
							ci->pszVal = dbv.ptszVal;
							return 0;
						}
						break;
					}
					case 5: // Unique id
					{
						// protocol must define a PFLAG_UNIQUEIDSETTING
						char *uid = (char*)CallProtoService(ci->szProto,PS_GETCAPS,PFLAG_UNIQUEIDSETTING,0);
						if ((int)uid!=CALLSERVICE_NOTFOUND&&uid) {
							if (!GetDatabaseString(ci,uid,&dbv)) {
								if (dbv.type==DBVT_BYTE||dbv.type==DBVT_WORD||dbv.type==DBVT_DWORD) {
									char buf[256];
									ci->type = CNFT_ASCIIZ;
									mir_snprintf(buf,sizeof(buf),"%u",dbv.type==DBVT_BYTE?dbv.bVal:(dbv.type==DBVT_WORD?dbv.wVal:dbv.dVal));
									ci->pszVal = ( TCHAR* )_strdup(buf);
									return 0;
								}
								else if (dbv.type==DBVT_ASCIIZ) {
									ci->type = CNFT_ASCIIZ;
									ci->pszVal = dbv.ptszVal;
									return 0;
								}
							}
						}
						break;
					}
					case 6: // first + last name
						if(!GetDatabaseString(ci,"FirstName",&dbv)) {
							char *firstName=_strdup(dbv.pszVal);
							if(!GetDatabaseString(ci,"LastName",&dbv)) {
								char *buffer = (char*)malloc(strlen(firstName)+strlen(dbv.pszVal)+2);
								ci->type = CNFT_ASCIIZ;
								mir_snprintf(buffer,strlen(firstName)+strlen(dbv.pszVal)+2,"%s %s",firstName,dbv.pszVal);
								free(dbv.pszVal);
								free(firstName);
								ci->pszVal = ( TCHAR* )_strdup(buffer);
								free(buffer);
								return 0;
							}
						}
						break;
					case 7:
						if ( ci->dwFlag & CNF_UNICODE )
							ci->pszVal = ( TCHAR* )wcsdup( TranslateW( L"'(Unknown Contact)'" ));
						else
							ci->pszVal = ( TCHAR* )_strdup( Translate("'(Unknown Contact)'"));
						ci->type = CNFT_ASCIIZ;
						return 0;
				}
			}
		}
	}
	return 1;
}

struct ContactOptionsData {
	int dragging;
	HTREEITEM hDragItem;
};

static BOOL CALLBACK ContactOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{	struct ContactOptionsData *dat;

	dat=(struct ContactOptionsData*)GetWindowLong(hwndDlg,GWL_USERDATA);
	switch (msg)
	{
		case WM_INITDIALOG: 
		{	TranslateDialogDefault(hwndDlg);
			dat=(struct ContactOptionsData*)malloc(sizeof(struct ContactOptionsData));
			SetWindowLong(hwndDlg,GWL_USERDATA,(LONG)dat);
			dat->dragging=0;
			SetWindowLong(GetDlgItem(hwndDlg,IDC_NAMEORDER),GWL_STYLE,GetWindowLong(GetDlgItem(hwndDlg,IDC_NAMEORDER),GWL_STYLE)|TVS_NOHSCROLL);
			{	TVINSERTSTRUCTA tvis;
				int i;
				tvis.hParent=NULL;
				tvis.hInsertAfter=TVI_LAST;
				tvis.item.mask=TVIF_TEXT|TVIF_PARAM;
				for(i=0;i<sizeof(nameOrderDescr)/sizeof(nameOrderDescr[0]);i++) {
					tvis.item.lParam=nameOrder[i];
					tvis.item.pszText=Translate(nameOrderDescr[nameOrder[i]]);
					SendMessageA( GetDlgItem(hwndDlg,IDC_NAMEORDER), TVM_INSERTITEMA, 0, (LPARAM)&tvis );
				}
			}
			return TRUE;
		}
		case WM_NOTIFY:
			switch (((LPNMHDR)lParam)->idFrom) {
				case 0:
					switch (((LPNMHDR)lParam)->code)
					{
						case PSN_APPLY:
							{	DBCONTACTWRITESETTING cws;
								TVITEM tvi;
								int i;
								cws.szModule="Contact";
								cws.szSetting="NameOrder";
								cws.value.type=DBVT_BLOB;
								cws.value.cpbVal=sizeof(nameOrderDescr)/sizeof(nameOrderDescr[0]);
								cws.value.pbVal=nameOrder;
								tvi.hItem=TreeView_GetRoot(GetDlgItem(hwndDlg,IDC_NAMEORDER));
								i=0;
								while(tvi.hItem!=NULL) {
									tvi.mask=TVIF_PARAM|TVIF_HANDLE;
									TreeView_GetItem(GetDlgItem(hwndDlg,IDC_NAMEORDER),&tvi);
									nameOrder[i++]=(BYTE)tvi.lParam;
									tvi.hItem=TreeView_GetNextSibling(GetDlgItem(hwndDlg,IDC_NAMEORDER),tvi.hItem);
								}
								CallService(MS_DB_CONTACT_WRITESETTING,(WPARAM)(HANDLE)NULL,(LPARAM)&cws);
								CallService(MS_CLIST_INVALIDATEDISPLAYNAME,(WPARAM)INVALID_HANDLE_VALUE,0);
							}
					}
					break;
				case IDC_NAMEORDER:
					switch (((LPNMHDR)lParam)->code) {
						case TVN_BEGINDRAG:
							if(((LPNMTREEVIEW)lParam)->itemNew.lParam==0 || ((LPNMTREEVIEW)lParam)->itemNew.lParam==sizeof(nameOrderDescr)/sizeof(nameOrderDescr[0])-1) break;
							SetCapture(hwndDlg);
							dat->dragging=1;
							dat->hDragItem=((LPNMTREEVIEW)lParam)->itemNew.hItem;
							TreeView_SelectItem(GetDlgItem(hwndDlg,IDC_NAMEORDER),dat->hDragItem);
							break;
					}
					break;
			}
			break;
		case WM_MOUSEMOVE:
			if(!dat->dragging) break;
			{	TVHITTESTINFO hti;
				hti.pt.x=(short)LOWORD(lParam);
				hti.pt.y=(short)HIWORD(lParam);
				ClientToScreen(hwndDlg,&hti.pt);
				ScreenToClient(GetDlgItem(hwndDlg,IDC_NAMEORDER),&hti.pt);
				TreeView_HitTest(GetDlgItem(hwndDlg,IDC_NAMEORDER),&hti);
				if(hti.flags&(TVHT_ONITEM|TVHT_ONITEMRIGHT)) {
					hti.pt.y-=TreeView_GetItemHeight(GetDlgItem(hwndDlg,IDC_NAMEORDER))/2;
					TreeView_HitTest(GetDlgItem(hwndDlg,IDC_NAMEORDER),&hti);
					TreeView_SetInsertMark(GetDlgItem(hwndDlg,IDC_NAMEORDER),hti.hItem,1);
				}
				else {
					if(hti.flags&TVHT_ABOVE) SendDlgItemMessage(hwndDlg,IDC_NAMEORDER,WM_VSCROLL,MAKEWPARAM(SB_LINEUP,0),0);
					if(hti.flags&TVHT_BELOW) SendDlgItemMessage(hwndDlg,IDC_NAMEORDER,WM_VSCROLL,MAKEWPARAM(SB_LINEDOWN,0),0);
					TreeView_SetInsertMark(GetDlgItem(hwndDlg,IDC_NAMEORDER),NULL,0);
				}
			}
			break;
		case WM_LBUTTONUP:
			if(!dat->dragging) break;
			TreeView_SetInsertMark(GetDlgItem(hwndDlg,IDC_NAMEORDER),NULL,0);
			dat->dragging=0;
			ReleaseCapture();
			{	TVHITTESTINFO hti;
				TVITEM tvi;
				hti.pt.x=(short)LOWORD(lParam);
				hti.pt.y=(short)HIWORD(lParam);
				ClientToScreen(hwndDlg,&hti.pt);
				ScreenToClient(GetDlgItem(hwndDlg,IDC_NAMEORDER),&hti.pt);
				hti.pt.y-=TreeView_GetItemHeight(GetDlgItem(hwndDlg,IDC_NAMEORDER))/2;
				TreeView_HitTest(GetDlgItem(hwndDlg,IDC_NAMEORDER),&hti);
				if(dat->hDragItem==hti.hItem) break;
				tvi.mask=TVIF_HANDLE|TVIF_PARAM;
				tvi.hItem=hti.hItem;
				TreeView_GetItem(GetDlgItem(hwndDlg,IDC_NAMEORDER),&tvi);
				if(tvi.lParam==sizeof(nameOrderDescr)/sizeof(nameOrderDescr[0])-1) break;
				if(hti.flags&(TVHT_ONITEM|TVHT_ONITEMRIGHT)) {
					TVINSERTSTRUCT tvis;
					TCHAR name[128];
					tvis.item.mask=TVIF_HANDLE|TVIF_PARAM|TVIF_TEXT|TVIF_PARAM;
					tvis.item.stateMask=0xFFFFFFFF;
					tvis.item.pszText=name;
					tvis.item.cchTextMax=sizeof(name);
					tvis.item.hItem=dat->hDragItem;
					TreeView_GetItem(GetDlgItem(hwndDlg,IDC_NAMEORDER),&tvis.item);
					TreeView_DeleteItem(GetDlgItem(hwndDlg,IDC_NAMEORDER),dat->hDragItem);
					tvis.hParent=NULL;
					tvis.hInsertAfter=hti.hItem;
					TreeView_SelectItem(GetDlgItem(hwndDlg,IDC_NAMEORDER),TreeView_InsertItem(GetDlgItem(hwndDlg,IDC_NAMEORDER),&tvis));
					SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				}
			}
			break;
		case WM_DESTROY:
			free(dat);
			break;
	}
	return FALSE;
}

static int ContactOptInit(WPARAM wParam,LPARAM lParam) {
	OPTIONSDIALOGPAGE odp;

	ZeroMemory(&odp,sizeof(odp));
	odp.cbSize=sizeof(odp);
	odp.position=-1000000000;
	odp.hInstance=GetModuleHandle(NULL);
	odp.pszTemplate=MAKEINTRESOURCEA(IDD_OPT_CONTACT);
	odp.pszGroup=Translate("Contact List");
	odp.pszTitle=Translate("Contact Display");
	odp.pfnDlgProc=ContactOpts;
	odp.flags=ODPF_BOLDGROUPS;
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);
	return 0;
}

int LoadContactsModule(void) {
	{	// Load the name order
		int i;
		DBVARIANT dbv;
		for(i=0;i<NAMEORDERCOUNT;i++) nameOrder[i]=i;
		if(!DBGetContactSetting(NULL,"Contact","NameOrder",&dbv)) {
			CopyMemory(nameOrder,dbv.pbVal,dbv.cpbVal);
			DBFreeVariant(&dbv);
		}
	}
	CreateServiceFunction(MS_CONTACT_GETCONTACTINFO,GetContactInfo);
	HookEvent(ME_OPT_INITIALISE,ContactOptInit);
	return 0;
}
