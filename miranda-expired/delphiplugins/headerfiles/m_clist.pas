{***************************************************************
 * Project     : Miranda Plugin API for Delphi
 * Unit Name   : m_clist
 * Description : Converted Headerfile from m_clist.h
 *               
 * Author      : Christian Kästner
 * Date        : 29.04.2001
 * Last Change : 22.12.2001
 *
 * Copyright © 2001 by Christian Kästner (christian.k@stner.de)
 ****************************************************************}

unit m_clist;

interface

uses windows,statusmodes;

//sent when the user asks to change their status
//wParam=new status, from statusmodes.h
//also sent due to a ms_clist_setstatusmode call
const
  ME_CLIST_STATUSMODECHANGE      ='CList/StatusModeChange';

//force a change of status mode
//wParam=new status, from statusmodes.h
const
  MS_CLIST_SETSTATUSMODE		  ='CList/SetStatusMode';

//get the current status mode
//wParam=lParam=0
//returns the current status
//This is the status *as set by the user*, not any protocol-specific status
//All protocol modules will attempt to conform to this setting at all times
const
  MS_CLIST_GETSTATUSMODE		='CList/GetStatusMode';

//gets a textual description of the given status mode (v0.1.0.1+)
//wParam=status mode, from statusmodes.h
//lParam=flags, below
//returns a static buffer of the description of the given status mode
//returns NULL if the status mode was unknown
const
  GSMDF_PREFIXONLINE = 1;   //prefix "Online: " to all status modes that
                  //imply online, eg "Online: Away';
const
  MS_CLIST_GETSTATUSMODEDESCRIPTION ='CList/GetStatusModeDescription';

//add a new item to the main menu
//wParam=0
//lParam=(LPARAM)(CLISTMENUITEM*)&mi
//returns a handle to the new item, or NULL on failure
//the service that is called when the item is clicked is called with
//wParam=0, lParam=hwndContactList
//dividers are inserted every 100000 positions
//pszContactOwner is ignored for this service.
//there is a #define  PUTPOSITIONSINMENU in clistmenus.c which, when set, will
//cause the position numbers to be placed in brackets after the menu items

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
type
  TCLISTMENUITEM=record
    cbSize:integer;		//size in bytes of this structure
    pszName:PChar;		//text of the menu item
    flags:DWord;		//flags
    Position:Integer;		//approx position on the menu. lower numbers go nearer the top
    hIcon:HIcon;		//icon to put by the item
    pszService:PChar;	        //name of service to call when the item gets selected
    pszPopupName:PChar;	        //name of the popup menu that this item is on. If this
				//is NULL the item is on the root of the menu
    popupPosition:Integer;	//position of the popup menu on the root menu. Ignored
				//if pszPopupName is NULL or the popup menu already
				//existed
    hotKey:DWord;               //keyboard accelerator, same as lParam of WM_HOTKEY
	                        //0 for none
    pszContactOwner:Pchar;      //contact menus only. The protocol module that owns
	                        //the contacts to which this menu item applies. NULL if it
			        //applies to all contacts. If it applies to multiple but not all
			        //protocols, add multiple menu items.
  end;
const
  CMIF_GRAYED     =1;
  CMIF_CHECKED    =2;
  CMIF_HIDDEN     =4;     //only works on contact menus
  CMIF_NOTOFFLINE =8;	 //item won't appear for contacts that are offline
  CMIF_NOTONLINE  =16;	 //         ='      online
  CMIF_NOTONLIST  =32;   //item won't appear on standard contacts
  CMIF_NOTOFFLIST =64;   //item won't appear on contacts that have the 'NotOnList' setting
  MS_CLIST_ADDMAINMENUITEM = 'CList/AddMainMenuItem';

//add a new item to the user contact menus
//identical to clist/addmainmenuitem except when item is selected the service
//gets called with wParam=(WPARAM)(HANDLE)hContact
//pszContactOwner is obeyed.
//popup menus are not supported. pszPopupName and popupPosition are ignored.
//If ctrl is held down when right clicking, the menu position numbers will be
//displayed in brackets after the menu item text. This only works in debug
//builds.
  MS_CLIST_ADDCONTACTMENUITEM='CList/AddContactMenuItem';


