/*
dbRW

Copyright (c) 2005-2007 Robert Rainwater

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#ifndef DBRW_UTILS_H
#define DBRW_UTILS_H

#ifdef DBRW_LOGGING
void utils_log_init();
void utils_log_destroy();
void utils_log_fmt(const char *file,int line,const char *fmt,...);
#endif
int utils_setSafetyMode(WPARAM wParam, LPARAM lParam);
int utils_getProfileName(WPARAM wParam, LPARAM lParam);
int utils_getProfilePath(WPARAM wParam, LPARAM lParam);
int utils_encodeString(WPARAM wParam,LPARAM lParam);
int utils_decodeString(WPARAM wParam,LPARAM lParam);
DWORD utils_hashString(const char *szStr);
int utils_private_setting_get_int(const char *setting, int defval);
int utils_private_setting_set_int(const char *setting, int val);
void utils_vacuum_check();
void* utils_mem_alloc(size_t size);
void* utils_mem_realloc(void* ptr, size_t size);
void utils_mem_free(void* ptr);

#endif

