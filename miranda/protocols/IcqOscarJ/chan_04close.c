// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
//
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004,2005,2006 Joe Kucera
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


extern int isMigrating;
extern WORD wServSequence;
extern WORD wLocalSequence;
extern HANDLE hServerConn;
extern BYTE *cookieData;
extern int cookieDataLen;
extern char *migratedServer;
extern int isMigrating;
extern HANDLE hServerPacketRecver;
extern int bReinitRecver;

static void handleMigration();
static void handleRuntimeError(WORD wError);
static void handleSignonError(WORD wError);



void handleCloseChannel(unsigned char *buf, WORD datalen)
{
  WORD wCookieLen;
  WORD i;
  char* newServer;
  BYTE* cookie;
  char servip[16];
  oscar_tlv_chain* chain = NULL;
  WORD wError;
  NETLIBOPENCONNECTION nloc = {0};

  icq_sendCloseConnection(); // imitate icq5 behaviour

  if (!ICQGetContactSettingByte(NULL, "UseGateway", 0) || !ICQGetContactSettingByte(NULL, "NLUseProxy", 0))
  { // close connection only if not in gateway mode
    Netlib_CloseHandle(hServerConn);
    hServerConn = NULL;
  }

  // Todo: We really should sanity check this, maybe reset it with a timer
  // so we don't trigger on this by mistake
  if (isMigrating)
  {
    handleMigration();
    return;
  }

  // Datalen is 0 if we have signed off or if server has migrated
  if (datalen == 0)
  {
    if (hServerConn) Netlib_MyCloseHandle(hServerConn);
    return; // Signing off
  }


  if (!(chain = readIntoTLVChain(&buf, datalen, 0)))
  {
    NetLog_Server("Error: Missing chain on close channel");
    if (hServerConn) Netlib_MyCloseHandle(hServerConn);
    return;
  }

  // TLV 8 errors (signon errors?)
  wError = getWordFromChain(chain, 0x08, 1);
  if (wError)
  {
    disposeChain(&chain);
    handleSignonError(wError);

    // we return only if the server did not gave us cookie (possible to connect with soft error)
    if (getLenFromChain(chain, 0x06, 1) == 0) 
    {
      if (hServerConn) Netlib_MyCloseHandle(hServerConn);
      return;
    }
  }

  // TLV 9 errors (runtime errors?)
  wError = getWordFromChain(chain, 0x09, 1);
  if (wError)
  {
    SetCurrentStatus(ID_STATUS_OFFLINE);

    disposeChain(&chain);
    handleRuntimeError(wError);

    if (hServerConn) Netlib_MyCloseHandle(hServerConn);
    return;
  }

  // We are in the login phase and no errors were reported.
  // Extract communication server info.
  newServer = getStrFromChain(chain, 0x05, 1);
  cookie = getStrFromChain(chain, 0x06, 1);
  wCookieLen = getLenFromChain(chain, 0x06, 1);

  // We dont need this anymore
  disposeChain(&chain);

  if (!newServer || !cookie)
  {
    icq_LogMessage(LOG_FATAL, "You could not sign on because the server returned invalid data. Try again.");

    SAFE_FREE(&newServer);
    SAFE_FREE(&cookie);

    if (hServerConn) Netlib_MyCloseHandle(hServerConn);
    return;
  }

  NetLog_Server("Authenticated.");

  /* Get the ip and port */
  i = 0;
  while (newServer[i] != ':' && i < 20)
  {
    servip[i] = newServer[i];
    i++;
  }
  servip[i++] = '\0';

  nloc.cbSize = sizeof(nloc);
  nloc.flags = 0;
  nloc.szHost = servip;
  nloc.wPort = (WORD)atoi(&newServer[i]);

  if (!ICQGetContactSettingByte(NULL, "UseGateway", 0) || !ICQGetContactSettingByte(NULL, "NLUseProxy", 0))
  {
    { /* Time to release packet receiver, connection already closed */
      Netlib_CloseHandle(hServerPacketRecver);
      hServerPacketRecver = NULL; // clear the variable

      NetLog_Server("Closed connection to login server");
    }

    NetLog_Server("Connecting to %s", newServer);
    hServerConn = NetLib_OpenConnection(ghServerNetlibUser, &nloc);
    if (hServerConn == NULL)
    {
      SAFE_FREE(&cookie);
      icq_LogUsingErrorCode(LOG_ERROR, GetLastError(), "Unable to connect to ICQ communication server");
    }
    else
    { /* Time to recreate the packet receiver */
      cookieData = cookie;
      cookieDataLen = wCookieLen;
      hServerPacketRecver = (HANDLE)CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)hServerConn, 8192);
      if (!hServerPacketRecver)
      {
        NetLog_Server("Error: Failed to create packet receiver.");
      }
      else // we need to reset receiving structs
        bReinitRecver = 1;
    }
  }
  else
  { // TODO: We should really do some checks here
    NetLog_Server("Walking in Gateway to %s", newServer);
    // TODO: This REQUIRES more work (most probably some kind of mid-netlib module)
    cookieData = cookie;
    cookieDataLen = wCookieLen;
    icq_httpGatewayWalkTo(hServerConn, &nloc);
  }

  // Free allocated memory
  // NOTE: "cookie" will get freed when we have connected to the communication server.
  SAFE_FREE(&newServer);
}



