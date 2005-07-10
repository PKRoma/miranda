// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright � 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright � 2001,2002 Jon Keating, Richard Hughes
// Copyright � 2002,2003,2004 Martin �berg, Sam Kothari, Robert Rainwater
// Copyright � 2004,2005 Joe Kucera, Bio
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



void write_httphdr(icq_packet* pPacket, WORD wType, DWORD dwSeq)
{
	pPacket->wPlace = 0;
	pPacket->wLen += 14;
	pPacket->pData = (BYTE*)calloc(1, pPacket->wLen);

	packWord(pPacket, (WORD)(pPacket->wLen - 2));
	packWord(pPacket, HTTP_PROXY_VERSION);
	packWord(pPacket, wType);
	packDWord(pPacket, 0); // Flags?
	packDWord(pPacket, dwSeq); // Connection sequence ?
}



void __fastcall write_flap(icq_packet* pPacket, BYTE byFlapChannel)
{
	pPacket->wPlace = 0;
	pPacket->wLen += 6;
	pPacket->pData = (BYTE*)calloc(1, pPacket->wLen);

	packByte(pPacket, FLAP_MARKER);
	packByte(pPacket, byFlapChannel);
	packWord(pPacket, 0);                 // This is the sequence ID, it is filled in during the actual sending
	packWord(pPacket, (WORD)(pPacket->wLen - 6)); // This counter should not include the flap header (thus the -6)
}



void __fastcall directPacketInit(icq_packet* pPacket, DWORD dwSize)
{
	pPacket->wPlace = 0;
	pPacket->wLen   = (WORD)dwSize;
	pPacket->pData  = (BYTE *)calloc(1, dwSize + 2);

	packLEWord(pPacket, pPacket->wLen);
}



void __fastcall packByte(icq_packet* pPacket, BYTE byValue)
{
	pPacket->pData[pPacket->wPlace++] = byValue;
}



void __fastcall packWord(icq_packet* pPacket, WORD wValue)
{
	pPacket->pData[pPacket->wPlace++] = ((wValue & 0xff00) >> 8);
	pPacket->pData[pPacket->wPlace++] = (wValue & 0x00ff);
}



void __fastcall packDWord(icq_packet* pPacket, DWORD dwValue)
{
	pPacket->pData[pPacket->wPlace++] = (BYTE)((dwValue & 0xff000000) >> 24);
	pPacket->pData[pPacket->wPlace++] = (BYTE)((dwValue & 0x00ff0000) >> 16);
	pPacket->pData[pPacket->wPlace++] = (BYTE)((dwValue & 0x0000ff00) >> 8);
	pPacket->pData[pPacket->wPlace++] = (BYTE) (dwValue & 0x000000ff);
}



void packTLV(icq_packet* pPacket, WORD wType, WORD wLength, BYTE* pbyValue)
{
	packWord(pPacket, wType);
	packWord(pPacket, wLength);
	packBuffer(pPacket, pbyValue, wLength);
}



void packTLVWord(icq_packet* pPacket, WORD wType, WORD wValue)
{
	packWord(pPacket, wType);
	packWord(pPacket, 0x02);
	packWord(pPacket, wValue);
}



void packTLVDWord(icq_packet* pPacket, WORD wType, DWORD dwValue)
{
	packWord(pPacket, wType);
	packWord(pPacket, 0x04);
	packDWord(pPacket, dwValue);
}



// Pack a preformatted buffer.
// This can be used to pack strings or any type of raw data.
void packBuffer(icq_packet* pPacket, const BYTE* pbyBuffer, WORD wLength)
{

	while (wLength)
	{
		pPacket->pData[pPacket->wPlace++] = *pbyBuffer++;
		wLength--;
	}

}



// Pack a buffer and prepend it with the size as a LE WORD.
// Commented out since its not actually used anywhere right now.
//void packLEWordSizedBuffer(icq_packet* pPacket, const BYTE* pbyBuffer, WORD wLength)
//{
//
//	packLEWord(pPacket, wLength);
//	packBuffer(pPacket, pbyBuffer, wLength);
//
//}


int __fastcall getUINLen(DWORD dwUin)
{ // TODO: invent something more clever
  if (dwUin >= 1000000000) return 10;
  if (dwUin >= 100000000) return 9;
  if (dwUin >= 10000000) return 8;
  if (dwUin >= 1000000) return 7;
  if (dwUin >= 100000) return 6;
  if (dwUin >= 10000) return 5;
  if (dwUin >= 1000) return 4;
  if (dwUin >= 100) return 3;
  if (dwUin >= 10) return 2;
  return 1;
}


