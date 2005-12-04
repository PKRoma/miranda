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
#include "m_clc.h"
#include "clc.h"

//loads of stuff that didn't really fit anywhere else

char *GetGroupCountsText(struct ClcData *dat,struct ClcContact *contact)
{
	static char szName[32];
	int onlineCount,totalCount;
	struct ClcGroup *group,*topgroup;

	if(contact->type!=CLCIT_GROUP || !(dat->exStyle&CLS_EX_SHOWGROUPCOUNTS))
		return "";

	group=topgroup=contact->group;
	onlineCount=0;
	totalCount=group->totalMembers;
	group->scanIndex=0;
	for(;;) {
		if(group->scanIndex==group->cl.count) {
			if(group==topgroup) break;
			group=group->parent;
		}
		else if(group->cl.items[group->scanIndex]->type==CLCIT_GROUP) {
			group=group->cl.items[group->scanIndex]->group;
			group->scanIndex=0;
			totalCount+=group->totalMembers;
			continue;
		}
		else if(group->cl.items[group->scanIndex]->type==CLCIT_CONTACT)
			if(group->cl.items[group->scanIndex]->flags&CONTACTF_ONLINE && !group->cl.items[group->scanIndex]->isSubcontact) onlineCount++;
		group->scanIndex++;
	}
	if(onlineCount==0 && dat->exStyle&CLS_EX_HIDECOUNTSWHENEMPTY) return "";
	_snprintf(szName,SIZEOF(szName),"(%u/%u)",onlineCount,totalCount);
	return szName;
}

int HitTest(HWND hwnd,struct ClcData *dat,int testx,int testy,struct ClcContact **contact,struct ClcGroup **group,DWORD *flags)
{
	struct ClcContact *hitcontact;
	struct ClcGroup *hitgroup;
	int hit,indent,width,i,cxSmIcon;
	int checkboxWidth, subident,ic=0;
	SIZE textSize;
	HDC hdc;
	HFONT oldfont;
	RECT clRect;
	DWORD style=GetWindowLong(hwnd,GWL_STYLE);

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
	hit=GetRowByIndex(dat ,(testy+dat->yScroll)/dat->rowHeight,&hitcontact,&hitgroup);
	if(hit==-1) {
		if(flags) *flags|=CLCHT_NOWHERE|CLCHT_BELOWITEMS;
		return -1;
	}
	if(contact) *contact=hitcontact;
	if(group) *group=hitgroup;
	/////////
	if (hitcontact->type==CLCIT_CONTACT && hitcontact->isSubcontact)
		subident=dat->rowHeight/2;
	else
		subident=0;

	for(indent=0;hitgroup->parent;indent++,hitgroup=hitgroup->parent);
	if(testx<dat->leftMargin+indent*dat->groupIndent+subident) {
		if(flags) *flags|=CLCHT_ONITEMINDENT;
		return hit;
	}
	checkboxWidth=0;
	if(style&CLS_CHECKBOXES && hitcontact->type==CLCIT_CONTACT) checkboxWidth=dat->checkboxSize+2;
	if(style&CLS_GROUPCHECKBOXES && hitcontact->type==CLCIT_GROUP) checkboxWidth=dat->checkboxSize+2;
	if(hitcontact->type==CLCIT_INFO && hitcontact->flags&CLCIIF_CHECKBOX) checkboxWidth=dat->checkboxSize+2;
	if(testx<dat->leftMargin+indent*dat->groupIndent+checkboxWidth+subident) {
		if(flags) *flags|=CLCHT_ONITEMCHECK;
		return hit;
	}
	if(testx<dat->leftMargin+indent*dat->groupIndent+checkboxWidth+dat->iconXSpace+subident) {
		if(flags) *flags|=CLCHT_ONITEMICON;
		return hit;
	}
	
	hdc=GetDC(hwnd);
	GetTextExtentPoint32(hdc,hitcontact->szText,lstrlen(hitcontact->szText),&textSize);
	width=textSize.cx;

	cxSmIcon=GetSystemMetrics(SM_CXSMICON);

	for(i=0;i<dat->extraColumnsCount;i++) {
		int x;
		if(hitcontact->iExtraImage[i]==0xFF) continue;

			if ((style&CLS_EX_MULTICOLUMNALIGNLEFT))
			{							
				x=(dat->leftMargin+indent*dat->groupIndent+checkboxWidth+dat->iconXSpace-2+width);
				x+=16;
				x=x+dat->extraColumnSpacing*(ic);
				if (i==dat->extraColumnsCount-1) {x=clRect.right-18;}
			}else
			{
				x=clRect.right-dat->extraColumnSpacing*(dat->extraColumnsCount-i);
			}
		ic++;

		if(testx>=x &&
		   testx<x+cxSmIcon) {
			if(flags) *flags|=CLCHT_ONITEMEXTRA|(i<<24);
			
			ReleaseDC(hwnd,hdc);
			return hit;
		}
	}	


	if(hitcontact->type==CLCIT_GROUP) 
		oldfont=SelectObject(hdc,dat->fontInfo[FONTID_GROUPS].hFont);
	else oldfont=SelectObject(hdc,dat->fontInfo[FONTID_CONTACTS].hFont);
	if (DBGetContactSettingByte(NULL,"CLC","HiLightMode",0)==1)
	{
		if(flags) *flags|=CLCHT_ONITEMLABEL;
		SelectObject(hdc,oldfont);
		ReleaseDC(hwnd,hdc);
		return hit;
	}
	
	GetTextExtentPoint32(hdc,hitcontact->szText,lstrlen(hitcontact->szText),&textSize);
	width=textSize.cx;
	if(hitcontact->type==CLCIT_GROUP) {
		char *szCounts;
		szCounts=GetGroupCountsText(dat,hitcontact);
		if(szCounts[0]) {
			GetTextExtentPoint32A(hdc," ",1,&textSize);
			width+=textSize.cx;
			SelectObject(hdc,dat->fontInfo[FONTID_GROUPCOUNTS].hFont);
			GetTextExtentPoint32A(hdc,szCounts,lstrlenA(szCounts),&textSize);
			width+=textSize.cx;
		}
	}
	SelectObject(hdc,oldfont);
	ReleaseDC(hwnd,hdc);
	if(testx<dat->leftMargin+indent*dat->groupIndent+checkboxWidth+dat->iconXSpace+width+4+subident) {
		if(flags) *flags|=CLCHT_ONITEMLABEL;
		return hit;
	}
	if(flags) *flags|=CLCHT_NOWHERE;
	return -1;
}

