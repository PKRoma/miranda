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

#ifndef M_IO_H__
#define M_IO_H__ 1

// 1. given a handle return the associated protocol

// 1. UI can give the IO object text msgs

// need API for PROTO -> IO Object (inside io.c atm)

// need API for UI -> IO object (m_io.h)

// need API for IO object <-> proto (inside io.c atm)

// need API for IO object <-> UI (m_io_ui)


/*  A slight issue with the API, have to decide how contacts will be described, will it be via
just string szID's and optional DB hContacts, or complete hDB contact handles?


method #1

	The contact is described by its protocol ID inside szID and is binded to a szProto via IO object
	and lacks any custom string display string or any other data.
	
	An optional hContact is given if the contact is in the buddy list
	
	Advantages of this system include not having to have a DB, easy to tell if a contact is in the db or not
	no DB modifications needed. Adding any extra associated values require another API, and the DB API if hContacts
	exists.
	
	CON: Associated values require different API calls depending on hContact v.s. szID
	CON: This query system has to be written too.
	CON: szID has to kept around as a string
	CON: descriptions have to be shipped toO!

method #2

	The contact is always given as a hContact, if the contact isnt really in the contact list it is
	created as "private" contact handle which really is in a memory table, it works and acts EXACTLY
	like a hContact but you can tell if its on the contact list or not..
	
	This method requires modification of DB and needs extra APIs
	
	CON: Needs to modify all DBs (well CVS ones..)
	CON: Need to modify DB internals to allocate private contacts
	PRO: Easy to modify DB
	PRO: API Already well known
	PRO: ALl associated data can be put inside DB settings that arent on disk

*/


#endif /* M_IO_H__ */
