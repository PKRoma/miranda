{***************************************************************
 * Project     : Miranda Plugin API for Delphi
 * Unit Name   : m_skin
 * Description : Converted Headerfile from m_skin.h
 *
 * Author      : Christian Kästner
 * Date        : 28.04.2001
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

//status mode icons
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
//event icons
  SKINICON_EVENT_MESSAGE		=100;
  SKINICON_EVENT_URL			=101;
  SKINICON_EVENT_FILE			=102;
  //I think the 'unread' icons are obsolete now
//other icons
  SKINICON_OTHER_MIRANDA		=200;
  SKINICON_OTHER_EXIT			=201;
  SKINICON_OTHER_SHOWHIDE		=202;
  SKINICON_OTHER_GROUP	                =203;
//menu icons are owned by the module that uses them so are not and should not
//be skinnable. Except exit and show/hide

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

//random ideas for the future:
// Skin/CreateSkinnedDialog - just like CreateDialog
// Skin/LoadNetworkAnim - get some silly spinner thing when we want to be busy
//and somehow neatly interact with the contact list

implementation


end.
