{***************************************************************
 * Project     : Miranda Plugin API for Delphi
 * Description : Converted Headerfile
 *
 * Author      : Christian Kästner
 * Date        : 22.12.2001
 *
 * Copyright © 2001 by Christian Kästner (christian.k@stner.de)
 ****************************************************************}


unit m_utils;

interface


//this entire module is v0.1.0.1+
//this module cannot be redefined by a plugin, because it's not useful for it
//to be possible
//There are some more utility services in the database for dealing with time
//and simple string scrambling, but they are very db-orientated

(* Opens a URL in the user's default web browser   v0.1.0.1+
wParam=bOpenInNewWindow
lParam=(LPARAM)(const char* )szUrl
returns 0 on success, nonzero on failure
bOpenInNewWindow should be zero to open the URL in the browser window the user
last used, or nonzero to open in a new browser window. If there's no browser
running, one will be opened to show the URL.
*)
const
  MS_UTILS_OPENURL   ='Utils/OpenURL';

(* Resizes a dialog by calling a custom routine to move the individual
controls   v0.1.0.1+
wParam=0
lParam=(LPARAM)(UTILRESIZEDIALOG* )&urd
Returns 0 on success, or nonzero on failure
Does not support dialogtemplateex dialog boxes, and will return failure if you
try to resize one
The dialog itself should have been resized prior to calling this service
pfnResizer is called once for each control in the dialog
pfnResizer should return a combination of one rd_anchorx_ and one rd_anchory
constant
*)
type
  PUTILRESIZECONTROL=^TUTILRESIZECONTROL;
  TUTILRESIZECONTROL=record
    cbSize:integer;
    wId:LongWord;				//control ID
    rcItem:TRect;			//original control rectangle, relative to dialog
                               //modify in-place to specify the new position
    dlgOriginalSize:TPoint;	//size of dialog client area in template
    dlgNewSize:TPoint;		//current size of dialog client area
  end;
  TDIALOGRESIZERPROC=procedure(hwndDlg:THandle;lParam:DWord;urc:PUTILRESIZECONTROL) of object;//don't know if this is correctly translated, not tested!!!
  TUTILRESIZEDIALOG=record
    cbSize:integer;
    hwndDlg:HWND;
    hInstance:LongWord;	//module containing the dialog template
    lpTemplate:LPCTSTR;		//dialog template
    lParam:DWord;			//caller-defined
    pfnResizer:TDIALOGRESIZERPROC;
  end;
const
  RD_ANCHORX_CUSTOM   =0;	//function did everything required to the x axis, do no more processing
  RD_ANCHORX_LEFT     =0;	//move the control to keep it constant distance from the left edge of the dialog
  RD_ANCHORX_RIGHT    =1;	//move the control to keep it constant distance from the right edge of the dialog
  RD_ANCHORX_WIDTH    =2;	//size the control to keep it constant distance from both edges of the dialog
  RD_ANCHORX_CENTRE   =4;	//move the control to keep it constant distance from the centre of the dialog
  RD_ANCHORY_CUSTOM   =0;
  RD_ANCHORY_TOP      =0;
  RD_ANCHORY_BOTTOM   =8;
  RD_ANCHORY_HEIGHT   =16;
  RD_ANCHORY_CENTRE   =32;
const
  MS_UTILS_RESIZEDIALOG	='Utils/ResizeDialog';

(* Gets the name of a country given its number      v0.1.2.0+
wParam=countryId
lParam=0
Returns a pointer to the string containing the country name on success,
or NULL on failure
*)
const
  MS_UTILS_GETCOUNTRYBYNUMBER  ='Utils/GetCountryByNumber';

(* Gets the full list of country IDs     v0.1.2.0+
wParam=(WPARAM)(int* )piCount
lParam=(LPARAM)(struct CountryListEntry** )ppList
Returns 0 always
Neither wParam nor lParam can be NULL.
The list is sorted alphabetically by country name, on the assumption that it's
quicker to search numbers out of order than it is to search names out of order
*)
type
  TCountryListEntry=record//correct??? not tested
    id:integer;
    szName:PChar;
  end;
const
  MS_UTILS_GETCOUNTRYLIST   ='Utils/GetCountryList';

(******************************* Window lists *******************************)

//allocate a window list   v0.1.0.1+
//wParam=lParam=0
//returns a handle to the new window list
const
  MS_UTILS_ALLOCWINDOWLIST='Utils/AllocWindowList';

//adds a window to the specified window list   v0.1.0.1+
//wParam=0
//lParam=(LPARAM)(WINDOWLISTENTRY*)&wle
//returns 0 on success, nonzero on failure
type
  TWINDOWLISTENTRY=record
    hList:THandle;
    hwnd:LongWord;
    hContact:THandle;
  end;
