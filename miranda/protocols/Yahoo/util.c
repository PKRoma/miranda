/*
 * $Id$
 *
 * myYahoo Miranda Plugin 
 *
 * Authors: Gennady Feldman (aka Gena01) 
 *          Laurent Marechal (aka Peorth)
 *
 * This code is under GPL and is based on AIM, MSN and Miranda source code.
 * I want to thank Robert Rainwater and George Hazan for their code and support
 * and for answering some of my questions during development of this plugin.
 */
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <shlwapi.h>
#include <malloc.h>

#include "yahoo.h"
#include <m_popup.h>
#include <m_system.h>
#include <m_protomod.h>
#include <m_protosvc.h>
#include <m_langpack.h>
#include <m_skin.h>
#include <m_utils.h>
#include <m_options.h>

#include "resource.h"

HANDLE __stdcall YAHOO_CreateProtoServiceFunction( 
	const char* szService,
	MIRANDASERVICE serviceProc )
{
	char str[ MAXMODULELABELLENGTH ];

	strcpy( str, yahooProtocolName );
	strcat( str, szService );
	
	return CreateServiceFunction( str, serviceProc );
}

int __stdcall YAHOO_CallService( const char* szSvcName, WPARAM wParam, LPARAM lParam )
{
	return CallService( szSvcName, wParam, lParam );
}


int YAHOO_DebugLog( const char *fmt, ... )
{
	char		str[ 4096 ];
	va_list	vararg;
	int tBytes;

	va_start( vararg, fmt );
	
	tBytes = _vsnprintf( str, sizeof( str ), fmt, vararg );
	if ( tBytes > 0 )
		str[ tBytes ] = 0;

	va_end( vararg );
	
	return CallService( MS_NETLIB_LOG, ( WPARAM )hNetlibUser, ( LPARAM )str );
}

DWORD __stdcall YAHOO_GetByte( const char* valueName, int parDefltValue )
{
	return DBGetContactSettingByte( NULL, yahooProtocolName, valueName, parDefltValue );
}

DWORD __stdcall YAHOO_SetByte( const char* valueName, int parValue )
{
	return DBWriteContactSettingByte( NULL, yahooProtocolName, valueName, parValue );
}

DWORD __stdcall YAHOO_GetDword( const char* valueName, DWORD parDefltValue )
{
	return DBGetContactSettingDword( NULL, yahooProtocolName, valueName, parDefltValue );
}

DWORD __stdcall YAHOO_SetDword( const char* valueName, DWORD parValue )
{
    return DBWriteContactSettingDword(NULL, yahooProtocolName, valueName, parValue);
}

DWORD __stdcall YAHOO_SetWord( HANDLE hContact, const char* valueName, int parValue )
{
	return DBWriteContactSettingWord( hContact, yahooProtocolName, valueName, parValue );
}

WORD __stdcall YAHOO_GetWord( HANDLE hContact, const char* valueName, int parDefltValue )
{
	return DBGetContactSettingWord( hContact, yahooProtocolName, valueName, parDefltValue );
}

int __stdcall YAHOO_SendBroadcast( HANDLE hContact, int type, int result, HANDLE hProcess, LPARAM lParam )
{
	ACKDATA ack;
	
	ZeroMemory(&ack, sizeof(ack) );
	
	ack.cbSize = sizeof( ACKDATA );
	ack.szModule = yahooProtocolName; 
    ack.hContact = hContact;
	ack.type = type; 
    ack.result = result;
	ack.hProcess = hProcess; 
    ack.lParam = lParam;
	return YAHOO_CallService( MS_PROTO_BROADCASTACK, 0, ( LPARAM )&ack );
}

DWORD __stdcall YAHOO_SetString( HANDLE hContact, const char* valueName, const char* parValue )
{
	return DBWriteContactSettingString( hContact, yahooProtocolName, valueName, parValue );
}

DWORD __stdcall YAHOO_SetStringUtf( HANDLE hContact, const char* valueName, const char* parValue )
{
	return DBWriteContactSettingStringUtf( hContact, yahooProtocolName, valueName, parValue );
}

static int CALLBACK PopupWindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	//YAHOO_DebugLog("[PopupWindowProc] Got Message: %d", message);
	
	switch( message ) {
		case WM_COMMAND:
				YAHOO_DebugLog("[PopupWindowProc] WM_COMMAND");
				if ( HIWORD( wParam ) == STN_CLICKED) {
					char *szURL = (char *)PUGetPluginData( hWnd );
					if ( szURL != NULL ) 
						YahooOpenURL(szURL, 1);
				
					PUDeletePopUp( hWnd );
					return 0;
				}
				break;
				
		case WM_CONTEXTMENU:
			YAHOO_DebugLog("[PopupWindowProc] WM_CONTEXTMENU");
			PUDeletePopUp( hWnd ); 
			return TRUE;

		case UM_FREEPLUGINDATA: {
				YAHOO_DebugLog("[PopupWindowProc] UM_FREEPLUGINDATA");
				{
					char *szURL = (char *)PUGetPluginData( hWnd );
					if ( szURL != NULL ) 
						free(szURL);
				}
					
				return TRUE;
			}
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}


