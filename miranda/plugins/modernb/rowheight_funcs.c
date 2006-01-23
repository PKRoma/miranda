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

Created by Pescuma

*/
#include "commonheaders.h"
#include "rowheight_funcs.h"

extern int GetRealStatus(struct ClcContact * contact, int status);
extern int GetBasicFontID(struct ClcContact * contact);


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
		if (size > dat->row_heights_allocated)
		{
			int size_grow = size;

			size_grow += 100 - (size_grow % 100);

			if (dat->row_heights != NULL)
			{
				int *tmp = (int *) realloc((void *)dat->row_heights, sizeof(int) * size_grow);

				if (tmp == NULL)
				{
					TRACE("Out of memory: realloc returned NULL (RowHeights_Alloc)");
					RowHeights_Free(dat);
					return FALSE;
				}

				dat->row_heights = tmp;
        memset(dat->row_heights,0,sizeof(int) * size_grow);
			}
			else
			{
				dat->row_heights = (int *) malloc(sizeof(int) * size_grow);

				if (dat->row_heights == NULL)
				{
					TRACE("Out of memory: alloc returned NULL (RowHeights_Alloc)");
					RowHeights_Free(dat);
					return FALSE;
				}
        memset(dat->row_heights,0,sizeof(int) * size_grow);
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
	int max_height = 0, i, tmp;
	DWORD style=GetWindowLong(hwnd,GWL_STYLE);

	if (!dat->text_ignore_size_for_row_height)
	{
		int contact_fonts[] = {FONTID_CONTACTS, FONTID_INVIS, FONTID_OFFLINE, FONTID_NOTONLIST, FONTID_OFFINVIS, 
								FONTID_AWAY,FONTID_DND, FONTID_NA, FONTID_OCCUPIED, FONTID_CHAT, FONTID_INVISIBLE, 
								FONTID_PHONE, FONTID_LUNCH};
		int other_fonts[] = {FONTID_OPENGROUPS, FONTID_OPENGROUPCOUNTS,FONTID_CLOSEDGROUPS, FONTID_CLOSEDGROUPCOUNTS, FONTID_DIVIDERS, FONTID_CONTACT_TIME};

		// Get contact font size
		tmp = 0;
		for (i = 0 ; i < MAX_REGS(contact_fonts) ; i++)
		{
			if (tmp < dat->fontModernInfo[contact_fonts[i]].fontHeight)
				tmp = dat->fontModernInfo[contact_fonts[i]].fontHeight;
		}
		if (dat->text_replace_smileys && dat->first_line_draw_smileys && !dat->text_resize_smileys)
		{
			tmp = max(tmp, dat->text_smiley_height);
		}
		max_height += tmp; 

		if (dat->second_line_show)
		{
			tmp = dat->fontModernInfo[FONTID_SECONDLINE].fontHeight;
			if (dat->text_replace_smileys && dat->second_line_draw_smileys && !dat->text_resize_smileys)
			{
				tmp = max(tmp, dat->text_smiley_height);
			}
			max_height += dat->second_line_top_space + tmp; 
		}

		if (dat->third_line_show)
		{
			tmp = dat->fontModernInfo[FONTID_THIRDLINE].fontHeight;
			if (dat->text_replace_smileys && dat->third_line_draw_smileys && !dat->text_resize_smileys)
			{
				tmp = max(tmp, dat->text_smiley_height);
			}
			max_height += dat->third_line_top_space + tmp; 
		}

		// Get other font sizes
		for (i = 0 ; i < MAX_REGS(other_fonts) ; i++)
		{
			if (max_height < dat->fontModernInfo[other_fonts[i]].fontHeight)
				max_height = dat->fontModernInfo[other_fonts[i]].fontHeight;
		}
	}

	// Avatar size
	if (dat->avatars_show && !dat->avatars_ignore_size_for_row_height)
	{
		max_height = max(max_height, dat->avatars_size);
	}

	// Checkbox size
	if (style&CLS_CHECKBOXES || style&CLS_GROUPCHECKBOXES)
	{
		max_height = max(max_height, dat->checkboxSize);
	}

	// Icon size
	if (!dat->icon_ignore_size_for_row_height)
	{
		max_height = max(max_height, ICON_HEIGHT);
	}

	max_height += 2 * dat->row_border;

	// Min size
	max_height = max(max_height, dat->row_min_heigh);

	dat->max_row_height = max_height;

	return max_height;
}


// Calc and store row height for all itens in the list
void RowHeights_CalcRowHeights(struct ClcData *dat, HWND hwnd)
{
	int indent, subident, subindex, line_num;
	struct ClcContact *Drawing;
	struct ClcGroup *group;

	// Draw lines
	group=&dat->list;
	group->scanIndex=0;
	indent=0;
	subindex=-1;
	line_num = -1;

	RowHeights_Clear(dat);
	
	while(TRUE)
	{
		if (subindex==-1)
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
			subident = 0;
		}
		else
		{
			// Get item to draw
			Drawing = &(group->cl.items[group->scanIndex]->subcontacts[subindex]);
			subident = dat->subIndent;
		}

		line_num++;

		// Calc row height
		RowHeights_GetRowHeight(dat, hwnd, Drawing, line_num);

		//increment by subcontacts
		if (group->cl.items[group->scanIndex]->subcontacts!=NULL && group->cl.items[group->scanIndex]->type!=CLCIT_GROUP)
		{
			if (group->cl.items[group->scanIndex]->SubExpanded && dat->expandMeta)
			{
				if (subindex<group->cl.items[group->scanIndex]->SubAllocated-1)
				{
					subindex++;
				}
				else
				{
					subindex=-1;
				}
			}
		}

		if(subindex==-1)
		{
			if(group->cl.items[group->scanIndex]->type==CLCIT_GROUP && group->cl.items[group->scanIndex]->group->expanded) 
			{
				group=group->cl.items[group->scanIndex]->group;
				indent++;
				group->scanIndex=0;
				subindex=-1;
				continue;
			}
			group->scanIndex++;
		}
	}
}


