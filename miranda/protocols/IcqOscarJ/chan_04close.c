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


extern HANDLE hServerConn;

static void handleMigration(serverthread_info *info);
static int connectNewServer(serverthread_info *info);
static void handleRuntimeError(WORD wError);
static void handleSignonError(WORD wError);


void handleCloseChannel(unsigned char *buf, WORD datalen, serverthread_info *info)
{
  oscar_tlv_chain* chain = NULL;
  WORD wError;


  // Parse server reply, prepare reconnection
  if (!info->bLoggedIn && datalen && !info->newServerReady)
    handleLoginReply(buf, datalen, info);

  if (info->isMigrating)
    handleMigration(info);

  if ((!info->bLoggedIn || info->isMigrating) && info->newServerReady)
  {
    if (!connectNewServer(info))
    { // Connecting failed
      if (info->isMigrating)
        icq_LogUsingErrorCode(LOG_ERROR, GetLastError(), "Unable to connect to migrated ICQ communication server");
      else
        icq_LogUsingErrorCode(LOG_ERROR, GetLastError(), "Unable to connect to ICQ communication server");

      SetCurrentStatus(ID_STATUS_OFFLINE);

      info->isMigrating = 0;
    }
    info->newServerReady = 0;

    return;
  }

  if (chain = readIntoTLVChain(&buf, datalen, 0))
  {
    // TLV 9 errors (runtime errors?)
    wError = getWordFromChain(chain, 0x09, 1);
    if (wError)
    {
      SetCurrentStatus(ID_STATUS_OFFLINE);

      handleRuntimeError(wError);
    }

    disposeChain(&chain);
  }
  // Server closed connection on error, or sign off
  NetLib_SafeCloseHandle(&hServerConn, TRUE);
}



void handleLoginReply(unsigned char *buf, WORD datalen, serverthread_info *info)
{
  oscar_tlv_chain* chain = NULL;
  WORD wError;

  icq_sendCloseConnection(); // imitate icq5 behaviour

  if (!(chain = readIntoTLVChain(&buf, datalen, 0)))
  {
    NetLog_Server("Error: Missing chain on close channel");
    NetLib_SafeCloseHandle(&hServerConn, TRUE);
    return; // Invalid data
  }

  // TLV 8 errors (signon errors?)
  wError = getWordFromChain(chain, 0x08, 1);
  if (wError)
  {
    handleSignonError(wError);

    // we return only if the server did not gave us cookie (possible to connect with soft error)
    if (!getLenFromChain(chain, 0x06, 1)) 
    {
      disposeChain(&chain);
      SetCurrentStatus(ID_STATUS_OFFLINE);
      NetLib_SafeCloseHandle(&hServerConn, TRUE);
      return; // Failure
    }
  }

  // We are in the login phase and no errors were reported.
  // Extract communication server info.
  info->newServer = getStrFromChain(chain, 0x05, 1);
  info->cookieData = getStrFromChain(chain, 0x06, 1);
  info->cookieDataLen = getLenFromChain(chain, 0x06, 1);

  // We dont need this anymore
  disposeChain(&chain);

  if (!info->newServer || !info->cookieData)
  {
    icq_LogMessage(LOG_FATAL, "You could not sign on because the server returned invalid data. Try again.");

    SAFE_FREE(&info->newServer);
    SAFE_FREE(&info->cookieData);
    info->cookieDataLen = 0;

    SetCurrentStatus(ID_STATUS_OFFLINE);
    NetLib_SafeCloseHandle(&hServerConn, TRUE);
    return; // Failure
  }

  NetLog_Server("Authenticated.");
  info->newServerReady = 1;

  return;
}



static int connectNewServer(serverthread_info *info)
{
  WORD servport;
  NETLIBOPENCONNECTION nloc = {0};
  int res = 0;

  if (!gbGatewayMode)
  { // close connection only if not in gateway mode
    NetLib_SafeCloseHandle(&hServerConn, TRUE);
  }

  /* Get the ip and port */
  servport = info->wServerPort; // prepare default port
  parseServerAddress(info->newServer, &servport);

  nloc.flags = 0;
  nloc.szHost = info->newServer;
  nloc.wPort = servport;

  if (!gbGatewayMode)
  {
    { /* Time to release packet receiver, connection already closed */
      NetLib_SafeCloseHandle(&info->hPacketRecver, FALSE);

      NetLog_Server("Closed connection to login server");
    }

    hServerConn = NetLib_OpenConnection(ghServerNetlibUser, NULL, &nloc);
    if (hServerConn)
    { /* Time to recreate the packet receiver */
      info->hPacketRecver = (HANDLE)CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)hServerConn, 8192);
      if (!info->hPacketRecver)
      {
        NetLog_Server("Error: Failed to create packet receiver.");
      }
      else // we need to reset receiving structs
      {
        info->bReinitRecver = 1;
        res = 1;
      }
    }
  }
  else
  { // TODO: We should really do some checks here
    NetLog_Server("Walking in Gateway to %s", info->newServer);
    // TODO: This REQUIRES more work (most probably some kind of mid-netlib module)
    icq_httpGatewayWalkTo(hServerConn, &nloc);
    res = 1;
  }
  if (!res) SAFE_FREE(&info->cookieData);

  // Free allocated memory
  // NOTE: "cookie" will get freed when we have connected to the communication server.
  SAFE_FREE(&info->newServer);

  return res;
}



