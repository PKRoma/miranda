
// options dialog for protocol order and visibility
// written by daniel vijge
// gpl license ect...

#include "commonheaders.h"
#include "clc.h"

#define OFFSET_PROTOPOS 200
#define OFFSET_VISIBLE 400

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

#define PrVer 3

char **pszSettingName;
int arrlen;

int enumDB_ProtoProc( const char* szSetting, LPARAM lParam )
{
	if ( szSetting == NULL )
		return 0;

	arrlen++;
	pszSettingName = ( char** )mir_realloc( pszSettingName, arrlen*sizeof( char* ));
	pszSettingName[arrlen-1] = mir_strdup( szSetting );
	return 0;
}

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

int DeleteAllSettingInProtocols()
{
	DBCONTACTENUMSETTINGS dbces;
	arrlen=0;

	dbces.pfnEnumProc = enumDB_ProtoProc;
	dbces.szModule = "Protocols";
	dbces.ofsSettings = 0;

	CallService(MS_DB_CONTACT_ENUMSETTINGS,0,(LPARAM)&dbces);

	//delete all settings
	if ( arrlen ) {
		int i;
		for ( i=0; i < arrlen; i++ ) {
			DBDeleteContactSetting( 0, "Protocols", pszSettingName[i] );
			mir_free( pszSettingName[i] );
		}
		mir_free( pszSettingName );
		pszSettingName = NULL;
	}
	return 0;
}

static int __inline isProtoSuitable( const char* szProto )
{
   return CallProtoService(szProto, PS_GETCAPS, PFLAGNUM_2, 0 ) & 
				~CallProtoService(szProto, PS_GETCAPS, PFLAGNUM_5, 0 );
}

int CheckProtocolOrder()
{
	BOOL protochanged=FALSE;
	int StoredProtoCount;
	PROTOCOLDESCRIPTOR **protos;
	int i,j,count,v;
	char *curproto;
	char buf[10];
	tempProtoItem *items = NULL, *found;

	int ver = DBGetContactSettingDword( 0, "Protocols", "PrVer", -1 );
	if ( ver != PrVer )
		protochanged = TRUE;

	StoredProtoCount = DBGetContactSettingDword( 0,"Protocols","ProtoCount",-1 );
	if ( StoredProtoCount == -1 )
		protochanged = TRUE;
	else
		items = ( tempProtoItem* )alloca( sizeof( tempProtoItem )*StoredProtoCount );

	CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&count,(LPARAM)&protos);

	if ( !protochanged ) {
		v=0;

		//calc only needed protocols
		for ( i=0; i < count; i++ ) {
			if ( protos[i]->type != PROTOTYPE_PROTOCOL || !isProtoSuitable( protos[i]->szName ))
				continue;
			v++;
		}

		if ( StoredProtoCount != v )
			protochanged = TRUE;

		for ( i=0; i < StoredProtoCount; i++ ) {
			_itoa( i, buf, 10 );
			curproto = DBGetStringA( NULL, "Protocols", buf );
			if ( curproto == NULL )
				protochanged = TRUE;
			else
				items[i].protoName = NEWSTR_ALLOCA( curproto );

			if ( CallService( MS_PROTO_ISPROTOCOLLOADED, 0, ( LPARAM )curproto ) == 0 )
				protochanged = TRUE;

			mir_free( curproto );

			_itoa( OFFSET_VISIBLE+i, buf, 10 );
			items[i].visible = DBGetContactSettingDword( NULL, "Protocols", buf, 1 );
	}	}

	if ( !protochanged )
		return 0;

	//reseting all settings;
	DeleteAllSettingInProtocols();

	CallService( MS_PROTO_ENUMPROTOCOLS, ( WPARAM )&count, ( LPARAM )&protos );

	v=0;
	for ( i = 0; i < count; i++ ) {
		if ( protos[i]->type != PROTOTYPE_PROTOCOL || !isProtoSuitable( protos[i]->szName ))
			continue;

		found = NULL;
		for ( j = 0; j < StoredProtoCount; j++ )
			if ( !lstrcmpA( items[j].protoName, protos[i]->szName )) {
				found = items + j;
				break;
			}

		_itoa( v, buf, 10 );
		DBWriteContactSettingString( 0, "Protocols", buf, protos[i]->szName );

		_itoa( OFFSET_PROTOPOS + v, buf, 10 );//save pos in protos
		DBWriteContactSettingDword( 0, "Protocols", buf, v );

		_itoa( OFFSET_VISIBLE + v, buf, 10 );//save default visible status
		DBWriteContactSettingDword( 0, "Protocols", buf, ( found ) ? found->visible : 1 );

		v++;
	}

	DBDeleteContactSetting( 0, "Protocols", "ProtoCount" );
	DBWriteContactSettingDword( 0, "Protocols", "ProtoCount", v );
	DBWriteContactSettingDword( 0, "Protocols", "PrVer", PrVer );
	return 1;
}

