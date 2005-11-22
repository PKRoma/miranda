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

UNICODE done

*/
#include "commonheaders.h"
#include "richedit.h"

#ifndef SPI_GETDESKWALLPAPER
#define SPI_GETDESKWALLPAPER 115
#endif
//loads of stuff that didn't really fit anywhere else

extern HANDLE hHideInfoTipEvent;
extern void InvalidateDisplayNameCacheEntry(HANDLE hContact);
extern struct CluiData g_CluiData;
extern HWND hwndContactList;

extern struct ExtraCache *g_ExtraCache;
extern int g_nextExtraCacheEntry, g_maxExtraCacheEntry;
extern int g_isConnecting, during_sizing;

char * GetGroupCountsText(struct ClcData *dat, struct ClcContact *contact)
{
    static char szName[32];
    int onlineCount, totalCount;
    struct ClcGroup *group, *topgroup;

    if (contact->type != CLCIT_GROUP || !(dat->exStyle & CLS_EX_SHOWGROUPCOUNTS))
        return "";

    group = topgroup = contact->group;
    onlineCount = 0;
    totalCount = group->totalMembers;
    group->scanIndex = 0;
    for (; ;) {
        if (group->scanIndex == group->contactCount) {
            if (group == topgroup)
                break;            
            group = group->parent;
        } else if (group->contact[group->scanIndex].type == CLCIT_GROUP) {
            group = group->contact[group->scanIndex].group;
            group->scanIndex = 0;
            totalCount += group->totalMembers;
            continue;
        } else if (group->contact[group->scanIndex].type == CLCIT_CONTACT)
            if (group->contact[group->scanIndex].flags & CONTACTF_ONLINE)
                onlineCount++;
        group->scanIndex++;
    }
    if (onlineCount == 0 && dat->exStyle & CLS_EX_HIDECOUNTSWHENEMPTY)
        return "";
    _snprintf(szName, sizeof(szName), "(%u/%u)", onlineCount, totalCount);
    return szName;
}

/*
 * performs hit-testing for reversed (mirrored) contact rows when using RTL
 * shares all the init stuff with HitTest()
 */

int RTL_HitTest(HWND hwnd, struct ClcData *dat, int testx, int testy, struct ClcContact *hitcontact, DWORD *flags, int indent, int hit)
{
    RECT clRect;
    int right, checkboxWidth, cxSmIcon, i, width;
    DWORD style = GetWindowLong(hwnd, GWL_STYLE);
    SIZE textSize;
    HDC hdc;
    HFONT hFont;
    
    GetClientRect(hwnd, &clRect);
    right = clRect.right;
    
    // avatar check
    if(hitcontact->type == CLCIT_CONTACT && g_CluiData.dwFlags & CLUI_FRAME_AVATARS && hitcontact->ace != NULL && hitcontact->avatarLeft != -1) {
        if(testx < right - hitcontact->avatarLeft && testx > right - hitcontact->avatarLeft - g_CluiData.avatarSize) {
            if(flags)
                *flags |= CLCHT_ONAVATAR;
        }
    }
    if (testx > right - (dat->leftMargin + indent * dat->groupIndent)) {
        if (flags)
            *flags |= CLCHT_ONITEMINDENT;
        return hit;
    }
    checkboxWidth = 0;
    if (style & CLS_CHECKBOXES && hitcontact->type == CLCIT_CONTACT)
        checkboxWidth = dat->checkboxSize + 2;
    if (style & CLS_GROUPCHECKBOXES && hitcontact->type == CLCIT_GROUP)
        checkboxWidth = dat->checkboxSize + 2;
    if (hitcontact->type == CLCIT_INFO && hitcontact->flags & CLCIIF_CHECKBOX)
        checkboxWidth = dat->checkboxSize + 2;
    if (testx > right - (dat->leftMargin + indent * dat->groupIndent + checkboxWidth)) {
        if (flags)
            *flags |= CLCHT_ONITEMCHECK;
        return hit;
    }
    if (testx > right - (dat->leftMargin + indent * dat->groupIndent + checkboxWidth + dat->iconXSpace)) {
        if (flags)
            *flags |= CLCHT_ONITEMICON;
        return hit;
    }
    cxSmIcon = GetSystemMetrics(SM_CXSMICON);
    for (i = 0; i< dat->extraColumnsCount; i++) {
        if (hitcontact->iExtraImage[i] == 0xFF)
            continue;
        if (testx >= dat->extraColumnSpacing * (dat->extraColumnsCount - i) && testx < dat->extraColumnSpacing * (dat->extraColumnsCount - i) + cxSmIcon) {
            if (flags)
                *flags |= CLCHT_ONITEMEXTRA | (i << 24);
            return hit;
        }
    }
    if(hitcontact->extraCacheEntry >= 0 && hitcontact->extraCacheEntry < g_nextExtraCacheEntry && g_ExtraCache[hitcontact->extraCacheEntry].iExtraValid) {
        int rightOffset = hitcontact->extraIconRightBegin;
        int images_present = 0;

        for (i = 5; i >= 0; i--) {
            if (g_ExtraCache[hitcontact->extraCacheEntry].iExtraImage[i] == 0xFF)
                continue;
            if(!((1 << i) & g_CluiData.dwExtraImageMask))
                continue;
            images_present++;
            if (testx < right - (rightOffset - (g_CluiData.exIconScale + 2) * images_present) && testx > right - (rightOffset - (g_CluiData.exIconScale + 2) * images_present + (g_CluiData.exIconScale))) {
                if (flags)
                    *flags |= (CLCHT_ONITEMEXTRAEX | ((i + 1) << 24));
                return hit;
            }
        }
    }

    hdc = GetDC(hwnd);
    if (hitcontact->type == CLCIT_GROUP)
        hFont = SelectObject(hdc, dat->fontInfo[FONTID_GROUPS].hFont);
    else
        hFont = SelectObject(hdc, dat->fontInfo[FONTID_CONTACTS].hFont);
    GetTextExtentPoint32(hdc, hitcontact->szText, lstrlen(hitcontact->szText), &textSize);
    width = textSize.cx;
    if (hitcontact->type == CLCIT_GROUP) {
        char *szCounts;
        szCounts = GetGroupCountsText(dat, hitcontact);
        if (szCounts[0]) {
            GetTextExtentPoint32A(hdc, " ", 1, &textSize);
            width += textSize.cx;
            SelectObject(hdc, dat->fontInfo[FONTID_GROUPCOUNTS].hFont);
            GetTextExtentPoint32A(hdc, szCounts, lstrlenA(szCounts), &textSize);
            width += textSize.cx;
        }
    }
    SelectObject(hdc, hFont);
    ReleaseDC(hwnd, hdc);
    if (testx > right - (dat->leftMargin + indent * dat->groupIndent + checkboxWidth + dat->iconXSpace + width + 4 + (g_CluiData.dwFlags & CLUI_FRAME_AVATARS ? g_CluiData.avatarSize : 0))) {
        if (flags)
            *flags |= CLCHT_ONITEMLABEL;
        return hit;
    }
    if (g_CluiData.dwFlags & CLUI_FULLROWSELECT && !(GetKeyState(VK_SHIFT) & 0x8000) && testx < right - (dat->leftMargin + indent * dat->groupIndent + checkboxWidth + dat->iconXSpace + width + 4 + (g_CluiData.dwFlags & CLUI_FRAME_AVATARS ? g_CluiData.avatarSize : 0))) {
        if (flags)
            *flags |= CLCHT_ONITEMSPACE;
        return hit;
    }
    if (flags)
        *flags |= CLCHT_NOWHERE;
    return -1;
}

