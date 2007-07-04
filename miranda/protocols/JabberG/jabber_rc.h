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
Last change on : $Date: 2007-04-28 13:26:31 +0300 (��, 28 ��� 2007) $
Last change by : $Author: ghazan $

*/

#ifndef _JABBER_RC_H_
#define _JABBER_RC_H_

void JabberRcProcessItemsRequest( TCHAR* idStr, XmlNode* node );
void JabberRcProcessCommand( TCHAR* idStr, XmlNode* node, XmlNode* command );


#endif //_JABBER_RC_H_