void ScrollTo(HWND hwnd,struct ClcData *dat,int desty,int noSmooth)
{
	DWORD startTick,nowTick;
	int oldy=dat->yScroll;
	RECT clRect,rcInvalidate;
	int maxy,previousy;

	if(dat->iHotTrack!=-1 && dat->yScroll != desty) {
		InvalidateItem(hwnd,dat,dat->iHotTrack);
		dat->iHotTrack=-1;
		ReleaseCapture();
	}
	GetClientRect(hwnd,&clRect);
	rcInvalidate=clRect;
	maxy=dat->rowHeight*GetGroupContentsCount(&dat->list,2)-clRect.bottom;
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
			if(dat->backgroundBmpUse&CLBF_SCROLL || dat->hBmpBackground==NULL)
				ScrollWindowEx(hwnd,0,previousy-dat->yScroll,NULL,NULL,NULL,NULL,SW_INVALIDATE);
			else
				InvalidateRect(hwnd,NULL,FALSE);
			previousy=dat->yScroll;
		  	SetScrollPos(hwnd,SB_VERT,dat->yScroll,TRUE);
			UpdateWindow(hwnd);
		}
	}
	dat->yScroll=desty;
	if(dat->backgroundBmpUse&CLBF_SCROLL || dat->hBmpBackground==NULL)
		ScrollWindowEx(hwnd,0,previousy-dat->yScroll,NULL,NULL,NULL,NULL,SW_INVALIDATE);
	else
		InvalidateRect(hwnd,NULL,FALSE);
	SetScrollPos(hwnd,SB_VERT,dat->yScroll,TRUE);
}

