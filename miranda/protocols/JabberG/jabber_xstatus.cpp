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

File name      : $Source: /cvsroot/miranda/miranda/protocols/JabberG/jabber_xstatus.cpp,v $
Revision       : $Revision: 5370 $
Last change on : $Date: 2007-05-06 02:13:31 +0400 (В�?, 06 май 2007) $
Last change by : $Author: ghazan $

*/

#include "jabber.h"
#include "jabber_caps.h"

#include "m_genmenu.h"
#include "m_icolib.h"

#define NUM_XMODES 61

static HANDLE hHookExtraIconsRebuild = NULL;
static HANDLE hHookStatusBuild = NULL;
static HANDLE hHookExtraIconsApply = NULL;

static int jabberXStatus = 0;

static BOOL   bXStatusMenuBuilt = FALSE;
static HANDLE hXStatusItems[ NUM_XMODES+1 ];

const char* arXStatusNames[ NUM_XMODES ] = {
	LPGEN("Afraid"),      LPGEN("Amazed"),      LPGEN("Angry"),     LPGEN("Annoyed"),     LPGEN("Anxious"),      LPGEN("Aroused"),
	LPGEN("Ashamed"),     LPGEN("Bored"),       LPGEN("Brave"),     LPGEN("Calm"),        LPGEN("Cold"),         LPGEN("Confused"),
	LPGEN("Contented"),   LPGEN("Cranky"),      LPGEN("Curious"),   LPGEN("Depressed"),   LPGEN("Disappointed"), LPGEN("Disgusted"),
	LPGEN("Distracted"),  LPGEN("Embarrassed"), LPGEN("Excited"),   LPGEN("Flirtatious"), LPGEN("Frustrated"),   LPGEN("Grumpy"),
	LPGEN("Guilty"),      LPGEN("Happy"),       LPGEN("Hot"),       LPGEN("Humbled"),     LPGEN("Humiliated"),   LPGEN("Hungry"),
	LPGEN("Hurt"),        LPGEN("Impressed"),   LPGEN("In awe"),    LPGEN("In love"),     LPGEN("Indignant"),    LPGEN("Interested"),
	LPGEN("Intoxicated"), LPGEN("Invincible"),  LPGEN("Jealous"),   LPGEN("Lonely"),      LPGEN("Mean"),         LPGEN("Moody"),
	LPGEN("Nervous"),     LPGEN("Neutral"),     LPGEN("Offended"),  LPGEN("Playful"),     LPGEN("Proud"),        LPGEN("Relieved"),
	LPGEN("Remorseful"),  LPGEN("Restless"),    LPGEN("Sad"),       LPGEN("Sarcastic"),   LPGEN("Serious"),      LPGEN("Shocked "),
	LPGEN("Shy"),         LPGEN("Sick"),        LPGEN("Sleepy"),    LPGEN("Stressed"),    LPGEN("Surprised"),    LPGEN("Thirsty"),
	LPGEN("Worried")
};

static HANDLE arXStatusIcons[ NUM_XMODES ];

/////////////////////////////////////////////////////////////////////////////////////////
// JabberSetContactMood - sets the contact's mood

