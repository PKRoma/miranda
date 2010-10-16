/*
 * astyle --force-indent=tab=4 --brackets=linux --indent-switches
 *		  --pad=oper --one-line=keep-blocks  --unpad=paren
 *
 * Miranda IM: the free IM client for Microsoft* Windows*
 *
 * Copyright 2000-2010 Miranda ICQ/IM project,
 * all portions of this codebase are copyrighted to the people
 * listed in contributors.txt.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * you should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * part of clist_nicer plugin for Miranda.
 *
 * (C) 2005-2010 by silvercircle _at_ gmail _dot_ com and contributors
 *
 * $Id: rowheight_funcs.cpp 12484 2010-08-27 04:02:43Z silvercircle $
 *
 */

#include <commonheaders.h>
#include <rowheight_funcs.h>

BOOL RowHeight::Init(ClcData *dat)
{
	dat->max_row_height = 0;
	dat->row_heights_size = 0;
	dat->row_heights_allocated = 0;
	dat->row_heights = NULL;

	return TRUE;
}

void RowHeight::Free(ClcData *dat)
{
	if (dat->row_heights != NULL)
	{
		free(dat->row_heights);
		dat->row_heights = NULL;
	}

	dat->row_heights_allocated = 0;
	dat->row_heights_size = 0;
}

void RowHeight::Clear(ClcData *dat)
{
	dat->row_heights_size = 0;
}


BOOL RowHeight::Alloc(ClcData *dat, int size)
{
	if (size > dat->row_heights_size)
	{
		if (size > dat->row_heights_allocated) {
			int size_grow = size;

			size_grow += 100 - (size_grow % 100);

			if (dat->row_heights != NULL) {
				int *tmp = (int *) realloc((void *)dat->row_heights, sizeof(int) * size_grow);

				if (tmp == NULL) {
					Free(dat);
					return FALSE;
				}

				dat->row_heights = tmp;
			}
			else {
				dat->row_heights = (int *) malloc(sizeof(int) * size_grow);

				if (dat->row_heights == NULL) {
					Free(dat);
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
int RowHeight::getMaxRowHeight(ClcData *dat, const HWND hwnd)
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

    if (cfg::dat.dualRowMode == 1 && !dat->bisEmbedded)
        max_height += ROW_SPACE_BEETWEEN_LINES + dat->fontInfo[FONTID_STATUS].fontHeight;

    // Get other font sizes
    for (i = 0 ; i < MAX_REGS(other_fonts) ; i++) {
        if (max_height < dat->fontInfo[other_fonts[i]].fontHeight)
            max_height = dat->fontInfo[other_fonts[i]].fontHeight;
    }

	// Avatar size
	if (cfg::dat.dwFlags & CLUI_FRAME_AVATARS && !dat->bisEmbedded)
		max_height = max(max_height, cfg::dat.avatarSize + cfg::dat.avatarPadding);

	// Checkbox size
	if (style&CLS_CHECKBOXES || style&CLS_GROUPCHECKBOXES)
		max_height = max(max_height, dat->checkboxSize);

	//max_height += 2 * dat->row_border;
	// Min size
	max_height = max(max_height, dat->min_row_heigh);
    max_height += cfg::dat.bRowSpacing;

	dat->max_row_height = max_height;

	return max_height;
}

// Calc and store row height for all itens in the list
void RowHeight::calcRowHeights(ClcData *dat, HWND hwnd)
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

	Clear(dat);
	
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
		getRowHeight(dat, hwnd, Drawing, line_num, dwStyle);

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


// Calc item top Y (using stored data)
int RowHeight::getItemTopY(ClcData *dat, int item)
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
int RowHeight::getItemBottomY(ClcData *dat, int item)
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
int RowHeight::getTotalHeight(ClcData *dat)
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
int RowHeight::hitTest(ClcData *dat, int pos_y)
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

int RowHeight::getHeight(ClcData *dat, int item)
{
	if ( dat->row_heights == 0 )
		return 0;

	return dat->row_heights[ item ];
}

int RowHeight::getFloatingRowHeight(const ClcData *dat, HWND hwnd, ClcContact *contact, DWORD dwFlags)
{
	int height = 0;

    height = dat->fontInfo[GetBasicFontID(contact)].fontHeight;

    if(!dat->bisEmbedded) {
		if(!(dwFlags & FLT_SIMPLE)){
			if(dwFlags & FLT_DUALROW) {
				height += (dat->fontInfo[FONTID_STATUS].fontHeight + cfg::dat.avatarPadding);
			}
			// Avatar size
			if (dwFlags & FLT_AVATARS && contact->cFlags & ECF_AVATAR && contact->type == CLCIT_CONTACT && contact->ace != NULL && !(contact->ace->dwFlags & AVS_HIDEONCLIST))
				height = max(height, cfg::dat.avatarSize + cfg::dat.avatarPadding);
		}
	}

    height = max(height, dat->min_row_heigh);
    height += cfg::dat.bRowSpacing;

	return height;
}