static void handleMigration()
{
  NETLIBOPENCONNECTION nloc = {0};
  char servip[20];
  unsigned int i;
  char *port;


  ZeroMemory(servip, 16);

  // Check the data that was saved when the migration was announced
  NetLog_Server("Migrating to %s", migratedServer);
  if (!migratedServer || !cookieData)
  {
    icq_LogMessage(LOG_FATAL, "You have been disconnected from the ICQ network because the current server shut down.");

    SAFE_FREE(&migratedServer);
    SAFE_FREE(&cookieData);

    return;
  }

  // Default port
  nloc.wPort = DEFAULT_SERVER_PORT;

  // Warning, sloppy coding follows. I'll clean this up
  // when I know the migration really works

  /* Get the ip */
  for (i = 0; i < strlennull(migratedServer); i++)
  {
    if (migratedServer[i] == ':')
      break;
    servip[i] = migratedServer[i];
  }

  // Use specified port if one exists
  if (port = strrchr(migratedServer, ':'))
    nloc.wPort = (WORD)atoi(port + 1);
  if (!nloc.wPort)
    nloc.wPort = ICQGetContactSettingWord(NULL, "OscarPort", DEFAULT_SERVER_PORT);
  if (!nloc.wPort)
    nloc.wPort = (WORD)RandRange(1024, 65535);

  nloc.cbSize = sizeof(nloc);
  nloc.flags = 0;
  nloc.szHost = servip;

  if (!ICQGetContactSettingByte(NULL, "UseGateway", 0) || !ICQGetContactSettingByte(NULL, "NLUseProxy", 0))
  {
    // Open connection to new server
    hServerConn = NetLib_OpenConnection(ghServerNetlibUser, &nloc);
    if (hServerConn == NULL)
    {
      icq_LogUsingErrorCode(LOG_ERROR, GetLastError(), "Unable to connect to migrated ICQ communication server");
      SAFE_FREE(&cookieData);
    }
    else
    {
      // Kill the old packet receiver
      Netlib_CloseHandle(hServerPacketRecver);
      // Create new packer receiver
#ifdef _DEBUG
      NetLog_Server("Created new packet receiver");
#endif
      hServerPacketRecver = (HANDLE)CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)hServerConn, 8192);
      bReinitRecver = 1;
    }
  }
  else
  { // TODO: We should really do some checks here
    NetLog_Server("Walking in Gateway to %s", migratedServer);
    // TODO: This REQUIRES more work (most probably some kind of mid-netlib module)
    icq_httpGatewayWalkTo(hServerConn, &nloc);
  }
  // Clean up an exit
  SAFE_FREE(&migratedServer);
  isMigrating = 0;
}



