{***************************************************************
 * Project     : Miranda Plugin API for Delphi
 * Description : Converted Headerfile
 *
 * Author      : Christian Kästner
 * Date        : 22.12.2001
 *
 * Copyright © 2001 by Christian Kästner (christian.k@stner.de)
 ****************************************************************}

unit m_message;

interface

//brings up the send message dialog for a contact
//wParam=(WPARAM)(HANDLE)hContact
//lParam=(LPARAM)(char*)szText
//returns 0 on success or nonzero on failure
//returns immediately, just after the dialog is shown
//szText is the text to put in the edit box of the window (but not send)
//szText=NULL will not use any text
//szText!=NULL is only supported on v0.1.2.0+
const
  MS_MSG_SENDMESSAGE  ='SRMsg/SendCommand';

implementation
end.