void EnsureVisible(HWND hwnd,struct ClcData *dat,int iItem,int partialOk)
{
	int itemy,newy;
	int moved=0;
	RECT clRect;


	if (dat==NULL||IsBadCodePtr((void *)dat)) 
	{
		OutputDebugStringA("dat is null __FILE____LINE__");
		return ;
	}

	GetClientRect(hwnd,&clRect);
	itemy=iItem*dat->rowHeight;
	if(partialOk) {
		if(itemy+dat->rowHeight-1<dat->yScroll) {newy=itemy; moved=1;}
		else if(itemy>=dat->yScroll+clRect.bottom) {newy=itemy-clRect.bottom+dat->rowHeight; moved=1;}
	}
	else {
		if(itemy<dat->yScroll) {newy=itemy; moved=1;}
		else if(itemy>=dat->yScroll+clRect.bottom-dat->rowHeight) {newy=itemy-clRect.bottom+dat->rowHeight; moved=1;}
	}
	if(moved)
		ScrollTo(hwnd,dat,newy,0);
}

void RecalcScrollBar(HWND hwnd,struct ClcData *dat)
{
	SCROLLINFO si={0};
	RECT clRect;
	NMCLISTCONTROL nm;
	boolean sbar=FALSE;
	
	GetClientRect(hwnd,&clRect);
	

	si.cbSize=sizeof(si);
	si.fMask=SIF_ALL;
	si.nMin=0;
	si.nMax=dat->rowHeight*GetGroupContentsCount(&dat->list,2)-1;
	si.nPage=clRect.bottom;
	si.nPos=dat->yScroll;
	
	nm.hdr.code=CLN_LISTSIZECHANGE;
	nm.hdr.hwndFrom=hwnd;
	nm.hdr.idFrom=GetDlgCtrlID(hwnd);
	nm.pt.y=si.nMax;
	SendMessage(GetParent(hwnd),WM_NOTIFY,0,(LPARAM)&nm);


	GetClientRect(hwnd,&clRect);
	si.cbSize=sizeof(si);
	si.fMask=SIF_ALL;
	si.nMin=0;
	si.nMax=dat->rowHeight*GetGroupContentsCount(&dat->list,2)-1;
	si.nPage=clRect.bottom;
	si.nPos=dat->yScroll;


	sbar=(dat->noVScrollbar==1||(int)si.nPage>si.nMax);
	
	ShowScrollBar(hwnd,SB_VERT,sbar? FALSE : TRUE);
	if (!sbar)
	{	
	if ( GetWindowLong(hwnd,GWL_STYLE)&CLS_CONTACTLIST ) {
		if ( dat->noVScrollbar==0 ) SetScrollInfo(hwnd,SB_VERT,&si,TRUE);
		else SetScrollInfo(hwnd,SB_VERT,&si,FALSE);
	} else SetScrollInfo(hwnd,SB_VERT,&si,TRUE);
	
	}
	ScrollTo(hwnd,dat,dat->yScroll,1);
}

void SetGroupExpand(HWND hwnd,struct ClcData *dat,struct ClcGroup *group,int newState)
{
	int contentCount;
	int groupy;
	int newy;
	RECT clRect;
	NMCLISTCONTROL nm;

	if(newState==-1) group->expanded^=1;
	else {
		if(group->expanded==(newState!=0)) return;
		group->expanded=newState!=0;
	}
	InvalidateRect(hwnd,NULL,FALSE);
	contentCount=GetGroupContentsCount(group,1);
	groupy=GetRowsPriorTo(&dat->list,group,-1);
	if(dat->selection>groupy && dat->selection<groupy+contentCount) dat->selection=groupy;
	GetClientRect(hwnd,&clRect);
	newy=dat->yScroll;
	if((groupy+contentCount+1)*dat->rowHeight>=newy+clRect.bottom)
		newy=(groupy+contentCount+1)*dat->rowHeight-clRect.bottom;
	if(newy>groupy*dat->rowHeight) newy=groupy*dat->rowHeight;
	RecalcScrollBar(hwnd,dat);
	ScrollTo(hwnd,dat,newy,0);
	nm.hdr.code=CLN_EXPANDED;
	nm.hdr.hwndFrom=hwnd;
	nm.hdr.idFrom=GetDlgCtrlID(hwnd);
	nm.hItem=(HANDLE)group->groupId;
	nm.action=group->expanded;
	SendMessage(GetParent(hwnd),WM_NOTIFY,0,(LPARAM)&nm);
}