//modify an existing menu item     v0.1.0.1+
//wParam=(WPARAM)(HANDLE)hMenuItem
//lParam=(LPARAM)(CLISTMENUITEM*)&clmi
//returns 0 on success, nonzero on failure
//hMenuItem will have been returned by clist/add*menuItem
//clmi.flags should contain cmim_ constants below specifying which fields to
//update. Fields without a mask flag cannot be changed and will be ignored
const
  CMIM_NAME   =  0x80000000 ;
  CMIM_FLAGS  =	  0x40000000;
  CMIM_ICON   =  0x20000000;
  CMIM_HOTKEY =  0x10000000;
  CMIM_ALL    =  0xF0000000;
const
  MS_CLIST_MODIFYMENUITEM        ='CList/ModifyMenuItem';

//the context menu for a contact is about to be built     v0.1.0.1+
//wParam=(WPARAM)(HANDLE)hContact
//lParam=0
//modules should use this to change menu items that are specific to the
//contact that has them
const
  ME_CLIST_PREBUILDCONTACTMENU   ='CList/PreBuildContactMenu';


//sets the service to call when a contact is double-clicked
//wParam=0
//lParam=(LPARAM)(CLISTDOUBLECLICKACTION*)&dca
//contactType is one or more of the constants below
//pszService is called with wParam=hContact, lParam=0
//pszService will only be called if there is no outstanding event on the
//selected contact
//returns 0 on success, nonzero on failure
//in case of conflicts, the first module to have registered will get the
//double click, no others will. This service will return success even for
//duplicates.
type
  TCLISTDOUBLECLICKACTION=record
    cbSize:Integer;
    pszContactOwner:PChar;	//name of protocol owning contact, or NULL for all
    flags:DWord;			//any of the CMIF_NOT... flags above
    pszService:PChar;		//service to call on double click
  end;
const
  MS_CLIST_SETDOUBLECLICKACTION='CList/SetDoubleClickAction';

//gets the string that the contact list will use to represent a contact
//wParam=(WPARAM)(HANDLE)hContact
//lParam=0
//returns a pointer to the name, will always succeed, even if it needs to
//return "(Unknown Contact)';
//this pointer is to a statically allocated buffer which will be overwritten
//on every call to this service. Callers should make sure that they copy the
//information before they call this service again.
  MS_CLIST_GETCONTACTDISPLAYNAME='CList/GetContactDisplayName';


//adds an event to the contact list's queue
//wParam=0
//lParam=(LPARAM)(CLISTEVENT* )
//The contact list will flash hIcon next to the contact hContact (use NULL for
//a system message). szServiceName will be called when the user double clicks
//the icon, at which point the event will be removed from the contact list's
//queue automatically
//pszService is called with wParam=(WPARAM)(HWND)hwndContactList,
//lParam=(LPARAM)(CLISTEVENT* )cle. Its return value is ignored. cle is
//invalidated when your service returns, so take copies of any important
//information in it.
//hDbEvent should be unique since it and hContact are the identifiers used by
//clist/removeevent if, for example, your module implements a 'read next' that
//bypasses the double-click.
type
  PCLISTEVENT=^TCLISTEVENT;
  TCLISTEVENT=record
    cbSize:Integer;              //size in bytes of this structure
    hContact:THandle;	         //handle to the contact to put the icon by
    hIcon:HIcon;		 //icon to flash
    flags:DWord;		 //...of course
    hDbEvent:THandle;	         //caller defined but should be unique for hContact
    lParam:DWord;		 //caller defined
    pszService:PChar;	         //name of the service to call on activation
    pszTooltip:PChar;            //short description of the event to display as a
				 //tooltip on the system tray
  end;
const
  CLEF_URGENT    =1;    //flashes the icon even if the user is occupied,
			//and puts the event at the top of the queue
  CLEF_ONLYAFEW  =2;	//the icon will not flash for ever, only a few
			//times. This is for eg online alert
const
  MS_CLIST_ADDEVENT     ='CList/AddEvent';

//removes an event from the contact list's queue
//wParam=(WPARAM)(HANDLE)hContact
//lParam=(LPARAM)(HANDLE)hDbEvent
//returns 0 if the event was successfully removed, or nonzero if the event
//was not found
const
  MS_CLIST_REMOVEEVENT  ='Clist/RemoveEvent';




