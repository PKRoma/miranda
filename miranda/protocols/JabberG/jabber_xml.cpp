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

#include "jabber.h"

#define TAG_MAX_LEN 128
#define ATTR_MAX_LEN 8192

/////////////////////////////////////////////////////////////////////////////////////////
// XmlNodeIq class members

XmlNodeIq::XmlNodeIq( const char* type, int id, LPCTSTR to ) :
	XmlNode( _T( "iq" ))
{
	if ( type != NULL ) addAttr( "type", type );
	if ( to   != NULL ) addAttr( "to",   to );
	if ( id   != -1   ) addAttrID( id );
}

XmlNodeIq::XmlNodeIq( const char* type, LPCTSTR idStr, LPCTSTR to ) :
	XmlNode( _T( "iq" ))
{
	if ( type  != NULL ) addAttr( "type", type  );
	if ( to    != NULL ) addAttr( "to",   to    );
	if ( idStr != NULL ) addAttr( "id",   idStr );
}

XmlNodeIq::XmlNodeIq( const char* type, XmlNode node, LPCTSTR to ) :
	XmlNode( _T( "iq" ))
{
	if ( type  != NULL ) addAttr( "type", type  );
	if ( to    != NULL ) addAttr( "to",   to    );
	if ( node  != NULL ) {
		const TCHAR *iqId = getAttrValue( _T( "id" ));
		if ( iqId != NULL ) addAttr( "id", iqId );
	}
}

#if defined( _UNICODE )
XmlNodeIq::XmlNodeIq( const char* type, int id, const char* to ) :
	XmlNode( _T( "iq" ))
{
	if ( type != NULL ) addAttr( "type", type );
	if ( to   != NULL ) addAttr( "to",   to );
	if ( id   != -1  ) addAttrID( id );
}
#endif

XmlNodeIq::XmlNodeIq( CJabberIqInfo* pInfo ) :
	XmlNode( _T( "iq" ))
{
	if ( pInfo ) {
		if ( pInfo->GetCharIqType() != NULL ) addAttr( "type", pInfo->GetCharIqType() );
		if ( pInfo->GetReceiver()   != NULL ) addAttr( "to", pInfo->GetReceiver() );
		if ( pInfo->GetIqId()       != -1 ) addAttrID( pInfo->GetIqId() );
	}
}

