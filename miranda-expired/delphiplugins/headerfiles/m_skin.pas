{***************************************************************
 * Project     : Miranda Plugin API for Delphi
 * Unit Name   : m_skin
 * Description : Converted Headerfile from m_skin.h
 *
 * Author      : Christian Kästner
 * Date        : 28.04.2001
 * Last Change : 22.12.2001
 *
 * Copyright © 2001 by Christian Kästner (christian.k@stner.de)
 ****************************************************************}

unit m_skin;

interface

uses windows;

//loads an icon from the user's custom skin library, or from the exe if there
//isn't one of them
//wParam=id of icon to load - see below
//lParam=0
//returns an hIcon for the new icon. Does not need to be DestroyIcon()ed, since
//it was LoadIcon()ed
//returns NULL on error
const
  MS_SKIN_LOADICON    ='Skin/Icons/Load';
//nice function to wrap this:
//see: function LoadSkinnedIcon(id:Integer):hicon;

//event icons
  SKINICON_EVENT_MESSAGE		=100;
  SKINICON_EVENT_URL			=101;
  SKINICON_EVENT_FILE			=102;
//other icons
  SKINICON_OTHER_MIRANDA		=200;
  SKINICON_OTHER_EXIT			=201;
  SKINICON_OTHER_SHOWHIDE		=202;
  SKINICON_OTHER_GROUPOPEN              =203;		//v0.1.1.0+
  SKINICON_OTHER_GROUPSHUT              =205;		//v0.1.1.0+
  SKINICON_OTHER_USERONLINE             =204;     //v0.1.0.1+
//menu icons are owned by the module that uses them so are not and should not
//be skinnable. Except exit and show/hide

//status mode icons. NOTE: These are deprecated in favour of LoadSkinnedProtoIcon()
const
  SKINICON_STATUS_OFFLINE		=0;
  SKINICON_STATUS_ONLINE		=1;
  SKINICON_STATUS_AWAY		        =2;
  SKINICON_STATUS_NA			=3;
  SKINICON_STATUS_OCCUPIED	        =4;
  SKINICON_STATUS_DND			=5;
  SKINICON_STATUS_FREE4CHAT	        =6;
  SKINICON_STATUS_INVISIBLE	        =7;
  SKINICON_STATUS_ONTHEPHONE	        =8;
  SKINICON_STATUS_OUTTOLUNCH	        =9;
//menu icons are owned by the module that uses them so are not and should not
//be skinnable. Except exit and show/hide

//Loads an icon representing the status mode for a particular protocol.
//wParam=(WPARAM)(const char*)szProto
//lParam=status
//returns an hIcon for the new icon. Do *not* DestroyIcon() the return value
//returns NULL on failure
//if szProto is NULL the function will load the user's selected 'all protocols'
//status icon.
const
  MS_SKIN_LOADPROTOICON     ='Skin/Icons/LoadProto';
//nice function to wrap this:
//not implemented: static HICON __inline LoadSkinnedProtoIcon(const char *szProto,int status) {return (HICON)CallService(MS_SKIN_LOADPROTOICON,(WPARAM)szProto,status);}


//add a new sound so it has a default and can be changed in the options dialog
//wParam=0
//lParam=(LPARAM)(SKINSOUNDDESC*)ssd;
//returns 0 on success, nonzero otherwise
type
  TSKINSOUNDDESC=record
    cbSize:Integer;
    pszName:PChar;		   //name to refer to sound when playing and in db
    pszDescription:PChar;	   //description for options dialog
    pszDefaultFile:PChar;	   //default sound file to use, without path
  end;
const
  MS_SKIN_ADDNEWSOUND     ='Skin/Sounds/AddNew';
//see: function SkinAddNewSound(Name,Description,defaultFile:PChar):Integer;

//play a named sound event
//wParam=0
//lParam=(LPARAM)(const char*)pszName
//pszName should have been added with Skin/Sounds/AddNew, but if not the
//function will not fail, it will play the Windows default sound instead.
const
  MS_SKIN_PLAYSOUND  ='Skin/Sounds/Play';
//see: function SkinPlaySound(PluginLink:TPluginLink;Name:Pchar):integer;

//sent when the icons DLL has been changed in the options dialog, and everyone
//should re-make their image lists
//wParam=lParam=0
const
  ME_SKIN_ICONSCHANGED     ='Skin/IconsChanged';

//random ideas for the future:
// Skin/LoadNetworkAnim - get some silly spinner thing when we want to be busy

implementation


end.
