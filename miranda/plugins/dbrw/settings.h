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
#ifndef DBRW_SETTINGS_H
#define DBRW_SETTINGS_H

void settings_init();
void settings_destroy();
int setting_getSetting(WPARAM wParam, LPARAM lParam);
int setting_getSettingStr(WPARAM wParam, LPARAM lParam);
int setting_getSettingStatic(WPARAM wParam, LPARAM lParam);
int setting_freeVariant(WPARAM wParam, LPARAM lParam);
int setting_writeSetting(WPARAM wParam, LPARAM lParam);
int setting_deleteSetting(WPARAM wParam, LPARAM lParam);
int setting_enumSettings(WPARAM wParam, LPARAM lParam);
int setting_modulesEnum(WPARAM wParam, LPARAM lParam);
void settings_emptyContactCache(HANDLE hContact);
int settings_setResident(WPARAM wParam, LPARAM lParam);

#endif
