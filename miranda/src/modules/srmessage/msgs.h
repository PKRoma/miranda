/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2003 Miranda ICQ/IM project, 
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
*/
#include <richedit.h>
#include <richole.h>
#define MSGERROR_CANCEL	0
#define MSGERROR_RETRY	    1

struct NewMessageWindowLParam {
	HANDLE hContact;
	int isSend;
	const char *szInitialText;
};

struct MessageSendInfo {
	HANDLE hContact;
	HANDLE hSendId;
};

struct MessageWindowData {
	HANDLE hContact;
	HANDLE hDbEventFirst,hDbEventLast;
	struct MessageSendInfo *sendInfo;
	int sendCount;
	HANDLE hAckEvent;
	HANDLE hNewEvent;
	int showTime;
	int multiple;
	HBRUSH hBkgBrush;
	int splitterY,originalSplitterY;
	int multiSplitterX;
	HWND hwndToolTips;
	char *sendBuffer;
	HICON hIcons[6];
	SIZE minEditBoxSize;
	int isSend;
	int lineHeight;
	int windowWasCascaded;
	int nFlash;
	int nFlashMax;
};

#define HM_EVENTSENT         (WM_USER+10)
#define DM_REMAKELOG         (WM_USER+11)
#define HM_DBEVENTADDED      (WM_USER+12)
#define DM_CASCADENEWWINDOW  (WM_USER+13)
#define DM_OPTIONSAPPLIED    (WM_USER+14)
#define DM_SPLITTERMOVED     (WM_USER+15)
#define DM_UPDATETITLE       (WM_USER+16)
#define DM_APPENDTOLOG       (WM_USER+17)
#define DM_ERRORDECIDED      (WM_USER+18)
#define DM_SCROLLLOGTOBOTTOM (WM_USER+19)

struct CREOleCallback {
	IRichEditOleCallbackVtbl *lpVtbl;
	unsigned refCount;
	IStorage *pictStg;
	int nextStgId;
};

BOOL CALLBACK DlgProcMessage(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int InitOptions(void);
BOOL CALLBACK ErrorDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
char *QuoteText(char *text,int charsPerLine,int removeExistingQuotes);
int DbEventIsShown(DBEVENTINFO *dbei);
void StreamInEvents(HWND hwndDlg,HANDLE hDbEventFirst,int count,int fAppend);
void LoadMsgLogIcons(void);
void FreeMsgLogIcons(void);

#define MSGFONTID_MYMSG		0
#define MSGFONTID_MYURL		1
#define MSGFONTID_MYFILE	2
#define MSGFONTID_YOURMSG	3
#define MSGFONTID_YOURURL	4
#define MSGFONTID_YOURFILE	5
#define MSGFONTID_MYNAME	6
#define MSGFONTID_MYTIME	7
#define MSGFONTID_MYCOLON	8
#define MSGFONTID_YOURNAME	9
#define MSGFONTID_YOURTIME	10
#define MSGFONTID_YOURCOLON	11
void LoadMsgDlgFont(int i,LOGFONT *lf,COLORREF *colour);
extern const int msgDlgFontCount;

#define LOADHISTORY_UNREAD    0
#define LOADHISTORY_COUNT     1
#define LOADHISTORY_TIME      2

#define SRMSGDEFSET_SPLIT          1
#define SRMSGDEFSET_SHOWBUTTONLINE 1
#define SRMSGDEFSET_SHOWINFOLINE   1
#define SRMSGDEFSET_SHOWQUOTE      0
#define SRMSGDEFSET_AUTOPOPUP      0
#define SRMSGDEFSET_POPUPMIN       0
#define SRMSGDEFSET_AUTOMIN        0
#define SRMSGDEFSET_AUTOCLOSE      0
#define SRMSGDEFSET_CLOSEONREPLY   1
#define SRMSGDEFSET_SAVEPERCONTACT 0
#define SRMSGDEFSET_CASCADE        1
#define SRMSGDEFSET_SENDONENTER    0
#define SRMSGDEFSET_SENDONDBLENTER 0

#define SRMSGDEFSET_LOADHISTORY    LOADHISTORY_UNREAD
#define SRMSGDEFSET_LOADCOUNT      10
#define SRMSGDEFSET_LOADTIME       5

#define SRMSGDEFSET_SHOWLOGICONS   1
#define SRMSGDEFSET_HIDENAMES      0
#define SRMSGDEFSET_SHOWTIME       0
#define SRMSGDEFSET_SHOWDATE       0
#define SRMSGDEFSET_SHOWREADNEXT   0
#define SRMSGDEFSET_SHOWURLS       1
#define SRMSGDEFSET_SHOWFILES      1
#define SRMSGDEFSET_SHOWSENT       1
#define SRMSGDEFSET_BKGCOLOUR      GetSysColor(COLOR_WINDOW)