//gets the details of an event in the queue             v0.1.2.1+
//wParam=(WPARAM)(HANDLE)hContact
//lParam=iEvent
//returns a CLISTEVENT* on success, NULL on failure
//Returns the iEvent-th event from the queue for hContact, so iEvent=0 will
//get the event that will be got when the user clicks on that contact.
//Use hContact=INVALID_HANDLE_VALUE to search over all contacts, so iEvent=0
//will get the event that will be got if the user clicks the systray icon.
const
  MS_CLIST_GETEVENT    ='CList/GetEvent';

//process a WM_MEASUREITEM message for user context menus   v0.1.1.0+
//wParam, lParam, return value as for WM_MEASUREITEM
//This is for displaying the icons by the menu items. If you don't call this
//and clist/menudrawitem whne drawing a menu returned by one of the three menu
//services below then it'll work but you won't get any icons
const
  MS_CLIST_MENUMEASUREITEM ='CList/MenuMeasureItem';

//process a WM_DRAWITEM message for user context menus      v0.1.1.0+
//wParam, lParam, return value as for WM_MEASUREITEM
//See comments for clist/menumeasureitem
const
  MS_CLIST_MENUDRAWITEM    ='CList/MenuDrawItem';

//builds the context menu for a specific contact            v0.1.1.0+
//wParam=(WPARAM)(HANDLE)hContact
//lParam=0
//returns a HMENU identifying the menu. This should be DestroyMenu()ed when
//finished with.
const
  MS_CLIST_MENUBUILDCONTACT='CList/MenuBuildContact';

//gets the image list with all the useful icons in it     v0.1.1.0+
//wParam=lParam=0
//returns a HIMAGELIST
//the members of this image list are opaque, and you should trust what you
//are given
const
  MS_CLIST_GETICONSIMAGELIST   ='CList/GetIconsImageList';
const
  IMAGE_GROUPOPEN    = 11;
  IMAGE_GROUPSHUT    = 12;

//get the icon that should be associated with a contact     v0.1.2.0+
//wParam=(WPARAM)(HANDLE)hContact
//lParam=0
//returns an index into the contact list imagelist. See clist/geticonsimagelist
//If the contact is flashing an icon, this function will not return that
//flashing icon. Use me_clist_contacticonchanged to get info about that.
const
  MS_CLIST_GETCONTACTICON  ='CList/GetContactIcon';

//The icon of a contact in the contact list has changed    v0.1.2.0+
//wParam=(WPARAM)(HANDLE)hContact
//lParam=iconId
//iconId is an offset into the clist's imagelist. See clist/geticonsimagelist
const
  ME_CLIST_CONTACTICONCHANGED  ='CList/ContactIconChanged';

//******************************* CLUI only *********************************/

// Stuff below here is ideally for the use of a CList UI module only.

//get a handle to the main Miranda menu						v0.1.1.0+
//wParam=lParam=0
//returns a HMENU. This need not be freed since it's owned by clist
const
  MS_CLIST_MENUGETMAIN   ='CList/MenuGetMain';

//get a handle to the Miranda status menu					v0.1.1.0+
//wParam=lParam=0
//returns a HMENU. This need not be freed since it's owned by clist
const
  MS_CLIST_MENUGETSTATUS ='CList/MenuGetStatus';

//processes a menu selection from a menu                    v0.1.1.0+
//wParam=MAKEWPARAM(LOWORD(wParam from WM_COMMAND),flags)
//lParam=(LPARAM)(HANDLE)hContact
//returns TRUE if it processed the command, FALSE otherwise
//hContact is the currently selected contact. It it not used if this is a main
//menu command. If this is NULL and the command is a contact menu one, the
//command is ignored
const
  MPCF_CONTACTMENU   =1;	//test commands from a contact menu
  MPCF_MAINMENU      =2;	//test commands from the main menu
const
  MS_CLIST_MENUPROCESSCOMMAND='CList/MenuProcessCommand';

//processes a menu hotkey                                   v0.1.1.0+
//wParam=virtual key code
//lParam=MPCF_ flags
//returns TRUE if it processed the command, FALSE otherwise
//this should be called in WM_KEYDOWN
const
  MS_CLIST_MENUPROCESSHOTKEY='CList/MenuProcessHotkey';

