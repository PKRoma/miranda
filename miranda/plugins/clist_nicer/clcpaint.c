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

extern int hClcProtoCount;
extern ClcProtoStatus *clcProto;
extern HIMAGELIST himlCListClc;
static BYTE divide3[765]={255};

static void ChangeToFont(HDC hdc,struct ClcData *dat,int id,int *fontHeight)
{
	SelectObject(hdc,dat->fontInfo[id].hFont);
	SetTextColor(hdc,dat->fontInfo[id].colour);
	if(fontHeight) *fontHeight=dat->fontInfo[id].fontHeight;
}

static void __inline SetHotTrackColour(HDC hdc,struct ClcData *dat)
{
	if(dat->gammaCorrection) {
		COLORREF oldCol,newCol;
		int oldLum,newLum;

		oldCol=GetTextColor(hdc);
		oldLum=(GetRValue(oldCol)*30+GetGValue(oldCol)*59+GetBValue(oldCol)*11)/100;
		newLum=(GetRValue(dat->hotTextColour)*30+GetGValue(dat->hotTextColour)*59+GetBValue(dat->hotTextColour)*11)/100;
		if(newLum==0) {
			SetTextColor(hdc,dat->hotTextColour);
			return;
		}
		if(newLum>=oldLum+20) {
			oldLum+=20;
			newCol=RGB(GetRValue(dat->hotTextColour)*oldLum/newLum,GetGValue(dat->hotTextColour)*oldLum/newLum,GetBValue(dat->hotTextColour)*oldLum/newLum);
		}
		else if(newLum<=oldLum) {
			int r,g,b;
			r=GetRValue(dat->hotTextColour)*oldLum/newLum;
			g=GetGValue(dat->hotTextColour)*oldLum/newLum;
			b=GetBValue(dat->hotTextColour)*oldLum/newLum;
			if(r>255) {
				g+=(r-255)*3/7;
				b+=(r-255)*3/7;
				r=255;
			}
			if(g>255) {
				r+=(g-255)*59/41;
				if(r>255) r=255;
				b+=(g-255)*59/41;
				g=255;
			}
			if(b>255) {
				r+=(b-255)*11/89;
				if(r>255) r=255;
				g+=(b-255)*11/89;
				if(g>255) g=255;
				b=255;
			}
			newCol=RGB(r,g,b);
		}
		else newCol=dat->hotTextColour;
		SetTextColor(hdc,newCol);
	}
	else
		SetTextColor(hdc,dat->hotTextColour);
}

static int GetStatusOnlineness(int status)
{
	switch(status) {
		case ID_STATUS_FREECHAT:   return 110;
		case ID_STATUS_ONLINE:     return 100;
		case ID_STATUS_OCCUPIED:   return 60;
		case ID_STATUS_ONTHEPHONE: return 50;
		case ID_STATUS_DND:        return 40;
		case ID_STATUS_AWAY:	   return 30;
		case ID_STATUS_OUTTOLUNCH: return 20;
		case ID_STATUS_NA:		   return 10;
		case ID_STATUS_INVISIBLE:  return 5;
	}
	return 0;
}

static int GetGeneralisedStatus(void)
{
	int i,status,thisStatus,statusOnlineness,thisOnlineness;

	status=ID_STATUS_OFFLINE;
	statusOnlineness=0;

	for (i=0;i<hClcProtoCount;i++) {
		thisStatus = clcProto[i].dwStatus;
		if(thisStatus==ID_STATUS_INVISIBLE) return ID_STATUS_INVISIBLE;
		thisOnlineness=GetStatusOnlineness(thisStatus);
		if(thisOnlineness>statusOnlineness) {
			status=thisStatus;
			statusOnlineness=thisOnlineness;
		}
	}
	return status;
}

static int GetRealStatus(struct ClcContact * contact, int status) 
{
	int i;
	char *szProto=contact->proto;
	if (!szProto) return status;
	for (i=0;i<hClcProtoCount;i++) {
		if (!lstrcmp(clcProto[i].szProto,szProto)) {
			return clcProto[i].dwStatus;
		}
	}
	return status;
}

static HMODULE  themeAPIHandle = NULL; // handle to uxtheme.dll
static HANDLE   (WINAPI *MyOpenThemeData)(HWND,LPCWSTR);
static HRESULT  (WINAPI *MyCloseThemeData)(HANDLE);
static HRESULT  (WINAPI *MyDrawThemeBackground)(HANDLE,HDC,int,int,const RECT *,const RECT *);

#define MGPROC(x) GetProcAddress(themeAPIHandle,x)

