/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-08  George Hazan
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

File name      : $URL$
Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#include "jabber.h"
#include "jabber_caps.h"

#include <m_genmenu.h>
#include <m_icolib.h>
#include <m_fontservice.h>

#include "sdk/m_cluiframes.h"

#include "sdk/m_proto_listeningto.h"
#include "sdk/m_skin_eng.h"

struct
{
	char	*szName;
	char	*szTag;
} static g_arrMoods[] = 
{
#define _T_STUB_
	{ LPGEN("None"),			NULL					},
	{ LPGEN("Afraid"),			_T_STUB_("afraid")		},
	{ LPGEN("Amazed"),			_T_STUB_("amazed")		},
	{ LPGEN("Angry"),			_T_STUB_("angry")		},
	{ LPGEN("Annoyed"),			_T_STUB_("annoyed")		},
	{ LPGEN("Anxious"),			_T_STUB_("anxious")		},
	{ LPGEN("Aroused"),			_T_STUB_("aroused")		},
	{ LPGEN("Ashamed"),			_T_STUB_("ashamed")		},
	{ LPGEN("Bored"),			_T_STUB_("bored")		},
	{ LPGEN("Brave"),			_T_STUB_("brave")		},
	{ LPGEN("Calm"),			_T_STUB_("calm")		},
	{ LPGEN("Cold"),			_T_STUB_("cold")		},
	{ LPGEN("Confused"),		_T_STUB_("confused")	},
	{ LPGEN("Contented"),		_T_STUB_("contented")	},
	{ LPGEN("Cranky"),			_T_STUB_("cranky")		},
	{ LPGEN("Curious"),			_T_STUB_("curious")		},
	{ LPGEN("Depressed"),		_T_STUB_("depressed")	},
	{ LPGEN("Disappointed"),	_T_STUB_("disappointed")},
	{ LPGEN("Disgusted"),		_T_STUB_("disgusted")	},
	{ LPGEN("Distracted"),		_T_STUB_("distracted")	},
	{ LPGEN("Embarrassed"),		_T_STUB_("embarrassed")	},
	{ LPGEN("Excited"),			_T_STUB_("excited")		},
	{ LPGEN("Flirtatious"),		_T_STUB_("flirtatious")	},
	{ LPGEN("Frustrated"),		_T_STUB_("frustrated")	},
	{ LPGEN("Grumpy"),			_T_STUB_("grumpy")		},
	{ LPGEN("Guilty"),			_T_STUB_("guilty")		},
	{ LPGEN("Happy"),			_T_STUB_("happy")		},
	{ LPGEN("Hot"),				_T_STUB_("hot")			},
	{ LPGEN("Humbled"),			_T_STUB_("humbled")		},
	{ LPGEN("Humiliated"),		_T_STUB_("humiliated")	},
	{ LPGEN("Hungry"),			_T_STUB_("hungry")		},
	{ LPGEN("Hurt"),			_T_STUB_("hurt")		},
	{ LPGEN("Impressed"),		_T_STUB_("impressed")	},
	{ LPGEN("In awe"),			_T_STUB_("in_awe")		},
	{ LPGEN("In love"),			_T_STUB_("in_love")		},
	{ LPGEN("Indignant"),		_T_STUB_("indignant")	},
	{ LPGEN("Interested"),		_T_STUB_("interested")	},
	{ LPGEN("Intoxicated"),		_T_STUB_("intoxicated")	},
	{ LPGEN("Invincible"),		_T_STUB_("invincible")	},
	{ LPGEN("Jealous"),			_T_STUB_("jealous")		},
	{ LPGEN("Lonely"),			_T_STUB_("lonely")		},
	{ LPGEN("Mean"),			_T_STUB_("mean")		},
	{ LPGEN("Moody"),			_T_STUB_("moody")		},
	{ LPGEN("Nervous"),			_T_STUB_("nervous")		},
	{ LPGEN("Neutral"),			_T_STUB_("neutral")		},
	{ LPGEN("Offended"),		_T_STUB_("offended")	},
	{ LPGEN("Playful"),			_T_STUB_("playful")		},
	{ LPGEN("Proud"),			_T_STUB_("proud")		},
	{ LPGEN("Relieved"),		_T_STUB_("relieved")	},
	{ LPGEN("Remorseful"),		_T_STUB_("remorseful")	},
	{ LPGEN("Restless"),		_T_STUB_("restless")	},
	{ LPGEN("Sad"),				_T_STUB_("sad")			},
	{ LPGEN("Sarcastic"),		_T_STUB_("sarcastic")	},
	{ LPGEN("Serious"),			_T_STUB_("serious")		},
	{ LPGEN("Shocked"),			_T_STUB_("shocked")		},
	{ LPGEN("Shy"),				_T_STUB_("shy")			},
	{ LPGEN("Sick"),			_T_STUB_("sick")		},
	{ LPGEN("Sleepy"),			_T_STUB_("sleepy")		},
	{ LPGEN("Stressed"),		_T_STUB_("stressed")	},
	{ LPGEN("Surprised"),		_T_STUB_("surprised")	},
	{ LPGEN("Thirsty"),			_T_STUB_("thirsty")		},
	{ LPGEN("Worried"),			_T_STUB_("worried")		},
#undef _T_STUB_
};

HICON CJabberProto::GetXStatusIcon(int bStatus, UINT flags)
{
	HICON icon = ( HICON )CallService( MS_SKIN2_GETICONBYHANDLE, 0, (LPARAM)m_xstatusIcons[ bStatus ].hIcon);
	return ( flags & LR_SHARED ) ? icon : CopyIcon( icon );
}

void CJabberProto::JabberUpdateContactExtraIcon( HANDLE hContact )
{
	DWORD bXStatus = JGetByte(hContact, DBSETTING_XSTATUSID, 0);
	HICON hIcon;

	if (bXStatus > 0 && bXStatus <= NUM_XMODES && m_xstatusIcons[ bXStatus-1 ].hCListXStatusIcon ) 
		hIcon = (HICON)m_xstatusIcons[ bXStatus-1 ].hCListXStatusIcon;
	else 
		hIcon = (HICON)-1;

	IconExtraColumn iec;
	iec.cbSize = sizeof(iec);
	iec.hImage = hIcon;
	iec.ColumnType = EXTRA_ICON_ADV1;
	CallService( MS_CLIST_EXTRA_SET_ICON, (WPARAM)hContact, (LPARAM)&iec );

	NotifyEventHooks(m_hEventXStatusIconChanged, (WPARAM)hContact, (LPARAM)hIcon);
}

int CJabberProto::CListMW_ExtraIconsRebuild( WPARAM wParam, LPARAM lParam ) 
{
	if ( ServiceExists( MS_CLIST_EXTRA_ADD_ICON ))
		for ( int i = 0; i < NUM_XMODES; i++ ) 
			m_xstatusIcons[i].hCListXStatusIcon = (HANDLE)CallService(MS_CLIST_EXTRA_ADD_ICON, (WPARAM)GetXStatusIcon(i + 1, LR_SHARED), 0);

	return 0;
}

