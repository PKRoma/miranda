unit m_crypt;

interface

uses windows;

(*
Miranda ICQ: the free icq client for MS Windows
Copyright (C) 2000-1  Richard Hughes, Roland Rabien & Tristan Van de Vreede

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*)

//This encryption is for high-protection encryption (typically public key) to
//be used for transferring data over the network. Not to be confused with the
//routines from the database module which are just designed to obscure
//passwords, etc stored on disk.

//Gets the type of encryption employed by the encryption plugin, for
//informational purposes only (v0.1.0.1+)
//wParam=lParam=0
//returns a pointer to a string containing the type of encryption
//The built in encryption module returns "null"	(the string, not the pointer)
const
  MS_CRYPT_GETTYPE   ='Crypt/GetType';

//requests that a string be encrypted (v0.1.0.1+)
//wParam=0
//lParam=(LPARAM)(CRYPTMESSAGE*)&cm
//returns 0 on success, or one of the error codes below on failure
//success is reported even if no encryption was done. Use
//MS_CRYPT_ISCONTACTSECURE to find out if this was the case
//If cchMessageMax is 0, the service will always return ERROR_NOT_ENOUGH_MEMORY
//and set cchMessageMax to the right value.
//cchMessageMax will be set to the size of the output message on success
//if messageType is anything but CMTF_FILEDATA, the output will be nul-
//terminated. Space for the terminator should be included in the input value
//of cchMessageMax, but will not be included in the output value.
type
  TCRYPTMESSAGE=record
    cbSize:integer;
    hContact:THandle;  //contact the string is to be sent to
    pMessage:Pointer; //message to be encrypted, and destination for encrypted
	                //message
    cchMessage:integer;	//size of the input message, excluding any terminator
    cchMessageMax:integer;  //size of the buffer at pMessage, for output
    messageType:integer;  //one of the cmtf_ constants below
  end;
//ERROR_NOT_ENOUGH_MEMORY  cchMessageMax is too small, it has been set to the
                          //right value, and no encryption has been done
//ERROR_INVALID_PARAMETER  cbSize is wrong
//plugins are free to define their own error codes
const
  CMTF_MESSAGE   =0;
  CMTF_URL       =1;	//just the url of an url transfer
  CMTF_URLDESCR  =2;	//the description section of an url
  CMTF_FILEDATA  =3;	//a section of a file being transferred
const
  MS_CRYPT_ENCRYPT   ='Crypt/Encrypt';

//requests that a string be decrypted (v0.1.0.1+)
//wParam=0
//lParam=(LPARAM)(CRYPTMESSAGE*)&cm
//returns 0 on success or an error code on success
//Encryption plugins should be capable of determining for themselves whether
//a given message is encrypted or not, and doing the right thing to it.
//Should return 0 if the message wasn't encrypted
//Use MS_CRYPT_ISMESSAGEENCRYPTED to see what happened
const
  MS_CRYPT_DECRYPT   ='Crypt/Decrypt';

//tests if a message is signed and/or encrypted (v0.1.0.1+)
//wParam=0
//lParam=(LPARAM)(CRYPTMESSAGE*)&cm
//returns one of the me_ constants below
//cm.cchMessageMax is not used
//if something is encrypted then it must also be signed
const
  ME_ERROR          =-1;
  ME_NOPROTECTION   = 0;
  ME_SIGNED         = 1;
  ME_ENCRYPTED	   =2;
  ME_SIGNEDANDENCRYPTED = (ME_SIGNED or ME_ENCRYPTED);
const
  MS_CRYPT_ISMESSAGEENCRYPTED  ='Crypt/IsMessageEncrypted';

//finds out if a message will be sent in a secure manner (v0.1.0.1+)
//wParam=(WPARAM)(HANDLE)hContact
//lParam=messageType, one of the cmtf_ constants
//returns 1 if messages of the given type to and from the given contact will
//be encrypted during transit, or 0 if they won't
const
  MS_CRYPT_ISCONTACTSECURE  ='Crypt/IsContactSecure';

//gets the public key for a given contact (v0.1.0.1+)
//wParam=0
//lParam=(LPARAM)(CRYPTPUBLICKEY*)&cpk
//returns 0 on success, or an error code on failure
//Crypto plugins are free not to implement this, and return nonzero always.
//For plugins that use symmetric encryption, it's probably for the best not to
//implement this.
//pKey can be NULL only if cchKeyMax is zero
type
  TCRYPTPUBLICKEY=record
    cbSize:integer;
    hContact:THandle;  //the contact to get the key for, or NULL for self.
    pKey:Pointer;   //pointer to memory to get the key
    cchKeyMax:integer;  //size of memory pointed to by pKey
  end;
//ERROR_NOT_ENOUGH_MEMORY  cchKeyMax is too small, it has been set to the
                          //right value, and nothing else has been done
//ERROR_INVALID_PARAMETER  cbSize is wrong
//ERROR_NOT_SUPPORTED      plugin doesn't support this call
//plugins are free to define their own error codes
const
  MS_CRYPT_GETPUBLICKEY   ='Crypt/GetPublicKey';


implementation

end.
