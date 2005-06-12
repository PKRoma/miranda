/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2005 Miranda ICQ/IM project, 
all portions of this codebase are copyrighted to the people 
listed in contributors.txt.

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

#ifndef M_DATABASE2_H__
#define M_DATABASE2_H__ 1

/* m_database2.h (2005/06)

This header file declares extended services to the classical "3.xx" APIs. 
These should be implemented by database plugins which want to work with a 0.5.xx core

*/

#include <m_database.h>

/*
	wParam: 0
	lParam: (LPARAM) &DBCONTACTVOLATILE
	Affect: Add a 'volatile' entry to the database, this handle must work like any other hContact value but all
		associated data MUST NOT exist on disk, the handle must also work with existind DB API queries such as read/write
		setting. 
	Note: The DBCONTACT_FLAG* given is just an indication of what the voltile handle will be used for, that is all.
	Returns: 0 on success, non zero on failure, see .hContact for result handle
	Version: 0.5+
	Note: ME_DB_CONTACT_ADDED MUST NOT be fired
	Warning: INVALID_HANDLE_VALUE will be returned if the contact can not be allocated.
	Warning: getCapability() inside DATABASELINK will be queried with DB_CAP_FLAGNUM1 which you must return
		DB_CAP_VOLATILE_CONTACTS stating that you support this feature fully otherwise the db plugin won't be loaded
		and Miranda will say it is too old.
*/
#define DBCONTACT_FLAG_CONTACT 0x0	// this volatile handle is going to be used as a hContact
typedef struct {
	int cbSize;
	unsigned flags;		// DBCONTACT_FLAG_
	char * szProto;		// the protocol this contact is on
	HANDLE hContact;	// out
} DBCONTACTVOLATILE;

#define MS_DB_ADD_VOLATILE "DB/Volatile/Add"

/*
	wParam: (HANDLE)hContact
	lParam: 0
	Affect: Given a volatile hContact will delete it and all settings, freeing all DBVARIANTS as it goes
	Returns: 0 on success, non zero on failure
	Note: Passing a real hContact will do nothing
	Warning: ME_DB_CONTACT_DELETED WILL NOT be fired.
*/
#define MS_DB_DELETE_VOLATILE "DB/Volatile/Delete"

static __inline HANDLE DB_AddVolatile(unsigned flags, char * szProto)
{
	DBCONTACTVOLATILE av={0};
	av.cbSize=sizeof(av);
	av.flags=flags;
	av.szProto=szProto;
	av.hContact=INVALID_HANDLE_VALUE;
	if ( CallService(MS_DB_ADD_VOLATILE, 0, (LPARAM)&av) ) return INVALID_HANDLE_VALUE;
	return av.hContact;
}

static __inline int DB_DeleteVolatile(HANDLE hContact)
{
	return CallService(MS_DB_DELETE_VOLATILE, (WPARAM)hContact, 0);
}

#endif /* M_DATABASE2_H__ */

