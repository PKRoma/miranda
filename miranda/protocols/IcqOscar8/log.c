// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin Öberg, Sam Kothari, Robert Rainwater
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// -----------------------------------------------------------------------------
//
// File name      : $Source$
// Revision       : $Revision$
// Last change on : $Date$
// Last change by : $Author$
//
// DESCRIPTION:
//
//  Describe me here please...
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"



extern HANDLE ghServerNetlibUser;
extern char gpszICQProtoName[MAX_PATH];

static const char *szLevelDescr[] = {"ICQ Note", "ICQ Warning", "ICQ Error", "ICQ Fatal"};

typedef struct {
    char *szMsg;
    char *szTitle;
} LogMessageInfo;

static void __cdecl icq_LogMessageThread(void* arg) {
    LogMessageInfo *err = (LogMessageInfo*)arg;
    if (!err) return;
    if (err->szMsg&&err->szTitle)
        MessageBox(NULL, err->szMsg, err->szTitle, MB_OK);
    if (err->szMsg) free(err->szMsg);
    if (err->szTitle) free(err->szTitle);
    free(err);
}

void icq_LogMessage(int level, const char *szMsg)
{

	int displayLevel;

	
	Netlib_Logf(ghServerNetlibUser, "%s", szMsg);
	
	displayLevel = DBGetContactSettingByte(NULL, gpszICQProtoName, "ShowLogLevel", LOG_FATAL);
	if (level >= displayLevel) {
        LogMessageInfo *lmi = (LogMessageInfo*)malloc(sizeof(LogMessageInfo));
        lmi->szMsg = _strdup(szMsg);
        lmi->szTitle = _strdup(Translate(szLevelDescr[level]));
        forkthread(icq_LogMessageThread, 0, lmi);
    }
}



void icq_LogUsingErrorCode(int level, DWORD dwError, const char *szMsg)
{

	char szBuf[1024];
	char szErrorMsg[256];
	char* pszErrorMsg;


	switch(dwError)
	{

		case ERROR_TIMEOUT:
		case WSAETIMEDOUT:
			pszErrorMsg = "The server did not respond to the connection attempt within a reasonable time, it may be temporarily down. Try again later.";
			break;

		case ERROR_GEN_FAILURE:
			pszErrorMsg = "The connection with the server was abortively closed during the connection attempt. You may have lost your local network connection.";
			break;

		case WSAEHOSTUNREACH:
		case WSAENETUNREACH:
			pszErrorMsg = "Miranda was unable to resolve the name of a server to its numeric address. This is most likely caused by a catastrophic loss of your network connection (for example, your modem has disconnected), but if you are behind a proxy, you may need to use the 'Resolve hostnames through proxy' option in M->Options->Network.";
			break;

		case WSAEHOSTDOWN:
		case WSAENETDOWN:
		case WSAECONNREFUSED:
			pszErrorMsg = "Miranda was unable to make a connection with a server. It is likely that the server is down, in which case you should wait for a while and try again later.";
			break;

		case ERROR_ACCESS_DENIED:
	 		pszErrorMsg = "Your proxy rejected the user name and password that you provided. Please check them in M->Options->Network.";
			break;

		case WSAHOST_NOT_FOUND:
		case WSANO_DATA:
			pszErrorMsg = "The server to which you are trying to connect does not exist. Check your spelling in M->Options->Network->ICQ.";
			break;

		default:
			if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwError, 0, szErrorMsg, sizeof(szErrorMsg), NULL))
				pszErrorMsg = szErrorMsg;
			else 
				pszErrorMsg = "";
			break;
	}

	_snprintf(szBuf, sizeof(szBuf), "%s%s%s (%s %d)", szMsg?Translate(szMsg):"", szMsg?"\r\n\r\n":"", pszErrorMsg, Translate("error"), dwError);
	icq_LogMessage(level, szBuf);

}