void __fastcall packUIN(icq_packet* pPacket, DWORD dwUin)
{
	unsigned char pszUin[UINMAXLEN];
  BYTE nUinLen = getUINLen(dwUin);

	ltoa(dwUin, pszUin, 10);

	packByte(pPacket, nUinLen);           // Length of user id
	packBuffer(pPacket, pszUin, nUinLen); // Receiving user's id
}


void packFNACHeader(icq_packet* pPacket, WORD wFamily, WORD wSubtype, WORD wFlags, DWORD dwSeq)
{
  WORD wSeq = (WORD)dwSeq & 0x7FFF; // this is necessary, if that bit is there we get disconnected

	packWord(pPacket, wFamily);   // Family type
	packWord(pPacket, wSubtype);  // Family subtype
	packWord(pPacket, wFlags);    // SNAC flags
  packWord(pPacket, wSeq);      // SNAC request id (sequence)
	packWord(pPacket, (WORD)(dwSeq>>0x10));  // SNAC request id (command)
}



void __fastcall packLEWord(icq_packet* pPacket, WORD wValue)
{
	pPacket->pData[pPacket->wPlace++] =  (wValue & 0x00ff);
	pPacket->pData[pPacket->wPlace++] = ((wValue & 0xff00) >> 8);
}



void __fastcall packLEDWord(icq_packet* pPacket, DWORD dwValue)
{
	pPacket->pData[pPacket->wPlace++] = (BYTE) (dwValue & 0x000000ff);
	pPacket->pData[pPacket->wPlace++] = (BYTE)((dwValue & 0x0000ff00) >> 8);
	pPacket->pData[pPacket->wPlace++] = (BYTE)((dwValue & 0x00ff0000) >> 16);
	pPacket->pData[pPacket->wPlace++] = (BYTE)((dwValue & 0xff000000) >> 24);
}


void ppackByte(PBYTE *buf,int *buflen,BYTE b)
{
  *buf=(PBYTE)realloc(*buf,1+*buflen);
  *(*buf+*buflen)=b;
  ++*buflen;
}


void ppackLEWord(PBYTE *buf,int *buflen,WORD w)
{
  *buf=(PBYTE)realloc(*buf,2+*buflen);
  *(PWORD)(*buf+*buflen)=w;
  *buflen+=2;
}


void ppackLEDWord(PBYTE *buf, int *buflen, DWORD d)
{
	*buf = (PBYTE)realloc(*buf, 4 + *buflen);
	*(PDWORD)(*buf + *buflen) = d;
	*buflen += 4;
}


/*void ppackLNTS(PBYTE *buf, int *buflen, const char *str)
{
	WORD len = strlen(str);
	ppackWord(buf, buflen, len);
	*buf = (PBYTE)realloc(*buf, *buflen + len);
	memcpy(*buf + *buflen, str, len);
	*buflen += len;
}*/


void ppackLELNTS(PBYTE *buf, int *buflen, const char *str)
{
	WORD len = strlen(str);
	ppackLEWord(buf, buflen, len);
	*buf = (PBYTE)realloc(*buf, *buflen + len);
	memcpy(*buf + *buflen, str, len);
	*buflen += len;
}


void ppackLELNTSfromDB(PBYTE *buf, int *buflen, const char *szSetting)
{
	DBVARIANT dbv;

	if (ICQGetContactSetting(NULL, szSetting, &dbv))
	{
		ppackLEWord(buf, buflen, 0);
	}
	else
	{
		ppackLELNTS(buf, buflen, dbv.pszVal);
    DBFreeVariant(&dbv);
	}
}


// *** TLV based (!!! WORDs and DWORDs are LE !!!)
void ppackTLVByte(PBYTE *buf, int *buflen, BYTE b, WORD wType, BYTE always)
{
  if (!always && !b) return;

	*buf = (PBYTE)realloc(*buf, 5 + *buflen);
	*(PWORD)(*buf + *buflen) = wType;
	*(PWORD)(*buf + *buflen + 2) = 1;
	*(*buf + *buflen + 4) = b;
	*buflen += 5;
}


