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

#include "m_genmenu.h"
#include "m_icolib.h"
#include "sdk/m_cluiframes.h"

#include "sdk/m_proto_listeningto.h"

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
		for ( bXStatus=1; bXStatus <= NUM_XMODES; bXStatus++ ) {
			if ( !strcmp( moodType, g_arrMoods[bXStatus].szTag )) {
				JSetByte( hContact, DBSETTING_XSTATUSID, bXStatus );
				JSetString( hContact, DBSETTING_XSTATUSNAME, g_arrMoods[bXStatus].szName );
				break;
		}	}

		if ( bXStatus > NUM_XMODES ) {
			JDeleteSetting( hContact, DBSETTING_XSTATUSID );
			JSetString( hContact, DBSETTING_XSTATUSNAME, moodType );
		}
	}
	else {
		JDeleteSetting( hContact, DBSETTING_XSTATUSID );
		JDeleteSetting( hContact, DBSETTING_XSTATUSNAME );
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
