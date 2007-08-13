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

File name      : $Source: /cvsroot/miranda/miranda/protocols/JabberG/jabber_privacy.cpp,v $
Revision       : $Revision: 5337 $
Last change on : $Date: 2007-04-28 13:26:31 +0300 (бс, 28 ря№ 2007) $
Last change by : $Author: ghazan $

*/

#ifndef _JABBER_IQ_HANDLERS_H_
#define _JABBER_IQ_HANDLERS_H_

void JabberProcessIqVersion( XmlNode* node, void* userdata, CJabberIqInfo* pInfo );
void JabberProcessIqLast( XmlNode* node, void* userdata, CJabberIqInfo *pInfo );
void JabberProcessIqPing( XmlNode* node, void* userdata, CJabberIqInfo *pInfo );
void JabberProcessIqTime202( XmlNode* node, void* userdata, CJabberIqInfo *pInfo );
void JabberProcessIqAvatar( XmlNode* node, void* userdata, CJabberIqInfo *pInfo );
void JabberHandleSiRequest( XmlNode* node, void* userdata, CJabberIqInfo *pInfo );
void JabberHandleRosterPushRequest( XmlNode* node, void* userdata, CJabberIqInfo *pInfo );
void JabberHandleIqRequestOOB( XmlNode* node, void* userdata, CJabberIqInfo *pInfo );


#endif //_JABBER_IQ_HANDLERS_H_