//process all the messages required for docking             v0.1.1.0+
//wParam=(WPARAM)(MSG*)&msg
//lParam=(LPARAM)(LRESULT*)&lResult
//returns TRUE if the message should not be processed further, FALSE otherwise
//only msg.hwnd, msg.message, msg.wParam and msg.lParam are used
//your wndproc should return lResult if and only if TRUE is returned
const
  MS_CLIST_DOCKINGPROCESSMESSAGE ='CList/DockingProcessMessage';

//determines whether the contact list is docked             v0.1.1.0+
//wParam=lParam=0
//returns nonzero if the contact list is docked, of 0 if it is not
const
  MS_CLIST_DOCKINGISDOCKED       ='CList/DockingIsDocked';

//process all the messages required for the tray icon       v0.1.1.0+
//wParam=(WPARAM)(MSG*)&msg
//lParam=(LPARAM)(LRESULT*)&lResult
//returns TRUE if the message should not be processed further, FALSE otherwise
//only msg.hwnd, msg.message, msg.wParam and msg.lParam are used
//your wndproc should return lResult if and only if TRUE is returned
const
  MS_CLIST_TRAYICONPROCESSMESSAGE ='CList/TrayIconProcessMessage';

//process all the messages required for hotkeys             v0.1.1.0+
//wParam=(WPARAM)(MSG*)&msg
//lParam=(LPARAM)(LRESULT*)&lResult
//returns TRUE if the message should not be processed further, FALSE otherwise
//only msg.hwnd, msg.message, msg.wParam and msg.lParam are used
//your wndproc should return lResult if and only if TRUE is returned
const
  MS_CLIST_HOTKEYSPROCESSMESSAGE ='CList/HotkeysProcessMessage';

//toggles the show/hide status of the contact list          v0.1.1.0+
//wParam=lParam=0
//returns 0 on success, nonzero on failure
const
  MS_CLIST_SHOWHIDE    ='CList/ShowHide';

//creates a new group and calls CLUI to display it          v0.1.1.0+
//wParam=hParentGroup
//lParam=0
//returns a handle to the new group
//hParentGroup is NULL to create the new group at the root, or can be the
//handle of the group of which the new group should be a subgroup.
const
  MS_CLIST_GROUPCREATE  ='CList/GroupCreate';

//deletes a group and calls CLUI to display the change      v0.1.1.0+
//wParam=(WPARAM)(HANDLE)hGroup
//lParam=0
//returns 0 on success, nonzero on failure
const
  MS_CLIST_GROUPDELETE  ='CList/GroupDelete';

//change the expanded state flag for a group internally     v0.1.1.0+
//wParam=(WPARAM)(HANDLE)hGroup
//lParam=newState
//returns 0 on success, nonzero on failure
//newState is nonzero if the group is expanded, 0 if it's collapsed
//CLUI is not called when this change is made
const
  MS_CLIST_GROUPSETEXPANDED ='CList/GroupSetExpanded';

//changes the flags for a group                             v0.1.2.1+
//wParam=(WPARAM)(HANDLE)hGroup
//lParam=MAKELPARAM(flags,flagsMask)
//returns 0 on success, nonzero on failure
//Only the flags given in flagsMask are altered.
//CLUI is called on changes to GROUPF_HIDEOFFLINE.
const
  MS_CLIST_GROUPSETFLAGS  ='CList/GroupSetFlags';

//get the name of a group                                   v0.1.1.0+
//wParam=(WPARAM)(HANDLE)hGroup
//lParam=(LPARAM)(int*)&isExpanded
//returns a static buffer pointing to the name of the group
//returns NULL if hGroup is invalid.
//this buffer is only valid until the next call to this service
//&isExpanded can be NULL if you don't want to know if the group is expanded
//or not.
const
  MS_CLIST_GROUPGETNAME     ='CList/GroupGetName';

