/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-07  George Hazan
Copyright ( C ) 2007     Victor Pavlychko

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

*/

#include "jabber.h"

#ifdef _WIN32_WINNT
	#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x501

#include <commctrl.h>

#define TLIF_VISIBLE	0x01
#define TLIF_EXPANDED	0x02
#define TLIF_MODIFIED	0x04
#define TLIF_ROOT		0X08
#define TLIF_HASITEM	0X10
#define TLIF_REBUILD	0x20

struct TTreeList_ItemInfo
{
	BYTE flags;
	int indent, sortIndex;

	struct TTreeList_ItemInfo *parent;
	int iIcon, iOverlay;
	LIST<TCHAR> text;
	LPARAM data;
	LIST<TTreeList_ItemInfo> subItems;

	TTreeList_ItemInfo(int columns = 3, int children = 5):
		text(columns), subItems(children), parent(NULL),
		flags(0), indent(0), sortIndex(0), iIcon(0), iOverlay(0), data(0) {}
	~TTreeList_ItemInfo()
	{
		text.destroy();
		for (int i = subItems.getCount(); i--; )
			delete subItems[i];
	}
};

// static utilities
static void sttTreeList_RecursiveApply(HTREELISTITEM hItem, void (*func)(HTREELISTITEM, LPARAM), LPARAM data);
static void sttTreeList_ResetIndex(HTREELISTITEM hItem, LPARAM data);
static void sttTreeList_CreateItems(HTREELISTITEM hItem, LPARAM data);
static int CALLBACK sttTreeList_SortFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

// tree list implementation
LPARAM TreeList_GetData(HTREELISTITEM hItem)
{
	return hItem->data;
}

int TreeList_GetChildrenCount(HTREELISTITEM hItem)
{
	return hItem->subItems.getCount();
}

HTREELISTITEM TreeList_GetChild(HTREELISTITEM hItem, int i)
{
	return hItem->subItems[i];
}

void TreeList_Create(HWND hwnd)
{
	TTreeList_ItemInfo *root = new TTreeList_ItemInfo;
	root->flags = TLIF_EXPANDED|TLIF_VISIBLE|TLIF_ROOT;
	root->indent = -1;
	SetWindowLong(hwnd, GWL_USERDATA, (LONG)root);

	ListView_SetExtendedListViewStyle(hwnd, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES | LVS_EX_INFOTIP );

	HIMAGELIST hIml;
	hIml = ImageList_Create(16, 16, ILC_MASK + ( IsWinVerXPPlus() ? ILC_COLOR32 : ILC_COLOR16 ), 2, 1);
	ListView_SetImageList (hwnd, hIml, LVSIL_SMALL);

	hIml = ImageList_Create(16, 16, ILC_MASK + ( IsWinVerXPPlus() ? ILC_COLOR32 : ILC_COLOR16 ), 2, 1);
	ImageList_AddIcon(hIml, (HICON)JCallService( MS_SKIN_LOADICON, SKINICON_OTHER_GROUPOPEN, 0 ));
	ImageList_AddIcon(hIml, (HICON)JCallService( MS_SKIN_LOADICON, SKINICON_OTHER_GROUPSHUT, 0 ));
	ListView_SetImageList (hwnd, hIml, LVSIL_STATE);
}

void TreeList_Destroy(HWND hwnd)
{
	ListView_DeleteAllItems(hwnd);
	TTreeList_ItemInfo *root = (TTreeList_ItemInfo *)GetWindowLong(hwnd, GWL_USERDATA);
	delete root;
}

HTREELISTITEM TreeList_AddItem(HWND hwnd, HTREELISTITEM hParent, TCHAR *text, LPARAM data)
{
	if (!hParent)
		hParent = (HTREELISTITEM)GetWindowLong(hwnd, GWL_USERDATA);

	TTreeList_ItemInfo *item = new TTreeList_ItemInfo;
	item->data = data;
	item->parent = hParent;
	item->text.insert(mir_tstrdup(text));
	item->flags |= TLIF_MODIFIED;
	item->indent = hParent->indent+1;
	hParent->subItems.insert(item);
	return item;
}

void TreeList_AppendColumn(HTREELISTITEM hItem, TCHAR *text)
{
	hItem->text.insert(mir_tstrdup(text));
	hItem->flags |= TLIF_MODIFIED;
}

int TreeList_AddIcon(HWND hwnd, HICON hIcon, int iOverlay)
{
	HIMAGELIST hIml = ListView_GetImageList(hwnd, LVSIL_SMALL);
	int idx = ImageList_AddIcon(hIml, hIcon);
	if (iOverlay) ImageList_SetOverlayImage(hIml, idx, iOverlay);
	return idx;
}

