/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2008 Miranda ICQ/IM project, 
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

#include <tchar.h>

typedef struct
{
	int cbSize;

	HANDLE   ( *createNode )( LPCTSTR name, LPCTSTR text );
	void     ( *destroyNode )( HANDLE node );

	HANDLE   ( *parseString )( LPCTSTR string, int* datalen, LPCTSTR tag );
	LPTSTR   ( *toString )( HANDLE node, int* datalen );

	HANDLE   ( *addChild )( HANDLE parent, LPCTSTR name, LPCTSTR text );
	void     ( *addChild2 )( HANDLE child, HANDLE parent );
	HANDLE   ( *copyNode )( HANDLE parent);
	HANDLE   ( *getChild )( HANDLE parent, int number );
	int      ( *getChildCount )( HANDLE );
	HANDLE   ( *getChildByAttrValue )( HANDLE parent, LPCTSTR name, LPCTSTR attrName, LPCTSTR attrValue );
	HANDLE   ( *getFirstChild )( HANDLE parent );
	HANDLE   ( *getNthChild )( HANDLE parent, LPCTSTR name, int n );
	HANDLE   ( *getChildByPath )( HANDLE parent, LPCTSTR path, char createNodeIfMissing );
	LPCTSTR  ( *getName )( HANDLE );
	LPCTSTR  ( *getText )( HANDLE );

	LPCTSTR  ( *getAttr )( HANDLE, int n );
	LPCTSTR  ( *getAttrName )( HANDLE, int n );
	LPCTSTR  ( *getAttrValue )( HANDLE, LPCTSTR attrName );
	int      ( *getAttrCount )( HANDLE );
	void     ( *addAttr )( HANDLE, LPCTSTR attrName, LPCTSTR attrValue );
	void     ( *addAttrInt )( HANDLE, LPCTSTR attrName, int attrValue );

	void     ( *freeMem )( void* );
}
	XML_API;

/* every protocol should declare this variable to use the XML API */
extern XML_API xi;

/*
a service to obtain the XML API 

wParam = 0;
lParam = (LPARAM)(XML_API*).

returns TRUE if all is Ok, and FALSE otherwise
*/

#define MS_SYSTEM_GET_XI "Miranda/System/GetXmlApi"

__forceinline int mir_getXI( XML_API* dest )
{
	dest->cbSize = sizeof(*dest);
	return CallService( MS_SYSTEM_GET_XI, 0, (LPARAM)dest );
}
