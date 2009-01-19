/*
Plugin of Miranda IM for communicating with users of the AIM protocol.
Copyright (c) 2008-2009 Boris Krasnovskiy
Copyright (C) 2005-2006 Aaron Myles Landwehr

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
#ifndef STRL_H
#define STRL_H
size_t strlcpy(char* dst,const char *src, size_t siz);
size_t wcslcpy(wchar_t *dst, const wchar_t *src, size_t siz);
char* strldup(const char* src,size_t siz);
char* strldup(const char* src);
void strlrep(char*& dest, const char* src);
wchar_t* wcsldup(const wchar_t* src,size_t siz);
char* strlcat(const char* dst,const char *src);
#endif
