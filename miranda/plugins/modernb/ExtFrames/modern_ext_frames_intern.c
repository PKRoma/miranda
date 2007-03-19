/**************************************************************************\

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2007 Miranda ICQ/IM project,
all portions of this code base are copyrighted to Artem Shpynov and/or
the people listed in contributors.txt.

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

****************************************************************************

Created: Mar 19, 2007

Author and Copyright:  Artem Shpynov aka FYR:  ashpynov@gmail.com

****************************************************************************

File contains realization of internal only available procedures 
for modern_ext_frames.c module.

This file have to be excluded from compilation and need to be adde to project via
#include preprocessor derective in modern_ext_frames.c

\**************************************************************************/

#include "..\commonheaders.h"  //only for precompiled headers

#ifdef __modern_ext_frames_c__include_c_file   //protection from adding to compilation
//  Closed module methods
static int _ExtFrames_GetMinParentSize(IN SortedList* pList, OUT SIZE * size )
{
	EXTFRAME * clcFrame=(EXTFRAME *) pList->items[pList->realCount-1];
	int minCX=clcFrame->minCX;
	int minCY=clcFrame->minCY;
	int i=pList->realCount-1;
	for (; i>0; --i)
	{
		EXTFRAME * extFrame=(EXTFRAME *)pList->items[i];
		if (extFrame && extFrame->bVisible && extFrame->bDocked)
		{
			if (extFrame->nType==EFT_VERTICAL)
			{
				minCX+=extFrame->minCX;
				minCY=max( minCY, extFrame->minCY );
			}
			else
			{
				minCY+=extFrame->minCY;
				minCX=max( minCX,extFrame->minCX );
			}
		}
	}
	if (size)
	{
		size->cx=minCX;
		size->cy=minCY;
	}
	return minCY;
}

static int _ExtFrames_CalcFramesRect(IN SortedList* pList, IN int width, IN int height, OUT RECT * pOutRect )
{
	int outHeight=height;
	int i;
	SIZE size;
	RECT outRect={0};
	int frmCount=pList->realCount-1;
	EXTFRAME * clcFrame=(EXTFRAME *)pList->items[frmCount];
	if (ExtFrames_GetMinWindowSize(&size))   //ensure that we provide normal size
	{
		width=max(size.cx,width);
		height=max(size.cy,height);
	}
	outRect.right=width;
	outRect.bottom=height;

	for (i=0; i<frmCount; i++)
	{
		EXTFRAME * extFrame=(EXTFRAME *)pList->items[i];
		if (extFrame && extFrame->bVisible && extFrame->bDocked)
		{	
			extFrame->rcFrameRect=outRect;
			switch(extFrame->nEdge)
			{
			case EFP_LEFT:
				extFrame->rcFrameRect.right=extFrame->rcFrameRect.left+extFrame->minCX;
				outRect.left+=extFrame->minCX;
				break;
			case EFP_RIGHT:
				extFrame->rcFrameRect.left=extFrame->rcFrameRect.right-extFrame->minCX;
				outRect.right-=extFrame->minCX;
				break;
			case EFP_TOP:
				extFrame->rcFrameRect.bottom=extFrame->rcFrameRect.top+extFrame->minCY;
				outRect.top+=extFrame->minCY;
				break;
			case EFP_BOTTOM:
				extFrame->rcFrameRect.top=extFrame->rcFrameRect.bottom-extFrame->minCY;
				outRect.bottom-=extFrame->minCY;
				break;
			}
		}
	}
	clcFrame->rcFrameRect=outRect;
	if (pOutRect)
	{
		pOutRect->top=0;
		pOutRect->left=0;
		pOutRect->right=width;
		pOutRect->bottom=height;
	}
	outHeight=height;
	return height;
}

static void _ExtFrames_Clear_EXTFRAMEWND(void * extFrame)
{
	mir_free(extFrame);
	return;
}


#endif