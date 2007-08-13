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
//  Provides capability & signature based client detection
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"


capstr* MatchCap(char* buf, int bufsize, const capstr* cap, int capsize)
{
  while (bufsize>0) // search the buffer for a capability
  {
    if (!memcmp(buf, cap, capsize))
    {
      return (capstr*)buf; // give found capability for version info
    }
    else
    {
      buf += 0x10;
      bufsize -= 0x10;
    }
  }
  return 0;
}



static void makeClientVersion(char *szBuf, const char* szClient, unsigned v1, unsigned v2, unsigned v3, unsigned v4)
{
  if (v4) 
    null_snprintf(szBuf, 64, "%s%u.%u.%u.%u", szClient, v1, v2, v3, v4);
  else if (v3) 
    null_snprintf(szBuf, 64, "%s%u.%u.%u", szClient, v1, v2, v3);
  else
    null_snprintf(szBuf, 64, "%s%u.%u", szClient, v1, v2);
}



static void verToStr(char* szStr, int v)
{
  char szVer[64];

  makeClientVersion(szVer, "", (v>>24)&0x7F, (v>>16)&0xFF, (v>>8)&0xFF, v&0xFF); 
  strcat(szStr, szVer);
  if (v&0x80000000) strcat(szStr, " alpha");
}



static char* MirandaVersionToStringEx(char* szStr, int bUnicode, char* szPlug, int v, int m)
{
  if (!v) // this is not Miranda
    return NULL;
  else
  {
    strcpy(szStr, "Miranda IM ");

    if (!m && v == 1)
      verToStr(szStr, 0x80010200);
    else if (!m && (v&0x7FFFFFFF) <= 0x030301)
      verToStr(szStr, v);
    else 
    {
      if (m)
      {
        verToStr(szStr, m);
        strcat(szStr, " ");
      }
      if (bUnicode)
        strcat(szStr, "Unicode ");

      strcat(szStr, "(");
      strcat(szStr, szPlug);
      strcat(szStr, " v");
      verToStr(szStr, v);
      strcat(szStr, ")");
    }
  }

  return szStr;
}



char* MirandaVersionToString(char* szStr, int bUnicode, int v, int m)
{
  return MirandaVersionToStringEx(szStr, bUnicode, "ICQ", v, m);
}



char* MirandaModToString(char* szStr, capstr* capId, int bUnicode, char* szModName)
{ // decode icqj mod version
  char* szClient;
  DWORD mver = (*capId)[0x4] << 0x18 | (*capId)[0x5] << 0x10 | (*capId)[0x6] << 8 | (*capId)[0x7];
  DWORD iver = (*capId)[0x8] << 0x18 | (*capId)[0x9] << 0x10 | (*capId)[0xA] << 8 | (*capId)[0xB];
  DWORD scode = (*capId)[0xC] << 0x18 | (*capId)[0xD] << 0x10 | (*capId)[0xE] << 8 | (*capId)[0xF];

  szClient = MirandaVersionToStringEx(szStr, bUnicode, szModName, iver, mver);
  if (scode == 0x5AFEC0DE)
  {
    strcat(szClient, " + SecureIM");
  }
  return szClient;
}



