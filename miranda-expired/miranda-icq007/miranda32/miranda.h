/*
Miranda ICQ: the free icq client for MS Windows 
Copyright (C) 2000  Roland Rabien

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

For more information, e-mail figbug@users.sourceforge.net
*/

#ifndef _global_h
#define _global_h

//#define WINVER 0x0500
//#define _WIN32_WINNT 0x0500

#include <windows.h>
#include <commctrl.h>


//If u dont have the Win2k SDK
//#ifdef _NOSDK //in ops, get rid of _NOSDK to ignore the contents of this ifdef
	#define LWA_ALPHA 0x02
	#define WS_EX_LAYERED 0x080000
//#endif

#define MIRANDABOT		0L
#define MIRANDAVERSTR   "0.0.7" //dont forget to update the Version info Res
#define VERSIONSTR		"http://miranda-icq.sourceforge.net/cgi-bin/version?0.0.7"
#define HELPSTR			"http://miranda-icq.sourceforge.net/help"
#define MIRANDANAME		"Miranda"
#define MIRANDAWEBSITE  "http://Miranda-icq.SourceForge.net/"


// defines
#define TM_CHECKDATA	0x01
#define TM_KEEPALIVE	0x02
#define TM_OTHER		0x03
#define TM_ONCE			0x04
#define TM_TIMEOUT		0x05
#define TM_AUTOALPHA	0x06
#define TM_MSGFLASH		0x07
#define TM_CHECKDATA_MSN 0x08



#define TIMEOUT_TIMEOUT 20000 //timeou in mili seconds
#define TIMEOUT_MSGFLASH 500

#define TIMEOUT_MSGSEND 10000

//hotkey defines
#define HOTKEY_SHOWHIDE 0x01
#define HOTKEY_SETFOCUS 0x02
#define HOTKEY_NETSEARCH 0x04

//history defines - type of history event (passed to AddToHistory)
#define HISTORY_MSGRECV 1
#define HISTORY_MSGSEND 2
#define HISTORY_URLRECV 3
#define HISTORY_URLSEND 4
#define HISTORY_AUTHREQ 5
#define HISTORY_ADDED 6

#define WT_RECVMSG		0x01
#define WT_SENDMSG		0x02
#define WT_SENDURL		0x03
#define WT_ADDED		0x04
#define WT_RECVURL		0x05
#define WT_AUTHREQ		0x06
#define WT_ADDCONT		0x07
#define WT_RESULT		0x08
#define WT_DETAILS		0x09
#define WT_RECVFILE		0x10
#define WT_NEWICQ		0x11

//which slot (setstatustextEx) for each IM
#define STATUS_SLOT_ICQ 0
#define STATUS_SLOT_MSN 1


#define MAX_CONTACT		1024

//systray defs
#define TI_CREATE		0
#define TI_DESTROY		1
#define TI_UPDATE		2

#define ET_AMES			0x0001
#define ET_RIH			0x0002
#define ET_RINH			0x0004
#define ET_ANET			0x0008
#define ET_USTAT		0x0010
#define ET_PWM			0x0020

// message types
#define TI_CALLBACK		WM_USER + 4
#define TI_USERFOUND	WM_USER + 5
#define TI_INFOREPLY	WM_USER + 6
#define TI_EXTINFOREPLY WM_USER + 7
#define TI_SRVACK		WM_USER + 8
#define TI_SORT			WM_USER + 9
#define TI_SHOW			WM_USER + 10
#define TI_POP			WM_USER + 11

// some kick ass o rama flags
#define FG_MIN2TRAY		0x01
#define FG_ONTOP		0x02
#define FG_POPUP		0x04
#define FG_TRANSP		0x08
#define FG_TOOLWND		0x10
#define FG_ONECLK		0x20
#define FG_HIDEOFFLINE  0x40

#define UF_IGNORE		0x01
#define UF_ALWAYSSHOW	0x02

#define MAX_PROFILESIZE 30

#define MAX_GROUPS 1000


//MSN Definitions

#define MSN_UHANDLE_LEN 130
#define MSN_NICKNAME_LEN 30	
#define MSN_PASSWORD_LEN 30
#define MSN_BASEUIN (MAX_GROUPS+1) //base (pseudo icq)UIN to use for MSN contacts

#define MSN_AUTHINF_LEN 30 //how long the authinf (from the server) can be 
#define MSN_SID_LEN 30 //how logn the sid (from the server) can be

#define MSN_SES_MAX_USERS 10 //how many ppl u can chat with in _a_ session


#define MSN_SESSIONPEND "MSN_NEEDSESSION" //Prop string for pending session sendmsg

#define TI_MSN_TRYAGAIN (WM_USER+108)

//(SUB)STATUS DEFINITONS
#define MSN_STATUS_OFFLINE "FLN" 
#define MSN_STATUS_ONLINE "NLN" 
#define MSN_STATUS_BUSY "BSY"
#define MSN_STATUS_IDLE "IDL"
#define MSN_STATUS_BRB "BRB"
#define MSN_STATUS_AWAY "AWY"
#define MSN_STATUS_PHONE "PHN"
#define MSN_STATUS_LUNCH "LUN"

#define MSN_INTSTATUS_OFFLINE 30
#define MSN_INTSTATUS_ONLINE (MSN_INTSTATUS_OFFLINE+1)
#define MSN_INTSTATUS_BUSY (MSN_INTSTATUS_OFFLINE+2)
#define MSN_INTSTATUS_IDLE 0
#define MSN_INTSTATUS_BRB (MSN_INTSTATUS_OFFLINE+3)
#define MSN_INTSTATUS_AWAY (MSN_INTSTATUS_OFFLINE+4)
#define MSN_INTSTATUS_PHONE (MSN_INTSTATUS_OFFLINE+5)
#define MSN_INTSTATUS_LUNCH (MSN_INTSTATUS_OFFLINE+6)


