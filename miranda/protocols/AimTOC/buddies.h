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
#ifndef BUDDIES_H
#define BUDDIES_H

extern pthread_mutex_t buddyMutex;
extern int hServerSideList;

HANDLE aim_buddy_get(char *nick, int create, int inlist, int noadd, char *group);
void aim_buddy_updatemode(HANDLE hContact);
void aim_buddy_offlineall();
void aim_buddy_delete(HANDLE hContact);
void aim_buddy_update(char *nick, int online, int type, int idle, int evil, time_t signon, time_t idle_time);
void aim_buddy_parseconfig(char *config);
void aim_buddy_updateconfig(int ssilist);
int aim_buddy_cfglen();
void aim_buddy_delaydeletefree();

#endif
