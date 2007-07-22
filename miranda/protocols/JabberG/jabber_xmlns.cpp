/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-07  George Hazan
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

File name      : $Source: /cvsroot/miranda/miranda/protocols/JabberG/jabber_xmlns.cpp,v $
Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#include "jabber.h"
#include "jabber_rc.h"

/////////////////////////////////////////////////////////////////////////////////////////
// JabberXmlnsDisco

void JabberHandleDiscoInfoRequest( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo )
{
	if ( !pInfo->GetChildNode() )
		return;

	TCHAR* szNode = JabberXmlGetAttrValue( pInfo->GetChildNode(), "node" );
	// caps hack
	if ( g_JabberClientCapsManager.HandleInfoRequest( iqNode, userdata, pInfo, szNode ))
		return;

	// ad-hoc hack:
	if ( szNode && g_JabberAdhocManager.HandleInfoRequest( iqNode, userdata, pInfo, szNode ))
		return;

	// another request, send empty result
	XmlNodeIq iq( "error", pInfo );

	XmlNode *errorNode = iq.addChild( "error" );
	errorNode->addAttr( "code", "404" );
	errorNode->addAttr( "type", "cancel" );
	XmlNode *notfoundNode = errorNode->addChild( "item-not-found" );
	notfoundNode->addAttr( "xmlns", "urn:ietf:params:xml:ns:xmpp-stanzas" );

	jabberThreadInfo->send( iq );
}

void JabberHandleDiscoItemsRequest( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo )
{
	if ( !pInfo->GetChildNode() )
		return;

	// ad-hoc commands check:
	TCHAR* szNode = JabberXmlGetAttrValue( pInfo->GetChildNode(), "node" );
	if ( szNode && g_JabberAdhocManager.HandleItemsRequest( iqNode, userdata, pInfo, szNode ))
		return;

	// another request, send empty result
	XmlNodeIq iq( "result", pInfo );
	XmlNode* resultQuery = iq.addChild( "query" );
	resultQuery->addAttr( "xmlns", _T(JABBER_FEAT_DISCO_ITEMS));
	if ( szNode )
		resultQuery->addAttr( "node", szNode );

	if ( !szNode && JGetByte( "EnableRemoteControl", FALSE )) {
		XmlNode* item = resultQuery->addChild( "item" );
		item->addAttr( "jid", jabberThreadInfo->fullJID );
		item->addAttr( "node", _T(JABBER_FEAT_COMMANDS) );
		item->addAttr( "name", "Ad-hoc commands" );
	}

	jabberThreadInfo->send( iq );
}