void DoSelectionDefaultAction(HWND hwnd,struct ClcData *dat)
{
	struct ClcContact *contact;

	if(dat->selection==-1) return;
	dat->szQuickSearch[0]=0;
	if(GetRowByIndex(dat,dat->selection,&contact,NULL)==-1) return;
	if(contact->type==CLCIT_GROUP)
		SetGroupExpand(hwnd,dat,contact->group,-1);
	if(contact->type==CLCIT_CONTACT)
		CallService(MS_CLIST_CONTACTDOUBLECLICKED,(WPARAM)contact->hContact,0);
}

int FindRowByText(HWND hwnd,struct ClcData *dat,const TCHAR *text,int prefixOk)
{
	struct ClcGroup *group=&dat->list;
	int testlen=lstrlen(text);

	group->scanIndex=0;
	for(;;) {
		if(group->scanIndex==group->cl.count) {
			group=group->parent;
			if(group==NULL) break;
			group->scanIndex++;
			continue;
		}
		if(group->cl.items[group->scanIndex]->type!=CLCIT_DIVIDER) {
			if((prefixOk && !_tcsnicmp(text,group->cl.items[group->scanIndex]->szText,testlen)) ||
			   (!prefixOk && !lstrcmpi(text,group->cl.items[group->scanIndex]->szText))) {
				struct ClcGroup *contactGroup=group;
				int contactScanIndex=group->scanIndex;
				for(;group;group=group->parent) SetGroupExpand(hwnd,dat,group,1);
				return GetRowsPriorTo(&dat->list,contactGroup,contactScanIndex);
			}
			if(group->cl.items[group->scanIndex]->type==CLCIT_GROUP) {
				if(!(dat->exStyle&CLS_EX_QUICKSEARCHVISONLY) || group->cl.items[group->scanIndex]->group->expanded) {
					group=group->cl.items[group->scanIndex]->group;
					group->scanIndex=0;
					continue;
				}
			}
		}
		group->scanIndex++;
	}
	return -1;
}

void EndRename(HWND hwnd,struct ClcData *dat,int save)
{
	HWND hwndEdit=dat->hwndRenameEdit;
	
	if(dat->hwndRenameEdit==NULL) return;
	dat->hwndRenameEdit=NULL;
	if(save) {
		TCHAR text[120];
		struct ClcContact *contact;
		GetWindowText(hwndEdit,text,SIZEOF(text));
		if(GetRowByIndex(dat,dat->selection,&contact,NULL)!=-1) {
			if(lstrcmp(contact->szText,text)) {
				if(contact->type==CLCIT_GROUP && !_tcschr(text,'\\')) {
					TCHAR szFullName[256];
					if(contact->group->parent && contact->group->parent->parent)
						mir_sntprintf(szFullName,SIZEOF(szFullName),_T("%s\\%s"),pcli->pfnGetGroupName(contact->group->parent->groupId,NULL),text);
					else lstrcpyn(szFullName,text,SIZEOF(szFullName));
					pcli->pfnRenameGroup(contact->groupId,szFullName);
				}
				else if(contact->type==CLCIT_CONTACT) {
					TCHAR *otherName = pcli->pfnGetContactDisplayName(contact->hContact,GCDNF_NOMYHANDLE);
					pcli->pfnInvalidateDisplayNameCacheEntry(contact->hContact);
					if(text[0]=='\0') {
						DBDeleteContactSetting(contact->hContact,"CList","MyHandle");
					}
					else {
						if(!lstrcmp(otherName,text)) DBDeleteContactSetting(contact->hContact,"CList","MyHandle");
						else DBWriteContactSettingTString(contact->hContact,"CList","MyHandle",text);
					}
					if (otherName) mir_free(otherName);
				}
			}
		}
	}
	DestroyWindow(hwndEdit);
}