int HitTest(HWND hwnd, struct ClcData *dat, int testx, int testy, struct ClcContact **contact, struct ClcGroup **group, DWORD *flags)
{
    struct ClcContact *hitcontact;
    struct ClcGroup *hitgroup;
    int hit, indent, width, i, cxSmIcon;
    int checkboxWidth;
    SIZE textSize;
    HDC hdc;
    RECT clRect;
    HFONT hFont;
    DWORD style = GetWindowLong(hwnd, GWL_STYLE);
	BYTE mirror_mode = g_CluiData.bUseDCMirroring;

    if (flags)
        *flags = 0;
    GetClientRect(hwnd, &clRect);
    if (testx < 0 || testy < 0 || testy >= clRect.bottom || testx >= clRect.right) {
        if (flags) {
            if (testx < 0)
                *flags |= CLCHT_TOLEFT;
            else if (testx >= clRect.right)
                *flags |= CLCHT_TORIGHT;
            if (testy < 0)
                *flags |= CLCHT_ABOVE;
            else if (testy >= clRect.bottom)
                *flags |= CLCHT_BELOW;
        }
        return -1;
    }
    if (testx< dat->leftMargin) {
        if (flags)
            *flags |= CLCHT_INLEFTMARGIN | CLCHT_NOWHERE;
        return -1;
    }
   	hit = RowHeights_HitTest(dat, dat->yScroll + testy);
	if (hit != -1) 
		hit = GetRowByIndex(dat, hit, &hitcontact, &hitgroup);

    //hit = GetRowByIndex(dat, (testy + dat->yScroll) / (dat->rowHeight ? dat->rowHeight : 1), &hitcontact, &hitgroup);
    if (hit == -1) {
        if (flags)
            *flags |= CLCHT_NOWHERE | CLCHT_BELOWITEMS;
        return -1;
    }
    if (contact)
        *contact = hitcontact;
    if (group)
        *group = hitgroup;
    for (indent = 0; hitgroup->parent; indent++,hitgroup = hitgroup->parent) {
        ;
    }

    if(mirror_mode == 1 || (mirror_mode == 2 && ((hitcontact->type == CLCIT_CONTACT && g_ExtraCache[hitcontact->extraCacheEntry].dwCFlags & ECF_RTLNICK) ||
		(hitcontact->type == CLCIT_GROUP && hitcontact->isRtl))) || (mirror_mode == 3 && hitcontact->type == CLCIT_GROUP && hitcontact->isRtl))
        return RTL_HitTest(hwnd, dat, testx, testy, hitcontact, flags, indent, hit);
    
    // avatar check
    if(hitcontact->type == CLCIT_CONTACT && g_CluiData.dwFlags & CLUI_FRAME_AVATARS && hitcontact->ace != NULL && hitcontact->avatarLeft != -1) {
        if(testx >hitcontact->avatarLeft && testx < hitcontact->avatarLeft + g_CluiData.avatarSize) {
            if(flags)
                *flags |= CLCHT_ONAVATAR;
        }
    }
    if (testx< dat->leftMargin + indent * dat->groupIndent) {
        if (flags)
            *flags |= CLCHT_ONITEMINDENT;
        return hit;
    }
    checkboxWidth = 0;
    if (style & CLS_CHECKBOXES && hitcontact->type == CLCIT_CONTACT)
        checkboxWidth = dat->checkboxSize + 2;
    if (style & CLS_GROUPCHECKBOXES && hitcontact->type == CLCIT_GROUP)
        checkboxWidth = dat->checkboxSize + 2;
    if (hitcontact->type == CLCIT_INFO && hitcontact->flags & CLCIIF_CHECKBOX)
        checkboxWidth = dat->checkboxSize + 2;
    if (testx< dat->leftMargin + indent * dat->groupIndent + checkboxWidth) {
        if (flags)
            *flags |= CLCHT_ONITEMCHECK;
        return hit;
    }
    if (testx< dat->leftMargin + indent * dat->groupIndent + checkboxWidth + dat->iconXSpace) {
        if (flags)
            *flags |= CLCHT_ONITEMICON;
        return hit;
    }
    cxSmIcon = GetSystemMetrics(SM_CXSMICON);
    for (i = 0; i< dat->extraColumnsCount; i++) {
        if (hitcontact->iExtraImage[i] == 0xFF)
            continue;
        if (testx >= clRect.right - dat->extraColumnSpacing * (dat->extraColumnsCount - i) && testx< clRect.right - dat->extraColumnSpacing * (dat->extraColumnsCount - i) + cxSmIcon) {
            if (flags)
                *flags |= CLCHT_ONITEMEXTRA | (i << 24);
            return hit;
        }
    }
    if(hitcontact->extraCacheEntry >= 0 && hitcontact->extraCacheEntry < g_nextExtraCacheEntry && g_ExtraCache[hitcontact->extraCacheEntry].iExtraValid) {
        //int rightOffset = clRect.right;
        int rightOffset = hitcontact->extraIconRightBegin;
        int images_present = 0;

        for (i = 5; i >= 0; i--) {
            if (g_ExtraCache[hitcontact->extraCacheEntry].iExtraImage[i] == 0xFF)
                continue;
            if(!((1 << i) & g_CluiData.dwExtraImageMask))
                continue;
            images_present++;
            if (testx > (rightOffset - (g_CluiData.exIconScale + 2) * images_present) && testx < (rightOffset - (g_CluiData.exIconScale + 2) * images_present + (g_CluiData.exIconScale))) {
                if (flags)
                    *flags |= (CLCHT_ONITEMEXTRAEX | ((i + 1) << 24));
                return hit;
            }
        }
    }
    hdc = GetDC(hwnd);
    if (hitcontact->type == CLCIT_GROUP)
        hFont = SelectObject(hdc, dat->fontInfo[FONTID_GROUPS].hFont);
    else
        hFont = SelectObject(hdc, dat->fontInfo[FONTID_CONTACTS].hFont);
    GetTextExtentPoint32(hdc, hitcontact->szText, lstrlen(hitcontact->szText), &textSize);
    width = textSize.cx;
    if (hitcontact->type == CLCIT_GROUP) {
        char *szCounts;
        szCounts = GetGroupCountsText(dat, hitcontact);
        if (szCounts[0]) {
            GetTextExtentPoint32A(hdc, " ", 1, &textSize);
            width += textSize.cx;
            SelectObject(hdc, dat->fontInfo[FONTID_GROUPCOUNTS].hFont);
            GetTextExtentPoint32A(hdc, szCounts, lstrlenA(szCounts), &textSize);
            width += textSize.cx;
        }
    }
    SelectObject(hdc, hFont);
    ReleaseDC(hwnd, hdc);
    if (g_CluiData.dwFlags & CLUI_FULLROWSELECT && !(GetKeyState(VK_SHIFT) & 0x8000) && testx > dat->leftMargin + indent * dat->groupIndent + checkboxWidth + dat->iconXSpace + width + 4 + (g_CluiData.dwFlags & CLUI_FRAME_AVATARS ? g_CluiData.avatarSize : 0)) {
        if (flags)
            *flags |= CLCHT_ONITEMSPACE;
        return hit;
    }
    if (testx< dat->leftMargin + indent * dat->groupIndent + checkboxWidth + dat->iconXSpace + width + 4 + (g_CluiData.dwFlags & CLUI_FRAME_AVATARS ? g_CluiData.avatarSize : 0)) {
        if (flags)
            *flags |= CLCHT_ONITEMLABEL;
        return hit;
    }
    if (flags)
        *flags |= CLCHT_NOWHERE;
    return -1;
}

