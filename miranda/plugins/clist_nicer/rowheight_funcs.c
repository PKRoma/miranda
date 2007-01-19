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
#include "commonheaders.h"
#include "rowheight_funcs.h"

extern struct   CluiData g_CluiData;
extern struct   ExtraCache *g_ExtraCache;

BOOL RowHeights_Initialize(struct ClcData *dat)
{
	dat->max_row_height = 0;
	dat->row_heights_size = 0;
	dat->row_heights_allocated = 0;
	dat->row_heights = NULL;

	return TRUE;
}

void RowHeights_Free(struct ClcData *dat)
{
	if (dat->row_heights != NULL)
	{
		free(dat->row_heights);
		dat->row_heights = NULL;
	}

	dat->row_heights_allocated = 0;
	dat->row_heights_size = 0;
}

void RowHeights_Clear(struct ClcData *dat)
{
	dat->row_heights_size = 0;
}


BOOL RowHeights_Alloc(struct ClcData *dat, int size)
{
	if (size > dat->row_heights_size)
	{
		if (size > dat->row_heights_allocated) {
			int size_grow = size;

			size_grow += 100 - (size_grow % 100);

			if (dat->row_heights != NULL) {
				int *tmp = (int *) realloc((void *)dat->row_heights, sizeof(int) * size_grow);

				if (tmp == NULL) {
					RowHeights_Free(dat);
					return FALSE;
				}

				dat->row_heights = tmp;
			}
			else {
				dat->row_heights = (int *) malloc(sizeof(int) * size_grow);

				if (dat->row_heights == NULL) {
					RowHeights_Free(dat);
					return FALSE;
				}
			}
			dat->row_heights_allocated = size_grow;
		}
		dat->row_heights_size = size;
	}
	return TRUE;
}

// Calc and store max row height
int RowHeights_GetMaxRowHeight(struct ClcData *dat, HWND hwnd)
{
	int max_height = 0, i;
	DWORD style=GetWindowLong(hwnd,GWL_STYLE);

    int contact_fonts[] = {FONTID_CONTACTS, FONTID_INVIS, FONTID_OFFLINE, FONTID_NOTONLIST, FONTID_OFFINVIS};
    int other_fonts[] = {FONTID_GROUPS, FONTID_GROUPCOUNTS, FONTID_DIVIDERS};

    // Get contact font size
    for (i = 0 ; i < MAX_REGS(contact_fonts) ; i++)
    {
        if (max_height < dat->fontInfo[contact_fonts[i]].fontHeight)
            max_height = dat->fontInfo[contact_fonts[i]].fontHeight;
    }

    if (g_CluiData.dualRowMode == 1 && !dat->bisEmbedded)
        max_height += ROW_SPACE_BEETWEEN_LINES + dat->fontInfo[FONTID_STATUS].fontHeight;

    // Get other font sizes
    for (i = 0 ; i < MAX_REGS(other_fonts) ; i++) {
        if (max_height < dat->fontInfo[other_fonts[i]].fontHeight)
            max_height = dat->fontInfo[other_fonts[i]].fontHeight;
    }

	// Avatar size
	if (g_CluiData.dwFlags & CLUI_FRAME_AVATARS && !dat->bisEmbedded)
		max_height = max(max_height, g_CluiData.avatarSize + g_CluiData.avatarPadding);

	// Checkbox size
	if (style&CLS_CHECKBOXES || style&CLS_GROUPCHECKBOXES)
		max_height = max(max_height, dat->checkboxSize);

	//max_height += 2 * dat->row_border;
	// Min size
	max_height = max(max_height, dat->min_row_heigh);
    max_height += g_CluiData.bRowSpacing;

	dat->max_row_height = max_height;

	return max_height;
}