void DeleteFromContactList(HWND hwnd,struct ClcData *dat)
{
	struct ClcContact *contact;
	if(dat->selection==-1) return;
	dat->szQuickSearch[0]=0;
	if(GetRowByIndex(dat,dat->selection,&contact,NULL)==-1) return;
	switch (contact->type) {
		case CLCIT_GROUP: CallService(MS_CLIST_GROUPDELETE,(WPARAM)(HANDLE)contact->groupId,0); break;
		case CLCIT_CONTACT: 
			CallService("CList/DeleteContactCommand",(WPARAM)(HANDLE)
			contact->hContact,(LPARAM)hwnd); break;
	}
}

static WNDPROC OldRenameEditWndProc;
static LRESULT CALLBACK RenameEditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) {
		case WM_KEYDOWN:
			switch(wParam) {
				case VK_RETURN:
					EndRename(GetParent(hwnd),(struct ClcData*)GetWindowLong(GetParent(hwnd),0),1);
					return 0;
				case VK_ESCAPE:
					EndRename(GetParent(hwnd),(struct ClcData*)GetWindowLong(GetParent(hwnd),0),0);
					return 0;
			}
			break;
		case WM_GETDLGCODE:
			if(lParam) {
				MSG *msg=(MSG*)lParam;
				if(msg->message==WM_KEYDOWN && msg->wParam==VK_TAB) return 0;
				if(msg->message==WM_CHAR && msg->wParam=='\t') return 0;
			}
			return DLGC_WANTMESSAGE;
		case WM_KILLFOCUS:
			EndRename(GetParent(hwnd),(struct ClcData*)GetWindowLong(GetParent(hwnd),0),1);
			return 0;
	}
	return CallWindowProc(OldRenameEditWndProc,hwnd,msg,wParam,lParam);
}

void BeginRenameSelection(HWND hwnd,struct ClcData *dat)
{
	struct ClcContact *contact;
	struct ClcGroup *group;
	int indent,x,y,subident;
	RECT clRect;

	KillTimer(hwnd,TIMERID_RENAME);
	ReleaseCapture();
	dat->iHotTrack=-1;
	dat->selection=GetRowByIndex(dat,dat->selection,&contact,&group);
	if(dat->selection==-1) return;
	if(contact->type!=CLCIT_CONTACT && contact->type!=CLCIT_GROUP) return;
	
	if (contact->type==CLCIT_CONTACT && contact->isSubcontact)
		subident=dat->rowHeight/2;
	else 
		subident=0;
	
	for(indent=0;group->parent;indent++,group=group->parent);
	GetClientRect(hwnd,&clRect);
	x=indent*dat->groupIndent+dat->iconXSpace-2+subident;
	y=dat->selection*dat->rowHeight-dat->yScroll;
	dat->hwndRenameEdit=CreateWindow(_T("EDIT"),contact->szText,WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,x,y,clRect.right-x,dat->rowHeight,hwnd,NULL,g_hInst,NULL);
	OldRenameEditWndProc=(WNDPROC)SetWindowLong(dat->hwndRenameEdit,GWL_WNDPROC,(LONG)RenameEditSubclassProc);
	SendMessage(dat->hwndRenameEdit,WM_SETFONT,(WPARAM)(contact->type==CLCIT_GROUP?dat->fontInfo[FONTID_GROUPS].hFont:dat->fontInfo[FONTID_CONTACTS].hFont),0);
	SendMessage(dat->hwndRenameEdit,EM_SETMARGINS,EC_LEFTMARGIN|EC_RIGHTMARGIN|EC_USEFONTINFO,0);
	SendMessage(dat->hwndRenameEdit,EM_SETSEL,0,(LPARAM)(-1));
	ShowWindow(dat->hwndRenameEdit,SW_SHOW);
	SetFocus(dat->hwndRenameEdit);
}

