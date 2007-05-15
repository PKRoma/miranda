/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-07  George Hazan

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
Last change on : $Date: 2007-05-06 02:13:31 +0400 (Вс, 06 май 2007) $
Last change by : $Author: ghazan $

*/

#include "jabber.h"
#include "jabber_caps.h"

#include "m_genmenu.h"
#include "m_cluiframes.h"
#include "m_icolib.h"

#define NUM_XMODES 61

static HANDLE hHookExtraIconsRebuild = NULL;
static HANDLE hHookStatusBuild = NULL;
static HANDLE hHookExtraIconsApply = NULL;

static int jabberXStatus = 0;

static BOOL   bXStatusMenuBuilt = FALSE;
static HANDLE hXStatusItems[ NUM_XMODES+1 ];

const char* arXStatusNames[ NUM_XMODES ] = {
	"Afraid",      "Amazed",      "Angry",     "Annoyed",     "Anxious",      "Aroused",
	"Ashamed",     "Bored",       "Brave",     "Calm",        "Cold",         "Confused",
	"Contented",   "Cranky",      "Curious",   "Depressed",   "Disappointed", "Disgusted",
	"Distracted",  "Embarrassed", "Excited",   "Flirtatious", "Frustrated",   "Grumpy",
	"Guilty",      "Happy",       "Hot",       "Humbled",     "Humiliated",   "Hungry",
	"Hurt",        "Impressed",   "In awe",    "In love",     "Indignant",    "Interested",
	"Intoxicated", "Invincible",  "Jealous",   "Lonely",      "Mean",         "Moody",
	"Nervous",     "Neutral",     "Offended",  "Playful",     "Proud",        "Relieved",
	"Remorseful",  "Restless",    "Sad",       "Sarcastic",   "Serious",      "Shocked ",
	"Shy",         "Sick",        "Sleepy",    "Stressed",    "Surprised",    "Thirsty",
	"Worried"
};	   

static HANDLE arXStatusIcons[ NUM_XMODES ];

/////////////////////////////////////////////////////////////////////////////////////////
// JabberSetContactMood - sets the contact's mood

void JabberSetContactMood( HANDLE hContact, const char* moodType, const TCHAR* moodText )
{
	int i;
	for ( i=0; i < NUM_XMODES; i++ ) {
		if ( !strcmpi( moodType, arXStatusNames[i] )) {
			JSetByte( hContact, DBSETTING_XSTATUSID, i );
			break;
	}	}

	if ( i == NUM_XMODES )
		JDeleteSetting( hContact, DBSETTING_XSTATUSID );

 	if ( moodType )
 		JSetString( hContact, DBSETTING_XSTATUSNAME, moodType );
	else
		JDeleteSetting( hContact, DBSETTING_XSTATUSNAME );

	if ( moodText )
 		JSetStringT( hContact, DBSETTING_XSTATUSMSG, moodText );
	else
		JDeleteSetting( hContact, DBSETTING_XSTATUSMSG );
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberGetXStatus - sets the extended status info (mood)

int JabberGetXStatus( WPARAM wParam, LPARAM lParam )
{
	if ( !jabberOnline )
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
	if ( wParam >= 0 && wParam <= NUM_XMODES ) {
		jabberXStatus = wParam;
		JSetByte( NULL, DBSETTING_XSTATUSID, wParam );

		if ( jabberOnline ) {
			XmlNodeIq iq( "set", JabberSerialNext() );
			XmlNode* pubsubNode = iq.addChild( "pubsub" );
			pubsubNode->addAttr( "xmlns", JABBER_FEAT_PUBSUB );
			XmlNode* publishNode = pubsubNode->addChild( "publish" );
			publishNode->addAttr( "node", JABBER_FEAT_USER_MOOD );
			XmlNode* itemNode = publishNode->addChild( "item" );
			itemNode->addAttr( "id", "current" );
			XmlNode* moodNode = itemNode->addChild( "mood" );
			moodNode->addAttr( "xmlns", JABBER_FEAT_USER_MOOD );

			if ( wParam ) {
				char* mood = NEWSTR_ALLOCA( arXStatusNames[ wParam-1 ] );
				strlwr( mood );
				moodNode->addChild( mood );
			}
			else moodNode->addChild( "" );
			moodNode->addChild( "text", "Miranda can User Moods!" );

			XmlNode* cofigureNode = pubsubNode->addChild( "configure" );
			XmlNode* xNode = cofigureNode->addChild( "x" );
			XmlNode* fieldNode = xNode->addChild( "field" );
			fieldNode->addAttr( "type", "hidden" );
			fieldNode->addAttr( "var", "FORM_TYPE" );
			XmlNode* valueNode = fieldNode->addChild( "value", JABBER_FEAT_PUBSUB_NODE_CONFIG );
			fieldNode = xNode->addChild( "field" );
			fieldNode->addAttr( "var", "pubsub#access_model" );
			valueNode = fieldNode->addChild( "value", "presence" );

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
	CLISTMENUITEM mi = { 0 };
	int i;
	char srvFce[MAX_PATH + 64];
	char szItem[MAX_PATH + 64];
	HANDLE hXStatusRoot;
	HANDLE hRoot = ( HANDLE )szItem;

	mir_snprintf( szItem, sizeof(szItem), "%s Custom Status", jabberProtoName );
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
	}

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
