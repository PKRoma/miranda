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

#define ID_EXTBKEXPANDEDGROUP   40081
#define ID_EXTBKCOLLAPSEDDGROUP 40082
#define ID_EXTBKEMPTYGROUPS     40083
#define ID_EXTBKFIRSTITEM       40084
#define ID_EXTBKSINGLEITEM      40085
#define ID_EXTBKLASTITEM        40086


#define ID_EXTBKFIRSTITEM_NG    40087
#define ID_EXTBKSINGLEITEM_NG   40088
#define ID_EXTBKLASTITEM_NG     40089

#define ID_EXTBKEVEN_CNTCTPOS   40090
#define ID_EXTBKODD_CNTCTPOS    40091

#define ID_EXTBKSELECTION       40092
#define ID_EXTBKHOTTRACK        40093
#define ID_EXTBKFRAMETITLE      40094
#define ID_EXTBKEVTAREA         40095
#define ID_EXTBKSTATUSBAR       40096
#define ID_EXTBKBUTTONBAR       40097
#define ID_EXTBKBUTTONSPRESSED  40098
#define ID_EXTBKBUTTONSNPRESSED 40099
#define ID_EXTBKBUTTONSMOUSEOVER 40100
#define ID_EXTBKTBBUTTONSPRESSED  40101
#define ID_EXTBKTBBUTTONSNPRESSED 40102
#define ID_EXTBKTBBUTTONMOUSEOVER 40103
#define ID_EXTBKSTATUSFLOATER	40104
#define ID_EXTBKOWNEDFRAMEBORDER 40105
#define ID_EXTBKOWNEDFRAMEBORDERTB 40106
#define ID_EXTBK_LAST_D         40106

#define ID_EXTBKSEPARATOR       40200

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

#define IMAGE_PERPIXEL_ALPHA 1
#define IMAGE_FLAG_DIVIDED 2
#define IMAGE_FILLSOLID 4

#define IMAGE_STRETCH_V 1
#define IMAGE_STRETCH_H 2
#define IMAGE_STRETCH_B 4

#define BUTTON_ISINTERNAL 1
#define BUTTON_ISTOGGLE 2

typedef struct _tagImageItem {
    char szName[40];
    HBITMAP hbm;
    BYTE bLeft, bRight, bTop, bBottom;      // sizing margins
    BYTE alpha;
    DWORD dwFlags;
    HDC hdc;
    HBITMAP hbmOld;
    LONG inner_height, inner_width;
    LONG width, height;
    BLENDFUNCTION bf;
    BYTE bStretch;
    HBRUSH fillBrush;
    struct _tagImageItem *nextItem;
} ImageItem;

typedef struct _tagButtonItem {
    char szName[40];
    HWND hWnd;
    LONG xOff, yOff;
    LONG width, height;
    ImageItem *imgNormal, *imgPressed, *imgHover;
    LONG normalGlyphMetrics[4];
    LONG hoverGlyphMetrics[4];
    LONG pressedGlyphMetrics[4];
    DWORD dwFlags;
    DWORD uId;
    TCHAR szTip[256];
    struct _tagButtonItem *nextItem;
} ButtonItem;

typedef struct {
    char szName[40];
    char szDBname[40];
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
    DWORD BORDERSTYLE;
    ImageItem *imageItem;
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
    BOOL bBORDERSTYLE;
} ChangedSItems_t;

BOOL CheckItem(int item, HWND hwndDlg);
BOOL isValidItem(void);
void SetChangedStatusItemFlag(WPARAM wParam, HWND hwndDlg);
void ChangeControlItems(HWND hwndDlg, int status, int except);
void extbk_export(char *file);
void extbk_import(char *file, HWND hwndDlg);

void SaveLatestChanges(HWND hwndDlg);
void LoadExtBkSettingsFromDB();
void IMG_LoadItems();
void IMG_CreateItem(ImageItem *item, const char *fileName, HDC hdc);
void IMG_DeleteItem(ImageItem *item);
void __fastcall IMG_RenderImageItem(HDC hdc, ImageItem *item, RECT *rc);
void IMG_InitDecoder();
void LoadPerContactSkins(char *file);

void SaveCompleteStructToDB();
StatusItems_t *GetProtocolStatusItem(const char *szProto);

void OnListItemsChange(HWND hwndDlg);

void UpdateStatusStructSettingsFromOptDlg(HWND hwndDlg, int index);

void SaveNonStatusItemsSettings(HWND hwndDlg);

void FillItemList(HWND hwndDlg);
void FillOptionDialogByCurrentSel(HWND hwndDlg);
void ReActiveCombo(HWND hwndDlg);
//BOOL __fastcall GetItemByStatus(int status, StatusItems_t *retitem);

void FillOptionDialogByStatusItem(HWND hwndDlg, StatusItems_t *item);

#define MS_SKIN_DRAWGLYPH "ModernList/DrawGlyph"

/* EVENTS */
#define ME_SKIN_SERVICESCREATED "ModernList/ServicesCreated"

/* DRAWGLYPH Request structure */
typedef struct s_SKINDRAWREQUEST
{
  char szObjectID[255];      // Unic Object ID (path) to paint
  RECT rcDestRect;           // Rectangle to fit
  RECT rcClipRect;           // Rectangle to paint in.
  HDC hDC;                   // Handler to device context to paint in. 
} SKINDRAWREQUEST,*LPSKINDRAWREQUEST;

