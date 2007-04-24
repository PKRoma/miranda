/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2007 Miranda ICQ/IM project, 
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
#include "m_clc.h"
#include "clc.h"
#include "commonprototypes.h"

//loads of stuff that didn't really fit anywhere else

BOOL RectHitTest(RECT *rc, int testx, int testy)
{
	return testx >= rc->left && testx < rc->right && testy >= rc->top && testy < rc->bottom;
}

int cliHitTest(HWND hwnd,struct ClcData *dat,int testx,int testy,struct ClcContact **contact,struct ClcGroup **group,DWORD *flags)
{
	struct ClcContact *hitcontact=NULL;
	struct ClcGroup *hitgroup=NULL;
	int hit=-1;
	RECT clRect;
	if (CLUI_TestCursorOnBorders()!=0)
	{
		if(flags) *flags=CLCHT_NOWHERE;
		return -1;
	}
	if(flags) *flags=0;
	GetClientRect(hwnd,&clRect);
	if(testx<0 || testy<0 || testy>=clRect.bottom || testx>=clRect.right) {
		if(flags) {
			if(testx<0) *flags|=CLCHT_TOLEFT;
			else if(testx>=clRect.right) *flags|=CLCHT_TORIGHT;
			if(testy<0) *flags|=CLCHT_ABOVE;
			else if(testy>=clRect.bottom) *flags|=CLCHT_BELOW;
		}
		return -1;
	}
	if(testx<dat->leftMargin) {
		if(flags) *flags|=CLCHT_INLEFTMARGIN|CLCHT_NOWHERE;
		return -1;
	}

	// Get hit item 
	hit = cliRowHitTest(dat, dat->yScroll + testy);

	if (hit != -1) 
		hit = cliGetRowByIndex(dat, hit, &hitcontact, &hitgroup);

	if(hit==-1) {
		if(flags) *flags|=CLCHT_NOWHERE|CLCHT_BELOWITEMS;
		return -1;
	}
	if(contact) *contact=hitcontact;
	if(group) *group=hitgroup;
	/////////

	if ( ((testx<hitcontact->pos_indent) && !dat->text_rtl) ||
		((testx>clRect.right-hitcontact->pos_indent) && dat->text_rtl) ) 
	{
		if(flags) *flags|=CLCHT_ONITEMINDENT;
		return hit;
	}

	if (RectHitTest(&hitcontact->pos_check, testx, testy)) 
	{
		if(flags) *flags|=CLCHT_ONITEMCHECK;
		return hit;
	}

	if (RectHitTest(&hitcontact->pos_avatar, testx, testy)) 
	{
		if(flags) *flags|=CLCHT_ONITEMICON;
		return hit;
	}

	if (RectHitTest(&hitcontact->pos_icon, testx, testy)) 
	{
		if(flags) *flags|=CLCHT_ONITEMICON;
		return hit;
	}

	//	if (testx>hitcontact->pos_extra) {
	//		if(flags)
	{
		//			int c = -1;
		int i;
		for(i = 0; i < dat->extraColumnsCount; i++) 
		{  
			if (RectHitTest(&hitcontact->pos_extra[i], testx, testy)) 
			{
				if(flags) *flags|=CLCHT_ONITEMEXTRA|(i<<24);
				return hit;
			}
		}
	}

	if (DBGetContactSettingByte(NULL,"CLC","HiLightMode",0)==1) // || DBGetContactSettingByte(NULL,"CLC","HiLightMode",0)==2)
	{
		if(flags) *flags|=CLCHT_ONITEMLABEL;
		return hit;
	}

	if (RectHitTest(&hitcontact->pos_label, testx, testy)) 
	{
		if(flags) *flags|=CLCHT_ONITEMLABEL;
		return hit;
	}

	if(flags) *flags|=CLCHT_NOWHERE;
	return hit;
}

