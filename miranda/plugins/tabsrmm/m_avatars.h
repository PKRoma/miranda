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

#define AVS_BITMAP_VALID 1
#define AVS_BITMAP_EXPIRED 2        // the bitmap has been expired from the cache. (unused, currently.
#define AVS_HIDEONCLIST 4           // set, when the avatar has the "hidden" flag. plugins may or may
                                    // not ignore this flag. It does not really affect the avatar itself -
                                    // the bitmap is still valid - it is merely a hint that the user has
                                    // choosen to hide the avatar for a given contact.

struct avatarCacheEntry {
    DWORD cbSize;                   // set to sizeof(struct)
    HANDLE hContact;                // contacts handle, 0, if it is a protocol avatar
    HBITMAP hbmPic;                 // bitmap handle of the picutre itself
    DWORD dwFlags;                  // see above for flag values
    LONG bmHeight, bmWidth;         // bitmap dimensions
    time_t t_lastAccess;            // last access time (currently unused, but plugins should still
                                    // use it whenever they access the avatar. may be used in the future
                                    // to implement cache expiration
    DWORD dwReserved;
    char szFilename[MAX_PATH];      // filename of the avatar (absolute path)
};

#define INITIAL_AVATARCACHESIZE 100

#define AVS_MODULE "AVS_Settings"          // db settings module path
#define PPICT_MODULE "AVS_ProtoPics"   // protocol pictures are saved here

// obtain the bitmap handle of the avatar for the given contact
// wParam = (HANDLE)hContact
// lParam = 0;
// returns: pointer to a struct avatarCacheEntry *, NULL on failure
// if it returns a failure, the avatar may be ready later and the caller may receive
// a notification via ME_AV_AVATARCHANGED
// DONT modify the contents of the returned data structure except t_lastAccess

#define MS_AV_GETAVATARBITMAP "SV_Avatars/GetAvatar"

// protect the current contact picture from being overwritten by automatic
// avatar updates. Actually, it only backups the contact picture filename
// and will used the backuped version until the contact picture gets unlocked
// again. So this service does not disable avatar updates, but it "fakes"
// a locked contact picture to the users of the GetAvatar service.
// 
// wParam = (HANDLE)hContact
// lParam = 1 -> lock the avatar, lParam = 0 -> unlock

#define MS_AV_PROTECTAVATAR "SV_Avatars/ProtectAvatar"

// set a local contact picture for the given hContact
// 
// wParam = (HANDLE)hContact
// lParam = either a full picture filename or NULL. If lParam == NULL, the service
// will open a file selection dialog.

#define MS_AV_SETAVATAR "SV_Avatars/SetAvatar"

// Call avatar option dialog for contact
// 
// wParam = (HANDLE)hContact

#define MS_AV_CONTACTOPTIONS "SV_Avatars/ContactOptions"

// fired when the contacts avatar changes
// wParam = hContact
// lParam = struct avatarCacheEntry *cacheEntry
// the event CAN pass a NULL pointer in lParam which means that the avatar has changed,
// but is no longer valid (happens, when a contact removes his avatar, for example).
// DONT DESTROY the bitmap handle passed in the struct avatarCacheEntry *
// 
// It is also possible that this event passes 0 as wParam (hContact), in which case,
// a protocol picture (pseudo - avatar) has been changed. 
 
#define ME_AV_AVATARCHANGED "SV_Avatars/AvatarChanged"

#endif