void UpdateIfLocked(HWND hwnd, struct ClcData *dat)
{
    if(g_isConnecting && dat->forceScroll) {
        dat->forcePaint = TRUE;
        UpdateWindow(hwnd);
    }
}

void ScrollTo(HWND hwnd, struct ClcData *dat, int desty, int noSmooth)
{
    DWORD startTick, nowTick;
    int oldy = dat->yScroll;
    RECT clRect, rcInvalidate;
    int maxy, previousy;

    if (dat->iHotTrack != -1 && dat->yScroll != desty) {
        InvalidateItem(hwnd, dat, dat->iHotTrack);
        UpdateIfLocked(hwnd, dat);
        dat->iHotTrack = -1;
        ReleaseCapture();
    }
    GetClientRect(hwnd, &clRect);
    rcInvalidate = clRect;

    //maxy = dat->rowHeight * GetGroupContentsCount(&dat->list, 1) - clRect.bottom;
   	maxy=RowHeights_GetTotalHeight(dat)-clRect.bottom;
    if (desty > maxy)
        desty = maxy;
    if (desty < 0)
        desty = 0;
    if (abs(desty - dat->yScroll) < 4)
        noSmooth = 1;
    if (!noSmooth && dat->exStyle & CLS_EX_NOSMOOTHSCROLLING)
        noSmooth = 1;
    previousy = dat->yScroll;
    if (!noSmooth) {
        startTick = GetTickCount();
        for (; ;) {
            nowTick = GetTickCount();
            if (nowTick >= startTick + dat->scrollTime)
                break;
            dat->yScroll = oldy + (desty - oldy) * (int) (nowTick - startTick) / dat->scrollTime;
            if (dat->backgroundBmpUse & CLBF_SCROLL || dat->hBmpBackground == NULL)
                ScrollWindowEx(hwnd, 0, previousy - dat->yScroll, NULL, NULL, NULL, NULL, SW_INVALIDATE);
            else
                InvalidateRect(hwnd, NULL, FALSE);
            previousy = dat->yScroll;
            SetScrollPos(hwnd, SB_VERT, dat->yScroll, TRUE);
            dat->forcePaint = TRUE;
            UpdateWindow(hwnd);
        }
    }
    dat->yScroll = desty;
    if (dat->backgroundBmpUse & CLBF_SCROLL || dat->hBmpBackground == NULL) {
        if(!noSmooth) {
            ScrollWindowEx(hwnd, 0, previousy - dat->yScroll, NULL, NULL, NULL, NULL, SW_INVALIDATE);
            UpdateIfLocked(hwnd, dat);
        }
        else {
            InvalidateRect(hwnd, NULL, FALSE);
            UpdateIfLocked(hwnd, dat);
        }
    }
    else {
        InvalidateRect(hwnd, NULL, FALSE);
        UpdateIfLocked(hwnd, dat);
    }

    SetScrollPos(hwnd, SB_VERT, dat->yScroll, TRUE);
    dat->forceScroll = 0;
}

