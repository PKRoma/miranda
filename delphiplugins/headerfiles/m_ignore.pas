{***************************************************************
 * Project     : Miranda Plugin API for Delphi
 * Description : Converted Headerfile
 *
 * Author      : Christian Kästner
 * Date        : 22.12.2001
 *
 * Copyright © 2001 by Christian Kästner (christian.k@stner.de)
 ****************************************************************}

unit m_ignore;

interface

//this module provides UI and storage for blocking only, protocol modules are
//responsible for implementing the block

const
  IGNOREEVENT_ALL       =-1;
  IGNOREEVENT_MESSAGE   =1;
  IGNOREEVENT_URL       =2;
  IGNOREEVENT_FILE      =3;
  IGNOREEVENT_USERONLINE =4;

//determines if a message type to a contact should be ignored  v0.1.0.1+
//wParam=(WPARAM)(HANDLE)hContact
//lParam=message type, an ignoreevent_ constant
//returns 0 if the message should be shown, or nonzero if it should be ignored
//Use hContact=NULL to retrieve the setting for unknown contacts (not on the
//contact list, as either permanent or temporary).
//don't use ignoreevent_all when calling this service
const
  MS_IGNORE_ISIGNORED  ='Ignore/IsIgnored';

//ignore future messages from a contact    v0.1.0.1+
//wParam=(WPARAM)(HANDLE)hContact
//lParam=message type, an ignoreevent_ constant
//returns 0 on success or nonzero on failure
//Use hContact=NULL to retrieve the setting for unknown contacts
const
  MS_IGNORE_IGNORE     ='Ignore/Ignore';

//receive future messages from a contact    v0.1.0.1+
//wParam=(WPARAM)(HANDLE)hContact
//lParam=message type, an ignoreevent_ constant
//returns 0 on success or nonzero on failure
//Use hContact=NULL to retrieve the setting for unknown contacts
const
  MS_IGNORE_UNIGNORE   ='Ignore/Unignore';

implementation
end.
