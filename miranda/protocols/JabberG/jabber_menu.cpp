/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005     George Hazan

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

*/

#include "jabber.h"
#include "jabber_list.h"

int JabberMenuPrebuildContactMenu( WPARAM wParam, LPARAM lParam )
{
	HANDLE hContact;
	DBVARIANT dbv;
	JABBER_LIST_ITEM *item;
	CLISTMENUITEM clmi = {0};

	clmi.cbSize = sizeof( CLISTMENUITEM );
	if (( hContact=( HANDLE ) wParam )!=NULL && jabberOnline ) {
		if ( !DBGetContactSetting( hContact, jabberProtoName, "jid", &dbv )) {
			if (( item=JabberListGetItemPtr( LIST_ROSTER, dbv.pszVal )) != NULL ) {
				if ( item->subscription==SUB_NONE || item->subscription==SUB_FROM )
					clmi.flags = CMIM_FLAGS;
				else
					clmi.flags = CMIM_FLAGS|CMIF_HIDDEN;
				JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM ) hMenuRequestAuth, ( LPARAM )&clmi );

				if ( item->subscription==SUB_NONE || item->subscription==SUB_TO )
					clmi.flags = CMIM_FLAGS;
				else
					clmi.flags = CMIM_FLAGS|CMIF_HIDDEN;
				JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM ) hMenuGrantAuth, ( LPARAM )&clmi );

				JFreeVariant( &dbv );
				return 0;
			}
			JFreeVariant( &dbv );
		}
	}
	clmi.flags = CMIM_FLAGS|CMIF_HIDDEN;
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM ) hMenuRequestAuth, ( LPARAM )&clmi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM ) hMenuGrantAuth, ( LPARAM )&clmi );

	return 0;
}

int JabberMenuHandleRequestAuth( WPARAM wParam, LPARAM lParam )
{
	HANDLE hContact;
	DBVARIANT dbv;

	if (( hContact=( HANDLE ) wParam )!=NULL && jabberOnline ) {
		if ( !DBGetContactSetting( hContact, jabberProtoName, "jid", &dbv )) {
			// JID is already in UTF-8 format, no encoding required
			JabberSend( jabberThreadInfo->s, "<presence to='%s' type='subscribe'/>", dbv.pszVal );
			JFreeVariant( &dbv );
		}
	}
	return 0;
}

int JabberMenuHandleGrantAuth( WPARAM wParam, LPARAM lParam )
{
	HANDLE hContact;
	DBVARIANT dbv;

	if (( hContact=( HANDLE ) wParam )!=NULL && jabberOnline ) {
		if ( !DBGetContactSetting( hContact, jabberProtoName, "jid", &dbv )) {
			// JID is already in UTF-8 format, no encoding required
			JabberSend( jabberThreadInfo->s, "<presence to='%s' type='subscribed'/>", dbv.pszVal );
			JFreeVariant( &dbv );
		}
	}
	return 0;
}
