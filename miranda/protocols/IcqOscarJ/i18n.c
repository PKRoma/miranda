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
// File name      : $Source: /cvsroot/miranda/miranda/protocols/IcqOscarJ/i18n.c,v $
// Revision       : $Revision$
// Last change on : $Date$
// Last change by : $Author$
//
// DESCRIPTION:
//
//  Contains helper functions to convert text messages between different
//  character sets.
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"


static BOOL bHasCP_UTF8 = FALSE;


void InitI18N(void)
{
  CPINFO CPInfo;


  bHasCP_UTF8 = GetCPInfo(CP_UTF8, &CPInfo);
}



// Returns true if the buffer only contains 7-bit characters.
BOOL IsUSASCII(const unsigned char* pBuffer, int nSize)
{
  BOOL bResult = TRUE;
  int nIndex;

  for (nIndex = 0; nIndex < nSize; nIndex++)
  {
    if (pBuffer[nIndex] > 0x7F)
    {
      bResult = FALSE;
      break;
    }
  }

  return bResult;
}

// Returns true if the unicode buffer only contains 7-bit characters.
BOOL IsUnicodeAscii(const wchar_t* pBuffer, int nSize)
{
  BOOL bResult = TRUE;
  int nIndex;


  for (nIndex = 0; nIndex < nSize; nIndex++)
  {
    if (pBuffer[nIndex] > 0x7F)
    {
      bResult = FALSE;
      break;
    }
  }

  return bResult;
}


// Scans a string encoded in UTF-8 to verify that it contains
// only valid sequences. It will return 1 if the string contains
// only legitimate encoding sequences; otherwise it will return 0;
// From 'Secure Programming Cookbook', John Viega & Matt Messier, 2003
int UTF8_IsValid(const unsigned char* pszInput)
{
  int nb, i;
  const unsigned char* c = pszInput;

  if (!pszInput) return 0;

  for (c = pszInput; *c; c += (nb + 1))
  {
    if (!(*c & 0x80))
      nb = 0;
    else if ((*c & 0xc0) == 0x80) return 0;
    else if ((*c & 0xe0) == 0xc0) nb = 1;
    else if ((*c & 0xf0) == 0xe0) nb = 2;
    else if ((*c & 0xf8) == 0xf0) nb = 3;
    else if ((*c & 0xfc) == 0xf8) nb = 4;
    else if ((*c & 0xfe) == 0xfc) nb = 5;

    for (i = 1; i<=nb; i++) // we this forward, do not cross end of string
      if ((*(c + i) & 0xc0) != 0x80)
        return 0;
  }

  return 1;
}


// returns ansi string in all cases
char* detect_decode_utf8(const char *from)
{
  char* temp = NULL;

  if (IsUSASCII(from, strlennull(from)) || !UTF8_IsValid(from) || !utf8_decode(from, &temp)) return (char*)from;
  SAFE_FREE((char**)&from);

  return temp;
}


/*
 * The following UTF8 routines are
 *
 * Copyright (C) 2001 Peter Harris <peter.harris@hummingbird.com>
 * Copyright (C) 2001 Edmund Grimley Evans <edmundo@rano.org>
 *
 * under a GPL license
 *
 * --------------------------------------------------------------
 * Convert a string between UTF-8 and the locale's charset.
 * Invalid bytes are replaced by '#', and characters that are
 * not available in the target encoding are replaced by '?'.
 *
 * If the locale's charset is not set explicitly then it is
 * obtained using nl_langinfo(CODESET), where available, the
 * environment variable CHARSET, or assumed to be US-ASCII.
 *
 * Return value of conversion functions:
 *
 *  -1 : memory allocation failed
 *   0 : data was converted exactly
 *   1 : valid data was converted approximately (using '?')
 *   2 : input was invalid (but still converted, using '#')
 *   3 : unknown encoding (but still converted, using '?')
 */