int FillTree(HWND hwnd)
{
	TVINSERTSTRUCT tvis;
	ProtocolData *PD;
	char szName[64];
	char *szSTName;
	char buf[10];
	TCHAR *buf2=NULL;
	int i,count;

	tvis.hParent=NULL;
	tvis.hInsertAfter=TVI_LAST;
	tvis.item.mask=TVIF_PARAM|TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;	

	//	ProtocolOrder_CheckOrder();
	TreeView_DeleteAllItems(hwnd);
	count=DBGetContactSettingDword(0,"Protocols","ProtoCount",-1);
	if (count==-1){return(FALSE);};

	for ( i = 0; i < count; i++ ) {
		_itoa(i,	(char *)&buf,10);
		szSTName = DBGetStringA(0,"Protocols",(char *)&buf);		
        if ( szSTName == NULL )
			continue;

		CallProtoService( szSTName, PS_GETNAME, sizeof( szName ), ( LPARAM )szName );

		PD = ( ProtocolData* )mir_alloc( sizeof( ProtocolData ));
		PD->RealName = GetUniqueProtoName(szName); //it return static pointer to protocol name-> not net to be freed
		mir_free(szSTName);
		szSTName=PD->RealName;

		_itoa( OFFSET_VISIBLE+i, buf, 10 );
		PD->show=(boolean)DBGetContactSettingDword(0,"Protocols", buf, 1 );

		_itoa( OFFSET_PROTOPOS+i, buf, 10 );
		PD->protopos = DBGetContactSettingDword(0,"Protocols", buf, -1 );

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
				DBWriteContactSettingDword(0,"Protocols","ProtoCount",-1);
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
				int count;
				char buf[10];

				TVITEM tvi;
				tvi.hItem = TreeView_GetRoot( GetDlgItem( hwndDlg, IDC_PROTOCOLORDER ));
				tvi.cchTextMax = 32;
				tvi.mask = TVIF_PARAM | TVIF_HANDLE;
				count = 0;

				while ( tvi.hItem != NULL ) {
					_itoa( count, buf, 10 );
					TreeView_GetItem(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),&tvi);

					if (tvi.lParam!=0) {
						ProtocolData* ppd = ( ProtocolData* )tvi.lParam;
						DBWriteContactSettingString( NULL, "Protocols", buf, ppd->RealName );

						_itoa( OFFSET_PROTOPOS + count, buf, 10 );  //save position in protos
						DBWriteContactSettingDword( NULL, "Protocols", buf, ppd->protopos );

						_itoa( OFFSET_VISIBLE + count, buf, 10 );  //save visible in protos
						DBWriteContactSettingDword( NULL, "Protocols", buf, ppd->show );
					}

					tvi.hItem = TreeView_GetNextSibling( GetDlgItem( hwndDlg, IDC_PROTOCOLORDER ), tvi.hItem );
					count++;
				}

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
