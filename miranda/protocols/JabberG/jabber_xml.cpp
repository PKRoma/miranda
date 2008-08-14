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

XmlNode::XmlNode( MXmlNode n )
{
	unused = NULL;
	xi.copyNode( this, &n );
}

XmlNode::XmlNode( const XmlNode& n )
{
	unused = NULL;
	xi.copyNode( this, &n );
}

XmlNode& XmlNode::operator =( const XmlNode& n )
{	
	xi.copyNode( this, &n );
	return *this;
}

void XmlNode::addAttr( LPCTSTR name, LPCTSTR value )
{
	xi.addAttr( this, name, value );
}

LPCTSTR XmlNode::getAttr( int n )
{
	return xi.getAttr( this, n );
}

LPCTSTR XmlNode::getAttrName( int n )
{
	return xi.getAttrName( this, n );
}

int XmlNode::getAttrCount()
{
	return xi.getAttrCount( this );
}

LPCTSTR XmlNode::getAttrValue( LPCTSTR key )
{
	return xi.getAttrValue( this, key );
}

XmlNode XmlNode::addChild( LPCTSTR name, int value )
{
	TCHAR buf[40];
	_itot( value, buf, 10 );

	XmlNode result;
	xi.addChild( &result, this, name, buf );
	return result;
}

XmlNode XmlNode::addChild( LPCTSTR name, LPCTSTR value )
{
	XmlNode result;
	xi.addChild( &result, this, name, value );
	return result;
}

XmlNode XmlNode::addChild( LPCTSTR name )
{
	XmlNode result;
	xi.addChild( &result, this, name, NULL );
	return result;
}

XmlNode XmlNode::getChild( int n )
{
	XmlNode result;
	xi.getChild( &result, this, n );
	return result;
}

XmlNode XmlNode::getChild( LPCTSTR key )
{
	XmlNode result;
	xi.getNthChild( &result, this, key, 0 );
	return result;
}

#if defined( _UNICODE )
XmlNode XmlNode::getChild( LPCSTR key )
{
	LPTSTR wszKey = mir_a2t( key );
	XmlNode result;
	xi.getNthChild( &result, this, wszKey, 0 );
	mir_free( wszKey );
	return result;
}

XmlNode XmlNode::getChildByTag( LPCSTR key, LPCSTR attrName, LPCTSTR attrValue )
{
	LPTSTR wszKey = mir_a2t( key ), wszName = mir_a2t( attrName );
	XmlNode result;
	xi.getChildByAttrValue( &result, this, wszKey, wszName, attrValue );
	mir_free( wszKey ), mir_free( wszName );
	return result;
}
#endif

XmlNode XmlNode::getChildByTag( LPCTSTR key, LPCTSTR attrName, LPCTSTR attrValue )
{
	XmlNode result;
	xi.getChildByAttrValue( &result, this, key, attrName, attrValue );
	return result;
}

int XmlNode::getChildCount() 
{
	return xi.getChildCount( this );
}

LPCTSTR XmlNode::getName()
{
	return xi.getName( this );
}

XmlNode XmlNode::getNthChild( LPCTSTR tag, int nth )
{
	int i, num;

	if ( !unused || tag == NULL || _tcslen( tag ) <= 0 || nth < 1 )
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
	return xi.getText( this );
}

LPTSTR XmlNode::getAsString()
{
	int datalen;
	return xi.toString( this, &datalen );
}

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
	unused = NULL;
	xi.createNode( this, pszName, NULL );
}

XmlNode::XmlNode( LPCTSTR pszName, LPCTSTR ptszText )
{
	unused = NULL;
	xi.createNode( this, pszName, ptszText );
}

#if defined( _UNICODE )
XmlNode::XmlNode( const char* pszName, const char* ptszText )
{
	TCHAR *wszName = mir_a2t( pszName ), *wszValue = mir_a2t( ptszText );
	unused = NULL;
	xi.createNode( this, wszName, wszValue );
	mir_free( wszName ), mir_free( wszValue );
}
#endif

XmlNode::~XmlNode()
{
	if ( unused ) {
		xi.destroyNode( this );
		unused = NULL;
	}
}

void XmlNode::addAttr( const char* pszName, LPCTSTR ptszValue )
{
	TCHAR *wszName = mir_a2t( pszName );
	xi.addAttr( this, wszName, ptszValue );
	mir_free( wszName );
}

#if defined( _UNICODE )
void XmlNode::addAttr( const char* pszName, const char* pszValue )
{
	TCHAR *wszName = mir_a2t( pszName ), *wszValue = mir_a2t( pszValue );
	xi.addAttr( this, wszName, wszValue );
	mir_free( wszName ), mir_free( wszValue );
}
#endif

void XmlNode::addAttr( LPCTSTR pszName, int value )
{
	xi.addAttrInt( this, pszName, value );
}

void XmlNode::addAttrID( int id )
{
	TCHAR text[ 100 ];
	mir_sntprintf( text, SIZEOF(text), _T("mir_%d"), id );
	addAttr( _T("id"), text );
}

void XmlNode::addChild( XmlNode& pNode )
{
	xi.addChild2( &pNode, this );
}

XmlNode XmlNode::addChild( const char* pszName, LPCTSTR ptszValue )
{
	XmlNode n;
	TCHAR *wszName = mir_a2t( pszName );
	xi.addChild( &n, this, wszName, ptszValue );
	mir_free( wszName );
	return n;
}

#if defined( _UNICODE )
XmlNode XmlNode::addChild( const char* pszName, const char* pszValue )
{
	XmlNode n;
	TCHAR *wszName = mir_a2t( pszName ), *wszValue = mir_a2t( pszValue );
	xi.addChild( &n, this, wszName, wszValue );
	mir_free( wszName ), mir_free( wszValue );
	return n;
}
#endif

XmlNode XmlNode::addQuery( LPCTSTR szNameSpace )
{
	XmlNode n = addChild( "query" );
	if ( n )
		n.addAttr( "xmlns", szNameSpace );
	return n;
}