/*
 * Convert a string between UTF-8 and the locale's charset.
 */
unsigned char *make_utf8_string(const wchar_t *unicode)
{
  int size = 0;
  int index = 0;
  int out_index = 0;
  unsigned char* out;
  unsigned short c;

  if (!unicode) return NULL;

  /* first calculate the size of the target string */
  c = unicode[index++];
  while (c)
  {
    if (c < 0x0080) 
      size += 1;
    else if (c < 0x0800) 
      size += 2;
    else 
      size += 3;
    c = unicode[index++];
  }

  out = (unsigned char*)SAFE_MALLOC(size + 1);
  if (out == NULL)
    return NULL;
  index = 0;

  c = unicode[index++];
  while (c)
  {
    if (c < 0x080) 
    {
      out[out_index++] = (unsigned char)c;
    }
    else if (c < 0x800) 
    {
      out[out_index++] = 0xc0 | (c >> 6);
      out[out_index++] = 0x80 | (c & 0x3f);
    }
    else
    {
      out[out_index++] = 0xe0 | (c >> 12);
      out[out_index++] = 0x80 | ((c >> 6) & 0x3f);
      out[out_index++] = 0x80 | (c & 0x3f);
    }
    c = unicode[index++];
  }
  out[out_index] = 0x00;

  return out;
}



wchar_t *make_unicode_string(const unsigned char *utf8)
{
  int size = 0, index = 0, out_index = 0;
  wchar_t *out;
  unsigned char c;

  if (!utf8) return NULL;

  /* first calculate the size of the target string */
  c = utf8[index++];
  while (c) 
  {
    if ((c & 0x80) == 0) 
    {
      index += 0;
    }
    else if ((c & 0xe0) == 0xe0) 
    {
      index += 2;
    }
    else
    {
      index += 1;
    }
    size += 1;
    c = utf8[index++];
  }

  out = (wchar_t*)SAFE_MALLOC((size + 1) * sizeof(wchar_t));
  if (out == NULL)
    return NULL;
  index = 0;

  c = utf8[index++];
  while (c)
  {
    if((c & 0x80) == 0) 
    {
      out[out_index++] = c;
    } 
    else if((c & 0xe0) == 0xe0) 
    {
      out[out_index] = (c & 0x1F) << 12;
      c = utf8[index++];
      out[out_index] |= (c & 0x3F) << 6;
      c = utf8[index++];
      out[out_index++] |= (c & 0x3F);
    }
    else
    {
      out[out_index] = (c & 0x3F) << 6;
      c = utf8[index++];
      out[out_index++] |= (c & 0x3F);
    }
    c = utf8[index++];
  }
  out[out_index] = 0;

  return out;
}



int utf8_encode(const char *from, char **to)
{
  wchar_t *unicode;
  int wchars, err;


  wchars = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, from,
      strlennull(from), NULL, 0);

  if (wchars == 0)
  {
    fprintf(stderr, "Unicode translation error %d\n", GetLastError());
    return -1;
  }

  unicode = (wchar_t*)_alloca((wchars + 1) * sizeof(unsigned short));
  unicode[wchars] = 0;

  err = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, from,
      strlennull(from), unicode, wchars);
  if(err != wchars)
  {
    fprintf(stderr, "Unicode translation error %d\n", GetLastError());
    return -1;
  }

  /* On NT-based windows systems, we could use WideCharToMultiByte(), but
   * MS doesn't actually have a consistent API across win32.
   */
  *to = make_utf8_string(unicode);

  return 0;
}



char *ansi_to_utf8(const char *szAnsi)
{
  char *szUtf = NULL;

  if (strlennull(szAnsi))
  {
    utf8_encode(szAnsi, &szUtf);

    return szUtf;
  }
  else
    return null_strdup("");
}