XmlNodeIq::XmlNodeIq( const char* type, CJabberIqInfo* pInfo ) :
	XmlNode( _T( "iq" ))
{
	if ( type != NULL ) addAttr( "type", type );
	if ( pInfo ) {
		if ( pInfo->GetFrom()  != NULL ) addAttr( "to", pInfo->GetFrom() );
		if ( pInfo->GetIdStr() != NULL ) addAttr( "id", pInfo->GetIdStr() );
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// XmlNode class members

XmlNode::XmlNode( LPCTSTR pszName )
{
	__unused = xi.createNode( pszName, NULL );
}

XmlNode::XmlNode( LPCTSTR pszName, LPCTSTR ptszText )
{
	__unused = xi.createNode( pszName, ptszText );
}

#if defined( _UNICODE )
XmlNode::XmlNode( const char* pszName, const char* ptszText )
{
	TCHAR *wszName = mir_a2t( pszName ), *wszValue = mir_a2t( ptszText );
	__unused = xi.createNode( wszName, wszValue );
	mir_free( wszName ), mir_free( wszValue );
}
#endif

XmlNode::XmlNode( const XmlNode& n )
{
	__unused = xi.copyNode( n );
}

XmlNode& XmlNode::operator =( const XmlNode& n )
{	
	if ( __unused )
		xi.destroyNode( __unused );
	__unused = xi.copyNode( n );
	return *this;
}

XmlNode::~XmlNode()
{
	if ( __unused ) {
		xi.destroyNode( __unused );
		__unused = NULL;
}	}

/////////////////////////////////////////////////////////////////////////////////////////

void XmlNode::addAttr( LPCTSTR name, LPCTSTR value )
{
	xi.addAttr( __unused, name, value );
}

void XmlNode::addAttr( const char* pszName, LPCTSTR ptszValue )
{
	TCHAR *wszName = mir_a2t( pszName );
	xi.addAttr( __unused, wszName, ptszValue );
	mir_free( wszName );
}

#if defined( _UNICODE )
void XmlNode::addAttr( const char* pszName, const char* pszValue )
{
	TCHAR *wszName = mir_a2t( pszName ), *wszValue = mir_a2t( pszValue );
	xi.addAttr( __unused, wszName, wszValue );
	mir_free( wszName ), mir_free( wszValue );
}
#endif

void XmlNode::addAttr( LPCTSTR pszName, int value )
{
	xi.addAttrInt( __unused, pszName, value );
}

void XmlNode::addAttrID( int id )
{
	TCHAR text[ 100 ];
	mir_sntprintf( text, SIZEOF(text), _T("mir_%d"), id );
	addAttr( _T("id"), text );
}

/////////////////////////////////////////////////////////////////////////////////////////

XmlNode XmlNode::addChild( LPCTSTR name )
{
	return xi.addChild( __unused, name, NULL );
}

XmlNode XmlNode::addChild( LPCTSTR name, LPCTSTR value )
{
	return xi.addChild( __unused, name, value );
}

XmlNode XmlNode::addChild( const char* pszName, LPCTSTR ptszValue )
{
	TCHAR *wszName = mir_a2t( pszName );
	HANDLE n = xi.addChild( __unused, wszName, ptszValue );
	mir_free( wszName );
	return n;
}

#if defined( _UNICODE )
XmlNode XmlNode::addChild( const char* pszName, const char* pszValue )
{
	TCHAR *wszName = mir_a2t( pszName ), *wszValue = mir_a2t( pszValue );
	HANDLE n = xi.addChild( __unused, wszName, wszValue );
	mir_free( wszName ), mir_free( wszValue );
	return n;
}
#endif

XmlNode XmlNode::addChild( LPCTSTR name, int value )
{
	TCHAR buf[40];
	_itot( value, buf, 10 );
	return xi.addChild( __unused, name, buf );
}

void XmlNode::addChild( XmlNode& pNode )
{
	xi.addChild2( pNode, __unused );
}

XmlNode XmlNode::addQuery( LPCTSTR szNameSpace )
{
	XmlNode n = addChild( "query" );
	if ( n )
		n.addAttr( "xmlns", szNameSpace );
	return n;
}

/////////////////////////////////////////////////////////////////////////////////////////

LPTSTR XmlNode::getAsString()
{
	int datalen;
	return xi.toString( __unused, &datalen );
}

LPCTSTR XmlNode::getAttr( int n )
{
	return xi.getAttr( __unused, n );
}

int XmlNode::getAttrCount()
{
	return xi.getAttrCount( __unused );
}

LPCTSTR XmlNode::getAttrName( int n )
{
	return xi.getAttrName( __unused, n );
}

LPCTSTR XmlNode::getAttrValue( LPCTSTR key )
{
	return xi.getAttrValue( __unused, key );
}

XmlNode XmlNode::getChild( int n )
{
	return xi.getChild( __unused, n );
}

XmlNode XmlNode::getChild( LPCTSTR key )
{
	return xi.getNthChild( __unused, key, 0 );
}

#if defined( _UNICODE )
XmlNode XmlNode::getChild( LPCSTR key )
{
	LPTSTR wszKey = mir_a2t( key );
	HANDLE result = xi.getNthChild( __unused, wszKey, 0 );
	mir_free( wszKey );
	return result;
}

XmlNode XmlNode::getChildByTag( LPCSTR key, LPCSTR attrName, LPCTSTR attrValue )
{
	LPTSTR wszKey = mir_a2t( key ), wszName = mir_a2t( attrName );
	HANDLE result = xi.getChildByAttrValue( __unused, wszKey, wszName, attrValue );
	mir_free( wszKey ), mir_free( wszName );
	return result;
}
#endif

XmlNode XmlNode::getChildByTag( LPCTSTR key, LPCTSTR attrName, LPCTSTR attrValue )
{
	return xi.getChildByAttrValue( __unused, key, attrName, attrValue );
}

int XmlNode::getChildCount() 
{
	return xi.getChildCount( __unused );
}

LPCTSTR XmlNode::getName()
{
	return xi.getName( __unused );
}

XmlNode XmlNode::getNthChild( LPCTSTR tag, int nth )
{
	int i, num;

	if ( !__unused || tag == NULL || _tcslen( tag ) <= 0 || nth < 1 )
		return XmlNode();

	num = 1;
	for ( i=0; ; i++ ) {
		XmlNode n = getChild(i);
		if ( !n )
			break;
		if ( !lstrcmp( tag, n.getName())) {
			if ( num == nth )
				return n;

			num++;
	}	}

	return XmlNode();
}

LPCTSTR XmlNode::getText()
{
	return xi.getText( __unused );
}