void cliScrollTo(HWND hwnd,struct ClcData *dat,int desty,int noSmooth)
{
	DWORD startTick,nowTick;
	int oldy=dat->yScroll;
	RECT clRect,rcInvalidate;
	int maxy,previousy;

	if(dat->iHotTrack!=-1 && dat->yScroll != desty) {
		pcli->pfnInvalidateItem(hwnd,dat,dat->iHotTrack);
		dat->iHotTrack=-1;
		ReleaseCapture();
	}
	GetClientRect(hwnd,&clRect);
	rcInvalidate=clRect;
	//maxy=dat->rowHeight*GetGroupContentsCount(&dat->list,2)-clRect.bottom;
	maxy=cliGetRowTotalHeight(dat)-clRect.bottom;
	if(desty>maxy) desty=maxy;
	if(desty<0) desty=0;
	if(abs(desty-dat->yScroll)<4) noSmooth=1;
	if(!noSmooth && dat->exStyle&CLS_EX_NOSMOOTHSCROLLING) noSmooth=1;
	previousy=dat->yScroll;
	if(!noSmooth) {
		startTick=GetTickCount();
		for(;;) {
			nowTick=GetTickCount();
			if(nowTick>=startTick+dat->scrollTime) break;
			dat->yScroll=oldy+(desty-oldy)*(int)(nowTick-startTick)/dat->scrollTime;
			if(/*dat->backgroundBmpUse&CLBF_SCROLL || dat->hBmpBackground==NULL &&*/FALSE)
				ScrollWindowEx(hwnd,0,previousy-dat->yScroll,NULL,NULL,NULL,NULL,SW_INVALIDATE);
			else
			{
				CallService(MS_SKINENG_UPTATEFRAMEIMAGE,(WPARAM) hwnd, (LPARAM) 0); 
				//InvalidateRectZ(hwnd,NULL,FALSE);
			}
			previousy=dat->yScroll;
			SetScrollPos(hwnd,SB_VERT,dat->yScroll,TRUE);
			CallService(MS_SKINENG_UPTATEFRAMEIMAGE,(WPARAM) hwnd, (LPARAM) 0); 
			UpdateWindow(hwnd);
		}
	}
	dat->yScroll=desty;
	if((dat->backgroundBmpUse&CLBF_SCROLL || dat->hBmpBackground==NULL) && FALSE)
		ScrollWindowEx(hwnd,0,previousy-dat->yScroll,NULL,NULL,NULL,NULL,SW_INVALIDATE);
	else
		CLUI__cliInvalidateRect(hwnd,NULL,FALSE);
	SetScrollPos(hwnd,SB_VERT,dat->yScroll,TRUE);
}


void cliRecalcScrollBar(HWND hwnd,struct ClcData *dat)
{
	SCROLLINFO si={0};
	RECT clRect;
	NMCLISTCONTROL nm;
	if (LOCK_RECALC_SCROLLBAR) return;

	RowHeights_CalcRowHeights(dat, hwnd);

	GetClientRect(hwnd,&clRect);
	si.cbSize=sizeof(si);
	si.fMask=SIF_ALL;
	si.nMin=0;
	si.nMax=cliGetRowTotalHeight(dat)-1;
	si.nPage=clRect.bottom;
	si.nPos=dat->yScroll;

	nm.hdr.code=CLN_LISTSIZECHANGE;
	nm.hdr.hwndFrom=hwnd;
	nm.hdr.idFrom=0;//GetDlgCtrlID(hwnd);
	nm.pt.y=si.nMax;

	SendMessage(GetParent(hwnd),WM_NOTIFY,0,(LPARAM)&nm);       //post

	GetClientRect(hwnd,&clRect);
	si.cbSize=sizeof(si);
	si.fMask=SIF_ALL;
	si.nMin=0;
	si.nMax=cliGetRowTotalHeight(dat)-1;
	si.nPage=clRect.bottom;
	si.nPos=dat->yScroll;

	if ( GetWindowLong(hwnd,GWL_STYLE)&CLS_CONTACTLIST ) {
		if ( dat->noVScrollbar==0 ) SetScrollInfo(hwnd,SB_VERT,&si,TRUE);
		//else SetScrollInfo(hwnd,SB_VERT,&si,FALSE);
	} 
	else 
		SetScrollInfo(hwnd,SB_VERT,&si,TRUE);
	g_mutex_bSizing=1;
	cliScrollTo(hwnd,dat,dat->yScroll,1);
	g_mutex_bSizing=0;
}


static WNDPROC OldRenameEditWndProc;
static LRESULT CALLBACK RenameEditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) 
	{
	case WM_KEYDOWN:
		switch(wParam) 
		{
		case VK_RETURN:
			pcli->pfnEndRename(GetParent(hwnd),(struct ClcData*)GetWindowLong(hwnd,GWL_USERDATA),1);
			return 0;
		case VK_ESCAPE:
			pcli->pfnEndRename(GetParent(hwnd),(struct ClcData*)GetWindowLong(hwnd,GWL_USERDATA),0);
			return 0;
		}
		break;
	case WM_GETDLGCODE:
		if(lParam) 
		{
			MSG *msg=(MSG*)lParam;
			if(msg->message==WM_KEYDOWN && msg->wParam==VK_TAB) return 0;
			if(msg->message==WM_CHAR && msg->wParam=='\t') return 0;
		}
		return DLGC_WANTMESSAGE;
	case WM_KILLFOCUS:
		pcli->pfnEndRename(GetParent(hwnd),(struct ClcData*)GetWindowLong(hwnd,GWL_USERDATA),1);
		return 0;
	}
	return CallWindowProc(OldRenameEditWndProc,hwnd,msg,wParam,lParam);
}