int GetDropTargetInformation(HWND hwnd,struct ClcData *dat,POINT pt)
{
	RECT clRect;
	int hit;
	struct ClcContact *contact,*movecontact;
	struct ClcGroup *group,*movegroup;
	DWORD hitFlags;

	GetClientRect(hwnd,&clRect);
	dat->selection=dat->iDragItem;
	dat->iInsertionMark=-1;
	if(!PtInRect(&clRect,pt)) return DROPTARGET_OUTSIDE;

	hit=HitTest(hwnd,dat,pt.x,pt.y,&contact,&group,&hitFlags);
	GetRowByIndex(dat,dat->iDragItem,&movecontact,&movegroup);
	if(hit==dat->iDragItem) return DROPTARGET_ONSELF;
	if(hit==-1 || hitFlags&CLCHT_ONITEMEXTRA) return DROPTARGET_ONNOTHING;

	if(movecontact->type==CLCIT_GROUP) {
		struct ClcContact *bottomcontact=NULL,*topcontact=NULL;
		struct ClcGroup *topgroup=NULL;
		int topItem=-1,bottomItem;
		int ok=0;
		if(pt.y+dat->yScroll<hit*dat->rowHeight+dat->insertionMarkHitHeight) {
			//could be insertion mark (above)
			topItem=hit-1; bottomItem=hit;
			bottomcontact=contact;
			topItem=GetRowByIndex(dat,topItem,&topcontact,&topgroup);
			ok=1;
		}
		if(pt.y+dat->yScroll>=(hit+1)*dat->rowHeight-dat->insertionMarkHitHeight) {
			//could be insertion mark (below)
			topItem=hit; bottomItem=hit+1;
			topcontact=contact; topgroup=group;
			bottomItem=GetRowByIndex(dat,bottomItem,&bottomcontact,NULL);
			ok=1;
		}
		if(ok) {
			ok=0;
			if(bottomItem==-1 || bottomcontact->type!=CLCIT_GROUP) {	   //need to special-case moving to end
				if(topItem!=dat->iDragItem) {
					for(;topgroup;topgroup=topgroup->parent) {
						if(topgroup==movecontact->group) break;
						if(topgroup==movecontact->group->parent) {ok=1; break;}
					}
					if(ok) bottomItem=topItem+1;
				}
			}
			else if(bottomItem!=dat->iDragItem && bottomcontact->type==CLCIT_GROUP && bottomcontact->group->parent==movecontact->group->parent) {
				if(bottomcontact!=movecontact+1) ok=1;
			}
			if(ok) {
			    dat->iInsertionMark=bottomItem;
				dat->selection=-1;
				return DROPTARGET_INSERTION;
			}
		}
	}
	if(contact->type==CLCIT_GROUP) {
		if(dat->iInsertionMark==-1) {
			if(movecontact->type==CLCIT_GROUP) {	 //check not moving onto its own subgroup
				for(;group;group=group->parent) if(group==movecontact->group) return DROPTARGET_ONSELF;
			}
			dat->selection=hit;
			return DROPTARGET_ONGROUP;
		}
	}
	return DROPTARGET_ONCONTACT;
}

int ClcStatusToPf2(int status)
{
	switch(status) {
		case ID_STATUS_ONLINE: return PF2_ONLINE;
		case ID_STATUS_AWAY: return PF2_SHORTAWAY;
		case ID_STATUS_DND: return PF2_HEAVYDND;
		case ID_STATUS_NA: return PF2_LONGAWAY;
		case ID_STATUS_OCCUPIED: return PF2_LIGHTDND;
		case ID_STATUS_FREECHAT: return PF2_FREECHAT;
		case ID_STATUS_INVISIBLE: return PF2_INVISIBLE;
		case ID_STATUS_ONTHEPHONE: return PF2_ONTHEPHONE;
		case ID_STATUS_OUTTOLUNCH: return PF2_OUTTOLUNCH;
		case ID_STATUS_OFFLINE: return MODEF_OFFLINE;
	}
	return 0;
}

int IsHiddenMode(struct ClcData *dat,int status)
{
	return dat->offlineModes&ClcStatusToPf2(status);
}

void HideInfoTip(HWND hwnd,struct ClcData *dat)
{
	CLCINFOTIP it={0};

	if(dat->hInfoTipItem==NULL) return;
	it.isGroup=IsHContactGroup(dat->hInfoTipItem);
	it.hItem=(HANDLE)((unsigned)dat->hInfoTipItem&~HCONTACT_ISGROUP);
	it.cbSize=sizeof(it);
	dat->hInfoTipItem=NULL;
//	NotifyEventHooks(hHideInfoTipEvent,0,(LPARAM)&it);
}

