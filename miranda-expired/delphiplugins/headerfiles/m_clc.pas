{***************************************************************
 * Project     : Miranda Plugin API for Delphi
 * Description : Converted Headerfile
 *
 * Author      : Christian Kästner
 * Date        : 22.12.2001
 *
 * Copyright © 2001 by Christian Kästner (christian.k@stner.de)
 ****************************************************************}

unit m_clc;

interface

//This module is new in 0.1.2.1

const
  CLISTCONTROL_CLASS ='CListControl';

//styles
const
  CLS_MANUALUPDATE    =$0001;	 //todo
  CLS_SHOWHIDDEN      =$0002;
  CLS_HIDEOFFLINE     =$0004;
  CLS_CHECKBOXES      =$0008;	 //todo
  CLS_MULTICOLUMN	    =$0010;   //todo //not true multi-column, just for ignore/vis options
  CLS_HIDEEMPTYGROUPS =$0020;   //note: this flag will be spontaneously removed if the new subgroup menu item is clicked, for obvious reasons
  CLS_USEGROUPS       =$0040;

  CLS_EX_DISABLEDRAGDROP     =$00000001;
  CLS_EX_EDITLABELS          =$00000002;
  CLS_EX_SHOWSELALWAYS       =$00000004;
  CLS_EX_TRACKSELECT         =$00000008;
  CLS_EX_SHOWGROUPCOUNTS     =$00000010;
  CLS_EX_DIVIDERONOFF        =$00000020;
  CLS_EX_HIDECOUNTSWHENEMPTY =$00000040;
  CLS_EX_NOTRANSLUCENTSEL    =$00000080;
  CLS_EX_LINEWITHGROUPS      =$00000100;
  CLS_EX_QUICKSEARCHVISONLY  =$00000200;
  CLS_EX_SORTGROUPSALPHA     =$00000400;
  CLS_EX_NOSMOOTHSCROLLING   =$00000800;

  CLM_FIRST   =$1000;    //this is the same as LVM_FIRST
  CLM_LAST    =$1100;

