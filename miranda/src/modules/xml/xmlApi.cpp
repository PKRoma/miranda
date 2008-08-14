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

static void xmlCreateNode( XMLNode* result, LPCTSTR name, LPCTSTR text )
{
	*result = XMLNode::createXMLTopNode( name );
	if ( text )
		result->updateText( text );
}

static void xmlDestroyNode( XMLNode* n )
{
	*n = XMLNode();
}

static int xmlParseString( XMLNode* result, LPCTSTR str, int* datalen, LPCTSTR tag )
{
	XMLResults res;
	*result = XMLNode::parseString( str, tag, &res );

	if ( datalen != NULL ) {
		#if defined( _DEBUG )
			if ( res.nColumn > 1000000 )
				DebugBreak();
		#endif

		*datalen = res.nColumn-1;
	}
	return tag != NULL || res.error == eXMLErrorNone;
}

static void xmlAddChild( XMLNode* result, XMLNode* n, LPCTSTR name, LPCTSTR text )
{
	*result = n->addChild( name );
	if ( text != NULL )
		result->updateText( text );
}

static void xmlAddChild2( XMLNode* child, XMLNode* parent )
{
	parent->addChild( *child );
}

static void xmlCopyNode( XMLNode* result, const XMLNode* n )
{
	*result = *n;
}

static LPCTSTR xmlGetAttr( XMLNode* n, int i )
{
	return n->getAttributeValue( i );
}

static int xmlGetAttrCount( XMLNode* n )
{
	return n->nAttribute();
}

static LPCTSTR xmlGetAttrName( XMLNode* n, int i )
{
	return n->getAttributeName( i );
}

static void xmlGetChild( XMLNode* result, const XMLNode* n, int i )
{
	*result = n->getChildNode( i );
}

static void xmlGetChildByAttrValue( XMLNode* result, const XMLNode* n, LPCTSTR name, LPCTSTR attrName, LPCTSTR attrValue )
{
	*result = n->getChildNodeWithAttribute( name, attrName, attrValue );
}

static int xmlGetChildCount( XMLNode* n )
{
	return n->nChildNode();
}

static void xmlGetFirstChild( XMLNode* result, const XMLNode* n )
{
	*result = n->getChildNode( 0 );
}

static void xmlGetNthChild( XMLNode* result, const XMLNode* n, LPCTSTR name, int i )
{
	*result = n->getChildNode( name, i );
}

static LPCTSTR xmlGetName( XMLNode* n )
{
	return n->getName();
}

static LPCTSTR xmlGetText( XMLNode* n )
{
	return n->getText();
}

static LPCTSTR xmlGetAttrValue( XMLNode* n, LPCTSTR attrName )
{
	return n->getAttribute( attrName );
}

static LPTSTR xmlToString( XMLNode* n, int* datalen )
{
	return n->createXMLString( 0, datalen );
}

static void xmlAddAttr( XMLNode* n, LPCTSTR attrName, LPCTSTR attrValue )
{
	n->addAttribute( attrName, attrValue );
}

static void xmlAddAttrInt( XMLNode* n, LPCTSTR attrName, int attrValue )
{
	TCHAR buf[40];
	_itot( attrValue, buf, 10 );
	n->addAttribute( attrName, buf );
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
									
	xi->addChild				= xmlAddChild;
	xi->addChild2				= xmlAddChild2;
	xi->copyNode				= xmlCopyNode;
	xi->getChild				= xmlGetChild;
	xi->getChildByAttrValue = xmlGetChildByAttrValue;
	xi->getChildCount       = xmlGetChildCount;
	xi->getFirstChild			= xmlGetFirstChild;
	xi->getNthChild			= xmlGetNthChild;
	xi->getName					= xmlGetName;
	xi->getText					= xmlGetText;
									
	xi->getAttr     		   = xmlGetAttr;
	xi->getAttrCount        = xmlGetAttrCount;
	xi->getAttrName			= xmlGetAttrName;
	xi->getAttrValue			= xmlGetAttrValue;
	xi->addAttr					= xmlAddAttr;
	xi->addAttrInt				= xmlAddAttrInt;
	return TRUE;
}

void InitXmlApi( void )
{
	CreateServiceFunction( MS_SYSTEM_GET_XI, GetXmlApi );
}