void EnsureVisible(HWND hwnd,struct ClcData *dat,int iItem,int partialOk)
{
	int itemy,newy;
	int moved=0;
	RECT clRect;

	GetClientRect(hwnd,&clRect);
	itemy = RowHeights_GetItemTopY(dat,iItem);
	if(partialOk) {
		if(itemy+dat->row_heights[iItem]-1<dat->yScroll) { 
            newy=itemy; 
            moved=1;
        }
		else if(itemy>=dat->yScroll+clRect.bottom) {
            newy=itemy-clRect.bottom+dat->row_heights[iItem]; 
            moved=1;
        }
	}
	else {
		if(itemy<dat->yScroll) {
            newy=itemy; 
            moved=1;
        }
		else if(itemy>=dat->yScroll+clRect.bottom-dat->row_heights[iItem]) {
            newy=itemy-clRect.bottom+dat->row_heights[iItem]; 
            moved=1;
        }
	}
	if(moved)
		ScrollTo(hwnd,dat,newy,0);
}

void RecalcScrollBar(HWND hwnd, struct ClcData *dat)
{
    SCROLLINFO si = {0};
    RECT clRect;
    NMCLISTCONTROL nm;

	RowHeights_CalcRowHeights(dat, hwnd);
    
    GetClientRect(hwnd, &clRect);
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    si.nMin = 0;
   	si.nMax=RowHeights_GetTotalHeight(dat)-1;
    si.nPage = clRect.bottom;
    si.nPos = dat->yScroll;

    if (GetWindowLong(hwnd, GWL_STYLE) & CLS_CONTACTLIST) {
        if (dat->noVScrollbar == 0)
            SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
    } else
        SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
    ScrollTo(hwnd, dat, dat->yScroll, 1);
    nm.hdr.code = CLN_LISTSIZECHANGE;
    nm.hdr.hwndFrom = hwnd;
    nm.hdr.idFrom = GetDlgCtrlID(hwnd);
    nm.pt.y = si.nMax;
    if(!during_sizing)
        SendMessage(GetParent(hwnd), WM_NOTIFY, 0, (LPARAM) &nm);
}

void SetGroupExpand(HWND hwnd,struct ClcData *dat,struct ClcGroup *group,int newState)
{
	int contentCount;
	int groupy;
	int newy;
	int posy;
	RECT clRect;
	NMCLISTCONTROL nm;

	if(newState==-1) 
        group->expanded^=1;
	else {
		if(group->expanded==(newState!=0)) return;
		group->expanded=newState!=0;
	}
	InvalidateRect(hwnd,NULL,FALSE);

	if (group->expanded)
		contentCount=GetGroupContentsCount(group,1);
	else
		contentCount=0;

	groupy=GetRowsPriorTo(&dat->list,group,-1);
	if(dat->selection>groupy && dat->selection<groupy+contentCount) dat->selection=groupy;
	RecalcScrollBar(hwnd,dat);

	GetClientRect(hwnd,&clRect);
	newy=dat->yScroll;
	posy = RowHeights_GetItemBottomY(dat, groupy+contentCount);
	if(posy>=newy+clRect.bottom)
		newy=posy-clRect.bottom;
	posy = RowHeights_GetItemTopY(dat, groupy);
	if(newy>posy) newy=posy;
	ScrollTo(hwnd,dat,newy,0);

	nm.hdr.code=CLN_EXPANDED;
	nm.hdr.hwndFrom=hwnd;
	nm.hdr.idFrom=GetDlgCtrlID(hwnd);
	nm.hItem=(HANDLE)group->groupId;
	nm.action=group->expanded;
	SendMessage(GetParent(hwnd),WM_NOTIFY,0,(LPARAM)&nm);
}

void DoSelectionDefaultAction(HWND hwnd, struct ClcData *dat)
{
    struct ClcContact *contact;

    if (dat->selection == -1)
        return;
    dat->szQuickSearch[0] = 0;
    if (GetRowByIndex(dat, dat->selection, &contact, NULL) == -1)
        return;
    if (contact->type == CLCIT_GROUP)
        SetGroupExpand(hwnd, dat, contact->group, -1);
    if (contact->type == CLCIT_CONTACT)
        CallService(MS_CLIST_CONTACTDOUBLECLICKED, (WPARAM) contact->hContact, 0);
}

int FindRowByText(HWND hwnd, struct ClcData *dat, const TCHAR *text, int prefixOk)
{
    struct ClcGroup *group = &dat->list;
    int testlen = lstrlen(text);

    group->scanIndex = 0;
    for (; ;) {
        if (group->scanIndex == group->contactCount) {
            group = group->parent;
            if (group == NULL)
                break;
            group->scanIndex++;
            continue;
        }
        if (group->contact[group->scanIndex].type != CLCIT_DIVIDER) {
            if ((prefixOk && !_tcsnicmp(text, group->contact[group->scanIndex].szText, testlen)) || (!prefixOk && !lstrcmpi(text, group->contact[group->scanIndex].szText))) {
                struct ClcGroup *contactGroup = group;
                int contactScanIndex = group->scanIndex;
                for (; group; group = group->parent) {
                    SetGroupExpand(hwnd, dat, group, 1);
                }
                return GetRowsPriorTo(&dat->list, contactGroup, contactScanIndex);
            }
            if (group->contact[group->scanIndex].type == CLCIT_GROUP) {
                if (!(dat->exStyle & CLS_EX_QUICKSEARCHVISONLY) || group->contact[group->scanIndex].group->expanded) {
                    group = group->contact[group->scanIndex].group;
                    group->scanIndex = 0;
                    continue;
                }
            }
        }
        group->scanIndex++;
    }
    return -1;
}

