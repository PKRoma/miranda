#ifndef __ROWHEIGHT_FUNCS_H__
# define __ROWHEIGHT_FUNCS_H__

#define ROW_SPACE_BEETWEEN_LINES 0
#define ICON_HEIGHT 16

extern struct   CluiData g_CluiData;
extern struct   ExtraCache *g_ExtraCache;

BOOL RowHeights_Initialize(struct ClcData *dat);
void RowHeights_Free(struct ClcData *dat);
void RowHeights_Clear(struct ClcData *dat);

BOOL RowHeights_Alloc(struct ClcData *dat, int size);

// Calc and store max row height
int RowHeights_GetMaxRowHeight(struct ClcData *dat, HWND hwnd);

// Calc and store row height
int __forceinline RowHeights_GetRowHeight(struct ClcData *dat, HWND hwnd, struct ClcContact *contact, int item, DWORD style)
{
	int height = 0;
	//DWORD style=GetWindowLong(hwnd,GWL_STYLE);

    //if(contact->iRowHeight == item)
    //    return(dat->row_heights[item]);

    if (!RowHeights_Alloc(dat, item + 1))
		return -1;

    height = dat->fontInfo[GetBasicFontID(contact)].fontHeight;

    if(!dat->bisEmbedded) {
        if(contact->bSecondLine != MULTIROW_NEVER && contact->bSecondLine != MULTIROW_IFSPACE && contact->type == CLCIT_CONTACT) {
            if ((contact->bSecondLine == MULTIROW_ALWAYS || ((g_CluiData.dwFlags & CLUI_FRAME_SHOWSTATUSMSG && contact->bSecondLine == MULTIROW_IFNEEDED) && (contact->xStatus > 0 || g_ExtraCache[contact->extraCacheEntry].bStatusMsgValid > STATUSMSG_XSTATUSID))))
                height += (dat->fontInfo[FONTID_STATUS].fontHeight + g_CluiData.avatarPadding);
        }

        // Avatar size
        if (contact->cFlags & ECF_AVATAR && contact->type == CLCIT_CONTACT && contact->ace != NULL && !(contact->ace->dwFlags & AVS_HIDEONCLIST))
            height = max(height, g_CluiData.avatarSize + g_CluiData.avatarPadding);
    }

    // Checkbox size
    if((style&CLS_CHECKBOXES && contact->type==CLCIT_CONTACT) ||
        (style&CLS_GROUPCHECKBOXES && contact->type==CLCIT_GROUP) ||
        (contact->type==CLCIT_INFO && contact->flags&CLCIIF_CHECKBOX))
    {
        height = max(height, dat->checkboxSize);
    }

    //height += 2 * dat->row_border;
    // Min size
    height = max(height, contact->type == CLCIT_GROUP ? dat->group_row_height : dat->min_row_heigh);
    height += g_CluiData.bRowSpacing;

	dat->row_heights[item] = height;
    //contact->iRowHeight = item;

	return height;
}

// Calc and store row height for all itens in the list
void RowHeights_CalcRowHeights(struct ClcData *dat, HWND hwnd);

// Calc item top Y (using stored data)
int RowHeights_GetItemTopY(struct ClcData *dat, int item);

// Calc item bottom Y (using stored data)
int RowHeights_GetItemBottomY(struct ClcData *dat, int item);

// Calc total height of rows (using stored data)
int RowHeights_GetTotalHeight(struct ClcData *dat);

// Return the line that pos_y is at or -1 (using stored data). Y start at 0
int RowHeights_HitTest(struct ClcData *dat, int pos_y);

// Returns the height of the chosen row
int RowHeights_GetHeight(struct ClcData *dat, int item);

// returns the height for a floating contact
int RowHeights_GetFloatingRowHeight(struct ClcData *dat, HWND hwnd, struct ClcContact *contact, DWORD dwFlags);

#endif // __ROWHEIGHT_FUNCS_H__
