/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-08  George Hazan
Copyright ( C ) 2007     Maxim Mluhov

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or ( at your option ) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

File name      : $URL$
Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#ifndef _JABBER_XML_H_
#define _JABBER_XML_H_

#define NOID -1

#include <m_xml.h>

typedef HANDLE HXML;

void    __fastcall xmlAddChild( HXML, HXML );
HXML    __fastcall xmlAddChild( HXML, LPCTSTR pszName );
HXML    __fastcall xmlAddChild( HXML, LPCTSTR pszName, LPCTSTR ptszValue );
HXML    __fastcall xmlAddChild( HXML, LPCTSTR pszName, int iValue );
HXML    __fastcall xmlAddChild( HXML, LPCSTR pszName, LPCTSTR ptszValue );
#if defined( _UNICODE )
	HXML __fastcall xmlAddChild( HXML, LPCSTR pszName, LPCSTR pszValue = NULL );
#endif

LPCTSTR __fastcall xmlGetAttrValue( HXML, LPCTSTR key );
HXML    __fastcall xmlGetChild( HXML, int n = 0 );
HXML    __fastcall xmlGetChild( HXML, LPCSTR key );
HXML    __fastcall xmlGetChild( HXML, LPCTSTR key );
int     __fastcall xmlGetChildCount( HXML );
HXML    __fastcall xmlGetChildByTag( HXML, LPCTSTR key, LPCTSTR attrName, LPCTSTR attrValue );
HXML    __fastcall xmlGetChildByTag( HXML, LPCSTR key, LPCSTR attrName, LPCTSTR attrValue );
HXML    __fastcall xmlGetNthChild( HXML, LPCTSTR key, int n = 0 );

LPCTSTR __fastcall xmlGetName( HXML );
LPCTSTR __fastcall xmlGetText( HXML );

void    __fastcall xmlAddAttr( HXML, LPCTSTR pszName, LPCTSTR ptszValue );
void    __fastcall xmlAddAttr( HXML, LPCSTR pszName, LPCTSTR ptszValue );
#if defined( _UNICODE )
	void __fastcall xmlAddAttr( HXML, LPCSTR pszName, LPCSTR pszValue );
#endif
void    __fastcall xmlAddAttr( HXML, LPCTSTR pszName, int value );
void    __fastcall xmlAddAttrID( HXML, int id );

int     __fastcall xmlGetAttrCount( HXML );
LPCTSTR __fastcall xmlGetAttr( HXML, int n );
LPCTSTR __fastcall xmlGetAttrName( HXML, int n );
LPCTSTR __fastcall xmlGetAttrValue( HXML, LPCTSTR key );

struct XmlNode
{
	__forceinline XmlNode() { m_hXml = NULL; }
	__forceinline XmlNode( HXML h ) { m_hXml = h; }

	XmlNode( const XmlNode& n );
	XmlNode( LPCTSTR name );
	XmlNode( LPCTSTR pszName, LPCTSTR ptszText );
	#if defined( _UNICODE )
		XmlNode( const char* pszName, const char* ptszText );
	#endif
	~XmlNode();

	XmlNode& operator =( const XmlNode& n );

	__forceinline operator HXML() const
	{	return m_hXml;
	}

	void __forceinline addAttr( LPCTSTR pszName, LPCTSTR ptszValue ) { xmlAddAttr( m_hXml, pszName, ptszValue ); }
	void __forceinline addAttr( LPCSTR pszName, LPCTSTR ptszValue ) { xmlAddAttr( m_hXml, pszName, ptszValue ); }
	#if defined( _UNICODE )
		void __forceinline addAttr( LPCSTR pszName, LPCSTR pszValue ) { xmlAddAttr( m_hXml, pszName, pszValue ); }
	#endif
	void __forceinline addAttr( LPCTSTR pszName, int value ) { xmlAddAttr( m_hXml, pszName, value ); }
	void __forceinline addAttrID( int id ) { xmlAddAttrID( m_hXml, id ); }

	HXML    addQuery( LPCTSTR szNameSpace );

private:
	HXML m_hXml;
};

class CJabberIqInfo;

struct XmlNodeIq : public XmlNode
{
	XmlNodeIq( const char* type, int id = -1, const TCHAR* to = NULL );
	XmlNodeIq( const char* type, const TCHAR* idStr, const TCHAR* to );
	XmlNodeIq( const char* type, HXML node, const TCHAR* to );
	#if defined( _UNICODE )
		XmlNodeIq( const char* type, int id, const char* to );
	#endif
	// new request
	XmlNodeIq( CJabberIqInfo* pInfo );
	// answer to request
	XmlNodeIq( const char* type, CJabberIqInfo* pInfo );
};

typedef void ( *JABBER_XML_CALLBACK )( HXML, void* );

#endif
