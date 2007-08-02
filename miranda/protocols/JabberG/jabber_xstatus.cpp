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
#include "resource.h"

#include "sdk/m_proto_listeningto.h"

#define NUM_XMODES 61

static BOOL CALLBACK SetMoodMsgDlgProc( HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam );

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
	LPGEN("Remorseful"),  LPGEN("Restless"),    LPGEN("Sad"),       LPGEN("Sarcastic"),   LPGEN("Serious"),      LPGEN("Shocked"),
	LPGEN("Shy"),         LPGEN("Sick"),        LPGEN("Sleepy"),    LPGEN("Stressed"),    LPGEN("Surprised"),    LPGEN("Thirsty"),
	LPGEN("Worried")
};

#define _T_STUB_
const char* arXStatusTags[ NUM_XMODES ] = {
	_T_STUB_("afraid"),      _T_STUB_("amazed"),      _T_STUB_("angry"),     _T_STUB_("annoyed"),     _T_STUB_("anxious"),      _T_STUB_("aroused"),
	_T_STUB_("ashamed"),     _T_STUB_("bored"),       _T_STUB_("brave"),     _T_STUB_("calm"),        _T_STUB_("cold"),         _T_STUB_("confused"),
	_T_STUB_("contented"),   _T_STUB_("cranky"),      _T_STUB_("curious"),   _T_STUB_("depressed"),   _T_STUB_("disappointed"), _T_STUB_("disgusted"),
	_T_STUB_("distracted"),  _T_STUB_("embarrassed"), _T_STUB_("excited"),   _T_STUB_("flirtatious"), _T_STUB_("frustrated"),   _T_STUB_("grumpy"),
	_T_STUB_("guilty"),      _T_STUB_("happy"),       _T_STUB_("hot"),       _T_STUB_("humbled"),     _T_STUB_("humiliated"),   _T_STUB_("hungry"),
	_T_STUB_("hurt"),        _T_STUB_("impressed"),   _T_STUB_("in_awe"),    _T_STUB_("in_love"),     _T_STUB_("indignant"),    _T_STUB_("interested"),
	_T_STUB_("intoxicated"), _T_STUB_("invincible"),  _T_STUB_("jealous"),   _T_STUB_("lonely"),      _T_STUB_("mean"),         _T_STUB_("moody"),
	_T_STUB_("nervous"),     _T_STUB_("neutral"),     _T_STUB_("offended"),  _T_STUB_("playful"),     _T_STUB_("proud"),        _T_STUB_("relieved"),
	_T_STUB_("remorseful"),  _T_STUB_("restless"),    _T_STUB_("sad"),       _T_STUB_("sarcastic"),   _T_STUB_("serious"),      _T_STUB_("shocked"),
	_T_STUB_("shy"),         _T_STUB_("sick"),        _T_STUB_("sleepy"),    _T_STUB_("stressed"),    _T_STUB_("surprised"),    _T_STUB_("thirsty"),
	_T_STUB_("worried")
};
#undef _T_STUB_

static HANDLE arXStatusIcons[ NUM_XMODES ];

/////////////////////////////////////////////////////////////////////////////////////////
// JabberSetContactMood - sets the contact's mood

