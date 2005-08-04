/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2004 Miranda ICQ/IM project, 
all portions of this codebase are copyrighted to the people 
listed in contributors.txt.

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

Avatar service - load and maintain contact avatars

(c) 2005 by Nightwish, silvercircle@gmail.com

*/

#ifndef _M_AVATARS_H
#define _M_AVATARS_H

struct avatarCacheEntry {
    DWORD cbSize;
    HANDLE hContact;
    HBITMAP hbmPic;
    double dAspect;
    LONG bmHeight, bmWidth;
};

#define INITIAL_AVATARCACHESIZE 100
#define AV_QUEUESIZE 500
    
// obtain the bitmap handle of the avatar for the given contact
// wParam = (HANDLE)hContact
// lParam = struct avatarCacheEntry *cacheEntry
// returns: hbitmap on success, 0 on failure (avatar not yet ready or not available)
// if it returns a failure, the avatar may be ready later and the caller may receive
// a notification via ME_AV_AVATARCHANGED

#define MS_AV_GETAVATARBITMAP "SV_Avatars/GetAvatar"

// fired when the contacts avatar changes
// wParam = hContact
// lParam = struct avatarCacheEntry *cacheEntry
 
#define ME_AV_AVATARCHANGED "SV_Avatars/AvatarChanged"

#endif
