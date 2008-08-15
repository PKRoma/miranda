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
#include "m_xml.h"

static HANDLE xmlCreateNode( LPCTSTR name, LPCTSTR text )
{
	XMLNode result = XMLNode::createXMLTopNode( name );
	if ( text )
		result.updateText( text );
	return result.detach();
}

static void xmlDestroyNode( HANDLE n )
{
	XMLNode tmp(n);
	tmp = XMLNode();
}

static HANDLE xmlParseString( LPCTSTR str, int* datalen, LPCTSTR tag )
{
	XMLResults res;
	XMLNode result = XMLNode::parseString( str, tag, &res );

	if ( datalen != NULL ) {
		#if defined( _DEBUG )
		if ( res.nColumn > 1000000 )
			DebugBreak();
		#endif

		*datalen = res.nColumn-1;
	}
	return (tag != NULL || res.error == eXMLErrorNone) ? result.detach() : NULL;
}

static HANDLE xmlAddChild( HANDLE _n, LPCTSTR name, LPCTSTR text )
{
	XMLNode result = XMLNode(_n).addChild( name );
	if ( text != NULL )
		result.updateText( text );
	return result.detach();
}

static void xmlAddChild2( HANDLE _child, HANDLE _parent )
{
	XMLNode child(_child), parent(_parent);
	parent.addChild( child );
}

static HANDLE xmlCopyNode( HANDLE _n )
{
	XMLNode result = XMLNode(_n);
	return result.detach();
}

static LPCTSTR xmlGetAttr( HANDLE _n, int i )
{
	return XMLNode(_n).getAttributeValue( i );
}

static int xmlGetAttrCount( HANDLE _n )
{
	return XMLNode(_n).nAttribute();
}

static LPCTSTR xmlGetAttrName( HANDLE _n, int i )
{
	return XMLNode(_n).getAttributeName( i );
}

static HANDLE xmlGetChild( HANDLE _n, int i )
{
	XMLNode result = XMLNode(_n).getChildNode( i );
	return result.detach();
}

static HANDLE xmlGetChildByAttrValue( HANDLE _n, LPCTSTR name, LPCTSTR attrName, LPCTSTR attrValue )
{
	XMLNode result = XMLNode(_n).getChildNodeWithAttribute( name, attrName, attrValue );
	return result.detach();
}

static int xmlGetChildCount( HANDLE _n )
{
	return XMLNode(_n).nChildNode();
}

static HANDLE xmlGetFirstChild( HANDLE _n )
{
	XMLNode result = XMLNode(_n).getChildNode( 0 );
	return result.detach();
}

static HANDLE xmlGetNthChild( HANDLE _n, LPCTSTR name, int i )
{
	XMLNode result = XMLNode(_n).getChildNode( name, i );
	return result.detach();
}

static HANDLE xmlGetNextChild( HANDLE _n, LPCTSTR name, int* i )
{
	XMLNode result = XMLNode(_n).getChildNode( name, i );
	return result.detach();
}

static HANDLE xmlGetChildByPath( HANDLE _n, LPCTSTR path, char createNodeIfMissing )
{
	XMLNode result = XMLNode(_n).getChildNodeByPath( path, createNodeIfMissing );
	return result.detach();
}

static LPCTSTR xmlGetName( HANDLE _n )
{
	return XMLNode(_n).getName();
}

static LPCTSTR xmlGetText( HANDLE _n )
{
	return XMLNode(_n).getText();
}

static LPCTSTR xmlGetAttrValue( HANDLE _n, LPCTSTR attrName )
{
	return XMLNode(_n).getAttribute( attrName );
}

static LPTSTR xmlToString( HANDLE _n, int* datalen )
{
	return XMLNode(_n).createXMLString( 0, datalen );
}

static void xmlAddAttr( HANDLE _n, LPCTSTR attrName, LPCTSTR attrValue )
{
	XMLNode(_n).addAttribute( attrName, attrValue );
}

static void xmlAddAttrInt( HANDLE _n, LPCTSTR attrName, int attrValue )
{
	TCHAR buf[40];
	_itot( attrValue, buf, 10 );
	XMLNode(_n).addAttribute( attrName, buf );
}

static void xmlFree( void* p )
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

	xi->createNode          = xmlCreateNode;
	xi->destroyNode         = xmlDestroyNode;

	xi->parseString         = xmlParseString;
	xi->toString            = xmlToString;
	xi->freeMem             = xmlFree;

	xi->addChild            = xmlAddChild;
	xi->addChild2           = xmlAddChild2;
	xi->copyNode            = xmlCopyNode;
	xi->getChild            = xmlGetChild;
	xi->getChildByAttrValue = xmlGetChildByAttrValue;
	xi->getChildCount       = xmlGetChildCount;
	xi->getFirstChild       = xmlGetFirstChild;
	xi->getNthChild         = xmlGetNthChild;
	xi->getNextChild        = xmlGetNextChild;
	xi->getChildByPath      = xmlGetChildByPath;
	xi->getName             = xmlGetName;
	xi->getText             = xmlGetText;

	xi->getAttr             = xmlGetAttr;
	xi->getAttrCount        = xmlGetAttrCount;
	xi->getAttrName         = xmlGetAttrName;
	xi->getAttrValue        = xmlGetAttrValue;
	xi->addAttr             = xmlAddAttr;
	xi->addAttrInt          = xmlAddAttrInt;
	return TRUE;
}

void InitXmlApi( void )
{
	CreateServiceFunction( MS_SYSTEM_GET_XI, GetXmlApi );
}