void cliBeginRenameSelection(HWND hwnd,struct ClcData *dat)
{
	struct ClcContact *contact;
	struct ClcGroup *group;
	int indent,x,y,subident, h,w;
	RECT clRect;
	RECT r;


	KillTimer(hwnd,TIMERID_RENAME);
	ReleaseCapture();
	dat->iHotTrack=-1;
	dat->selection=cliGetRowByIndex(dat,dat->selection,&contact,&group);
	if(dat->selection==-1) return;
	if(contact->type!=CLCIT_CONTACT && contact->type!=CLCIT_GROUP) return;

	if (contact->type==CLCIT_CONTACT && contact->isSubcontact)
		subident=dat->subIndent;
	else 
		subident=0;

	for(indent=0;group->parent;indent++,group=group->parent);
	GetClientRect(hwnd,&clRect);
	x=indent*dat->groupIndent+dat->iconXSpace-2+subident;
	w=clRect.right-x;
	y=cliGetRowTopY(dat, dat->selection)-dat->yScroll;
	h=dat->row_heights[dat->selection];
	{
		int i;
		for (i=0; i<=FONTID_MODERN_MAX; i++)
			if (h<dat->fontModernInfo[i].fontHeight+4) h=dat->fontModernInfo[i].fontHeight+4;
	}
	//TODO contact->pos_label 


	{

		RECT rectW;      
		int h2;
		GetWindowRect(hwnd,&rectW);
		//       w=contact->pos_full_first_row.right-contact->pos_full_first_row.left;
		//       h=contact->pos_full_first_row.bottom-contact->pos_full_first_row.top;
		//w=clRect.right-x;
		//w=clRect.right-x;
		//x+=rectW.left;//+contact->pos_full_first_row.left;
		//y+=rectW.top;//+contact->pos_full_first_row.top;
		x=contact->pos_rename_rect.left+rectW.left;
		y=contact->pos_label.top+rectW.top;
		w=contact->pos_rename_rect.right-contact->pos_rename_rect.left;
		h2=contact->pos_label.bottom-contact->pos_label.top+4;
		h=h2;//max(h,h2);

	}

	{
		int a=0;
		if (contact->type==CLCIT_GROUP)
		{
			if (dat->row_align_group_mode==1) a|=ES_CENTER;
			else if (dat->row_align_group_mode==2) a|=ES_RIGHT;
		}
		if (dat->text_rtl) a|=EN_ALIGN_RTL_EC;
		if (contact->type==CLCIT_GROUP)
			dat->hwndRenameEdit=CreateWindow(TEXT("EDIT"),contact->szText,WS_POPUP|WS_BORDER|ES_AUTOHSCROLL|a,x,y,w,h,hwnd,NULL,g_hInst,NULL);
		else
			dat->hwndRenameEdit=CreateWindow(TEXT("EDIT"),pcli->pfnGetContactDisplayName(contact->hContact,0),WS_POPUP|WS_BORDER|ES_AUTOHSCROLL|a,x,y,w,h,hwnd,NULL,g_hInst,NULL);
	}
	SetWindowLong(dat->hwndRenameEdit,GWL_STYLE,GetWindowLong(dat->hwndRenameEdit,GWL_STYLE)&(~WS_CAPTION)|WS_BORDER);
	SetWindowLong(dat->hwndRenameEdit,GWL_USERDATA,(long)dat);
	OldRenameEditWndProc=(WNDPROC)SetWindowLong(dat->hwndRenameEdit,GWL_WNDPROC,(long)RenameEditSubclassProc);
	SendMessage(dat->hwndRenameEdit,WM_SETFONT,(WPARAM)(contact->type==CLCIT_GROUP?dat->fontModernInfo[FONTID_OPENGROUPS].hFont:dat->fontModernInfo[FONTID_CONTACTS].hFont),0);
	SendMessage(dat->hwndRenameEdit,EM_SETMARGINS,EC_LEFTMARGIN|EC_RIGHTMARGIN|EC_USEFONTINFO,0);
	SendMessage(dat->hwndRenameEdit,EM_SETSEL,0,(LPARAM)(-1));
	// SetWindowLong(dat->hwndRenameEdit,GWL_USERDATA,(long)hwnd);
	r.top=1;
	r.bottom=h-1;
	r.left=0;
	r.right=w;

	//ES_MULTILINE

	SendMessage(dat->hwndRenameEdit,EM_SETRECT,0,(LPARAM)(&r));

	CLUI_ShowWindowMod(dat->hwndRenameEdit,SW_SHOW);
	SetWindowPos(dat->hwndRenameEdit,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
	SetFocus(dat->hwndRenameEdit);
}

int GetDropTargetInformation(HWND hwnd,struct ClcData *dat,POINT pt)
{
	RECT clRect;
	int hit;
	struct ClcContact *contact=NULL,*movecontact=NULL;
	struct ClcGroup *group,*movegroup;
	DWORD hitFlags;
	int nSetSelection=-1;

	GetClientRect(hwnd,&clRect);
	dat->selection=dat->iDragItem;
	dat->iInsertionMark=-1;
	dat->nInsertionLevel=0;
	if(!PtInRect(&clRect,pt)) return DROPTARGET_OUTSIDE;

	hit=cliHitTest(hwnd,dat,pt.x,pt.y,&contact,&group,&hitFlags);
	cliGetRowByIndex(dat,dat->iDragItem,&movecontact,&movegroup);
	if(hit==dat->iDragItem) return DROPTARGET_ONSELF;
	if(hit==-1 || hitFlags&CLCHT_ONITEMEXTRA || !movecontact) return DROPTARGET_ONNOTHING;

	if(movecontact->type==CLCIT_GROUP) {
		struct ClcContact *bottomcontact=NULL,*topcontact=NULL;
		struct ClcGroup *topgroup=NULL, *bottomgroup=NULL;
		int topItem=-1,bottomItem=-1;
		int ok=0;
		if(pt.y+dat->yScroll<cliGetRowTopY(dat,hit)+dat->insertionMarkHitHeight || contact->type!=CLCIT_GROUP) {
			//could be insertion mark (above)
			topItem=hit-1; bottomItem=hit;
			bottomcontact=contact;
			bottomgroup=group;
			topItem=cliGetRowByIndex(dat,topItem,&topcontact,&topgroup);
			ok=1;
		} else if ((pt.y+dat->yScroll>=cliGetRowTopY(dat,hit+1)-dat->insertionMarkHitHeight)
			||(contact->type==CLCIT_GROUP && contact->group->expanded && contact->group->cl.count>0)) 
		{
			//could be insertion mark (below)
			topItem=hit; bottomItem=hit+1;
			topcontact=contact; topgroup=group;
			bottomItem=cliGetRowByIndex(dat,bottomItem,&bottomcontact,&bottomgroup);
			ok=1;	
		}
		if (ok)
		{
			if (bottomItem==-1 && contact->type==CLCIT_GROUP)
			{
				bottomItem=topItem+1;
			} 
			else 
			{
				if (bottomItem==-1 && contact->type!=CLCIT_GROUP && contact->groupId==0)
				{
					if (contact->type!=CLCIT_GROUP && contact->groupId==0)
					{
						bottomItem=topItem;
						cliGetRowByIndex(dat,bottomItem,&bottomcontact,&bottomgroup);
					}
				}
				if (bottomItem!=-1 && bottomcontact->type!=CLCIT_GROUP)
				{
					struct ClcGroup * gr=bottomgroup;
					do 
					{
						bottomItem=cliGetRowByIndex(dat,bottomItem-1,&bottomcontact,&bottomgroup);}
					while (bottomItem>=0 && bottomcontact->type!=CLCIT_GROUP && bottomgroup==gr);
					nSetSelection=bottomItem;
					bottomItem=cliGetRowByIndex(dat,bottomItem+1,&bottomcontact,&bottomgroup);
				}
			}

			if (bottomItem==-1)	bottomItem=topItem+1;
			{
				int bi=cliGetRowByIndex(dat,bottomItem,&bottomcontact,&bottomgroup);
				if (bi!=-1)
				{
					group=bottomgroup;                    
					if (bottomcontact==movecontact || group==movecontact->group)	return DROPTARGET_ONSELF;
					dat->nInsertionLevel=-1; // decreasing here
					for(;group;group=group->parent)
					{   
						dat->nInsertionLevel++;
						if (group==movecontact->group) return DROPTARGET_ONSELF;
					}
				}
			}           
			dat->iInsertionMark=bottomItem;
			dat->selection=nSetSelection;
			return DROPTARGET_INSERTION;
		}
	}
	if(contact->type==CLCIT_GROUP) 
	{
		if(dat->iInsertionMark==-1) 
		{
			if(movecontact->type==CLCIT_GROUP) 
			{	 //check not moving onto its own subgroup
				dat->iInsertionMark=hit+1;
				for(;group;group=group->parent) 
				{
					dat->nInsertionLevel++;
					if(group==movecontact->group) return DROPTARGET_ONSELF;
				}
			}
			dat->selection=hit;            
			return DROPTARGET_ONGROUP;
		}
	}
	dat->selection=hit;

	if (!mir_strcmp(contact->proto,"MetaContacts")&& (ServiceExists(MS_MC_ADDTOMETA))) return DROPTARGET_ONMETACONTACT;
	if (contact->isSubcontact && (ServiceExists(MS_MC_ADDTOMETA))) return DROPTARGET_ONSUBCONTACT;
	return DROPTARGET_ONCONTACT;
}
void LoadCLCOptions(HWND hwnd, struct ClcData *dat)
{ 
	int i;
	g_CluiData.fDisableSkinEngine=DBGetContactSettingByte(NULL,"ModernData","DisableEngine", 0);
	{	

		LOGFONTA lf;
		HFONT holdfont;
		SIZE fontSize;
		HDC hdc=GetDC(hwnd);
		for(i=0;i<=FONTID_MODERN_MAX;i++) {
			if(!dat->fontModernInfo[i].changed && dat->fontModernInfo[i].hFont) DeleteObject(dat->fontModernInfo[i].hFont);
			GetFontSetting(i,&lf,&dat->fontModernInfo[i].colour,&dat->fontModernInfo[i].effect,&dat->fontModernInfo[i].effectColour1,&dat->fontModernInfo[i].effectColour2);
			{
				LONG height;
				HDC hdc=GetDC(NULL);
				height=lf.lfHeight;
				lf.lfHeight=-MulDiv(lf.lfHeight, GetDeviceCaps(hdc, LOGPIXELSY), 72);
				ReleaseDC(NULL,hdc);				

				dat->fontModernInfo[i].hFont=CreateFontIndirectA(&lf);

				lf.lfHeight=height;
			}

			dat->fontModernInfo[i].changed=0;
			holdfont=SelectObject(hdc,dat->fontModernInfo[i].hFont);
			GetTextExtentPoint32A(hdc,"x",1,&fontSize);
			dat->fontModernInfo[i].fontHeight=fontSize.cy;
			//			if(fontSize.cy>dat->rowHeight && (!DBGetContactSettingByte(NULL,"CLC","DoNotCheckFontSize",0))) dat->rowHeight=fontSize.cy;
			if(holdfont) SelectObject(hdc,holdfont);
		}
		ReleaseDC(hwnd,hdc);
	}

	g_CluiData.bSortByOrder[0]=DBGetContactSettingByte(NULL,"CList","SortBy1",SETTING_SORTBY1_DEFAULT);
	g_CluiData.bSortByOrder[1]=DBGetContactSettingByte(NULL,"CList","SortBy2",SETTING_SORTBY2_DEFAULT);
	g_CluiData.bSortByOrder[2]=DBGetContactSettingByte(NULL,"CList","SortBy3",SETTING_SORTBY3_DEFAULT);
	g_CluiData.fSortNoOfflineBottom=DBGetContactSettingByte(NULL,"CList","NoOfflineBottom",SETTING_NOOFFLINEBOTTOM_DEFAULT);

	// Row
	dat->row_min_heigh = DBGetContactSettingWord(NULL,"CList","MinRowHeight",CLCDEFAULT_ROWHEIGHT);
	dat->row_border = DBGetContactSettingWord(NULL,"CList","RowBorder",1);
	dat->row_before_group_space =((hwnd!=pcli->hwndContactTree&&pcli->hwndContactTree!=NULL) || !DBGetContactSettingByte(NULL,"ModernData","UseAdvancedRowLayout",0))?0:DBGetContactSettingWord(NULL,"ModernSkin","SpaceBeforeGroup",0);
	dat->row_variable_height = DBGetContactSettingByte(NULL,"CList","VariableRowHeight",1);
	dat->row_align_left_items_to_left = DBGetContactSettingByte(NULL,"CList","AlignLeftItemsToLeft",1);
	dat->row_hide_group_icon = DBGetContactSettingByte(NULL,"CList","HideGroupsIcon",0);
	dat->row_align_right_items_to_right = DBGetContactSettingByte(NULL,"CList","AlignRightItemsToRight",1);
	//TODO: Add to settings
	dat->row_align_group_mode=DBGetContactSettingByte(NULL,"CList","AlignGroupCaptions",0);
	if (pcli->hwndContactTree==NULL || dat->hWnd==pcli->hwndContactTree)
	{
		for (i = 0 ; i < NUM_ITEM_TYPE ; i++)
		{
			char tmp[128];
			mir_snprintf(tmp, sizeof(tmp), "RowPos%d", i);
			dat->row_items[i] = DBGetContactSettingWord(NULL, "CList", tmp, i);
		}
	}
	else
	{
		int defItems[]= {ITEM_ICON, ITEM_TEXT, ITEM_EXTRA_ICONS,};
		for (i = 0 ; i < NUM_ITEM_TYPE; i++)
			dat->row_items[i]=(i<SIZEOF(defItems)) ? defItems[i] : -1;
	}

	// Avatar
	if (pcli->hwndContactTree == hwnd  || pcli->hwndContactTree==NULL)
	{
		dat->avatars_show = DBGetContactSettingByte(NULL,"CList","AvatarsShow",0);
		dat->avatars_draw_border = DBGetContactSettingByte(NULL,"CList","AvatarsDrawBorders",0);
		dat->avatars_border_color = (COLORREF)DBGetContactSettingDword(NULL,"CList","AvatarsBorderColor",0);
		dat->avatars_round_corners = DBGetContactSettingByte(NULL,"CList","AvatarsRoundCorners",1);
		dat->avatars_use_custom_corner_size = DBGetContactSettingByte(NULL,"CList","AvatarsUseCustomCornerSize",0);
		dat->avatars_custom_corner_size = DBGetContactSettingWord(NULL,"CList","AvatarsCustomCornerSize",4);
		dat->avatars_ignore_size_for_row_height = DBGetContactSettingByte(NULL,"CList","AvatarsIgnoreSizeForRow",0);
		dat->avatars_draw_overlay = DBGetContactSettingByte(NULL,"CList","AvatarsDrawOverlay",0);
		dat->avatars_overlay_type = DBGetContactSettingByte(NULL,"CList","AvatarsOverlayType",SETTING_AVATAR_OVERLAY_TYPE_NORMAL);
		dat->avatars_maxheight_size = DBGetContactSettingWord(NULL,"CList","AvatarsSize",30);
		dat->avatars_maxwidth_size = DBGetContactSettingWord(NULL,"CList","AvatarsWidth",0);
	}
	else
	{
		dat->avatars_show = 0;
		dat->avatars_draw_border = 0;
		dat->avatars_border_color = 0;
		dat->avatars_round_corners = 0;
		dat->avatars_use_custom_corner_size = 0;
		dat->avatars_custom_corner_size = 4;
		dat->avatars_ignore_size_for_row_height = 0;
		dat->avatars_draw_overlay = 0;
		dat->avatars_overlay_type = SETTING_AVATAR_OVERLAY_TYPE_NORMAL;
		dat->avatars_maxheight_size = 30;
		dat->avatars_maxwidth_size = 0;
	}

	// Icon
	if (pcli->hwndContactTree == hwnd|| pcli->hwndContactTree==NULL)
	{
		dat->icon_hide_on_avatar = DBGetContactSettingByte(NULL,"CList","IconHideOnAvatar",0);
		dat->icon_draw_on_avatar_space = DBGetContactSettingByte(NULL,"CList","IconDrawOnAvatarSpace",0);
		dat->icon_ignore_size_for_row_height = DBGetContactSettingByte(NULL,"CList","IconIgnoreSizeForRownHeight",0);
	}
	else
	{
		dat->icon_hide_on_avatar = 0;
		dat->icon_draw_on_avatar_space = 0;
		dat->icon_ignore_size_for_row_height = 0;
	}

	// Contact time
	if (pcli->hwndContactTree == hwnd|| pcli->hwndContactTree==NULL)
	{
		dat->contact_time_show = DBGetContactSettingByte(NULL,"CList","ContactTimeShow",0);
		dat->contact_time_show_only_if_different = DBGetContactSettingByte(NULL,"CList","ContactTimeShowOnlyIfDifferent",1);
	}
	else
	{
		dat->contact_time_show = 0;
		dat->contact_time_show_only_if_different = 0;
	}
	//issue #0065 "Client time shows wrong" fix
	{
		TIME_ZONE_INFORMATION tzinfo;
		int nOffset=0;
		DWORD dwResult;
		dwResult = GetTimeZoneInformation(&tzinfo);
		nOffset = -(tzinfo.Bias + tzinfo.StandardBias) * 60;
		dat->local_gmt_diff=dat->local_gmt_diff_dst=(DWORD)nOffset;
	}
	// Text
	dat->text_rtl = DBGetContactSettingByte(NULL,"CList","TextRTL",0);
	dat->text_align_right = DBGetContactSettingByte(NULL,"CList","TextAlignToRight",0);
	dat->text_replace_smileys = DBGetContactSettingByte(NULL,"CList","TextReplaceSmileys",1);
	dat->text_resize_smileys = DBGetContactSettingByte(NULL,"CList","TextResizeSmileys",1);
	dat->text_smiley_height = 0;
	dat->text_use_protocol_smileys = DBGetContactSettingByte(NULL,"CList","TextUseProtocolSmileys",1);

	if (pcli->hwndContactTree == hwnd|| pcli->hwndContactTree==NULL)
	{
		dat->text_ignore_size_for_row_height = DBGetContactSettingByte(NULL,"CList","TextIgnoreSizeForRownHeight",0);
	}
	else
	{
		dat->text_ignore_size_for_row_height = 0;
	}

	// First line
	dat->first_line_draw_smileys = DBGetContactSettingByte(NULL,"CList","FirstLineDrawSmileys",1);
	dat->first_line_append_nick = DBGetContactSettingByte(NULL,"CList","FirstLineAppendNick",0);
	gl_TrimText=DBGetContactSettingByte(NULL,"CList","TrimText",1);

	// Second line
	if (pcli->hwndContactTree == hwnd || pcli->hwndContactTree==NULL)
	{
		dat->second_line_show = DBGetContactSettingByte(NULL,"CList","SecondLineShow",1);
		dat->second_line_top_space = DBGetContactSettingWord(NULL,"CList","SecondLineTopSpace",2);
		dat->second_line_draw_smileys = DBGetContactSettingByte(NULL,"CList","SecondLineDrawSmileys",1);
		dat->second_line_type = DBGetContactSettingWord(NULL,"CList","SecondLineType",TEXT_STATUS_MESSAGE);
		{
			DBVARIANT dbv={0};

			if (!DBGetContactSettingTString(NULL, "CList","SecondLineText", &dbv))
			{
				lstrcpyn(dat->second_line_text, dbv.ptszVal, SIZEOF(dat->second_line_text)-1);
				dat->second_line_text[SIZEOF(dat->second_line_text)-1] = '\0';
				DBFreeVariant(&dbv);
			}
			else
			{
				dat->second_line_text[0] = '\0';
			}
		}
		dat->second_line_xstatus_has_priority = DBGetContactSettingByte(NULL,"CList","SecondLineXStatusHasPriority",1);
		dat->second_line_show_status_if_no_away=DBGetContactSettingByte(NULL,"CList","SecondLineShowStatusIfNoAway",0);
		dat->second_line_show_listening_if_no_away=DBGetContactSettingByte(NULL,"CList","SecondLineShowListeningIfNoAway",1);
		dat->second_line_use_name_and_message_for_xstatus = DBGetContactSettingByte(NULL,"CList","SecondLineUseNameAndMessageForXStatus",0);
	}
	else
	{
		dat->second_line_show = 0;
		dat->second_line_top_space = 0;
		dat->second_line_draw_smileys = 0;
		dat->second_line_type = TEXT_STATUS_MESSAGE;
		dat->second_line_text[0] = '\0';
		dat->second_line_xstatus_has_priority = 1;
		dat->second_line_use_name_and_message_for_xstatus = 0;
	}


	// Third line
	if (pcli->hwndContactTree == hwnd || pcli->hwndContactTree==NULL)
	{
		dat->third_line_show = DBGetContactSettingByte(NULL,"CList","ThirdLineShow",0);
		dat->third_line_top_space = DBGetContactSettingWord(NULL,"CList","ThirdLineTopSpace",2);
		dat->third_line_draw_smileys = DBGetContactSettingByte(NULL,"CList","ThirdLineDrawSmileys",0);
		dat->third_line_type = DBGetContactSettingWord(NULL,"CList","ThirdLineType",TEXT_STATUS);
		{
			DBVARIANT dbv={0};

			if (!DBGetContactSettingTString(NULL, "CList","ThirdLineText", &dbv))
			{
				lstrcpyn(dat->third_line_text, dbv.ptszVal, SIZEOF(dat->third_line_text)-1);
				dat->third_line_text[SIZEOF(dat->third_line_text)-1] = '\0';
				DBFreeVariant(&dbv);
			}
			else
			{
				dat->third_line_text[0] = '\0';
			}
		}
		dat->third_line_xstatus_has_priority = DBGetContactSettingByte(NULL,"CList","ThirdLineXStatusHasPriority",1);
		dat->third_line_show_status_if_no_away=DBGetContactSettingByte(NULL,"CList","ThirdLineShowStatusIfNoAway",0);
		dat->third_line_show_listening_if_no_away=DBGetContactSettingByte(NULL,"CList","ThirdLineShowListeningIfNoAway",1);
		dat->third_line_use_name_and_message_for_xstatus = DBGetContactSettingByte(NULL,"CList","ThirdLineUseNameAndMessageForXStatus",0);
	}
	else
	{
		dat->third_line_show = 0;
		dat->third_line_top_space = 0;
		dat->third_line_draw_smileys = 0;
		dat->third_line_type = TEXT_STATUS_MESSAGE;
		dat->third_line_text[0] = '\0';
		dat->third_line_xstatus_has_priority = 1;
		dat->third_line_use_name_and_message_for_xstatus = 0;
	}

	dat->leftMargin=DBGetContactSettingByte(NULL,"CLC","LeftMargin",CLCDEFAULT_LEFTMARGIN);
	dat->rightMargin=DBGetContactSettingByte(NULL,"CLC","RightMargin",CLCDEFAULT_RIGHTMARGIN);
	dat->exStyle=DBGetContactSettingDword(NULL,"CLC","ExStyle",GetDefaultExStyle());
	dat->scrollTime=DBGetContactSettingWord(NULL,"CLC","ScrollTime",CLCDEFAULT_SCROLLTIME);
	dat->force_in_dialog=(pcli->hwndContactTree)?(hwnd!=pcli->hwndContactTree):0;
	dat->groupIndent=DBGetContactSettingByte(NULL,"CLC","GroupIndent",CLCDEFAULT_GROUPINDENT);
	dat->subIndent=DBGetContactSettingByte(NULL,"CLC","SubIndent",CLCDEFAULT_GROUPINDENT);
	dat->gammaCorrection=DBGetContactSettingByte(NULL,"CLC","GammaCorrect",CLCDEFAULT_GAMMACORRECT);
	dat->showIdle=DBGetContactSettingByte(NULL,"CLC","ShowIdle",CLCDEFAULT_SHOWIDLE);
	dat->noVScrollbar=DBGetContactSettingByte(NULL,"CLC","NoVScrollBar",0);
	SendMessage(hwnd,INTM_SCROLLBARCHANGED,0,0);
	
	if (dat->hBmpBackground) {DeleteObject(dat->hBmpBackground); dat->hBmpBackground=NULL;}
	if (dat->hMenuBackground) {DeleteObject(dat->hMenuBackground); dat->hMenuBackground=NULL;}
	
	dat->useWindowsColours = DBGetContactSettingByte(NULL, "CLC", "UseWinColours", CLCDEFAULT_USEWINDOWSCOLOURS);

	if (g_CluiData.fDisableSkinEngine)
	{
		DBVARIANT dbv;
		if(!dat->bkChanged) 
		{
			dat->bkColour=DBGetContactSettingDword(NULL,"CLC","BkColour",GetSysColor(COLOR_3DFACE));
			{	
				if(DBGetContactSettingByte(NULL,"CLC","UseBitmap",CLCDEFAULT_USEBITMAP)) 
				{
					if(!DBGetContactSetting(NULL,"CLC","BkBitmap",&dbv)) 
					{
						dat->hBmpBackground=(HBITMAP)CallService(MS_UTILS_LOADBITMAP,0,(LPARAM)dbv.pszVal);				
						DBFreeVariant(&dbv);						
					}
				}
			}
			dat->backgroundBmpUse=DBGetContactSettingWord(NULL,"CLC","BkBmpUse",CLCDEFAULT_BKBMPUSE);
		}		
		dat->MenuBkColor=DBGetContactSettingDword(NULL,"Menu","BkColour",CLCDEFAULT_BKCOLOUR);
		dat->MenuBkHiColor=DBGetContactSettingDword(NULL,"Menu","SelBkColour",CLCDEFAULT_SELBKCOLOUR);

		dat->MenuTextColor=DBGetContactSettingDword(NULL,"Menu","TextColour",CLCDEFAULT_TEXTCOLOUR);
		dat->MenuTextHiColor=DBGetContactSettingDword(NULL,"Menu","SelTextColour",CLCDEFAULT_SELTEXTCOLOUR);
		
		if(DBGetContactSettingByte(NULL,"Menu","UseBitmap",CLCDEFAULT_USEBITMAP)) {
			if(!DBGetContactSetting(NULL,"Menu","BkBitmap",&dbv)) {
				dat->hMenuBackground=(HBITMAP)CallService(MS_UTILS_LOADBITMAP,0,(LPARAM)dbv.pszVal);
				//mir_free_and_nill(dbv.pszVal);
				DBFreeVariant(&dbv);
			}
		}
		dat->MenuBmpUse=DBGetContactSettingWord(NULL,"Menu","BkBmpUse",CLCDEFAULT_BKBMPUSE);
	}

	dat->greyoutFlags=DBGetContactSettingDword(NULL,"CLC","GreyoutFlags",CLCDEFAULT_GREYOUTFLAGS);
	dat->offlineModes=DBGetContactSettingDword(NULL,"CLC","OfflineModes",CLCDEFAULT_OFFLINEMODES);
	dat->selBkColour=DBGetContactSettingDword(NULL,"CLC","SelBkColour",CLCDEFAULT_SELBKCOLOUR);
	dat->selTextColour=DBGetContactSettingDword(NULL,"CLC","SelTextColour",CLCDEFAULT_SELTEXTCOLOUR);
	dat->hotTextColour=DBGetContactSettingDword(NULL,"CLC","HotTextColour",CLCDEFAULT_HOTTEXTCOLOUR);
	dat->quickSearchColour=DBGetContactSettingDword(NULL,"CLC","QuickSearchColour",CLCDEFAULT_QUICKSEARCHCOLOUR);
	dat->IsMetaContactsEnabled=(!(GetWindowLong(hwnd,GWL_STYLE)&CLS_MANUALUPDATE)) &&
		DBGetContactSettingByte(NULL,"MetaContacts","Enabled",1) && ServiceExists(MS_MC_GETDEFAULTCONTACT);
	dat->MetaIgnoreEmptyExtra=DBGetContactSettingByte(NULL,"CLC","MetaIgnoreEmptyExtra",1);
	dat->expandMeta=DBGetContactSettingByte(NULL,"CLC","MetaExpanding",1);
	dat->useMetaIcon=DBGetContactSettingByte(NULL,"CLC","Meta",0);
	
	dat->drawOverlayedStatus=DBGetContactSettingByte(NULL,"CLC","DrawOverlayedStatus",3);

	dat->dbbMetaHideExtra=DBGetContactSettingByte(NULL,"CLC","MetaHideExtra",0);
	dat->dbbBlendInActiveState=DBGetContactSettingByte(NULL,"CLC","BlendInActiveState",0);
	dat->dbbBlend25=DBGetContactSettingByte(NULL,"CLC","Blend25%",1);
	if ((pcli->hwndContactTree == hwnd || pcli->hwndContactTree==NULL))
	{
		IvalidateDisplayNameCache(16);

	}

	{
		NMHDR hdr;
		hdr.code=CLN_OPTIONSCHANGED;
		hdr.hwndFrom=hwnd;
		hdr.idFrom=0;//GetDlgCtrlID(hwnd);
		SendMessage(GetParent(hwnd),WM_NOTIFY,0,(LPARAM)&hdr);
	}
	SendMessage(hwnd,WM_SIZE,0,0);

}