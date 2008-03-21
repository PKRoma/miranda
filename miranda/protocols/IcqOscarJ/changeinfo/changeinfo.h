// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2001-2004 Richard Hughes, Martin Öberg
// Copyright © 2004-2008 Joe Kucera, Bio
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// -----------------------------------------------------------------------------
//
// File name      : $URL$
// Revision       : $Revision$
// Last change on : $Date$
// Last change by : $Author$
//
// DESCRIPTION:
//
//  ChangeInfo Plugin stuff
//
// -----------------------------------------------------------------------------


#ifndef __CHANGEINFO_H
#define __CHANGEINFO_H


#ifndef AW_SLIDE
#define SPI_GETCOMBOBOXANIMATION 0x1004
#define AW_SLIDE      0x40000
#define AW_ACTIVATE      0x20000
#define AW_VER_POSITIVE    0x4
#define UDM_SETPOS32            (WM_USER+113)
#define UDM_GETPOS32            (WM_USER+114)
#endif


#define LI_DIVIDER       0
#define LI_STRING        1
#define LI_LIST          2
#define LI_LONGSTRING    3
#define LI_NUMBER        4
#define LIM_TYPE         0x0000FFFF
#define LIF_ZEROISVALID  0x80000000
#define LIF_SIGNED       0x40000000
#define LIF_PASSWORD     0x20000000
#define LIF_CHANGEONLY   0x10000000

typedef struct {
  char *szDescription;
  unsigned displayType;    //LI_ constant
  int dbType;              //DBVT_ constant
  char *szDbSetting;
  void *pList;
  int listCount;
  LPARAM value;
  int changed;
} SettingItem;

typedef struct {
  int id;
  char *szValue;
} ListTypeDataItem;

// contants.c
extern SettingItem setting[];
extern const int settingCount;

//dlgproc.c
BOOL CALLBACK ChangeInfoDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

//editstring.c
void  BeginStringEdit(int iItem,RECT *rc,int i,WORD wVKey);
void  EndStringEdit(int save);
int   IsStringEditWindow(HWND hwnd);
char* BinaryToEscapes(char *str);

//editlist.c
void BeginListEdit(int iItem,RECT *rc,int i,WORD wVKey);
void EndListEdit(int save);
int IsListEditWindow(HWND hwnd);

#endif /* __CHANGEINFO_H */