int CJabberProto::CListMW_ExtraIconsApply( WPARAM wParam, LPARAM lParam ) 
{
	if (m_bJabberOnline && m_bPepSupported && ServiceExists(MS_CLIST_EXTRA_SET_ICON)) 
	{
		char* szProto = ( char* )JCallService( MS_PROTO_GETCONTACTBASEPROTO, wParam, 0 );
		if ( szProto==NULL || strcmp( szProto, m_szModuleName ))
			return 0; // only apply icons to our contacts, do not mess others

		JabberUpdateContactExtraIcon((HANDLE)wParam);
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberSetContactMood - sets the contact's mood

void CJabberProto::SetContactMood( HANDLE hContact, const char* moodType, const TCHAR* moodText )
{
	int bXStatus = 0;
	if ( moodType ) {
		TCHAR *moodTitle = NULL;
		char moodIcon[128] = {0};

		for ( bXStatus=1; bXStatus <= NUM_XMODES; bXStatus++ ) {
			if ( !strcmp( moodType, g_arrMoods[bXStatus].szTag )) {
				JSetByte( hContact, DBSETTING_XSTATUSID, bXStatus );
				JSetString( hContact, DBSETTING_XSTATUSNAME, g_arrMoods[bXStatus].szName );

				mir_snprintf( moodIcon, SIZEOF(moodIcon), "%s_%s", m_szModuleName, Translate(g_arrMoods[bXStatus].szName) );
				moodTitle = mir_a2t(g_arrMoods[bXStatus].szName);
				break;
		}	}

		if ( bXStatus > NUM_XMODES ) {
			JDeleteSetting( hContact, DBSETTING_XSTATUSID );
			JSetString( hContact, DBSETTING_XSTATUSNAME, moodType );
		}

		if (!moodTitle)
			moodTitle = mir_a2t(moodType);
		WriteAdvStatus( hContact, ADVSTATUS_MOOD, moodType, moodIcon, moodTitle, moodText );
		mir_free(moodTitle);
	}
	else {
		JDeleteSetting( hContact, DBSETTING_XSTATUSID );
		JDeleteSetting( hContact, DBSETTING_XSTATUSNAME );

		ResetAdvStatus( hContact, ADVSTATUS_MOOD );
	}

	if ( moodText )
		JSetStringT( hContact, DBSETTING_XSTATUSMSG, moodText );
	else
		JDeleteSetting( hContact, DBSETTING_XSTATUSMSG );

	JabberUpdateContactExtraIcon( hContact );
	NotifyEventHooks( m_hEventXStatusChanged, (WPARAM)hContact, 0 );
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberGetXStatus - gets the extended status info (mood)

int __cdecl CJabberProto::OnGetXStatus( WPARAM wParam, LPARAM lParam )
{
	if ( !m_bJabberOnline || !m_bPepSupported )
		return 0;

	if ( m_nJabberXStatus < 1 || m_nJabberXStatus > NUM_XMODES )
		m_nJabberXStatus = 0;

	if ( wParam ) *(( char** )wParam ) = DBSETTING_XSTATUSNAME;
	if ( lParam ) *(( char** )lParam ) = DBSETTING_XSTATUSMSG;
	return m_nJabberXStatus;
}


/////////////////////////////////////////////////////////////////////////////////////////
// JabberGetXStatusIcon - Retrieves specified custom status icon
//wParam = (int)N  // custom status id, 0 = my current custom status
//lParam = flags   // use LR_SHARED for shared HICON
//return = HICON   // custom status icon (use DestroyIcon to release resources if not LR_SHARED)

int __cdecl CJabberProto::OnGetXStatusIcon( WPARAM wParam, LPARAM lParam )
{
	if ( !m_bJabberOnline )
		return 0;

	if ( !wParam )
		wParam = JGetByte( DBSETTING_XSTATUSID, 0 );

	if ( wParam < 1 || wParam > NUM_XMODES || !ServiceExists( MS_SKIN2_GETICONBYHANDLE ))
		return 0;

	if ( lParam & LR_SHARED )
		return (int)GetXStatusIcon( wParam, LR_SHARED );

	return (int)GetXStatusIcon( wParam, 0 );
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberSendPepMood - sends mood

BOOL CJabberProto::SendPepMood( int nMoodNumber, TCHAR* szMoodText )
{
	if ( !m_bJabberOnline || !m_bPepSupported || ( nMoodNumber > NUM_XMODES ))
		return FALSE;

	TCHAR *mood_name = mir_a2t(g_arrMoods[nMoodNumber].szName);
	m_pInfoFrame->UpdateInfoItem("$/PEP/mood",
		nMoodNumber ? m_xstatusIcons[nMoodNumber].hIcon : LoadSkinnedIconHandle(SKINICON_OTHER_SMALLDOT),
		TranslateTS(mood_name));
	mir_free(mood_name);

	XmlNodeIq iq( "set", SerialNext() );
	XmlNode* pubsubNode = iq.addChild( "pubsub" );
	pubsubNode->addAttr( "xmlns", JABBER_FEAT_PUBSUB );

	if ( nMoodNumber ) {
		XmlNode* publishNode = pubsubNode->addChild( "publish" );
		publishNode->addAttr( "node", JABBER_FEAT_USER_MOOD );
		XmlNode* itemNode = publishNode->addChild( "item" );
		itemNode->addAttr( "id", "current" );
		XmlNode* moodNode = itemNode->addChild( "mood" );
		moodNode->addAttr( "xmlns", JABBER_FEAT_USER_MOOD );
		moodNode->addChild( g_arrMoods[ nMoodNumber ].szTag );
		if ( szMoodText )
			moodNode->addChild( "text", szMoodText );
	}
	else {
		XmlNode* retractNode = pubsubNode->addChild( "retract" );
		retractNode->addAttr( "node", JABBER_FEAT_USER_MOOD );
		retractNode->addAttr( "notify", 1 );
		XmlNode* itemNode = retractNode->addChild( "item" );
		itemNode->addAttr( "id", "current" );
	}
	m_ThreadInfo->send( iq );
	
	return TRUE;
}

BOOL CJabberProto::SendPepTune( TCHAR* szArtist, TCHAR* szLength, TCHAR* szSource, TCHAR* szTitle, TCHAR* szTrack, TCHAR* szUri )
{
	if ( !m_bJabberOnline || !m_bPepSupported )
		return FALSE;

	XmlNodeIq iq( "set", SerialNext() );
	XmlNode* pubsubNode = iq.addChild( "pubsub" );
	pubsubNode->addAttr( "xmlns", JABBER_FEAT_PUBSUB );

	if ( !szArtist && !szLength && !szSource && !szTitle && !szUri ) {
		XmlNode* retractNode = pubsubNode->addChild( "retract" );
		retractNode->addAttr( "node", JABBER_FEAT_USER_TUNE );
		retractNode->addAttr( "notify", 1 );
		XmlNode* itemNode = retractNode->addChild( "item" );
		itemNode->addAttr( "id", "current" );
	}
	else {
		XmlNode* publishNode = pubsubNode->addChild( "publish" );
		publishNode->addAttr( "node", JABBER_FEAT_USER_TUNE );
		XmlNode* itemNode = publishNode->addChild( "item" );
		itemNode->addAttr( "id", "current" );
		XmlNode* tuneNode = itemNode->addChild( "tune" );
		tuneNode->addAttr( "xmlns", JABBER_FEAT_USER_TUNE );
		if ( szArtist ) tuneNode->addChild( "artist", szArtist );
		if ( szLength ) tuneNode->addChild( "length", szLength );
		if ( szSource ) tuneNode->addChild( "source", szSource );
		if ( szTitle ) tuneNode->addChild( "title", szTitle );
		if ( szTrack ) tuneNode->addChild( "track", szTrack );
		if ( szUri ) tuneNode->addChild( "uri", szUri );
	}
	m_ThreadInfo->send( iq );

	return TRUE;
}

void CJabberProto::SetContactTune( HANDLE hContact,  TCHAR* szArtist, TCHAR* szLength, TCHAR* szSource, TCHAR* szTitle, TCHAR* szTrack, TCHAR* szUri )
{
	if ( !szArtist && !szTitle ) {
		JDeleteSetting( hContact, "ListeningTo" );
		ResetAdvStatus( hContact, ADVSTATUS_TUNE );
		return;
	}

	TCHAR *szListeningTo;
	if ( ServiceExists( MS_LISTENINGTO_GETPARSEDTEXT )) {
		LISTENINGTOINFO li;
		ZeroMemory( &li, sizeof( li ));
		li.cbSize = sizeof( li );
		li.dwFlags = LTI_TCHAR;
		li.ptszArtist = szArtist;
		li.ptszLength = szLength;
		li.ptszAlbum = szSource;
		li.ptszTitle = szTitle;
		li.ptszTrack = szTrack;
		szListeningTo = (TCHAR *)CallService( MS_LISTENINGTO_GETPARSEDTEXT, (WPARAM)_T("%title% - %artist%"), (LPARAM)&li );
	}
	else {
		szListeningTo = (TCHAR *) mir_alloc( 2048 * sizeof( TCHAR ));
		mir_sntprintf( szListeningTo, 2047, _T("%s - %s"), szTitle ? szTitle : _T(""), szArtist ? szArtist : _T("") );
	}

	JSetStringT( hContact, "ListeningTo", szListeningTo );

	char tuneIcon[128];
	mir_snprintf(tuneIcon, SIZEOF(tuneIcon), "%s_%s", m_szModuleName, "main");
	WriteAdvStatus( hContact, ADVSTATUS_TUNE, "listening_to", tuneIcon, TranslateT("Listening To"), szListeningTo );

	mir_free( szListeningTo );
}

TCHAR* a2tf( const TCHAR* str, BOOL unicode )
{
	if ( str == NULL )
		return NULL;

	#if defined( _UNICODE )
		return ( unicode ) ? mir_tstrdup( str ) : mir_a2t(( char* )str );
	#else
		return mir_strdup( str );
	#endif
}

void overrideStr( TCHAR*& dest, const TCHAR* src, BOOL unicode, const TCHAR* def = NULL )
{
	if ( dest != NULL )
	{
		mir_free( dest );
		dest = NULL;
	}

	if ( src != NULL )
		dest = a2tf( src, unicode );
	else if ( def != NULL )
		dest = mir_tstrdup( def );
}

int __cdecl CJabberProto::OnSetListeningTo( WPARAM wParam, LPARAM lParam )
{
	LISTENINGTOINFO *cm = (LISTENINGTOINFO *)lParam;
	if ( !cm || cm->cbSize != sizeof(LISTENINGTOINFO) ) {
		SendPepTune( NULL, NULL, NULL, NULL, NULL, NULL );
		JDeleteSetting( NULL, "ListeningTo" );
	}
	else {
		TCHAR *szArtist = NULL, *szLength = NULL, *szSource = NULL;
		TCHAR *szTitle = NULL, *szTrack = NULL;

		BOOL unicode = cm->dwFlags & LTI_UNICODE;

		overrideStr( szArtist, cm->ptszArtist, unicode );
		overrideStr( szSource, cm->ptszAlbum, unicode );
		overrideStr( szTitle, cm->ptszTitle, unicode );
		overrideStr( szTrack, cm->ptszTrack, unicode );
		overrideStr( szLength, cm->ptszLength, unicode );

		TCHAR szLengthInSec[ 32 ];
		szLengthInSec[ 0 ] = _T('\0');
		if ( szLength ) {
			unsigned int multiplier = 1, result = 0;
			for ( TCHAR *p = szLength; *p; p++ ) {
				if ( *p == _T(':')) multiplier *= 60;
			}
			if ( multiplier <= 3600 ) {
				TCHAR *szTmp = szLength;
				while ( szTmp[0] ) {
					result += ( _ttoi( szTmp ) * multiplier );
					multiplier /= 60;
					szTmp = _tcschr( szTmp, _T(':') );
					if ( !szTmp )
						break;
					szTmp++;
				}
			}
			mir_sntprintf( szLengthInSec, SIZEOF( szLengthInSec ), _T("%d"), result );
		}

		SendPepTune( szArtist, szLength ? szLengthInSec : NULL, szSource, szTitle, szTrack, NULL );
		SetContactTune( NULL, szArtist, szLength, szSource, szTitle, szTrack, NULL );
		
		mir_free( szArtist );
		mir_free( szLength );
		mir_free( szSource );
		mir_free( szTitle );
		mir_free( szTrack );
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// menuSetXStatus - a stub for status menus

int __cdecl CJabberProto::OnMenuSetXStatus( WPARAM wParam, LPARAM lParam, LPARAM param )
{
	OnSetXStatus( param, 0 );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// process InfoFrame click

void CJabberProto::InfoFrame_OnUserMood(CJabberInfoFrame_Event *evt)
{
	HMENU hMenu = JMenuCreate(true);
	for( int i = 0; i <= NUM_XMODES; i++ ) {
		TCHAR *mood_name = mir_a2t(g_arrMoods[i].szName);
		JMenuAddItem(hMenu, i+1,
			i ? TranslateTS(mood_name) : TranslateT("None"),
			i ? m_xstatusIcons[i].hIcon : LoadSkinnedIconHandle(SKINICON_OTHER_SMALLDOT), true,
			i == JGetByte(DBSETTING_XSTATUSID, 0));
		mir_free(mood_name);
	}
	int res = JMenuShow(hMenu);
	if (res) OnSetXStatus(res-1, 0);
	JMenuDestroy(hMenu, NULL, NULL);
}

/////////////////////////////////////////////////////////////////////////////////////////
// builds xstatus menu

void CJabberProto::BuildXStatusItems( WPARAM wParam, LPARAM lParam )
{
	if ( !m_bJabberOnline || !m_bPepSupported )
		return;

	CLISTMENUITEM mi = { 0 };
	int i;
	char srvFce[MAX_PATH + 64], *svcName = srvFce+strlen( m_szModuleName );
	char szItem[MAX_PATH + 64];
	HANDLE hXStatusRoot;
	HANDLE hRoot = ( HANDLE )szItem;

	mir_snprintf( szItem, sizeof(szItem), LPGEN("%s Custom Status"), m_szModuleName );
	mi.cbSize = sizeof(mi);
	mi.popupPosition= 500084000;
	mi.position = 2000040000;
	mi.pszContactOwner = m_szModuleName;

	for( i = 0; i <= NUM_XMODES; i++ ) {
		mir_snprintf( srvFce, sizeof(srvFce), "%s/menuXStatus%d", m_szModuleName, i );

		if ( !m_bXStatusMenuBuilt )
			JCreateServiceParam( svcName, &CJabberProto::OnMenuSetXStatus, i );

		if ( i ) {
			char szIcon[ MAX_PATH ];
			mir_snprintf( szIcon, sizeof( szIcon ), "xicon%d", i );
			mi.icolibItem = m_xstatusIcons[ i ].hIcon;
			mi.pszName = ( char* )g_arrMoods[ i ].szName;
		}
		else {
			mi.pszName = "None";
			mi.icolibItem = LoadSkinnedIconHandle(SKINICON_OTHER_SMALLDOT);
		}

		mi.position++;
		mi.pszPopupName = ( char* )hRoot;
		mi.flags = CMIF_ICONFROMICOLIB + (( m_nJabberXStatus == i ) ? CMIF_CHECKED : 0 );
		mi.pszService = srvFce;
		m_xstatusIcons[ i ].hItem = ( HANDLE )CallService( MS_CLIST_ADDSTATUSMENUITEM, ( WPARAM )&hXStatusRoot, ( LPARAM )&mi );
	}

	m_bXStatusMenuBuilt = 1;
	return;
}

void CJabberProto::InitXStatusIcons()
{
	char szFile[MAX_PATH];
	GetModuleFileNameA( hInst, szFile, MAX_PATH );
	char* p = strrchr( szFile, '\\' );
	if ( p != NULL )
		strcpy( p+1, "..\\Icons\\jabber_xstatus.dll" );

	TCHAR szSection[ 100 ];
	TCHAR szDescription[ 100 ];

	mir_sntprintf( szSection, SIZEOF( szSection ), _T("Status Icons/%s/Moods"), m_tszUserName);

	SKINICONDESC sid = {0};
	sid.cbSize = sizeof(SKINICONDESC);
	sid.pszDefaultFile = szFile;
	sid.cx = sid.cy = 16;
	sid.ptszSection = szSection;
	sid.ptszDescription = szDescription;
	sid.flags = SIDF_TCHAR;

	for ( int i = 1; i < SIZEOF(g_arrMoods); i++ ) {
		char szSettingName[100];
		mir_snprintf( szSettingName, SIZEOF( szSettingName ), "%s_%s", m_szModuleName, g_arrMoods[i].szName );
		sid.pszName = szSettingName;
		mir_sntprintf(szDescription, SIZEOF(szDescription), _T(TCHAR_STR_PARAM), g_arrMoods[i].szName);
		sid.iDefaultIndex = -( i+200 );
		m_xstatusIcons[ i ].hIcon = ( HANDLE )CallService( MS_SKIN2_ADDICON, 0, ( LPARAM )&sid );
	}
}

void CJabberProto::XStatusInit()
{
	m_nJabberXStatus = JGetByte( NULL, DBSETTING_XSTATUSID, 0 );

	InitXStatusIcons();

	JHookEvent( ME_CLIST_EXTRA_LIST_REBUILD, &CJabberProto::CListMW_ExtraIconsRebuild );
	JHookEvent( ME_CLIST_EXTRA_IMAGE_APPLY,  &CJabberProto::CListMW_ExtraIconsApply );

	RegisterAdvStatusSlot( ADVSTATUS_MOOD );
	RegisterAdvStatusSlot( ADVSTATUS_TUNE );
}

void CJabberProto::XStatusUninit()
{
	if ( m_hHookStatusBuild )
		UnhookEvent( m_hHookStatusBuild );

	if ( m_hHookExtraIconsRebuild )
		UnhookEvent( m_hHookExtraIconsRebuild );

	if ( m_hHookExtraIconsApply )
		UnhookEvent( m_hHookExtraIconsApply );
}


static WNDPROC OldMessageEditProc;

static LRESULT CALLBACK MessageEditSubclassProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch(msg) {
		case WM_CHAR:
			if(wParam=='\n' && GetKeyState(VK_CONTROL)&0x8000) {
				PostMessage(GetParent(hwnd),WM_COMMAND,IDOK,0);
				return 0;
			}
			if (wParam == 1 && GetKeyState(VK_CONTROL) & 0x8000) {      //ctrl-a
				SendMessage(hwnd, EM_SETSEL, 0, -1);
				return 0;
			}
			if (wParam == 23 && GetKeyState(VK_CONTROL) & 0x8000) {     // ctrl-w
				SendMessage(GetParent(hwnd), WM_CLOSE, 0, 0);
				return 0;
			}
			if (wParam == 127 && GetKeyState(VK_CONTROL) & 0x8000) {    //ctrl-backspace
				DWORD start, end;
				TCHAR *text;
				int textLen;
				SendMessage(hwnd, EM_GETSEL, (WPARAM) & end, (LPARAM) (PDWORD) NULL);
				SendMessage(hwnd, WM_KEYDOWN, VK_LEFT, 0);
				SendMessage(hwnd, EM_GETSEL, (WPARAM) & start, (LPARAM) (PDWORD) NULL);
				textLen = GetWindowTextLength(hwnd);
				text = (TCHAR *) mir_alloc(sizeof(TCHAR) * (textLen + 1));
				GetWindowText(hwnd, text, textLen + 1);
				MoveMemory(text + start, text + end, sizeof(TCHAR) * (textLen + 1 - end));
				SetWindowText(hwnd, text);
				mir_free(text);
				SendMessage(hwnd, EM_SETSEL, start, start);
				SendMessage( GetParent(hwnd), WM_COMMAND, MAKEWPARAM( GetDlgCtrlID(hwnd), EN_CHANGE ), (LPARAM) hwnd);
				return 0;
			}
			break;
	}
	return CallWindowProc(OldMessageEditProc,hwnd,msg,wParam,lParam);
}

TCHAR gszOkBuffonFormat[ 1024 ];
int gnCountdown = 5;
HANDLE ghPreshutdown = NULL;
#define DM_MOOD_SHUTDOWN WM_USER+10

struct SetMoodMsgDlgProcParam
{
	int mood;
	CJabberProto* ppro;
};

static BOOL CALLBACK SetMoodMsgDlgProc( HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch ( message ) {
	case WM_INITDIALOG:
		{
			SetMoodMsgDlgProcParam* param = ( SetMoodMsgDlgProcParam* )lParam;
			if ( param->mood == 0 || param->mood > NUM_XMODES )
				param->mood = 1;

			SetWindowLong( hwndDlg, GWL_USERDATA, lParam );
			
			TranslateDialogDefault(hwndDlg);

			SendDlgItemMessage( hwndDlg, IDC_MSG_MOOD, EM_LIMITTEXT, 1024, 0 );

			OldMessageEditProc = (WNDPROC)SetWindowLong( GetDlgItem( hwndDlg, IDC_MSG_MOOD ), GWL_WNDPROC, (LONG)MessageEditSubclassProc );

			TCHAR str[256], format[128];
			GetWindowText( hwndDlg, format, SIZEOF( format ));
			TCHAR* szMood = mir_a2t( Translate(g_arrMoods[ param->mood ].szName));
			mir_sntprintf( str, SIZEOF(str), format, szMood );
			mir_free( szMood );
			SetWindowText( hwndDlg, str );

			GetDlgItemText( hwndDlg, IDOK, gszOkBuffonFormat, SIZEOF(gszOkBuffonFormat));

			char szSetting[64];
			sprintf(szSetting, "XStatus%dMsg", param->mood);
			DBVARIANT dbv;
			if ( !param->ppro->JGetStringT( NULL, szSetting, &dbv )) {
				SetDlgItemText( hwndDlg, IDC_MSG_MOOD, dbv.ptszVal );
				JFreeVariant( &dbv );
			}
			else SetDlgItemTextA( hwndDlg, IDC_MSG_MOOD, g_arrMoods[ param->mood ].szName );

			gnCountdown = 5;
			SendMessage( hwndDlg, WM_TIMER, 0, 0 );
			SetTimer( hwndDlg, 1, 1000, 0 );
			ghPreshutdown = HookEventMessage( ME_SYSTEM_PRESHUTDOWN, hwndDlg, DM_MOOD_SHUTDOWN );
		}
		return TRUE;

	case WM_TIMER:
		if ( gnCountdown == -1 )
			DestroyWindow( hwndDlg );
		else {
			TCHAR str[ 512 ];
			mir_sntprintf( str, SIZEOF(str), gszOkBuffonFormat, gnCountdown );
			SetDlgItemText( hwndDlg, IDOK, str );
			gnCountdown--;
		}
		break;

	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDOK:
		case IDCANCEL:
			DestroyWindow( hwndDlg );
			break;
		case IDC_MSG_MOOD:
			KillTimer( hwndDlg, 1 );
			SetDlgItemText( hwndDlg, IDOK, TranslateT( "OK" ));
			break;
		}
		break;

	case DM_MOOD_SHUTDOWN:
		DestroyWindow( hwndDlg );
		break;

	case WM_DESTROY:
		{
			TCHAR szMoodText[ 1024 ];
			szMoodText[ 0 ] = 0;
			GetDlgItemText( hwndDlg, IDC_MSG_MOOD, szMoodText, SIZEOF( szMoodText ));
			SetWindowLong( GetDlgItem( hwndDlg, IDC_MSG_MOOD ), GWL_WNDPROC, (LONG)OldMessageEditProc );

			SetMoodMsgDlgProcParam* param = ( SetMoodMsgDlgProcParam* )GetWindowLong( hwndDlg, GWL_USERDATA );

			int nStatus = param->mood;
			if ( !nStatus || nStatus > NUM_XMODES )
				nStatus = 1;
			
			param->ppro->SendPepMood( nStatus, szMoodText );

			param->ppro->JSetByte( NULL, DBSETTING_XSTATUSID, nStatus );
			param->ppro->JSetStringT( NULL, DBSETTING_XSTATUSMSG, szMoodText );
			param->ppro->JSetString( NULL, DBSETTING_XSTATUSNAME, g_arrMoods[ nStatus ].szName );

			char szSetting[64];
			sprintf(szSetting, "XStatus%dMsg", nStatus);
			param->ppro->JSetStringT(NULL, szSetting, szMoodText);
			delete param;
		}
		break;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberSetXStatus - sets the extended status info (mood)

int __cdecl CJabberProto::OnSetXStatus( WPARAM wParam, LPARAM lParam )
{
	if ( !m_bPepSupported || !m_bJabberOnline || ( wParam > NUM_XMODES ))
		return 0;

	if ( !wParam ) {
		SendPepMood( 0, NULL );
		JDeleteSetting( NULL, DBSETTING_XSTATUSMSG );
		JDeleteSetting( NULL, DBSETTING_XSTATUSNAME );
		JDeleteSetting( NULL, DBSETTING_XSTATUSID );
	}
	else {
		SetMoodMsgDlgProcParam* param = new SetMoodMsgDlgProcParam;
		param->mood = wParam;
		param->ppro = this;
		CreateDialogParam( hInst, MAKEINTRESOURCE(IDD_SETMOODMSG), NULL, SetMoodMsgDlgProc, ( LPARAM )param );
	}

	m_nJabberXStatus = wParam;
	return wParam;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Advanced status slots
// DB data format:
//     Contact / AdvStatus / Proto/Status/id = mode_id
//     Contact / AdvStatus / Proto/Status/icon = icon
//     Contact / AdvStatus / Proto/Status/title = title
//     Contact / AdvStatus / Proto/Status/text = title

void CJabberProto::RegisterAdvStatusSlot(const char *pszSlot)
{
/*
	char szSetting[256];
	mir_snprintf(szSetting, SIZEOF(szSetting), "AdvStatus/%s/%s/id", m_szModuleName, pszSlot);
	CallService(MS_DB_SETSETTINGRESIDENT, TRUE, (LPARAM)szSetting);
	mir_snprintf(szSetting, SIZEOF(szSetting), "AdvStatus/%s/%s/icon", m_szModuleName, pszSlot);
	CallService(MS_DB_SETSETTINGRESIDENT, TRUE, (LPARAM)szSetting);
	mir_snprintf(szSetting, SIZEOF(szSetting), "AdvStatus/%s/%s/title", m_szModuleName, pszSlot);
	CallService(MS_DB_SETSETTINGRESIDENT, TRUE, (LPARAM)szSetting);
	mir_snprintf(szSetting, SIZEOF(szSetting), "AdvStatus/%s/%s/text", m_szModuleName, pszSlot);
	CallService(MS_DB_SETSETTINGRESIDENT, TRUE, (LPARAM)szSetting);
*/
}

void CJabberProto::ResetAdvStatus(HANDLE hContact, const char *pszSlot)
{
	char szSetting[128];
	mir_snprintf(szSetting, SIZEOF(szSetting), "%s/%s/id", m_szModuleName, pszSlot);
	DBDeleteContactSetting(hContact, "AdvStatus", szSetting);
	mir_snprintf(szSetting, SIZEOF(szSetting), "%s/%s/icon", m_szModuleName, pszSlot);
	DBDeleteContactSetting(hContact, "AdvStatus", szSetting);
	mir_snprintf(szSetting, SIZEOF(szSetting), "%s/%s/title", m_szModuleName, pszSlot);
	DBDeleteContactSetting(hContact, "AdvStatus", szSetting);
	mir_snprintf(szSetting, SIZEOF(szSetting), "%s/%s/text", m_szModuleName, pszSlot);
	DBDeleteContactSetting(hContact, "AdvStatus", szSetting);
}

void CJabberProto::WriteAdvStatus(HANDLE hContact, const char *pszSlot, const char *pszMode, const char *pszIcon, const TCHAR *pszTitle, const TCHAR *pszText)
{
	char szSetting[128];

	mir_snprintf(szSetting, SIZEOF(szSetting), "%s/%s/id", m_szModuleName, pszSlot);
	DBWriteContactSettingString(hContact, "AdvStatus", szSetting, pszMode);

	mir_snprintf(szSetting, SIZEOF(szSetting), "%s/%s/icon", m_szModuleName, pszSlot);
	DBWriteContactSettingString(hContact, "AdvStatus", szSetting, pszIcon);

	mir_snprintf(szSetting, SIZEOF(szSetting), "%s/%s/title", m_szModuleName, pszSlot);
	DBWriteContactSettingTString(hContact, "AdvStatus", szSetting, pszTitle);

	mir_snprintf(szSetting, SIZEOF(szSetting), "%s/%s/text", m_szModuleName, pszSlot);
	if (pszText)
		DBWriteContactSettingTString(hContact, "AdvStatus", szSetting, pszText);
	else
		DBDeleteContactSetting(hContact, "AdvStatus", szSetting);
}

/////////////////////////////////////////////////////////////////////////////////////////
// CJabberInfoFrame

class CJabberInfoFrameItem
{
public:
	char *m_pszName;
	HANDLE m_hIcolibIcon;
	TCHAR *m_pszText;
	LPARAM m_pUserData;
	bool m_bCompact;
	bool m_bShow;
	void (CJabberProto::*m_onEvent)(CJabberInfoFrame_Event *);
	RECT m_rcItem;
	int m_tooltipId;

public:
/*
	CJabberInfoFrameItem():
		m_pszName(NULL), m_hIcolibIcon(NULL), m_pszText(NULL)
	{
	}
*/
	CJabberInfoFrameItem(char *pszName, bool bCompact=false, LPARAM pUserData=0):
		m_pszName(NULL), m_hIcolibIcon(NULL), m_pszText(NULL), m_bShow(true), m_bCompact(bCompact), m_pUserData(pUserData), m_onEvent(NULL)
	{
		m_pszName = mir_strdup(pszName);
	}
	~CJabberInfoFrameItem()
	{
		mir_free(m_pszName);
		mir_free(m_pszText);
	}

	void SetInfo(HANDLE hIcolibIcon, TCHAR *pszText)
	{
		mir_free(m_pszText);
		m_pszText = pszText ? mir_tstrdup(pszText) : NULL;
		m_hIcolibIcon = hIcolibIcon;
	}

	static int cmp(const CJabberInfoFrameItem *p1, const CJabberInfoFrameItem *p2)
	{
		return lstrcmpA(p1->m_pszName, p2->m_pszName);
	}
};

CJabberInfoFrame::CJabberInfoFrame(CJabberProto *proto):
	m_pItems(3, CJabberInfoFrameItem::cmp), m_compact(false)
{
	m_proto = proto;
	m_hwnd = m_hwndToolTip = NULL;
	m_clickedItem = -1;
	m_hiddenItemCount = 0;
	m_bLocked = false;
	m_nextTooltipId = 0;
	m_hhkFontsChanged = 0;

	if (ServiceExists(MS_CLIST_FRAMES_ADDFRAME))
	{
		InitClass();

		CLISTFrame frame = {0};
		frame.cbSize = sizeof(frame);
		HWND hwndClist = (HWND)CallService(MS_CLUI_GETHWND, 0, 0);
		frame.hWnd = CreateWindowEx(0, _T("JabberInfoFrameClass"), NULL, WS_CHILD|WS_VISIBLE, 0, 0, 100, 100, hwndClist, NULL, hInst, this);
		int e = GetLastError();
		frame.hIcon = NULL;
		frame.align = alBottom;
		frame.height = 2 * SZ_FRAMEPADDING + GetSystemMetrics(SM_CYSMICON) + SZ_LINEPADDING; // compact height by default
		frame.Flags = F_VISIBLE|F_LOCKED|F_NOBORDER|F_TCHAR;
		frame.tname = mir_a2t(proto->m_szModuleName);
		frame.TBtname = proto->m_tszUserName;
		m_frameId = CallService(MS_CLIST_FRAMES_ADDFRAME, (WPARAM)&frame, 0);
		mir_free(frame.tname);

		m_hhkFontsChanged = HookEventMessage(ME_FONT_RELOAD, m_hwnd, WM_APP);
		ReloadFonts();

		m_hwndToolTip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
			WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			m_hwnd, NULL, hInst, NULL);
		SetWindowPos(m_hwndToolTip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

		CreateInfoItem("$", true);
		UpdateInfoItem("$", proto->GetIconHandle(IDI_JABBER), proto->m_tszUserName);

		CreateInfoItem("$/JID", true);
		UpdateInfoItem("$/JID", LoadSkinnedIconHandle(SKINICON_OTHER_USERDETAILS), _T("Offline"));
		SetInfoItemCallback("$/JID", &CJabberProto::InfoFrame_OnSetup);
	}
}

CJabberInfoFrame::~CJabberInfoFrame()
{
	if (!m_hwnd) return;

	if (m_hhkFontsChanged) UnhookEvent(m_hhkFontsChanged);
	CallService(MS_CLIST_FRAMES_REMOVEFRAME, (WPARAM)m_frameId, 0);
	SetWindowLong(m_hwnd, GWL_USERDATA, 0);
	DestroyWindow(m_hwnd);
	DestroyWindow(m_hwndToolTip);
	DeleteObject(m_hfntText);
	DeleteObject(m_hfntTitle);
	m_hwnd = NULL;
}

void CJabberInfoFrame::InitClass()
{
	static bool bClassRegistered = false;
	if (bClassRegistered) return;

	WNDCLASSEX wcx = {0};
	wcx.cbSize = sizeof(wcx);
	wcx.style = CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW;
	wcx.lpfnWndProc = GlobalWndProc;
	wcx.hInstance = hInst;
	wcx.lpszClassName = _T("JabberInfoFrameClass");
	RegisterClassEx(&wcx);
	bClassRegistered = true;
}

LRESULT CALLBACK CJabberInfoFrame::GlobalWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CJabberInfoFrame *pFrame;

	if (msg == WM_CREATE)
	{
		CREATESTRUCT *pcs = (CREATESTRUCT *)lParam;
		pFrame = (CJabberInfoFrame *)pcs->lpCreateParams;
		if (pFrame) pFrame->m_hwnd = hwnd;
		SetWindowLong(hwnd, GWL_USERDATA, (LONG)pFrame);
	} else
	{
		pFrame = (CJabberInfoFrame *)GetWindowLong(hwnd, GWL_USERDATA);
	}

	return pFrame ? pFrame->WndProc(msg, wParam, lParam) : DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CJabberInfoFrame::WndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_APP:
		{
			ReloadFonts();
			return 0;
		}

		case WM_PAINT:
		{
			RECT rc; GetClientRect(m_hwnd, &rc);
			m_compact = rc.bottom < (2 * (GetSystemMetrics(SM_CYSMICON) + SZ_LINEPADDING) + SZ_LINESPACING + 2 * SZ_FRAMEPADDING);

			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(m_hwnd, &ps);
			m_compact ? PaintCompact(hdc) : PaintNormal(hdc);
			EndPaint(m_hwnd, &ps);
			return 0;
		}

		case WM_RBUTTONUP:
		{
			POINT pt = { LOWORD(lParam), HIWORD(lParam) };
			MapWindowPoints(m_hwnd, NULL, &pt, 1);
			HMENU hMenu = (HMENU)CallService(MS_CLIST_MENUBUILDFRAMECONTEXT, m_frameId, 0);
			int res = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, (HWND)CallService(MS_CLUI_GETHWND, 0, 0), NULL);
			CallService(MS_CLIST_MENUPROCESSCOMMAND, MAKEWPARAM(res, 0), m_frameId);
			return 0;
		}

		case WM_LBUTTONDOWN:
		{
			POINT pt = { LOWORD(lParam), HIWORD(lParam) };
			for (int i = 0; i < m_pItems.getCount(); ++i)
				if (m_pItems[i].m_onEvent && PtInRect(&m_pItems[i].m_rcItem, pt))
				{
					m_clickedItem = i;
					return 0;
				}

			return 0;
		}

		case WM_LBUTTONUP:
		{
			POINT pt = { LOWORD(lParam), HIWORD(lParam) };
			if ((m_clickedItem >= 0) && (m_clickedItem < m_pItems.getCount()) && m_pItems[m_clickedItem].m_onEvent && PtInRect(&m_pItems[m_clickedItem].m_rcItem, pt))
			{
				CJabberInfoFrame_Event evt;
				evt.m_event = CJabberInfoFrame_Event::CLICK;
				evt.m_pszName = m_pItems[m_clickedItem].m_pszName;
				evt.m_pUserData = m_pItems[m_clickedItem].m_pUserData;
				(m_proto->*m_pItems[m_clickedItem].m_onEvent)(&evt);
				return 0;
			}

			m_clickedItem = -1;

			return 0;
		}

		case WM_LBUTTONDBLCLK:
		{
			m_compact = !m_compact;
			UpdateSize();
			return 0;
		}
	}

	return DefWindowProc(m_hwnd, msg, wParam, lParam);
}

void CJabberInfoFrame::LockUpdates()
{
	m_bLocked = true;
}

void CJabberInfoFrame::Update()
{
	m_bLocked = false;
	UpdateSize();
}

void CJabberInfoFrame::ReloadFonts()
{
	LOGFONT lfFont;

	FontID fontid = {0};
	fontid.cbSize = sizeof(fontid);
	lstrcpyA(fontid.group, "Jabber");
	lstrcpyA(fontid.name, "Frame title");
	m_clTitle = CallService(MS_FONT_GET, (WPARAM)&fontid, (LPARAM)&lfFont);
	m_hfntTitle = CreateFontIndirect(&lfFont);
	lstrcpyA(fontid.name, "Frame text");
	m_clText = CallService(MS_FONT_GET, (WPARAM)&fontid, (LPARAM)&lfFont);
	m_hfntText = CreateFontIndirect(&lfFont);

	ColourID colourid = {0};
	colourid.cbSize = sizeof(colourid);
	lstrcpyA(colourid.group, "Jabber");
	lstrcpyA(colourid.name, "Background");
	m_clBack = CallService(MS_COLOUR_GET, (WPARAM)&colourid, 0);

	UpdateSize();
}

void CJabberInfoFrame::UpdateSize()
{
	if (!m_hwnd) return;
	if (m_bLocked) return;

	int line_count = m_compact ? 1 : (m_pItems.getCount() - m_hiddenItemCount);
	int height = 2 * SZ_FRAMEPADDING + line_count * (GetSystemMetrics(SM_CYSMICON) + SZ_LINEPADDING) + (line_count - 1) * SZ_LINESPACING;

	if (CallService(MS_CLIST_FRAMES_GETFRAMEOPTIONS, MAKEWPARAM(FO_FLAGS, m_frameId), 0) & F_VISIBLE)
	{
		if (!ServiceExists(MS_SKIN_DRAWGLYPH))
		{
			// crazy resizing for clist_nicer...
			CallService(MS_CLIST_FRAMES_SHFRAME, m_frameId, 0);
			CallService(MS_CLIST_FRAMES_SETFRAMEOPTIONS, MAKEWPARAM(FO_HEIGHT, m_frameId), height);
			CallService(MS_CLIST_FRAMES_SHFRAME, m_frameId, 0);
		} else
		{
			CallService(MS_CLIST_FRAMES_SETFRAMEOPTIONS, MAKEWPARAM(FO_HEIGHT, m_frameId), height);
			RedrawWindow(m_hwnd, NULL, NULL, RDW_INVALIDATE);
		}
	} else
	{
		CallService(MS_CLIST_FRAMES_SETFRAMEOPTIONS, MAKEWPARAM(FO_HEIGHT, m_frameId), height);
	}
}

void CJabberInfoFrame::RemoveTooltip(int id)
{
	TOOLINFO ti = {0};
	ti.cbSize = sizeof(TOOLINFO);

	ti.hwnd = m_hwnd;
	ti.uId = id;
	SendMessage(m_hwndToolTip, TTM_DELTOOLW, 0, (LPARAM)&ti);
}

void CJabberInfoFrame::SetToolTip(int id, RECT *rc, TCHAR *pszText)
{
	TOOLINFO ti = {0};
	ti.cbSize = sizeof(TOOLINFO);

	ti.hwnd = m_hwnd;
	ti.uId = id;
	SendMessage(m_hwndToolTip, TTM_DELTOOLW, 0, (LPARAM)&ti);

	ti.uFlags = TTF_SUBCLASS;
	ti.hwnd = m_hwnd;
	ti.uId = id;
	ti.hinst = hInst;
	ti.lpszText = pszText;
	ti.rect = *rc;
	SendMessage(m_hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&ti);
}

void CJabberInfoFrame::PaintSkinGlyph(HDC hdc, RECT *rc, char **glyphs, COLORREF fallback)
{
	if (ServiceExists(MS_SKIN_DRAWGLYPH))
	{
		SKINDRAWREQUEST rq = {0};
		rq.hDC = hdc;
		rq.rcDestRect = *rc;
		rq.rcClipRect = *rc;

		for ( ; *glyphs; ++glyphs)
		{
			strncpy(rq.szObjectID, *glyphs, sizeof(rq.szObjectID));
			if (!CallService(MS_SKIN_DRAWGLYPH, (WPARAM)&rq, 0))
				return;
		}
	}

	if (fallback != 0xFFFFFFFF)
	{
		HBRUSH hbr = CreateSolidBrush(fallback);
		FillRect(hdc, rc, hbr);
		DeleteObject(hbr);
	}
}

void CJabberInfoFrame::PaintCompact(HDC hdc)
{
	RECT rc; GetClientRect(m_hwnd, &rc);
	char *glyphs[] = { "Main,ID=ProtoInfo", "Main,ID=EventArea", "Main,ID=StatusBar", NULL };
	PaintSkinGlyph(hdc, &rc, glyphs, m_clBack);

	HFONT hfntSave = (HFONT)SelectObject(hdc, m_hfntTitle);
	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, m_clTitle);

	int cx_icon = GetSystemMetrics(SM_CXSMICON);
	int cy_icon = GetSystemMetrics(SM_CYSMICON);

	int cx = rc.right - cx_icon - SZ_FRAMEPADDING;
	for (int i = m_pItems.getCount(); i--; )
	{
		CJabberInfoFrameItem &item = m_pItems[i];

		SetRect(&item.m_rcItem, 0, 0, 0, 0);
		if (!item.m_bShow) continue;
		if (!item.m_bCompact) continue;

		int depth = 0;
		for (char *p = item.m_pszName; p = strchr(p+1, '/'); ++depth) ;

		if (depth == 0)
		{
			if (item.m_hIcolibIcon)
			{
				HICON hIcon = (HICON)CallService(MS_SKIN2_GETICONBYHANDLE, 0, (LPARAM)item.m_hIcolibIcon);
				if (hIcon)
				{
					DrawIconEx(hdc, SZ_FRAMEPADDING, (rc.bottom-cy_icon)/2, hIcon, cx_icon, cy_icon, 0, NULL, DI_NORMAL);
					CallService(MS_SKIN2_RELEASEICON, (WPARAM)hIcon, 0);
				}
			}

			RECT rcText; SetRect(&rcText, cx_icon + SZ_FRAMEPADDING + SZ_ICONSPACING, 0, rc.right - SZ_FRAMEPADDING, rc.bottom);
			DrawText(hdc, item.m_pszText, lstrlen(item.m_pszText), &rcText, DT_NOPREFIX|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);
		} else
		{
			if (item.m_hIcolibIcon)
			{
				HICON hIcon = (HICON)CallService(MS_SKIN2_GETICONBYHANDLE, 0, (LPARAM)item.m_hIcolibIcon);
				if (hIcon)
				{
					SetRect(&item.m_rcItem, cx, (rc.bottom-cy_icon)/2, cx+cx_icon, (rc.bottom-cy_icon)/2+cy_icon);
					DrawIconEx(hdc, cx, (rc.bottom-cy_icon)/2, hIcon, cx_icon, cy_icon, 0, NULL, DI_NORMAL);
					cx -= cx_icon;

					CallService(MS_SKIN2_RELEASEICON, (WPARAM)hIcon, 0);

					SetToolTip(item.m_tooltipId, &item.m_rcItem, item.m_pszText);
				}
			}
		}
	}

	SelectObject(hdc, hfntSave);
}

void CJabberInfoFrame::PaintNormal(HDC hdc)
{
	RECT rc; GetClientRect(m_hwnd, &rc);
	char *glyphs[] = { "Main,ID=ProtoInfo", "Main,ID=EventArea", "Main,ID=StatusBar", NULL };
	PaintSkinGlyph(hdc, &rc, glyphs, m_clBack);

	HFONT hfntSave = (HFONT)SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
	SetBkMode(hdc, TRANSPARENT);

	int cx_icon = GetSystemMetrics(SM_CXSMICON);
	int cy_icon = GetSystemMetrics(SM_CYSMICON);
	int line_height = cy_icon + SZ_LINEPADDING;
	int cy = SZ_FRAMEPADDING;

	for (int i = 0; i < m_pItems.getCount(); ++i)
	{
		CJabberInfoFrameItem &item = m_pItems[i];
		
		if (!item.m_bShow)
		{
			SetRect(&item.m_rcItem, 0, 0, 0, 0);
			continue;
		}

		int cx = SZ_FRAMEPADDING;
		int depth = 0;
		for (char *p = item.m_pszName; p = strchr(p+1, '/'); cx += cx_icon) ++depth;

		SetRect(&item.m_rcItem, cx, cy, rc.right - SZ_FRAMEPADDING, cy + line_height);

		if (item.m_hIcolibIcon)
		{
			HICON hIcon = (HICON)CallService(MS_SKIN2_GETICONBYHANDLE, 0, (LPARAM)item.m_hIcolibIcon);
			if (hIcon)
			{
				DrawIconEx(hdc, cx, cy + (line_height-cy_icon)/2, hIcon, cx_icon, cy_icon, 0, NULL, DI_NORMAL);
				cx += cx_icon + SZ_ICONSPACING;

				CallService(MS_SKIN2_RELEASEICON, (WPARAM)hIcon, 0);
			}
		}

		SelectObject(hdc, depth ? m_hfntText : m_hfntTitle);
		SetTextColor(hdc, depth ? m_clText : m_clTitle);

		RECT rcText; SetRect(&rcText, cx, cy, rc.right - SZ_FRAMEPADDING, cy + line_height);
		DrawText(hdc, item.m_pszText, lstrlen(item.m_pszText), &rcText, DT_NOPREFIX|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);

		RemoveTooltip(item.m_tooltipId);

		cy += line_height + SZ_LINESPACING;
	}

	SelectObject(hdc, hfntSave);
}

void CJabberInfoFrame::CreateInfoItem(char *pszName, bool bCompact, LPARAM pUserData)
{
	CJabberInfoFrameItem item(pszName);
	if (CJabberInfoFrameItem *pItem = m_pItems.find(&item))
		return;

	CJabberInfoFrameItem *newItem = new CJabberInfoFrameItem(pszName, bCompact, pUserData);
	newItem->m_tooltipId = m_nextTooltipId++;
	m_pItems.insert(newItem);
	UpdateSize();
}

void CJabberInfoFrame::SetInfoItemCallback(char *pszName, void (CJabberProto::*onEvent)(CJabberInfoFrame_Event *))
{
	CJabberInfoFrameItem item(pszName);
	if (CJabberInfoFrameItem *pItem = m_pItems.find(&item))
	{
		pItem->m_onEvent = onEvent;
	}
}

void CJabberInfoFrame::UpdateInfoItem(char *pszName, HANDLE hIcolibIcon, TCHAR *pszText)
{
	CJabberInfoFrameItem item(pszName);
	if (CJabberInfoFrameItem *pItem = m_pItems.find(&item))
		pItem->SetInfo(hIcolibIcon, pszText);
	if (m_hwnd)
		RedrawWindow(m_hwnd, NULL, NULL, RDW_INVALIDATE);
}

void CJabberInfoFrame::ShowInfoItem(char *pszName, bool bShow)
{
	bool bUpdate = false;
	int length = strlen(pszName);
	for (int i = 0; i < m_pItems.getCount(); ++i)
		if ((m_pItems[i].m_bShow != bShow) && !strncmp(m_pItems[i].m_pszName, pszName, length))
		{
			m_pItems[i].m_bShow = bShow;
			m_hiddenItemCount += bShow ? -1 : 1;
			bUpdate = true;
		}

	if (bUpdate)
		UpdateSize();
}

void CJabberInfoFrame::RemoveInfoItem(char *pszName)
{
	bool bUpdate = false;
	int length = strlen(pszName);
	for (int i = 0; i < m_pItems.getCount(); ++i)
		if (!strncmp(m_pItems[i].m_pszName, pszName, length))
		{
			if (!m_pItems[i].m_bShow) --m_hiddenItemCount;
			RemoveTooltip(m_pItems[i].m_tooltipId);
			m_pItems.remove(i);
			bUpdate = true;
			--i;
		}

	if (bUpdate)
		UpdateSize();
}
