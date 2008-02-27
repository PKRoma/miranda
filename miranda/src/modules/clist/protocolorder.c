
// options dialog for protocol order and visibility
// written by daniel vijge
// gpl license ect...

#include "commonheaders.h"
#include "clc.h"

typedef struct tagProtocolData
{
	char *RealName;
	int protopos;
	boolean show;
}
	ProtocolData;

struct ProtocolOrderData
{
	int dragging;
	HTREEITEM hDragItem;
};

typedef struct {
	char* protoName;
	int   visible;
}
	tempProtoItem;

char* DBGetStringA( HANDLE hContact,const char *szModule,const char *szSetting )
{
	char* str = NULL;
   DBVARIANT dbv;
	if ( !DBGetContactSetting( hContact, szModule, szSetting, &dbv )) {
		if ( dbv.type == DBVT_ASCIIZ )
        str = mir_strdup( dbv.pszVal );
		DBFreeVariant( &dbv );
	}
	return str;
}

static int __inline isProtoSuitable( const char* szProto )
{
   return CallProtoService(szProto, PS_GETCAPS, PFLAGNUM_2, 0 ) & 
				~CallProtoService(szProto, PS_GETCAPS, PFLAGNUM_5, 0 );
}

int CheckProtocolOrder()
{
	BOOL protochanged = FALSE;
	int ver = DBGetContactSettingDword( 0, "Protocols", "PrVer", -1 ), i;
	if ( ver != 4 )
		protochanged = TRUE;

	if ( accounts.count == 0 )
		protochanged = TRUE;
	else if ( accounts.count != DBGetContactSettingDword( 0, "Protocols", "ProtoCount", -1 ))
		protochanged = TRUE;

	for ( i=0; i < accounts.count; i++ ) {
		PROTOACCOUNT* pa = accounts.items[i];
		int bCalcVisibility = isProtoSuitable( pa->szModuleName ) ? 1 : 0;
		if ( pa->bIsVisible != bCalcVisibility ) {
			pa->bIsVisible = bCalcVisibility;
			protochanged = TRUE;
	}	}

	if ( !protochanged )
		return 0;

	WriteDbAccounts();
	return 1;
}

int FillTree(HWND hwnd)
{
	ProtocolData *PD;
	char szName[64];
	TCHAR *buf2=NULL;
	int i;

	TVINSERTSTRUCT tvis;
	tvis.hParent=NULL;
	tvis.hInsertAfter=TVI_LAST;
	tvis.item.mask=TVIF_PARAM|TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;	

	//	ProtocolOrder_CheckOrder();
	TreeView_DeleteAllItems(hwnd);
	if ( accounts.count == 0 )
		return FALSE;

	for ( i = 0; i < accounts.count; i++ ) {
		PROTOACCOUNT* pa = accounts.items[i];
		if ( !pa->bIsEnabled )
			continue;

		if ( CALLSERVICE_NOTFOUND == CallProtoService( pa->szModuleName, PS_GETNAME, sizeof( szName ), ( LPARAM )szName ))
			continue;

		PD = ( ProtocolData* )mir_alloc( sizeof( ProtocolData ));
		PD->RealName = pa->szModuleName;
		PD->show = pa->bIsVisible;
		PD->protopos = pa->iOrder;

		tvis.item.lParam = ( LPARAM )PD;
		#ifdef UNICODE
			buf2 = a2u( szName );
			tvis.item.pszText = TranslateTS( buf2 );
		#else
			tvis.item.pszText = Translate( szName );
		#endif			

		tvis.item.iImage = tvis.item.iSelectedImage = PD->show;
		TreeView_InsertItem( hwnd, &tvis );
		#ifdef UNICODE
			mir_free( buf2 );
		#endif
	}

	return 0;
}

