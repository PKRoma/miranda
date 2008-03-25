/*
#if 0 // Items below are just copy/pasted from m_clist.h 
	  // and not used actually

//add a new item to the main menu
//wParam=0
//lParam=(LPARAM)(CLISTMENUITEM*)&mi
//returns a handle to the new item, or NULL on failure
//the service that is called when the item is clicked is called with
//wParam=0, lParam=hwndContactList
//dividers are inserted every 100000 positions
//pszContactOwner is ignored for this service.
//there is a #define PUTPOSITIONSINMENU in clistmenus.c which, when set, will
//cause the position numbers to be placed in brackets after the menu items
typedef struct 
{
	int cbSize;			//size in bytes of this structure
	union 
	{
		char*  pszName;      //text of the menu item
		TCHAR* ptszName;     //Unicode text of the menu item
	};
	DWORD flags;			//flags
	int position;			//approx position on the menu. lower numbers go nearer the top
	HICON hIcon;			//icon to put by the item. If this was not loaded from
							//a resource, you can delete it straight after the call
	char* pszService;		//name of service to call when the item gets selected
	union 
	{
		char* pszPopupName;  //name of the popup menu that this item is on. If this
						  	 //is NULL the item is on the root of the menu
		TCHAR* ptszPopupName;
	};

	int popupPosition;		//position of the popup menu on the root menu. Ignored
							//if pszPopupName is NULL or the popup menu already
							//existed
	DWORD hotKey;			//keyboard accelerator, same as lParam of WM_HOTKEY
							//0 for none
	char *pszContactOwner;  //contact menus only. The protocol module that owns
							//the contacts to which this menu item applies. NULL if it
							//applies to all contacts. If it applies to multiple but not all
							//protocols, add multiple menu items or use ME_CLIST_PREBUILDCONTACTMENU
} CLISTMENUITEM;

#define CMIF_GRAYED     1
#define CMIF_CHECKED    2
#define CMIF_HIDDEN     4		//only works on contact menus
#define CMIF_NOTOFFLINE 8		//item won't appear for contacts that are offline
#define CMIF_NOTONLINE  16		//          "      online
#define CMIF_NOTONLIST  32		//item won't appear on standard contacts
#define CMIF_NOTOFFLIST 64		//item won't appear on contacts that have the 'NotOnList' setting
#define MS_CLIST_ADDMAINMENUITEM        "CList/AddMainMenuItem"

//add a new item to the user contact menus
//identical to clist/addmainmenuitem except when item is selected the service
//gets called with wParam=(WPARAM)(HANDLE)hContact
//pszContactOwner is obeyed.
//popup menus are not supported. pszPopupName and popupPosition are ignored.
//If ctrl is held down when right clicking, the menu position numbers will be
//displayed in brackets after the menu item text. This only works in debug
//builds.
#define MS_CLIST_ADDCONTACTMENUITEM     "CList/AddContactMenuItem"

//modify an existing menu item     v0.1.0.1+
//wParam=(WPARAM)(HANDLE)hMenuItem
//lParam=(LPARAM)(CLISTMENUITEM*)&clmi
//returns 0 on success, nonzero on failure
//hMenuItem will have been returned by clist/add*menuItem
//clmi.flags should contain cmim_ constants below specifying which fields to
//update. Fields without a mask flag cannot be changed and will be ignored
#define CMIM_NAME     0x80000000
#define CMIM_FLAGS	  0x40000000
#define CMIM_ICON     0x20000000
#define CMIM_HOTKEY   0x10000000
#define CMIM_ALL      0xF0000000
#define MS_CLIST_MODIFYMENUITEM         "CList/ModifyMenuItem"

//the context menu for a contact is about to be built     v0.1.0.1+
//wParam=(WPARAM)(HANDLE)hContact
//lParam=0
//modules should use this to change menu items that are specific to the
//contact that has them
#define ME_CLIST_PREBUILDCONTACTMENU    "CList/PreBuildContactMenu"

//process a WM_MEASUREITEM message for user context menus   v0.1.1.0+
//wParam, lParam, return value as for WM_MEASUREITEM
//This is for displaying the icons by the menu items. If you don't call this
//and clist/menudrawitem whne drawing a menu returned by one of the three menu
//services below then it'll work but you won't get any icons
#define MS_CLIST_MENUMEASUREITEM  "CList/MenuMeasureItem"

//process a WM_DRAWITEM message for user context menus      v0.1.1.0+
//wParam, lParam, return value as for WM_MEASUREITEM
//See comments for clist/menumeasureitem
#define MS_CLIST_MENUDRAWITEM     "CList/MenuDrawItem"

//builds the context menu for a specific contact            v0.1.1.0+
//wParam=(WPARAM)(HANDLE)hContact
//lParam=0
//returns a HMENU identifying the menu. This should be DestroyMenu()ed when
//finished with.
#define MS_CLIST_MENUBUILDCONTACT "CList/MenuBuildContact"

#endif

*/



/** 

 MenuService_CList_AddMainMenuItem [MS_CLIST_ADDMAINMENUITEM]

 IN:	wParam: None
		lParam: (CLISTMENUITEM*) lpCListMenuItem
 OUT:	None 
 
 RETURN:Handle to the new item, or NULL on failure  
 
 DESCRIPTION: adds a new item to the main menu, returns a handle to the new
 item, or NULL on failure. The service that is called when the item is 
 clicked is called with wParam=0, lParam=hwndContactList. Dividers are 
 inserted every 100000 positions. pszContactOwner is ignored for 
 this service. 

*/
int MenuService_CList_AddMainMenuItem(WPARAM wParam, LPARAM lParam);

//add a new item to the user contact menus
//identical to clist/addmainmenuitem except when item is selected the service
//gets called with wParam=(WPARAM)(HANDLE)hContact
//pszContactOwner is obeyed.
//popup menus are not supported. pszPopupName and popupPosition are ignored.
//If ctrl is held down when right clicking, the menu position numbers will be
//displayed in brackets after the menu item text. This only works in debug
//builds.


/** 

 MenuService_CList_AddContactMenuItem [MS_CLIST_ADDCONTACTMENUITEM]

 IN:	wParam: None
		lParam: (CLISTMENUITEM*) lpCListMenuItem
 OUT:	None 
 
 RETURN:Handle to the new item, or NULL on failure  
 
 DESCRIPTION: adds a new item to the user contact menus, returns a 
 handle to the new item, or NULL on failure. The service that is called 
 when the item is clicked gets called with wParam=(WPARAM)(HANDLE)hContact
 //pszContactOwner is obeyed.
 Dividers are inserted every 100000 positions. 

*/
int MenuService_CList_AddContactMenuItem(WPARAM wParam, LPARAM lParam);