void ppackTLVWord(PBYTE *buf, int *buflen, WORD w, WORD wType, BYTE always)
{
	if (!always && !w) return;

	*buf = (PBYTE)realloc(*buf, 6 + *buflen);
	*(PWORD)(*buf + *buflen) = wType;
	*(PWORD)(*buf + *buflen + 2) = 2;
	*(PWORD)(*buf + *buflen + 4) = w;
	*buflen += 6;
}


void ppackTLVDWord(PBYTE *buf, int *buflen, DWORD d, WORD wType, BYTE always)
{
  if (!always && !d) return;

	*buf = (PBYTE)realloc(*buf, 8 + *buflen);
	*(PWORD)(*buf + *buflen) = wType;
	*(PWORD)(*buf + *buflen + 2) = 4;
	*(PDWORD)(*buf + *buflen + 4) = d;
	*buflen += 8;
}


void ppackTLVLNTS(PBYTE *buf, int *buflen, const char *str, WORD wType, BYTE always)
{
	int len = strlen(str) + 1;

	if (!always && len < 2) return;

	*buf = (PBYTE)realloc(*buf, 6 + *buflen + len);
	*(PWORD)(*buf + *buflen) = wType;
	*(PWORD)(*buf + *buflen + 2) = len + 2;
	*(PWORD)(*buf + *buflen + 4) = len;
	memcpy(*buf + *buflen + 6, str, len);
	*buflen += len + 6;
}


void ppackTLVWordLNTS(PBYTE *buf, int *buflen, WORD w, const char *str, WORD wType, BYTE always)
{
	int len = strlen(str) + 1;

	if (!always && len < 2 && !w) return;

	*buf = (PBYTE)realloc(*buf, 8 + *buflen + len);
	*(PWORD)(*buf + *buflen) = wType;
	*(PWORD)(*buf + *buflen + 2) = len + 4;
	*(PWORD)(*buf + *buflen + 4) = w;
	*(PWORD)(*buf + *buflen + 6) = len;
	memcpy(*buf + *buflen + 8, str, len);
	*buflen += len + 8;
}


void ppackTLVLNTSByte(PBYTE *buf, int *buflen, const char *str, BYTE b, WORD wType)
{
	int len = strlen(str) + 1;

	*buf = (PBYTE)realloc(*buf, 7 + *buflen + len);
	*(PWORD)(*buf + *buflen) = wType;
	*(PWORD)(*buf + *buflen + 2) = len + 3;
	*(PWORD)(*buf + *buflen + 4) = len;
	memcpy(*buf + *buflen + 6, str, len);
	*(*buf + *buflen + 6 + len) = b;
	*buflen += len + 7;
}



void ppackTLVLNTSfromDB(PBYTE *buf, int *buflen, const char *szSetting, WORD wType)
{
	char szTmp[1024];
	char *str = "";

	if (!ICQGetContactStaticString(NULL, szSetting, szTmp, sizeof(szTmp)))
		str = szTmp;

	ppackTLVLNTS(buf, buflen, str, wType, 1);
}


void ppackTLVWordLNTSfromDB(PBYTE *buf, int *buflen, WORD w, const char *szSetting, WORD wType)
{
	char szTmp[1024];
	char *str = "";

	if (!ICQGetContactStaticString(NULL, szSetting, szTmp, sizeof(szTmp)))
		str = szTmp;

	ppackTLVWordLNTS(buf, buflen, w, str, wType, 1);
}


void ppackTLVLNTSBytefromDB(PBYTE *buf, int *buflen, const char *szSetting, BYTE b, WORD wType)
{
	char szTmp[1024];
	char *str = "";

	if (!ICQGetContactStaticString(NULL, szSetting, szTmp, sizeof(szTmp)))
		str = szTmp;

	ppackTLVLNTSByte(buf, buflen, str, b, wType);
}


void __fastcall unpackByte(BYTE** pSource, BYTE* byDestination)
{
	if (byDestination)
	{
		*byDestination = *(*pSource)++;
	}
	else
	{
		*pSource += 1;
	}
}



void __fastcall unpackWord(BYTE** pSource, WORD* wDestination)
{
  unsigned char *tmp = *pSource;

	if (wDestination)
	{
		*wDestination  = *tmp++ << 8;
		*wDestination |= *tmp++;

    *pSource = tmp;
	}
	else
	{
		*pSource += 2;
	}
}