BOOL CALLBACK ProtocolOrderOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct ProtocolOrderData *dat = (struct ProtocolOrderData*)GetWindowLong(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),GWL_USERDATA);

	switch (msg) {
	case WM_DESTROY:
		mir_free( dat );
		break;

	case WM_INITDIALOG: 
		TranslateDialogDefault(hwndDlg);
		dat=(struct ProtocolOrderData*)mir_alloc(sizeof(struct ProtocolOrderData));
		SetWindowLong(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),GWL_USERDATA,(long)dat);
		dat->dragging=0;

		SetWindowLong(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),GWL_STYLE,GetWindowLong(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),GWL_STYLE)|TVS_NOHSCROLL);
		{
			HIMAGELIST himlCheckBoxes = ImageList_Create( GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ), ILC_COLOR32|ILC_MASK, 2, 2 );
			ImageList_AddIcon_IconLibLoaded(himlCheckBoxes, SKINICON_OTHER_NOTICK);
			ImageList_AddIcon_IconLibLoaded(himlCheckBoxes, SKINICON_OTHER_TICK);
			TreeView_SetImageList(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),himlCheckBoxes,TVSIL_NORMAL);
		}

		CheckProtocolOrder();
		FillTree( GetDlgItem( hwndDlg, IDC_PROTOCOLORDER ));
		return TRUE;

	case WM_COMMAND:
		if ( HIWORD( wParam ) == BN_CLICKED ) {
			if (lParam==(LPARAM)GetDlgItem(hwndDlg,IDC_RESETPROTOCOLDATA))	{
				DBWriteContactSettingDword( 0, "Protocols", "PrVer", -1 );
				CheckProtocolOrder();
				FillTree(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER));
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
				return 0;
		}	}
		break;

	case WM_NOTIFY:
		switch(((LPNMHDR)lParam)->idFrom) {		
		case 0: 
			if (((LPNMHDR)lParam)->code == PSN_APPLY ) {
				int count = 0;

				TVITEM tvi;
				tvi.hItem = TreeView_GetRoot( GetDlgItem( hwndDlg, IDC_PROTOCOLORDER ));
				tvi.cchTextMax = 32;
				tvi.mask = TVIF_PARAM | TVIF_HANDLE;

				while ( tvi.hItem != NULL ) {
					TreeView_GetItem(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),&tvi);

					if (tvi.lParam!=0) {
						ProtocolData* ppd = ( ProtocolData* )tvi.lParam;
						PROTOACCOUNT* pa = Proto_GetAccount( ppd->RealName );
						if ( pa != NULL ) {
							pa->iOrder = count;
							pa->bIsVisible = ppd->show;
						}
					}

					tvi.hItem = TreeView_GetNextSibling( GetDlgItem( hwndDlg, IDC_PROTOCOLORDER ), tvi.hItem );
					count++;
				}
				
				WriteDbAccounts();
				cli.pfnReloadProtoMenus();
				cli.pfnTrayIconIconsChanged();
				cli.pfnClcBroadcast( INTM_RELOADOPTIONS, 0, 0 );
				cli.pfnClcBroadcast( INTM_INVALIDATE, 0, 0 );
			}
			break;

		case IDC_PROTOCOLORDER:
			switch (((LPNMHDR)lParam)->code) {
			case TVN_DELETEITEMA: 
				{
					NMTREEVIEWA * pnmtv = (NMTREEVIEWA *) lParam;
					if (pnmtv && pnmtv->itemOld.lParam)
						mir_free((ProtocolData*)pnmtv->itemOld.lParam);
				}
				break;

			case TVN_BEGINDRAGA:
				SetCapture(hwndDlg);
				dat->dragging=1;
				dat->hDragItem=((LPNMTREEVIEW)lParam)->itemNew.hItem;
				TreeView_SelectItem(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),dat->hDragItem);
				break;

			case NM_CLICK:
				{
					TVHITTESTINFO hti;
					hti.pt.x=(short)LOWORD(GetMessagePos());
					hti.pt.y=(short)HIWORD(GetMessagePos());
					ScreenToClient(((LPNMHDR)lParam)->hwndFrom,&hti.pt);
					if ( TreeView_HitTest(((LPNMHDR)lParam)->hwndFrom, &hti )) {
						if ( hti.flags & TVHT_ONITEMICON ) {
							TVITEMA tvi;
							tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
							tvi.hItem=hti.hItem;
							TreeView_GetItem(((LPNMHDR)lParam)->hwndFrom,&tvi);
							tvi.iImage=tvi.iSelectedImage=!tvi.iImage;
							((ProtocolData *)tvi.lParam)->show=tvi.iImage;
							TreeView_SetItem(((LPNMHDR)lParam)->hwndFrom,&tvi);
							SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
			}	}	}	}
			break;
		}
		break;

	case WM_MOUSEMOVE:
		if ( dat->dragging ) {
			TVHITTESTINFO hti;
			hti.pt.x=(short)LOWORD(lParam);
			hti.pt.y=(short)HIWORD(lParam);
			ClientToScreen(hwndDlg,&hti.pt);
			ScreenToClient(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),&hti.pt);
			TreeView_HitTest(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),&hti);
			if ( hti.flags & (TVHT_ONITEM|TVHT_ONITEMRIGHT )) {
				HTREEITEM it = hti.hItem;
				hti.pt.y -= TreeView_GetItemHeight(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER))/2;
				TreeView_HitTest(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),&hti);
				if ( !( hti.flags & TVHT_ABOVE ))
					TreeView_SetInsertMark(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),hti.hItem,1);
				else 
					TreeView_SetInsertMark(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),it,0);
			}
			else {
				if(hti.flags&TVHT_ABOVE) SendDlgItemMessage(hwndDlg,IDC_PROTOCOLORDER,WM_VSCROLL,MAKEWPARAM(SB_LINEUP,0),0);
				if(hti.flags&TVHT_BELOW) SendDlgItemMessage(hwndDlg,IDC_PROTOCOLORDER,WM_VSCROLL,MAKEWPARAM(SB_LINEDOWN,0),0);
				TreeView_SetInsertMark(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),NULL,0);
		}	}
		break;

	case WM_LBUTTONUP:
		if ( dat->dragging ) {
			TVHITTESTINFO hti;
			TVITEM tvi;

			TreeView_SetInsertMark(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),NULL,0);
			dat->dragging=0;
			ReleaseCapture();
		
			hti.pt.x=(short)LOWORD(lParam);
			hti.pt.y=(short)HIWORD(lParam);
			ClientToScreen(hwndDlg,&hti.pt);
			ScreenToClient(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),&hti.pt);
			hti.pt.y-=TreeView_GetItemHeight(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER))/2;
			TreeView_HitTest(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),&hti);
			if(dat->hDragItem==hti.hItem) break;
			if (hti.flags&TVHT_ABOVE) hti.hItem=TVI_FIRST;
			tvi.mask=TVIF_HANDLE|TVIF_PARAM;
			tvi.hItem=dat->hDragItem;
			TreeView_GetItem(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),&tvi);
			if ( hti.flags & (TVHT_ONITEM|TVHT_ONITEMRIGHT )||(hti.hItem==TVI_FIRST)) {
				TVINSERTSTRUCT tvis;
				TCHAR name[128];
				ProtocolData * lpOldData;
				tvis.item.mask = TVIF_HANDLE|TVIF_PARAM|TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
				tvis.item.stateMask = 0xFFFFFFFF;
				tvis.item.pszText = name;
				tvis.item.cchTextMax = SIZEOF(name);
				tvis.item.hItem = dat->hDragItem;
				tvis.item.iImage = tvis.item.iSelectedImage = ((ProtocolData *)tvi.lParam)->show;
				TreeView_GetItem(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),&tvis.item);

				//the pointed lParam will be freed inside TVN_DELETEITEM 
				//so lets substitute it with 0
				lpOldData=(ProtocolData *)tvis.item.lParam; 
				tvis.item.lParam=0; 
				TreeView_SetItem(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),&tvis.item);               
				tvis.item.lParam=(LPARAM)lpOldData; 

				//now current item contain lParam=0 we can delete it. the memory will be kept.               
				TreeView_DeleteItem(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),dat->hDragItem);
				tvis.hParent = NULL;
				tvis.hInsertAfter = hti.hItem;
				TreeView_SelectItem(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),TreeView_InsertItem(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),&tvis));
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
		}	}
		break; 
	}
	return FALSE;
}
