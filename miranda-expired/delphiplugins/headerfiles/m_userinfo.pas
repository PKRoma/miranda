{***************************************************************
 * Project     : Miranda Plugin API for Delphi
 * Unit Name   : m_userinfo
 * Description : Converted Headerfile
 *               
 * Author      : Christian Kästner
 * Date        : 22.12.2001
 *
 * Copyright © 2001 by Christian Kästner (christian.k@stner.de)
 ****************************************************************}

unit m_userinfo;

interface

uses m_options;

//show the User Details dialog box
//wParam=(WPARAM)(HANDLE)hContact
//lParam=0
const
  MS_USERINFO_SHOWDIALOG      ='UserInfo/ShowDialog';

(* UserInfo/Initialise			v0.1.2.0+
The user opened a details dialog. Modules should do whatever initialisation
they need and call userinfo/addpage one or more times if they want pages
displayed in the options dialog
wParam=addInfo
lParam=(LPARAM)hContact
addInfo should be passed straight to the wParam of userinfo/addpage
NB: The built-in userinfo module is loaded after all plugins, so calling
HookEvent() in your plugin's Load() function will fail if you specify this
hook. Look up core/m_system.h:me_system_modulesloaded.
*)
const
  ME_USERINFO_INITIALISE  ='UserInfo/Initialise';

(* UserInfo/AddPage			  v0.1.2.0+
Must only be called during an userinfo/initialise hook
Adds a page to the details dialog
wParam=addInfo
lParam=(LPARAM)(OPTIONSDIALOGPAGE* )odp
addInfo must have come straight from the wParam of userinfo/initialise
Pages in the details dialog operate just like pages in property sheets. See the
Microsoft documentation for info on how they operate.
When the pages receive WM_INITDIALOG, lParam=(LPARAM)hContact
Strings in the structure can be released as soon as the service returns, but
icons must be kept around. This is not a problem if you're loading them from a
resource
The 3 'group' elements in the structure are ignored, and will always be ignored
Unlike the options dialog, the details dialog does not resize to fit its
largest page. Details dialog pages should be 222x132 dlus.
The details dialog (currently) has no Cancel button. I'm waiting to see if it's
sensible to have one.
Pages will be sent PSN_INFOCHANGED through WM_NOTIFY (idFrom=0) when a protocol
ack is broadcast for the correct contact and with type=ACKTYPE_GETINFO.
To help you out, PSN_INFOCHANGED will also be sent to each page just after it's
created.
All PSN_ WM_NOTIFY messages have PSHNOTIFY.lParam=(LPARAM)hContact
*)
const
  PSN_INFOCHANGED   =1;
  PSM_FORCECHANGED  =(WM_USER+100);   //force-send a PSN_INFOCHANGED to all pages
const
  MS_USERINFO_ADDPAGE     ='UserInfo/AddPage';


implementation

end.