void EndRename(HWND hwnd, struct ClcData *dat, int save)
{
    HWND hwndEdit = dat->hwndRenameEdit;

    if (dat->hwndRenameEdit == NULL)
        return;
    dat->hwndRenameEdit = NULL;
    if (save) {
        TCHAR text[120];
        struct ClcContact *contact;
        GetWindowText(hwndEdit, text, safe_sizeof(text));
        //MessageBox(0, text, L"foo", MB_RTLREADING | MB_RIGHT);
        if (GetRowByIndex(dat, dat->selection, &contact, NULL) != -1) {
            if (lstrcmp(contact->szText, text)) {
                if (contact->type == CLCIT_GROUP && !_tcsstr(text, _T("\\"))) {
                    TCHAR szFullName[256];
                    if (contact->group->parent && contact->group->parent->parent) {
                        _sntprintf(szFullName, safe_sizeof(szFullName), _T("%s\\%s"), GetGroupNameT(contact->group->parent->groupId, NULL), text);
                        szFullName[safe_sizeof(szFullName) - 1] = 0;
                    } else
                        lstrcpyn(szFullName, text, safe_sizeof(szFullName));
                    RenameGroupT(contact->groupId, szFullName);
                } else if (contact->type == CLCIT_CONTACT) {
                    TCHAR *otherName = GetContactDisplayNameW(contact->hContact, GCDNF_NOMYHANDLE);
                    InvalidateDisplayNameCacheEntry(contact->hContact);
                    if (text[0] == '\0') {
                        DBDeleteContactSetting(contact->hContact, "CList", "MyHandle");
                    } else {
                        if (!lstrcmp(otherName, text))
                            DBDeleteContactSetting(contact->hContact, "CList", "MyHandle");
                        else
                            DBWriteContactSettingTString(contact->hContact, "CList", "MyHandle", text);
                    }
                    if (otherName)
                        mir_free(otherName);
                }
            }
        }
    }
    DestroyWindow(hwndEdit);
}

void DeleteFromContactList(HWND hwnd, struct ClcData *dat)
{
    struct ClcContact *contact;
    if (dat->selection == -1)
        return;
    dat->szQuickSearch[0] = 0;
    if (GetRowByIndex(dat, dat->selection, &contact, NULL) == -1)
        return;
    switch (contact->type) {
        case CLCIT_GROUP:
            CallService(MS_CLIST_GROUPDELETE, (WPARAM) (HANDLE) contact->groupId, 0); break;
        case CLCIT_CONTACT:
            CallService("CList/DeleteContactCommand", (WPARAM) (HANDLE) contact->hContact, (LPARAM) hwnd); break;
    }
}

static WNDPROC OldRenameEditWndProc;
static LRESULT CALLBACK RenameEditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_KEYDOWN:
            switch (wParam) {
                case VK_RETURN:
                    EndRename(GetParent(hwnd), (struct ClcData *) GetWindowLong(GetParent(hwnd), 0), 1);
                    return 0;
                case VK_ESCAPE:
                    EndRename(GetParent(hwnd), (struct ClcData *) GetWindowLong(GetParent(hwnd), 0), 0);
                    return 0;
            }
            break;
        case WM_GETDLGCODE:
            if (lParam) {
                MSG *msg = (MSG *) lParam;
                if (msg->message == WM_KEYDOWN && msg->wParam == VK_TAB)
                    return 0;
                if (msg->message == WM_CHAR && msg->wParam == '\t')
                    return 0;
            }
            return DLGC_WANTMESSAGE;
        case WM_KILLFOCUS:
            EndRename(GetParent(hwnd), (struct ClcData *) GetWindowLong(GetParent(hwnd), 0), 1);
            return 0;
    }
    return CallWindowProc(OldRenameEditWndProc, hwnd, msg, wParam, lParam);
}

void BeginRenameSelection(HWND hwnd, struct ClcData *dat)
{
    struct ClcContact *contact;
    struct ClcGroup *group;
    int indent, x, y, h;
    RECT clRect;

    KillTimer(hwnd, TIMERID_RENAME);
    ReleaseCapture();
    dat->iHotTrack = -1;
    dat->selection = GetRowByIndex(dat, dat->selection, &contact, &group);
    if (dat->selection == -1)
        return;
    if (contact->type != CLCIT_CONTACT && contact->type != CLCIT_GROUP)
        return;
    for (indent = 0; group->parent; indent++,group = group->parent) {
        ;
    }
    GetClientRect(hwnd, &clRect);
    x = indent * dat->groupIndent + dat->iconXSpace - 2;
    //y = dat->selection * dat->rowHeight - dat->yScroll;
   	y=RowHeights_GetItemTopY(dat, dat->selection)-dat->yScroll;

    h=dat->row_heights[dat->selection];
    {
        int i;
        for (i=0; i<=FONTID_MAX; i++)
           if (h<dat->fontInfo[i].fontHeight+2) h=dat->fontInfo[i].fontHeight+2;
    }
#if defined(_UNICODE)
	dat->hwndRenameEdit = CreateWindowEx(0, _T("RichEdit20W"),contact->szText,WS_CHILD|WS_BORDER|ES_MULTILINE|ES_AUTOHSCROLL,x,y,clRect.right-x,h,hwnd,NULL,g_hInst,NULL);
    {
        if((contact->type == CLCIT_CONTACT && g_ExtraCache[contact->extraCacheEntry].dwCFlags & ECF_RTLNICK) || (contact->type == CLCIT_GROUP && contact->isRtl)) {
            PARAFORMAT2 pf2;
            ZeroMemory((void *)&pf2, sizeof(pf2));
            pf2.cbSize = sizeof(pf2);
            pf2.dwMask = PFM_RTLPARA;
            pf2.wEffects = PFE_RTLPARA;
            SetWindowText(dat->hwndRenameEdit, _T(""));
            SendMessage(dat->hwndRenameEdit, EM_SETPARAFORMAT, 0, (LPARAM)&pf2);
            SetWindowText(dat->hwndRenameEdit, contact->szText);
        }
    }
#else    
    dat->hwndRenameEdit = CreateWindow(_T("EDIT"),contact->szText,WS_CHILD|WS_BORDER|ES_MULTILINE|ES_AUTOHSCROLL,x,y,clRect.right-x,h,hwnd,NULL,g_hInst,NULL);
#endif    
    //dat->hwndRenameEdit = CreateWindow(_T("EDIT"), contact->szText, WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, x, y, clRect.right - x, dat->rowHeight, hwnd, NULL, g_hInst, NULL);
    OldRenameEditWndProc = (WNDPROC) SetWindowLong(dat->hwndRenameEdit, GWL_WNDPROC, (LONG) RenameEditSubclassProc);
    SendMessage(dat->hwndRenameEdit, WM_SETFONT, (WPARAM) (contact->type == CLCIT_GROUP ? dat->fontInfo[FONTID_GROUPS].hFont : dat->fontInfo[FONTID_CONTACTS].hFont), 0);
    SendMessage(dat->hwndRenameEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN | EC_USEFONTINFO, 0);
    SendMessage(dat->hwndRenameEdit, EM_SETSEL, 0, (LPARAM) (-1));
    ShowWindow(dat->hwndRenameEdit, SW_SHOW);
    SetFocus(dat->hwndRenameEdit);
}

