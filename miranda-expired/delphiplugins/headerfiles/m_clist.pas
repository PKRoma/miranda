{***************************************************************
 * Project     : Miranda Plugin API for Delphi
 * Unit Name   : m_clist
 * Description : Converted Headerfile from m_clist.h
 *               
 * Author      : Christian Kästner
 * Date        : 29.04.2001
 *
 * Copyright © 2001 by Christian Kästner (christian.k@stner.de)
 ****************************************************************}

unit m_clist;

interface

uses windows,statusmodes;

const
  //sent when the user asks to change their status
  //wParam=new status, from statusmodes.h
  //also sent due to a ms_clist_setstatusmode call
  ME_CLIST_STATUSMODECHANGE='CList/StatusModeChange';

  //force a change of status mode
  //wParam=new status, from statusmodes.h
  MS_CLIST_SETSTATUSMODE='CList/SetStatusMode';

  //get the current status mode
  //wParam=lParam=0
  //returns the current status
  //This is the status *as set by the user*, not any protocol-specific status
  MS_CLIST_GETSTATUSMODE='CList/GetStatusMode';

  //change icq-specific status indicators
  //wParam=new status
  MS_CLIST_ICQSTATUSCHANGED='CList/ICQStatusChanged';

  //change msn-specific status indicators
  //wParam=new status
  MS_CLIST_MSNSTATUSCHANGED='CList/MSNStatusChanged';

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
  CMIF_NOTONLINE  =16;	 //          "      online
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
//return "(Unknown Contact)"
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



implementation

end.
 