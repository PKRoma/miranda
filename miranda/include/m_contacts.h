/*
Miranda IM

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
*/

#ifndef M_CONTACTS_H__
#define M_CONTACTS_H__ 1

typedef struct {
	int cbSize;
	BYTE dwFlag;
	HANDLE hContact;
	char *szProto;
	BYTE type;
	union {
		BYTE bVal;
		WORD wVal;
		DWORD dVal;
		char *pszVal;
		WORD cchVal;
	};
} CONTACTINFO;

// Types of information you can retreive by setting the dwFlag in CONTACTINFO
#define CNF_FIRSTNAME	1  // returns first name (string)
#define CNF_LASTNAME	2  // returns last name (string)
#define CNF_NICK		3  // returns nick name (string)
#define	CNF_CUSTOMNICK	4  // returns custom nick name, clist name (string)
#define CNF_EMAIL		5  // returns email (string)
#define CNF_CITY		6  // returns city (string)
#define CNF_STATE		7  // returns state (string)
#define CNF_COUNTRY		8  // returns country (string)
#define CNF_PHONE		9  // returns phone (string)
#define CNF_HOMEPAGE	10 // returns homepage (string)
#define CNF_ABOUT		11 // returns about info (string)
#define CNF_GENDER		12 // returns gender (byte,'M','F' character)
#define CNF_AGE			13 // returns age (byte, 0==unspecified)
#define CNF_FIRSTLAST	14 // returns first name + last name (string)
#define CNF_UNIQUEID	15 // returns uniqueid, protocol username (must check type for type of return)

// Special types
// Return the custom name using the name order setting
// IMPORTANT: When using CNF_DISPLAY you MUST free the string returned
// You must **NOT** do this from your version of free() you have to use Miranda's free()
// you can get a function pointer to Miranda's free() via MS_SYSTEM_GET_MMI, see m_system.h
#define CNF_DISPLAY		16
// Same as CNF_DISPLAY except the custom handle is not used
// IMPORTANT: When using CNF_DISPLAYNC you MUST free the string returned
// You must **NOT** do this from your version of free() you have to use Miranda's free()
// you can get a function pointer to Miranda's free() via MS_SYSTEM_GET_MMI, see m_system.h
#define CNF_DISPLAYNC	17

// If MS_CONTACT_GETCONTACTINFO returns 0 (valid), then one of the following
// types is setting telling you what type of info you received
#define CNFT_BYTE		1
#define CNFT_WORD		2
#define CNFT_DWORD		3
#define CNFT_ASCIIZ		4

// Get contact information
// wParam = not used
// lParam = (CONTACTINFO *)
// Returns 1 on failure to retrieve the info and 0 on success.  If
// sucessful, the type is set and the result is put into the associated
// member of CONTACTINFO
#define MS_CONTACT_GETCONTACTINFO	"Miranda/Contact/GetContactInfo"

#endif // M_CONTACTS_H__