//messages, compare with equivalent TVM_s in the MSDN
  CLM_ADDCONTACT        =(CLM_FIRST+0);    //wParam=hContact
  CLM_ADDGROUP          =(CLM_FIRST+1);    //wParam=hGroup
  CLM_AUTOREBUILD       =(CLM_FIRST+2);
  CLM_DELETEITEM        =(CLM_FIRST+3);    //wParam=hItem
  CLM_EDITLABEL         =(CLM_FIRST+4);    //wParam=hItem
  CLM_ENDEDITLABELNOW   =(CLM_FIRST+5);    //wParam=cancel, 0 to save
  CLM_ENSUREVISIBLE     =(CLM_FIRST+6);    //wParam=hItem, lParam=partialOk
  CLE_TOGGLE       =-1;
  CLE_COLLAPSE     =0;
  CLE_EXPAND       =1;
  CLE_INVALID      =$FFFF;
  CLM_EXPAND            =(CLM_FIRST+7) ;   //wParam=hItem, lParam=CLE_
  CLM_FINDCONTACT       =(CLM_FIRST+8) ;   //wParam=hContact, returns an hItem
  CLM_FINDGROUP         =(CLM_FIRST+9) ;   //wParam=hGroup, returns an hItem
  CLM_GETBKCOLOR        =(CLM_FIRST+10);   //returns a COLORREF
  CLM_GETCHECKMARK      =(CLM_FIRST+11);   //todo //wParam=hItem, returns 1 or 0
  CLM_GETCOUNT          =(CLM_FIRST+12);   //returns the total number of items
  CLM_GETEDITCONTROL    =(CLM_FIRST+13);   //returns the HWND, or NULL
  CLM_GETEXPAND         =(CLM_FIRST+14);   //wParam=hItem, returns a CLE_, CLE_INVALID if not a group
  CLM_GETEXTRACOLUMNS   =(CLM_FIRST+15);   //todo //returns number of extra columns
  CLM_GETEXTRAIMAGE     =(CLM_FIRST+16);   //todo //wParam=hItem, lParam=MAKELPARAM(iColumn (0 based),0), returns iImage
  CLM_GETEXTRAIMAGELIST =(CLM_FIRST+17);   //todo //returns HIMAGELIST
  CLM_GETFONT           =(CLM_FIRST+18);   //wParam=fontId, see clm_setfont. returns hFont.
  CLM_GETINDENT         =(CLM_FIRST+19);   //wParam=new group indent
  CLM_GETISEARCHSTRING  =(CLM_FIRST+20);   //lParam=(char*)pszStr, max 120 bytes, returns number of chars in string
  CLM_GETITEMTEXT       =(CLM_FIRST+21);   //wParam=hItem, lParam=(char*)pszStr, max 120 bytes
  CLM_GETSCROLLTIME     =(CLM_FIRST+22);   //returns time in ms
  CLM_GETSELECTION      =(CLM_FIRST+23);   //returns hItem
  CLM_HITTEST           =(CLM_FIRST+25);   //lParam=MAKELPARAM(x,y) (relative to control), returns hItem or NULL
  CLM_SELECTITEM        =(CLM_FIRST+26);   //wParam=hItem
  CLB_TOPLEFT       =0  ;
  CLB_STRETCHV      =1  ;
  CLB_STRETCHH      =2  ;	 //and tile vertically
  CLB_STRETCH       =3                   ;
  CLBM_TYPE         =$00FF              ;
  CLBF_TILEH        =$1000              ;
  CLBF_TILEV        =$2000              ;
  CLBF_PROPORTIONAL =$4000              ;
  CLBF_SCROLL       =$8000              ;
  CLM_SETBKBITMAP      = (CLM_FIRST+27)  ; //wParam=mode, lParam=hBitmap (don't delete it)
  CLM_SETBKCOLOR       = (CLM_FIRST+28)  ; //wParam=a COLORREF, default is GetSysColor(COLOR_3DFACE)
  CLM_SETCHECKMARK     = (CLM_FIRST+29)  ; //todo //wParam=hItem, lParam=1 or 0
  CLM_SETEXTRACOLUMNS  = (CLM_FIRST+30)  ; //todo //wParam=number of extra columns
  CLM_SETEXTRAIMAGE    = (CLM_FIRST+31)  ; //todo //wParam=hItem, lParam=MAKELPARAM(iColumn (0 based),iImage)
  CLM_SETEXTRAIMAGELIST= (CLM_FIRST+32)  ; //todo //lParam=HIMAGELIST
  FONTID_CONTACTS    =0                  ;
  FONTID_INVIS       =1                  ;
  FONTID_OFFLINE     =2                  ;
  FONTID_NOTONLIST   =3                  ;
  FONTID_GROUPS      =4                  ;
  FONTID_GROUPCOUNTS =5                  ;
  FONTID_DIVIDERS    =6                  ;
  FONTID_OFFINVIS    =7                  ;
  FONTID_MAX         =7                  ;
  CLM_SETFONT        =   (CLM_FIRST+33)  ; //wParam=hFont, lParam=MAKELPARAM(fRedraw,fontId)
  CLM_SETINDENT      =   (CLM_FIRST+34)  ; //wParam=new indent, default is 3 pixels
  CLM_SETITEMTEXT    =   (CLM_FIRST+35)  ; //wParam=hItem, lParam=(char*)pszNewText
  CLM_SETSCROLLTIME  =   (CLM_FIRST+36)  ; //wParam=time in ms, default 200


  CLM_SETHIDEEMPTYGROUPS =(CLM_FIRST+38) ; //wParam=TRUE/FALSE
  GREYF_UNFOCUS     =$80000000          ;
  MODEF_OFFLINE     =$40000000          ;
//and use the PF2_ #defines from m_protosvc.h
  CLM_SETGREYOUTFLAGS   =(CLM_FIRST+39) ;  //wParam=new flags
  CLM_GETHIDEOFFLINEROOT= (CLM_FIRST+40);   //returns TRUE/FALSE
  CLM_SETHIDEOFFLINEROOT= (CLM_FIRST+41);   //wParam=TRUE/FALSE
  CLM_SETUSEGROUPS      =(CLM_FIRST+42) ;  //wParam=TRUE/FALSE
  CLM_SETOFFLINEMODES   =(CLM_FIRST+43) ;  //for 'hide offline', wParam=PF2_ flags and MODEF_OFFLINE
  CLM_GETEXSTYLE        =(CLM_FIRST+44) ;  //returns CLS_EX_ flags
  CLM_SETEXSTYLE        =(CLM_FIRST+45) ;  //wParam=CLS_EX_ flags
  CLM_GETLEFTMARGIN     =(CLM_FIRST+46) ;  //returns count of pixels
  CLM_SETLEFTMARGIN     =(CLM_FIRST+47) ;  //wParam=pixels

