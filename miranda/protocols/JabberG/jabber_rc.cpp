/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-07  George Hazan
Copyright ( C ) 2007     Maxim Mluhov

XEP-0146 support for Miranda IM

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

File name      : $Source: /cvsroot/miranda/miranda/protocols/JabberG/jabber_privacy.cpp,v $
Revision       : $Revision: 5337 $
Last change on : $Date: 2007-04-28 13:26:31 +0300 (бс, 28 ря№ 2007) $
Last change by : $Author: ghazan $

*/

#include "jabber.h"
#include "jabber_iq.h"
#include "jabber_rc.h"

void JabberRcProcessItemsRequest( TCHAR* idStr, XmlNode* node )
{
	if ( !idStr || !node)
		return;

	TCHAR* szTo = JabberXmlGetAttrValue( node, "to" );
	TCHAR* szFrom = JabberXmlGetAttrValue( node, "from" );
	if ( !szTo || !szFrom )
		return;

	// FIXME: check ACL

	XmlNodeIq iq( "result", node, szFrom );
	XmlNode* query = iq.addChild( "query" );
	query->addAttr( "xmlns", _T(JABBER_FEAT_DISCO_ITEMS) );
	query->addAttr( "node", _T(JABBER_FEAT_COMMANDS) );

	XmlNode* item = query->addChild( "item" );
	item->addAttr( "jid", szTo );
	item->addAttr( "node", _T(JABBER_FEAT_RC_SET_STATUS) );
	item->addAttr( "name", "Set status" );

	item = query->addChild( "item" );
	item->addAttr( "jid", szTo );
	item->addAttr( "node", _T(JABBER_FEAT_RC_SET_OPTIONS) );
	item->addAttr( "name", "Set options" );

	jabberThreadInfo->send( iq );
}

void JabberRcProcessCommand( TCHAR* idStr, XmlNode* node, XmlNode* command )
{
	if ( !idStr || !node || !command )
		return;

	TCHAR* szTo = JabberXmlGetAttrValue( node, "to" );
	TCHAR* szFrom = JabberXmlGetAttrValue( node, "from" );
	if ( !szTo || !szFrom )
		return;

	// FIXME: check ACL

	TCHAR* szNode = JabberXmlGetAttrValue( command, "node" );
	TCHAR* szAction = JabberXmlGetAttrValue( command, "action" );
}
