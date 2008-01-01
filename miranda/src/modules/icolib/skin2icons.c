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

#define _WIN32_IE 0x0560
#define _WIN32_WINNT 0x0501

#include "commonheaders.h"

#include "IcoLib.h"

static HANDLE hIcons2ChangedEvent, hIconsChangedEvent;
static HICON hIconBlank = NULL;

HANDLE hIcoLib_AddNewIcon, hIcoLib_RemoveIcon, hIcoLib_GetIcon, hIcoLib_GetIcon2, 
       hIcoLib_IsManaged, hIcoLib_AddRef, hIcoLib_ReleaseIcon;

static HIMAGELIST hCacheIconList;
static int iconEventActive = 0;

static BOOL bNeedRebuild = FALSE;

struct IcoLibOptsData {
	HWND hwndIndex;
};

CRITICAL_SECTION csIconList;

#define SECTIONPARAM_MAKE(index, level, flags) MAKELONG( (index)&0xFFFF, MAKEWORD( level, flags ) )
#define SECTIONPARAM_INDEX(lparam) LOWORD( lparam )
#define SECTIONPARAM_LEVEL(lparam) LOBYTE( HIWORD(lparam) )
#define SECTIONPARAM_FLAGS(lparam) HIBYTE( HIWORD(lparam) )
#define SECTIONPARAM_HAVEPAGE	0x0001

struct
{
	SectionItem** items;
	int  count,   limit, increment;
	FSortFunc     sortFunc;
}
static sectionList;

struct
{
	IconItem**  items;
	int  count, limit, increment;
	FSortFunc   sortFunc;
}
static iconList;

/////////////////////////////////////////////////////////////////////////////////////////
// Utility functions

static void __fastcall MySetCursor(TCHAR* nCursor)
{	SetCursor( LoadCursor( NULL, nCursor ));
}

static void __fastcall SAFE_FREE(void** p)
{
	if ( *p ) {
		mir_free( *p );
		*p = NULL;
}	}

static void __fastcall SafeDestroyIcon( HICON* icon )
{
	if ( *icon ) {
		DestroyIcon( *icon );
		*icon = NULL;
}	}


// Helper function to get valid icons with valid alpha from ImageList, borrowed from clist_modern code
static HICON ImageList_GetIcon_Fixed( HIMAGELIST himl, int i, UINT fStyle )
{
	IMAGEINFO imi = { 0 };
	BITMAP bm = { 0 };

	if (IsWinVerXPPlus() && i != -1) {
		ImageList_GetImageInfo( himl, i, &imi );
		GetObject( imi.hbmImage, sizeof(bm), &bm);
		if ( bm.bmBitsPixel == 32 ) //stupid bug of Microsoft 
			// Icons bitmaps are not premultiplied
			// So Imagelist_AddIcon - premultiply alpha
			// But incorrect - it is possible that alpha will
			// be less than color and
			// ImageList_GetIcon will return overflowed colors
		{
			BYTE *bits = bm.bmBits;
			if ( !bits ) {
				bits = malloc( bm.bmWidthBytes * bm.bmHeight );
				GetBitmapBits( imi.hbmImage, bm.bmWidthBytes * bm.bmHeight, bits );
			}
			{	int iy;
				BYTE *bcbits;
				int wb = (( imi.rcImage.right - imi.rcImage.left ) * bm.bmBitsPixel >> 3 );
				bcbits = bits + ( bm.bmHeight - imi.rcImage.bottom ) * bm.bmWidthBytes + ( imi.rcImage.left * bm.bmBitsPixel >> 3 );
				for ( iy = 0; iy < imi.rcImage.bottom - imi.rcImage.top; iy++ )	{
					int x;
					// Dummy microsoft fix - alpha can be less than r,g or b
					// Looks like color channels in icons should be non-premultiplied with alpha
					// But AddIcon store it premultiplied (incorrectly cause can be Alpha==7F, but R,G or B==80
					// So i check that alpha is 0x7F and set it to 0x80
					DWORD *c = ((DWORD*) bcbits);
					for ( x = 0; x < imi.rcImage.right - imi.rcImage.left; x++ ) {
						DWORD val = *c;
						BYTE a = (BYTE)(val >> 24);
						if ( a != 0 ) {
							BYTE r = (BYTE)((val & 0xFF0000) >> 16);
							BYTE g = (BYTE)((val & 0xFF00) >> 8);
							BYTE b = (BYTE)(val & 0xFF);
							if ( a < r || a < g || a < b ) {
								a = max(max(r, g), b);
								val = a << 24 | r << 16 | g << 8 | b;
								*c = val;
						}	}
						c++;
					}
					bcbits += bm.bmWidthBytes;
			}	}
			if ( !bm.bmBits ) { 
				SetBitmapBits( imi.hbmImage, bm.bmWidthBytes * bm.bmHeight, bits );
				free( bits );
	}	}	}
	return ImageList_GetIcon( himl, i, ILD_TRANSPARENT );
}

/////////////////////////////////////////////////////////////////////////////////////////
// Service functions

static HICON ExtractIconFromPath( const TCHAR *path, int cxIcon, int cyIcon )
{
	TCHAR *comma;
	TCHAR file[ MAX_PATH ], fileFull[ MAX_PATH ];
	int n;
	HICON hIcon;

	if ( !path )
		return (HICON)NULL;

	lstrcpyn( file, path, SIZEOF( file ));
	comma = _tcsrchr( file, ',' );
	if ( !comma )
		n = 0;
	else {  
		n = _ttoi( comma+1 );
		*comma = 0;
	}
	CallService( MS_UTILS_PATHTOABSOLUTET, ( WPARAM )file, ( LPARAM )fileFull );
	hIcon = NULL;

	//SHOULD BE REPLACED WITH GOOD ENOUGH FUNCTION
	_ExtractIconEx( fileFull, n, cxIcon, cyIcon, &hIcon, LR_COLOR );
	return hIcon;
}

static SectionItem* IcoLib_AddSection(TCHAR *sectionName, BOOL create_new)
{
	SectionItem key = { sectionName, 0 };    
	int indx;

	if ( !sectionName )
		return NULL;

	if ( List_GetIndex(( SortedList* )&sectionList, &key, &indx ))
		return sectionList.items[ indx ];

	if ( create_new ) {
		SectionItem* newItem = ( SectionItem* )mir_alloc( sizeof( SectionItem ));
		newItem->name = mir_tstrdup( sectionName );
		newItem->flags = 0;
		List_Insert(( SortedList* )&sectionList, newItem, indx );
		bNeedRebuild = TRUE;
		return newItem;
	}

	return NULL;
}

static IconItem* IcoLib_FindIcon(const char* pszIconName)
{
	IconItem key = { (char*)pszIconName };

	int indx;
	if ( List_GetIndex(( SortedList* )&iconList, &key, &indx ))
		return iconList.items[ indx ];

	return NULL;
}

static IconItem* IcoLib_FindHIcon(HICON hIcon)
{
	IconItem* item = NULL;
	int indx;

	for ( indx = 0; indx < iconList.count; indx++ ) {
		if (iconList.items[ indx ]->icon == hIcon) {
			item = iconList.items[ indx ];
			break;
	}	}

	return item;
}