//notifications  (most are omitted because the control processes everything)
const
  CLNF_ISGROUP=1;
type
  tNMCLISTCONTROL=record
    hdr:NMHDR;
    hItem:HANDLE;
    Action:integer;
    iColumn:integer;	//-1 if not on an extra column
    flags:DWord;
    pt:TPoint;
  end;

{don't know what the U is...)
const
  CLN_FIRST        =(0U-100U)      ;
  CLN_EXPANDED     =(CLN_FIRST-0)  ;    //hItem=hGroup, action=CLE_*
  CLN_LISTREBUILT  =(CLN_FIRST-1)  ;
  CLN_ITEMCHECKED  =(CLN_FIRST-2)  ;    //todo	//hItem,action,flags valid
  CLN_DRAGGING     =(CLN_FIRST-3)  ;		//hItem,pt,flags valid. only sent when cursor outside window, return nonzero if processed
  CLN_DROPPED      =(CLN_FIRST-4)  ;		//hItem,pt,flags valid. only sent when cursor outside window, return nonzero if processed
  CLN_LISTSIZECHANGE =(CLN_FIRST-5);    //pt.y valid. the vertical height of the visible items in the list has changed.
  CLN_OPTIONSCHANGED =(CLN_FIRST-6);	//nothing valid. If you set some extended options they have been overwritten and should be re-set
  CLN_DRAGSTOP     =(CLN_FIRST-7)  ;		//hItem,flags valid. sent when cursor goes back in to the window having been outside, return nonzero if processed
//NM_CLICK        							//todo  //hItem,iColumn,pt,flags valid
//NM_KEYDOWN                                //NMKEY structure, only sent when key is not already processed, return nonzero to prevent further processing
}

//an infotip for an item should be shown now
//wParam=0
//lParam=(LPARAM)(CLCINFOTIP*)&it
//Return nonzero if you process this, because it makes no sense for more than
//one plugin to grab it.
//It is up to the plugin to decide the best place to put the infotip. Normally
//it's a few pixels below and to the right of the cursor
//This event is called after the mouse has been stationary over a contact for
//(by default) 200ms, but see below.
//Everything is in screen coordinates.
type
  TCLCINFOTIP=record
    cbSize:integer;
    isTreeFocused:integer;   //so the plugin can provide an option
    isGroup:integer;     //0 if it's a contact, 1 if it's a group
    hItem:THandle;	 //handle to group or contact
    ptCursor:TPoint;
    rcItem:TRect;
  end;

const
  ME_CLC_SHOWINFOTIP   ='CLC/ShowInfoTip';

//it's time to destroy an infotip
//wParam=0
//lParam=(LPARAM)(CLCINFOTIP*)&it
//Only cbSize, isGroup and hItem are set
//Return nonzero if you process this.
//This is sent when the mouse moves off a contact when clc/showinfotip has
//previously been called.
//If you don't want this behaviour, you should have grabbed the mouse capture
//yourself and made your own arrangements.
const
  ME_CLC_HIDEINFOTIP   ='CLC/HideInfoTip';

//set the hover time before the infotip hooks are called
//wParam=newTime
//lParam=0
//Returns 0 on success or nonzero on failure
//The value of this setting is applied to all current CLC windows, and saved
//to be applied to all future windows, including after restarts.
//newTime is in ms.
//The default is 750ms.
const
  MS_CLC_SETINFOTIPHOVERTIME   ='CLC/SetInfoTipHoverTime';

//get the hover time before the infotip hooks are called
//wParam=lParam=0
//Returns the time in ms
const
  MS_CLC_GETINFOTIPHOVERTIME   ='CLC/GetInfoTipHoverTime';



  implementation
  end.
