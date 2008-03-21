// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
//
// Copyright © 2000-2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001-2002 Jon Keating, Richard Hughes
// Copyright © 2002-2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004-2008 Joe Kucera
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
//  Helper functions to convert text messages between different character sets.
//
// -----------------------------------------------------------------------------



BOOL __stdcall IsUSASCII(const char *pBuffer, int nSize);
BOOL __stdcall IsUnicodeAscii(const WCHAR *pBuffer, int nSize);
int  __stdcall UTF8_IsValid(const char *pszInput);

char* __stdcall detect_decode_utf8(const char *from);

WCHAR* __stdcall make_unicode_string(const char *utf8);
WCHAR* __stdcall make_unicode_string_static(const char *utf8, WCHAR *unicode, size_t unicode_len);

char*  __stdcall make_utf8_string(const WCHAR *unicode);
char*  __stdcall make_utf8_string_static(const WCHAR *unicode, char *utf8, size_t utf_size);

char*  __stdcall ansi_to_utf8(const char *szAnsi);
char*  __stdcall ansi_to_utf8_codepage(const char *szAnsi, WORD wCp);
char*  __stdcall tchar_to_utf8(const TCHAR *szTxt);
char*  __stdcall mtchar_to_utf8(const TCHAR *szTxt);

int   __stdcall utf8_encode(const char *from, char **to);
int   __stdcall utf8_decode(const char *from, char **to);
int   __stdcall utf8_decode_codepage(const char *from, char **to, WORD wCp);
int   __stdcall utf8_decode_static(const char *from, char *to, int to_size);

void InitI18N(void);
