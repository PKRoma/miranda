/*
Miranda IM Help Plugin
Copyright (C) 2002 Richard Hughes

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

//dlgboxsubclass.cpp
int InstallDialogBoxHook(void);
int RemoveDialogBoxHook(void);

//utils.cpp
#define CTLTYPE_UNKNOWN   0
#define CTLTYPE_DIALOG    1
#define CTLTYPE_BUTTON    2
#define CTLTYPE_CHECKBOX  3
#define CTLTYPE_RADIO     4
#define CTLTYPE_TEXT      5
#define CTLTYPE_IMAGE     6
#define CTLTYPE_EDIT      7
#define CTLTYPE_GROUP     8
#define CTLTYPE_COMBO     9
#define CTLTYPE_LIST      10
#define CTLTYPE_SPINEDIT  11
#define CTLTYPE_PROGRESS  12
#define CTLTYPE_TRACKBAR  13
#define CTLTYPE_LISTVIEW  14
#define CTLTYPE_TREEVIEW  15
#define CTLTYPE_DATETIME  16
#define CTLTYPE_IP        17
#define CTLTYPE_STATUSBAR 18
#define CTLTYPE_HYPERLINK 19
#define CTLTYPE_CLC       20
#define CTLTYPE_SCROLLBAR 21
#define CTLTYPE_ANIMATION 22
#define CTLTYPE_HOTKEY    23
#define CTLTYPE_TABS      24
#define CTLTYPE_COLOUR    25
extern const char *szControlTypeNames[];
int GetControlType(HWND hwndCtl);
char *CreateDialogIdString(HWND hwndDlg);
#ifdef EDITOR
int GetControlTitle(HWND hwndCtl,char *pszTitle,int cbTitle);
#endif
struct ResizableCharBuffer {
	char *sz;
	int iEnd,cbAlloced;
};
void AppendCharToCharBuffer(struct ResizableCharBuffer *rcb,char c);
void AppendToCharBuffer(struct ResizableCharBuffer *rcb,const char *fmt,...);

//helpdlg.cpp
#define M_CHANGEHELPCONTROL  (WM_USER+0x100)
#define M_HELPDOWNLOADED     (WM_USER+0x101)
#ifdef EDITOR
#define M_UPLOADCOMPLETE     (WM_USER+0x102)
#endif
#define M_LOADHELP           (WM_USER+0x103)
#define M_HELPDOWNLOADFAILED (WM_USER+0x104)
#define M_LANGLIST           (WM_USER+0x105)  //wParam=nLangs, lParam=(int*)langs (free() when done)
#define M_LANGLISTFAILED     (WM_USER+0x106)
BOOL CALLBACK HelpDlgProc(HWND hwndDlg,UINT message,WPARAM wParam,LPARAM lParam);

//streaminout.cpp
char *GetHtmlTagAttribute(const char *pszTag,const char *pszAttr);
void StreamInHtml(HWND hwndEdit,const char *szHtml);
#ifdef EDITOR
char *StreamOutHtml(HWND hwndEdit);
#endif
#define TEXTSIZE_BIG	  18	  //in half points
#define TEXTSIZE_NORMAL	  16
#define TEXTSIZE_SMALL    13
#ifndef EDITOR
void FreeHyperlinkData(void);
int IsHyperlink(int cpMin,int cpMax,char **ppszLink);
#endif

//datastore.cpp
void InitDialogCache(void);
void FreeDialogCache(void);
#define GCHF_DONTDOWNLOAD  1
int GetControlHelp(const char *pszDlgId,const char *pszModule,int ctrlId,char **ppszTitle,char **ppszText,int *pType,DWORD flags);
#ifdef EDITOR
void SetControlHelp(const char *pszDlgId,const char *pszModule,int ctrlId,char *pszTitle,char *pszText,int type);
void UploadDialogCache(void);
#endif
int GetLanguageList(HWND hwndReply);

//options.cpp
int InitOptions(void);
#define OPTION_SERVER  1
#define OPTION_PATH    2
#define OPTION_LANGUAGE 3
#ifdef EDITOR
#define OPTION_AUTH    4
#endif
int GetStringOption(int id,char *pStr,int cbStr);
