/*
AOL Instant Messenger Plugin for Miranda IM

Copyright © 2003-2004 Robert Rainwater

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
#ifndef GROUPCHAT_H
#define GROUPCHAT_H

#define AIM_GROUPCHAT_DEFEXCHANGE 4

#define GC_LOGOUT            (WM_USER+2)
#define GC_ADDUSER           (WM_USER+3)
#define GC_REMUSER           (WM_USER+4)
#define GC_UPDATEBUDDY       (WM_USER+5)
#define GC_QUIT		         (WM_USER+6)
#define GC_SCROLLLOGTOBOTTOM (WM_USER+7)
#define GC_FLASHWINDOW       (WM_USER+8)
#define GC_SPLITTERMOVE      (WM_USER+9)
#define GC_REFRESH           (WM_USER+10)
#define GC_UPDATECOUNT       (WM_USER+11)

typedef struct
{
    int dwExchange;
    int dwRoom;
    char szRoom[128];
    HWND hwnd;
    int dwMin;
}
TTOCRoom;

typedef struct
{
    int dwRoom;
    char szUser[128];
    char szMessage[512];
}
TTOCRoomMessage;

typedef struct
{
    int dwType;
    TTOCRoomMessage *msg;
    char szUser[128];
    char szRoom[128];
}
TTOCChannelMessage;
#define AIM_GCHAT_CHANPART 1
#define AIM_GCHAT_CHANJOIN 2
#define AIM_GCHAT_CHANMESS 3

struct MessageWindowData
{
    int roomid;
    int splitterY, originalSplitterY;
    SIZE minEditBoxSize;
    HBRUSH hBkgBrush;
    int nFlash;
    HWND hStatus;
};

struct aim_gchat_chatinfo
{
    char szRoom[128];
    char szUser[128];
    char szMsg[256];
    char chatid[32];
};

#define AIM_GCHAT_PREFIX "ChatNum"

void aim_gchat_getlogdir(char *szLog, int size);
void aim_gchat_logchat(char *szRoom, char *szMsg);
void aim_gchat_joinrequest(char *room, int exchange);
void aim_gchat_chatinvite(char *szRoom, char *szUser, char *chatid, char *msg);
void aim_gchat_updatechats();
void aim_gchat_updatemenu();
void aim_gchat_create(int dwRoom, char *szRoom);
void aim_gchat_delete(int dwRoom);
void aim_gchat_sendmessage(int dwRoom, char *szUser, char *szMessage, int whisper);
void aim_gchat_updatebuddy(int dwRoom, char *szUser, int joined);
void aim_gchat_init();
void aim_gchat_destroy();

#endif