void NotifyNewContact(HWND hwnd,HANDLE hContact)
{
	NMCLISTCONTROL nm;
	nm.hdr.code=CLN_NEWCONTACT;
	nm.hdr.hwndFrom=hwnd;
	nm.hdr.idFrom=GetDlgCtrlID(hwnd);
	nm.flags=0;
	nm.hItem=hContact;
	SendMessage(GetParent(hwnd),WM_NOTIFY,0,(LPARAM)&nm);
}

void LoadClcOptions(HWND hwnd,struct ClcData *dat)
{
	dat->style=GetWindowLong(hwnd,GWL_STYLE);
	dat->MetaIgnoreEmptyExtra=DBGetContactSettingByte(NULL,"CLC","MetaIgnoreEmptyExtra",1);
	dat->rowHeight=DBGetContactSettingByte(NULL,"CLC","RowHeight",CLCDEFAULT_ROWHEIGHT);
	{	int i;
		LOGFONT lf;
		SIZE fontSize;
		HDC hdc=GetDC(hwnd);
		HFONT holdfont;
		for(i=0;i<=FONTID_MAX;i++) {
			if(!dat->fontInfo[i].changed) DeleteObject(dat->fontInfo[i].hFont);
			GetFontSetting(i,&lf,&dat->fontInfo[i].colour);
			{
				LONG height;
				HDC hdc=GetDC(NULL);
				height=lf.lfHeight;
				lf.lfHeight=-MulDiv(lf.lfHeight, GetDeviceCaps(hdc, LOGPIXELSY), 72);
				ReleaseDC(NULL,hdc);				
				dat->fontInfo[i].hFont=CreateFontIndirect(&lf);
				lf.lfHeight=height;
			}
			dat->fontInfo[i].changed=0;
			holdfont=SelectObject(hdc,dat->fontInfo[i].hFont);
			GetTextExtentPoint32(hdc,_T("x"),1,&fontSize);
			dat->fontInfo[i].fontHeight=fontSize.cy;
			if(fontSize.cy>dat->rowHeight) dat->rowHeight=fontSize.cy;
			if(holdfont) SelectObject(hdc,holdfont);
		}
		
		ReleaseDC(hwnd,hdc);
	}
	dat->leftMargin=DBGetContactSettingByte(NULL,"CLC","LeftMargin",CLCDEFAULT_LEFTMARGIN);
	dat->exStyle=DBGetContactSettingDword(NULL,"CLC","ExStyle",GetDefaultExStyle());
	dat->scrollTime=DBGetContactSettingWord(NULL,"CLC","ScrollTime",CLCDEFAULT_SCROLLTIME);
	dat->groupIndent=DBGetContactSettingByte(NULL,"CLC","GroupIndent",CLCDEFAULT_GROUPINDENT);
	dat->gammaCorrection=DBGetContactSettingByte(NULL,"CLC","GammaCorrect",CLCDEFAULT_GAMMACORRECT);
	dat->showIdle=DBGetContactSettingByte(NULL,"CLC","ShowIdle",CLCDEFAULT_SHOWIDLE);
	dat->noVScrollbar=DBGetContactSettingByte(NULL,"CLC","NoVScrollBar",0);
	SendMessage(hwnd,INTM_SCROLLBARCHANGED,0,0);
//	ShowScrollBar(hwnd,SB_VERT,dat->noVScrollbar==1 ? FALSE : TRUE);
	if(!dat->bkChanged) {
		DBVARIANT dbv;
		dat->bkColour=DBGetContactSettingDword(NULL,"CLC","BkColour",CLCDEFAULT_BKCOLOUR);
		if(dat->hBmpBackground) {DeleteObject(dat->hBmpBackground); dat->hBmpBackground=NULL;}
		if(DBGetContactSettingByte(NULL,"CLC","UseBitmap",CLCDEFAULT_USEBITMAP)) {
			if(!DBGetContactSetting(NULL,"CLC","BkBitmap",&dbv)) {
				dat->hBmpBackground=(HBITMAP)CallService(MS_UTILS_LOADBITMAP,0,(LPARAM)dbv.pszVal);
				mir_free(dbv.pszVal);
			}
		}
		dat->backgroundBmpUse=DBGetContactSettingWord(NULL,"CLC","BkBmpUse",CLCDEFAULT_BKBMPUSE);
	}
	dat->greyoutFlags=DBGetContactSettingDword(NULL,"CLC","GreyoutFlags",CLCDEFAULT_GREYOUTFLAGS);
	dat->offlineModes=DBGetContactSettingDword(NULL,"CLC","OfflineModes",CLCDEFAULT_OFFLINEMODES);
	dat->selBkColour=DBGetContactSettingDword(NULL,"CLC","SelBkColour",CLCDEFAULT_SELBKCOLOUR);
	dat->selTextColour=DBGetContactSettingDword(NULL,"CLC","SelTextColour",CLCDEFAULT_SELTEXTCOLOUR);
	dat->hotTextColour=DBGetContactSettingDword(NULL,"CLC","HotTextColour",CLCDEFAULT_HOTTEXTCOLOUR);
	dat->quickSearchColour=DBGetContactSettingDword(NULL,"CLC","QuickSearchColour",CLCDEFAULT_QUICKSEARCHCOLOUR);
	{	NMHDR hdr;
		hdr.code=CLN_OPTIONSCHANGED;
		hdr.hwndFrom=hwnd;
		hdr.idFrom=GetDlgCtrlID(hwnd);
		SendMessage(GetParent(hwnd),WM_NOTIFY,0,(LPARAM)&hdr);
	}
	SendMessage(hwnd,WM_SIZE,0,0);
}