void TreeList_SetIcon(HTREELISTITEM hItem, int iIcon, int iOverlay)
{
	if (iIcon >= 0) hItem->iIcon = iIcon;
	if (iOverlay >= 0) hItem->iOverlay = iOverlay;
	if ((iIcon >= 0) || (iOverlay >= 0)) hItem->flags |= TLIF_MODIFIED;
}

void TreeList_Update(HWND hwnd)
{
	HTREELISTITEM hItem = (HTREELISTITEM)GetWindowLong(hwnd, GWL_USERDATA);
	LVITEM lvi = {0};
	int sortIndex = 0;

	SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);
	sttTreeList_RecursiveApply(hItem, sttTreeList_ResetIndex, (LPARAM)&sortIndex);
	for ( int i = ListView_GetItemCount(hwnd); i--; ) {
		LVITEM lvi = {0};
		lvi.mask = LVIF_PARAM;
		lvi.iItem = i;
		lvi.iSubItem = 0;
		ListView_GetItem(hwnd, &lvi);

		HTREELISTITEM ptli = ( HTREELISTITEM )lvi.lParam;
		if ( ptli->flags & TLIF_VISIBLE ) {
			ptli->flags |= TLIF_HASITEM;
			if ( ptli->flags & TLIF_MODIFIED ) {
				lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE | LVIF_TEXT;
				lvi.iItem = i;
				lvi.iSubItem = 0;
				lvi.pszText = ptli->text[0];
				lvi.stateMask = LVIS_OVERLAYMASK|LVIS_STATEIMAGEMASK;
				lvi.iImage = ptli->iIcon;
				lvi.state =
					INDEXTOSTATEIMAGEMASK(
						(ptli->subItems.getCount() == 0) ? 0 : (ptli->flags & TLIF_EXPANDED) ? 1 : 2 ) |
					INDEXTOOVERLAYMASK( ptli->iOverlay );
				ListView_SetItem(hwnd, &lvi);
				for (int j = 1; j < ptli->text.getCount(); ++j)
					ListView_SetItemText( hwnd, i, j, ptli->text[j]);
			}
		} 
		else ListView_DeleteItem(hwnd, i);
	}

	sttTreeList_RecursiveApply(hItem, sttTreeList_CreateItems, (LPARAM)hwnd);
	ListView_SortItems(hwnd, sttTreeList_SortFunc, 0);
	SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
	UpdateWindow(hwnd);
}