static void handleSignonError(WORD wError)
{
  char msg[256];
  char str[MAX_PATH];

  switch (wError)
  {

  case 0x01: // Unregistered uin
  case 0x04: // Incorrect uin or password
  case 0x05: // Mismatch uin or password
  case 0x06: // Internal Client error (bad input to authorizer)
  case 0x07: // Invalid account
    ICQBroadcastAck(NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_WRONGPASSWORD);
    null_snprintf(msg, 250, ICQTranslateUtfStatic("Connection failed.\nYour ICQ number or password was rejected (%d).", str), wError);
    icq_LogMessage(LOG_FATAL, msg);
    break;

  case 0x02: // Service temporarily unavailable
  case 0x10: // Service temporarily offline
  case 0x14: // Reservation map error
  case 0x15: // Reservation link error
    null_snprintf(msg, 250, ICQTranslateUtfStatic("Connection failed.\nThe server is temporally unavailable (%d).", str), wError);
    icq_LogMessage(LOG_FATAL, msg);
    break;

  case 0x16: // The users num connected from this IP has reached the maximum
  case 0x17: // The users num connected from this IP has reached the maximum (reserved)
    null_snprintf(msg, 250, ICQTranslateUtfStatic("Connection failed.\nServer has too many connections from your IP (%d).", str), wError);
    icq_LogMessage(LOG_FATAL, msg);
    break;

  case 0x18: // Rate limit exceeded (reserved)
  case 0x1D: // Rate limit exceeded
    null_snprintf(msg, 250, ICQTranslateUtfStatic("Connection failed.\nYou have connected too quickly,\nplease wait and retry 10 to 20 minutes later (%d).", str), wError);
    icq_LogMessage(LOG_FATAL, msg);
    break;

  case 0x1B: // You are using an older version of ICQ. Upgrade required
    icq_LogMessage(LOG_FATAL, "Connection failed.\nThe server did not accept this client version.");
    break;

  case 0x1C: // You are using an older version of ICQ. Upgrade recommended
    icq_LogMessage(LOG_WARNING, "The server sent warning, this version is getting old.\nTry to look for a new one.");

  case 0x1E: // Can't register on the ICQ network
    icq_LogMessage(LOG_FATAL, "Connection failed.\nYou were rejected by the server for an unknown reason.\nThis can happen if the UIN is already connected.");
    break;

  case 0:
    break;

  case 0x08: // Deleted account
  case 0x09: // Expired account
  case 0x0A: // No access to database
  case 0x0B: // No access to resolver
  case 0x0C: // Invalid database fields, MD5 login not supported
  case 0x0D: // Bad database status
  case 0x0E: // Bad resolver status
  case 0x11: // Suspended account
  case 0x19: // User too heavily warned
  case 0x1A: // Reservation timeout
  case 0x22: // Account suspended due to your age
  case 0x2A: // Blocked account

  default:
    null_snprintf(msg, 50, ICQTranslateUtfStatic("Connection failed.\nUnknown error during sign on: 0x%02x", str), wError);
    icq_LogMessage(LOG_FATAL, msg);
    break;
  }
}



static void handleRuntimeError(WORD wError)
{
  char msg[256];
  char str[MAX_PATH];

  switch (wError)
  {

  case 0x01:
  {
    ICQBroadcastAck(NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_OTHERLOCATION);
    icq_LogMessage(LOG_FATAL, "You have been disconnected from the ICQ network because you logged on from another location using the same ICQ number.");
    break;
  }

  default:
    null_snprintf(msg, 50, ICQTranslateUtfStatic("Unknown runtime error: 0x%02x", str), wError);
    icq_LogMessage(LOG_FATAL, msg);
    break;
  }
}
