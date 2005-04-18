#ifndef SRMM_GLOBALS_H
#define SRMM_GLOBALS_H

#define SMF_SHOWINFO        0x00000001
#define SMF_SHOWBTNS        0x00000002
#define SMF_SENDBTN         0x00000004
#define SMF_SHOWTYPING      0x00000008
#define SMF_SHOWTYPINGWIN   0x00000010
#define SMF_SHOWTYPINGTRAY  0x00000020
#define SMF_SHOWTYPINGCLIST 0x00000040
#define SMF_SHOWICONS       0x00000080
#define SMF_SHOWTIME        0x00000100
#define SMF_AVATAR          0x00000200
#define SMF_SHOWDATE        0x00000400
#define SMF_HIDENAMES       0x00000800
#define SMF_SHOWSECS        0x00001000

#define SMF_ICON_ADD        0
#define SMF_ICON_USERDETAIL 1
#define SMF_ICON_HISTORY    2
#define SMF_ICON_ARROW      3
#define SMF_ICON_TYPING     4

struct GlobalMessageData
{
	unsigned int flags;
	HICON hIcons[5];
	HANDLE hMessageWindowList;
};

void InitGlobals();
void FreeGlobals();
void ReloadGlobals();

extern struct GlobalMessageData *g_dat;

#endif
