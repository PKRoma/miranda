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
#ifndef DBRW_EVENTS_H
#define DBRW_EVENTS_H

void events_init();
void events_destroy();
int events_getCount(WPARAM wParam, LPARAM lParam);
int events_add(WPARAM wParam, LPARAM lParam);
int events_delete(WPARAM wParam, LPARAM lParam);
int events_getBlobSize(WPARAM wParam, LPARAM lParam);
int events_get(WPARAM wParam, LPARAM lParam);
int events_markRead(WPARAM wParam, LPARAM lParam);
int events_getContact(WPARAM wParam, LPARAM lParam);
int events_findFirst(WPARAM wParam, LPARAM lParam);
int events_findFirstUnread(WPARAM wParam, LPARAM lParam);
int events_findLast(WPARAM wParam, LPARAM lParam);
int events_findNext(WPARAM wParam, LPARAM lParam);
int events_findPrev(WPARAM wParam, LPARAM lParam);

#endif