void JabberSetContactMood( HANDLE hContact, const char* moodType, const TCHAR* moodText )
{
	if ( moodType ) {
		char* mood = NEWSTR_ALLOCA( moodType );
		_toupper(mood);
		char* spacePos = strchr( mood, '_' );
		if ( spacePos ) *spacePos = ' ';

		int i;
		for ( i=0; i < NUM_XMODES; i++ ) {
			if ( !strcmpi( mood, arXStatusNames[i] )) {
				JSetByte( hContact, DBSETTING_XSTATUSID, i );
				break;
		}	}

		if ( i == NUM_XMODES )
			JDeleteSetting( hContact, DBSETTING_XSTATUSID );

 		JSetString( hContact, DBSETTING_XSTATUSNAME, mood );
	}
	else {
		JDeleteSetting( hContact, DBSETTING_XSTATUSID );
		JDeleteSetting( hContact, DBSETTING_XSTATUSNAME );
	}

	if ( moodText )
 		JSetStringT( hContact, DBSETTING_XSTATUSMSG, moodText );
	else
		JDeleteSetting( hContact, DBSETTING_XSTATUSMSG );
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberGetXStatus - sets the extended status info (mood)

int JabberGetXStatus( WPARAM wParam, LPARAM lParam )
{
	if ( !jabberOnline || !jabberPepSupported )
		return 0;

	if ( jabberXStatus < 1 || jabberXStatus > NUM_XMODES )
		jabberXStatus = 0;

	if ( wParam ) *(( char** )wParam ) = DBSETTING_XSTATUSNAME;
	if ( lParam ) *(( char** )lParam ) = DBSETTING_XSTATUSMSG;
	return jabberXStatus;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberSetXStatus - sets the extended status info (mood)

int JabberSetXStatus( WPARAM wParam, LPARAM lParam )
{
	if ( jabberPepSupported && wParam >= 0 && wParam <= NUM_XMODES ) {
		jabberXStatus = wParam;
		JSetByte( NULL, DBSETTING_XSTATUSID, wParam );

		if ( jabberOnline ) {
			XmlNodeIq iq( "set", JabberSerialNext() );
			XmlNode* pubsubNode = iq.addChild( "pubsub" );
			pubsubNode->addAttr( "xmlns", JABBER_FEAT_PUBSUB );

			if ( wParam ) {
				XmlNode* publishNode = pubsubNode->addChild( "publish" );
				publishNode->addAttr( "node", JABBER_FEAT_USER_MOOD );
				XmlNode* itemNode = publishNode->addChild( "item" );
				itemNode->addAttr( "id", "current" );
				XmlNode* moodNode = itemNode->addChild( "mood" );
				moodNode->addAttr( "xmlns", JABBER_FEAT_USER_MOOD );
				
				char* mood = NEWSTR_ALLOCA( arXStatusNames[ wParam-1 ] );
				strlwr( mood );
				char* spacePos = strchr(mood, ' ');
				if ( spacePos ) *spacePos = '_';
				moodNode->addChild( mood );
				moodNode->addChild( "text", arXStatusNames[ wParam-1 ] );
			}
			else
			{
				XmlNode* retractNode = pubsubNode->addChild( "retract" );
				retractNode->addAttr( "node", JABBER_FEAT_USER_MOOD );
				retractNode->addAttr( "notify", 1 );
				XmlNode* itemNode = retractNode->addChild( "item" );
				itemNode->addAttr( "id", "current" );
			}

			jabberThreadInfo->send( iq );
		}
		return wParam;
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// menuSetXStatus - a stub for status menus

static int menuSetXStatus(WPARAM wParam,LPARAM lParam,LPARAM param)
{
	JabberSetXStatus( param, 0 );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// builds xstatus menu

int CListMW_BuildStatusItems( WPARAM wParam, LPARAM lParam )
{
	if ( !jabberPepSupported )
		return 0;

	if ( !bXStatusMenuBuilt ) {
		char szFile[MAX_PATH];
		GetModuleFileNameA( hInst, szFile, MAX_PATH );
		char* p = strrchr( szFile, '\\' );
		if ( p != NULL )
			strcpy( p+1, "..\\Icons\\jabber_xstatus.dll" );

		char szSection[ 100 ];
		mir_snprintf( szSection, sizeof( szSection ), "%s/Custom Status", JTranslate( jabberProtoName ));

		SKINICONDESC sid = {0};
		sid.cbSize = sizeof(SKINICONDESC);
		sid.pszDefaultFile = szFile;
		sid.cx = sid.cy = 16;
		sid.pszSection = szSection;

		for ( int i = 0; i < SIZEOF(arXStatusNames); i++ ) {
			char szSettingName[100];
			mir_snprintf( szSettingName, sizeof( szSettingName ), "%s_%s", jabberProtoName, arXStatusNames[i] );
			sid.pszName = szSettingName;
			sid.pszDescription = Translate( arXStatusNames[i] );
			sid.iDefaultIndex = -( i+200 );
			arXStatusIcons[ i ] = ( HANDLE )CallService( MS_SKIN2_ADDICON, 0, ( LPARAM )&sid );
	}	}

	CLISTMENUITEM mi = { 0 };
	int i;
	char srvFce[MAX_PATH + 64];
	char szItem[MAX_PATH + 64];
	HANDLE hXStatusRoot;
	HANDLE hRoot = ( HANDLE )szItem;

	mir_snprintf( szItem, sizeof(szItem), LPGEN("%s Custom Status"), jabberProtoName );
	mi.cbSize = sizeof(mi);
	mi.popupPosition= 500084000;
	mi.position = 2000040000;
	mi.pszContactOwner = jabberProtoName;

	for( i = 0; i <= NUM_XMODES; i++ ) {
		mir_snprintf( srvFce, sizeof(srvFce), "%s/menuXStatus%d", jabberProtoName, i );

		if ( !bXStatusMenuBuilt )
			CreateServiceFunctionParam( srvFce, menuSetXStatus, i );

		if ( i ) {
			char szIcon[ MAX_PATH ];
			mir_snprintf( szIcon, sizeof( szIcon ), "xicon%d", i );
			mi.icolibItem = arXStatusIcons[ i-1 ];
			mi.pszName = ( char* )arXStatusNames[ i-1 ];
		}
		else mi.pszName = "None";

		mi.position++;
		mi.pszPopupName = ( char* )hRoot;
		mi.flags = CMIF_ICONFROMICOLIB + ( jabberXStatus == i ) ? CMIF_CHECKED : 0;
		mi.pszService = srvFce;
		hXStatusItems[ i ] = ( HANDLE )CallService( MS_CLIST_ADDSTATUSMENUITEM, ( WPARAM )&hXStatusRoot, ( LPARAM )&mi );
	}

	bXStatusMenuBuilt = 1;
	return 0;
}

void JabberXStatusInit()
{
	jabberXStatus = JGetByte( NULL, DBSETTING_XSTATUSID, 0 );

	hHookStatusBuild = HookEvent(ME_CLIST_PREBUILDSTATUSMENU, CListMW_BuildStatusItems);

	JCreateServiceFunction( JS_GETXSTATUS, JabberGetXStatus );
	JCreateServiceFunction( JS_SETXSTATUS, JabberSetXStatus );
}

void JabberXStatusUninit()
{
	if ( hHookStatusBuild )
		UnhookEvent( hHookStatusBuild );

	if ( hHookExtraIconsRebuild )
		UnhookEvent( hHookExtraIconsRebuild );

	if ( hHookExtraIconsApply )
		UnhookEvent( hHookExtraIconsApply );
}