void __fastcall unpackDWord(BYTE** pSource, DWORD* dwDestination)
{
  unsigned char *tmp = *pSource;

	if (dwDestination)
	{
		*dwDestination  = *tmp++ << 24;
		*dwDestination |= *tmp++ << 16;
		*dwDestination |= *tmp++ << 8;
		*dwDestination |= *tmp++;

    *pSource = tmp;
	}
	else
	{
		*pSource += 4;
	}
}



void __fastcall unpackLEWord(unsigned char **buf, WORD *w)
{
	unsigned char *tmp = *buf;

	if (w)
	{
		*w = (*tmp++);
		*w |= ((*tmp++) << 8);
	}
	else
		tmp += 2;

	*buf = tmp;
}



void __fastcall unpackLEDWord(unsigned char **buf, DWORD *dw)
{
	unsigned char *tmp = *buf;

	if (dw)
	{
		*dw = (*tmp++);
		*dw |= ((*tmp++) << 8);
		*dw |= ((*tmp++) << 16);
		*dw |= ((*tmp++) << 24);
	}
	else
		tmp += 4;

	*buf = tmp;
}



void unpackString(unsigned char **buf, char *string, WORD len)
{
	unsigned char *tmp = *buf;

	while (len)	/* Can have 0x00 so go by len */
	{
		*string++ = *tmp++;
		len--;
	}

	*buf = tmp;
}



void unpackWideString(unsigned char **buf, WCHAR *string, WORD len)
{
	unsigned char *tmp = *buf;

	while (len > 1)
	{
		*string = (*tmp++ << 8);
		*string |= *tmp++;

		string++;
		len -= 2;
	}

	// We have a stray byte at the end, this means that the buffer had an odd length
	// which indicates an error.
	_ASSERTE(len == 0);
	if (len != 0)
	{
		// We dont copy the last byte but we still need to increase the buffer pointer
		// (we assume that 'len' was correct) since the calling function expects
		// that it is increased 'len' bytes.
		*tmp += len;
	}

	*buf = tmp;
}



void unpackTLV(unsigned char **buf, WORD *type, WORD *len, char **string)
{
	WORD wType, wLen;
	unsigned char *tmp = *buf;

	// Unpack type and length
	unpackWord(&tmp, &wType);
	unpackWord(&tmp, &wLen);

	// Make sure we have a good pointer
	if (string != NULL)
	{
		// Unpack and save value
		*string = (char *)malloc(wLen + 1); // Add 1 for \0
		unpackString(&tmp, *string, wLen);
		*(*string + wLen) = '\0';
	}
	else
	{
		*tmp += wLen;
	}

	// Save type and length
	if (type)
		*type = wType;
	if (len)
		*len = wLen;

	// Increase source pointer
	*buf = tmp;

}



BOOL unpackUID(unsigned char** ppBuf, WORD* pwLen, DWORD *pdwUIN, char** ppszUID)
{
	BYTE nUIDLen;
  char szUIN[UINMAXLEN+1];
	
	// Sender UIN
	unpackByte(ppBuf, &nUIDLen);
	*pwLen -= 1;
	
	if ((nUIDLen > *pwLen) || (nUIDLen == 0))
		return FALSE;

  if (nUIDLen <= UINMAXLEN)
  { // it can be uin, check
    unpackString(ppBuf, szUIN, nUIDLen);
    szUIN[nUIDLen] = '\0';
    *pwLen -= nUIDLen;

    if (IsStringUIN(szUIN))
    {
      *pdwUIN = atoi(szUIN);
      return TRUE;
    }
    else if (!ppszUID)
    {
      NetLog_Server("Malformed UIN in packet");
      return FALSE;
    }

  }
  else if (!ppszUID)
  {
    NetLog_Server("Malformed UIN in packet");
    return FALSE;
  }
#ifdef DBG_AIM_SUPPORT_HACK
	if (!(*ppszUID = malloc(nUIDLen+1)))
		return FALSE;

	unpackString(ppBuf, *ppszUID, nUIDLen);
	*pwLen -= nUIDLen;
	*ppszUID[nUIDLen] = '\0';

  *pdwUIN = -1; // this is hack, since we keep numeric uin, -1 is for all aim contacts

	return TRUE;
#else
  NetLog_Server("AOL screennames not accepted");

  return FALSE;
#endif
}
