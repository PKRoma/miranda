{***************************************************************
 * Project     : Miranda Plugin API for Delphi
 * Description : Converted Headerfile
 *
 * Author      : Christian Kästner
 * Date        : 22.12.2001
 *
 * Copyright © 2001 by Christian Kästner (christian.k@stner.de)
 ****************************************************************}

unit m_awaymsg;

interface

//show the away/na/etc message for a contact  v0.1.0.1+
//wParam=(WPARAM)(HANDLE)hContact
//lParam=0
//returns 0 on success or nonzero on failure
//returns immediately, without waiting for the message to retrieve
const
  MS_AWAYMSG_SHOWAWAYMSG   ='SRAway/GetMessage';


implementation
end;