#define GSIF_HASMEMBERS   0x80000000
#define GSIF_ALLCHECKED   0x40000000
#define GSIF_INDEXMASK    0x3FFFFFFF
void RecalculateGroupCheckboxes(HWND hwnd,struct ClcData *dat)
{
	struct ClcGroup *group;
	int check;

	group=&dat->list;
	group->scanIndex=GSIF_ALLCHECKED;
	for(;;) {
		if((group->scanIndex&GSIF_INDEXMASK)==group->cl.count) {
			check=(group->scanIndex&(GSIF_HASMEMBERS|GSIF_ALLCHECKED))==(GSIF_HASMEMBERS|GSIF_ALLCHECKED);
			group=group->parent;
			if(group==NULL) break;
			if(check) group->cl.items[(group->scanIndex&GSIF_INDEXMASK)]->flags|=CONTACTF_CHECKED;
			else {
				group->cl.items[(group->scanIndex&GSIF_INDEXMASK)]->flags&=~CONTACTF_CHECKED;
				group->scanIndex&=~GSIF_ALLCHECKED;
			}
		}
		else if(group->cl.items[(group->scanIndex&GSIF_INDEXMASK)]->type==CLCIT_GROUP) {
			group=group->cl.items[(group->scanIndex&GSIF_INDEXMASK)]->group;
			group->scanIndex=GSIF_ALLCHECKED;
			continue;
		}
		else if(group->cl.items[(group->scanIndex&GSIF_INDEXMASK)]->type==CLCIT_CONTACT) {
			group->scanIndex|=GSIF_HASMEMBERS;
			if(!(group->cl.items[(group->scanIndex&GSIF_INDEXMASK)]->flags&CONTACTF_CHECKED))
				group->scanIndex&=~GSIF_ALLCHECKED;
		}
		group->scanIndex++;
}	}

void SetGroupChildCheckboxes(struct ClcGroup *group,int checked)
{
	int i;

	for(i=0;i<group->cl.count;i++) {
		if(group->cl.items[i]->type==CLCIT_GROUP) {
			SetGroupChildCheckboxes(group->cl.items[i]->group,checked);
			if(checked) group->cl.items[i]->flags|=CONTACTF_CHECKED;
			else group->cl.items[i]->flags&=~CONTACTF_CHECKED;
		}
		else if(group->cl.items[i]->type==CLCIT_CONTACT) {
			if(checked) group->cl.items[i]->flags|=CONTACTF_CHECKED;
			else group->cl.items[i]->flags&=~CONTACTF_CHECKED;
		}
	}
}

void InvalidateItem(HWND hwnd,struct ClcData *dat,int iItem)
{
	RECT rc;
	GetClientRect(hwnd,&rc);
	rc.top=iItem*dat->rowHeight-dat->yScroll;
	rc.bottom=rc.top+dat->rowHeight;
	InvalidateRect(hwnd,&rc,FALSE);
}