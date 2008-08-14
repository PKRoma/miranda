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

struct XmlNode : public MXmlNode
{
	__forceinline XmlNode()
	{	unused = NULL;
	}

	__forceinline XmlNode( MXmlNode n )
	{	unused = NULL;
		xi.copyNode( this, &n );
	}

	__forceinline XmlNode( const XmlNode& n )
	{	unused = NULL;
		xi.copyNode( this, &n );
	}

	__forceinline XmlNode& operator =( const XmlNode& n )
	{	
		xi.copyNode( this, &n );
		return *this;
	}

	XmlNode( LPCTSTR name );
	XmlNode( LPCTSTR pszName, LPCTSTR ptszText );
	#if defined( _UNICODE )
		XmlNode( const char* pszName, const char* ptszText );
	#endif
	~XmlNode();

	__forceinline operator bool() const
	{	return unused != NULL;
	}

	void addAttr( LPCTSTR pszName, LPCTSTR ptszValue );
	void addAttr( LPCSTR pszName, LPCTSTR ptszValue );
	#if defined( _UNICODE )
		void addAttr( LPCSTR pszName, LPCSTR pszValue );
	#endif
	void addAttr( LPCTSTR pszName, int value );
	void addAttrID( int id );

	int     getAttrCount();
	LPCTSTR getAttr( int n );
	LPCTSTR getAttrName( int n );
	LPCTSTR getAttrValue( LPCTSTR attrName );

	XmlNode addChild( XmlNode );
	XmlNode addChild( LPCTSTR pszName );
	XmlNode addChild( LPCTSTR pszName, LPCTSTR ptszValue );
	XmlNode addChild( LPCTSTR pszName, int iValue );
	XmlNode addChild( LPCSTR pszName, LPCTSTR ptszValue );
	#if defined( _UNICODE )
		XmlNode addChild( LPCSTR pszName, LPCSTR pszValue = NULL );
	#endif
	XmlNode getChild( int n = 0 );
	XmlNode getChild( LPCSTR key );
	XmlNode getChild( LPCTSTR key );
	int     getChildCount();
	XmlNode getChildByTag( LPCTSTR key, LPCTSTR attrName, LPCTSTR attrValue );
	XmlNode getChildByTag( LPCSTR key, LPCSTR attrName, LPCTSTR attrValue );
	XmlNode getNthChild( LPCTSTR key, int n = 0 );

	XmlNode addQuery( LPCTSTR szNameSpace );

	LPCTSTR getName();
	LPCTSTR getText();

	LPTSTR getAsString();
};

class CJabberIqInfo;

struct XmlNodeIq : public XmlNode
{
	XmlNodeIq( const char* type, int id = -1, const TCHAR* to = NULL );
	XmlNodeIq( const char* type, const TCHAR* idStr, const TCHAR* to );
	XmlNodeIq( const char* type, XmlNode node, const TCHAR* to );
	#if defined( _UNICODE )
		XmlNodeIq( const char* type, int id, const char* to );
	#endif
	// new request
	XmlNodeIq( CJabberIqInfo* pInfo );
	// answer to request
	XmlNodeIq( const char* type, CJabberIqInfo* pInfo );
};

typedef void ( *JABBER_XML_CALLBACK )( XmlNode, void* );

inline XmlNode& operator+( XmlNode& n1, XmlNode& n2 )
{	n1.addChild( n2 );
	return n1;
}

#endif
