/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2006-2010 Boris Krasnovskiy.
Copyright (c) 2003-2005 George Hazan.
Copyright (c) 2002-2003 Richard Hughes (original version).

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "msn_global.h"

void replaceStr(char*& dest, const char* src)
{
	if (src != NULL) 
	{
		mir_free(dest);
		dest = mir_strdup(src);
	}	
}

void replaceStr(wchar_t*& dest, const wchar_t* src)
{
	if (src != NULL) 
	{
		mir_free(dest);
		dest = mir_wstrdup(src);
	}	
}

static TCHAR* a2tf(const TCHAR* str, bool unicode)
{
	if (str == NULL)
		return NULL;

	return unicode ? mir_tstrdup(str) : mir_a2t((char*)str);
}

void overrideStr(TCHAR*& dest, const TCHAR* src, bool unicode, const TCHAR* def)
{
	mir_free(dest);
	dest = NULL;

	if (src != NULL)
		dest = a2tf(src, unicode);
	else if (def != NULL)
		dest = mir_tstrdup(def);
}

char* __fastcall ltrimp(char* str)
{
	if (str == NULL) return NULL;
	char* p = str;

	for (;;)
	{
		switch (*p)
		{
		case ' ': case '\t': case '\n': case '\r':
			++p; break;
		default:
			return p;
		}
	}
}

