{***************************************************************
 * Project     : Miranda Plugin API for Delphi
 * Description : Converted Headerfile
 *
 * Author      : Christian Kästner
 * Date        : 22.12.2001
 *
 * Copyright © 2001 by Christian Kästner (christian.k@stner.de)
 ****************************************************************}

unit m_url;

interface


//bring up the send URL dialog for a user
//wParam=(WPARAM)(HANDLE)hContact
//lParam=0
//returns 0 on success or nonzero on failure
//returns immediately, before the url is sent
const
   MS_URL_SENDURL    'SRUrl/SendCommand';


implementation
end.