BOOL TreeList_ProcessMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT idc, BOOL *result)
{
	LVITEM lvi = {0};

	switch (msg) {
	case WM_NOTIFY:
		if (((LPNMHDR)lparam)->idFrom != idc)
			break;

		switch (((LPNMHDR)lparam)->code) {
		case LVN_KEYDOWN:
			{
				LPNMLVKEYDOWN lpnmlvk = (LPNMLVKEYDOWN)lparam;

				lvi.mask = LVIF_PARAM|LVIF_INDENT;
				lvi.iItem = ListView_GetNextItem(lpnmlvk->hdr.hwndFrom, -1, LVNI_SELECTED);
				if (lvi.iItem < 0) return FALSE;
				lvi.iSubItem = 0;
				ListView_GetItem(lpnmlvk->hdr.hwndFrom, &lvi);
				HTREELISTITEM hItem = (HTREELISTITEM)lvi.lParam;

				switch (lpnmlvk->wVKey) {
				case VK_SUBTRACT:
				case VK_LEFT:
				{
					if ( hItem->subItems.getCount() && (hItem->flags & TLIF_EXPANDED )) {
						hItem->flags &= ~TLIF_EXPANDED;
						hItem->flags |= TLIF_MODIFIED;
						TreeList_Update( lpnmlvk->hdr.hwndFrom );
					} 
					else if ( hItem->indent && (lpnmlvk->wVKey != VK_SUBTRACT )) {
						for ( int i = lvi.iItem; i--; ) {
							lvi.mask = LVIF_INDENT;
							lvi.iItem = i;
							lvi.iSubItem = 0;
							ListView_GetItem(lpnmlvk->hdr.hwndFrom, &lvi);
							if (lvi.iIndent < hItem->indent) {
								lvi.mask = LVIF_STATE;
								lvi.iItem = i;
								lvi.iSubItem = 0;
								lvi.state = lvi.stateMask = LVIS_FOCUSED|LVNI_SELECTED;
								ListView_SetItem(lpnmlvk->hdr.hwndFrom, &lvi);
								break;
					}	}	}
					break;
				}

				case VK_ADD:
				case VK_RIGHT:
					if ( hItem->subItems.getCount() && !( hItem->flags & TLIF_EXPANDED )) {
						hItem->flags |= TLIF_EXPANDED;
						hItem->flags |= TLIF_MODIFIED;

						NMTREEVIEW nmtv;
						nmtv.hdr.code = TVN_ITEMEXPANDED;
						nmtv.hdr.hwndFrom = lpnmlvk->hdr.hwndFrom;
						nmtv.hdr.idFrom = lpnmlvk->hdr.idFrom;
						nmtv.itemNew.hItem = (HTREEITEM)hItem;
						SendMessage(hwnd, WM_NOTIFY, lpnmlvk->hdr.idFrom, (LPARAM)&nmtv);
						TreeList_Update( lpnmlvk->hdr.hwndFrom );
					}
					break;
			}	}
			break;

		case NM_CLICK:
			{
				LPNMITEMACTIVATE lpnmia = (LPNMITEMACTIVATE)lparam;
				LVHITTESTINFO lvhti = {0};
				lvi.mask = LVIF_PARAM;
				lvi.iItem = lpnmia->iItem;
				ListView_GetItem(lpnmia->hdr.hwndFrom, &lvi);
				lvhti.pt = lpnmia->ptAction;
				ListView_HitTest(lpnmia->hdr.hwndFrom, &lvhti);

				HTREELISTITEM ptli = ( HTREELISTITEM )lvi.lParam;
				if ((lvhti.iSubItem == 0) && ( lvhti.flags & LVHT_ONITEMSTATEICON ) && ptli->subItems.getCount()) {
					if ( ptli->flags & TLIF_EXPANDED )
						ptli->flags &= ~TLIF_EXPANDED;
					else {
						ptli->flags |= TLIF_EXPANDED;

						NMTREEVIEW nmtv;
						nmtv.hdr.code = TVN_ITEMEXPANDED;
						nmtv.hdr.hwndFrom = lpnmia->hdr.hwndFrom;
						nmtv.hdr.idFrom = lpnmia->hdr.idFrom;
						nmtv.itemNew.hItem = (HTREEITEM)lvi.lParam;
						SendMessage(hwnd, WM_NOTIFY, lpnmia->hdr.idFrom, (LPARAM)&nmtv);
					}
					ptli->flags |= TLIF_MODIFIED;
					TreeList_Update( lpnmia->hdr.hwndFrom );
			}	}
			break;
		}
		break;
	}
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////
static void sttTreeList_RecursiveApply(HTREELISTITEM hItem, void (*func)(HTREELISTITEM, LPARAM), LPARAM data)
{
	for ( int i = 0; i < hItem->subItems.getCount(); i++ ) {
		func( hItem->subItems[i], data );
		sttTreeList_RecursiveApply( hItem->subItems[i], func, data );
}	}

static void sttTreeList_ResetIndex(HTREELISTITEM hItem, LPARAM data)
{
	hItem->flags &= ~TLIF_HASITEM;

	if ( !hItem->parent || (hItem->parent->flags & TLIF_VISIBLE) && (hItem->parent->flags & TLIF_EXPANDED ))
		hItem->flags |= TLIF_VISIBLE;
	else
		hItem->flags &= ~TLIF_VISIBLE;

	hItem->sortIndex = (*(int *)data)++;
}

static void sttTreeList_CreateItems(HTREELISTITEM hItem, LPARAM data)
{
	if (( hItem->flags & TLIF_VISIBLE ) && !( hItem->flags & TLIF_HASITEM ) && !( hItem->flags & TLIF_ROOT )) {
		LVITEM lvi = {0};
		lvi.mask = LVIF_INDENT | LVIF_PARAM | LVIF_IMAGE | LVIF_TEXT | LVIF_STATE;
		lvi.iIndent = hItem->indent;
		lvi.lParam = (LPARAM)hItem;
		lvi.pszText = hItem->text[0];
		lvi.stateMask = LVIS_OVERLAYMASK|LVIS_STATEIMAGEMASK;
		lvi.iImage = hItem->iIcon;
		lvi.state =
			INDEXTOSTATEIMAGEMASK(
				(hItem->subItems.getCount() == 0) ? 0 : (hItem->flags & TLIF_EXPANDED) ? 1 : 2) |
			INDEXTOOVERLAYMASK(hItem->iOverlay);

		int idx = ListView_InsertItem((HWND)data, &lvi);
		for ( int i = 1; i < hItem->text.getCount(); i++ )
			ListView_SetItemText((HWND)data, idx, i, hItem->text[i]);
}	}

static int CALLBACK sttTreeList_SortFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	HTREELISTITEM p1 = ( HTREELISTITEM )lParam1, p2 = ( HTREELISTITEM )lParam2; 
	if ( p1->sortIndex < p2->sortIndex )
		return -1;

	if ( p1->sortIndex > p2->sortIndex )
		return +1;

	return 0;
}