char* __fastcall rtrim(char *string)
{
   char* p = string + strlen(string) - 1;

   while (p >= string)
   {  
	   if (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\r')
		 break;

		*p-- = 0;
   }
   return string;
}

wchar_t* __fastcall rtrim(wchar_t* string)
{
   wchar_t* p = string + wcslen(string) - 1;

   while (p >= string)
   {  
	   if (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\r')
		 break;

		*p-- = 0;
   }
   return string;
}

char* arrayToHex(BYTE* data, size_t datasz)
{
	char* res = (char*)mir_alloc(2 * datasz + 1);

	char* resptr = res;
	for (unsigned i=0; i<datasz ; i++)
	{
		const BYTE ch = data[i];

		const char ch0 = ch >> 4;
		*resptr++ = (ch0 <= 9) ? ('0' + ch0) : (('a' - 10) + ch0);

		const char ch1 = ch & 0xF;
		*resptr++ = (ch1 <= 9) ? ('0' + ch1) : (('a' - 10) + ch1);
	}
	*resptr = '\0';
	return res;
} 

bool txtParseParam (const char* szData, const char* presearch, const char* start, const char* finish, char* param, const int size)
{
	const char *cp, *cp1;
	int len;
	
	if (szData == NULL) return false;

	if (presearch != NULL)
	{
		cp1 = strstr(szData, presearch);
		if (cp1 == NULL) return false;
	}
	else
		cp1 = szData;

	cp = strstr(cp1, start);
	if (cp == NULL) return false;
	cp += strlen(start);
	while (*cp == ' ') ++cp;

	if (finish)
	{
		cp1 = strstr(cp, finish);
		if (cp1 == NULL) return FALSE;
		while (*(cp1-1) == ' ' && cp1 > cp) --cp1;
	}
	else
		cp1 = strchr(cp, '\0');

	len = min(cp1 - cp, size - 1);
	memmove(param, cp, len);
	param[len] = 0;

	return true;
} 


/////////////////////////////////////////////////////////////////////////////////////////
// UrlDecode - converts URL chars like %20 into printable characters

static int SingleHexToDecimal(int c)
{
	if (c >= '0' && c <= '9') return c-'0';
	if (c >= 'a' && c <= 'f') return c-'a'+10;
	if (c >= 'A' && c <= 'F') return c-'A'+10;
	return -1;
}

template void UrlDecode(char* str);
#ifdef _UNICODE
template void UrlDecode(wchar_t* str);
#endif

template <class chartype> void UrlDecode(chartype* str)
{
	chartype* s = str, *d = str;

	while(*s)
	{
		if (*s == '%') 
		{
			int digit1 = SingleHexToDecimal(s[1]);
			if (digit1 != -1) 
			{
				int digit2 = SingleHexToDecimal(s[2]);
				if (digit2 != -1) 
				{
					s += 3;
					*d++ = (chartype)((digit1 << 4) | digit2);
					continue;
				}	
			}	
		}
		*d++ = *s++;
	}

	*d = 0;
}

void  HtmlDecode(char* str)
{
	char* p, *q;

	if (str == NULL)
		return;

	for (p=q=str; *p!='\0'; p++,q++) 
	{
		if (*p == '&') 
		{
			if (!strncmp(p, "&amp;", 5)) {	*q = '&'; p += 4; }
			else if (!strncmp(p, "&apos;", 6)) { *q = '\''; p += 5; }
			else if (!strncmp(p, "&gt;", 4)) { *q = '>'; p += 3; }
			else if (!strncmp(p, "&lt;", 4)) { *q = '<'; p += 3; }
			else if (!strncmp(p, "&quot;", 6)) { *q = '"'; p += 5; }
			else { *q = *p;	}
		}
		else 
		{
			*q = *p;
		}
	}
	*q = '\0';
}

char*  HtmlEncode(const char* str)
{
	char* s, *p, *q;
	int c;

	if (str == NULL)
		return NULL;

	for (c=0,p=(char*)str; *p!='\0'; p++) 
	{
		switch (*p) 
		{
		case '&': c += 5; break;
		case '\'': c += 6; break;
		case '>': c += 4; break;
		case '<': c += 4; break;
		case '"': c += 6; break;
		default: c++; break;
		}
	}
	if ((s=(char*)mir_alloc(c+1)) != NULL) 
	{
		for (p=(char*)str,q=s; *p!='\0'; p++) 
		{
			switch (*p) 
			{
			case '&': strcpy(q, "&amp;"); q += 5; break;
			case '\'': strcpy(q, "&apos;"); q += 6; break;
			case '>': strcpy(q, "&gt;"); q += 4; break;
			case '<': strcpy(q, "&lt;"); q += 4; break;
			case '"': strcpy(q, "&quot;"); q += 6; break;
			default: *q = *p; q++; break;
			}
		}
		*q = '\0';
	}

	return s;
}

/////////////////////////////////////////////////////////////////////////////////////////
// UrlEncode - converts printable characters into URL chars like %20

void  UrlEncode(const char* src, char* dest, size_t cbDest)
{
	unsigned char* d = (unsigned char*)dest;
	size_t   i = 0;

	for (const unsigned char* s = (unsigned char*)src; *s; s++) 
	{
		if ((*s <= '/' && *s != '.' && *s != '-') ||
			 (*s >= ':' && *s <= '?') ||
			 (*s >= '[' && *s <= '`' && *s != '_'))
		{
			if (i + 4 >= cbDest) break;

			*d++ = '%';
			_itoa(*s, (char*)d, 16);
			d += 2;
			i += 3;
		}
		else
		{
			if (++i >= cbDest) break;
			*d++ = *s;
	}	}

	*d = '\0';
}


void stripBBCode(char* src)
{
	bool tag = false; 
	char* ps = src;
	char* pd = src;

	while (*ps != 0)
	{
		if (!tag && *ps == '[') 
		{
			char ch = ps[1];
			if (ch  == '/') ch = ps[2];
			tag = ch == 'b' || ch == 'u' || ch == 'i' || ch == 'c' || ch == 'a' ||  ch == 's';
		}
		if (!tag) *(pd++) = *ps;
		else tag = *ps != ']';
		++ps;
	}
	*pd = 0;
}

void stripColorCode(char* src)
{
	bool tag = false; 
	unsigned char* ps = (unsigned char*)src;
	unsigned char* pd = (unsigned char*)src;

	while (*ps != 0)
	{
		if (ps[0] == 0xc2 && ps[1] == 0xb7)
		{
			char ch = ps[2];
			switch (ch)
			{
			case '#':
			case '&':
			case '\'':
			case '@':
			case '0':
				ps += 3;
				continue;

			case '$':
				if (isdigit(ps[3]))
				{
					ps += 3;
					if (isdigit(ps[1]))
					{
						ps += 2;
					}
					else
						++ps;

						if (ps[0] == ',' && isdigit(ps[1]))
						{
							ps += 2;
						if (isdigit(ps[1]))
								ps += 2;
						else
							++ps;
					}
					continue;
				}
				else if (ps[3] == '#')
				{
					ps += 4;
					for (int i=0; i<6; ++i)
						if (isxdigit(*ps)) ++ps;
						else break;
					continue;
				}
				break;
			}
		}
		*(pd++) = *(ps++);
	}
	*pd = 0;
}