///END MSN


//TITLEBAR
#define LIGHT_COLOR		250
#define DARK_COLOR		100


// global types

typedef struct {
	unsigned long uin;
	const char *msg;
	const char *url;
	const char *descr;
	const char *nick;
	const char *email;
	const char *first;
	const char *last;
	char auth;
	unsigned long seq;
	// the day
	unsigned char hour;
	unsigned char minute;
	unsigned char day;
	unsigned char month;
	unsigned short year;

	int msgid; //id to the src msgque array
} DLG_DATA;

typedef struct WNODE HWNDLIST;
typedef struct WNODE {
	HWND hwnd;
	HWNDLIST *next;
	int windowtype;
	unsigned int uin;
	
	unsigned int ack;

	
} HWNDNODE;

typedef struct {
	HWND				hwndStatus;
	HWND				hwndContact;
	HWND				hwndDlg;
	HIMAGELIST			himlIcon;
	
	BOOL				net_activity;
	HWNDLIST			*hwndlist;
	
	BOOL				hidden;
	int					sortmode;
	int					askdisconnect;
	int					useini;
	char				inifile[MAX_PATH];
	
} OPTIONS_RT;

enum CONTACTTYPE
{
	CT_ICQ,
	CT_MSN
};

typedef struct{
	int					structsize;
	enum CONTACTTYPE	id;			//ICQ or msn contact?

	//V1 stuff
	unsigned int		uin;  //will be ignored for MSN users
	char				nick[30];
	char				first[30];
	char				last[30];
	char				email[130]; //used as the handle for msn
									// (len has been upped form 50 to 130 for msn support)
	int					status;
	unsigned int		flags;

	int					groupid;	//which group does the contact belogn to 
									
} CONTACT;

typedef struct {
	int					structsize;
	int					grp;
	int					expanded; //BOOL, is the group expanded or not?
	int					parent;
	char				name[30];
	HTREEITEM			item;
} CNT_GROUP;

typedef struct {
	unsigned int		uin;
	char				nick[30];
	char				first[30];
	char				last[30];
	char				email[50];
	int					status;
	unsigned int		flags;
} CONTACT_OLD;


//run type data for contacts
typedef struct 
{
	unsigned long	IP;
	unsigned long	REALIP;

	BOOL MSN_PENDINGSEND; ///are we waiting for a session to send our msg
}RTOCONTACT;

typedef struct 
{
	unsigned long		xpos;
	unsigned long		ypos;
	unsigned long		width;
	unsigned long		height;
}WINPOS;

typedef struct {
	BOOL		enabled;

	int			sendmethod; //server? direct TCP?
	//icq server stuff
	unsigned long		uin;
	char				password[16];
	char				hostname[64];
	int					port;
	
	// proxy
	int					proxyuse;
	char				proxyhost[128];
	int					proxyport;
	int					proxyauth;
	char				proxyname[128];
	char				proxypass[128];
	//RTO ICQ STUFF
	BOOL				online;
	unsigned long		status;

	int					laststatus;

}ICQ_INFO;


typedef struct tagMSN_SESSION{
	SOCKET con;

	HWND pendingsend;
	char authinf[MSN_AUTHINF_LEN];
	char sid[MSN_SID_LEN];

	int usercnt;
	char users[MSN_SES_MAX_USERS][MSN_UHANDLE_LEN];

	char tmp_otheruser[MSN_UHANDLE_LEN];
}MSN_SESSION;

typedef struct {
	BOOL			enabled;
	
	char			uhandle[MSN_UHANDLE_LEN];
	
	char			password[MSN_PASSWORD_LEN];
	

	//RTO STUFF
	int				status;
	char			nickname[MSN_NICKNAME_LEN];
	SOCKET			sNS;
	MSN_SESSION		*SS; //each msn session
	int				sscnt; //how many ss instances
	BOOL			logedin; //have been authed

	char			laststatus[4];

}MSN_INFO;

enum DOCKPOS
{
	DOCK_NONE,
	DOCK_LEFT,
	DOCK_RIGHT
};

typedef struct {
	// postions on all the wnds
	WINPOS				pos_mainwnd;
	WINPOS				pos_sendmsg;
	WINPOS				pos_sendurl;
	WINPOS				pos_recvmsg;
	WINPOS				pos_recvurl;

	
	// the contact list
	char				contactlist[MAX_PATH];
	char				grouplist[MAX_PATH];
	char				history[MAX_PATH];
	CONTACT				clist[MAX_CONTACT];
	RTOCONTACT			clistrto[MAX_CONTACT]; //runtime info about client
	int					ccount;
	// some funky assed flags
	unsigned int		flags1;
	
	int					alpha;
	int					autoalpha;
	
	// other shit
	int					playsounds;
	//int					defaultmode;
	

	//hotkeys
	int					hotkey_showhide;
	int					hotkey_setfocus;
	int					hotkey_netsearch;
	char				netsearchurl[MAX_PATH];

	//DOCKING
	enum DOCKPOS				dockstate;

	ICQ_INFO			ICQ;
	MSN_INFO			MSN;	
} OPTIONS;

typedef struct {
	unsigned long uin;
	char *nick;
	char *first;
	char *last;
	char *email;
} INFOREPLY;

typedef struct {
	unsigned long uin;
	char *city;
	unsigned short country_code;
	char country_stat;
	char *state;
	unsigned short age;
	char gender;
	char *phone;
	char *hp;
	char *about;
} EXTINFOREPLY;

typedef struct {
	int count;
	int max;
	char **bodys;
} HISTORY;

typedef BOOL (WINAPI *TmySetLayeredWindowAttributes)(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);

#endif