void JabberSetContactMood( HANDLE hContact, const char* moodType, const TCHAR* moodText )
{
	if ( moodType ) {
		int i;
		for ( i=0; i < NUM_XMODES; i++ ) {
			if ( !strcmp( moodType, arXStatusTags[i] )) {
				JSetByte( hContact, DBSETTING_XSTATUSID, i );
				JSetString( hContact, DBSETTING_XSTATUSNAME, arXStatusNames[i] );
				break;
		}	}

		if ( i == NUM_XMODES ) {
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
// JabberSendPepMood - sends mood

BOOL JabberSendPepMood( int nMoodNumber, TCHAR* szMoodText )
{
	if ( !jabberOnline || !jabberPepSupported || ( nMoodNumber > NUM_XMODES ))
		return FALSE;

	XmlNodeIq iq( "set", JabberSerialNext() );
	XmlNode* pubsubNode = iq.addChild( "pubsub" );
	pubsubNode->addAttr( "xmlns", JABBER_FEAT_PUBSUB );

	if ( nMoodNumber ) {
		XmlNode* publishNode = pubsubNode->addChild( "publish" );
		publishNode->addAttr( "node", JABBER_FEAT_USER_MOOD );
		XmlNode* itemNode = publishNode->addChild( "item" );
		itemNode->addAttr( "id", "current" );
		XmlNode* moodNode = itemNode->addChild( "mood" );
		moodNode->addAttr( "xmlns", JABBER_FEAT_USER_MOOD );
		moodNode->addChild( arXStatusTags[ nMoodNumber - 1 ]);
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
	jabberThreadInfo->send( iq );
	
	return TRUE;
}

BOOL JabberSendPepTune( TCHAR* szArtist, TCHAR* szLength, TCHAR* szSource, TCHAR* szTitle, TCHAR* szTrack, TCHAR* szUri )
{
	if ( !jabberOnline || !jabberPepSupported )
		return FALSE;

	XmlNodeIq iq( "set", JabberSerialNext() );
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
	jabberThreadInfo->send( iq );

	return TRUE;
}

void JabberSetContactTune( HANDLE hContact,  TCHAR* szArtist, TCHAR* szLength, TCHAR* szSource, TCHAR* szTitle, TCHAR* szTrack, TCHAR* szUri )
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
	if ( unicode )
		return mir_tstrdup( str );
	else {
		int codepage = CallService( MS_LANGPACK_GETCODEPAGE, 0, 0 );

		int cbLen = MultiByteToWideChar( codepage, 0, (char*)str, -1, 0, 0 );
		TCHAR* result = ( TCHAR* )mir_alloc( sizeof(TCHAR)*( cbLen+1 ));
		if ( result == NULL )
			return NULL;

		MultiByteToWideChar( codepage, 0, (char*)str, -1, result, cbLen );
		result[ cbLen ] = 0;
		return result;
	}
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

int JabberSetListeningTo( WPARAM wParam, LPARAM lParam )
{
	LISTENINGTOINFO *cm = (LISTENINGTOINFO *)lParam;
	if ( !cm || cm->cbSize != sizeof(LISTENINGTOINFO) ) {
		JabberSendPepTune( NULL, NULL, NULL, NULL, NULL, NULL );
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

		JabberSendPepTune( szArtist, szLength, szSource, szTitle, szTrack, NULL );
		JabberSetContactTune( NULL, szArtist, szLength, szSource, szTitle, szTrack, NULL );
		mir_free( szArtist );
		mir_free( szLength );
		mir_free( szSource );
		mir_free( szTitle );
		mir_free( szTrack );
	}
	return 0;
}


/////////////////////////////////////////////////////////////////////////////////////////
// JabberSetXStatus - sets the extended status info (mood)

int JabberSetXStatus( WPARAM wParam, LPARAM lParam )
{
	if ( !jabberPepSupported || !jabberOnline || ( wParam > NUM_XMODES ))
		return 0;

	if ( !wParam ) {
		JabberSendPepMood( 0, NULL );
		JDeleteSetting( NULL, DBSETTING_XSTATUSMSG );
		JDeleteSetting( NULL, DBSETTING_XSTATUSNAME );
		JDeleteSetting( NULL, DBSETTING_XSTATUSID );
	}
	else {
		CreateDialogParam( hInst, MAKEINTRESOURCE(IDD_SETMOODMSG), NULL, SetMoodMsgDlgProc, wParam );
	}

	jabberXStatus = wParam;

	return wParam;
}

/////////////////////////////////////////////////////////////////////////////////////////
// menuSetXStatus - a stub for status menus

static int menuSetXStatus( WPARAM wParam, LPARAM lParam, LPARAM param )
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

static BOOL CALLBACK SetMoodMsgDlgProc( HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch ( message ) {
	case WM_INITDIALOG:
		{
			if ( lParam == 0 || lParam > NUM_XMODES )
				lParam = 1;

			SetWindowLong( hwndDlg, GWL_USERDATA, lParam );
			TranslateDialogDefault(hwndDlg);

			SendDlgItemMessage( hwndDlg, IDC_MSG_MOOD, EM_LIMITTEXT, 1024, 0 );

			OldMessageEditProc = (WNDPROC)SetWindowLong( GetDlgItem( hwndDlg, IDC_MSG_MOOD ), GWL_WNDPROC, (LONG)MessageEditSubclassProc );

			TCHAR str[256], format[128];
			GetWindowText( hwndDlg, format, SIZEOF( format ));
			TCHAR* szMood = mir_a2t( Translate(arXStatusNames[ lParam - 1 ]));
			mir_sntprintf( str, SIZEOF(str), format, szMood );
			mir_free( szMood );
			SetWindowText( hwndDlg, str );

			GetDlgItemText( hwndDlg, IDOK, gszOkBuffonFormat, SIZEOF(gszOkBuffonFormat));

			DBVARIANT dbv;
			if ( !JGetStringT( NULL, DBSETTING_XSTATUSMSG, &dbv )) {
				SetDlgItemText( hwndDlg, IDC_MSG_MOOD, dbv.ptszVal );
				JFreeVariant( &dbv );
			}
			else
				SetDlgItemTextA( hwndDlg, IDC_MSG_MOOD, arXStatusNames[ lParam - 1 ] );

			gnCountdown = 5;
			SendMessage( hwndDlg, WM_TIMER, 0, 0 );
			SetTimer( hwndDlg, 1, 1000, 0 );
			ghPreshutdown = HookEventMessage( ME_SYSTEM_PRESHUTDOWN, hwndDlg, DM_MOOD_SHUTDOWN );
		}
		return TRUE;

	case WM_TIMER:
		{
			if ( gnCountdown == -1 ) { DestroyWindow( hwndDlg ); break; }
			TCHAR str[ 512 ];
			mir_sntprintf( str, SIZEOF(str), gszOkBuffonFormat, gnCountdown );
			SetDlgItemText( hwndDlg, IDOK, str );
			gnCountdown--;
			break;
		}

	case WM_COMMAND:
		{
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
		}
	case DM_MOOD_SHUTDOWN:
		DestroyWindow( hwndDlg );
		break;

	case WM_DESTROY:
		{
			TCHAR szMoodText[ 1024 ];
			szMoodText[ 0 ] = 0;
			GetDlgItemText( hwndDlg, IDC_MSG_MOOD, szMoodText, SIZEOF( szMoodText ));
			SetWindowLong( GetDlgItem( hwndDlg, IDC_MSG_MOOD ), GWL_WNDPROC, (LONG)OldMessageEditProc );

			int nStatus = GetWindowLong( hwndDlg, GWL_USERDATA );
			if ( !nStatus || nStatus > NUM_XMODES )
				nStatus = 1;
			
			JabberSendPepMood( nStatus, szMoodText );

			JSetByte( NULL, DBSETTING_XSTATUSID, nStatus );
			JSetStringT( NULL, DBSETTING_XSTATUSMSG, szMoodText );
			JSetString( NULL, DBSETTING_XSTATUSNAME, arXStatusNames[ nStatus - 1 ] );
		}
		break;
	}
	return FALSE;
}
