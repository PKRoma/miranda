// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
//
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004,2005,2006,2007 Joe Kucera
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
// File name      : $URL$
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


static const char *szLevelDescr[] = {"ICQ Note", "ICQ Warning", "ICQ Error", "ICQ Fatal"};

typedef struct {
    char *szMsg;
    char *szTitle;
} LogMessageInfo;

static BOOL bErrorVisible = FALSE;

static DWORD __stdcall icq_LogMessageThread(void* arg) 
{
  LogMessageInfo *err = (LogMessageInfo*)arg;

  if (!err) return 0;
  bErrorVisible = TRUE;
  if (err->szMsg&&err->szTitle)
    MessageBoxUtf(NULL, err->szMsg, err->szTitle, MB_OK);
  SAFE_FREE(&err->szMsg);
  SAFE_FREE(&err->szTitle);
  SAFE_FREE(&err);
  bErrorVisible = FALSE;

  return 0;
}



void icq_LogMessage(int level, const char *szMsg)
{
  int displayLevel;

  NetLog_Server("%s", szMsg);

  displayLevel = ICQGetContactSettingByte(NULL, "ShowLogLevel", LOG_WARNING);
  if (level >= displayLevel)
  {
    LogMessageInfo *lmi;

    if (ICQGetContactSettingByte(NULL, "PopupsLogEnabled", DEFAULT_LOG_POPUPS_ENABLED))
    {
      if (!ShowPopUpMsg(NULL, szLevelDescr[level], szMsg, (BYTE)level))
        return; // Popup showed successfuly
    }
    if (!bErrorVisible || !ICQGetContactSettingByte(NULL, "IgnoreMultiErrorBox", 0))
    { // error not shown or allowed multi - show messagebox
      lmi = (LogMessageInfo*)SAFE_MALLOC(sizeof(LogMessageInfo));
      lmi->szMsg = ICQTranslateUtf(szMsg);
      lmi->szTitle = ICQTranslateUtf(szLevelDescr[level]);
      ICQCreateThread(icq_LogMessageThread, lmi);
    }
  }
}



void icq_LogUsingErrorCode(int level, DWORD dwError, const char *szMsg)
{
  char szBuf[1024];
  char str[1024];
  char str2[64];
  char szErrorMsg[512];
  char* pszErrorMsg;
  char* pszErrorMsgUtf = NULL;


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
  utf8_encode(pszErrorMsg, &pszErrorMsgUtf);

  null_snprintf(szBuf, sizeof(szBuf), "%s%s%s (%s %d)", szMsg?ICQTranslateUtfStatic(szMsg, str):"", szMsg?"\r\n\r\n":"", ICQTranslateUtfStatic(pszErrorMsgUtf, szErrorMsg), ICQTranslateUtfStatic("error", str2), dwError);
  SAFE_FREE(&pszErrorMsgUtf);
  
  icq_LogMessage(level, szBuf);
}



void icq_LogFatalParam(const char* szMsg, WORD wError)
{
  char str[MAX_PATH];
  char buf[MAX_PATH];

  null_snprintf(buf, MAX_PATH, ICQTranslateUtfStatic(szMsg, str), wError);
  icq_LogMessage(LOG_FATAL, buf);
}