char *ansi_to_utf8_codepage(const char *szAnsi, WORD wCp)
{
  wchar_t *unicode;
  int wchars = strlennull(szAnsi);

  unicode = (wchar_t*)_alloca((wchars + 1) * sizeof(wchar_t));
  ZeroMemory(unicode, (wchars + 1)*sizeof(wchar_t));

  MultiByteToWideChar(wCp, MB_PRECOMPOSED, szAnsi, wchars, unicode, wchars);

  return make_utf8_string(unicode);
}



// Returns 0 on error, 1 on success
int utf8_decode(const char *from, char **to)
{
  int nResult = 0;

  _ASSERTE(!(*to)); // You passed a non-zero pointer, make sure it doesnt point to unfreed memory

  // Validate the string
  if (!UTF8_IsValid(from))
    return 0;

  // Use the native conversion routines when available
  if (bHasCP_UTF8)
  {
    WCHAR *wszTemp = NULL;
    int inlen = strlennull(from);

    wszTemp = (WCHAR *)_alloca(sizeof(WCHAR) * (inlen + 1));

    // Convert the UTF-8 string to UCS
    if (MultiByteToWideChar(CP_UTF8, 0, from, -1, wszTemp, inlen + 1))
    {
      // Convert the UCS string to local ANSI codepage
      *to = (char*)SAFE_MALLOC(inlen+1);
      if (WideCharToMultiByte(CP_ACP, 0, wszTemp, -1, *to, inlen+1, NULL, NULL))
      {
        nResult = 1;
      }
      else
      {
        SAFE_FREE(&(*to));
      }
    }
  }
  else
  {
    wchar_t *unicode;
    int chars;
    int err;

    unicode = make_unicode_string(from);
    if(unicode == NULL)
    {
      fprintf(stderr, "Out of memory processing string from UTF8 to UNICODE16\n");
      return 0;
    }

    chars = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, unicode, -1, NULL, 0, NULL, NULL);

    if(chars == 0)
    {
      fprintf(stderr, "Unicode translation error %d\n", GetLastError());
      SAFE_FREE(&unicode);
      return 0;
    }

    *to = (char*)SAFE_MALLOC((chars + 1)*sizeof(unsigned char));
    if(*to == NULL)
    {
      fprintf(stderr, "Out of memory processing string to local charset\n");
      SAFE_FREE(&unicode);
      return 0;
    }

    err = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, unicode, -1, *to, chars, NULL, NULL);
    if (err != chars)
    {
      fprintf(stderr, "Unicode translation error %d\n", GetLastError());
      SAFE_FREE(&unicode);
      SAFE_FREE(to);
      return 0;
    }

    SAFE_FREE(&unicode);

    nResult = 1;
  }

  return nResult;
}



// Returns 0 on error, 1 on success
int utf8_decode_static(const char *from, char *to, int to_size)
{
  int nResult = 0;

  _ASSERTE(to); // You passed a zero pointer

  // Validate the string
  if (!UTF8_IsValid(from))
    return 0;

  // Use the native conversion routines when available
  if (bHasCP_UTF8)
  {
    WCHAR *wszTemp = NULL;
    int inlen = strlennull(from);

    wszTemp = (WCHAR *)_alloca(sizeof(WCHAR) * (inlen + 1));

    // Convert the UTF-8 string to UCS
    if (MultiByteToWideChar(CP_UTF8, 0, from, -1, wszTemp, inlen + 1))
    {
      // Convert the UCS string to local ANSI codepage
      if (WideCharToMultiByte(CP_ACP, 0, wszTemp, -1, to, to_size, NULL, NULL))
      {
        nResult = 1;
      }
    }
  }
  else
  {
    wchar_t *unicode = make_unicode_string(from);

    if (unicode == NULL)
    {
      fprintf(stderr, "Out of memory processing string from UTF8 to UNICODE16\n");
      return 0;
    }

    WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, unicode, -1, to, to_size, NULL, NULL);

    SAFE_FREE(&unicode);

    nResult = 1;
  }

  return nResult;
}
