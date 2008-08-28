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

#include "commonheaders.h"
#include "xmlParser.h"

static HXML xmlapiCreateNode( LPCTSTR name, LPCTSTR text, char isDeclaration )
{
	XMLNode result = XMLNode::createXMLTopNode( name, isDeclaration );
	if ( text )
		result.updateText( text );
	return result.detach();
}

static void xmlapiDestroyNode( HXML n )
{
	XMLNode tmp; tmp.attach(n);
}

static HXML xmlapiParseString( LPCTSTR str, int* datalen, LPCTSTR tag )
{
	XMLResults res;
	XMLNode result = XMLNode::parseString( str, tag, &res );

	if ( datalen != NULL ) {
		#if defined( _DEBUG )
		if ( res.nColumn > 1000000 )
			DebugBreak();
		#endif

		datalen[0] = 0;

		for ( int i=0; i < res.nLine; i++ ) {
			LPCTSTR p = _tcschr( str, '\n' );
			if ( p == NULL )
				break;

			datalen[0] += int( p-str )+1;
			str = p+1;
		}

		datalen[0] += res.nColumn-1;
	}
	return (tag != NULL || res.error == eXMLErrorNone) ? result.detach() : NULL;
}

static HXML xmlapiAddChild( HXML _n, LPCTSTR name, LPCTSTR text )
{
	XMLNode result = XMLNode(_n).addChild( name );
	if ( text != NULL )
		result.updateText( text );
	return result;
}

static void xmlapiAddChild2( HXML _child, HXML _parent )
{
	XMLNode child(_child), parent(_parent);
	parent.addChild( child );
}

static HXML xmlapiCopyNode( HXML _n )
{
	XMLNode result = XMLNode(_n);
	return result.detach();
}

static LPCTSTR xmlapiGetAttr( HXML _n, int i )
{
	return XMLNode(_n).getAttributeValue( i );
}

static int xmlapiGetAttrCount( HXML _n )
{
	return XMLNode(_n).nAttribute();
}

static LPCTSTR xmlapiGetAttrName( HXML _n, int i )
{
	return XMLNode(_n).getAttributeName( i );
}

static HXML xmlapiGetChild( HXML _n, int i )
{
	return XMLNode(_n).getChildNode( i );
}

static HXML xmlapiGetChildByAttrValue( HXML _n, LPCTSTR name, LPCTSTR attrName, LPCTSTR attrValue )
{
	return XMLNode(_n).getChildNodeWithAttribute( name, attrName, attrValue );
}

static int xmlapiGetChildCount( HXML _n )
{
	return XMLNode(_n).nChildNode();
}

static HXML xmlapiGetFirstChild( HXML _n )
{
	return XMLNode(_n).getChildNode( 0 );
}

static HXML xmlapiGetNthChild( HXML _n, LPCTSTR name, int i )
{
	return XMLNode(_n).getChildNode( name, i );
}

static HXML xmlapiGetNextChild( HXML _n, LPCTSTR name, int* i )
{
	return XMLNode(_n).getChildNode( name, i );
}

static HXML xmlapiGetNextNode( HXML _n )
{
	return XMLNode(_n).getNextNode( );
}

static HXML xmlapiGetChildByPath( HXML _n, LPCTSTR path, char createNodeIfMissing )
{
	return XMLNode(_n).getChildNodeByPath( path, createNodeIfMissing );
}

static LPCTSTR xmlapiGetName( HXML _n )
{
	return XMLNode(_n).getName();
}

static HXML xmlapiGetParent( HXML _n )
{
	return XMLNode(_n).getParentNode();
}

static LPCTSTR xmlapiGetText( HXML _n )
{
	return XMLNode(_n).getText();
}

static LPCTSTR xmlapiGetAttrValue( HXML _n, LPCTSTR attrName )
{
	return XMLNode(_n).getAttribute( attrName );
}

static LPTSTR xmlapiToString( HXML _n, int* datalen )
{
	return XMLNode(_n).createXMLString( 0, datalen );
}

static void xmlapiAddAttr( HXML _n, LPCTSTR attrName, LPCTSTR attrValue )
{
	if ( attrName != NULL && attrValue != NULL )
		XMLNode(_n).addAttribute( attrName, attrValue );
}

static void xmlapiAddAttrInt( HXML _n, LPCTSTR attrName, int attrValue )
{
	TCHAR buf[40];
	_itot( attrValue, buf, 10 );
	XMLNode(_n).addAttribute( attrName, buf );
}

static void xmlapiFree( void* p )
{
	free( p );
}

/////////////////////////////////////////////////////////////////////////////////////////

static int GetXmlApi( WPARAM wParam, LPARAM lParam )
{
	XML_API* xi = ( XML_API* )lParam;
	if ( xi == NULL )
		return FALSE;

	if ( xi->cbSize != sizeof(XML_API))
		return FALSE;

	xi->createNode          = xmlapiCreateNode;
	xi->destroyNode         = xmlapiDestroyNode;

	xi->parseString         = xmlapiParseString;
	xi->toString            = xmlapiToString;
	xi->freeMem             = xmlapiFree;

	xi->addChild            = xmlapiAddChild;
	xi->addChild2           = xmlapiAddChild2;
	xi->copyNode            = xmlapiCopyNode;
	xi->getChild            = xmlapiGetChild;
	xi->getChildByAttrValue = xmlapiGetChildByAttrValue;
	xi->getChildCount       = xmlapiGetChildCount;
	xi->getFirstChild       = xmlapiGetFirstChild;
	xi->getNthChild         = xmlapiGetNthChild;
	xi->getNextChild        = xmlapiGetNextChild;
	xi->getNextNode         = xmlapiGetNextNode;
	xi->getChildByPath      = xmlapiGetChildByPath;
	xi->getName             = xmlapiGetName;
	xi->getParent           = xmlapiGetParent;
	xi->getText             = xmlapiGetText;

	xi->getAttr             = xmlapiGetAttr;
	xi->getAttrCount        = xmlapiGetAttrCount;
	xi->getAttrName         = xmlapiGetAttrName;
	xi->getAttrValue        = xmlapiGetAttrValue;
	xi->addAttr             = xmlapiAddAttr;
	xi->addAttrInt          = xmlapiAddAttrInt;
	return TRUE;
}

void InitXmlApi( void )
{
	CreateServiceFunction( MS_SYSTEM_GET_XI, GetXmlApi );
}