static void IcoLib_FreeIcon(IconItem* icon)
{
	if ( !icon) return;

	SAFE_FREE( &icon->name );
	SAFE_FREE( &icon->description );
	SAFE_FREE( &icon->default_file );
	SAFE_FREE( &icon->temp_file );
	if ( icon->icon != icon->default_icon )
		SafeDestroyIcon( &icon->icon );
	SafeDestroyIcon( &icon->default_icon );
	SafeDestroyIcon( &icon->temp_icon );

	if ( icon->icon_cache_index != -1 ) {
		int indx;

		if ( ImageList_Remove( hCacheIconList, icon->icon_cache_index ) ) {
			for ( indx = 0; indx < iconList.count; indx++ ) {
				IconItem* item = iconList.items[ indx ];

				if ( !item ) continue;

				if ( item->icon_cache_index > icon->icon_cache_index )
					item->icon_cache_index--;
				if ( item->default_icon_index > icon->icon_cache_index )
					item->default_icon_index--;
			}
			icon->icon_cache_index = -1;
		}
	}
	if ( icon->default_icon_index != -1 ) {
		int indx;

		if ( ImageList_Remove( hCacheIconList, icon->default_icon_index ) ) {
			for ( indx = 0; indx < iconList.count; indx++ ) {
				IconItem* item = iconList.items[ indx ];

				if ( !item ) continue;

				if ( item->icon_cache_index > icon->default_icon_index )
					item->icon_cache_index--;
				if ( item->default_icon_index > icon->default_icon_index )
					item->default_icon_index--;
			}
			icon->default_icon_index = -1;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// IcoLib_AddNewIcon

HANDLE IcoLib_AddNewIcon( SKINICONDESC* sid )
{
	int utf = 0, utf_path = 0;
	IconItem* item;

	if ( !sid->cbSize )
		return NULL;

	if ( sid->cbSize < SKINICONDESC_SIZE_V1 )
		return NULL;

	if ( sid->cbSize >= SKINICONDESC_SIZE ) {
		utf = sid->flags & SIDF_UNICODE ? 1 : 0;
		utf_path = sid->flags & SIDF_PATH_UNICODE ? 1 : 0;
	}

	EnterCriticalSection( &csIconList );

	item = IcoLib_FindIcon( sid->pszName );
	if ( !item ) {
		item = ( IconItem* )mir_alloc( sizeof( IconItem ));
		item->name = sid->pszName;
		List_InsertPtr(( SortedList* )&iconList, item );
	}
	else IcoLib_FreeIcon( item );

	ZeroMemory( item, sizeof( *item ));
	item->name = mir_strdup( sid->pszName );
	if ( utf ) {
		#ifdef _UNICODE
			item->description = mir_tstrdup( sid->ptszDescription );
			item->section = IcoLib_AddSection( sid->pwszSection, TRUE );
		#else
			char* pszSection = sid->pwszSection ? u2a( sid->pwszSection ) : NULL;
			item->description = u2a( sid->pwszDescription );
			item->section = IcoLib_AddSection( pszSection, TRUE );
			SAFE_FREE( &pszSection );
		#endif
	}
	else {
		#ifdef _UNICODE
			WCHAR* pwszSection = sid->pszSection ? a2u( sid->pszSection ) : NULL;

			item->description = a2u( sid->pszDescription );
			item->section = IcoLib_AddSection( pwszSection, TRUE );
			SAFE_FREE( &pwszSection );
		#else
			item->description = mir_strdup( sid->pszDescription );
			item->section = IcoLib_AddSection( sid->pszSection, TRUE );
		#endif
	}
	if ( item->section )
		item->orderID = ++item->section->maxOrder;
	else
		item->orderID = 0;

	if ( sid->pszDefaultFile ) {
		if ( utf_path ) {
			#ifdef _UNICODE
				WCHAR fileFull[ MAX_PATH ];

				CallService( MS_UTILS_PATHTOABSOLUTEW, ( WPARAM )sid->pwszDefaultFile, ( LPARAM )fileFull );
				item->default_file = mir_wstrdup( fileFull );
			#else
				char* file = u2a( sid->pwszDefaultFile );
				char fileFull[ MAX_PATH ];

				CallService( MS_UTILS_PATHTOABSOLUTE, ( WPARAM )file, ( LPARAM )fileFull );
				SAFE_FREE( &file );
				item->default_file = mir_strdup( fileFull );
			#endif
		}
		else {
			#ifdef _UNICODE
				WCHAR *file = a2u( sid->pszDefaultFile );
				WCHAR fileFull[ MAX_PATH ];

				CallService( MS_UTILS_PATHTOABSOLUTEW, ( WPARAM )file, ( LPARAM )fileFull );
				SAFE_FREE( &file );
				item->default_file = mir_wstrdup( fileFull );
			#else
				char fileFull[ MAX_PATH ];

				CallService( MS_UTILS_PATHTOABSOLUTE, ( WPARAM )sid->pszDefaultFile, ( LPARAM )fileFull );
				item->default_file = mir_strdup( fileFull );
			#endif
	}	}
	item->default_indx = sid->iDefaultIndex;
	if ( sid->cbSize >= SKINICONDESC_SIZE_V2 && sid->hDefaultIcon )
		item->default_icon = DuplicateIcon( NULL, sid->hDefaultIcon );

	if ( sid->cbSize >= SKINICONDESC_SIZE_V3 ) {
		item->cx = sid->cx;
		item->cy = sid->cy;
	}
	else {
		item->cx = GetSystemMetrics( SM_CXSMICON );
		item->cy = GetSystemMetrics( SM_CYSMICON );
	}

	if ( item->cx == GetSystemMetrics( SM_CXSMICON ) && item->cy == GetSystemMetrics( SM_CYSMICON ) && item->default_icon ) {
		item->default_icon_index = ImageList_AddIcon( hCacheIconList, item->default_icon );
		if ( item->default_icon_index != -1 )
			SafeDestroyIcon( &item->default_icon );
	}
	else item->default_icon_index = -1;

	item->icon_cache_index = -1;

	if ( sid->cbSize >= SKINICONDESC_SIZE && item->section )
		item->section->flags = sid->flags & SIDF_SORTED;

	LeaveCriticalSection( &csIconList );
	return item;
}

/////////////////////////////////////////////////////////////////////////////////////////
// IcoLib_RemoveIcon

static int IcoLib_RemoveIcon( WPARAM wParam, LPARAM lParam )
{
	if ( lParam ) {
		int indx;
		EnterCriticalSection( &csIconList );

		if ( List_GetIndex(( SortedList* )&iconList, ( void* )lParam, &indx )) {
			IcoLib_FreeIcon( iconList.items[ indx ] );
			List_Remove(( SortedList* )&iconList, indx );
		}
		else indx = -1;

		LeaveCriticalSection( &csIconList );
		return ( indx == -1 ) ? 1 : 0;
	}
	return 1; // Failed
}

HICON IconItem_GetDefaultIcon( IconItem* item )
{
	HICON hIcon = NULL;

	if (item->default_icon) {
		hIcon = item->default_icon;
	}

	if ( !hIcon && item->default_icon_index != -1 )
		hIcon = ImageList_GetIcon_Fixed( hCacheIconList, item->default_icon_index, ILD_NORMAL );

	if ( !hIcon && item->default_file )
		_ExtractIconEx( item->default_file, item->default_indx, item->cx, item->cy, &hIcon, LR_COLOR );

	return hIcon;
}


/////////////////////////////////////////////////////////////////////////////////////////
// IcoLib_GetIcon

HICON IconItem_GetIcon( IconItem* item )
{
	DBVARIANT dbv = {0};
	HICON hIcon = NULL;

	if ( item->icon )
		return item->icon;

	if ( item->icon_cache_valid )
		hIcon = ImageList_GetIcon_Fixed( hCacheIconList, item->icon_cache_index, ILD_NORMAL );

	if ( !hIcon && !DBGetContactSettingTString( NULL, "SkinIcons", item->name, &dbv )) {
		hIcon = ExtractIconFromPath( dbv.ptszVal, item->cx, item->cy );
		DBFreeVariant( &dbv );
	}

	if ( !hIcon ) {
		hIcon = IconItem_GetDefaultIcon(item);

		if ( !hIcon )
			return hIconBlank;
	}

	if ( !item->icon_cache_valid && item->cx == GetSystemMetrics(SM_CXSMICON) && item->cy == GetSystemMetrics(SM_CYSMICON) ) {
		item->icon_cache_index = ImageList_ReplaceIcon( hCacheIconList, item->icon_cache_index, hIcon );

		if ( item->icon_cache_index != -1 )
			item->icon_cache_valid = TRUE;
	}

	item->icon = hIcon;
	return hIcon;
}

/////////////////////////////////////////////////////////////////////////////////////////
// IcoLib_GetIcon

HICON IconItem_GetIcon_Preview( IconItem* item )
{
	DBVARIANT dbv = {0};
	HICON hIcon = NULL;

	if ( !item->temp_reset ) {
		hIcon = CopyIcon(IconItem_GetIcon(item));
	}
	else {
		hIcon = IconItem_GetDefaultIcon(item);

		if (hIcon == item->default_icon) {
			hIcon = CopyIcon(hIcon);
		}

		if ( !hIcon )
			return CopyIcon(hIconBlank);
	}
	return hIcon;
}


/////////////////////////////////////////////////////////////////////////////////////////
// IcoLib_GetIcon
// lParam: pszIconName
// wParam: PLOADIMAGEPARAM or NULL.
// if wParam == NULL, default is used:
//     uType = IMAGE_ICON
//     cx/cyDesired = GetSystemMetrics(SM_CX/CYSMICON)
//     fuLoad = 0

HICON IcoLib_GetIcon( const char* pszIconName )
{
	IconItem* item;
	HICON result = NULL;

	if ( !pszIconName )
		return hIconBlank;

	EnterCriticalSection( &csIconList );

	item = IcoLib_FindIcon( pszIconName );
	if ( item ) {
		result = IconItem_GetIcon( item );
		item->ref_count++;
	}
	LeaveCriticalSection( &csIconList );
	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////
// IcoLib_GetIconByHandle
// lParam: icolib item handle
// wParam: 0

HICON IcoLib_GetIconByHandle( HANDLE hItem )
{
	HICON result;
	IconItem* pi = ( IconItem* )hItem;

	EnterCriticalSection( &csIconList );

	if ( List_IndexOf(( SortedList* )&iconList, pi ) != -1 ) {
		result = IconItem_GetIcon( pi );
		pi->ref_count++;
	}
	else result = hIconBlank;

	LeaveCriticalSection( &csIconList );
	return result;			
}

/////////////////////////////////////////////////////////////////////////////////////////
// IcoLib_IsManaged
// lParam: NULL
// wParam: HICON

HANDLE IcoLib_IsManaged( HICON hIcon )
{
	IconItem* item;

	EnterCriticalSection( &csIconList );

	item = IcoLib_FindHIcon( hIcon );
	if ( item && item->ref_count ) {
		LeaveCriticalSection( &csIconList );
		return item;
	}

	LeaveCriticalSection( &csIconList );
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////
// IcoLib_AddRef
// lParam: NULL
// wParam: HICON

static int IcoLib_AddRef( WPARAM wParam, LPARAM lParam )
{
	IconItem *item = NULL;

	EnterCriticalSection( &csIconList );

	item = IcoLib_FindHIcon(( HICON )wParam);

	if ( item && item->ref_count ) {
		item->ref_count++;
		LeaveCriticalSection( &csIconList );
		return 0;
	}

	LeaveCriticalSection( &csIconList );
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////
// IcoLib_ReleaseIcon
// lParam: pszIconName or NULL
// wParam: HICON or NULL

int IcoLib_ReleaseIcon( HICON hIcon, char* szIconName )
{
	IconItem *item = NULL;

	EnterCriticalSection(&csIconList);

	if ( szIconName )
		item = IcoLib_FindIcon( szIconName );

	if ( !item && hIcon ) // find by HICON
		item = IcoLib_FindHIcon( hIcon );

	if ( item && item->ref_count ) {
		item->ref_count--;
		if ( !iconEventActive && !item->ref_count && item->icon != item->default_icon )
			SafeDestroyIcon( &item->icon );
		LeaveCriticalSection( &csIconList );
		return 0;
	}

	LeaveCriticalSection( &csIconList );
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////
// IcoLib GUI service routines

static void LoadSectionIcons(TCHAR *filename, SectionItem* sectionActive)
{
	TCHAR path[ MAX_PATH ];
	int suffIndx;
	HICON hIcon;
	int indx;

	mir_sntprintf( path, SIZEOF(path), _T("%s,"), filename );
	suffIndx = lstrlen( path );

	EnterCriticalSection( &csIconList );

	for ( indx = 0; indx < iconList.count; indx++ ) {
		IconItem *item = iconList.items[ indx ];

		if ( item->default_file && item->section == sectionActive ) {
			_itot( item->default_indx, path + suffIndx, 10 );
			hIcon = ExtractIconFromPath( path, item->cx, item->cy );
			if ( hIcon ) {
				SAFE_FREE( &item->temp_file );
				SafeDestroyIcon( &item->temp_icon );

				item->temp_file = mir_tstrdup( path );
				item->temp_icon = hIcon;
				item->temp_reset = FALSE;
	}	}	}

	LeaveCriticalSection( &csIconList );
}

void LoadSubIcons(HWND htv, TCHAR *filename, HTREEITEM hItem)
{
	TVITEM tvi;
	TreeItem *treeItem;
	SectionItem* sectionActive;

	tvi.mask = TVIF_HANDLE | TVIF_PARAM;
	tvi.hItem = hItem;

	TreeView_GetItem( htv, &tvi );
	treeItem = (TreeItem *)tvi.lParam;
	sectionActive = sectionList.items[ SECTIONPARAM_INDEX(treeItem->value) ];

	tvi.hItem = TreeView_GetChild( htv, tvi.hItem );
	while ( tvi.hItem ) {
		LoadSubIcons( htv, filename, tvi.hItem );
		tvi.hItem = TreeView_GetNextSibling( htv, tvi.hItem );
	}

	if ( SECTIONPARAM_FLAGS(treeItem->value) & SECTIONPARAM_HAVEPAGE )
		LoadSectionIcons( filename, sectionActive );
}

static void UndoChanges( int iconIndx, int cmd )
{
	IconItem *item = iconList.items[ iconIndx ];

	SAFE_FREE( &item->temp_file );
	SafeDestroyIcon( &item->temp_icon );

	if ( cmd == ID_RESET )
		item->temp_reset = TRUE;
}

void UndoSubItemChanges( HWND htv, HTREEITEM hItem, int cmd )
{
	TVITEM tvi = {0};
	TreeItem *treeItem;
	int indx;

	tvi.mask = TVIF_HANDLE | TVIF_PARAM;
	tvi.hItem = hItem;
	TreeView_GetItem( htv, &tvi );
	treeItem = (TreeItem *)tvi.lParam;

	if ( SECTIONPARAM_FLAGS( treeItem->value ) & SECTIONPARAM_HAVEPAGE ) {
		EnterCriticalSection( &csIconList );

		for ( indx = 0; indx < iconList.count; indx++ )
			if ( iconList.items[ indx ]->section == sectionList.items[ SECTIONPARAM_INDEX(treeItem->value) ])
				UndoChanges( indx, cmd );

		LeaveCriticalSection( &csIconList );
	}

	tvi.hItem = TreeView_GetChild( htv, tvi.hItem );
	while ( tvi.hItem ) {
		UndoSubItemChanges( htv, tvi.hItem, cmd );
		tvi.hItem = TreeView_GetNextSibling( htv, tvi.hItem );
}	}

static void OpenIconsPage()
{
	CallService( MS_UTILS_OPENURL, 1, (LPARAM)"http://addons.miranda-im.org/index.php?action=display&id=35" );
}

static int OpenPopupMenu(HWND hwndDlg)
{
	HMENU hMenu, hPopup;
	POINT pt;
	int cmd;

	GetCursorPos(&pt);
	hMenu = LoadMenu( GetModuleHandle(NULL), MAKEINTRESOURCE( IDR_ICOLIB_CONTEXT ));
	hPopup = GetSubMenu( hMenu, 0 );
	CallService( MS_LANGPACK_TRANSLATEMENU, ( WPARAM )hPopup, 0 );
	cmd = TrackPopupMenu( hPopup, TPM_RIGHTBUTTON|TPM_RETURNCMD, pt.x,  pt.y, 0, hwndDlg, NULL );
	DestroyMenu( hMenu );
	return cmd;
}

static TCHAR* OpenFileDlg( HWND hParent, const TCHAR* szFile, BOOL bAll )
{
	OPENFILENAME ofn = {0};
	TCHAR filter[512],*pfilter,file[MAX_PATH*2];

	ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
	ofn.hwndOwner = hParent;

	lstrcpy(filter,TranslateT("Icon Sets"));
	if (bAll)
		lstrcat(filter,_T(" (*.dll;*.icl;*.exe;*.ico)"));
	else
		lstrcat(filter,_T(" (*.dll)"));

	pfilter=filter+lstrlen(filter)+1;
	if (bAll)
		lstrcpy(pfilter,_T("*.DLL;*.ICL;*.EXE;*.ICO"));
	else
		lstrcpy(pfilter,_T("*.DLL"));

	pfilter += lstrlen(pfilter) + 1;
	lstrcpy(pfilter, TranslateT("All Files"));
	lstrcat(pfilter,_T(" (*)"));
	pfilter += lstrlen(pfilter) + 1;
	lstrcpy(pfilter,_T("*"));
	pfilter += lstrlen(pfilter) + 1;
	*pfilter='\0';

	ofn.lpstrFilter = filter;
	ofn.lpstrDefExt = _T("dll");
	lstrcpyn(file, szFile, SIZEOF(file));
	ofn.lpstrFile = file;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.nMaxFile = MAX_PATH*2;

	if (!GetOpenFileName(&ofn)) 
		return NULL;

	return mir_tstrdup(file);
}

//
//  User interface
//

#define DM_REBUILDICONSPREVIEW   (WM_USER+10)
#define DM_CHANGEICON            (WM_USER+11)
#define DM_CHANGESPECIFICICON    (WM_USER+12)
#define DM_UPDATEICONSPREVIEW    (WM_USER+13)
#define DM_REBUILD_CTREE         (WM_USER+14)

BOOL CALLBACK DlgProcIconImport(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

void DoOptionsChanged(HWND hwndDlg)
{
	SendMessage(hwndDlg, DM_UPDATEICONSPREVIEW, 0, 0);
	SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
}

void DoIconsChanged(HWND hwndDlg)
{
	int indx;

	EnterCriticalSection(&csIconList); // Invalidate icons cache
	for (indx = 0; indx < iconList.count; indx++) {
		iconList.items[indx]->icon_cache_valid = FALSE;
	}
	LeaveCriticalSection(&csIconList);

	SendMessage(hwndDlg, DM_UPDATEICONSPREVIEW, 0, 0);

	iconEventActive = 1; // Disable icon destroying - performance boost
	NotifyEventHooks(hIconsChangedEvent, 0, 0);
	NotifyEventHooks(hIcons2ChangedEvent, 0, 0);
	iconEventActive = 0;

	EnterCriticalSection(&csIconList); // Destroy unused icons
	for (indx = 0; indx < iconList.count; indx++) {
		IconItem *item = iconList.items[indx];
		if (!item->ref_count && item->icon != item->default_icon)
			SafeDestroyIcon(&item->icon);
	}
	LeaveCriticalSection(&csIconList);
}

static HTREEITEM FindNamedTreeItemAt(HWND hwndTree, HTREEITEM hItem, const TCHAR *name)
{
	TVITEM tvi = {0};
	TCHAR str[MAX_PATH];

	if (hItem)
		tvi.hItem = TreeView_GetChild(hwndTree, hItem);
	else
		tvi.hItem = TreeView_GetRoot(hwndTree);

	if (!name)
		return tvi.hItem;

	tvi.mask = TVIF_TEXT;
	tvi.pszText = str;
	tvi.cchTextMax = MAX_PATH;

	while (tvi.hItem)
	{
		TreeView_GetItem(hwndTree, &tvi);

		if (!lstrcmp(tvi.pszText, name))
			return tvi.hItem;

		tvi.hItem = TreeView_GetNextSibling(hwndTree, tvi.hItem);
	}
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////
// icon import dialog's window procedure

static int IconDlg_Resize(HWND hwndDlg,LPARAM lParam, UTILRESIZECONTROL *urc)
{
	switch (urc->wId) {
	case IDC_ICONSET:
		return RD_ANCHORX_WIDTH | RD_ANCHORY_TOP;

	case IDC_BROWSE:
		return RD_ANCHORX_RIGHT | RD_ANCHORY_TOP;

	case IDC_PREVIEW:
		return RD_ANCHORX_WIDTH | RD_ANCHORY_HEIGHT;      

	case IDC_GETMORE:
		return RD_ANCHORX_CENTRE | RD_ANCHORY_BOTTOM;
	}
	return RD_ANCHORX_LEFT | RD_ANCHORY_TOP; // default
}

BOOL CALLBACK DlgProcIconImport(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND hwndParent,hwndDragOver;
	static int dragging;
	static int dragItem,dropHiLite;
	static HWND hPreview = NULL;

	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		hwndParent = (HWND)lParam;
		hPreview = GetDlgItem(hwndDlg, IDC_PREVIEW);
		dragging = dragItem = 0;
		ListView_SetImageList(hPreview, ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),ILC_COLOR32|ILC_MASK,0,100), LVSIL_NORMAL);
		ListView_SetIconSpacing(hPreview, 56, 67);
		{  
			RECT rcThis, rcParent;
			int cxScreen = GetSystemMetrics(SM_CXSCREEN);

			GetWindowRect(hwndDlg,&rcThis);
			GetWindowRect(hwndParent,&rcParent);
			OffsetRect(&rcThis,rcParent.right-rcThis.left,0);
			OffsetRect(&rcThis,0,rcParent.top-rcThis.top);
			GetWindowRect(GetParent(hwndParent),&rcParent);
			if (rcThis.right > cxScreen) {
				OffsetRect(&rcParent,cxScreen-rcThis.right,0);
				OffsetRect(&rcThis,cxScreen-rcThis.right,0);
				MoveWindow(GetParent(hwndParent),rcParent.left,rcParent.top,rcParent.right-rcParent.left,rcParent.bottom-rcParent.top,TRUE);
			}
			MoveWindow(hwndDlg,rcThis.left,rcThis.top,rcThis.right-rcThis.left,rcThis.bottom-rcThis.top,FALSE);
			GetClientRect(hwndDlg, &rcThis);
			SendMessage(hwndDlg, WM_SIZE, 0, MAKELPARAM(rcThis.right-rcThis.left, rcThis.bottom-rcThis.top));
		}
		{  
			HRESULT (STDAPICALLTYPE *MySHAutoComplete)(HWND,DWORD);

			MySHAutoComplete = (HRESULT (STDAPICALLTYPE*)(HWND,DWORD))GetProcAddress(GetModuleHandleA("shlwapi"),"SHAutoComplete");
			if (MySHAutoComplete) MySHAutoComplete(GetDlgItem(hwndDlg,IDC_ICONSET), 1);
		}
		SetDlgItemText(hwndDlg,IDC_ICONSET,_T("icons.dll"));
		return TRUE;

	case DM_REBUILDICONSPREVIEW:
		{  
			LVITEM lvi;
			TCHAR filename[MAX_PATH],caption[64];
			HIMAGELIST hIml;
			int count,i;
			HICON hIcon;

			MySetCursor(IDC_WAIT);
			ListView_DeleteAllItems(hPreview);
			hIml = ListView_GetImageList(hPreview, LVSIL_NORMAL);
			ImageList_RemoveAll(hIml);
			GetDlgItemText(hwndDlg, IDC_ICONSET, filename, SIZEOF(filename));
			{  
				RECT rcPreview,rcGroup;

				GetWindowRect(hPreview, &rcPreview);
				GetWindowRect(GetDlgItem(hwndDlg,IDC_IMPORTMULTI),&rcGroup);
				//SetWindowPos(hPreview,0,0,0,rcPreview.right-rcPreview.left,rcGroup.bottom-rcPreview.top,SWP_NOZORDER|SWP_NOMOVE);
			}

			if (_taccess(filename,0) != 0) {
				MySetCursor(IDC_ARROW);
				break;
			}

			lvi.mask = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
			lvi.iSubItem = 0;
			lvi.iItem = 0;
			count = (int)_ExtractIconEx( filename, -1, 16,16, NULL, LR_DEFAULTCOLOR );
			for (i = 0; i < count; lvi.iItem++, i++) {
				mir_sntprintf(caption, SIZEOF(caption), _T("%d"), i+1);
				lvi.pszText = caption;
				//hIcon = ExtractIcon(GetModuleHandle(NULL), filename, i);
				_ExtractIconEx( filename, i, 16,16, &hIcon, LR_DEFAULTCOLOR );
				lvi.iImage = ImageList_AddIcon(hIml, hIcon);
				DestroyIcon(hIcon);
				lvi.lParam = i;
				ListView_InsertItem(hPreview, &lvi);
			}
			MySetCursor(IDC_ARROW);
		}
		break;

	case WM_COMMAND:
		switch( LOWORD( wParam )) {
		case IDC_BROWSE:
			{ 
				TCHAR str[MAX_PATH];
				TCHAR *file;

				GetDlgItemText(hwndDlg,IDC_ICONSET,str,SIZEOF(str));
				if (!(file = OpenFileDlg(GetParent(hwndDlg), str, TRUE))) break;
				SetDlgItemText(hwndDlg,IDC_ICONSET,file);
				SAFE_FREE(&file);
			}
			break;

		case IDC_GETMORE:
			OpenIconsPage();
			break;

		case IDC_ICONSET:
			if (HIWORD(wParam) == EN_CHANGE)
				SendMessage(hwndDlg, DM_REBUILDICONSPREVIEW, 0, 0);
			break;
		}
		break;

	case WM_MOUSEMOVE:
		if (dragging) {
			LVHITTESTINFO lvhti;
			int onItem=0;
			HWND hwndOver;
			RECT rc;
			POINT ptDrag;
			HWND hPPreview = GetDlgItem(hwndParent, IDC_PREVIEW);

			lvhti.pt.x = (short)LOWORD(lParam); lvhti.pt.y = (short)HIWORD(lParam);
			ClientToScreen(hwndDlg, &lvhti.pt);
			hwndOver = WindowFromPoint(lvhti.pt);
			GetWindowRect(hwndOver, &rc);
			ptDrag.x = lvhti.pt.x - rc.left; ptDrag.y = lvhti.pt.y - rc.top;
			if (hwndOver != hwndDragOver) {
				ImageList_DragLeave(hwndDragOver);
				hwndDragOver = hwndOver;
				ImageList_DragEnter(hwndDragOver, ptDrag.x, ptDrag.y);
			}

			ImageList_DragMove(ptDrag.x, ptDrag.y);
			if (hwndOver == hPPreview) {
				ScreenToClient(hPPreview, &lvhti.pt);

				if (ListView_HitTest(hPPreview, &lvhti) != -1) {
					if (lvhti.iItem != dropHiLite) {
						ImageList_DragLeave(hwndDragOver);
						if (dropHiLite != -1)
							ListView_SetItemState(hPPreview, dropHiLite, 0, LVIS_DROPHILITED);
						dropHiLite = lvhti.iItem;
						ListView_SetItemState(hPPreview, dropHiLite, LVIS_DROPHILITED, LVIS_DROPHILITED);
						UpdateWindow(hPPreview);
						ImageList_DragEnter(hwndDragOver, ptDrag.x, ptDrag.y);
					}
					onItem = 1;
			}	}

			if (!onItem && dropHiLite != -1) {
				ImageList_DragLeave(hwndDragOver);
				ListView_SetItemState(hPPreview, dropHiLite, 0, LVIS_DROPHILITED);
				UpdateWindow(hPPreview);
				ImageList_DragEnter(hwndDragOver, ptDrag.x, ptDrag.y);
				dropHiLite = -1;
			}
			MySetCursor(onItem ? IDC_ARROW : IDC_NO);
		}
		break;

	case WM_LBUTTONUP:
		if (dragging) {
			ReleaseCapture();
			ImageList_EndDrag();
			dragging = 0;
			if (dropHiLite != -1) {
				TCHAR path[MAX_PATH],fullPath[MAX_PATH],filename[MAX_PATH];
				LVITEM lvi;

				GetDlgItemText(hwndDlg, IDC_ICONSET, fullPath, SIZEOF(fullPath));
				CallService(MS_UTILS_PATHTORELATIVET, (WPARAM)fullPath, (LPARAM)filename);
				lvi.mask=LVIF_PARAM;
				lvi.iItem = dragItem; lvi.iSubItem = 0;
				ListView_GetItem(hPreview, &lvi);
				mir_sntprintf(path, MAX_PATH, _T("%s,%d"), filename, (int)lvi.lParam);
				SendMessage(hwndParent, DM_CHANGEICON, dropHiLite, (LPARAM)path);
				ListView_SetItemState(GetDlgItem(hwndParent, IDC_PREVIEW), dropHiLite, 0, LVIS_DROPHILITED);
		}	}
		break;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->idFrom) {
		case IDC_PREVIEW:
			switch (((LPNMHDR)lParam)->code) {
			case LVN_BEGINDRAG:
				SetCapture(hwndDlg);
				dragging = 1;
				dragItem = ((LPNMLISTVIEW)lParam)->iItem;
				dropHiLite = -1;
				ImageList_BeginDrag(ListView_GetImageList(hPreview, LVSIL_NORMAL), dragItem, GetSystemMetrics(SM_CXICON)/2, GetSystemMetrics(SM_CYICON)/2);
				{  
					POINT pt;
					RECT rc;

					GetCursorPos(&pt);
					GetWindowRect(hwndDlg, &rc);
					ImageList_DragEnter(hwndDlg, pt.x - rc.left, pt.y - rc.top);
					hwndDragOver = hwndDlg;
				}
				break;
			}
			break;
		}
		break;

	case WM_CLOSE:
		DestroyWindow(hwndDlg);
		EnableWindow(GetDlgItem(hwndParent,IDC_IMPORT),TRUE);
		break;

	case WM_SIZE:
		{ // make the dlg resizeable
			UTILRESIZEDIALOG urd = {0};

			if (IsIconic(hwndDlg)) break;
			urd.cbSize = sizeof(urd);
			urd.hInstance = GetModuleHandle(NULL);
			urd.hwndDlg = hwndDlg;
			urd.lParam = 0; // user-defined
			urd.lpTemplate = MAKEINTRESOURCEA(IDD_ICOLIB_IMPORT);
			urd.pfnResizer = IconDlg_Resize;
			CallService(MS_UTILS_RESIZEDIALOG, 0, (LPARAM)&urd);
			break;
	}	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// IcoLib options window procedure

static int CALLBACK DoSortIconsFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{	return lstrcmpi(TranslateTS(iconList.items[lParam1]->description), TranslateTS(iconList.items[lParam2]->description));
}

static int CALLBACK DoSortIconsFuncByOrder(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{	return iconList.items[lParam1]->orderID - iconList.items[lParam2]->orderID;
}

static void SaveCollapseState( HWND hwndTree )
{
	HTREEITEM hti;
	TVITEM tvi;

	hti = TreeView_GetRoot( hwndTree );
	while( hti != NULL ) {
		HTREEITEM ht;
		TreeItem *treeItem;

		tvi.mask = TVIF_STATE | TVIF_HANDLE | TVIF_CHILDREN | TVIF_PARAM;
		tvi.hItem = hti;
		tvi.stateMask = (DWORD)-1;
		TreeView_GetItem( hwndTree, &tvi );

		if( tvi.cChildren > 0 ) {
			treeItem = (TreeItem *)tvi.lParam;
			if ( tvi.state & TVIS_EXPANDED )
				DBWriteContactSettingByte(NULL, "SkinIconsUI", treeItem->paramName, TVIS_EXPANDED );
			else
				DBWriteContactSettingByte(NULL, "SkinIconsUI", treeItem->paramName, 0 );
		}

		ht = TreeView_GetChild( hwndTree, hti );
		if( ht == NULL ) {
			ht = TreeView_GetNextSibling( hwndTree, hti );
			while( ht == NULL ) {
				hti = TreeView_GetParent( hwndTree, hti );
				if( hti == NULL ) break;
				ht = TreeView_GetNextSibling( hwndTree, hti );
		}	}

		hti = ht;
}	}

BOOL CALLBACK DlgProcIcoLibOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct IcoLibOptsData *dat;
	static HTREEITEM prevItem = 0;
	static HWND hPreview = NULL;

	dat = (struct IcoLibOptsData*)GetWindowLong(hwndDlg, GWL_USERDATA);
	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		hPreview = GetDlgItem(hwndDlg, IDC_PREVIEW);
		dat = (struct IcoLibOptsData*)mir_alloc(sizeof(struct IcoLibOptsData));
		dat->hwndIndex = NULL;
		SetWindowLong(hwndDlg, GWL_USERDATA, (LONG)dat);
		//
		//  Reset temporary data & upload sections list
		//
		EnterCriticalSection(&csIconList);
		{
			int indx;
			for (indx = 0; indx < iconList.count; indx++) {
				iconList.items[indx]->temp_file = NULL;
				iconList.items[indx]->temp_icon = NULL;
				iconList.items[indx]->temp_reset = FALSE;
			}
			bNeedRebuild = FALSE;
		}
		LeaveCriticalSection(&csIconList);
		//
		//  Setup preview listview
		//
		ListView_SetImageList(hPreview, ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),ILC_COLOR32|ILC_MASK,0,30), LVSIL_NORMAL);
		ListView_SetIconSpacing(hPreview, 56, 67);

		SendMessage(hwndDlg, DM_REBUILD_CTREE, 0, 0);
		return TRUE;

	case DM_REBUILD_CTREE:
		{
			HWND hwndTree = GetDlgItem(hwndDlg, IDC_CATEGORYLIST);
			int indx;
			TCHAR itemName[1024];
			HTREEITEM hSection;

			TreeView_SelectItem(hwndTree, NULL);
			TreeView_DeleteAllItems(hwndTree);

			for (indx = 0; indx < sectionList.count; indx++) {
				TCHAR* sectionName;
				int sectionLevel = 0;

				hSection = NULL;
				lstrcpy(itemName, sectionList.items[indx]->name);
				sectionName = itemName;

				while (sectionName) {
					// allow multi-level tree
					TCHAR* pItemName = sectionName;
					HTREEITEM hItem;

					if (sectionName = _tcschr(sectionName, '/')) {
						// one level deeper
						*sectionName = 0;
					}

					pItemName = TranslateTS( pItemName );

					hItem = FindNamedTreeItemAt(hwndTree, hSection, pItemName);
					if (!sectionName || !hItem) {
						if (!hItem) {
							TVINSERTSTRUCT tvis = {0};
							TreeItem *treeItem = (TreeItem *)mir_alloc(sizeof(TreeItem));
							treeItem->value = SECTIONPARAM_MAKE( indx, sectionLevel, sectionName?0:SECTIONPARAM_HAVEPAGE );
							treeItem->paramName = mir_t2a(itemName);

							tvis.hParent = hSection;
							tvis.hInsertAfter = TVI_SORT; //TVI_LAST;
							tvis.item.mask = TVIF_TEXT|TVIF_PARAM|TVIF_STATE;
							tvis.item.pszText = pItemName;
							tvis.item.lParam = (LPARAM) treeItem;

							tvis.item.state = tvis.item.stateMask = DBGetContactSettingByte(NULL, "SkinIconsUI", treeItem->paramName, TVIS_EXPANDED );
							hItem = TreeView_InsertItem(hwndTree, &tvis);
						}
						else {
							TVITEM tvi = {0};
							TreeItem *treeItem;
							tvi.hItem = hItem;
							tvi.mask = TVIF_HANDLE | TVIF_PARAM;
							TreeView_GetItem( hwndTree, &tvi );
							treeItem = (TreeItem *)tvi.lParam;
							treeItem->value = SECTIONPARAM_MAKE( indx, sectionLevel, SECTIONPARAM_HAVEPAGE );
					}	}

					if (sectionName) {
						*sectionName = '/';
						sectionName++;
					}
					sectionLevel++;


					hSection = hItem;
			}	}

			ShowWindow(hwndTree, SW_SHOW);

			TreeView_SelectItem(hwndTree, FindNamedTreeItemAt(hwndTree, 0, NULL));
		}
		break;

	//  Rebuild preview to new section
	case DM_REBUILDICONSPREVIEW:
		{  
			LVITEM lvi = {0};
			HIMAGELIST hIml;
			HICON hIcon;
			SectionItem* sectionActive = ( SectionItem* )lParam;
			int indx;

			EnableWindow(hPreview, sectionActive != NULL );

			ListView_DeleteAllItems(hPreview);
			hIml = ListView_GetImageList(hPreview, LVSIL_NORMAL);
			ImageList_RemoveAll(hIml);

			if (sectionActive == NULL)
				break;

			lvi.mask = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;

			EnterCriticalSection(&csIconList);

			for (indx = 0; indx < iconList.count; indx++) {
				IconItem *item = iconList.items[indx];

				if (item->section == sectionActive) {
					lvi.pszText = TranslateTS(item->description);
					hIcon = item->temp_icon;
					if ( !hIcon ) {
						if ( !item->icon ) item->temp = 1;
						hIcon = IconItem_GetIcon_Preview( item );
					}
					lvi.iImage = ImageList_AddIcon(hIml, hIcon);
					lvi.lParam = indx;
					ListView_InsertItem(hPreview, &lvi);
					if (hIcon != item->temp_icon) DestroyIcon(hIcon);
			}	}

			LeaveCriticalSection(&csIconList);

			if ( sectionActive->flags & SIDF_SORTED )
				ListView_SortItems(hPreview, DoSortIconsFunc, 0);
			else
				ListView_SortItems(hPreview, DoSortIconsFuncByOrder, 0);
		}
		break;

	// Refresh preview to new section
	case DM_UPDATEICONSPREVIEW:
		{  
			LVITEM lvi = {0};
			HICON hIcon;
			int indx, count;
			HIMAGELIST hIml = ListView_GetImageList(hPreview, LVSIL_NORMAL);

			lvi.mask = LVIF_IMAGE|LVIF_PARAM;
			count = ListView_GetItemCount(hPreview);

			for (indx = 0; indx < count; indx++) {
				lvi.iItem = indx;
				ListView_GetItem(hPreview, &lvi);
				EnterCriticalSection(&csIconList);
				hIcon = iconList.items[lvi.lParam]->temp_icon;
				if (!hIcon)
					hIcon = IconItem_GetIcon_Preview( iconList.items[lvi.lParam] );
				LeaveCriticalSection(&csIconList);

				if (hIcon)
					ImageList_ReplaceIcon(hIml, lvi.iImage, hIcon);
				if (hIcon != iconList.items[lvi.lParam]->temp_icon) DestroyIcon(hIcon);
			}
			ListView_RedrawItems(hPreview, 0, count);
		}
		break;

	// Temporary change icon - only inside options dialog
	case DM_CHANGEICON:
		{
			TCHAR *path=(TCHAR*)lParam;
			LVITEM lvi = {0};
			IconItem *item;

			lvi.mask = LVIF_PARAM;
			lvi.iItem = wParam;
			ListView_GetItem( hPreview, &lvi );

			EnterCriticalSection( &csIconList );
			item = iconList.items[ lvi.lParam ];

			SAFE_FREE( &item->temp_file );
			SafeDestroyIcon( &item->temp_icon );
			item->temp_file = mir_tstrdup( path );
			item->temp_icon = ( HICON )ExtractIconFromPath( path, item->cx, item->cy );
			item->temp_reset = FALSE;

			LeaveCriticalSection( &csIconList );
			DoOptionsChanged( hwndDlg );
		}
		break;

	case WM_COMMAND:
		if ( LOWORD(wParam) == IDC_IMPORT ) {
			dat->hwndIndex = CreateDialogParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ICOLIB_IMPORT), GetParent(hwndDlg), DlgProcIconImport, (LPARAM)hwndDlg);
			EnableWindow((HWND)lParam, FALSE);
		}
		else if ( LOWORD(wParam) == IDC_GETMORE ) {
			OpenIconsPage();
			break;
		}
		else if (LOWORD(wParam) == IDC_LOADICONS ) {
			TCHAR filetmp[1] = {0};
			TCHAR *file;

			if ( file = OpenFileDlg( hwndDlg, filetmp, FALSE )) {
				HWND htv = GetDlgItem( hwndDlg, IDC_CATEGORYLIST );
				TCHAR filename[ MAX_PATH ];

				CallService( MS_UTILS_PATHTORELATIVET, ( WPARAM )file, ( LPARAM )filename );
				SAFE_FREE( &file );

				MySetCursor( IDC_WAIT );
				LoadSubIcons( htv, filename, TreeView_GetSelection( htv ));
				MySetCursor( IDC_ARROW );

				DoOptionsChanged( hwndDlg );
		}	}
		break;

	case WM_CONTEXTMENU:
		if (( HWND )wParam == hPreview ) {
			UINT count = ListView_GetSelectedCount( hPreview );

			if ( count > 0 ) {
				int cmd = OpenPopupMenu( hwndDlg );
				switch( cmd ) {
				case ID_CANCELCHANGE:
				case ID_RESET:
					{
						LVITEM lvi = {0};
						int itemIndx = -1;

						while (( itemIndx = ListView_GetNextItem( hPreview, itemIndx, LVNI_SELECTED )) != -1 ) {
							lvi.mask = LVIF_PARAM;
							lvi.iItem = itemIndx; //lvhti.iItem;
							ListView_GetItem( hPreview, &lvi );

							UndoChanges( lvi.lParam, cmd );
						}

						DoOptionsChanged( hwndDlg );
						break;
			}	}	}
		}
		else {
         HWND htv = GetDlgItem( hwndDlg, IDC_CATEGORYLIST );
			if (( HWND )wParam == htv ) {
				int cmd = OpenPopupMenu( hwndDlg );

				switch( cmd ) {
				case ID_CANCELCHANGE:
				case ID_RESET:
					UndoSubItemChanges( htv, TreeView_GetSelection( htv ), cmd );
					DoOptionsChanged( hwndDlg );
					break;
		}	}	}
		break;

	case WM_NOTIFY:
		switch(((LPNMHDR)lParam)->idFrom) {
		case 0:
			switch(((LPNMHDR)lParam)->code) {
			case PSN_APPLY:
				{
					int indx;

					EnterCriticalSection(&csIconList);

					for (indx = 0; indx < iconList.count; indx++) {
						IconItem *item = iconList.items[indx];
						if (item->temp_reset) {
							DBDeleteContactSetting(NULL, "SkinIcons", item->name);
							if (item->icon != item->default_icon)
								SafeDestroyIcon(&item->icon);
							else
								item->icon = NULL;
						}
						else if (item->temp_file) {
							DBWriteContactSettingTString(NULL, "SkinIcons", item->name, item->temp_file);
							if (item->temp_icon) {
								if (item->icon != item->default_icon)
									SafeDestroyIcon(&item->icon);
								item->icon = item->temp_icon;
								item->temp_icon = NULL;
						}	}
						item->ref_count = 0;
					}
					LeaveCriticalSection(&csIconList);

					DoIconsChanged(hwndDlg);
					return TRUE;
			}	}
			break;

		case IDC_PREVIEW:
			if ( bNeedRebuild )	{
				EnterCriticalSection(&csIconList);
				bNeedRebuild = FALSE;
				LeaveCriticalSection(&csIconList);
				SendMessage(hwndDlg, DM_REBUILD_CTREE, 0, 0);
			}
			break;

		case IDC_CATEGORYLIST:
			switch(((NMHDR*)lParam)->code) {
			case TVN_SELCHANGEDA: // !!!! This needs to be here - both !!
			case TVN_SELCHANGEDW:
				{
					NMTREEVIEW *pnmtv = (NMTREEVIEW*)lParam;
					TVITEM tvi = pnmtv->itemNew;
					TreeItem *treeItem = (TreeItem *)tvi.lParam;
					if ( treeItem )
						SendMessage(hwndDlg, DM_REBUILDICONSPREVIEW, 0, ( LPARAM )(
							(SECTIONPARAM_FLAGS(treeItem->value)&SECTIONPARAM_HAVEPAGE)?
							sectionList.items[ SECTIONPARAM_INDEX(treeItem->value) ] : NULL ) );
					break;
				}
			case TVN_DELETEITEMA: // no idea why both TVN_SELCHANGEDA/W should be there but let's keep this both too...
			case TVN_DELETEITEMW:
				{
					TreeItem *treeItem = (TreeItem *)(((LPNMTREEVIEW)lParam)->itemOld.lParam);
					if (treeItem) {
						mir_free(treeItem->paramName);
						mir_free(treeItem);
					}
					break;
		}	}	}
		break;
	case WM_DESTROY:
		{
			int indx;

			SaveCollapseState( GetDlgItem(hwndDlg, IDC_CATEGORYLIST) );
			DestroyWindow(dat->hwndIndex);

			EnterCriticalSection(&csIconList);
			for (indx = 0; indx < iconList.count; indx++) {
				IconItem *item = iconList.items[indx];

				SAFE_FREE(&item->temp_file);
				SafeDestroyIcon(&item->temp_icon);
				if (item->temp && !item->ref_count)
					if (item->icon!=item->default_icon)
						SafeDestroyIcon(&item->icon);

				item->temp = 0;
			}
			LeaveCriticalSection(&csIconList);

			SAFE_FREE(&dat);
			break;
	}	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Module initialization and finalization procedure

static int sttCompareSections( const SectionItem* p1, const SectionItem* p2 )
{	return _tcscmp( p1->name, p2->name );
}

static int sttCompareIcons( const IconItem* p1, const IconItem* p2 )
{	return strcmp( p1->name, p2->name );
}

static int sttIcoLib_AddNewIcon( WPARAM wParam, LPARAM lParam )
{	return (int)IcoLib_AddNewIcon(( SKINICONDESC* )lParam );
}

static int sttIcoLib_GetIcon( WPARAM wParam, LPARAM lParam )
{	return (int)IcoLib_GetIcon(( const char* )lParam );
}

static int sttIcoLib_GetIconByHandle( WPARAM wParam, LPARAM lParam )
{	return (int)IcoLib_GetIconByHandle(( HANDLE )lParam );
}

int InitSkin2Icons(void)
{
	sectionList.increment = iconList.increment = 20;
	sectionList.sortFunc  = ( FSortFunc )sttCompareSections;
	iconList.sortFunc     = ( FSortFunc )sttCompareIcons;

	hIconBlank = LoadIconEx(NULL, MAKEINTRESOURCE(IDI_BLANK),0);

	hCacheIconList = ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),ILC_COLOR32|ILC_MASK,15,15);

	InitializeCriticalSection(&csIconList);
	hIcoLib_AddNewIcon  = CreateServiceFunction(MS_SKIN2_ADDICON,         sttIcoLib_AddNewIcon);
	hIcoLib_RemoveIcon  = CreateServiceFunction(MS_SKIN2_REMOVEICON,      IcoLib_RemoveIcon);
	hIcoLib_GetIcon     = CreateServiceFunction(MS_SKIN2_GETICON,         sttIcoLib_GetIcon);
	hIcoLib_GetIcon2    = CreateServiceFunction(MS_SKIN2_GETICONBYHANDLE, sttIcoLib_GetIconByHandle);
	hIcoLib_IsManaged   = CreateServiceFunction(MS_SKIN2_ISMANAGEDICON,   ( MIRANDASERVICE )IcoLib_IsManaged);
	hIcoLib_AddRef      = CreateServiceFunction(MS_SKIN2_ADDREFICON,      IcoLib_AddRef); 
	hIcoLib_ReleaseIcon = CreateServiceFunction(MS_SKIN2_RELEASEICON,     ( MIRANDASERVICE )IcoLib_ReleaseIcon);

	hIcons2ChangedEvent = CreateHookableEvent(ME_SKIN2_ICONSCHANGED);
	hIconsChangedEvent = CreateHookableEvent(ME_SKIN_ICONSCHANGED);
	return 0;
}

void UninitSkin2Icons(void)
{
	int indx;

	if (hIconBlank == NULL) return;

	DestroyHookableEvent(hIconsChangedEvent);
	DestroyHookableEvent(hIcons2ChangedEvent);

	DestroyServiceFunction(hIcoLib_AddNewIcon);
	DestroyServiceFunction(hIcoLib_RemoveIcon);
	DestroyServiceFunction(hIcoLib_GetIcon);
	DestroyServiceFunction(hIcoLib_GetIcon2);
	DestroyServiceFunction(hIcoLib_IsManaged);
	DestroyServiceFunction(hIcoLib_AddRef);
	DestroyServiceFunction(hIcoLib_ReleaseIcon);
	DeleteCriticalSection(&csIconList);

	for (indx = 0; indx < iconList.count; indx++) {
		IcoLib_FreeIcon( iconList.items[indx] );
		SAFE_FREE( &iconList.items[indx] );
	}
	List_Destroy(( SortedList* )&iconList );

	for (indx = 0; indx < sectionList.count; indx++) {
		SAFE_FREE( &sectionList.items[indx]->name );
		SAFE_FREE( &sectionList.items[indx] );
	}
	List_Destroy(( SortedList* )&sectionList );

	ImageList_Destroy(hCacheIconList);
	SafeDestroyIcon(&hIconBlank);
}