const
  MS_UTILS_ADDTOWINDOWLIST='Utils/AddToWindowList';
{procedure not converted yet: static int __inline WindowList_Add(HANDLE hList,HWND hwnd,HANDLE hContact) {
	WINDOWLISTENTRY wle;
	wle.hList=hList; wle.hwnd=hwnd; wle.hContact=hContact;
	return CallService(MS_UTILS_ADDTOWINDOWLIST,0,(LPARAM)&wle);
}
//removes a window from the specified window list  v0.1.0.1+
//wParam=(WPARAM)(HANDLE)hList
//lParam=(LPARAM)(HWND)hwnd
//returns 0 on success, nonzero on failure
const
  MS_UTILS_REMOVEFROMWINDOWLIST='Utils/RemoveFromWindowList';
{procedure not converted yet: static int __inline WindowList_Remove(HANDLE hList,HWND hwnd) {
	return CallService(MS_UTILS_REMOVEFROMWINDOWLIST,(WPARAM)hList,(LPARAM)hwnd);
}

//finds a window given the hContact  v0.1.0.1+
//wParam=(WPARAM)(HANDLE)hList
//lParam=(WPARAM)(HANDLE)hContact
//returns the window handle on success, or NULL on failure
const
  MS_UTILS_FINDWINDOWINLIST='Utils/FindWindowInList';
{procedure not converted yet: static HWND __inline WindowList_Find(HANDLE hList,HANDLE hContact) {
	return (HWND)CallService(MS_UTILS_FINDWINDOWINLIST,(WPARAM)hList,(LPARAM)hContact);
}

//broadcasts a message to all windows in a list  v0.1.0.1+
//wParam=(WPARAM)(HANDLE)hList
//lParam=(LPARAM)(MSG*)&msg
//returns 0 on success, nonzero on failure
//Only msg.message, msg.wParam and msg.lParam are used
const
  MS_UTILS_BROADCASTTOWINDOWLIST='Utils/BroadcastToWindowList';
{procedure not converted yet: static int __inline WindowList_Broadcast(HANDLE hList,UINT message,WPARAM wParam,LPARAM lParam) {
	MSG msg;
	msg.message=message; msg.wParam=wParam; msg.lParam=lParam;
	return CallService(MS_UTILS_BROADCASTTOWINDOWLIST,(WPARAM)hList,(LPARAM)&msg);
}

(***************************** Hyperlink windows ********************************)

//there aren't any services here, because you don't need them.
const
  WNDCLASS_HYPERLINK  ='Hyperlink';
//the control will obey the SS_LEFT (0), SS_CENTER (1), and SS_RIGHT (2) styles
//the control will send STN_CLICKED via WM_COMMAND when the link itself is clicked

(***************************** Window Position Saving ***************************)

//saves the position of a window in the database   v0.1.1.0+
//wParam=0
//lParam=(LPARAM)(SAVEWINDOWPOS*)&swp
//returns 0 on success, nonzero on failure
type
  TSAVEWINDOWPOS=record
    hwnd:LongWord;
    hContact:THandle;
    szModule:PChar;		//module name to store the setting in
    szNamePrefix:PChar;	//text to prefix on='x",='width", etc, to form setting names
  end;
const
  MS_UTILS_SAVEWINDOWPOSITION ='Utils/SaveWindowPos';
{procedure not converted yet: static int __inline Utils_SaveWindowPosition(HWND hwnd,HANDLE hContact,const char *szModule,const char *szNamePrefix) {
	SAVEWINDOWPOS swp;
	swp.hwnd=hwnd; swp.hContact=hContact; swp.szModule=szModule; swp.szNamePrefix=szNamePrefix;
	return CallService(MS_UTILS_SAVEWINDOWPOSITION,0,(LPARAM)&swp);
}

//restores the position of a window from the database	 v0.1.1.0+
//wParam=0
//lParam=(LPARAM)(SAVEWINDOWPOS*)&swp
//returns 0 on success, nonzero on failure
//if no position was found in the database, the function returns 1 and does
//nothing
const
  MS_UTILS_RESTOREWINDOWPOSITION ='Utils/RestoreWindowPos';
{procedure not converted yet: static int __inline Utils_RestoreWindowPosition(HWND hwnd,HANDLE hContact,const char *szModule,const char *szNamePrefix) {
	SAVEWINDOWPOS swp;
	swp.hwnd=hwnd; swp.hContact=hContact; swp.szModule=szModule; swp.szNamePrefix=szNamePrefix;
	return CallService(MS_UTILS_RESTOREWINDOWPOSITION,0,(LPARAM)&swp);
}

(************************ Colour Picker Control (0.1.2.1+) **********************)

const
  WNDCLASS_COLOURPICKER ='ColourPicker';

const
  CPM_SETCOLOUR          =$1000;	  //lParam=new colour
  CPM_GETCOLOUR          =$1001;	  //returns colour
  CPM_SETDEFAULTCOLOUR   =$1002;	  //lParam=default, used as first custom colour
  CPM_GETDEFAULTCOLOUR   =$1003;	  //returns colour
  CPN_COLOURCHANGED      =1	;	  //sent through WM_COMMAND



implementation


end.
