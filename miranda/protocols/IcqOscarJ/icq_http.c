// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright � 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright � 2001,2002 Jon Keating, Richard Hughes
// Copyright � 2002,2003,2004 Martin �berg, Sam Kothari, Robert Rainwater
// Copyright � 2004,2005,2006 Joe Kucera
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



int icq_httpGatewayInit(HANDLE hConn, NETLIBOPENCONNECTION *nloc, NETLIBHTTPREQUEST *nlhr)
{
  WORD wLen, wVersion, wType;
  WORD wIpLen;
  DWORD dwSid1, dwSid2, dwSid3, dwSid4;
  BYTE response[300], *buf;
  int responseBytes, recvResult;
  char szSid[33], szHttpServer[256], szHttpGetUrl[300], szHttpPostUrl[300];
  NETLIBHTTPPROXYINFO nlhpi = {0};

  
  for (responseBytes = 0; ; )
  {
    recvResult = Netlib_Recv(hConn, response + responseBytes, sizeof(response) - responseBytes, MSG_DUMPPROXY);
    if(recvResult<=0) break;
    responseBytes += recvResult;
    if(responseBytes == sizeof(response))
      break;
  }

  if (responseBytes < 31)
  {
    SetLastError(ERROR_INVALID_DATA);
    return 0;
  }

  buf = response;
  unpackWord(&buf, &wLen);
  unpackWord(&buf, &wVersion);    /* always 0x0443 */
  unpackWord(&buf, &wType);       /* hello reply */
  buf += 6;  /* dunno */
  unpackDWord(&buf, &dwSid1);
  unpackDWord(&buf, &dwSid2);
  unpackDWord(&buf, &dwSid3);
  unpackDWord(&buf, &dwSid4);
  null_snprintf(szSid, 33, "%08x%08x%08x%08x", dwSid1, dwSid2, dwSid3, dwSid4);
  unpackWord(&buf, &wIpLen);

  if(responseBytes < 30 + wIpLen || wIpLen == 0 || wIpLen > sizeof(szHttpServer) - 1)
  {
    SetLastError(ERROR_INVALID_DATA);
    return 0;
  }

  SetGatewayIndex(hConn, 1); // new master connection begins here

  memcpy(szHttpServer, buf, wIpLen);
  szHttpServer[wIpLen] = '\0';

  nlhpi.cbSize = sizeof(nlhpi);
  nlhpi.flags = NLHPIF_USEPOSTSEQUENCE;
  nlhpi.szHttpGetUrl = szHttpGetUrl;
  nlhpi.szHttpPostUrl = szHttpPostUrl;
  nlhpi.firstPostSequence = 1;
  null_snprintf(szHttpGetUrl, 300, "http://%s/monitor?sid=%s", szHttpServer, szSid);
  null_snprintf(szHttpPostUrl, 300, "http://%s/data?sid=%s&seq=", szHttpServer, szSid);

  return CallService(MS_NETLIB_SETHTTPPROXYINFO, (WPARAM)hConn, (LPARAM)&nlhpi);
}



int icq_httpGatewayBegin(HANDLE hConn, NETLIBOPENCONNECTION* nloc)
{
  icq_packet packet;
  int serverNameLen;

  serverNameLen = strlennull(nloc->szHost);

  packet.wLen = (WORD)(serverNameLen + 4);
  write_httphdr(&packet, HTTP_PACKETTYPE_LOGIN, GetGatewayIndex(hConn));
  packWord(&packet, (WORD)serverNameLen);
  packBuffer(&packet, nloc->szHost, (WORD)serverNameLen);
  packWord(&packet, nloc->wPort);
  Netlib_Send(hConn, packet.pData, packet.wLen, MSG_DUMPPROXY|MSG_NOHTTPGATEWAYWRAP);
  SAFE_FREE(&packet.pData);

  return 1;
}



int icq_httpGatewayWrapSend(HANDLE hConn, PBYTE buf, int len, int flags, MIRANDASERVICE pfnNetlibSend)
{
  icq_packet packet;
  int sendResult;

  packet.wLen = len;
  write_httphdr(&packet, HTTP_PACKETTYPE_FLAP, GetGatewayIndex(hConn));
  packBuffer(&packet, buf, (WORD)len);
  sendResult = Netlib_Send(hConn, packet.pData, packet.wLen, flags);
  SAFE_FREE(&packet.pData);

  if(sendResult <= 0)
    return sendResult;

  if(sendResult < 14)
    return 0;

  return sendResult - 14;
}



PBYTE icq_httpGatewayUnwrapRecv(NETLIBHTTPREQUEST* nlhr, PBYTE buf, int len, int* outBufLen, void *(*NetlibRealloc)(void *, size_t))
{
  WORD wLen, wType;
  DWORD dwPackSeq;
  PBYTE tbuf;
  int i, copyBytes;

  
  tbuf = buf;
  for(i = 0;;)
  {
    if (tbuf - buf + 2 > len)
      break;
    unpackWord(&tbuf, &wLen);
    if (wLen < 12)
      break;
    if (tbuf - buf + wLen > len)
      break;
    tbuf += 2;    /* version */
    unpackWord(&tbuf, &wType);
    tbuf += 4;    /* flags */
    unpackDWord(&tbuf, &dwPackSeq);
    if (wType == HTTP_PACKETTYPE_FLAP)
    {
      copyBytes = wLen - 12;
      if (copyBytes > len - i)
      {
        /* invalid data - do our best to get something out of it */
        copyBytes = len - i;
      }
      memcpy(buf + i, tbuf, copyBytes);
      i += copyBytes;
    }
    else if (wType == HTTP_PACKETTYPE_LOGINREPLY)
    {
      BYTE bRes;

      unpackByte(&tbuf, &bRes);
      wLen -= 1;
      if (!bRes)
        NetLog_Server("Gateway Connection #%d Established.", dwPackSeq);
      else
        NetLog_Server("Gateway Connection #%d Failed, error: %d", dwPackSeq, bRes);
    }
    else if (wType == HTTP_PACKETTYPE_CLOSEREPLY)
    {
      NetLog_Server("Gateway Connection #%d Closed.", dwPackSeq);
    }
    tbuf += wLen - 12;
  }
  *outBufLen = i;

  return buf;
}


int icq_httpGatewayWalkTo(HANDLE hConn, NETLIBOPENCONNECTION* nloc)
{
  icq_packet packet;
  DWORD dwGatewaySeq = GetGatewayIndex(hConn);

  packet.wLen = 0;
  write_httphdr(&packet, HTTP_PACKETTYPE_CLOSE, dwGatewaySeq);
  Netlib_Send(hConn, packet.pData, packet.wLen, MSG_DUMPPROXY|MSG_NOHTTPGATEWAYWRAP);
  // we closed virtual connection, open new one
  dwGatewaySeq++;
  SetGatewayIndex(hConn, dwGatewaySeq);
  return icq_httpGatewayBegin(hConn, nloc);
}
