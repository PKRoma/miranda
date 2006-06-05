/*
Change ICQ Details plugin for Miranda IM

Copyright © 2001,2002,2003,2004,2005 Richard Hughes, Martin Öberg

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


#define LI_DIVIDER       0
#define LI_STRING        1
#define LI_LIST          2
#define LI_LONGSTRING    3
#define LI_NUMBER        4
#define LIM_TYPE         0x0000FFFF
#define LIF_ZEROISVALID  0x80000000
#define LIF_SIGNED       0x40000000
#define LIF_PASSWORD     0x20000000
struct SettingItem {
	char *szDescription;
	unsigned displayType;    //LI_ constant
	int dbType;              //DBVT_ constant
	char *szDbSetting;
	void *pList;
	int listCount;
	LPARAM value;
	int changed;
};

struct ListTypeDataItem {
	int id;
	char *szValue;
};

extern SettingItem setting[];
extern const int settingCount;

//db.cpp
void LoadSettingsFromDb(int keepChanged);
void FreeStoredDbSettings(void);
void ClearChangeFlags(void);
int ChangesMade(void);
int SaveSettingsToDb(HWND hwndDlg);

//editstring.cpp
void BeginStringEdit(int iItem,RECT *rc,int i,WORD wVKey);
void EndStringEdit(int save);
int IsStringEditWindow(HWND hwnd);
char *BinaryToEscapes(char *str);

//editlist.cpp
void BeginListEdit(int iItem,RECT *rc,int i,WORD wVKey);
void EndListEdit(int save);
int IsListEditWindow(HWND hwnd);

//upload.cpp
int UploadSettings(HWND hwndParent);