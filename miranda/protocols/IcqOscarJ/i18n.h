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
//  Helper functions to convert text messages between different character sets.
//
// -----------------------------------------------------------------------------



BOOL IsUSASCII(const unsigned char *pBuffer, int nSize);
BOOL IsUnicodeAscii(const WCHAR *pBuffer, int nSize);
int UTF8_IsValid(const unsigned char* pszInput);

char* detect_decode_utf8(const char *from);

WCHAR *make_unicode_string(const unsigned char *utf8);

unsigned char *make_utf8_string(const WCHAR *unicode);

int utf8_encode(const char *from, char **to);
char *ansi_to_utf8(const char *szAnsi);
char *ansi_to_utf8_codepage(const char *szAnsi, WORD wCp);
int utf8_decode(const char *from, char **to);
int utf8_decode_codepage(const char *from, char **to, WORD wCp);
int utf8_decode_static(const char *from, char *to, int to_size);

void InitI18N(void);