int GetDropTargetInformation(HWND hwnd, struct ClcData *dat, POINT pt)
{
    RECT clRect;
    int hit;
    struct ClcContact *contact, *movecontact;
    struct ClcGroup *group, *movegroup;
    DWORD hitFlags;

    GetClientRect(hwnd, &clRect);
    dat->selection = dat->iDragItem;
    dat->iInsertionMark = -1;
    if (!PtInRect(&clRect, pt))
        return DROPTARGET_OUTSIDE;

    hit = HitTest(hwnd, dat, pt.x, pt.y, &contact, &group, &hitFlags);
    GetRowByIndex(dat, dat->iDragItem, &movecontact, &movegroup);
    if (hit == dat->iDragItem)
        return DROPTARGET_ONSELF;
    if (hit == -1 || hitFlags & CLCHT_ONITEMEXTRA)
        return DROPTARGET_ONNOTHING;

    if (movecontact->type == CLCIT_GROUP) {
        struct ClcContact *bottomcontact = NULL, *topcontact = NULL;
        struct ClcGroup *topgroup = NULL;
        int topItem = -1,bottomItem;
        int ok = 0;
        //if (pt.y + dat->yScroll< hit*dat->rowHeight + dat->insertionMarkHitHeight) {
		if(pt.y+dat->yScroll<RowHeights_GetItemTopY(dat,hit)+dat->insertionMarkHitHeight) {
    //could be insertion mark (above)
            topItem = hit - 1; bottomItem = hit;
            bottomcontact = contact;
            topItem = GetRowByIndex(dat, topItem, &topcontact, &topgroup);
            ok = 1;
        }
        //if (pt.y + dat->yScroll >= (hit + 1) * dat->rowHeight - dat->insertionMarkHitHeight) {
		if(pt.y+dat->yScroll>=RowHeights_GetItemTopY(dat,hit+1)-dat->insertionMarkHitHeight) {
    //could be insertion mark (below)
            topItem = hit; bottomItem = hit + 1;
            topcontact = contact; topgroup = group;
            bottomItem = GetRowByIndex(dat, bottomItem, &bottomcontact, NULL);
            ok = 1;
        }
        if (ok) {
            ok = 0;
            if (bottomItem == -1 || bottomcontact->type != CLCIT_GROUP) {
    //need to special-case moving to end
                if (topItem != dat->iDragItem) {
                    for (; topgroup; topgroup = topgroup->parent) {
                        if (topgroup == movecontact->group)
                            break;
                        if (topgroup == movecontact->group->parent) {
                            ok = 1; break;
                        }
                    }
                    if (ok)
                        bottomItem = topItem + 1;
                }
            } else if (bottomItem != dat->iDragItem && bottomcontact->type == CLCIT_GROUP && bottomcontact->group->parent == movecontact->group->parent) {
                if (bottomcontact != movecontact + 1)
                    ok = 1;
            }
            if (ok) {
                dat->iInsertionMark = bottomItem;
                dat->selection = -1;
                return DROPTARGET_INSERTION;
            }
        }
    }
    if (contact->type == CLCIT_GROUP) {
        if (dat->iInsertionMark == -1) {
            if (movecontact->type == CLCIT_GROUP) {
                //check not moving onto its own subgroup
                for (; group; group = group->parent) {
                    if (group == movecontact->group)
                        return DROPTARGET_ONSELF;
                }
            }
            dat->selection = hit;
            return DROPTARGET_ONGROUP;
        }
    }
    return DROPTARGET_ONCONTACT;
}

int ClcStatusToPf2(int status)
{
    switch (status) {
        case ID_STATUS_ONLINE:
            return PF2_ONLINE;
        case ID_STATUS_AWAY:
            return PF2_SHORTAWAY;
        case ID_STATUS_DND:
            return PF2_HEAVYDND;
        case ID_STATUS_NA:
            return PF2_LONGAWAY;
        case ID_STATUS_OCCUPIED:
            return PF2_LIGHTDND;
        case ID_STATUS_FREECHAT:
            return PF2_FREECHAT;
        case ID_STATUS_INVISIBLE:
            return PF2_INVISIBLE;
        case ID_STATUS_ONTHEPHONE:
            return PF2_ONTHEPHONE;
        case ID_STATUS_OUTTOLUNCH:
            return PF2_OUTTOLUNCH;
        case ID_STATUS_OFFLINE:
            return MODEF_OFFLINE;
    }
    return 0;
}

int IsHiddenMode(struct ClcData *dat, int status)
{
    return dat->offlineModes & ClcStatusToPf2(status);
}

