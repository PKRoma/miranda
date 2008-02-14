/*
IRC plugin for Miranda IM

Copyright (C) 2003-05 Jurgen Persson
Copyright (C) 2007-08 George Hazan

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "irc.h"

/////////////////////////////////////////////////////////////////////////////////////////
// Standard functions

void CIrcProto::setByte( const char* name, BYTE value )
{	DBWriteContactSettingByte(NULL, m_szModuleName, name, value );
}

void CIrcProto::setByte( HANDLE hContact, const char* name, BYTE value )
{	DBWriteContactSettingByte(hContact, m_szModuleName, name, value );
}

void CIrcProto::setDword( const char* name, DWORD value )
{	DBWriteContactSettingDword(NULL, m_szModuleName, name, value );
}

void CIrcProto::setDword( HANDLE hContact, const char* name, DWORD value )
{	DBWriteContactSettingDword(hContact, m_szModuleName, name, value );
}

void CIrcProto::setString( const char* name, const char* value )
{	DBWriteContactSettingString(NULL, m_szModuleName, name, value );
}

void CIrcProto::setString( HANDLE hContact, const char* name, const char* value )
{	DBWriteContactSettingString(hContact, m_szModuleName, name, value );
}

void CIrcProto::setTString( const char* name, const TCHAR* value )
{	DBWriteContactSettingTString(NULL, m_szModuleName, name, value );
}

void CIrcProto::setTString( HANDLE hContact, const char* name, const TCHAR* value )
{	DBWriteContactSettingTString(hContact, m_szModuleName, name, value );
}

void CIrcProto::setWord( const char* name, int value )
{	DBWriteContactSettingWord(NULL, m_szModuleName, name, value );
}

void CIrcProto::setWord( HANDLE hContact, const char* name, int value )
{	DBWriteContactSettingWord(hContact, m_szModuleName, name, value );
}

/////////////////////////////////////////////////////////////////////////////////////////

void CIrcProto::AddToJTemp(TString sCommand)
{
	DBVARIANT dbv;
	if ( !DBGetContactSettingTString(NULL, m_szModuleName, "JTemp", &dbv )) {
		sCommand = TString(dbv.ptszVal) + _T(" ") + sCommand;
		DBFreeVariant( &dbv );
	}

	setTString("JTemp", sCommand.c_str());
}

void CIrcProto::IrcHookEvent( const char* szEvent, IrcEventFunc pFunc )
{
	::HookEventObj( szEvent, ( MIRANDAHOOKOBJ )*( void** )&pFunc, this );
}

char* rtrim( char *string )
{
   char* p = string + strlen( string ) - 1;
   while ( p >= string ) {
		if ( *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r' )
         break;

		*p-- = 0;
   }
   return string;
}

TString GetWord(const TCHAR* text, int index)
{
	if (!text || !lstrlen( text ))
		return (TString)_T("");

	TCHAR* p1 = (TCHAR*)text;
	TCHAR* p2 = NULL;
	TString S = _T("");

	while (*p1 == ' ')
		p1++;

	if (*p1 != '\0') {
		for (int i =0; i < index; i++) {
			p2 = _tcschr( p1, ' ' );
			if ( !p2 )
				p2 = _tcschr( p1, '\0' );
			else
				while ( *p2 == ' ' )
					p2++;

			p1 = p2;
		}

		p2 = _tcschr(p1, ' ');
		if(!p2)
			p2 = _tcschr(p1, '\0');

		if (p1 != p2) {
			TCHAR* pszTemp = new TCHAR[p2-p1+1];
			lstrcpyn(pszTemp, p1, p2-p1+1);

			S = pszTemp;
			delete [] pszTemp;
	}	}

	return S;
}

TCHAR* GetWordAddress(const TCHAR* text, int index)
{
	if( !text || !lstrlen(text))
		return ( TCHAR* )text;

	TCHAR* temp = ( TCHAR* )text;

	while (*temp == ' ')
		temp++;

	if (index == 0)
		return temp;

	for (int i = 0; i < index; i++) {
		temp = _tcschr(temp, ' ');
		if ( !temp )
			temp = ( TCHAR* )_tcschr(text, '\0');
		else
			while (*temp == ' ')
				temp++;
		text = temp;
	}

	return temp;
}

void RemoveLinebreaks( TString& Message )
{
	while ( Message.find( _T("\r\n\r\n"), 0) != string::npos )
		ReplaceString( Message, _T("\r\n\r\n"), _T("\r\n"));

	if (Message.find( _T("\r\n"), 0) == 0)
		Message.erase(0,2);

	if (Message.length() >1 && Message.rfind( _T("\r\n"), Message.length()) == Message.length()-2)
		Message.erase(Message.length()-2, 2);
}

#if defined( _UNICODE )
String& ReplaceString ( String& text, const char* replaceme, const char* newword )
{
	if ( !text.empty() && replaceme != NULL) {
		int i = 0;
		while (( i = text.find(replaceme, i)) != string::npos ) {
			text.erase(i,lstrlenA(replaceme));
			text.insert(i, newword);
			i = i + lstrlenA(newword);
	}	}

	return text;
}
#endif

TString& ReplaceString ( TString& text, const TCHAR* replaceme, const TCHAR* newword)
{
	if ( !text.empty() && replaceme != NULL) {
		int i = 0;
		while (( i = text.find(replaceme, i)) != string::npos ) {
			text.erase(i,lstrlen(replaceme));
			text.insert(i, newword);
			i = i + lstrlen(newword);
	}	}

	return text;
}

char* IrcLoadFile( char* szPath)
{
	char * szContainer = NULL;
	DWORD dwSiz = 0;
	FILE *hFile = fopen(szPath,"rb");	
	if ( hFile != NULL )
	{
		fseek(hFile,0,SEEK_END); // seek to end
		dwSiz = ftell(hFile); // size
		fseek(hFile,0,SEEK_SET); // seek back to original pos
		szContainer = new char [dwSiz+1];
		fread(szContainer, 1, dwSiz, hFile);
		szContainer[dwSiz] = '\0';
		fclose(hFile);
		return szContainer;
	}

	return 0;
}

int WCCmp( const TCHAR* wild, const TCHAR* string )
{
	if ( wild == NULL || !lstrlen(wild) || string == NULL || !lstrlen(string))
		return 1;

	const TCHAR *cp, *mp;
	while ((*string) && (*wild != '*')) {
		if ((*wild != *string) && (*wild != '?'))
			return 0;

		wild++;
		string++;
	}
		
	while (*string) {
		if (*wild == '*') {
			if (!*++wild)
				return 1;

			mp = wild;
			cp = string+1;
		}
		else if ((*wild == *string) || (*wild == '?')) {
			wild++;
			string++;
		}
		else {
			wild = mp;
			string = cp++;
	}	}
		
	while (*wild == '*')
		wild++;

	return !*wild;
}	

bool CIrcProto::IsChannel(TString sName) 
{
	return ( sChannelPrefixes.find(( TCHAR )sName[0] ) != string::npos );
}

#if defined( _UNICODE )
String GetWord(const char* text, int index)
{
	if (!text || !lstrlenA( text ))
		return (String)"";

	char* p1 = (char*)text;
	char* p2 = NULL;
	String S = "";

	while (*p1 == ' ')
		p1++;

	if (*p1 != '\0') {
		for (int i =0; i < index; i++) {
			p2 = strchr( p1, ' ' );
			if ( !p2 )
				p2 = strchr( p1, '\0' );
			else
				while ( *p2 == ' ' )
					p2++;

			p1 = p2;
		}

		p2 = strchr(p1, ' ');
		if(!p2)
			p2 = strchr(p1, '\0');

		if (p1 != p2) {
			char* pszTemp = new char[p2-p1+1];
			lstrcpynA(pszTemp, p1, p2-p1+1);

			S = pszTemp;
			delete [] pszTemp;
	}	}

	return S;
}

bool CIrcProto::IsChannel(String sName) 
{
	return ( sChannelPrefixes.find(( char )sName[0] ) != string::npos );
}
#endif

TCHAR* my_strstri(const TCHAR* s1, const TCHAR* s2) 
{ 
	int i,j,k; 
	for(i=0;s1[i];i++) 
		for(j=i,k=0; _totlower(s1[j]) == _totlower(s2[k]);j++,k++) 
			if(!s2[k+1]) 
				return ( TCHAR* )(s1+i); 

	return NULL; 
} 

TCHAR* DoColorCodes (const TCHAR* text, bool bStrip, bool bReplacePercent)
{
	static TCHAR szTemp[4000]; szTemp[0] = '\0';
	TCHAR* p = szTemp;
	bool bBold = false;
	bool bUnderline = false;
	bool bItalics = false;

	if ( !text )
		return szTemp;

	while ( *text != '\0' ) {
		int iFG = -1;
		int iBG = -1;

		switch( *text ) {
		case '%': //escape
			*p++ = '%'; 
			if ( bReplacePercent )
				*p++ = '%'; 
			text++;
			break;

		case 2: //bold
			if ( !bStrip ) {
				*p++ = '%'; 
				*p++ = bBold ? 'B' : 'b'; 
			}
			bBold = !bBold;
			text++;
			break;

		case 15: //reset
			if ( !bStrip ) {
				*p++ = '%'; 
				*p++ = 'r'; 
			}
			bUnderline = false;
			bBold = false;
			text++;
			break;

		case 22: //italics
			if ( !bStrip ) {
				*p++ = '%'; 
				*p++ = bItalics ? 'I' : 'i'; 
			}
			bItalics = !bItalics;
			text++;
			break;

		case 31: //underlined
			if ( !bStrip ) {
				*p++ = '%'; 
				*p++ = bUnderline ? 'U' : 'u'; 
			}
			bUnderline = !bUnderline;
			text++;
			break;

		case 3: //colors
			text++;

			// do this if the colors should be reset to default
			if ( *text <= 47 || *text >= 58 || *text == '\0' ) {
				if ( !bStrip ) {
					*p++ = '%';
					*p++ = 'C';
					*p++ = '%';
					*p++ = 'F';
				}
				break;
			}
			else { // some colors should be set... need to find out who
				TCHAR buf[3];

				// fix foreground index
				if ( text[1] > 47 && text[1] < 58 && text[1] != '\0')
					lstrcpyn( buf, text, 3 );
				else
					lstrcpyn( buf, text, 2 );
				text += lstrlen( buf );
				iFG = _ttoi( buf );

				// fix background color
				if ( *text == ',' && text[1] > 47 && text[1] < 58 && text[1] != '\0' ) {
					text++;

					if ( text[1] > 47 && text[1] < 58 && text[1] != '\0' )
						lstrcpyn( buf, text, 3 );
					else
						lstrcpyn( buf, text, 2 );
					text += lstrlen( buf );
					iBG = _ttoi( buf );
			}	}
			
			if ( iFG >= 0 && iFG != 99 )
				while( iFG > 15 )
					iFG -= 16;
			if ( iBG >= 0 && iBG != 99 )
				while( iBG > 15 )
					iBG -= 16;

			// create tag for chat.dll
			if ( !bStrip ) {
				TCHAR buf[10];
				if ( iFG >= 0 && iFG != 99 ) {
					*p++ = '%';
					*p++ = 'c';

					mir_sntprintf( buf, SIZEOF(buf), _T("%02u"), iFG );
					for (int i = 0; i<2; i++)
						*p++ = buf[i];
				}
				else if (iFG == 99) {
					*p++ = '%';
					*p++ = 'C';
				}
				
				if ( iBG >= 0 && iBG != 99 ) {
					*p++ = '%';
					*p++ = 'f';

					mir_sntprintf( buf, SIZEOF(buf), _T("%02u"), iBG );
					for ( int i = 0; i<2; i++ )
						*p++ = buf[i];
				}
				else if ( iBG == 99 ) {
					*p++ = '%';
					*p++ = 'F';
			}	}
			break;

		default:
			*p++ = *text++;
			break;
	}	}
	
	*p = '\0';
	return szTemp;
}

int CIrcProto::CallChatEvent(WPARAM wParam, LPARAM lParam)
{
	GCEVENT * gce = (GCEVENT *)lParam;
	int iVal = 0;

	// first see if the scripting module should modify or stop this event
	if ( bMbotInstalled && ScriptingEnabled && gce 
		&& gce->time != 0 && (gce->pDest->pszID == NULL 
		|| lstrlen(gce->pDest->ptszID) != 0 && lstrcmpi(gce->pDest->ptszID , SERVERWINDOW)))
	{
		GCEVENT *gcevent= (GCEVENT*) lParam;
		GCEVENT *gcetemp = NULL;
		WPARAM wp = wParam;
		gcetemp = (GCEVENT *)mir_alloc(sizeof(GCEVENT));
		gcetemp->pDest = (GCDEST *)mir_alloc(sizeof(GCDEST));
		gcetemp->pDest->iType = gcevent->pDest->iType;
		gcetemp->dwFlags = gcevent->dwFlags;
		gcetemp->bIsMe = gcevent->bIsMe;
		gcetemp->cbSize = sizeof(GCEVENT);
		gcetemp->dwItemData = gcevent->dwItemData;
		gcetemp->time = gcevent->time;
		gcetemp->pDest->ptszID = mir_tstrdup( gcevent->pDest->ptszID );
		gcetemp->pDest->pszModule = mir_strdup( gcevent->pDest->pszModule );
		gcetemp->ptszText = mir_tstrdup( gcevent->ptszText );
		gcetemp->ptszUID = mir_tstrdup( gcevent->ptszUID );
		gcetemp->ptszNick = mir_tstrdup( gcevent->ptszNick );
		gcetemp->ptszStatus = mir_tstrdup( gcevent->ptszStatus );
		gcetemp->ptszUserInfo = mir_tstrdup( gcevent->ptszUserInfo );

		if ( Scripting_TriggerMSPGuiIn( &wp, gcetemp ) && gcetemp ) {
			//MBOT CORRECTIONS
			//if ( gcetemp && gcetemp->pDest && gcetemp->pDest->ptszID ) {
			if ( gcetemp && gcetemp->pDest && gcetemp->pDest->ptszID && 
				!my_strstri(gcetemp->pDest->ptszID, (IsConnected()) ? GetInfo().sNetwork.c_str() : TranslateT("Offline")) ) {

				TString sTempId = MakeWndID( gcetemp->pDest->ptszID );
				mir_realloc( gcetemp->pDest->ptszID, sizeof(TCHAR)*(sTempId.length() + 1));
				lstrcpyn(gcetemp->pDest->ptszID, sTempId.c_str(), sTempId.length()+1); 
			}
			iVal = CallServiceSync(MS_GC_EVENT, wp, (LPARAM) gcetemp);
		}

		if ( gcetemp ) {
			mir_free(( void* )gcetemp->pszNick);
			mir_free(( void* )gcetemp->pszUID);
			mir_free(( void* )gcetemp->pszStatus);
			mir_free(( void* )gcetemp->pszUserInfo);
			mir_free(( void* )gcetemp->pszText);
			mir_free(( void* )gcetemp->pDest->pszID);
			mir_free(( void* )gcetemp->pDest->pszModule);
			mir_free(( void* )gcetemp->pDest);
			mir_free(( void* )gcetemp);
		}

		return iVal;
	}

	return CallServiceSync( MS_GC_EVENT, wParam, ( LPARAM )gce );
}

int CIrcProto::DoEvent(int iEvent, const TCHAR* pszWindow, const TCHAR* pszNick, 
			const TCHAR* pszText, const TCHAR* pszStatus, const TCHAR* pszUserInfo, 
			DWORD dwItemData, bool bAddToLog, bool bIsMe, time_t timestamp)
{						   
	GCDEST gcd = {0};
	GCEVENT gce = {0};
	TString sID;
	TString sText = _T("");
	extern bool bEcho;

	if ( iEvent == GC_EVENT_INFORMATION && bIsMe && !bEcho )
		return false;

	if ( pszText ) {
		if (iEvent != GC_EVENT_SENDMESSAGE)
			sText = DoColorCodes(pszText, FALSE, TRUE);
		else
			sText = pszText;
	}

	if ( pszWindow ) {
		if ( lstrcmpi( pszWindow, SERVERWINDOW))
			sID = pszWindow + (TString)_T(" - ") + GetInfo().sNetwork;
		else
			sID = pszWindow;
		gcd.ptszID = (TCHAR*)sID.c_str();
	}
	else gcd.ptszID = NULL;

	gcd.pszModule = m_szModuleName;
	gcd.iType = iEvent;

	gce.cbSize = sizeof(GCEVENT);
	gce.pDest = &gcd;
	gce.ptszStatus = pszStatus;
	gce.dwFlags = GC_TCHAR + ((bAddToLog) ? GCEF_ADDTOLOG : 0);
	gce.ptszNick = pszNick;
	gce.ptszUID = pszNick;
	if (iEvent == GC_EVENT_TOPIC)
	  gce.ptszUserInfo = pszUserInfo;
	else
	  gce.ptszUserInfo = ShowAddresses ? pszUserInfo : NULL;

	if ( !sText.empty() )
		gce.ptszText = sText.c_str();

	gce.dwItemData = dwItemData;
	if(timestamp == 1)
		gce.time = time(NULL);
	else
		gce.time = timestamp;
	gce.bIsMe = bIsMe;
	return CallChatEvent((WPARAM)0, (LPARAM)&gce);
}

TString CIrcProto::ModeToStatus(int sMode) 
{
	if ( sUserModes.find( sMode ) != string::npos ) {
		switch( sMode ) {
		case 'q':
			return (TString)_T("Owner");
		case 'o':
			return (TString)_T("Op");
		case 'v':
			return (TString)_T("Voice");
		case 'h':
			return (TString)_T("Halfop");
		case 'a':
			return (TString)_T("Admin");
		default:
			return (TString)_T("Unknown");
	}	}

	return (TString)_T("Normal");
}

TString CIrcProto::PrefixToStatus(int cPrefix) 
{
	const TCHAR* p = _tcschr( sUserModePrefixes.c_str(), cPrefix );
	if ( p ) {
		int index = int( p - sUserModePrefixes.c_str());
		return ModeToStatus( sUserModes[index] );
	}

	return (TString)_T("Normal");
}

/////////////////////////////////////////////////////////////////////////////////////////
// Timer functions 

void CIrcProto::SetChatTimer(UINT_PTR &nIDEvent,UINT uElapse, IrcTimerFunc lpTimerFunc)
{
	if (nIDEvent)
		KillChatTimer(nIDEvent);

	nIDEvent = SetTimer( m_hwndTimer, NULL, uElapse, ( TIMERPROC )*( void ** )&lpTimerFunc);
}

void CIrcProto::KillChatTimer(UINT_PTR &nIDEvent)
{
	if (nIDEvent)
		KillTimer(NULL, nIDEvent);

	nIDEvent = NULL;
}

static LRESULT CALLBACK TimerWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if ( msg == WM_TIMER )
	{
		CIrcProto* ppro = ( CIrcProto* )GetWindowLong( hwnd, GWL_USERDATA );

		typedef void ( *pfnTimerStub )( CIrcProto*, int );
		pfnTimerStub foo = ( pfnTimerStub )lParam;
		( *foo )( ppro, wParam );
	}

	return FALSE;
}

void InitTimers()
{
	// register a window class for timers
	WNDCLASS wcl;
	wcl.lpfnWndProc = TimerWndProc;
	wcl.cbClsExtra = 0;
	wcl.cbWndExtra = sizeof(void*);
	wcl.hInstance = GetModuleHandle(NULL);
	wcl.hCursor = NULL;
	wcl.lpszClassName = _T(WNDCLASS_IRCWINDOW);
	wcl.hbrBackground = NULL;
	wcl.hIcon = NULL;
	wcl.lpszMenuName = NULL;
	wcl.style = 0;
	RegisterClass(&wcl);
}

/////////////////////////////////////////////////////////////////////////////////////////

int CIrcProto::SetChannelSBText(TString sWindow, CHANNELINFO * wi)
{
	TString sTemp = _T("");
	if(wi->pszMode)
	{
		sTemp += _T("[");
		sTemp += wi->pszMode;
		sTemp += _T("] ");
	}
	if(wi->pszTopic)
		sTemp += wi->pszTopic;
	sTemp = DoColorCodes(sTemp.c_str(), TRUE, FALSE);
	return DoEvent(GC_EVENT_SETSBTEXT, sWindow.c_str(), NULL, sTemp.c_str(), NULL, NULL, NULL, FALSE, FALSE, 0);
}

TString CIrcProto::MakeWndID(const TCHAR* sWindow)
{
	TCHAR buf[200];
	mir_sntprintf( buf, SIZEOF(buf), _T("%s - %s"), sWindow, (IsConnected()) ? GetInfo().sNetwork.c_str() : TranslateT("Offline"));
	return TString(buf);
}

bool CIrcProto::FreeWindowItemData(TString window, CHANNELINFO* wis)
{
	CHANNELINFO* wi;
	if ( !wis )
		wi = (CHANNELINFO *)DoEvent(GC_EVENT_GETITEMDATA, window.c_str(), NULL, NULL, NULL, NULL, NULL, FALSE, FALSE, 0);
	else
		wi = wis;
	if ( wi ) {
		delete[] wi->pszLimit;
		delete[]wi->pszMode;
		delete[]wi->pszPassword;
		delete[]wi->pszTopic;
		delete wi;
		return true;
	}
	return false;
}

bool CIrcProto::AddWindowItemData(TString window, const TCHAR* pszLimit, const TCHAR* pszMode, const TCHAR* pszPassword, const TCHAR* pszTopic)
{
	CHANNELINFO* wi = (CHANNELINFO *)DoEvent(GC_EVENT_GETITEMDATA, window.c_str(), NULL, NULL, NULL, NULL, NULL, FALSE, FALSE, 0);
	if ( wi ) {
		if ( pszLimit ) {
			wi->pszLimit = ( TCHAR* )realloc( wi->pszLimit, sizeof(TCHAR)*(lstrlen(pszLimit)+1));
			lstrcpy( wi->pszLimit, pszLimit );
		}
		if ( pszMode ) {
			wi->pszMode = ( TCHAR* )realloc( wi->pszMode, sizeof(TCHAR)*(lstrlen(pszMode)+1));
			lstrcpy( wi->pszMode, pszMode );
		}
		if ( pszPassword ) {
			wi->pszPassword = ( TCHAR* )realloc( wi->pszPassword, sizeof(TCHAR)*(lstrlen(pszPassword)+1));
			lstrcpy( wi->pszPassword, pszPassword );
		}
		if ( pszTopic ) {
			wi->pszTopic = ( TCHAR* )realloc( wi->pszTopic, sizeof(TCHAR)*(lstrlen(pszTopic)+1));
			lstrcpy( wi->pszTopic, pszTopic );
		}

		SetChannelSBText(window, wi);
		return true;
	}
	return false;
}

void CIrcProto::CreateProtoService( const char* serviceName, IrcServiceFunc pFunc )
{
	char temp[MAXMODULELABELLENGTH];
	mir_snprintf( temp, sizeof(temp), "%s%s", m_szModuleName, serviceName );
	CreateServiceFunctionObj( temp, ( MIRANDASERVICEOBJ )*( void** )&pFunc, this );
}

void CIrcProto::FindLocalIP(HANDLE con) // inspiration from jabber
{
	// Determine local IP
	int socket = CallService( MS_NETLIB_GETSOCKET, (WPARAM) con, 0);
	if ( socket != INVALID_SOCKET ) {
		struct sockaddr_in saddr;
		int len = sizeof(saddr);
		getsockname(socket, (struct sockaddr *) &saddr, &len);
		lstrcpynA(MyLocalHost, inet_ntoa(saddr.sin_addr), 49);
}	} 

void CIrcProto::DoUserhostWithReason(int type, TString reason, bool bSendCommand, TString userhostparams, ...)
{
	TCHAR temp[4096];
	TString S = _T("");
	switch( type ) {
	case 1:
		S = _T("USERHOST");
		break;
	case 2:
		S = _T("WHO");
		break;
	default:
		S= _T("USERHOST");
		break;
	}

	va_list ap;
	va_start(ap, userhostparams);
	mir_vsntprintf(temp, SIZEOF(temp), (S + _T(" ") + userhostparams).c_str(), ap);
	va_end(ap);

	// Add reason
	if ( type == 1 )
		vUserhostReasons.push_back(reason);
	else if ( type == 2 )
		vWhoInProgress.push_back(reason);

	// Do command
	if ( IsConnected() && bSendCommand )
		*this << CIrcMessage( this, temp, getCodepage(), false, false);
}

TString CIrcProto::GetNextUserhostReason(int type)
{
	TString reason = _T("");
	switch( type ) {
	case 1:
		if(!vUserhostReasons.size())
			return (TString)_T("");

		// Get reason
		reason = vUserhostReasons.front();
		vUserhostReasons.erase(vUserhostReasons.begin());
		break;
	case 2:
		if(!vWhoInProgress.size())
			return (TString)_T("");

		// Get reason
		reason = vWhoInProgress.front();
		vWhoInProgress.erase(vWhoInProgress.begin());
		break;
	}

	return reason;
}

TString CIrcProto::PeekAtReasons( int type )
{
	switch ( type ) {
	case 1:
		if(!vUserhostReasons.size())
			return (TString)_T("");
		return vUserhostReasons.front();

	case 2:
		if(!vWhoInProgress.size())
			return (TString)_T("");
		return vWhoInProgress.front();

	}
	return (TString)_T("");
}

void CIrcProto::ClearUserhostReasons(int type)
{
	switch (type) {
	case 1:
		vUserhostReasons.clear();
		break;
	case 2:
		vWhoInProgress.clear();
		break;
}	}

////////////////////////////////////////////////////////////////////

SERVER_INFO::~SERVER_INFO()
{
	mir_free( Name );
	mir_free( Address );
	mir_free( PortStart );
	mir_free( PortEnd );
	mir_free( Group );
}