static void handleMigration(serverthread_info *info)
{
  // Check the data that was saved when the migration was announced
  NetLog_Server("Migrating to %s", info->newServer);
  if (!info->newServer || !info->cookieData)
  {
    icq_LogMessage(LOG_FATAL, "You have been disconnected from the ICQ network because the current server shut down.");

    SAFE_FREE(&info->newServer);
    SAFE_FREE(&info->cookieData);
    info->newServerReady = 0;
    info->isMigrating = 0;
  }
}



static void handleSignonError(WORD wError)
{
  switch (wError)
  {

  case 0x01: // Unregistered uin
  case 0x04: // Incorrect uin or password
  case 0x05: // Mismatch uin or password
  case 0x06: // Internal Client error (bad input to authorizer)
  case 0x07: // Invalid account
    ICQBroadcastAck(NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_WRONGPASSWORD);
    ZeroMemory(gpszPassword, sizeof(gpszPassword));
    icq_LogFatalParam("Connection failed.\nYour ICQ number or password was rejected (%d).", wError);
    break;

  case 0x02: // Service temporarily unavailable
  case 0x0D: // Bad database status
  case 0x10: // Service temporarily offline
  case 0x12: // Database send error
  case 0x14: // Reservation map error
  case 0x15: // Reservation link error
  case 0x1A: // Reservation timeout
    icq_LogFatalParam("Connection failed.\nThe server is temporarily unavailable (%d).", wError);
    break;

  case 0x16: // The users num connected from this IP has reached the maximum
  case 0x17: // The users num connected from this IP has reached the maximum (reserved)
    icq_LogFatalParam("Connection failed.\nServer has too many connections from your IP (%d).", wError);
    break;

  case 0x18: // Reservation rate limit exceeded
  case 0x1D: // Rate limit exceeded
    icq_LogFatalParam("Connection failed.\nYou have connected too quickly,\nplease wait and retry 10 to 20 minutes later (%d).", wError);
    break;

  case 0x1B: // You are using an older version of ICQ. Upgrade required
    icq_LogMessage(LOG_FATAL, "Connection failed.\nThe server did not accept this client version.");
    break;

  case 0x1C: // You are using an older version of ICQ. Upgrade recommended
    icq_LogMessage(LOG_WARNING, "The server sent warning, this version is getting old.\nTry to look for a new one.");
    break;

  case 0x1E: // Can't register on the ICQ network
    icq_LogMessage(LOG_FATAL, "Connection failed.\nYou were rejected by the server for an unknown reason.\nThis can happen if the UIN is already connected.");
    break;

  case 0x0C: // Invalid database fields, MD5 login not supported
    icq_LogMessage(LOG_FATAL, "Connection failed.\nSecure (MD5) login is not supported on this account.");
    break;

  case 0:    // No error
    break;

  case 0x08: // Deleted account
  case 0x09: // Expired account
  case 0x0A: // No access to database
  case 0x0B: // No access to resolver
  case 0x0E: // Bad resolver status
  case 0x0F: // Internal error
  case 0x11: // Suspended account
  case 0x13: // Database link error
  case 0x19: // User too heavily warned
  case 0x1F: // Token server timeout
  case 0x20: // Invalid SecureID number
  case 0x21: // MC error
  case 0x22: // Age restriction
  case 0x23: // RequireRevalidation
  case 0x24: // Link rule rejected
  case 0x25: // Missing information or bad SNAC format
  case 0x26: // Link broken
  case 0x27: // Invalid client IP
  case 0x28: // Partner rejected
  case 0x29: // SecureID missing
  case 0x2A: // Blocked account | Bump user

  default:
    icq_LogFatalParam("Connection failed.\nUnknown error during sign on: 0x%02x", wError);
    break;
  }
}



static void handleRuntimeError(WORD wError)
{
  switch (wError)
  {

  case 0x01:
  {
    ICQBroadcastAck(NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_OTHERLOCATION);
    icq_LogMessage(LOG_FATAL, "You have been disconnected from the ICQ network because you logged on from another location using the same ICQ number.");
    break;
  }

  default:
    icq_LogFatalParam("Unknown runtime error: 0x%02x", wError);
    break;
  }
}