int __stdcall	YAHOO_ShowPopup( const char* nickname, const char* msg, const char *szURL )
{
	POPUPDATAEX ppd;

	if ( !ServiceExists( MS_POPUP_ADDPOPUPEX )) 
		return 0;

	ZeroMemory(&ppd, sizeof(ppd) );
	lstrcpy( ppd.lpzContactName, nickname );
	lstrcpy( ppd.lpzText, msg );

	ppd.PluginWindowProc = ( WNDPROC )PopupWindowProc;

	if (szURL != NULL) {
		if (lstrcmpi(szURL, "http://mail.yahoo.com") == 0) {
			ppd.lchIcon = LoadIconEx( "mail" );
		} else {
			ppd.lchIcon = LoadIconEx( "calendar" );
		}
		
		ppd.PluginData =  (void *)strdup( szURL );
	} else {
		ppd.lchIcon = LoadIconEx( "yahoo" );
	}
	YAHOO_DebugLog("[MS_POPUP_ADDPOPUPEX] Generating a popup for %s", nickname);
	YAHOO_CallService( MS_POPUP_ADDPOPUPEX, (WPARAM)&ppd, 0 );
	
	return 1;
}

int YAHOO_shownotification(const char *title, const char *info, DWORD flags)
{
    if (ServiceExists(MS_CLIST_SYSTRAY_NOTIFY)) {
        MIRANDASYSTRAYNOTIFY err;
		int ret;
		
        err.szProto = yahooProtocolName;
        err.cbSize = sizeof(err);
        err.szInfoTitle = (char *)title;
        err.szInfo = (char *)info;
        err.dwInfoFlags = flags;
        err.uTimeout = 1000 * 3;
        ret = CallService(MS_CLIST_SYSTRAY_NOTIFY, 0, (LPARAM) & err);
		
        if (ret == 0)
			return 1;
    } 
	
	MessageBox(NULL, info, title, MB_OK | MB_ICONINFORMATION);
	
    return 0;
}

void YAHOO_ShowError(const char *title, const char *buff)
{
	if (YAHOO_GetByte( "ShowErrors", 1 )) 
		if (!YAHOO_ShowPopup(title, buff, NULL))
				YAHOO_shownotification(title, buff, NIIF_ERROR);
}

int YAHOO_util_dbsettingchanged(WPARAM wParam, LPARAM lParam)
{
    DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING *) lParam;

    if ((HANDLE) wParam == NULL)
        return 0;
    if (!yahooLoggedIn)
        return 0;
        
    if (!strcmp(cws->szModule, "CList") && !strcmp(cws->szModule, yahooProtocolName)) {
        // A temporary contact has been added permanently
        if (!strcmp(cws->szSetting, "NotOnList")) {
            if (DBGetContactSettingByte((HANDLE) wParam, "CList", "Hidden", 0))
                return 0;
            if (cws->value.type == DBVT_DELETED || (cws->value.type == DBVT_BYTE && cws->value.bVal == 0)) {
                char *szProto;
                DBVARIANT dbv;
  			
                szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);
                if (szProto==NULL || strcmp(szProto, yahooProtocolName)) return 0;

                if ( !DBGetContactSetting( (HANDLE) wParam, yahooProtocolName, YAHOO_LOGINID, &dbv )){
						YAHOO_DebugLog("Adding Permanently %s to list.", dbv.pszVal);
                        YAHOO_add_buddy(dbv.pszVal, "miranda", NULL);
           		 		DBFreeVariant(&dbv);
           		}

            }
        }
    }else if (!strcmp(cws->szModule, yahooProtocolName) && !strcmp(cws->szSetting, "ApparentMode")) {
		DBVARIANT dbv;
		
        YAHOO_DebugLog("DB Setting changed.  YAHOO user's visible setting changed.");
		
		if ( !DBGetContactSetting( (HANDLE) wParam, yahooProtocolName, YAHOO_LOGINID, &dbv )){
			int iAdd;
			
			
			iAdd = (ID_STATUS_OFFLINE == DBGetContactSettingWord((HANDLE) wParam, yahooProtocolName, "ApparentMode", 0));
			yahoo_stealth(dbv.pszVal, iAdd);
			DBFreeVariant(&dbv);
		}
        
    }
    return 0;
}

char* YAHOO_GetContactName( HANDLE hContact )
{
	return ( char* )YAHOO_CallService( MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) hContact, 0 );
}

extern PLUGININFOEX pluginInfo;

/*
 * Thanks Robert for the following function. Copied from AIM plugin.
 */
