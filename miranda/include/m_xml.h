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

#if defined( _STATIC )
	typedef struct XMLNode MXmlNode;
#else
	typedef struct 
	{
		void* unused;
	}
		MXmlNode;
#endif

typedef struct
{
	int cbSize;

	void     ( *createNode )( MXmlNode* result, LPCTSTR name, LPCTSTR text );
	void     ( *destroyNode )( MXmlNode* node );

	int      ( *parseString )( MXmlNode* result, LPCTSTR string, int* datalen, LPCTSTR tag );
	LPTSTR   ( *toString )( MXmlNode* result, int* datalen );

	void     ( *addChild )( MXmlNode* result, MXmlNode* parent, LPCTSTR name, LPCTSTR text );
	void     ( *addChild2 )( MXmlNode* child, MXmlNode* parent );
	void     ( *copyNode )( MXmlNode* result, const MXmlNode* parent);
	void     ( *getChild )( MXmlNode* result, const MXmlNode* parent, int number );
	int      ( *getChildCount )( MXmlNode* );
	void     ( *getChildByAttrValue )( MXmlNode* result, const MXmlNode* parent, LPCTSTR name, LPCTSTR attrName, LPCTSTR attrValue );
	void     ( *getFirstChild )( MXmlNode* result, const MXmlNode* parent );
	void     ( *getNthChild )( MXmlNode* result, const MXmlNode* parent, LPCTSTR name, int n );
	LPCTSTR  ( *getName )( MXmlNode* );
	LPCTSTR  ( *getText )( MXmlNode* );

	LPCTSTR  ( *getAttr )( MXmlNode*, int n );
	LPCTSTR  ( *getAttrName )( MXmlNode*, int n );
	LPCTSTR  ( *getAttrValue )( MXmlNode*, LPCTSTR attrName );
	int      ( *getAttrCount )( MXmlNode* );
	void     ( *addAttr )( MXmlNode*, LPCTSTR attrName, LPCTSTR attrValue );
	void     ( *addAttrInt )( MXmlNode*, LPCTSTR attrName, int attrValue );

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