//get the name of a group                                   v0.1.2.1+
//wParam=(WPARAM)(HANDLE)hGroup
//lParam=(LPARAM)(DWORD*)&flags
//returns a static buffer pointing to the name of the group
//returns NULL if hGroup is invalid.
//this buffer is only valid until the next call to this service
//&flags can be NULL if you don't want any of that info.
const
  GROUPF_EXPANDED    =$04;
  GROUPF_HIDEOFFLINE =$08;
  MS_CLIST_GROUPGETNAME2     ='CList/GroupGetName2';

//move a group to directly before another group             v0.1.2.1+
//wParam=(WPARAM)(HANDLE)hGroup
//lParam=(LPARAM)(HANDLE)hBeforeGroup
//returns the new handle of the group on success, NULL on failure
//The order is represented by the order in which MS_CLUI_GROUPADDED is called,
//however UIs are free to ignore this order and sort alphabetically if they
//wish.
const
  MS_CLIST_GROUPMOVEBEFORE  ='CList/GroupMoveBefore';

//rename a group internally									v0.1.1.0+
//wParam=(WPARAM)(HANDLE)hGroup
//lParam=(LPARAM)(char*)szNewName
//returns 0 on success, nonzero on failure
//this will fail if the group name is a duplicate of an existing name
//CLUI is not called when this change is made
const
  MS_CLIST_GROUPRENAME      ='CList/GroupRename';

//changes the 'hide offline contacts' flag and call CLUI    v0.1.1.0+
//wParam=newValue
//lParam=0
//returns 0 on success, nonzero on failure
//newValue is 0 to show all contacts, 1 to only show online contacts
//or -1 to toggle the value
const
  MS_CLIST_SETHIDEOFFLINE ='CList/SetHideOffline';

//do the message processing associated with double clicking a contact v0.1.1.0+
//wParam=(WPARAM)(HANDLE)hContact
//lParam=0
//returns 0 on success, nonzero on failure
const
  MS_CLIST_CONTACTDOUBLECLICKED='CList/ContactDoubleClicked';

//change the group a contact belongs to       v0.1.1.0+
//wParam=(WPARAM)(HANDLE)hContact
//lParam=(LPARAM)(HANDLE)hGroup
//returns 0 on success, nonzero on failure
//use hGroup=NULL to put the contact in no group
const
  MS_CLIST_CONTACTCHANGEGROUP  ='CList/ContactChangeGroup';

//determines the ordering of two contacts              v0.1.1.0+
//wParam=(WPARAM)(HANDLE)hContact1
//lParam=(LPARAM)(HANDLE)hContact2
//returns 0 if hContact1 is the same as hContact2
//returns +1 if hContact2 should be displayed after hContact1
//returns -1 if hContact1 should be displayed after hContact2
const
  MS_CLIST_CONTACTSCOMPARE     ='CList/ContactsCompare';

const
  SETTING_TOOLWINDOW_DEFAULT   =1    ;
  SETTING_ONTOP_DEFAULT        =1    ;
  SETTING_MIN2TRAY_DEFAULT     =1    ;
  SETTING_TRAY1CLICK_DEFAULT   =0   ;
  SETTING_HIDEOFFLINE_DEFAULT  =0   ;
  SETTING_HIDEEMPTYGROUPS_DEFAULT  00;
  SETTING_USEGROUPS_DEFAULT    =1  ;
  SETTING_SORTBYSTATUS_DEFAULT =0  ;
  SETTING_TRANSPARENT_DEFAULT  =0  ;
  SETTING_ALPHA_DEFAULT        =200;
  SETTING_AUTOALPHA_DEFAULT    =150;
  SETTING_AUTOHIDE_DEFAULT     =0 ;
  SETTING_HIDETIME_DEFAULT     =30;
  SETTING_CYCLETIME_DEFAULT    =4;
  SETTING_TRAYICON_DEFAULT     =SETTING_TRAYICON_SINGLE;
  SETTING_ALWAYSSTATUS_DEFAULT =0;
  SETTING_ALWAYSMULTI_DEFAULT  =0;

  SETTING_TRAYICON_SINGLE   =0 ;
  SETTING_TRAYICON_CYCLE    =1 ;
  SETTING_TRAYICON_MULTI    =2;

  SETTING_STATE_HIDDEN      =0;
  SETTING_STATE_MINIMIZED   =1;
  SETTING_STATE_NORMAL      =2;

implementation

end.
 