{***************************************************************
 * Project     : Miranda Plugin API for Delphi
 * Description : Converted Headerfile
 *
 * Author      : Christian Kästner
 * Date        : 22.12.2001
 *
 * Copyright © 2001 by Christian Kästner (christian.k@stner.de)
 ****************************************************************}


//brings up the send file dialog for a contact
//wParam=(WPARAM)(HANDLE)hContact
//lParam=0
//returns 0 on success or nonzero on failure
//returns immediately, without waiting for the send
const
  MS_FILE_SENDFILE   ='SRFile/SendCommand';

//brings up the send file dialog with the specified file already chosen
//v0.1.0.1+
//wParam=(WPARAM)(HANDLE)hContact
//lParam=(LPARAM)(const char*)pszFile
//returns 0 on success or nonzero on failure
//returns immediately, without waiting for the send
//the user is not prevented from changing the filename with the 'choose again'
//button
//multiple files can be specified in the same way that OPENFILENAME.lpstrFile
//encodes them (long filename version)
const
  MS_FILE_SENDSPECIFICFILE  ='SRFile/SendSpecificFile';