void HideInfoTip(HWND hwnd, struct ClcData *dat)
{
    CLCINFOTIP it = {
        0
    };

    if (dat->hInfoTipItem == NULL)
        return;
    it.isGroup = IsHContactGroup(dat->hInfoTipItem);
    it.hItem = (HANDLE) ((unsigned) dat->hInfoTipItem & ~HCONTACT_ISGROUP);
    it.cbSize = sizeof(it);
    dat->hInfoTipItem = NULL;
    NotifyEventHooks(hHideInfoTipEvent, 0, (LPARAM) &it);
}

void NotifyNewContact(HWND hwnd, HANDLE hContact)
{
    NMCLISTCONTROL nm;
    nm.hdr.code = CLN_NEWCONTACT;
    nm.hdr.hwndFrom = hwnd;
    nm.hdr.idFrom = GetDlgCtrlID(hwnd);
    nm.flags = 0;
    nm.hItem = hContact;
    SendMessage(GetParent(hwnd), WM_NOTIFY, 0, (LPARAM) &nm);
}

void LoadClcOptions(HWND hwnd, struct ClcData *dat)
{
    dat->min_row_heigh = (int)DBGetContactSettingByte(NULL,"CLC","RowHeight",CLCDEFAULT_ROWHEIGHT);
    dat->group_row_height = (int)DBGetContactSettingByte(NULL,"CLC","GRowHeight",CLCDEFAULT_ROWHEIGHT);
    dat->row_border = 0;
    {
        int i;
        LOGFONTA lf;
        SIZE fontSize;
        HDC hdc = GetDC(hwnd);
        HFONT hFont = NULL, hFont1;
        HFONT hFontOld;
        hFontOld = SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
        for (i = 0; i <= FONTID_MAX; i++) {
            if (!dat->fontInfo[i].changed)
                DeleteObject(dat->fontInfo[i].hFont);
            GetFontSetting(i, &lf, &dat->fontInfo[i].colour); 
            {
                LONG height;
                HDC hdc = GetDC(NULL);
                height = lf.lfHeight;
                lf.lfHeight = -MulDiv(lf.lfHeight, GetDeviceCaps(hdc, LOGPIXELSY), 72);
                ReleaseDC(NULL, hdc);                
                dat->fontInfo[i].hFont = CreateFontIndirectA(&lf);
                lf.lfHeight = height;
            }
            dat->fontInfo[i].changed = 0;
            SelectObject(hdc, dat->fontInfo[i].hFont);
            hFont1 = SelectObject(hdc, dat->fontInfo[i].hFont);
            if (hFont == NULL)
                hFont = hFont1;
            GetTextExtentPoint32A(hdc, "x", 1, &fontSize);
            dat->fontInfo[i].fontHeight = fontSize.cy;
        //if(fontSize.cy>dat->rowHeight) dat->rowHeight=fontSize.cy;
        }
        SelectObject(hdc, hFontOld);
        ReleaseDC(hwnd, hdc);
    }
    dat->leftMargin = DBGetContactSettingByte(NULL, "CLC", "LeftMargin", CLCDEFAULT_LEFTMARGIN);
    dat->rightMargin = DBGetContactSettingByte(NULL, "CLC", "RightMargin", CLCDEFAULT_LEFTMARGIN);
    dat->exStyle = DBGetContactSettingDword(NULL, "CLC", "ExStyle", GetDefaultExStyle());
    dat->scrollTime = DBGetContactSettingWord(NULL, "CLC", "ScrollTime", CLCDEFAULT_SCROLLTIME);
    dat->groupIndent = DBGetContactSettingByte(NULL, "CLC", "GroupIndent", CLCDEFAULT_GROUPINDENT);
    dat->gammaCorrection = DBGetContactSettingByte(NULL, "CLC", "GammaCorrect", CLCDEFAULT_GAMMACORRECT);
    dat->showIdle = DBGetContactSettingByte(NULL, "CLC", "ShowIdle", CLCDEFAULT_SHOWIDLE);
    dat->noVScrollbar = DBGetContactSettingByte(NULL, "CLC", "NoVScrollBar", 0); 
    SendMessage(hwnd, INTM_SCROLLBARCHANGED, 0, 0);
    if (!dat->bkChanged) {
        DBVARIANT dbv;
        dat->bkColour = DBGetContactSettingDword(NULL, "CLC", "BkColour", CLCDEFAULT_BKCOLOUR);
		if(g_CluiData.hBrushCLCBk)
			DeleteObject(g_CluiData.hBrushCLCBk);
		g_CluiData.hBrushCLCBk = CreateSolidBrush(dat->bkColour);
        if (dat->hBmpBackground) {
            if(g_CluiData.hdcPic) {
                SelectObject(g_CluiData.hdcPic, g_CluiData.hbmPicOld);
                DeleteDC(g_CluiData.hdcPic);
                g_CluiData.hdcPic = 0;
                g_CluiData.hbmPicOld = 0;
            }
            DeleteObject(dat->hBmpBackground); dat->hBmpBackground = NULL;
        }
        if (DBGetContactSettingByte(NULL, "CLC", "UseBitmap", CLCDEFAULT_USEBITMAP)) {
            if (!DBGetContactSetting(NULL, "CLC", "BkBitmap", &dbv)) {
                dat->hBmpBackground = (HBITMAP) CallService(MS_UTILS_LOADBITMAP, 0, (LPARAM) dbv.pszVal);
                mir_free(dbv.pszVal);
            }
        }
        dat->backgroundBmpUse = DBGetContactSettingWord(NULL, "CLC", "BkBmpUse", CLCDEFAULT_BKBMPUSE);
        g_CluiData.bmpBackground = dat->hBmpBackground;
        if(g_CluiData.bmpBackground) {
            HDC hdcThis = GetDC(hwndContactList);
            GetObject(g_CluiData.bmpBackground, sizeof(g_CluiData.bminfoBg), &(g_CluiData.bminfoBg));
            g_CluiData.hdcPic = CreateCompatibleDC(hdcThis);
            g_CluiData.hbmPicOld = SelectObject(g_CluiData.hdcPic, g_CluiData.bmpBackground);
            ReleaseDC(hwndContactList, hdcThis);
        }
    }
    if (DBGetContactSettingByte(NULL, "CLCExt", "EXBK_FillWallpaper", 0)) {
        char wpbuf[MAX_PATH];
        if (dat->hBmpBackground) {
            DeleteObject(dat->hBmpBackground); dat->hBmpBackground = NULL;
        }
        SystemParametersInfoA(SPI_GETDESKWALLPAPER, MAX_PATH, wpbuf, 0);

        // we have a wallpaper string
        if (strlen(wpbuf) > 0) {
            dat->hBmpBackground = LoadImageA(NULL, wpbuf, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
        }
        g_CluiData.bmpBackground = dat->hBmpBackground;
        if(g_CluiData.bmpBackground) {
            HDC hdcThis = GetDC(hwndContactList);
            GetObject(g_CluiData.bmpBackground, sizeof(g_CluiData.bminfoBg), &(g_CluiData.bminfoBg));
            g_CluiData.hdcPic = CreateCompatibleDC(hdcThis);
            g_CluiData.hbmPicOld = SelectObject(g_CluiData.hdcPic, g_CluiData.bmpBackground);
            ReleaseDC(hwndContactList, hdcThis);
        }
    }

    dat->greyoutFlags = DBGetContactSettingDword(NULL, "CLC", "GreyoutFlags", CLCDEFAULT_GREYOUTFLAGS);
    dat->offlineModes = DBGetContactSettingDword(NULL, "CLC", "OfflineModes", CLCDEFAULT_OFFLINEMODES);
    //dat->selBkColour=DBGetContactSettingDword(NULL,"CLC","SelBkColour",CLCDEFAULT_SELBKCOLOUR);
    dat->selTextColour = DBGetContactSettingDword(NULL, "CLC", "SelTextColour", CLCDEFAULT_SELTEXTCOLOUR);
    dat->hotTextColour = DBGetContactSettingDword(NULL, "CLC", "HotTextColour", CLCDEFAULT_HOTTEXTCOLOUR);
    dat->quickSearchColour = DBGetContactSettingDword(NULL, "CLC", "QuickSearchColour", CLCDEFAULT_QUICKSEARCHCOLOUR);
    dat->useWindowsColours = DBGetContactSettingByte(NULL, "CLC", "UseWinColours", CLCDEFAULT_USEWINDOWSCOLOURS); {
        NMHDR hdr;
        hdr.code = CLN_OPTIONSCHANGED;
        hdr.hwndFrom = hwnd;
        hdr.idFrom = GetDlgCtrlID(hwnd);
        SendMessage(GetParent(hwnd), WM_NOTIFY, 0, (LPARAM) &hdr);
    }
    SendMessage(hwnd, WM_SIZE, 0, 0);
}

#define GSIF_HASMEMBERS   0x80000000
#define GSIF_ALLCHECKED   0x40000000
#define GSIF_INDEXMASK    0x3FFFFFFF
void RecalculateGroupCheckboxes(HWND hwnd, struct ClcData *dat)
{
    struct ClcGroup *group;
    int check;

    group = &dat->list;
    group->scanIndex = GSIF_ALLCHECKED;
    for (; ;) {
        if ((group->scanIndex & GSIF_INDEXMASK) == group->contactCount) {
            check = (group->scanIndex & (GSIF_HASMEMBERS | GSIF_ALLCHECKED)) == (GSIF_HASMEMBERS | GSIF_ALLCHECKED);
            group = group->parent;
            if (group == NULL)
                break;
            if (check)
                group->contact[(group->scanIndex & GSIF_INDEXMASK)].flags |= CONTACTF_CHECKED;
            else {
                group->contact[(group->scanIndex & GSIF_INDEXMASK)].flags &= ~CONTACTF_CHECKED;
                group->scanIndex &= ~GSIF_ALLCHECKED;
            }
        } else if (group->contact[(group->scanIndex & GSIF_INDEXMASK)].type == CLCIT_GROUP) {
            group = group->contact[(group->scanIndex & GSIF_INDEXMASK)].group;
            group->scanIndex = GSIF_ALLCHECKED;
            continue;
        } else if (group->contact[(group->scanIndex & GSIF_INDEXMASK)].type == CLCIT_CONTACT) {
            group->scanIndex |= GSIF_HASMEMBERS;
            if (!(group->contact[(group->scanIndex & GSIF_INDEXMASK)].flags & CONTACTF_CHECKED))
                group->scanIndex &= ~GSIF_ALLCHECKED;
        }
        group->scanIndex++;
    }
}

void SetGroupChildCheckboxes(struct ClcGroup *group, int checked)
{
    int i;

    for (i = 0; i< group->contactCount; i++) {
        if (group->contact[i].type == CLCIT_GROUP) {
            SetGroupChildCheckboxes(group->contact[i].group, checked);
            if (checked)
                group->contact[i].flags |= CONTACTF_CHECKED;
            else
                group->contact[i].flags &= ~CONTACTF_CHECKED;
        } else if (group->contact[i].type == CLCIT_CONTACT) {
            if (checked)
                group->contact[i].flags |= CONTACTF_CHECKED;
            else
                group->contact[i].flags &= ~CONTACTF_CHECKED;
        }
    }
}

void InvalidateItem(HWND hwnd, struct ClcData *dat, int iItem)
{
    RECT rc;

    if(iItem >= 0) {
        GetClientRect(hwnd, &rc);
        rc.top=RowHeights_GetItemTopY(dat,iItem)-dat->yScroll;
        rc.bottom=rc.top+dat->row_heights[iItem];
    //    rc.top = iItem * dat->rowHeight - dat->yScroll;
    //    rc.bottom = rc.top + dat->rowHeight;
        InvalidateRect(hwnd, &rc, FALSE);
    }
    //InvalidateRect(hwnd,NULL,FALSE); // workaround
}

BYTE MY_DBGetContactSettingByte(HANDLE hContact, char *szModule, char *szSetting, BYTE defaultval)
{
    return((BYTE)DBGetContactSettingByte(hContact, szModule, szSetting, defaultval));
}