// Calc and store row height
int RowHeights_GetRowHeight(struct ClcData *dat, HWND hwnd, struct ClcContact *contact, int item)
{
	int height = 0, tmp;
	DWORD style=GetWindowLong(hwnd,GWL_STYLE);

	if (!RowHeights_Alloc(dat, item + 1))
		return -1;

	if (dat->row_variable_height)
	{
		if (!dat->text_ignore_size_for_row_height)
		{
			tmp = dat->fontModernInfo[GetBasicFontID(contact)].fontHeight;
			if (dat->text_replace_smileys && dat->first_line_draw_smileys && !dat->text_resize_smileys)
			{
				tmp = max(tmp, contact->iTextMaxSmileyHeight);
			}
			height += tmp;

			if (dat->second_line_show && contact->szSecondLineText)
			{
				tmp = dat->fontModernInfo[FONTID_SECONDLINE].fontHeight;
				if (dat->text_replace_smileys && dat->second_line_draw_smileys && !dat->text_resize_smileys)
				{
					tmp = max(tmp, contact->iSecondLineMaxSmileyHeight);
				}
				height += dat->second_line_top_space + tmp;
			}

			if (dat->third_line_show && contact->szThirdLineText)
			{
				tmp = dat->fontModernInfo[FONTID_THIRDLINE].fontHeight;
				if (dat->text_replace_smileys && dat->third_line_draw_smileys && !dat->text_resize_smileys)
				{
					tmp = max(tmp, contact->iThirdLineMaxSmileyHeight);
				}
				height += dat->third_line_top_space + tmp;
			}
		}

		// Avatar size
		if (dat->avatars_show && !dat->avatars_ignore_size_for_row_height && 
				contact->type == CLCIT_CONTACT && 
				(
					(dat->use_avatar_service && contact->avatar_data != NULL) ||
      (!dat->use_avatar_service && contact->avatar_pos != AVATAR_POS_DONT_HAVE)
      )
      )
		{
			height = max(height, dat->avatars_size);
		}

		// Checkbox size
		if((style&CLS_CHECKBOXES && contact->type==CLCIT_CONTACT) ||
			(style&CLS_GROUPCHECKBOXES && contact->type==CLCIT_GROUP) ||
			(contact->type==CLCIT_INFO && contact->flags&CLCIIF_CHECKBOX))
		{
			height = max(height, dat->checkboxSize);
		}

		// Icon size
		if (!dat->icon_ignore_size_for_row_height)
		{
			if (contact->type == CLCIT_GROUP 
				|| (contact->type == CLCIT_CONTACT && contact->iImage != -1 
					&& !(dat->icon_hide_on_avatar && dat->avatars_show
						&& ( (dat->use_avatar_service && contact->avatar_data != NULL) ||
		  					 (!dat->use_avatar_service && contact->avatar_pos != AVATAR_POS_DONT_HAVE)
						   )
						&& !contact->image_is_special)))
			{
				height = max(height, ICON_HEIGHT);
			}
		}

		height += 2 * dat->row_border;

		// Min size
		height = max(height, dat->row_min_heigh);
	}
	else
	{
		height = dat->max_row_height;
	}

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
	if ( item >= dat->row_heights_size)
		return dat->max_row_height;

	return dat->row_heights[ item ];
}
