{***************************************************************
 * Project     : Miranda Plugin API for Delphi
 * Unit Name   : m_email
 * Description : Converted Headerfile from m_email.h
 *
 * Author      : Christian Kästner
 * Date        : 24.05.2001
 *
 * Copyright © 2001 by Christian Kästner (christian.k@stner.de)
 ****************************************************************}

unit m_email;

interface

//send an e-mail to the specified contact     v0.1.0.1+
//wParam=(WPARAM)(HANDLE)hContact
//lParam=0
//returns 0 on success or nonzero on failure
//if an error occurs the service will display a message box with the error
//text, so this service should not be used if you do not want this behaviour.
const
  MS_EMAIL_SENDEMAIL  ='SREMail/SendCommand';

implementation

end.