void YAHOO_utils_logversion()
{
    char str[256];

#ifdef YAHOO_CVSBUILD
    _snprintf(str, sizeof(str), "Yahoo v%d.%d.%d.%da (%s %s)", (pluginInfo.version >> 24) & 0xFF, (pluginInfo.version >> 16) & 0xFF,
              (pluginInfo.version >> 8) & 0xFF, pluginInfo.version & 0xFF, __DATE__, __TIME__);
#else
    _snprintf(str, sizeof(str), "Yahoo v%d.%d.%d.%d", (pluginInfo.version >> 24) & 0xFF, (pluginInfo.version >> 16) & 0xFF,
              (pluginInfo.version >> 8) & 0xFF, pluginInfo.version & 0xFF);
#endif
    YAHOO_DebugLog(str);
#ifdef YAHOO_CVSBUILD
    YAHOO_DebugLog("You are using a development version of Yahoo.  Please make sure you are using the latest version before posting bug reports.");
#endif
}

//=======================================================================================
// Utf8Decode - converts UTF8-encoded string to the UCS2/MBCS format
//=======================================================================================

void __stdcall Utf8Decode( char* str, int maxSize, wchar_t** ucs2 )
{
	wchar_t* tempBuf;

	int len = strlen( str );
	if ( len < 2 ) {	
		if ( ucs2 != NULL ) {
			*ucs2 = ( wchar_t* )malloc(( len+1 )*sizeof( wchar_t ));
			MultiByteToWideChar( CP_ACP, 0, str, len, *ucs2, len );
			( *ucs2 )[ len ] = 0;
		}
		return;
	}

	tempBuf = ( wchar_t* )alloca(( len+1 )*sizeof( wchar_t ));
	{
		wchar_t* d = tempBuf;
		BYTE* s = ( BYTE* )str;

		while( *s )
		{
			if (( *s & 0x80 ) == 0 ) {
				*d++ = *s++;
				continue;
			}

			if (( s[0] & 0xE0 ) == 0xE0 && ( s[1] & 0xC0 ) == 0x80 && ( s[2] & 0xC0 ) == 0x80 ) {
				*d++ = (( WORD )( s[0] & 0x0F) << 12 ) + ( WORD )(( s[1] & 0x3F ) << 6 ) + ( WORD )( s[2] & 0x3F );
				s += 3;
				continue;
			}

			if (( s[0] & 0xE0 ) == 0xC0 && ( s[1] & 0xC0 ) == 0x80 ) {
				*d++ = ( WORD )(( s[0] & 0x1F ) << 6 ) + ( WORD )( s[1] & 0x3F );
				s += 2;
				continue;
			}

			*d++ = *s++;
		}

		*d = 0;
	}

	if ( ucs2 != NULL ) {
		int fullLen = ( len+1 )*sizeof( wchar_t );
		*ucs2 = ( wchar_t* )malloc( fullLen );
		memcpy( *ucs2, tempBuf, fullLen );
	}

	if ( maxSize == 0 )
		maxSize = len;

   WideCharToMultiByte( CP_ACP, 0, tempBuf, -1, str, maxSize, NULL, NULL );
}

//=======================================================================================
// Utf8Encode - converts UCS2 string to the UTF8-encoded format
//=======================================================================================

char* __stdcall Utf8EncodeUcs2( const wchar_t* src )
{
	int len = wcslen( src );
	char* result = ( char* )malloc( len*3 + 1 );
	if ( result == NULL )
		return NULL;

	{	const wchar_t* s = src;
		BYTE*	d = ( BYTE* )result;

		while( *s ) {
			int U = *s++;

			if ( U < 0x80 ) {
				*d++ = ( BYTE )U;
			}
			else if ( U < 0x800 ) {
				*d++ = 0xC0 + (( U >> 6 ) & 0x3F );
				*d++ = 0x80 + ( U & 0x003F );
			}
			else {
				*d++ = 0xE0 + ( U >> 12 );
				*d++ = 0x80 + (( U >> 6 ) & 0x3F );
				*d++ = 0x80 + ( U & 0x3F );
		}	}

		*d = 0;
	}

	return result;
}

char* __stdcall Utf8EncodeANSI(const char *msg)
{
	wchar_t *unicode;
	int wchars, err;
	char *ut;
	
	wchars = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, msg, lstrlen(msg), NULL, 0);
	
	if(wchars == 0)
		return NULL;
	
	unicode = calloc(wchars + 1, sizeof(unsigned short));
	if(unicode == NULL)
		return NULL;
	
	err = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, msg, lstrlen(msg), unicode, wchars);
	if(err != wchars) {
		free(unicode);
		return NULL;
	}
			
	ut = Utf8EncodeUcs2(unicode );
	free(unicode);
			
	return ut;
}

void SetButtonCheck(HWND hwndDlg, int CtrlID, BOOL bCheck)
{
	HWND hwndCtrl = GetDlgItem(hwndDlg, CtrlID);
	
	Button_SetCheck(hwndCtrl, (bCheck)?BST_CHECKED:BST_UNCHECKED);
}

