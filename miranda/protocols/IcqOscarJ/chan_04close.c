// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
//
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004,2005 Joe Kucera
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
extern char *cookieData;
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
	char* cookie;
	char servip[16];
	oscar_tlv_chain* chain = NULL;
	WORD wError;
	NETLIBOPENCONNECTION nloc = {0};

  if (!DBGetContactSettingByte(NULL, gpszICQProtoName, "UseGateway", 0) || !DBGetContactSettingByte(NULL, gpszICQProtoName, "NLUseProxy", 0))
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
		Netlib_Logf(ghServerNetlibUser, "Error: Missing chain on close channel");
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
		icq_LogMessage(LOG_FATAL, Translate("You could not sign on because the server returned invalid data. Try again."));

		SAFE_FREE(&newServer);
		SAFE_FREE(&cookie);

    if (hServerConn) Netlib_MyCloseHandle(hServerConn);
		return;
	}

	Netlib_Logf(ghServerNetlibUser, "Authenticated.");

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

  if (!DBGetContactSettingByte(NULL, gpszICQProtoName, "UseGateway", 0) || !DBGetContactSettingByte(NULL, gpszICQProtoName, "NLUseProxy", 0))
  {
    { /* Time to release packet receiver, connection already closed */
      Netlib_CloseHandle(hServerPacketRecver);
      hServerPacketRecver = NULL; // clear the variable

      Netlib_Logf(ghServerNetlibUser, "Closed connection to login server");
    }

	  Netlib_Logf(ghServerNetlibUser, "Connecting to %s", newServer);
    hServerConn = (HANDLE)CallService(MS_NETLIB_OPENCONNECTION, (WPARAM)ghServerNetlibUser, (LPARAM)&nloc);
    if (!hServerConn && (GetLastError() == 87))
    { // this ensures that an old Miranda can also connect
      nloc.cbSize = NETLIBOPENCONNECTION_V1_SIZE;
      hServerConn = (HANDLE)CallService(MS_NETLIB_OPENCONNECTION, (WPARAM)ghServerNetlibUser, (LPARAM)&nloc);
    }
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
			  Netlib_Logf(ghServerNetlibUser, "Error: Failed to create packet receiver.");
      }
      else // we need to reset receiving structs
        bReinitRecver = 1;
    }
	}
  else
  { // TODO: We should really do some checks here
    Netlib_Logf(ghServerNetlibUser, "Walking in Gateway to %s", newServer);
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
	Netlib_Logf(ghServerNetlibUser, "Migrating to %s", migratedServer);
	if (!migratedServer || !cookieData)
	{
		icq_LogMessage(LOG_FATAL, Translate("You have been disconnected from the ICQ network because the current server shut down."));

		SAFE_FREE(&migratedServer);
		SAFE_FREE(&cookieData);

		return;
	}

	// Default port
	nloc.wPort = DEFAULT_SERVER_PORT;

	// Warning, sloppy coding follows. I'll clean this up
	// when I know the migration really works

	/* Get the ip */
	for (i = 0; i < strlen(migratedServer); i++)
	{
		if (migratedServer[i] == ':')
			break;
		servip[i] = migratedServer[i];
	}

	// Use specified port if one exists
	if (port = strrchr(migratedServer, ':'))
		nloc.wPort = (WORD)atoi(port + 1);
	if (!nloc.wPort)
		nloc.wPort = (WORD)DBGetContactSettingWord(NULL, gpszICQProtoName, "OscarPort", DEFAULT_SERVER_PORT);
	if (!nloc.wPort)
		nloc.wPort = (WORD)RandRange(1024, 65535);

	nloc.cbSize = sizeof(nloc);
	nloc.flags = 0;
	nloc.szHost = servip;

  if (!DBGetContactSettingByte(NULL, gpszICQProtoName, "UseGateway", 0) || !DBGetContactSettingByte(NULL, gpszICQProtoName, "NLUseProxy", 0))
  {
	  // Open connection to new server
	  hServerConn = (HANDLE)CallService(MS_NETLIB_OPENCONNECTION, (WPARAM)ghServerNetlibUser, (LPARAM)&nloc);
    if (!hServerConn && (GetLastError() == 87))
    { // this ensures that an old Miranda can also connect
      nloc.cbSize = NETLIBOPENCONNECTION_V1_SIZE;
      hServerConn = (HANDLE)CallService(MS_NETLIB_OPENCONNECTION, (WPARAM)ghServerNetlibUser, (LPARAM)&nloc);
    }
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
		  Netlib_Logf(ghServerNetlibUser, "Created new packet receiver");
#endif
		  hServerPacketRecver = (HANDLE)CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)hServerConn, 8192);
      bReinitRecver = 1;
    }
  }
  else
  { // TODO: We should really do some checks here
    Netlib_Logf(ghServerNetlibUser, "Walking in Gateway to %s", migratedServer);
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

	switch (wError)
	{

	case 0x01:
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
		ProtoBroadcastAck(gpszICQProtoName, NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_WRONGPASSWORD);
		mir_snprintf(msg, 250, Translate("Connection failed.\nYour ICQ number or password was rejected (%d)."), wError);
		icq_LogMessage(LOG_FATAL, msg);
		break;

	case 0x14:
  case 0x15:
		mir_snprintf(msg, 250, Translate("Connection failed.\nThe server is temporally unavailable (%d)."), wError);
		icq_LogMessage(LOG_FATAL, msg);
		break;

	case 0x16:
	case 0x17:
		mir_snprintf(msg, 250, Translate("Connection failed.\nServer has too many connections from your IP (%d)."), wError);
		icq_LogMessage(LOG_FATAL, msg);
		break;

	case 0x18:
	case 0x1D:
		mir_snprintf(msg, 250, Translate("Connection failed.\nYou have connected too quickly,\nplease wait and retry 10 to 20 minutes later (%d)."), wError);
		icq_LogMessage(LOG_FATAL, msg);
		break;

	case 0x1B:
		icq_LogMessage(LOG_FATAL, Translate("Connection failed.\nThe server did not accept this client version."));
		break;

  case 0x1C:
    icq_LogMessage(LOG_WARNING, Translate("The server sent warning, this version is getting old.\nTry to look for a new one."));

	case 0x1E:
		icq_LogMessage(LOG_FATAL, Translate("Connection failed.\nYou were rejected by the server for an unknown reason.\nThis can happen if the UIN is already connected."));
		break;

	case 0:
		break;

	default:
		mir_snprintf(msg, 50, Translate("Connection failed.\nUnknown error during sign on: 0x%02x"), wError);
		icq_LogMessage(LOG_FATAL, msg);
		break;
	}
}



static void handleRuntimeError(WORD wError)
{
	char msg[256];

	switch (wError)
	{

	case 0x01:
	{
		ProtoBroadcastAck(gpszICQProtoName, NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_OTHERLOCATION);
		icq_LogMessage(LOG_FATAL, Translate("You have been disconnected from the ICQ network because you logged on from another location using the same ICQ number."));
		break;
	}

	default:
		mir_snprintf(msg, 50, Translate("Unknown runtime error: 0x%02x"), wError);
		icq_LogMessage(LOG_FATAL, msg);
		break;
	}
}
