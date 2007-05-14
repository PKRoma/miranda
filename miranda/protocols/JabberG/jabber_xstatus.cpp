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

#include "m_genmenu.h"
#include "m_cluiframes.h"

#define NUM_XMODES 61

static HANDLE hHookExtraIconsRebuild = NULL;
static HANDLE hHookStatusBuild = NULL;
static HANDLE hHookExtraIconsApply = NULL;

static BOOL   bXStatusMenuBuilt = FALSE;
static HANDLE hXStatusItems[ NUM_XMODES+1 ];

const char* szXStatusNames[ NUM_XMODES ] = {
	"Afraid",      "Amazed",      "Angry",     "Annoyed",     "Anxious",      "Aroused",
	"Ashamed",     "Bored",       "Brave",     "Calm",        "Cold",         "Confused",
	"Contented",   "Cranky",      "Curious",   "Depressed",   "Disappointed", "Disgusted",
	"Distracted",  "Embarrassed", "Excited",   "Flirtatious", "Frustrated",   "Grumpy",
	"Guilty",      "Happy",       "Hot",       "Humbled",     "Humiliated",   "Hungry",
	"Hurt",        "Impressed",   "In awe",    "In love",     "Indignant",    "Interested",
	"Intoxicated", "Invincible",  "Jealous",   "Lonely",      "Mean",         "Moody",
	"Nervous",     "Neutral",     "Offended",  "Playful",     "Proud",        "Relieved",
	"Remorseful",  "Restless",    "Sad",       "Sarcastic",   "Serious",      "Shocked",
	"Shy",         "Sick",        "Sleepy",    "Stressed",    "Surprised",    "Thirsty",
	"Worried" };

/////////////////////////////////////////////////////////////////////////////////////////
// XStatus support events

static int menuSetXStatus(WPARAM wParam,LPARAM lParam,LPARAM param)
{
	/*
	XmlNodeIq iq( "set", JabberSerialNext() );
	XmlNode* pubsubNode = iq.addChild( "pubsub" );
	pubsubNode->addAttr( "xmlns", JABBER_FEAT_PUBSUB );
	XmlNode* publishNode = pubsubNode->addChild( "publish" );
	publishNode->addAttr( "node", JABBER_FEAT_USER_MOOD );
	XmlNode* itemNode = publishNode->addChild( "item" );
	itemNode->addAttr( "id", "current" );
	XmlNode* moodNode = itemNode->addChild( "mood" );
	moodNode->addAttr( "xmlns", JABBER_FEAT_USER_MOOD );

	moodNode->addChild( "happy" );
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

	jabberThreadInfo->send( iq ); */
	return 0;
}

int CListMW_BuildStatusItems( WPARAM wParam, LPARAM lParam )
{
	CLISTMENUITEM mi = { 0 };
	int i = 0;
	char srvFce[MAX_PATH + 64];
	char szItem[MAX_PATH + 64];
	HANDLE hXStatusRoot;
	int bXStatus = JGetByte( NULL, DBSETTING_XSTATUSID, 0 );

	mir_snprintf( szItem, sizeof(szItem), "%s Custom Status", jabberProtoName );
	mi.cbSize = sizeof(mi);
	mi.pszPopupName = szItem;
	mi.popupPosition= 500084000;
	mi.position = 2000040000;

	for( i = 0; i <= NUM_XMODES; i++ ) {
		mir_snprintf( srvFce, sizeof(srvFce), "%s/menuXStatus%d", jabberProtoName, i );

		if ( !bXStatusMenuBuilt )
			CreateServiceFunctionParam( srvFce, menuSetXStatus, i );

		if ( i ) {
			char szIcon[ MAX_PATH ];
			mir_snprintf( szIcon, sizeof( szIcon ), "xicon%d", i );
			mi.hIcon = LoadIconEx( szIcon );
		}
		mi.position++;
		mi.flags = bXStatus == i?CMIF_CHECKED:0;
		mi.pszName = ( i ) ? ( char* )szXStatusNames[ i-1 ] : "None";
		mi.pszService = srvFce;
		mi.pszContactOwner = jabberProtoName;

		hXStatusItems[i] = (HANDLE)CallService(MS_CLIST_ADDSTATUSMENUITEM, (WPARAM)&hXStatusRoot, (LPARAM)&mi);
	}

	bXStatusMenuBuilt = 1;
	return 0;
}

int CListMW_ExtraIconsRebuild( WPARAM wParam, LPARAM lParam )
{
	return 0;
}

int CListMW_ExtraIconsApply( WPARAM wParam, LPARAM lParam )
{
	return 0;
}

void JabberXStatusInit()
{
	hHookStatusBuild = HookEvent(ME_CLIST_PREBUILDSTATUSMENU, CListMW_BuildStatusItems);
	hHookExtraIconsRebuild = HookEvent(ME_CLIST_EXTRA_LIST_REBUILD, CListMW_ExtraIconsRebuild);
	hHookExtraIconsApply = HookEvent(ME_CLIST_EXTRA_IMAGE_APPLY, CListMW_ExtraIconsApply);
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