// Calc and store row height for all itens in the list
void RowHeights_CalcRowHeights(struct ClcData *dat, HWND hwnd)
{
	int indent, subindex, line_num;
	struct ClcContact *Drawing;
	struct ClcGroup *group;
    DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);

	// Draw lines
	group=&dat->list;
	group->scanIndex=0;
	indent=0;
	//subindex=-1;
	line_num = -1;

	RowHeights_Clear(dat);
	
	while(TRUE)
	{
        if (group->scanIndex==group->cl.count) 
		{
			group=group->parent;
			indent--;
			if(group==NULL) break;	// Finished list
			group->scanIndex++;
			continue;
		}

		// Get item to draw
		Drawing = group->cl.items[group->scanIndex];
		line_num++;

		// Calc row height
		RowHeights_GetRowHeight(dat, hwnd, Drawing, line_num, dwStyle);

		if(group->cl.items[group->scanIndex]->type==CLCIT_GROUP && /*!IsBadCodePtr((FARPROC)group->cl.items[group->scanIndex]->group) && */ (group->cl.items[group->scanIndex]->group->expanded & 0x0000ffff)) {
			group=group->cl.items[group->scanIndex]->group;
			indent++;
			group->scanIndex=0;
			subindex=-1;
			continue;
		}
		group->scanIndex++;
	}
}


// Calc and store row height
int __forceinline RowHeights_GetRowHeight(struct ClcData *dat, HWND hwnd, struct ClcContact *contact, int item, DWORD style)
{
	int height = 0;
	//DWORD style=GetWindowLong(hwnd,GWL_STYLE);

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

	return height;
}


// Calc item top Y (using stored data)
int RowHeights_GetItemTopY(struct ClcData *dat, int item)
{
	int i;
	int y = 0;

	if (item >= dat->row_heights_size)
		return -1;

	for (i = 0 ; i < item ; i++)
	{
		y += dat->row_heights[i];
	}

	return y;
}


// Calc item bottom Y (using stored data)
int RowHeights_GetItemBottomY(struct ClcData *dat, int item)
{
	int i;
	int y = 0;

	if (item >= dat->row_heights_size)
		return -1;

	for (i = 0 ; i <= item ; i++)
	{
		y += dat->row_heights[i];
	}

	return y;
}


// Calc total height of rows (using stored data)
int RowHeights_GetTotalHeight(struct ClcData *dat)
{
	int i;
	int y = 0;

	for (i = 0 ; i < dat->row_heights_size ; i++)
	{
		y += dat->row_heights[i];
	}

	return y;
}

// Return the line that pos_y is at or -1 (using stored data)
int RowHeights_HitTest(struct ClcData *dat, int pos_y)
{
	int i;
	int y = 0;

	if (pos_y < 0)
		return -1;

	for (i = 0 ; i < dat->row_heights_size ; i++)
	{
		y += dat->row_heights[i];

		if (pos_y < y)
			return i;
	}

	return -1;
}

int RowHeights_GetHeight(struct ClcData *dat, int item)
{
	if ( dat->row_heights == 0 )
		return 0;

	return dat->row_heights[ item ];
}

int RowHeights_GetFloatingRowHeight(struct ClcData *dat, HWND hwnd, struct ClcContact *contact, DWORD dwFlags)
{
	int height = 0;

    height = dat->fontInfo[GetBasicFontID(contact)].fontHeight;

    if(!dat->bisEmbedded) {
		if(dwFlags & FLT_DUALROW) {
			if(contact->bSecondLine != MULTIROW_NEVER && contact->bSecondLine != MULTIROW_IFSPACE) {
				if ((contact->bSecondLine == MULTIROW_ALWAYS || ((g_CluiData.dwFlags & CLUI_FRAME_SHOWSTATUSMSG && contact->bSecondLine == MULTIROW_IFNEEDED) && (contact->xStatus > 0 || g_ExtraCache[contact->extraCacheEntry].bStatusMsgValid > STATUSMSG_XSTATUSID))))
					height += (dat->fontInfo[FONTID_STATUS].fontHeight + g_CluiData.avatarPadding);
			}
		}
        // Avatar size
        if (dwFlags & FLT_AVATARS && contact->cFlags & ECF_AVATAR && contact->type == CLCIT_CONTACT && contact->ace != NULL && !(contact->ace->dwFlags & AVS_HIDEONCLIST))
            height = max(height, g_CluiData.avatarSize + g_CluiData.avatarPadding);
    }

    height = max(height, dat->min_row_heigh);
    height += g_CluiData.bRowSpacing;

	return height;
}