const capstr capMirandaIm = {'M', 'i', 'r', 'a', 'n', 'd', 'a', 'M', 0, 0, 0, 0, 0, 0, 0, 0};
const capstr capIcqJs7    = {'i', 'c', 'q', 'j', ' ', 'S', 'e', 'c', 'u', 'r', 'e', ' ', 'I', 'M', 0, 0};
const capstr capIcqJSin   = {'s', 'i', 'n', 'j', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // Miranda ICQJ S!N
const capstr capIcqJp     = {'i', 'c', 'q', 'p', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const capstr capAimOscar  = {'M', 'i', 'r', 'a', 'n', 'd', 'a', 'A', 0, 0, 0, 0, 0, 0, 0, 0};
const capstr capMimMobile = {'M', 'i', 'r', 'a', 'n', 'd', 'a', 'M', 'o', 'b', 'i', 'l', 'e', 0, 0, 0};
const capstr capMimPack   = {'M', 'I', 'M', '/', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // Custom Miranda Pack
const capstr capTrillian  = {0x97, 0xb1, 0x27, 0x51, 0x24, 0x3c, 0x43, 0x34, 0xad, 0x22, 0xd6, 0xab, 0xf7, 0x3f, 0x14, 0x09};
const capstr capTrilCrypt = {0xf2, 0xe7, 0xc7, 0xf4, 0xfe, 0xad, 0x4d, 0xfb, 0xb2, 0x35, 0x36, 0x79, 0x8b, 0xdf, 0x00, 0x00};
const capstr capSim       = {'S', 'I', 'M', ' ', 'c', 'l', 'i', 'e', 'n', 't', ' ', ' ', 0, 0, 0, 0};
const capstr capSimOld    = {0x97, 0xb1, 0x27, 0x51, 0x24, 0x3c, 0x43, 0x34, 0xad, 0x22, 0xd6, 0xab, 0xf7, 0x3f, 0x14, 0x00};
const capstr capLicq      = {'L', 'i', 'c', 'q', ' ', 'c', 'l', 'i', 'e', 'n', 't', ' ', 0, 0, 0, 0};
const capstr capKopete    = {'K', 'o', 'p', 'e', 't', 'e', ' ', 'I', 'C', 'Q', ' ', ' ', 0, 0, 0, 0};
const capstr capmIcq      = {'m', 'I', 'C', 'Q', ' ', 0xA9, ' ', 'R', '.', 'K', '.', ' ', 0, 0, 0, 0};
const capstr capAndRQ     = {'&', 'R', 'Q', 'i', 'n', 's', 'i', 'd', 'e', 0, 0, 0, 0, 0, 0, 0};
const capstr capRAndQ     = {'R', '&', 'Q', 'i', 'n', 's', 'i', 'd', 'e', 0, 0, 0, 0, 0, 0, 0};
const capstr capIMadering = {'I', 'M', 'a', 'd', 'e', 'r', 'i', 'n', 'g', ' ', 'C', 'l', 'i', 'e', 'n', 't'};
const capstr capmChat     = {'m', 'C', 'h', 'a', 't', ' ', 'i', 'c', 'q', ' ', 0, 0, 0, 0, 0, 0};
const capstr capJimm      = {'J', 'i', 'm', 'm', ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const capstr capVmIcq     = {'V', 'm', 'I', 'C', 'Q', ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const capstr capAnastasia = {0x44, 0xE5, 0xBF, 0xCE, 0xB0, 0x96, 0xE5, 0x47, 0xBD, 0x65, 0xEF, 0xD6, 0xA3, 0x7E, 0x36, 0x02};
const capstr capPalmJicq  = {'J', 'I', 'C', 'Q', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const capstr capInluxMsgr = {0xA7, 0xE4, 0x0A, 0x96, 0xB3, 0xA0, 0x47, 0x9A, 0xB8, 0x45, 0xC9, 0xE4, 0x67, 0xC5, 0x6B, 0x1F};
const capstr capMipClient = {0x4d, 0x49, 0x50, 0x20, 0x43, 0x6c, 0x69, 0x65, 0x6e, 0x74, 0x20, 0x76, 0x00, 0x00, 0x00, 0x00};
const capstr capNaim      = {0xFF, 0xFF, 0xFF, 0xFF, 'n', 'a', 'i', 'm', 0, 0, 0, 0, 0, 0, 0, 0};
const capstr capQip       = {0x56, 0x3F, 0xC8, 0x09, 0x0B, 0x6F, 0x41, 'Q', 'I', 'P', ' ', '2', '0', '0', '5', 'a'};
const capstr capQipPDA    = {0x56, 0x3F, 0xC8, 0x09, 0x0B, 0x6F, 0x41, 'Q', 'I', 'P', ' ', ' ', ' ', ' ', ' ', '!'};
const capstr capQipSymbian= {0x51, 0xAD, 0xD1, 0x90, 0x72, 0x04, 0x47, 0x3D, 0xA1, 0xA1, 0x49, 0xF4, 0xA3, 0x97, 0xA4, 0x1F};
const capstr capQipMobile = {0xB0, 0x82, 0x62, 0xF6, 0x7F, 0x7C, 0x45, 0x61, 0xAD, 0xC1, 0x1C, 0x6D, 0x75, 0x70, 0x5E, 0xC5};
const capstr capQipInfium = {0x7C, 0x73, 0x75, 0x02, 0xC3, 0xBE, 0x4F, 0x3E, 0xA6, 0x9F, 0x01, 0x53, 0x13, 0x43, 0x1E, 0x1A};
const capstr capIm2       = {0x74, 0xED, 0xC3, 0x36, 0x44, 0xDF, 0x48, 0x5B, 0x8B, 0x1C, 0x67, 0x1A, 0x1F, 0x86, 0x09, 0x9F}; // IM2 Ext Msg
const capstr capMacIcq    = {0xdd, 0x16, 0xf2, 0x02, 0x84, 0xe6, 0x11, 0xd4, 0x90, 0xdb, 0x00, 0x10, 0x4b, 0x9b, 0x4b, 0x7d};
const capstr capIs2001    = {0x2e, 0x7a, 0x64, 0x75, 0xfa, 0xdf, 0x4d, 0xc8, 0x88, 0x6f, 0xea, 0x35, 0x95, 0xfd, 0xb6, 0xdf};
const capstr capIs2002    = {0x10, 0xcf, 0x40, 0xd1, 0x4c, 0x7f, 0x11, 0xd1, 0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00};
const capstr capComm20012 = {0xa0, 0xe9, 0x3f, 0x37, 0x4c, 0x7f, 0x11, 0xd1, 0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00};
const capstr capStrIcq    = {0xa0, 0xe9, 0x3f, 0x37, 0x4f, 0xe9, 0xd3, 0x11, 0xbc, 0xd2, 0x00, 0x04, 0xac, 0x96, 0xdd, 0x96};
const capstr capAimIcon   = {0x09, 0x46, 0x13, 0x46, 0x4c, 0x7f, 0x11, 0xd1, 0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}; // CAP_AIM_BUDDYICON
const capstr capAimDirect = {0x09, 0x46, 0x13, 0x45, 0x4c, 0x7f, 0x11, 0xd1, 0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}; // CAP_AIM_DIRECTIM
const capstr capIcqLite   = {0x17, 0x8C, 0x2D, 0x9B, 0xDA, 0xA5, 0x45, 0xBB, 0x8D, 0xDB, 0xF3, 0xBD, 0xBD, 0x53, 0xA1, 0x0A};
const capstr capAimChat   = {0x74, 0x8F, 0x24, 0x20, 0x62, 0x87, 0x11, 0xD1, 0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00};
const capstr capUim       = {0xA7, 0xE4, 0x0A, 0x96, 0xB3, 0xA0, 0x47, 0x9A, 0xB8, 0x45, 0xC9, 0xE4, 0x67, 0xC5, 0x6B, 0x1F};
const capstr capRambler   = {0x7E, 0x11, 0xB7, 0x78, 0xA3, 0x53, 0x49, 0x26, 0xA8, 0x02, 0x44, 0x73, 0x52, 0x08, 0xC4, 0x2A};
const capstr capAbv       = {0x00, 0xE7, 0xE0, 0xDF, 0xA9, 0xD0, 0x4F, 0xe1, 0x91, 0x62, 0xC8, 0x90, 0x9A, 0x13, 0x2A, 0x1B};
const capstr capNetvigator= {0x4C, 0x6B, 0x90, 0xA3, 0x3D, 0x2D, 0x48, 0x0E, 0x89, 0xD6, 0x2E, 0x4B, 0x2C, 0x10, 0xD9, 0x9F};
const capstr captZers     = {0xb2, 0xec, 0x8f, 0x16, 0x7c, 0x6f, 0x45, 0x1b, 0xbd, 0x79, 0xdc, 0x58, 0x49, 0x78, 0x88, 0xb9}; // CAP_TZERS
const capstr capHtmlMsgs  = {0x01, 0x38, 0xca, 0x7b, 0x76, 0x9a, 0x49, 0x15, 0x88, 0xf2, 0x13, 0xfc, 0x00, 0x97, 0x9e, 0xa8}; // icq6
const capstr capSimpLite  = {0x53, 0x49, 0x4D, 0x50, 0x53, 0x49, 0x4D, 0x50, 0x53, 0x49, 0x4D, 0x50, 0x53, 0x49, 0x4D, 0x50};
const capstr capSimpPro   = {0x53, 0x49, 0x4D, 0x50, 0x5F, 0x50, 0x52, 0x4F, 0x53, 0x49, 0x4D, 0x50, 0x5F, 0x50, 0x52, 0x4F};
const capstr capIMsecure  = {'I', 'M', 's', 'e', 'c', 'u', 'r', 'e', 'C', 'p', 'h', 'r', 0x00, 0x00, 0x06, 0x01}; // ZoneLabs
const capstr capIMSecKey1 = {1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // ZoneLabs
const capstr capIMSecKey2 = {2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // ZoneLabs


char* cliLibicq2k  = "libicq2000";
char* cliLicqVer   = "Licq ";
char* cliCentericq = "Centericq";
char* cliLibicqUTF = "libicq2000 (Unicode)";
char* cliTrillian  = "Trillian";
char* cliQip       = "QIP %s";
char* cliIM2       = "IM2";
char* cliSpamBot   = "Spam Bot";


char* detectUserClient(HANDLE hContact, DWORD dwUin, WORD wVersion, DWORD dwFT1, DWORD dwFT2, DWORD dwFT3, DWORD dwOnlineSince, BYTE bDirectFlag, DWORD dwDirectCookie, DWORD dwWebPort, BYTE* caps, WORD wLen, BYTE* bClientId, char* szClientBuf)
{
  LPSTR szClient = NULL;
  int bMirandaIM = FALSE;

  *bClientId = 1; // Most clients does not tick as MsgIDs

  // Is this a Miranda IM client?
  if (dwFT1 == 0xffffffff)
  {
    if (dwFT2 == 0xffffffff)
    { // This is Gaim not Miranda
      szClient = "Gaim";
    }
    else if (!dwFT2 && wVersion == 7)
    { // This is WebICQ not Miranda
      szClient = "WebICQ";
    }
    else if (!dwFT2 && dwFT3 == 0x3B7248ED)
    { // And this is most probably Spam Bot
      szClient = cliSpamBot;
    }
    else 
    { // Yes this is most probably Miranda, get the version info
      szClient = MirandaVersionToString(szClientBuf, 0, dwFT2, 0);
      *bClientId = 2;
      bMirandaIM = TRUE;
    }
  }
  else if (dwFT1 == 0x7fffffff)
  { // This is Miranda with unicode core
    szClient = MirandaVersionToString(szClientBuf, 1, dwFT2, 0);
    *bClientId = 2;
    bMirandaIM = TRUE;
  }
  else if ((dwFT1 & 0xFF7F0000) == 0x7D000000)
  { // This is probably an Licq client
    DWORD ver = dwFT1 & 0xFFFF;

    makeClientVersion(szClientBuf, cliLicqVer, ver / 1000, (ver / 10) % 100, ver % 10, 0);
    if (dwFT1 & 0x00800000)
      strcat(szClientBuf, "/SSL");

    szClient = szClientBuf;
  }
  else if (dwFT1 == 0xffffff8f)
  {
    szClient = "StrICQ";
  }
  else if (dwFT1 == 0xffffff42)
  {
    szClient = "mICQ";
  }
  else if (dwFT1 == 0xffffffbe)
  {
    unsigned ver1 = (dwFT2>>24)&0xFF;
    unsigned ver2 = (dwFT2>>16)&0xFF;
    unsigned ver3 = (dwFT2>>8)&0xFF;
  
    makeClientVersion(szClientBuf, "Alicq ", ver1, ver2, ver3, 0);

    szClient = szClientBuf;
  }
  else if (dwFT1 == 0xFFFFFF7F)
  {
    szClient = "&RQ";
  }
  else if (dwFT1 == 0xFFFFFFAB)
  {
    szClient = "YSM";
  }
  else if (dwFT1 == 0x04031980)
  {
    szClient = "vICQ";
  }
  else if ((dwFT1 == 0x3AA773EE) && (dwFT2 == 0x3AA66380))
  {
    szClient = cliLibicq2k;
  }
  else if (dwFT1 == 0x3B75AC09)
  {
    szClient = cliTrillian;
  }
  else if (dwFT1 == 0x3BA8DBAF) // FT2: 0x3BEB5373; FT3: 0x3BEB5262;
  {
    if (wVersion == 2)
      szClient = "stICQ";
  }
  else if (dwFT1 == 0xFFFFFFFE && dwFT3 == 0xFFFFFFFE)
  {
    szClient = "Jimm";
  }
  else if (dwFT1 == 0x3FF19BEB && dwFT3 == 0x3FF19BEB)
  {
    szClient = cliIM2;
  }
  else if (dwFT1 == 0xDDDDEEFF && !dwFT2 && !dwFT3)
  {
    szClient = "SmartICQ";
  }
  else if ((dwFT1 & 0xFFFFFFF0) == 0x494D2B00 && !dwFT2 && !dwFT3)
  { // last byte of FT1: (5 = Win32, 3 = SmartPhone, Pocket PC)
    szClient = "IM+";
  }
  else if (dwFT1 == 0x3B4C4C0C && !dwFT2 && dwFT3 == 0x3B7248ed)
  {
    szClient = "KXicq2";
  }
  else if (dwFT1 == 0xFFFFF666 && !dwFT3)
  { // this is R&Q (Rapid Edition)
    null_snprintf(szClientBuf, 64, "R&Q %u", (unsigned)dwFT2);
    szClient = szClientBuf;   
  }
  else if (dwFT1 == dwFT2 && dwFT2 == dwFT3 && wVersion == 8)
  {
    if ((dwFT1 < dwOnlineSince + 3600) && (dwFT1 > (dwOnlineSince - 3600)))
    {
      szClient = cliSpamBot;
    }
  }
  else if (!dwFT1 && !dwFT2 && !dwFT3 && !wVersion && !wLen && dwWebPort == 0x75BB)
  {
    szClient = cliSpamBot;
  }
  else if (dwFT1 == 0x44F523B0 && dwFT2 == 0x44F523A6 && dwFT3 == 0x44F523A6 && wVersion == 8)
  {
    szClient = "Virus";
  }

  { // capabilities based detection
    capstr* capId;

    if (dwUin && caps)
    {
      // check capabilities for client identification
      if (capId = MatchCap(caps, wLen, &capMirandaIm, 8))
      { // new Miranda Signature
        DWORD iver = (*capId)[0xC] << 0x18 | (*capId)[0xD] << 0x10 | (*capId)[0xE] << 8 | (*capId)[0xF];
        DWORD mver = (*capId)[0x8] << 0x18 | (*capId)[0x9] << 0x10 | (*capId)[0xA] << 8 | (*capId)[0xB];

        szClient = MirandaVersionToString(szClientBuf, dwFT1 == 0x7fffffff, iver, mver);

        if (MatchCap(caps, wLen, &capIcqJs7, 0x4))
        { // detect mod
          strcat(szClient, " (s7 & sss)");
          if (MatchCap(caps, wLen, &capIcqJs7, 0xE))
          {
            strcat(szClient, " + SecureIM");
          }
        }
        else if ((dwFT1 & 0x7FFFFFFF) == 0x7FFFFFFF)
        {
          if (MatchCap(caps, wLen, &capMimMobile, 0x10))
            strcat(szClient, " (Mobile)");

          if (dwFT3 == 0x5AFEC0DE)
            strcat(szClient, " + SecureIM");
        }
        *bClientId = 2;
        bMirandaIM = TRUE;
      }
      else if (capId = MatchCap(caps, wLen, &capIcqJs7, 4))
      { // detect newer icqj mod
        szClient = MirandaModToString(szClientBuf, capId, dwFT3 == 0x80000000, "ICQ S7 & SSS");
        bMirandaIM = TRUE;
      }
      else if (capId = MatchCap(caps, wLen, &capIcqJSin, 4))
      { // detect newer icqj mod
        szClient = MirandaModToString(szClientBuf, capId, dwFT3 == 0x80000000, "ICQ S!N");
        bMirandaIM = TRUE;
      }
      else if (capId = MatchCap(caps, wLen, &capIcqJp, 4))
      { // detect icqj plus mod
        szClient = MirandaModToString(szClientBuf, capId, dwFT3 == 0x80000000, "ICQ Plus");
        bMirandaIM = TRUE;
      }
      else if (MatchCap(caps, wLen, &capTrillian, 0x10) || MatchCap(caps, wLen, &capTrilCrypt, 0x10))
      { // this is Trillian, check for new versions
        if (CheckContactCapabilities(hContact, CAPF_RTF))
        {
          if (CheckContactCapabilities(hContact, CAPF_OSCAR_FILE))
            szClient = "Trillian Astra";
          else
            szClient = "Trillian v3";
        }
        else
          szClient = cliTrillian;
      }
      else if ((capId = MatchCap(caps, wLen, &capSimOld, 0xF)) && ((*capId)[0xF] != 0x92 && (*capId)[0xF] >= 0x20 || (*capId)[0xF] == 0))
      {
        int hiVer = (((*capId)[0xF]) >> 6) - 1;
        unsigned loVer = (*capId)[0xF] & 0x1F;

        if ((hiVer < 0) || ((hiVer == 0) && (loVer == 0))) 
          szClient = "Kopete";
        else
        {
          null_snprintf(szClientBuf, 64, "SIM %u.%u", (unsigned)hiVer, loVer);
          szClient = szClientBuf;
        }
      }
      else if (capId = MatchCap(caps, wLen, &capSim, 0xC))
      {
        unsigned ver1 = (*capId)[0xC];
        unsigned ver2 = (*capId)[0xD];
        unsigned ver3 = (*capId)[0xE];
        unsigned ver4 = (*capId)[0xF];

        makeClientVersion(szClientBuf, "SIM ", ver1, ver2, ver3, ver4 & 0x0F);
        if (ver4 & 0x80) 
          strcat(szClientBuf,"/Win32");
        else if (ver4 & 0x40) 
          strcat(szClientBuf,"/MacOS X");

        szClient = szClientBuf;
      }
      else if (capId = MatchCap(caps, wLen, &capLicq, 0xC))
      {
        unsigned ver1 = (*capId)[0xC];
        unsigned ver2 = (*capId)[0xD] % 100;
        unsigned ver3 = (*capId)[0xE];

        makeClientVersion(szClientBuf, cliLicqVer, ver1, ver2, ver3, 0);
        if ((*capId)[0xF]) 
          strcat(szClientBuf,"/SSL");

        szClient = szClientBuf;
      }
      else if (capId = MatchCap(caps, wLen, &capKopete, 0xC))
      {
        unsigned ver1 = (*capId)[0xC];
        unsigned ver2 = (*capId)[0xD];
        unsigned ver3 = (*capId)[0xE];
        unsigned ver4 = (*capId)[0xF];

        makeClientVersion(szClientBuf, "Kopete ", ver1, ver2, ver3, ver4);

        szClient = szClientBuf;
      }
      else if (capId = MatchCap(caps, wLen, &capmIcq, 0xC))
      {
        unsigned ver1 = (*capId)[0xC];
        unsigned ver2 = (*capId)[0xD];
        unsigned ver3 = (*capId)[0xE];
        unsigned ver4 = (*capId)[0xF];

        makeClientVersion(szClientBuf, "mICQ ", ver1, ver2, ver3, ver4);

        szClient = szClientBuf;
      }
      else if (MatchCap(caps, wLen, &capIm2, 0x10))
      { // IM2 v2 provides also Aim Icon cap
        szClient = cliIM2;
      }
      else if (capId = MatchCap(caps, wLen, &capAndRQ, 9))
      {
        unsigned ver1 = (*capId)[0xC];
        unsigned ver2 = (*capId)[0xB];
        unsigned ver3 = (*capId)[0xA];
        unsigned ver4 = (*capId)[9];

        makeClientVersion(szClientBuf, "&RQ ", ver1, ver2, ver3, ver4);

        szClient = szClientBuf;
      }
      else if (capId = MatchCap(caps, wLen, &capRAndQ, 9))
      {
        unsigned ver1 = (*capId)[0xC];
        unsigned ver2 = (*capId)[0xB];
        unsigned ver3 = (*capId)[0xA];
        unsigned ver4 = (*capId)[9];

        makeClientVersion(szClientBuf, "R&Q ", ver1, ver2, ver3, ver4);

        szClient = szClientBuf;
      }
      else if (MatchCap(caps, wLen, &capIMadering, 0x10))
      { // http://imadering.com
        szClient = "IMadering";
      }
      else if (MatchCap(caps, wLen, &capQipPDA, 0x10))
      {
        szClient = "QIP PDA (Windows)";
      }
      else if (MatchCap(caps, wLen, &capQipSymbian, 0x10))
      {
        szClient = "QIP PDA (Symbian)";
      }
      else if (MatchCap(caps, wLen, &capQipMobile, 0x10))
      {
        szClient = "QIP Mobile (Java)";
      }
      else if (MatchCap(caps, wLen, &capQipInfium, 0x10))
      {
        char ver[10];

        strcpy(szClientBuf, "QIP Infium");
        if (dwFT1)
        { // add build
          null_snprintf(ver, 10, " (%d)", dwFT1);
          strcat(szClientBuf, ver);
        }
        if (dwFT2 == 0x0B)
          strcat(szClientBuf, " Beta");

        szClient = szClientBuf;
      }
      else if (capId = MatchCap(caps, wLen, &capQip, 0xE))
      {
        char ver[10];

        if (dwFT3 == 0x0F)
          strcpy(ver, "2005");
        else
        {
          strncpy(ver, (*capId)+11, 5);
          ver[5] = '\0'; // fill in missing zero
        }

        null_snprintf(szClientBuf, 64, cliQip, ver);
        if (dwFT1 && dwFT2 == 0x0E)
        { // add QIP build
          null_snprintf(ver, 10, " (%d%d%d%d)", dwFT1 >> 0x18, (dwFT1 >> 0x10) & 0xFF, (dwFT1 >> 0x08) & 0xFF, dwFT1 & 0xFF);
          strcat(szClientBuf, ver);
        }
        szClient = szClientBuf;
      }
      else if (capId = MatchCap(caps, wLen, &capmChat, 0xA))
      {
        strcpy(szClientBuf, "mChat ");
        strncat(szClientBuf, (*capId) + 0xA, 6);
        szClient = szClientBuf;
      }
      else if (capId = MatchCap(caps, wLen, &capJimm, 5))
      {
        strcpy(szClientBuf, "Jimm ");
        strncat(szClientBuf, (*capId) + 5, 11);
        szClient = szClientBuf;
      }
      else if (MatchCap(caps, wLen, &capMacIcq, 0x10))
      {
        szClient = "ICQ for Mac";
      }
      else if (MatchCap(caps, wLen, &capUim, 0x10))
      {
        szClient = "uIM";
      }
      else if (MatchCap(caps, wLen, &capAnastasia, 0x10))
      { // http://chis.nnov.ru/anastasia
        szClient = "Anastasia";
      }
      else if (capId = MatchCap(caps, wLen, &capPalmJicq, 0xC))
      { // http://www.jsoft.ru
        unsigned ver1 = (*capId)[0xC];
        unsigned ver2 = (*capId)[0xD];
        unsigned ver3 = (*capId)[0xE];
        unsigned ver4 = (*capId)[0xF];

        makeClientVersion(szClientBuf, "JICQ ", ver1, ver2, ver3, ver4);

        szClient = szClientBuf;
      }
      else if (MatchCap(caps, wLen, &capInluxMsgr, 0x10))
      { // http://www.inlusoft.com
        szClient = "Inlux Messenger";
      }
      else if (capId = MatchCap(caps, wLen, &capMipClient, 0xC))
      { // http://mip.rufon.net
        unsigned ver1 = (*capId)[0xC];
        unsigned ver2 = (*capId)[0xD];
        unsigned ver3 = (*capId)[0xE];
        unsigned ver4 = (*capId)[0xF];

        if (ver1 < 30)
        {
          makeClientVersion(szClientBuf, "MIP ", ver1, ver2, ver3, ver4);
        }
        else
        {
          strcpy(szClientBuf, "MIP ");
          strncat(szClientBuf, (*capId) + 11, 5);
        }
        szClient = szClientBuf;
      }
      else if (capId = MatchCap(caps, wLen, &capMipClient, 0x04))
      { //http://mip.rufon.net - new signature
        strcpy(szClientBuf, "MIP ");
        strncat(szClientBuf, (*capId) + 4, 12);
        szClient = szClientBuf;
      }
      else if (capId = MatchCap(caps, wLen, &capVmIcq, 0x06))
      {
        strcpy(szClientBuf, "VmICQ");
        strncat(szClientBuf, (*capId) + 5, 11);
        szClient = szClientBuf;
      }
      else if (szClient == cliLibicq2k)
      { // try to determine which client is behind libicq2000
        if (CheckContactCapabilities(hContact, CAPF_RTF))
          szClient = cliCentericq; // centericq added rtf capability to libicq2000
        else if (CheckContactCapabilities(hContact, CAPF_UTF))
          szClient = cliLibicqUTF; // IcyJuice added unicode capability to libicq2000
        // others - like jabber transport uses unmodified library, thus cannot be detected
      }
      else if (szClient == NULL) // HERE ENDS THE SIGNATURE DETECTION, after this only feature default will be detected
      {
        if (wVersion == 8 && CheckContactCapabilities(hContact, CAPF_XTRAZ) && (MatchCap(caps, wLen, &capIMSecKey1, 6) || MatchCap(caps, wLen, &capIMSecKey2, 6)))
        { // ZA mangled the version, OMG!
          wVersion = 9;
        }
        if (wVersion == 8 && (MatchCap(caps, wLen, &capComm20012, 0x10) || CheckContactCapabilities(hContact, CAPF_SRV_RELAY)))
        { // try to determine 2001-2003 versions
          if (MatchCap(caps, wLen, &capIs2001, 0x10))
          {
            if (!dwFT1 && !dwFT2 && !dwFT3)
              if (CheckContactCapabilities(hContact, CAPF_RTF))
                szClient = "TICQClient"; // possibly also older GnomeICU
              else
                szClient = "ICQ for Pocket PC";
            else
            {
              *bClientId = 0;
              szClient = "ICQ 2001";
            }
          }
          else if (MatchCap(caps, wLen, &capIs2002, 0x10))
          {
            *bClientId = 0;
            szClient = "ICQ 2002";
          }
          else if (CheckContactCapabilities(hContact, CAPF_SRV_RELAY | CAPF_UTF | CAPF_RTF))
          {
            if (!dwFT1 && !dwFT2 && !dwFT3)
            {
              if (!dwWebPort)
                szClient = "GnomeICU 0.99.5+"; // no other way
              else
                szClient = "IC@";
              }
            else
            {
              *bClientId = 0;
              szClient = "ICQ 2002/2003a";
            }
          }
          else if (CheckContactCapabilities(hContact, CAPF_SRV_RELAY | CAPF_UTF | CAPF_TYPING))
          {
            if (!dwFT1 && !dwFT2 && !dwFT3)
            {
              szClient = "PreludeICQ";
            }
          }
        }
        else if (wVersion == 9)
        { // try to determine lite versions
          if (CheckContactCapabilities(hContact, CAPF_XTRAZ))
          {
            *bClientId = 0;
            if (CheckContactCapabilities(hContact, CAPF_OSCAR_FILE))
            {
              if (MatchCap(caps, wLen, &captZers, 0x10))
              { // capable of tZers ?
                if (MatchCap(caps, wLen, &capHtmlMsgs, 0x10))
                {
                  strcpy(szClientBuf, "ICQ 6");
                }
                else
                {
                  strcpy(szClientBuf, "icq5.1");
                }
                SetContactCapabilities(hContact, CAPF_STATUSMSGEXT);
              }
              else
              {
                strcpy(szClientBuf, "icq5");
              }
              if (MatchCap(caps, wLen, &capRambler, 0x10))
              {
                strcat(szClientBuf, " (Rambler)");
              }
              else if (MatchCap(caps, wLen, &capAbv, 0x10))
              {
                strcat(szClientBuf, " (Abv)");
              }
              else if (MatchCap(caps, wLen, &capNetvigator, 0x10))
              {
                strcat(szClientBuf, " (Netvigator)");
              }
              szClient = szClientBuf;
            }
            else if (!CheckContactCapabilities(hContact, CAPF_ICQDIRECT))
              if (CheckContactCapabilities(hContact, CAPF_RTF))
              { // most probably Qnext - try to make that shit at least receiving our msgs
                ClearContactCapabilities(hContact, CAPF_SRV_RELAY);
                NetLog_Server("Forcing simple messages (QNext client).");
                szClient = "QNext";
              }
              else
                szClient = "pyICQ";
            else
              szClient = "ICQ Lite v4";
          }
          else if (!CheckContactCapabilities(hContact, CAPF_ICQDIRECT))
          {
            if (CheckContactCapabilities(hContact, CAPF_UTF) && !CheckContactCapabilities(hContact, CAPF_RTF))
              szClient = "pyICQ";
          }
        }
        else if (wVersion == 7)
        {
          if (CheckContactCapabilities(hContact, CAPF_RTF))
            szClient = "GnomeICU"; // this is an exception
          else if (CheckContactCapabilities(hContact, CAPF_SRV_RELAY))
          {
            if (!dwFT1 && !dwFT2 && !dwFT3)
              szClient = "&RQ";
            else
            {
              *bClientId = 0;
              szClient = "ICQ 2000";
            }
          }
          else if (CheckContactCapabilities(hContact, CAPF_UTF))
          {
            if (CheckContactCapabilities(hContact, CAPF_TYPING))
              szClient = "Icq2Go! (Java)";
            else if (bDirectFlag == 0x04)
              szClient = "Pocket Web 1&1";
            else
              szClient = "Icq2Go!";
          }
        }
        else if (wVersion == 0xA)
        {
          if (!CheckContactCapabilities(hContact, CAPF_RTF) && !CheckContactCapabilities(hContact, CAPF_UTF))
          { // this is bad, but we must do it - try to detect QNext
            ClearContactCapabilities(hContact, CAPF_SRV_RELAY);
            NetLog_Server("Forcing simple messages (QNext client).");
            szClient = "QNext";
          }
          else if (!CheckContactCapabilities(hContact, CAPF_RTF) && CheckContactCapabilities(hContact, CAPF_UTF) && !dwFT1 && !dwFT2 && !dwFT3)
          { // not really good, but no other option
            szClient = "NanoICQ";
          }
        }
        else if (wVersion == 0)
        { // capability footprint based detection - not really reliable
          if (!dwFT1 && !dwFT2 && !dwFT3 && !dwWebPort && !dwDirectCookie)
          { // DC info is empty
            if (CheckContactCapabilities(hContact, CAPF_TYPING) && MatchCap(caps, wLen, &capIs2001, 0x10) &&
              MatchCap(caps, wLen, &capIs2002, 0x10) && MatchCap(caps, wLen, &capComm20012, 0x10))
              szClient = cliSpamBot;
            else if (MatchCap(caps, wLen, &capAimIcon, 0x10) && MatchCap(caps, wLen, &capAimDirect, 0x10) && 
              CheckContactCapabilities(hContact, CAPF_OSCAR_FILE | CAPF_UTF))
            { // detect libgaim
              if (CheckContactCapabilities(hContact, CAPF_SRV_RELAY))
                szClient = "Adium X"; // yeah, AFAIK only Adium has this fixed
              else
                szClient = "libgaim";
            }
            else if (MatchCap(caps, wLen, &capAimIcon, 0x10) && MatchCap(caps, wLen, &capAimDirect, 0x10) &&
              MatchCap(caps, wLen, &capAimChat, 0x10) && CheckContactCapabilities(hContact, CAPF_OSCAR_FILE) && wLen == 0x40)
              szClient = "libgaim"; // Gaim 1.5.1 most probably
            else if (MatchCap(caps, wLen, &capAimChat, 0x10) && CheckContactCapabilities(hContact, CAPF_OSCAR_FILE) && wLen == 0x20)
              szClient = "Easy Message";
            else if (MatchCap(caps, wLen, &capAimIcon, 0x10) && MatchCap(caps, wLen, &capAimChat, 0x10) && CheckContactCapabilities(hContact, CAPF_UTF) && wLen == 0x30)
              szClient = "Meebo";
            else if (MatchCap(caps, wLen, &capAimIcon, 0x10) && CheckContactCapabilities(hContact, CAPF_UTF) && wLen == 0x20)
              szClient = "PyICQ-t Jabber Transport";
            else if (MatchCap(caps, wLen, &capAimIcon, 0x10) && MatchCap(caps, wLen, &capIcqLite, 0x10) && CheckContactCapabilities(hContact, CAPF_UTF | CAPF_XTRAZ))
              szClient = "PyICQ-t Jabber Transport";
            else if (CheckContactCapabilities(hContact, CAPF_UTF | CAPF_SRV_RELAY | CAPF_ICQDIRECT | CAPF_TYPING) && wLen == 0x40)
              szClient = "Agile Messenger"; // Smartphone 2002
          }
        }
      }
    }
    else if (!dwUin)
    { // detect AIM clients
      if (caps)
      {
        if (capId = MatchCap(caps, wLen, &capAimOscar, 8))
        { // AimOscar Signature
          DWORD aver = (*capId)[0xC] << 0x18 | (*capId)[0xD] << 0x10 | (*capId)[0xE] << 8 | (*capId)[0xF];
          DWORD mver = (*capId)[0x8] << 0x18 | (*capId)[0x9] << 0x10 | (*capId)[0xA] << 8 | (*capId)[0xB];

          szClient = MirandaVersionToStringEx(szClientBuf, 0, "AimOscar", aver, mver);
          bMirandaIM = TRUE;
        }
        else if (capId = MatchCap(caps, wLen, &capSim, 0xC))
        { // Sim is universal
          unsigned ver1 = (*capId)[0xC];
          unsigned ver2 = (*capId)[0xD];
          unsigned ver3 = (*capId)[0xE];

          makeClientVersion(szClientBuf, "SIM ", ver1, ver2, ver3, 0);
          if ((*capId)[0xF] & 0x80) 
            strcat(szClientBuf,"/Win32");
          else if ((*capId)[0xF] & 0x40) 
            strcat(szClientBuf,"/MacOS X");

          szClient = szClientBuf;
        }
        else if (capId = MatchCap(caps, wLen, &capKopete, 0xC))
        {
          unsigned ver1 = (*capId)[0xC];
          unsigned ver2 = (*capId)[0xD];
          unsigned ver3 = (*capId)[0xE];
          unsigned ver4 = (*capId)[0xF];

          makeClientVersion(szClientBuf, "Kopete ", ver1, ver2, ver3, ver4);

          szClient = szClientBuf;
        }
        else if (MatchCap(caps, wLen, &capIm2, 0x10))
        { // IM2 extensions
          szClient = cliIM2;
        }
        else if (MatchCap(caps, wLen, &capNaim, 0x8))
        {
          szClient = "naim";
        }
        else if (MatchCap(caps, wLen, &capAimIcon, 0x10) && MatchCap(caps, wLen, &capAimChat, 0x10) && CheckContactCapabilities(hContact, CAPF_UTF) && wLen == 0x30)
          szClient = "Meebo";
        else
          szClient = "AIM";
      }
      else
        szClient = "AIM";
    }
  }
  if (caps && bMirandaIM)
  { // custom miranda packs
    capstr* capId;

    if (capId = MatchCap(caps, wLen, &capMimPack, 4))
    {
      char szPack[16];

      null_snprintf(szPack, 16, " [%.12s]", (*capId)+4);
      strcat(szClient, szPack);
    }
  }

  if (!szClient)
  {
    NetLog_Server("No client identification, put default ICQ client for protocol.");

    *bClientId = 0;

    switch (wVersion)
    {  // client detection failed, provide default clients
      case 6:
        szClient = "ICQ99";
        break;
      case 7:
        szClient = "ICQ 2000/Icq2Go";
        break;
      case 8: 
        szClient = "ICQ 2001-2003a";
        break;
      case 9: 
        szClient = "ICQ Lite";
        break;
      case 0xA:
        szClient = "ICQ 2003b";
    }
  }
  else
  {
    NetLog_Server("Client identified as %s", szClient);
  }

  if (szClient)
  {
    char* szExtra = NULL;

    if (MatchCap(caps, wLen, &capSimpLite, 0x10))
      szExtra = " + SimpLite";
    else if (MatchCap(caps, wLen, &capSimpPro, 0x10))
      szExtra = " + SimpPro";
    else if (MatchCap(caps, wLen, &capIMsecure, 0x10) || MatchCap(caps, wLen, &capIMSecKey1, 6) || MatchCap(caps, wLen, &capIMSecKey2, 6))
      szExtra = " + IMsecure";

    if (szExtra)
    {
      if (szClient != szClientBuf)
      {
        strcpy(szClientBuf, szClient);
        szClient = szClientBuf;
      }
      strcat(szClient, szExtra);
    }
  }
  return szClient;
}