void PaintClc(HWND hwnd,struct ClcData *dat,HDC hdc,RECT *rcPaint)
{
	HDC hdcMem;
	RECT clRect, rc;
	int y,indent,index,fontHeight;
	struct ClcGroup *group;
	HBITMAP hBmpOsb;
	DWORD style=GetWindowLong(hwnd,GWL_STYLE);
	int status=GetGeneralisedStatus();
	int grey=0,groupCountsFontTopShift;
	HBRUSH hBrushAlternateGrey=NULL;
	DWORD savedCORNER=-1; // for selection shape
	BOOL bFirstNGdrawn = FALSE;

	// yes I know about GetSysColorBrush()
	COLORREF tmpbkcolour = style&CLS_CONTACTLIST ? ( dat->useWindowsColours ? GetSysColor(COLOR_3DFACE) : dat->bkColour ) : dat->bkColour;
		 

	if(dat->greyoutFlags&ClcStatusToPf2(status) || style&WS_DISABLED) grey=1;
	else if(GetFocus()!=hwnd && dat->greyoutFlags&GREYF_UNFOCUS) grey=1;
	GetClientRect(hwnd,&clRect);
	if(rcPaint==NULL) rcPaint=&clRect;
	if(IsRectEmpty(rcPaint)) return;
	y=-dat->yScroll;
	hdcMem=CreateCompatibleDC(hdc);
	hBmpOsb=CreateBitmap(clRect.right,clRect.bottom,1,GetDeviceCaps(hdc,BITSPIXEL),NULL);
	SelectObject(hdcMem,hBmpOsb);
	{	TEXTMETRIC tm;
		SelectObject(hdcMem,dat->fontInfo[FONTID_GROUPS].hFont);
		GetTextMetrics(hdcMem,&tm);
		groupCountsFontTopShift=tm.tmAscent;
		SelectObject(hdcMem,dat->fontInfo[FONTID_GROUPCOUNTS].hFont);
		GetTextMetrics(hdcMem,&tm);
		groupCountsFontTopShift-=tm.tmAscent;
	}
	if(style&CLS_GREYALTERNATE) 
		hBrushAlternateGrey=CreateSolidBrush(GetNearestColor(hdcMem,RGB(GetRValue(tmpbkcolour)-10,GetGValue(tmpbkcolour)-10,GetBValue(tmpbkcolour)-10)));
	
	ChangeToFont(hdcMem,dat,FONTID_CONTACTS,&fontHeight);
	SetBkMode(hdcMem,TRANSPARENT);
	{	HBRUSH hBrush,hoBrush;
		
		hBrush=CreateSolidBrush(tmpbkcolour);
		hoBrush=(HBRUSH)SelectObject(hdcMem,hBrush);
		FillRect(hdcMem,rcPaint,hBrush);
		SelectObject(hdcMem,hoBrush);
		DeleteObject(hBrush);
		if(	dat->hBmpBackground	) {
			BITMAP bmp;
			HDC hdcBmp;
			int x,y;
			int bitx,bity;
			int maxx,maxy;
			int destw,desth;			
			// XXX: Halftone isnt supported on 9x, however the scretch problems dont happen on 98.
			SetStretchBltMode(hdcMem, HALFTONE);

			GetObject(dat->hBmpBackground,sizeof(bmp),&bmp);
			hdcBmp=CreateCompatibleDC(hdcMem);
			SelectObject(hdcBmp,dat->hBmpBackground);
			y=dat->backgroundBmpUse&CLBF_SCROLL?-dat->yScroll:0;
			maxx=dat->backgroundBmpUse&CLBF_TILEH?clRect.right:1;
			maxy=dat->backgroundBmpUse&CLBF_TILEV?maxy=rcPaint->bottom:y+1;
			if (!DBGetContactSettingByte(NULL,"CLCExt","EXBK_FillWallpaper",0))
			{
				switch(dat->backgroundBmpUse&CLBM_TYPE) {
				case CLB_STRETCH:
					if(dat->backgroundBmpUse&CLBF_PROPORTIONAL) {
						if(clRect.right*bmp.bmHeight<clRect.bottom*bmp.bmWidth) {
							desth=clRect.bottom;
							destw=desth*bmp.bmWidth/bmp.bmHeight;
						}
						else {
							destw=clRect.right;
							desth=destw*bmp.bmHeight/bmp.bmWidth;
						}
					}
					else {
						destw=clRect.right;
						desth=clRect.bottom;
					}
					break;
				case CLB_STRETCHH:
					if(dat->backgroundBmpUse&CLBF_PROPORTIONAL) {
						destw=clRect.right;
						desth=destw*bmp.bmHeight/bmp.bmWidth;
					}
					else {
						destw=clRect.right;
						desth=bmp.bmHeight;
					}
					break;
				case CLB_STRETCHV:
					if(dat->backgroundBmpUse&CLBF_PROPORTIONAL) {
						desth=clRect.bottom;
						destw=desth*bmp.bmWidth/bmp.bmHeight;
					}
					else {
						destw=bmp.bmWidth;
						desth=clRect.bottom;
					}
					break;
				default:    //clb_topleft
					destw=bmp.bmWidth;
					desth=bmp.bmHeight;
					break;
				}
			}
			else{
				destw=bmp.bmWidth;
				desth=bmp.bmHeight;
			}

			if (DBGetContactSettingByte(NULL,"CLCExt","EXBK_FillWallpaper",0))
			{
				RECT mrect;
				GetWindowRect(hwnd,&mrect);
				bitx = mrect.left;
				bity = mrect.top;
			} else
			{
				bitx = 0;
				bity = 0;
			}
			
			for(;y<maxy;y+=desth) {
				if(y<rcPaint->top-desth) continue;
				for(x=0;x<maxx;x+=destw)
					StretchBlt(hdcMem,x,y,destw,desth,hdcBmp,bitx,bity,bmp.bmWidth,bmp.bmHeight,SRCCOPY);
			}			
			DeleteDC(hdcBmp);
		}		
	} 

	group=&dat->list;
	group->scanIndex=0;
	indent=0;		

	for(index=0;y<rcPaint->bottom;) {		
		if(group->scanIndex==group->contactCount) {
			group=group->parent;
			indent--;
			if(group==NULL) {
				break;
			}
			group->scanIndex++;
			continue;
		}
		if(y>rcPaint->top-dat->rowHeight) {
			int iImage=-1;
			int selected=index==dat->selection && (dat->showSelAlways || dat->exStyle&CLS_EX_SHOWSELALWAYS || GetFocus()==hwnd) && group->contact[group->scanIndex].type!=CLCIT_DIVIDER;
			int hottrack=dat->exStyle&CLS_EX_TRACKSELECT && group->contact[group->scanIndex].type!=CLCIT_DIVIDER && dat->iHotTrack==index;
			SIZE textSize,countsSize,spaceSize;
			int width,checkboxWidth;
			char *szCounts;
			
			//alternating grey
			if(style&CLS_GREYALTERNATE && index&1) {
				RECT rc;
				rc.top=y; rc.bottom=rc.top+dat->rowHeight;
				rc.left=0; rc.right=clRect.right;
				FillRect(hdcMem,&rc,hBrushAlternateGrey);
			}

			//setup
			if(group->contact[group->scanIndex].type==CLCIT_GROUP)
				ChangeToFont(hdcMem,dat,FONTID_GROUPS,&fontHeight);
			else if(group->contact[group->scanIndex].type==CLCIT_INFO) {
				if(group->contact[group->scanIndex].flags&CLCIIF_GROUPFONT) ChangeToFont(hdcMem,dat,FONTID_GROUPS,&fontHeight);
				else ChangeToFont(hdcMem,dat,FONTID_CONTACTS,&fontHeight);
			}
			else if(group->contact[group->scanIndex].type==CLCIT_DIVIDER)
				ChangeToFont(hdcMem,dat,FONTID_DIVIDERS,&fontHeight);
			else if(group->contact[group->scanIndex].type==CLCIT_CONTACT && group->contact[group->scanIndex].flags&CONTACTF_NOTONLIST)
				ChangeToFont(hdcMem,dat,FONTID_NOTONLIST,&fontHeight);
			else if ( group->contact[group->scanIndex].type==CLCIT_CONTACT && 
				(	
					(group->contact[group->scanIndex].flags&CONTACTF_INVISTO && GetRealStatus(&group->contact[group->scanIndex], status) != ID_STATUS_INVISIBLE )
					||
					(group->contact[group->scanIndex].flags&CONTACTF_VISTO && GetRealStatus(&group->contact[group->scanIndex], status)==ID_STATUS_INVISIBLE)
				) 
			) 
			{
				// the contact is in the always visible list and the proto is invisible
				// the contact is in the always invisible and the proto is in any other mode
				ChangeToFont(hdcMem,dat, group->contact[group->scanIndex].flags&CONTACTF_ONLINE ? FONTID_INVIS:FONTID_OFFINVIS ,&fontHeight);
			}
			else if(group->contact[group->scanIndex].type==CLCIT_CONTACT && !(group->contact[group->scanIndex].flags&CONTACTF_ONLINE))
				ChangeToFont(hdcMem,dat,FONTID_OFFLINE,&fontHeight);
			else
				ChangeToFont(hdcMem,dat,FONTID_CONTACTS,&fontHeight);
			GetTextExtentPoint32(hdcMem,group->contact[group->scanIndex].szText,lstrlen(group->contact[group->scanIndex].szText),&textSize);
			width=textSize.cx;
			if(group->contact[group->scanIndex].type==CLCIT_GROUP) {
				szCounts=GetGroupCountsText(dat,&group->contact[group->scanIndex]);
				if(szCounts[0]) {
					GetTextExtentPoint32(hdcMem," ",1,&spaceSize);
					ChangeToFont(hdcMem,dat,FONTID_GROUPCOUNTS,&fontHeight);
					GetTextExtentPoint32(hdcMem,szCounts,lstrlen(szCounts),&countsSize);
					width+=spaceSize.cx+countsSize.cx;
				}
			}

			if((style&CLS_CHECKBOXES && group->contact[group->scanIndex].type==CLCIT_CONTACT) ||
			   (style&CLS_GROUPCHECKBOXES && group->contact[group->scanIndex].type==CLCIT_GROUP) ||
			   (group->contact[group->scanIndex].type==CLCIT_INFO && group->contact[group->scanIndex].flags&CLCIIF_CHECKBOX))
				checkboxWidth=dat->checkboxSize+2;
			else checkboxWidth=0;

			/***** BACKGROUND DRAWING *****/
			// contacts
			if ( group->contact[group->scanIndex].type==CLCIT_CONTACT )
			{
				StatusItems_t sitem;	
				DWORD cstatus = DBGetContactSettingWord(group->contact[group->scanIndex].hContact, group->contact[group->scanIndex].proto, "Status", ID_STATUS_OFFLINE);
				
				if (GetItemByStatus(cstatus,&sitem))
				{					
					StatusItems_t sfirstitem, ssingleitem , slastitem, slastitem_NG;
					StatusItems_t sfirstitem_NG, ssingleitem_NG;

					if (!sitem.IGNORED)
						SetTextColor(hdcMem,sitem.TEXTCOLOR);
					//else // set std text color
						//SetTextColor();

					// test
					// SetTextColor(hdcMem,~sitem.COLOR&0x00FFFFFF);

					rc.left = sitem.MARGIN_LEFT;
					rc.top = y  + sitem.MARGIN_TOP;
					rc.right = clRect.right - sitem.MARGIN_RIGHT;
					rc.bottom = y+dat->rowHeight - sitem.MARGIN_BOTTOM;

					GetItemByStatus(ID_EXTBKFIRSTITEM, &sfirstitem);
					GetItemByStatus(ID_EXTBKSINGLEITEM, &ssingleitem);
					GetItemByStatus(ID_EXTBKLASTITEM, &slastitem);

					// non-grouped					
					GetItemByStatus(ID_EXTBKFIRSTITEM_NG, &sfirstitem_NG);
					GetItemByStatus(ID_EXTBKSINGLEITEM_NG, &ssingleitem_NG);
					GetItemByStatus(ID_EXTBKLASTITEM_NG, &slastitem_NG);
					
					// check for special cases (first item, single item, last item)
					// this will only change the shape for this status. Color will be blended over with ALPHA value										
					if (group->scanIndex==0 && group->contactCount==1 && !ssingleitem.IGNORED)
					{
						if (group->parent!=NULL)
						{						
							rc.left = ssingleitem.MARGIN_LEFT;
							rc.top = y  + ssingleitem.MARGIN_TOP;
							rc.right = clRect.right - ssingleitem.MARGIN_RIGHT;
							rc.bottom = y+dat->rowHeight - ssingleitem.MARGIN_BOTTOM;				
							if (!sitem.IGNORED)
							{	
								if (!selected || DBGetContactSettingByte(NULL,"CLCExt","SelBlend",1))
									DrawAlpha(hwnd,hdcMem,&rc,sitem.COLOR,sitem.ALPHA, sitem.COLOR2, sitem.COLOR2_TRANSPARENT, sitem.GRADIENT,ssingleitem.CORNER);
								savedCORNER = ssingleitem.CORNER;
							}
							if (!selected || DBGetContactSettingByte(NULL,"CLCExt","SelBlend",1))
								DrawAlpha(hwnd,hdcMem,&rc,ssingleitem.COLOR,ssingleitem.ALPHA, ssingleitem.COLOR2, ssingleitem.COLOR2_TRANSPARENT,ssingleitem.GRADIENT,ssingleitem.CORNER);
						} 
					}
					else if (group->scanIndex==0 && group->contactCount>1 && !sfirstitem.IGNORED)
					{
						if (group->parent!=NULL)
						{
							rc.left = sfirstitem.MARGIN_LEFT;
							rc.top = y  + sfirstitem.MARGIN_TOP;
							rc.right = clRect.right - sfirstitem.MARGIN_RIGHT;
							rc.bottom = y+dat->rowHeight - sfirstitem.MARGIN_BOTTOM;
							if (!sitem.IGNORED)
							{
								if (!selected || DBGetContactSettingByte(NULL,"CLCExt","SelBlend",1))
									DrawAlpha(hwnd,hdcMem,&rc,sitem.COLOR,sitem.ALPHA, sitem.COLOR2, sitem.COLOR2_TRANSPARENT,sitem.GRADIENT,sfirstitem.CORNER);
								savedCORNER = sfirstitem.CORNER;
							}
							if (!selected || DBGetContactSettingByte(NULL,"CLCExt","SelBlend",1))
								DrawAlpha(hwnd,hdcMem,&rc,sfirstitem.COLOR,sfirstitem.ALPHA, sfirstitem.COLOR2, sfirstitem.COLOR2_TRANSPARENT,sfirstitem.GRADIENT,sfirstitem.CORNER);
						} 

					} else if (group->scanIndex==group->contactCount-1) // last item of group
																		// or list
					{
						if (group->parent!=NULL && !slastitem.IGNORED) // last item of group
						{											
							rc.left = slastitem.MARGIN_LEFT;
							rc.top = y  + slastitem.MARGIN_TOP;
							rc.right = clRect.right - slastitem.MARGIN_RIGHT;
							rc.bottom = y+dat->rowHeight - slastitem.MARGIN_BOTTOM;
							if (!sitem.IGNORED)
							{
								if (!selected || DBGetContactSettingByte(NULL,"CLCExt","SelBlend",1))
									DrawAlpha(hwnd,hdcMem,&rc,sitem.COLOR,sitem.ALPHA, sitem.COLOR2, sitem.COLOR2_TRANSPARENT,sitem.GRADIENT,slastitem.CORNER);
								savedCORNER = slastitem.CORNER;
							}
							if (!selected || DBGetContactSettingByte(NULL,"CLCExt","SelBlend",1))
								DrawAlpha(hwnd,hdcMem,&rc,slastitem.COLOR,slastitem.ALPHA, slastitem.COLOR2, slastitem.COLOR2_TRANSPARENT,slastitem.GRADIENT,slastitem.CORNER);

						} else if (group->parent==NULL && !slastitem_NG.IGNORED && bFirstNGdrawn) // last item of list (NON-group)
						{							
							rc.left = slastitem_NG.MARGIN_LEFT;
							rc.top = y  + slastitem_NG.MARGIN_TOP;
							rc.right = clRect.right - slastitem_NG.MARGIN_RIGHT;
							rc.bottom = y+dat->rowHeight - slastitem_NG.MARGIN_BOTTOM;
							if (!sitem.IGNORED)
							{
								if (!selected || DBGetContactSettingByte(NULL,"CLCExt","SelBlend",1))
									DrawAlpha(hwnd,hdcMem,&rc,sitem.COLOR,sitem.ALPHA, sitem.COLOR2, sitem.COLOR2_TRANSPARENT,sitem.GRADIENT,slastitem_NG.CORNER);
								savedCORNER = slastitem_NG.CORNER;
							}
							if (!selected || DBGetContactSettingByte(NULL,"CLCExt","SelBlend",1))
								DrawAlpha(hwnd,hdcMem,&rc,slastitem_NG.COLOR,slastitem_NG.ALPHA, slastitem_NG.COLOR2, slastitem_NG.COLOR2_TRANSPARENT,slastitem_NG.GRADIENT,slastitem_NG.CORNER);
						}
						else if (group->parent==NULL && !slastitem_NG.IGNORED && !bFirstNGdrawn) // single item of NON-group
						{							
							rc.left = ssingleitem_NG.MARGIN_LEFT;
							rc.top = y  + ssingleitem_NG.MARGIN_TOP;
							rc.right = clRect.right - ssingleitem_NG.MARGIN_RIGHT;
							rc.bottom = y+dat->rowHeight - ssingleitem_NG.MARGIN_BOTTOM;
							if (!sitem.IGNORED)
							{
								if (!selected || DBGetContactSettingByte(NULL,"CLCExt","SelBlend",1))
									DrawAlpha(hwnd,hdcMem,&rc,sitem.COLOR,sitem.ALPHA, sitem.COLOR2, sitem.COLOR2_TRANSPARENT,sitem.GRADIENT,ssingleitem_NG.CORNER);
								savedCORNER = ssingleitem_NG.CORNER;
							}
							if (!selected || DBGetContactSettingByte(NULL,"CLCExt","SelBlend",1))
								DrawAlpha(hwnd,hdcMem,&rc,ssingleitem_NG.COLOR,ssingleitem_NG.ALPHA, ssingleitem_NG.COLOR2, ssingleitem_NG.COLOR2_TRANSPARENT,ssingleitem_NG.GRADIENT,ssingleitem_NG.CORNER);
						}

					} else 					
					// Non-grouped items
					// we've already handled the case of last NON-grouped item above
					if (	group->contact[group->scanIndex].type!=CLCIT_GROUP // not a group
						&& group->parent==NULL // not grouped
						&& !bFirstNGdrawn
						)
					{
						// first NON-grouped
						bFirstNGdrawn = TRUE;						
						rc.left = sfirstitem_NG.MARGIN_LEFT;
						rc.top = y + sfirstitem_NG.MARGIN_TOP;
						rc.right = clRect.right - sfirstitem_NG.MARGIN_RIGHT;
						rc.bottom = y+dat->rowHeight - sfirstitem_NG.MARGIN_BOTTOM;
						if (!sitem.IGNORED)
						{
							if (!selected || DBGetContactSettingByte(NULL,"CLCExt","SelBlend",1))
								DrawAlpha(hwnd,hdcMem,&rc,sitem.COLOR,sitem.ALPHA, sitem.COLOR2, sitem.COLOR2_TRANSPARENT,sitem.GRADIENT,sfirstitem_NG.CORNER);
							savedCORNER = sfirstitem_NG.CORNER;
						}
						if (!selected || DBGetContactSettingByte(NULL,"CLCExt","SelBlend",1))
							DrawAlpha(hwnd,hdcMem,&rc,sfirstitem_NG.COLOR,sfirstitem_NG.ALPHA, sfirstitem_NG.COLOR2, sfirstitem_NG.COLOR2_TRANSPARENT,sfirstitem_NG.GRADIENT,sfirstitem_NG.CORNER);
					} 
					else if (	group->contact[group->scanIndex].type!=CLCIT_GROUP // not a group
						&& group->parent == NULL // not grouped
						&& group->scanIndex!=group->contactCount-1 // not last item
						&& bFirstNGdrawn
					)
					{
						if (!sitem.IGNORED) // draw default
						{					
							if (!selected || DBGetContactSettingByte(NULL,"CLCExt","SelBlend",1))
								DrawAlpha(hwnd,hdcMem,&rc,sitem.COLOR,sitem.ALPHA, sitem.COLOR2, sitem.COLOR2_TRANSPARENT,sitem.GRADIENT,sitem.CORNER);
							savedCORNER = sitem.CORNER;
						}
					}
					else if (!sitem.IGNORED) // draw default
					{						
						if (!selected || DBGetContactSettingByte(NULL,"CLCExt","SelBlend",1))
							DrawAlpha(hwnd,hdcMem,&rc,sitem.COLOR,sitem.ALPHA, sitem.COLOR2, sitem.COLOR2_TRANSPARENT,sitem.GRADIENT,sitem.CORNER);
						savedCORNER = sitem.CORNER;
					}
				}
			}
		
			// groups
			if (group->contact[group->scanIndex].type==CLCIT_GROUP)
			{
				
				StatusItems_t sempty;
				StatusItems_t sexpanded;
				StatusItems_t scollapsed;
				
				GetItemByStatus(ID_EXTBKEMPTYGROUPS, &sempty);
				GetItemByStatus(ID_EXTBKEXPANDEDGROUP, &sexpanded);
				GetItemByStatus(ID_EXTBKCOLLAPSEDDGROUP, &scollapsed);

				if (group->contact[group->scanIndex].group->contactCount==0)
				{
					if (!sempty.IGNORED)
					{					
						rc.left = sempty.MARGIN_LEFT;
						rc.top = y  + sempty.MARGIN_TOP;
						rc.right = clRect.right - sempty.MARGIN_RIGHT;
						rc.bottom = y+dat->rowHeight - sempty.MARGIN_BOTTOM;
						DrawAlpha(hwnd,hdcMem,&rc,sempty.COLOR,sempty.ALPHA, sempty.COLOR2, sempty.COLOR2_TRANSPARENT,sempty.GRADIENT,sempty.CORNER);						
						savedCORNER = sempty.CORNER;
					}
					
				} else if (group->contact[group->scanIndex].group->expanded)
				{
					if (!sexpanded.IGNORED)
					{					
						rc.left = sexpanded.MARGIN_LEFT;
						rc.top = y  + sexpanded.MARGIN_TOP;
						rc.right = clRect.right - sexpanded.MARGIN_RIGHT;
						rc.bottom = y+dat->rowHeight - sexpanded.MARGIN_BOTTOM;
						DrawAlpha(hwnd,hdcMem,&rc,sexpanded.COLOR,sexpanded.ALPHA, sexpanded.COLOR2, sexpanded.COLOR2_TRANSPARENT,sexpanded.GRADIENT,sexpanded.CORNER);						
						savedCORNER = sexpanded.CORNER;
					}
					
				} else 
				{
					if (!scollapsed.IGNORED) // collapsed but not empty
					{
						rc.left = scollapsed.MARGIN_LEFT;
						rc.top = y  + scollapsed.MARGIN_TOP;
						rc.right = clRect.right - scollapsed.MARGIN_RIGHT;
						rc.bottom = y+dat->rowHeight - scollapsed.MARGIN_BOTTOM;
						DrawAlpha(hwnd,hdcMem,&rc,scollapsed.COLOR,scollapsed.ALPHA, scollapsed.COLOR2, scollapsed.COLOR2_TRANSPARENT,scollapsed.GRADIENT,scollapsed.CORNER);
						savedCORNER = scollapsed.CORNER;
					} 						
				}
			}

			if(selected) {
				StatusItems_t sselected;
                GetItemByStatus(ID_EXTBKSELECTION, &sselected);
				
				if (!sselected.IGNORED)
				{					
					if (DBGetContactSettingByte(NULL,"CLCExt","EXBK_EqualSelection",0)==1 && savedCORNER!=-1)
					{						
						DrawAlpha(hwnd,hdcMem,&rc,sselected.COLOR,sselected.ALPHA, sselected.COLOR2, sselected.COLOR2_TRANSPARENT,sselected.GRADIENT,savedCORNER);
					} else 
					{
						rc.left = sselected.MARGIN_LEFT;
						rc.top = y  + sselected.MARGIN_TOP;
						rc.right = clRect.right - sselected.MARGIN_RIGHT;
						rc.bottom = y+dat->rowHeight - sselected.MARGIN_BOTTOM;						
						DrawAlpha(hwnd,hdcMem,&rc,sselected.COLOR,sselected.ALPHA, sselected.COLOR2, sselected.COLOR2_TRANSPARENT,sselected.GRADIENT,sselected.CORNER);
					}
				}
				if ( !DBGetContactSettingByte(NULL,"CLC","IgnoreSelforGroups",0) 
					|| group->contact[group->scanIndex].type!=CLCIT_GROUP)
					SetTextColor(hdcMem,dat->selTextColour);
			}
			else if(hottrack) {
				SetHotTrackColour(hdcMem,dat);
			}

			//checkboxes
			if(checkboxWidth) {
				RECT rc;
				HANDLE hTheme = NULL;

				// THEME
				if (IsWinVerXPPlus()) {
					if (!themeAPIHandle) {
						themeAPIHandle = GetModuleHandle("uxtheme");
						if (themeAPIHandle) {
							MyOpenThemeData = (HANDLE (WINAPI *)(HWND,LPCWSTR))MGPROC("OpenThemeData");
							MyCloseThemeData = (HRESULT (WINAPI *)(HANDLE))MGPROC("CloseThemeData");
							MyDrawThemeBackground = (HRESULT (WINAPI *)(HANDLE,HDC,int,int,const RECT *,const RECT *))MGPROC("DrawThemeBackground");
						}
					}
					// Make sure all of these methods are valid (i would hope either all or none work)
					if (MyOpenThemeData
							&&MyCloseThemeData
							&&MyDrawThemeBackground) {
						hTheme = MyOpenThemeData(hwnd,L"BUTTON");
					}
				}
				rc.left=dat->leftMargin+indent*dat->groupIndent;
				rc.right=rc.left+dat->checkboxSize;
				rc.top=y+((dat->rowHeight-dat->checkboxSize)>>1);
				rc.bottom=rc.top+dat->checkboxSize;
				if (hTheme) {
					MyDrawThemeBackground(hTheme, hdcMem, BP_CHECKBOX, group->contact[group->scanIndex].flags&CONTACTF_CHECKED?(hottrack?CBS_CHECKEDHOT:CBS_CHECKEDNORMAL):(hottrack?CBS_UNCHECKEDHOT:CBS_UNCHECKEDNORMAL), &rc, &rc);
				}
				else DrawFrameControl(hdcMem,&rc,DFC_BUTTON,DFCS_BUTTONCHECK|DFCS_FLAT|(group->contact[group->scanIndex].flags&CONTACTF_CHECKED?DFCS_CHECKED:0)|(hottrack?DFCS_HOT:0));
				if (hTheme&&MyCloseThemeData) {
					MyCloseThemeData(hTheme);
					hTheme = NULL;
				}
			}

			//icon
			if(group->contact[group->scanIndex].type==CLCIT_GROUP)
				iImage=group->contact[group->scanIndex].group->expanded?IMAGE_GROUPOPEN:IMAGE_GROUPSHUT;
			else if(group->contact[group->scanIndex].type==CLCIT_CONTACT)
				iImage=group->contact[group->scanIndex].iImage;
			if(iImage!=-1) {
				// this doesnt use CLS_CONTACTLIST since the colour prolly wont match anyway
				COLORREF colourFg=dat->selBkColour;
				int mode=ILD_NORMAL;
				if(hottrack) {colourFg=dat->hotTextColour;}
				else if(group->contact[group->scanIndex].type==CLCIT_CONTACT && group->contact[group->scanIndex].flags&CONTACTF_NOTONLIST) {colourFg=dat->fontInfo[FONTID_NOTONLIST].colour; mode=ILD_BLEND50;}
				if (group->contact[group->scanIndex].type==CLCIT_CONTACT && dat->showIdle && (group->contact[group->scanIndex].flags&CONTACTF_IDLE) && GetRealStatus(&group->contact[group->scanIndex],ID_STATUS_OFFLINE)!=ID_STATUS_OFFLINE)
					mode=ILD_SELECTED;
				ImageList_DrawEx(himlCListClc,iImage,hdcMem,dat->leftMargin+indent*dat->groupIndent+checkboxWidth,y+((dat->rowHeight-16)>>1),0,0,CLR_NONE,colourFg,mode);
			}

			//text
			if(group->contact[group->scanIndex].type==CLCIT_DIVIDER) {
				RECT rc;
				rc.top=y+(dat->rowHeight>>1); rc.bottom=rc.top+2;
				rc.left=dat->leftMargin+indent*dat->groupIndent;
				rc.right=rc.left+((clRect.right-rc.left-textSize.cx)>>1)-3;
				DrawEdge(hdcMem,&rc,BDR_SUNKENOUTER,BF_RECT);
				TextOut(hdcMem,rc.right+3,y+((dat->rowHeight-fontHeight)>>1),group->contact[group->scanIndex].szText,lstrlen(group->contact[group->scanIndex].szText));
				rc.left=rc.right+6+textSize.cx;
				rc.right=clRect.right;
				DrawEdge(hdcMem,&rc,BDR_SUNKENOUTER,BF_RECT);
			}
			else if(group->contact[group->scanIndex].type==CLCIT_GROUP) {
				RECT rc;
				if(szCounts[0]) {
					fontHeight=dat->fontInfo[FONTID_GROUPS].fontHeight;
					rc.left=dat->leftMargin+indent*dat->groupIndent+checkboxWidth+dat->iconXSpace;
					rc.right=min(clRect.right-countsSize.cx,rc.left+textSize.cx+spaceSize.cx);
					rc.top=y+((dat->rowHeight-fontHeight)>>1);
					rc.bottom=rc.top+textSize.cy;
					if(rc.right<rc.left+4) rc.right=clRect.right+1;
					else TextOut(hdcMem,rc.right,rc.top+groupCountsFontTopShift,szCounts,lstrlen(szCounts));
					ChangeToFont(hdcMem,dat,FONTID_GROUPS,&fontHeight);
					if(selected && !DBGetContactSettingByte(NULL,"CLC","IgnoreSelforGroups",0) )
						SetTextColor(hdcMem,dat->selTextColour);
					else if(hottrack)
						SetHotTrackColour(hdcMem,dat);
					rc.right--;
					ExtTextOut(hdcMem,rc.left,rc.top,ETO_CLIPPED,&rc,group->contact[group->scanIndex].szText,lstrlen(group->contact[group->scanIndex].szText),NULL);
				}				
				else if (DBGetContactSettingByte(NULL,"CLCExt","EXBK_CenterGroupnames",0)) 
				{
					rc.left=0;
					rc.right=clRect.right;
					rc.top=y+((dat->rowHeight-fontHeight)>>1);
					rc.bottom=rc.top+textSize.cy;
					DrawText(hdcMem, group->contact[group->scanIndex].szText,-1,&rc,DT_CENTER | DT_NOPREFIX);
				} else 
					TextOut(hdcMem,dat->leftMargin+indent*dat->groupIndent+checkboxWidth+dat->iconXSpace,y+((dat->rowHeight-fontHeight)>>1),group->contact[group->scanIndex].szText,lstrlen(group->contact[group->scanIndex].szText));
				
				if(dat->exStyle&CLS_EX_LINEWITHGROUPS) {
					rc.top=y+(dat->rowHeight>>1); rc.bottom=rc.top+2;
					rc.left=dat->leftMargin+indent*dat->groupIndent+checkboxWidth+dat->iconXSpace+width+3;
					rc.right=clRect.right-1-dat->extraColumnSpacing*dat->extraColumnsCount;
					if(rc.right-rc.left>1) DrawEdge(hdcMem,&rc,BDR_SUNKENOUTER,BF_RECT);
				}
			}
			else
			{
				char * szText = group->contact[group->scanIndex].szText;
				RECT rc;				
				rc.left=dat->leftMargin+indent*dat->groupIndent+checkboxWidth+dat->iconXSpace;
				rc.top=y+((dat->rowHeight-fontHeight)>>1);
				rc.right=(clRect.right - clRect.left) - ( rc.left >> 1 );
				rc.bottom=rc.top;
				DrawText(hdcMem, szText, lstrlen(szText), &rc, DT_EDITCONTROL | DT_NOPREFIX | DT_NOCLIP | DT_WORD_ELLIPSIS);
			}			
			if(selected) {
				if(group->contact[group->scanIndex].type!=CLCIT_DIVIDER) {
					char * szText = group->contact[group->scanIndex].szText;
					RECT rc;
					int qlen=lstrlen(dat->szQuickSearch);
					SetTextColor(hdcMem,dat->quickSearchColour);				
					rc.left=dat->leftMargin+indent*dat->groupIndent+checkboxWidth+dat->iconXSpace;
					rc.top=y+((dat->rowHeight-fontHeight)>>1);
					rc.right=(clRect.right - clRect.left) - ( rc.left >> 1 );
					rc.bottom=rc.top;
					if ( qlen ) DrawText(hdcMem, szText, qlen, &rc, DT_EDITCONTROL | DT_NOPREFIX | DT_NOCLIP | DT_WORD_ELLIPSIS);
				}
			}

			//extra icons
			for(iImage=0;iImage<dat->extraColumnsCount;iImage++) {
				COLORREF colourFg=dat->selBkColour;
				int mode=ILD_NORMAL;
				if(group->contact[group->scanIndex].iExtraImage[iImage]==0xFF) continue;
				if(selected) mode=ILD_SELECTED;
				else if(hottrack) {mode=ILD_FOCUS; colourFg=dat->hotTextColour;}
				else if(group->contact[group->scanIndex].type==CLCIT_CONTACT && group->contact[group->scanIndex].flags&CONTACTF_NOTONLIST) {colourFg=dat->fontInfo[FONTID_NOTONLIST].colour; mode=ILD_BLEND50;}
				ImageList_DrawEx(dat->himlExtraColumns,group->contact[group->scanIndex].iExtraImage[iImage],hdcMem,clRect.right-dat->extraColumnSpacing*(dat->extraColumnsCount-iImage),y+((dat->rowHeight-16)>>1),0,0,CLR_NONE,colourFg,mode);
			}
		}
		index++;
		y+=dat->rowHeight;
		if(group->contact[group->scanIndex].type==CLCIT_GROUP && group->contact[group->scanIndex].group->expanded) {
			group=group->contact[group->scanIndex].group;
			indent++;
			group->scanIndex=0;
			continue;
		}
		group->scanIndex++;
	}

	if(dat->iInsertionMark!=-1) {	//insertion mark
		HBRUSH hBrush,hoBrush;
		POINT pts[8];
		HRGN hRgn;

		pts[0].x=dat->leftMargin; pts[0].y=dat->iInsertionMark*dat->rowHeight-dat->yScroll-4;
		pts[1].x=pts[0].x+2;      pts[1].y=pts[0].y+3;
		pts[2].x=clRect.right-4;  pts[2].y=pts[1].y;
		pts[3].x=clRect.right-1;  pts[3].y=pts[0].y-1;
		pts[4].x=pts[3].x;        pts[4].y=pts[0].y+7;
		pts[5].x=pts[2].x+1;      pts[5].y=pts[1].y+2;
		pts[6].x=pts[1].x;        pts[6].y=pts[5].y;
		pts[7].x=pts[0].x;        pts[7].y=pts[4].y;
		hRgn=CreatePolygonRgn(pts,sizeof(pts)/sizeof(pts[0]),ALTERNATE);
		hBrush=CreateSolidBrush(dat->fontInfo[FONTID_CONTACTS].colour);
		hoBrush=(HBRUSH)SelectObject(hdcMem,hBrush);
		FillRgn(hdcMem,hRgn,hBrush);
		SelectObject(hdcMem,hoBrush);
		DeleteObject(hBrush);
	}
	if(!grey)
		BitBlt(hdc,rcPaint->left,rcPaint->top,rcPaint->right-rcPaint->left,rcPaint->bottom-rcPaint->top,hdcMem,rcPaint->left,rcPaint->top,SRCCOPY);	

	DeleteDC(hdcMem);
	if(hBrushAlternateGrey) DeleteObject(hBrushAlternateGrey);
	if(grey) {
		PBYTE bits;
		BITMAPINFOHEADER bmih={0};
		int i;
		int greyRed,greyGreen,greyBlue;
		COLORREF greyColour;
		bmih.biBitCount=32;
		bmih.biSize=sizeof(bmih);
		bmih.biCompression=BI_RGB;
		bmih.biHeight=-clRect.bottom;
		bmih.biPlanes=1;
		bmih.biWidth=clRect.right;
		bits=(PBYTE)mir_alloc(4*bmih.biWidth*-bmih.biHeight);
		GetDIBits(hdc,hBmpOsb,0,clRect.bottom,bits,(BITMAPINFO*)&bmih,DIB_RGB_COLORS);
		greyColour=GetSysColor(COLOR_3DFACE);
		greyRed=GetRValue(greyColour)*2;
		greyGreen=GetGValue(greyColour)*2;
		greyBlue=GetBValue(greyColour)*2;
		if(divide3[0]==255) {
			for(i=0;i<sizeof(divide3)/sizeof(divide3[0]);i++) divide3[i]=(i+1)/3;
		}
		for(i=4*clRect.right*clRect.bottom-4;i>=0;i-=4) {
			bits[i]=divide3[bits[i]+greyBlue];
			bits[i+1]=divide3[bits[i+1]+greyGreen];
			bits[i+2]=divide3[bits[i+2]+greyRed];
		}
		SetDIBitsToDevice(hdc,0,0,clRect.right,clRect.bottom,0,0,0,clRect.bottom,bits,(BITMAPINFO*)&bmih,DIB_RGB_COLORS);
		mir_free(bits);
	}
	DeleteObject(hBmpOsb);	
}