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

#define CLCDEFAULT_GRADIENT 0
#define CLCDEFAULT_CORNER 0

#define CLCDEFAULT_COLOR 0xE0E0E0
#define CLCDEFAULT_COLOR2 0xE0E0E0

#define CLCDEFAULT_TEXTCOLOR 0x000000

#define CLCDEFAULT_COLOR2_TRANSPARENT 1

#define CLCDEFAULT_ALPHA 85
#define CLCDEFAULT_MRGN_LEFT 0
#define CLCDEFAULT_MRGN_TOP 0
#define CLCDEFAULT_MRGN_RIGHT 0
#define CLCDEFAULT_MRGN_BOTTOM 0
#define CLCDEFAULT_IGNORE 0

#define ID_EXTBKSELECTION		40100
#define ID_EXTBKEXPANDEDGROUP	40101
#define ID_EXTBKCOLLAPSEDDGROUP	40102
#define ID_EXTBKFIRSTITEM		40103
#define ID_EXTBKSINGLEITEM		40104
#define ID_EXTBKLASTITEM		40105

#define ID_EXTBKEMPTYGROUPS		40106

#define ID_EXTBKFIRSTITEM_NG	40107
#define ID_EXTBKSINGLEITEM_NG	40108
#define ID_EXTBKLASTITEM_NG		40109

#define ID_EXTBKSEPARATOR		40110


// FLAGS
#define CORNER_NONE 0
#define CORNER_ACTIVE 1
#define CORNER_TL 2
#define CORNER_TR 4
#define CORNER_BR 8
#define CORNER_BL 16

#define GRADIENT_NONE 0
#define GRADIENT_ACTIVE 1
#define GRADIENT_LR 2
#define GRADIENT_RL 4
#define GRADIENT_TB 8
#define GRADIENT_BT 16

typedef struct {
	char *szName;
	char *szDBname;
	int statusID;

	BYTE GRADIENT;
	BYTE CORNER;

	DWORD COLOR;
	DWORD COLOR2;

	BYTE COLOR2_TRANSPARENT;

	DWORD TEXTCOLOR;
	
	int ALPHA;

	int MARGIN_LEFT;
	int MARGIN_TOP;
	int MARGIN_RIGHT;
	int MARGIN_BOTTOM;

	BYTE IGNORED;
} StatusItems_t;

typedef struct {
	BOOL bGRADIENT;
	BOOL bCORNER;
	BOOL bCOLOR;
	BOOL bCOLOR2;
	BOOL bCOLOR2_TRANSPARENT;
	BOOL bTEXTCOLOR;
	BOOL bALPHA;
	BOOL bMARGIN_LEFT;
	BOOL bMARGIN_TOP;
	BOOL bMARGIN_RIGHT;
	BOOL bMARGIN_BOTTOM;
	BOOL bIGNORED;
} ChangedSItems_t;

BOOL CheckItem(int item, HWND hwndDlg);
BOOL isValidItem(void);
void SetChangedStatusItemFlag(WPARAM wParam, HWND hwndDlg);
void ChangeControlItems(HWND hwndDlg, int status, int except);
void export(char *file);
void import(char *file, HWND hwndDlg);

void SaveLatestChanges(HWND hwndDlg);
void LoadExtBkSettingsFromDB();
void SaveCompleteStructToDB();

void OnListItemsChange(HWND hwndDlg);

void UpdateStatusStructSettingsFromOptDlg(HWND hwndDlg, int index);

void SaveNonStatusItemsSettings(HWND hwndDlg);

void FillItemList(HWND hwndDlg);
void FillOptionDialogByCurrentSel(HWND hwndDlg);
void ReActiveCombo(HWND hwndDlg);
BOOL GetItemByStatus(int status, StatusItems_t* retitem);

void FillOptionDialogByStatusItem(HWND hwndDlg, StatusItems_t *item);
