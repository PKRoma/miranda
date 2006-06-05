/*
AOL Instant Messenger Plugin for Miranda IM

Copyright © 2003-2005 Robert Rainwater

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
#ifndef UTILS_H
#define UTILS_H

typedef struct
{
    HANDLE hList;
    HWND hwnd;
}
SIMPLEWINDOWLISTENTRY;

int aim_util_isonline();
int aim_util_netsend(char *data, int datalen);
void aim_util_broadcaststatus(int s);
int aim_util_userdeleted(WPARAM wParam, LPARAM lParam);
int aim_util_dbsettingchanged(WPARAM wParam, LPARAM lParam);
void aim_util_striphtml(char *dest, const char *src, size_t destsize);
int aim_util_escape(char *msg);
void aim_util_browselostpwd();
int aim_util_randomnum(int lowrange, int highrange);
char *aim_util_normalize(const char *s);
unsigned char *aim_util_roastpwd(char *pass);
void aim_util_statusupdate();
void aim_util_parseurl(char *url);
void aim_util_formatnick(char *nick);
int aim_util_isme(char *nick);
HANDLE aim_util_winlistalloc();
void aim_util_winlistadd(HANDLE hList, HWND hwnd);
void aim_util_winlistdel(HANDLE hList, HWND hwnd);
void aim_util_winlistbroadcast(HANDLE hList, UINT message, WPARAM wParam, LPARAM lParam);
int aim_util_isicquserbyhandle(HANDLE hContact);
int aim_util_isicquser(char *sn);
int aim_util_shownotification(char *title, char *info, DWORD flags);
void aim_utils_logversion();
void aim_util_base64decode(char *in, char **out);

#endif
